#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "axitangxi.h"
#include "coding.h"
#include "config.h"
#include "main.h"
#include "transmission_protocol.h"
#include "utils.h"

// 权重、因子、图片的地址大小
#define WEIGHT_ADDR 0x10000000
#define QUANTIFY_ADDR 0x10010000
#define PICTURE_BASE_ADDR 0x20000000

static void init_opt(opt_t *opt) {
  opt->tty = "/tmp/ttyS1";
  opt->weight = "/usr/share/autostart/weight.bin";
  opt->quantization_coefficience = "/usr/share/autostart/quantify.bin";
}

static char shortopts[] = "t:w:q:hV";
static struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
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
      printf("%s " PROJECT_VERSION "\n"
             "Copyright (C) 2023\n"
             "Written by Wu Zhenyu <wuzhenyu@ustc.edu>\n",
             argv[0]);
      return 2;
    case 'h':
      if (print_help(longopts, argv[0]) == 0)
        return 1;
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

int main(int argc, char *argv[]) {
  opt_t opt;
  int status = parse(argc, argv, &opt);
  if (status == -1)
    errx(EXIT_FAILURE, "parse failure!");
  else if (status > 0)
    return EXIT_SUCCESS;

  // configure device
  int fd_dev = open(AXITX_DEV_PATH, O_RDWR);
  if (fd_dev == -1)
    err(errno, AXITX_DEV_PATH);
  struct network_acc_reg reg;
  pl_init(fd_dev, &reg, opt.weight, WEIGHT_ADDR, opt.quantization_coefficience,
          QUANTIFY_ADDR);
  int fd = open(opt.tty, O_RDWR | O_NOCTTY);
  if (fd == -1)
    err(errno, "%s", opt.tty);
  frame_t input_frame;
  ssize_t n;
  while (1) {
    do {
      n = receive_frame(fd, &input_frame);
    } while (n <= 0);
    frame_t output_frame = {
        .address = TP_ADDRESS_SLAVE,
        .frame_type = TP_FRAME_TYPE_NACK,
        .n_file = input_frame.n_file,
        .n_frame = input_frame.n_frame,
    };
    send_frame(fd, &output_frame);
    output_frame.frame_type = TP_FRAME_TYPE_ACK;
    send_frame(fd, &output_frame);
    data_frame_t *input_data_frames =
        malloc(input_frame.n_frame * sizeof(data_frame_t));
    if (input_data_frames == NULL)
      err(errno, NULL);
    for (int i = 0; i < input_frame.n_frame; i++)
      do {
        n = receive_data_frame(fd, &input_data_frames[i]);
      } while (n <= 0);
    ssize_t yuv_len =
        data_frame_to_data_len(input_data_frames, input_frame.n_frame);
    data_t yuv[3] = {};
    yuv[0].addr = ps_mmap(fd_dev, yuv_len);
    data_frames_to_data(input_data_frames, input_frame.n_frame, yuv[0].addr,
                        yuv_len);
    free(input_data_frames);
    yuv[2].len = data_to_yuv420(yuv[0].addr, yuv[1].addr, yuv[2].addr, yuv_len);
    yuv[1].len = yuv[2].addr - yuv[1].addr;
    yuv[0].len = yuv[1].addr - yuv[0].addr;

    for (int k = 0; k < 3; k++) {
      if (pl_write(fd_dev, yuv[k].addr, reg.picture_addr = PICTURE_BASE_ADDR,
                   reg.picture_size = yuv[k].len) == -1)
        err(errno, NULL);

      pl_run(fd_dev, &reg);
      uint16_t *trans_addr = NULL, *entropy_addr = NULL;
      pl_get(fd_dev, &reg, trans_addr, entropy_addr);

      gmm_t gmm;
      entropy_to_gmm(entropy_addr, &gmm);
      uint8_t *bits = NULL;
      size_t bits_len = coding(gmm, trans_addr, reg.trans_size, bits);
      output_frame.frame_type = TP_FRAME_TYPE_REQUEST;
      output_frame.n_frame = (bits_len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
      do {
        n = receive_frame(fd, &input_frame);
      } while (n <= 0);
      send_frame(fd, &output_frame);
      data_frame_t *output_data_frames =
          malloc(output_frame.n_frame * sizeof(data_frame_t));
      data_to_data_frames(bits, bits_len, output_data_frames,
                          output_frame.n_frame);
      for (int i = 0; i < output_frame.n_frame; i++)
        send_data_frame(fd, &output_data_frames[i]);
      while (1) {
        do {
          n = receive_frame(fd, &input_frame);
        } while (n <= 0);
        if (input_frame.frame_type == TP_FRAME_TYPE_NACK)
          send_data_frame(fd, &output_data_frames[input_frame.n_frame]);
        else if (input_frame.frame_type == TP_FRAME_TYPE_ACK) {
          break;
        }
      }
    }
  }

  if (close(fd) == -1)
    err(errno, "%s", opt.tty);
  if (close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);
  err(errno, NULL);
}
