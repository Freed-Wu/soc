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

static void init_opt(opt_t *opt) {
  opt->tty = "/tmp/ttyS1";
  opt->weight = "/root/weight.bin";
  opt->quantization_coefficience = "/root/quantify.bin";
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

  int fd_dev = open(AXITX_DEV_PATH, O_RDWR);
  if (fd_dev == -1)
    err(errno, AXITX_DEV_PATH);

  struct network_acc_reg reg;
  reg.picture_addr = PICTURE_BASE_ADDR;
#define YUV_FILE "/root/test.yuv"
  if (pl_config(fd_dev, YUV_FILE, reg.picture_addr, &reg.picture_size) == -1)
    err(errno, YUV_FILE);
  if (pl_config(fd_dev, opt.weight, reg.weight_addr = WEIGHT_ADDR,
                &reg.weight_size) == -1)
    err(errno, "%s", opt.weight);
  if (pl_config(fd_dev, opt.quantization_coefficience,
                reg.quantify_addr = QUANTIFY_ADDR, &reg.quantify_size) == -1)
    err(errno, "%s", opt.quantization_coefficience);

  pl_run(fd_dev, &reg);

  void *trans_addr = NULL;
  if (pl_read(fd_dev, trans_addr, reg.trans_addr, reg.trans_size) == -1)
    err(errno, AXITX_DEV_PATH);
  void *entropy_addr = NULL;
  if (pl_read(fd_dev, entropy_addr, reg.entropy_addr, reg.entropy_size) == -1)
    err(errno, AXITX_DEV_PATH);

#define TRANS_FILE "/tmp/trans.bin"
  if (dump_mem(TRANS_FILE, trans_addr, reg.trans_size) == -1)
    err(errno, TRANS_FILE);
#define ENTROPY_FILE "/tmp/entropy.bin"
  if (dump_mem(ENTROPY_FILE, entropy_addr, reg.entropy_size) == -1)
    err(errno, ENTROPY_FILE);

  if (close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);

    // TODO: comment temporarily
#if 0
  int fd = open(p_opt->tty, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if (fd == -1)
    err(errno, "%s", p_opt->tty);
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
