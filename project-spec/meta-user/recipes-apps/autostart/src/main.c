#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "axitangxi_ioctl.h"
#include "coding.h"
#include "config.h"
#include "main.h"

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

static int print_help(struct option *longopts, char *arg0) {
  unsigned int i = 0;
  struct option o = longopts[i];
  char *name;
  char *value;
  char *meta;
  char *str;
  puts(arg0);
  while (o.name != NULL) {
    name = malloc(strlen(o.name) + strlen("(|0)"));
    value = malloc(strlen(o.name) + strlen("( )"));
    meta = malloc(strlen(o.name));
    if (name == NULL || value == NULL || meta == NULL)
      return EXIT_FAILURE;

    if (isascii(o.val))
      sprintf(name, "(%s|%c)", o.name, (char)o.val);
    else
      sprintf(name, "%s", o.name);

    sprintf(meta, "%s", o.name);
    str = meta;
    while (*str++)
      *str = (char)toupper(*str);

    if (o.has_arg == required_argument)
      sprintf(value, " %s", meta);
    else if (o.has_arg == optional_argument)
      sprintf(value, "( %s)", meta);
    else
      sprintf(value, "");

    printf(" [%s%s]", name, value);

    free(name);
    free(value);
    free(meta);

    o = longopts[i++];
  }
  return EXIT_SUCCESS;
}

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
      exit(EXIT_SUCCESS);
    case 'h':
      exit(print_help(longopts, argv[0]));
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

void *ps_mmap(int fd_dev, size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dev, 0);
}

static void init_trans(struct axitangxi_transaction *trans, void *ps_addr,
                       uint32_t pl_addr, uint32_t size) {
  trans->tx_data_size = size;
  trans->rx_data_size = size;
  trans->burst_size = BURST_SIZE;
  trans->burst_data = 16 * BURST_SIZE;
  // ceil(a / b) = floor((a - 1) / b) + 1
  trans->burst_count = (size - 1) / (BURST_SIZE * 16) + 1;
  trans->tx_data_ps_ptr = ps_addr;
  trans->rx_data_pl_ptr = pl_addr;
}

ssize_t pl_io(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size,
              unsigned long request) {
  if (ps_addr == NULL)
    ps_addr = ps_mmap(fd_dev, size);
  if (ps_addr == MAP_FAILED)
    return -1;
  struct axitangxi_transaction trans;
  init_trans(&trans, ps_addr, pl_addr, size);
  if (ioctl(fd_dev, request, &trans) == -1)
    return -1;
  return size;
}

ssize_t pl_write(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size) {
  return pl_io(fd_dev, ps_addr, pl_addr, size, PSDDR_TO_PLDDR);
}

ssize_t pl_read(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size) {
  return pl_io(fd_dev, ps_addr, pl_addr, size, PLDDR_TO_PSDDR);
}

ssize_t ps_read_file(int fd_dev, char *filename, void *addr) {
  int fd = open(filename, O_RDWR);
  if (fd == -1)
    return -1;
  struct stat status;
  if (fstat(fd, &status) == -1)
    return -1;
  addr = ps_mmap(fd_dev, status.st_size);
  if (addr == MAP_FAILED)
    return -1;
  ssize_t size = read(fd, addr, status.st_size);
  if (close(fd) == -1)
    return -1;
  return size;
}

ssize_t pl_config(int fd_dev, char *filename, uint32_t pl_addr,
                  uint32_t *p_size) {
  void *ps_addr = NULL;
  *p_size = ps_read_file(fd_dev, filename, ps_addr);
  if (*p_size == -1)
    return -1;
  return pl_write(fd_dev, ps_addr, pl_addr, *p_size);
}

static ssize_t dump_mem(char *filename, void *ps_addr, size_t size) {
  int fd = open(filename, O_RDWR);
  if (fd == -1)
    return -1;
  ssize_t _size = write(fd, ps_addr, size);
  if (close(fd) == -1)
    return -1;
  return _size;
}

int main(int argc, char *argv[]) {
  opt_t opt;
  if (parse(argc, argv, &opt) == -1)
    errx(EXIT_FAILURE, "parse failure!");

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

  if (ioctl(fd_dev, NETWORK_ACC_CONFIG, &reg) == -1)
    err(errno, AXITX_DEV_PATH);
  if (ioctl(fd_dev, NETWORK_ACC_START) == -1)
    err(errno, AXITX_DEV_PATH);
  if (ioctl(fd_dev, NETWORK_ACC_GET, &reg) == -1)
    err(errno, AXITX_DEV_PATH);

  void *trans_addr;
  if (pl_read(fd_dev, trans_addr, reg.trans_addr, reg.trans_size) == -1)
    err(errno, AXITX_DEV_PATH);
  void *entropy_addr;
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
