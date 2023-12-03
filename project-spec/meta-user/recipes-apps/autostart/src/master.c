/**
 * socat pty,rawer,link=/tmp/ttyS0 pty,rawer,link=/tmp/ttyS1
 * journalctl -tslave -tmaster -fn0
 */
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <wordexp.h>
// https://stackoverflow.com/a/48521433/16027269
#define termios asmtermios
#include <asm/termios.h>
#undef termios
#include <termios.h>

#include "master.h"
#include "utils.h"

#define TIMEOUT 3
#define LOOP_PERIOD 10000

extern const uint8_t tp_header[4];

static void init_opt(opt_t *opt) {
  wordexp_t exp;
  char out_dir[] = "~/Downloads";
  if (wordexp(out_dir, &exp, 0) != 0) {
    fprintf(stderr, "%s expand failure. use . as fallback.", out_dir);
    opt->out_dir = ".";
  } else {
    opt->out_dir = exp.we_wordv[0];
    wordfree(&exp);
  }
  opt->tty = "/tmp/ttyS0";
  opt->level = LOG_NOTICE;
}

static char shortopts[] = "hVvqt:o:";
static struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                                   {"version", no_argument, NULL, 'V'},
                                   {"verbose", no_argument, NULL, 'v'},
                                   {"quiet", no_argument, NULL, 'q'},
                                   {"tty", required_argument, NULL, 't'},
                                   {"out_dir", required_argument, NULL, 'w'},
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
        puts(" [yuv_file] ...");
      return 1;
    case 'v':
      opt->level++;
      break;
    case 'q':
      opt->level--;
      break;
    case 't':
      opt->tty = optarg;
      break;
    case 'o':
      opt->out_dir = optarg;
      break;
    default:
      return -1;
    }
  }
  setlogmask(LOG_UPTO(opt->level));
  opt->number = argc - optind + 1;
  opt->files = malloc(opt->number * sizeof(char *));
  for (int i = 0; i < opt->number; i++)
    opt->files[i] = argv[optind + i];
  return 0;
}

ssize_t dump_data_frames(data_frame_t *input_data_frames, n_frame_t n_frame,
                         char *filename) {
  int fd = open(filename, O_RDWR);
  if (fd == -1)
    return -1;
  ssize_t size = 0;
  for (n_frame_t i = 0; i < n_frame; i++) {
    size += write(fd, input_data_frames[i].data, input_data_frames[i].data_len);
  }
  if (close(fd) == -1)
    return -1;
  return size;
}

int main(int argc, char *argv[]) {
  opt_t opt;
  openlog(NULL, LOG_CONS | LOG_PERROR, 0);
  int ret = parse(argc, argv, &opt);
  if (ret == -1)
    errx(EXIT_FAILURE, "parse failure!");
  else if (ret > 0)
    return EXIT_SUCCESS;

  // configure device
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
  frame_t input_frame, output_frame = {
                           .address = TP_ADDRESS_MASTER,
                           .frame_type = TP_FRAME_TYPE_QUERY,
                       };
  ssize_t n;
  // query status to make sure idle
  do {
    syslog(LOG_NOTICE, "request query");
    send_frame(send_fd, &output_frame, -1);
    n = receive_frame(recv_fd, &input_frame, LOOP_PERIOD);
  } while (n <= 0 || input_frame.address != TP_ADDRESS_SLAVE ||
           input_frame.frame_type != output_frame.frame_type);

  // send data
  for (n_file_t n_file = 0; n_file < opt.number; n_file++) {
    // request to send data
    output_frame.frame_type = TP_FRAME_TYPE_SEND;
    output_frame.n_file = n_file;
    int fd_file = open(opt.files[n_file], O_RDONLY);
    if (fd_file == -1) {
    error:
      perror(opt.files[n_file]);
      continue;
    }
    struct stat status;
    if (fstat(fd_file, &status) == -1)
      goto error;
    output_frame.n_frame = (status.st_size - 1) / TP_FRAME_DATA_LEN_MAX + 1;
    do {
      syslog(LOG_NOTICE, "request to send file");
      send_frame(send_fd, &output_frame, -1);
      n = receive_frame(recv_fd, &input_frame, LOOP_PERIOD);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_SLAVE ||
             input_frame.frame_type != output_frame.frame_type ||
             input_frame.n_file != output_frame.n_file);

    // send data frames
    data_frame_t *output_data_frames = alloc_data_frames(
        output_frame.n_frame, output_frame.n_file, NULL, fd_file);
    if (close(fd_file) == -1)
      perror(opt.files[n_file]);
    for (int i = 0; i < output_frame.n_frame; i++)
      send_data_frame(send_fd, &output_data_frames[i], -1);
    bool cont = true;
    while (cont) {
      do {
        n = receive_frame(recv_fd, &input_frame, -1);
      } while (n <= 0 || input_frame.address != TP_ADDRESS_SLAVE);
      switch (input_frame.frame_type) {
      case TP_FRAME_TYPE_ACK:
        cont = false;
        free(output_data_frames);
        break;

      case TP_FRAME_TYPE_NACK:
        send_data_frame(send_fd, &output_data_frames[input_frame.n_frame], -1);
        break;

      default:
        fputs("Send ACK/NACK type frame, please!", stderr);
      }
    }
  }

  // receive data
  for (n_file_t n_file = 0; n_file < opt.number; n_file++) {
    // request to receive data
    output_frame.frame_type = TP_FRAME_TYPE_RECV;
    output_frame.n_file = n_file;
    output_frame.n_frame = 0;
    do {
      send_frame(send_fd, &output_frame, -1);
      n = receive_frame(recv_fd, &input_frame, LOOP_PERIOD);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_SLAVE ||
             input_frame.frame_type != output_frame.frame_type ||
             input_frame.n_file != output_frame.n_file);

    // receive data frames
    data_frame_t *input_data_frames =
        calloc(input_frame.n_frame, sizeof(data_frame_t));
    if (input_data_frames == NULL) {
      perror(opt.files[n_file]);
      continue;
    }
    data_frame_t data_frame;
    for (int i = 0; i < input_frame.n_frame; i++) {
      n = receive_data_frame(recv_fd, &data_frame, TIMEOUT);
      if (n <= 0 || data_frame.n_file != input_frame.n_file ||
          data_frame.n_frame >= input_frame.n_frame)
        continue;
      memcpy(&input_data_frames[data_frame.n_frame], &data_frame,
             sizeof(data_frame));
    }
    // request to resend data frames
    output_frame.frame_type = TP_FRAME_TYPE_NACK;
    for (int i = 0; i < input_frame.n_frame; i++) {
      if (input_data_frames[i].data_len == 0) {
        send_frame(send_fd, &output_frame, -1);
        do {
          n = receive_data_frame(recv_fd, &data_frame, -1);
        } while (n <= 0 || data_frame.n_file != input_frame.n_file ||
                 data_frame.n_frame != i);
      }
    }
    output_frame.frame_type = TP_FRAME_TYPE_ACK;
    send_frame(send_fd, &output_frame, -1);
    // TODO: multithread
    char *filename =
        malloc((strlen(opt.out_dir) + sizeof("XX.bin") - 1) * sizeof(char));
    sprintf(filename, "%s/%d.bin", opt.out_dir, n_file);
    if (dump_data_frames(input_data_frames, input_frame.n_frame, filename) ==
        -1)
      perror(opt.files[n_file]);
    free(filename);
  }

  tcsetattr(fd, TCSANOW, &oldattr);
  if (close(fd) == -1)
    err(errno, "%s", opt.tty);
  return EXIT_SUCCESS;
}
