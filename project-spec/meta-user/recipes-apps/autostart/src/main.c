#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <syslog.h>
#include <unistd.h>
// https://stackoverflow.com/a/48521433/16027269
#define termios asmtermios
#include <asm/termios.h>
#undef termios
#include <termios.h>

#include "axitangxi.h"
#include "coding.h"
#include "main.h"
#include "utils.h"

// 分别引用 算术编码器、比特IO（IO到数组）、GMM频率表

#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include "GmmTable.h"


// millisecond / frame
#define TIMEOUT 3
// 权重、因子、图片的地址
#define WEIGHT_ADDR 0x10000000
#define QUANTIFY_ADDR 0x10010000
#define PICTURE_BASE_ADDR 0x20000000

extern const uint8_t tp_header[4];
// status of program, not the status of every picture
status_t status;
// maximum of n_file_t
#define PICTURES_NUMBER_MAX 8
// picture status: unreceived -> received (processing) -> processed
// look up len to see if a picture have been processed
data_t bit_streams[PICTURES_NUMBER_MAX];
// look up len == total_len to see if a picture have been received
data_frame_info_t data_frame_infos[PICTURES_NUMBER_MAX];

static status_t get_status(n_file_t number) {
  if (bit_streams[number].len > 0)
    return TP_STATUS_PROCESSED;
  if (data_frame_infos[number].len == data_frame_infos[number].total_len &&
      data_frame_infos[number].total_len)
    return TP_STATUS_PROCESSING;
  return TP_STATUS_UNRECEIVED;
}

static void init_opt(opt_t *opt) {
  opt->tty = "/tmp/ttyS1";
  opt->weight = "/usr/share/autostart/weight.bin";
  opt->quantization_coefficience = "/usr/share/autostart/quantify.bin";
#ifdef DRY_RUN
  opt->dry_run = true;
#else
  opt->dry_run = false;
#endif
  opt->level = LOG_NOTICE;
}

static char shortopts[] = "hVvqdt:w:c:";
static struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"dry-run", no_argument, NULL, 'd'},
    {"tty", required_argument, NULL, 't'},
    {"weight", required_argument, NULL, 'w'},
    {"quantization_coefficience", required_argument, NULL, 'c'},
    {NULL, 0, NULL, 0}};

static int parse(int argc, char *argv[], opt_t *opt) {
  int c;
  init_opt(opt);
  while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (c) {
    case 'V':
      printf("%s " VERSION, argv[0]);
      return 2;
    case 'h':
      if (print_help(longopts, argv[0]) == 0)
        puts("");
      return 1;
    case 'd':
      opt->dry_run = true;
      break;
    case 'v':
      opt->level++;
      break;
    case 'q':
      opt->level--;
      break;
    case 't':
      opt->tty = optarg;
      break;
    case 'w':
      opt->weight = optarg;
      break;
    case 'c':
      opt->quantization_coefficience = optarg;
      break;
    default:
      return -1;
    }
  }
  setlogmask(LOG_UPTO(opt->level));
  return 0;
}

size_t process_data_frames(int fd, data_frame_t *input_data_frames,
                           n_frame_t n_frame, struct network_acc_reg reg,
                           uint8_t *addr) {
  // convert data frames to yuv
  ssize_t yuv_len = data_frame_to_data_len(input_data_frames, n_frame);
  data_t yuv[3] = {};
  yuv[0].addr = ps_mmap(fd, yuv_len);
  data_frames_to_data(input_data_frames, n_frame, yuv[0].addr);
  yuv[2].len = data_to_yuv420(yuv[0].addr, &yuv[1].addr, &yuv[2].addr, yuv_len);
  yuv[1].len = yuv[2].addr - yuv[1].addr;
  yuv[0].len = yuv[1].addr - yuv[0].addr;

  // neural encoding yuv
  data_t trans[3] = {};
  data_t entropy[3] = {};
  for (int k = 0; k < 3; k++) {
    if (pl_write(fd, yuv[k].addr, reg.picture_addr = PICTURE_BASE_ADDR,
                 reg.picture_size = yuv[k].len) == -1)
      err(errno, NULL);

    status |= TP_STATUS_NETWORK_ENCODING;
    pl_run(fd, &reg);
    status &= ~TP_STATUS_NETWORK_ENCODING;

    status |= TP_STATUS_NETWORK_ENCODING;
    // TODO: multithread
    pl_get(fd, &reg, (uint16_t *)trans[k].addr, (uint16_t *)entropy[k].addr);
    status &= ~TP_STATUS_NETWORK_ENCODING;

    trans[k].len = reg.trans_size;
    entropy[k].len = reg.entropy_size;
  }
  munmap(yuv[0].addr, yuv_len);

  // entropy encoding y' u' v'
  data_t data[3] = {};
  size_t len = 0;
  status |= TP_STATUS_ENTROPY_ENCODING;
  for (int k = 0; k < 3; k++) {
    size_t gmm_len = reg.entropy_size / 9;
    gmm_t *gmm = malloc(gmm_len * sizeof(gmm_t));
    entropy_to_gmm((uint16_t *)entropy[k].addr, gmm, gmm_len);

    // TODO: multithread
    gmm->freqs_resolution=1e6;
    uint16_t low_bound=0;
    uint16_t high_bound=65535;
    // 相对于原来，额外增加上下界
    data[k].len =coding(gmm,(uint16_t *)trans[k].addr,trans[k].len,data[k].addr,low_bound,high_bound)
        // coding(gmm, (uint16_t *)trans[k].addr, trans[k].len, data[k].addr);

    len += data[k].len;
  }
  status &= ~TP_STATUS_ENTROPY_ENCODING;
  // combine 3 channels to one
  if (addr == NULL)
    addr = malloc(len);
  uint8_t *p = addr;
  for (int k = 0; k < 3; k++) {
    memcpy(p, data[k].addr, data[k].len);
    p += data[k].len;
    free(data[k].addr);
  }
  return len;
}

int main(int argc, char *argv[]) {
  opt_t opt;
  openlog("slave0", LOG_CONS | LOG_PERROR, 0);
  int ret = parse(argc, argv, &opt);
  if (ret == -1)
    errx(EXIT_FAILURE, "parse failure!");
  else if (ret > 0)
    return EXIT_SUCCESS;

  // configure device
  int fd_dev;
  struct network_acc_reg reg;
  if (!opt.dry_run) {
    fd_dev = open(AXITX_DEV_PATH, O_RDWR);
    if (fd_dev == -1)
      err(errno, AXITX_DEV_PATH);
    pl_init(fd_dev, &reg, opt.weight, WEIGHT_ADDR,
            opt.quantization_coefficience, QUANTIFY_ADDR);
  }

  int fd = open(opt.tty, O_RDWR | O_NOCTTY), send_fd, recv_fd;
  if (fd == -1)
    err(errno, "%s", opt.tty);
  struct termios newattr, oldattr;
  tcgetattr(fd, &oldattr);
  newattr = oldattr;
  cfsetispeed(&newattr, B1152000);
  cfsetospeed(&newattr, B1152000);
  tcsetattr(fd, TCSANOW, &newattr);

  fd_to_epoll_fds(fd, &send_fd, &recv_fd);
  syslog(LOG_NOTICE, "%s: initial successfully%s", opt.tty,
         opt.dry_run ? " (dry run)" : "");
  frame_t input_frame, output_frame = {.address = TP_ADDRESS_SLAVE};
  ssize_t n;
  while (true) {
    do {
      n = receive_frame(recv_fd, &input_frame, -1);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);

    switch (input_frame.frame_type) {
    case TP_FRAME_TYPE_QUERY:
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.status = get_status(input_frame.n_file);
      output_frame.n_frame = data_frame_infos[input_frame.n_file].len;
      syslog(LOG_NOTICE, "response query");
      send_frame(send_fd, &output_frame, -1);
      break;

    case TP_FRAME_TYPE_SEND:
      // check
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        syslog(LOG_ERR, "picture number exceeds maximum: %d\n",
               PICTURES_NUMBER_MAX);
        break;
      }

      // response to send data
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.n_frame = input_frame.n_frame;
      syslog(LOG_NOTICE, "response to receive yuv %d with %d frames",
             output_frame.n_file, output_frame.n_frame);
      send_frame(send_fd, &output_frame, -1);

      // prepare to receive data frames, set data_frame_infos
      // every picture only malloc once!
      if (data_frame_infos[input_frame.n_file].addr == NULL) {
        data_frame_infos[input_frame.n_file].addr =
            calloc(input_frame.n_frame, sizeof(data_frame_t));
        if (data_frame_infos[input_frame.n_file].addr == NULL)
          err(errno, NULL);
      }
      data_frame_infos[input_frame.n_file].total_len = input_frame.n_frame;
      data_frame_t *input_data_frames =
          data_frame_infos[input_frame.n_file].addr;

      // receive data frames
      data_frame_t data_frame;
      for (n_frame_t _ = 0; _ < input_frame.n_frame; _++) {
        n = receive_data_frame(recv_fd, &data_frame, TIMEOUT);
        n_frame_t id = n_frame_to_id(data_frame.n_frame, input_frame.n_frame);
        if (n <= 0 || data_frame.n_file != input_frame.n_file ||
            id >= input_frame.n_frame || data_frame.data_len == 0 ||
            // if input_data_frames have this frame, skip it
            input_data_frames[id].data_len > 0 ||
            (data_frame.flag != TP_FLAG_1_YUV420 &&
             data_frame.flag != TP_FLAG_2_YUV420) ||
            memcmp(data_frame.header, tp_header, sizeof(tp_header)))
          continue;
        memcpy(&input_data_frames[id], &data_frame, sizeof(data_frame));
      }

      // statistic unreceived data frames
      n_frame_t sum = 0;
      for (n_frame_t i = 0; i < input_frame.n_frame; i++)
        if (input_data_frames[i].data_len == 0)
          sum++;
      data_frame_infos[input_frame.n_file].len =
          data_frame_infos[input_frame.n_file].total_len - sum;
      syslog(LOG_NOTICE, "%d incorrect frames need to be corrected", sum);

      // process data frames
      if (sum == 0)
        if (opt.dry_run) {
          bit_streams[input_frame.n_file].len =
              data_frame_to_data_len(input_data_frames, input_frame.n_frame);
          bit_streams[input_frame.n_file].addr =
              malloc(bit_streams[input_frame.n_file].len * sizeof(uint8_t));
          uint8_t *p = bit_streams[input_frame.n_file].addr;
          for (n_frame_t i = 0; i < input_frame.n_frame; i++) {
            memcpy(p, input_data_frames[i].data, input_data_frames[i].data_len);
            p += input_data_frames[i].data_len;
          }
        } else
          // TODO: multithread
          bit_streams[input_frame.n_file].len = process_data_frames(
              fd_dev, input_data_frames, input_frame.n_frame, reg,
              bit_streams[input_frame.n_file].addr);
      break;

    case TP_FRAME_TYPE_RECV:
      // check
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        syslog(LOG_ERR, "picture %d exceeds maximum: %d\n", input_frame.n_file,
               PICTURES_NUMBER_MAX);
        break;
      }
      // pictures haven't been encoded
      if (bit_streams[input_frame.n_file].len == 0) {
        syslog(LOG_ERR, "picture %d haven't been encoded\n",
               input_frame.n_file);
        break;
      }
      // alloc output_data_frames
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.n_frame =
          (bit_streams[input_frame.n_file].len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
      data_frame_t *output_data_frames =
          alloc_data_frames(output_frame.n_frame, output_frame.n_file,
                            bit_streams[input_frame.n_file].addr,
                            bit_streams[input_frame.n_file].len, TP_FLAG_DATA);
      if (output_data_frames == NULL) {
        syslog(LOG_ERR, "%s", strerror(errno));
        break;
      }

      // response to receive data
      syslog(LOG_NOTICE, "response to send data %d with %d frames",
             output_frame.n_file, output_frame.n_frame);
      send_frame(send_fd, &output_frame, -1);

      // send data
      for (n_frame_t i = 0; i < output_frame.n_frame; i++) {
        // cppcheck-suppress moduloofone
        if (i % SAFE_FRAMES == SAFE_FRAMES - 1)
          usleep(SAFE_TIME);
        send_data_frame(send_fd, &output_data_frames[i], -1);
      }
      syslog(LOG_NOTICE, "send data %d with %d frames", output_frame.n_file,
             output_frame.n_frame);

      // complete
      free(output_data_frames);
      break;

    default:
      syslog(LOG_ERR, "Unknown frame type: %d\n", input_frame.frame_type);
    }
  }

  for (n_file_t i = 0; i < PICTURES_NUMBER_MAX; i++) {
    free(bit_streams[i].addr);
    free(data_frame_infos[i].addr);
  }
  tcsetattr(fd, TCSANOW, &oldattr);
  if (close(fd) == -1)
    err(errno, "%s", opt.tty);
  if (close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);
  err(errno, NULL);
}
