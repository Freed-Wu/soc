#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
<<<<<<< HEAD
#include <sys/time.h>
    == == ==
    =
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
#include <unistd.h>
// https://stackoverflow.com/a/48521433/16027269
#define termios asmtermios
#include <asm/termios.h>
#undef termios
#include <termios.h>

#include "axitangxi.h"
        <<<<<<<HEAD
#include "config.h"
#include "main.h"
#include "utils.h"

// 增加算术编码涉及到的头文件： "编解码器"，"GMM和频率表"
#include "ArithmeticCoder.hpp"
#include "GmmTable.h"

// usecond / frame
#define TIMEOUT 3000
                   == == ==
               =

#include "main.h"
#include "utils.h"

// 增加算术编码涉及到的头文件： "编解码器"，"GMM和频率表"
#include "ArithmeticCoder.hpp"
#include "GmmTable.h"

// millisecond / frame
#define TIMEOUT 3
                   >>>>>>>
            be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
// 权重、因子、图片的地址
#define WEIGHT_ADDR 0x10000000
#define QUANTIFY_ADDR 0x10010000
#define PICTURE_BASE_ADDR 0x20000000

        < < < < < < < HEAD
=======
        extern const uint8_t tp_header[4];
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
// request status will return it.
status_t status;
// picture number
n_file_t number;
// maximum of n_file_t
#define PICTURES_NUMBER_MAX 8
data_t bit_streams[PICTURES_NUMBER_MAX];

static void init_opt(opt_t *opt) {
  opt->tty = "/tmp/ttyS1";
  opt->weight = "/usr/share/autostart/weight.bin";
  opt->quantization_coefficience = "/usr/share/autostart/quantify.bin";
  opt->dry_run = false;
}

static char shortopts[] = "t:w:q:hVd";
static struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"dry-run", no_argument, NULL, 'd'},
    {"tty", required_argument, NULL, 't'},
    {"weight", required_argument, NULL, 'w'},
    {"quantization_coefficience", required_argument, NULL, 'q'},
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
    case 't':
      opt->tty = optarg;
      break;
    case 'w':
      opt->weight = optarg;
      break;
    case 'q':
      opt->quantization_coefficience = optarg;
      break;
    default:
      return -1;
    }
  }
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

<<<<<<< HEAD
    gmm->freqs_resolution = 1e6;

    // TODO: multithread
    //*****新增上下边界，是要传入的参数****
    uint32_t low_bound = 0;
    uint32_t high_bound = (1 << 16) - 1;
    data[k].len = coding(gmm, (uint16_t *)trans[k].addr, trans[k].len,
                         data[k].addr, low_bound, high_bound);
=======
    // TODO: multithread
    // TODO: multithread
    //*****新增上下边界，是要传入的参数 ****
    uint32_t low_bound = 10000;
    uint32_t high_bound = 50000;
    gmm->freqs_resolution = 1e6; // 存辉师兄代码里默认的，

    data[k].len = coding(gmm, (uint16_t *)trans[k].addr, trans[k].len,
                         data[k].addr, low_bound, high_bound);
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1

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

<<<<<<< HEAD
static inline __suseconds_t tvdiff(struct timeval new_tv,
                                   struct timeval old_tv) {
  return (new_tv.tv_sec - old_tv.tv_sec) * 1000000 + new_tv.tv_usec -
         old_tv.tv_usec;
}

int main(int argc, char *argv[]) {
  opt_t opt;
=======
int main(int argc, char *argv[]) {
  opt_t opt;
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
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
<<<<<<< HEAD
=======
  struct termios newattr, oldattr;
  tcgetattr(fd, &oldattr);
  newattr = oldattr;
  cfsetispeed(&newattr, B1152000);
  cfsetospeed(&newattr, B1152000);
  tcsetattr(fd, TCSANOW, &newattr);

  fd_to_epoll_fds(fd, &send_fd, &recv_fd);
  print_log("%s: initial finished!", opt.tty);
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
  frame_t input_frame, output_frame = {.address = TP_ADDRESS_SLAVE};
  ssize_t n;
  while (true) {
    do {
<<<<<<< HEAD
      n = receive_frame(fd, &input_frame);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);

=======
      n = receive_frame(recv_fd, &input_frame, -1);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);

    print_log("receive a frame!");
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
    switch (input_frame.frame_type) {
    case TP_FRAME_TYPE_QUERY:
      output_frame.frame_type = input_frame.frame_type;
      output_frame.status = status;
<<<<<<< HEAD
      send_frame(fd, &output_frame);
=======
      send_frame(send_fd, &output_frame, -1);
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
      break;

    case TP_FRAME_TYPE_SEND:
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        fprintf(stderr, "picture number exceeds maximum: %d\n",
                PICTURES_NUMBER_MAX);
        break;
      }
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.n_frame = input_frame.n_frame;
<<<<<<< HEAD
      send_frame(fd, &output_frame);
=======
      send_frame(send_fd, &output_frame, -1);
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
      // receive data frames
      data_frame_t *input_data_frames =
          calloc(input_frame.n_frame, sizeof(data_frame_t));
      if (input_data_frames == NULL)
        err(errno, NULL);
      data_frame_t data_frame;
<<<<<<< HEAD
      struct timeval tv0, tv;
      gettimeofday(&tv0, NULL);
      tv = tv0;
      while (tvdiff(tv, tv0) < input_frame.n_frame * TIMEOUT) {
        do {
          n = receive_data_frame(fd, &data_frame);
        } while (n <= 0 || data_frame.n_file != input_frame.n_file ||
                 data_frame.n_frame >= input_frame.n_frame);
        memcpy(&input_data_frames[data_frame.n_frame], &data_frame,
               sizeof(data_frame));
        gettimeofday(&tv, NULL);
=======
      for (int i = 0; i < input_frame.n_frame; i++) {
        n = receive_data_frame(recv_fd, &data_frame, TIMEOUT);
        if (n <= 0 || data_frame.n_file != input_frame.n_file ||
            data_frame.n_frame >= input_frame.n_frame)
          continue;
        memcpy(&input_data_frames[data_frame.n_frame], &data_frame,
               sizeof(data_frame));
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
      }
      // request to resend data frames
      output_frame.frame_type = TP_FRAME_TYPE_NACK;
      for (int i = 0; i < input_frame.n_frame; i++) {
        if (input_data_frames[i].data_len == 0) {
<<<<<<< HEAD
          send_frame(fd, &output_frame);
          do {
            n = receive_data_frame(fd, &data_frame);
          } while (n <= 0 || data_frame.n_file != input_frame.n_file ||
                   data_frame.n_frame != i);
        }
      }
      output_frame.frame_type = TP_FRAME_TYPE_ACK;
      send_frame(fd, &output_frame);
      // TODO: multithread
      bit_streams[number].len =
          process_data_frames(fd_dev, input_data_frames, input_frame.n_frame,
                              reg, bit_streams[number].addr);
      number++;
      free(input_data_frames);
      break;

    case TP_FRAME_TYPE_RECV:
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        fprintf(stderr, "picture number exceeds maximum: %d\n",
                PICTURES_NUMBER_MAX);
        break;
      }
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_frame =
          (bit_streams[number].len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
      send_frame(fd, &output_frame);
      data_frame_t *output_data_frames =
          malloc(output_frame.n_frame * sizeof(data_frame_t));
      // data, data_len and check_sum should be set in data_to_data_frames()
      for (int i = 0; i < output_frame.n_frame; i++) {
        memcpy(output_data_frames[i].header, tp_header, sizeof(tp_header));
        output_data_frames[i].n_file = input_frame.n_file;
        output_data_frames[i].n_frame = i;
      }
      data_to_data_frames(bit_streams[number].addr, bit_streams[number].len,
                          output_data_frames);
      for (int i = 0; i < output_frame.n_frame; i++)
        send_data_frame(fd, &output_data_frames[i]);
      bool cont = true;
      while (cont) {
        do {
          n = receive_frame(fd, &input_frame);
        } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);
        switch (input_frame.frame_type) {
        case TP_FRAME_TYPE_ACK:
          cont = false;
          free(output_data_frames);
          break;

        case TP_FRAME_TYPE_NACK:
          send_data_frame(fd, &output_data_frames[input_frame.n_frame]);
          break;

        default:
          fputs("Send ACK/NACK type frame, please!", stderr);
=======
          send_frame(send_fd, &output_frame, -1);
          do {
            n = receive_data_frame(recv_fd, &data_frame, -1);
          } while (n <= 0 || data_frame.n_file != input_frame.n_file ||
                   data_frame.n_frame != i);
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
        }
        break;
      }
<<<<<<< HEAD
=======
      output_frame.frame_type = TP_FRAME_TYPE_ACK;
      send_frame(send_fd, &output_frame, -1);
      // TODO: multithread
      if (!opt.dry_run)
        bit_streams[number].len =
            process_data_frames(fd_dev, input_data_frames, input_frame.n_frame,
                                reg, bit_streams[number].addr);
      else {
        ssize_t yuv_len =
            data_frame_to_data_len(input_data_frames, input_frame.n_frame);
        bit_streams[number].addr = malloc(yuv_len * sizeof(uint8_t));
        for (n_frame_t i = 0; i < input_frame.n_frame; i++)
          memcpy(bit_streams[number].addr, input_data_frames[i].data,
                 input_data_frames[i].data_len);
      }
      number++;
      free(input_data_frames);
      break;

    case TP_FRAME_TYPE_RECV:
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        fprintf(stderr, "picture number exceeds maximum: %d\n",
                PICTURES_NUMBER_MAX);
        break;
      }
      // haven't encoded pictures
      if (bit_streams[number].len == 0)
        break;
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.n_frame =
          (bit_streams[number].len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
      // send data frames
      data_frame_t *output_data_frames =
          alloc_data_frames(output_frame.n_frame, output_frame.n_file,
                            bit_streams[number].addr, bit_streams[number].len);
      if (output_data_frames == NULL) {
        perror(NULL);
        break;
      }
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
          fputs("Send ACK/NACK type frame, please!", stderr);
        }
        break;
      }
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1

    case TP_FRAME_TYPE_ACK:
    case TP_FRAME_TYPE_NACK:
      fputs("Send receive type frame firstly!", stderr);
      break;

    default:
      fprintf(stderr, "Unknown frame type: %d\n", input_frame.frame_type);
    }
  }

  tcsetattr(fd, TCSANOW, &oldattr);
  if (close(fd) == -1)
    err(errno, "%s", opt.tty);
  if (close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);
  err(errno, NULL);
}
