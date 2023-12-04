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

// millisecond / frame
#define TIMEOUT 3
// 权重、因子、图片的地址
#define WEIGHT_ADDR 0x10000000
#define QUANTIFY_ADDR 0x10010000
#define PICTURE_BASE_ADDR 0x20000000

extern const uint8_t tp_header[4];
// request status will return it.
status_t status;
// maximum of n_file_t
#define PICTURES_NUMBER_MAX 8
data_t bit_streams[PICTURES_NUMBER_MAX];

static void init_opt(opt_t *opt) {
  opt->tty = "/tmp/ttyS1";
  opt->weight = "/usr/share/autostart/weight.bin";
  opt->quantization_coefficience = "/usr/share/autostart/quantify.bin";
  opt->dry_run = false;
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
    data[k].len =
        coding(gmm, (uint16_t *)trans[k].addr, trans[k].len, data[k].addr);

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
  syslog(LOG_NOTICE, "%s: initial successfully", opt.tty);
  frame_t input_frame, output_frame = {.address = TP_ADDRESS_SLAVE};
  ssize_t n;
  while (true) {
    do {
      n = receive_frame(recv_fd, &input_frame, -1);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);

    switch (input_frame.frame_type) {
    case TP_FRAME_TYPE_QUERY:
      output_frame.frame_type = input_frame.frame_type;
      output_frame.status = status;
      syslog(LOG_NOTICE, "response query");
      send_frame(send_fd, &output_frame, -1);
      break;

    case TP_FRAME_TYPE_SEND:
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        syslog(LOG_ERR, "picture number exceeds maximum: %d\n",
               PICTURES_NUMBER_MAX);
        break;
      }
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.n_frame = input_frame.n_frame;
      syslog(LOG_NOTICE, "response to receive yuv %d with %d frames",
             output_frame.n_file, output_frame.n_frame);
      send_frame(send_fd, &output_frame, -1);
      // receive data frames
      data_frame_t *input_data_frames =
          calloc(input_frame.n_frame, sizeof(data_frame_t));
      if (input_data_frames == NULL)
        err(errno, NULL);
      data_frame_t data_frame;
      for (int i = 0; i < input_frame.n_frame; i++) {
        n = receive_data_frame(recv_fd, &data_frame, TIMEOUT);
        if (n <= 0 || data_frame.n_file != input_frame.n_file ||
            data_frame.n_frame >= input_frame.n_frame ||
            data_frame.data_len == 0)
          continue;
        memcpy(&input_data_frames[data_frame.n_frame], &data_frame,
               sizeof(data_frame));
      }
      // request to resend data frames
      output_frame.frame_type = TP_FRAME_TYPE_NACK;
      for (int i = 0; i < input_frame.n_frame; i++) {
        if (input_data_frames[i].data_len > 0)
          continue;
        send_frame(send_fd, &output_frame, -1);
        do {
          n = receive_data_frame(recv_fd, &data_frame, -1);
        } while (n <= 0 || data_frame.n_file != input_frame.n_file ||
                 data_frame.n_frame != i || data_frame.data_len == 0);
        memcpy(&input_data_frames[data_frame.n_frame], &data_frame,
               sizeof(data_frame));
      }
      output_frame.frame_type = TP_FRAME_TYPE_ACK;
      send_frame(send_fd, &output_frame, -1);
      // TODO: multithread
      if (!opt.dry_run)
        bit_streams[input_frame.n_file].len =
            process_data_frames(fd_dev, input_data_frames, input_frame.n_frame,
                                reg, bit_streams[input_frame.n_file].addr);
      else {
        bit_streams[input_frame.n_file].len =
            data_frame_to_data_len(input_data_frames, input_frame.n_frame);
        bit_streams[input_frame.n_file].addr =
            malloc(bit_streams[input_frame.n_file].len * sizeof(uint8_t));
        for (n_frame_t i = 0; i < input_frame.n_frame; i++)
          memcpy(bit_streams[input_frame.n_file].addr,
                 input_data_frames[i].data, input_data_frames[i].data_len);
      }
      free(input_data_frames);
      break;

    case TP_FRAME_TYPE_RECV:
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        syslog(LOG_ERR, "picture %d exceeds maximum: %d\n", input_frame.n_file,
               PICTURES_NUMBER_MAX);
      error:
        output_frame.n_frame = 0;
        send_frame(fd, &output_frame, -1);
        break;
      }
      // haven't encoded pictures
      if (bit_streams[input_frame.n_file].len == 0) {
        syslog(LOG_ERR, "picture %d haven't been encoded\n",
               input_frame.n_file);
        goto error;
      }
      output_frame.n_frame =
          (bit_streams[input_frame.n_file].len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
      // send data frames
      data_frame_t *output_data_frames =
          alloc_data_frames(output_frame.n_frame, output_frame.n_file,
                            bit_streams[input_frame.n_file].addr,
                            bit_streams[input_frame.n_file].len);
      if (output_data_frames == NULL) {
        syslog(LOG_ERR, "%s", strerror(errno));
        break;
      }
      syslog(LOG_NOTICE, "response to send data %d with %d frames",
             output_frame.n_file, output_frame.n_frame);
      send_frame(send_fd, &output_frame, -1);
      for (int i = 0; i < output_frame.n_frame; i++)
        send_data_frame(send_fd, &output_data_frames[i], -1);
      bool cont = true;
      while (cont) {
        do {
          n = receive_frame(recv_fd, &input_frame, -1);
        } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);
        switch (input_frame.frame_type) {
        case TP_FRAME_TYPE_ACK:
          cont = false;
          free(output_data_frames);
          break;

        case TP_FRAME_TYPE_NACK:
          send_data_frame(send_fd, &output_data_frames[input_frame.n_frame],
                          -1);
          break;

        default:
          syslog(LOG_ERR, "Send ACK/NACK type frame, please!");
        }
        break;
      }
      break;

    case TP_FRAME_TYPE_ACK:
    case TP_FRAME_TYPE_NACK:
      syslog(LOG_ERR, "Send receive type frame firstly!");
      break;

    default:
      syslog(LOG_ERR, "Unknown frame type: %d\n", input_frame.frame_type);
    }
  }

  tcsetattr(fd, TCSANOW, &oldattr);
  if (close(fd) == -1)
    err(errno, "%s", opt.tty);
  if (close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);
  err(errno, NULL);
}
