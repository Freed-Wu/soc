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

#define YUV_FILE "/root/test.yuv"
  // TODO: use UART to get yuv files
  reg.picture_addr = PICTURE_BASE_ADDR;
  if (pl_config(fd_dev, YUV_FILE, reg.picture_addr, &reg.picture_size) == -1)
    err(errno, YUV_FILE);

  pl_run(fd_dev, &reg);
  void *trans_addr = NULL, *entropy_addr = NULL;
  pl_get(fd_dev, &reg, trans_addr, entropy_addr);

  gmm_t gmm;
  gmm.mean1 = ((double *)entropy_addr)[0];
  gmm.mean2 = ((double *)entropy_addr)[1];
  gmm.mean3 = ((double *)entropy_addr)[2];
  gmm.std1 = ((double *)entropy_addr)[3];
  gmm.std2 = ((double *)entropy_addr)[4];
  gmm.std3 = ((double *)entropy_addr)[5];
  gmm.prob1 = ((double *)entropy_addr)[3];
  gmm.prob2 = ((double *)entropy_addr)[4];
  gmm.prob3 = ((double *)entropy_addr)[5];
  coding(gmm, trans_addr, reg.trans_size);

  if (close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);

    // TODO: comment temporarily
#if 0
  frame_t input_frame, output_frame = default_frame;
  send_frame(fd, &output_frame);
  printf("slave: request data: raw image 0\n");
  for (;;) {
    wait_frame(fd, &input_frame, &output_frame);

    switch (input_frame.frame_type) {
      case TP_FRAME_TYPE_CONTROL:
        /*! TODO: not implemented yet
       *  \todo not implemented yet
       */
        break;
      case TP_FRAME_TYPE_REQUEST_DATA:
        output_frame.frame_type = TP_FRAME_TYPE_TRANSPORT_DATA;
        output_frame.n_file = input_frame.n_file;
        output_frame.data_len = TP_FRAME_DATA_LEN_MAX;
        /*! TODO: Create a thread to compress raw image and send a frame after
       * finishing \todo Create a thread to compress raw image and send a frame
       * after finishing
       */
        break;
      case TP_FRAME_TYPE_TRANSPORT_DATA:
        output_frame.frame_type = TP_FRAME_TYPE_TRANSPORT_DATA;
        printf("slave: receive data: raw image %d\n", input_frame.n_file);
        size_t len = strlen(p_opt->output_dir) + 4 + strlen("/.yuv");
        char *filename = malloc(len);
        if (sprintf(filename, "%s/%d.yuv", p_opt->output_dir,
                    input_frame.n_file) != len)
          err(errno, "%s", p_opt->output_dir);
        FILE *file = fopen(filename, "a");
        if (file == NULL)
          err(errno, "%s", filename);
        if (fwrite(input_frame.data, 1, input_frame.data_len, file) !=
          input_frame.data_len)
          err(errno, "%s", filename);
        if (fclose(file))
          err(errno, "%s", filename);
    }
    output_frame.n_frame++;
  }
#endif
}
