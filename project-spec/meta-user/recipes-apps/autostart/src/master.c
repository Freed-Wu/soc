#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>
#include <wordexp.h>

#include "master.h"
#include "utils.h"

#define LOOP_PERIOD 1000

extern const uint8_t tp_header[4];

static void init_opt(opt_t *opt) {
  wordexp_t exp;
  char out_dir[] = "~/Downloads";
  if (wordexp(out_dir, &exp, 0) != 0) {
    syslog(LOG_ERR, "%s expand failure. use . as fallback.", out_dir);
    opt->out_dir = ".";
  } else {
    opt->out_dir = strdup(exp.we_wordv[0]);
    wordfree(&exp);
  }
  opt->binary = false;
  opt->tty = MASTER_TTY;
  opt->timeout = 3;
  opt->safe_time = 3;
  opt->level = LOG_NOTICE;
  opt->dump = false;
}

static char shortopts[] = "hVvqbdt:T:S:W:o:";
static struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                                   {"version", no_argument, NULL, 'V'},
                                   {"verbose", no_argument, NULL, 'v'},
                                   {"quiet", no_argument, NULL, 'q'},
                                   {"binary", no_argument, NULL, 'b'},
                                   {"dump", no_argument, NULL, 'd'},
                                   {"tty", required_argument, NULL, 't'},
                                   {"timeout", required_argument, NULL, 'T'},
                                   {"safe-time", required_argument, NULL, 'S'},
                                   {"wait", required_argument, NULL, 'W'},
                                   {"out-dir", required_argument, NULL, 'w'},
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
    case 'b':
      opt->binary = true;
      break;
    case 'd':
      opt->dump = true;
      break;
    case 't':
      opt->tty = optarg;
      break;
    case 'T':
      opt->timeout = strtol(optarg, NULL, 0);
      break;
    case 'S':
      opt->safe_time = strtol(optarg, NULL, 0);
      break;
    case 'W':
      opt->wait = strtol(optarg, NULL, 0);
      break;
    case 'o':
      opt->out_dir = optarg;
      break;
    default:
      return -1;
    }
  }
  setlogmask(LOG_UPTO(opt->level));
  opt->number = argc - optind;
  opt->files = malloc(opt->number * sizeof(char *));
  for (int i = 0; i < opt->number; i++)
    opt->files[i] = argv[optind + i];
  return 0;
}

ssize_t dump_data_frames(data_frame_t *input_data_frames, n_frame_t n_frame,
                         char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd == -1)
    return -1;
  ssize_t size = 0;
  for (n_frame_t i = 0; i < n_frame; i++)
    size += write(fd, input_data_frames[i].data, input_data_frames[i].data_len);
  if (close(fd) == -1)
    return -1;
  return size;
}

static inline __suseconds_t tvdiff(struct timeval new_tv,
                                   struct timeval old_tv) {
  return (new_tv.tv_sec - old_tv.tv_sec) * 1000000 + new_tv.tv_usec -
         old_tv.tv_usec;
}

static ssize_t send_and_receive_frame(int send_fd, frame_t *output_frame,
                                      int send_timeout, int recv_fd,
                                      frame_t *input_frame, int recv_timeout) {
  struct timeval tv0, tv;
  while (true) {
    gettimeofday(&tv0, NULL);
    bool ret = send_frame(send_fd, output_frame, send_timeout) > 0;
    syslog(LOG_NOTICE, "%s to %s", ret ? "succeed" : "failed",
           output_frame->frame_type == TP_FRAME_TYPE_QUERY ? "query"
           : output_frame->frame_type == TP_FRAME_TYPE_SEND
               ? "request to send data"
               : "request to receive data");
    ssize_t n = -1;
    if (ret) {
      memset(input_frame, 0, sizeof(*input_frame));
      n = receive_frame(recv_fd, input_frame, recv_timeout);
    }
    if (n <= 0 || input_frame->address != TP_ADDRESS_SLAVE ||
        input_frame->frame_type != output_frame->frame_type ||
        input_frame->n_file != output_frame->n_file) {
      switch (n) {
      case -1:
        syslog(LOG_WARNING, "timeout");
        break;
      case -2:
        syslog(LOG_WARNING, "received not enough");
      }
      gettimeofday(&tv, NULL);
      __suseconds_t usec = LOOP_PERIOD * 1000 - tvdiff(tv, tv0);
      usleep(usec > 0 ? usec : 0);
    } else
      return n;
  }
}

/**
 * query status is a very usual action.
 * set output_frame.n_file by yourself.
 */
static void query_status(int send_fd, frame_t *output_frame, int send_timeout,
                         int recv_fd, frame_t *input_frame, int recv_timeout) {
  output_frame->frame_type = TP_FRAME_TYPE_QUERY;
  // don't modify output_frame->n_frame!
  output_frame->status = 0;
  send_and_receive_frame(send_fd, output_frame, send_timeout, recv_fd,
                         input_frame, recv_timeout);

  switch (input_frame->status) {
  case TP_STATUS_UNRECEIVED:
    syslog(LOG_NOTICE, "yuv %d only received %d frames", input_frame->n_file,
           input_frame->n_frame);
    break;
  case TP_STATUS_PROCESSING:
    syslog(LOG_NOTICE, "yuv %d with %d frames is processing",
           input_frame->n_file, input_frame->n_frame);
    break;
  case TP_STATUS_PROCESSED:
    syslog(LOG_NOTICE, "yuv %d have been processed to %d frames",
           input_frame->n_file, input_frame->n_frame);
    break;
  default:
    syslog(LOG_ERR, "Unknown frame type: %d\n", input_frame->frame_type);
  }
}

static n_frame_t receive_data_frames(int recv_fd,
                                     data_frame_t *input_data_frames,
                                     frame_t input_frame, n_frame_t sum,
                                     int timeout) {
  data_frame_t data_frame;
  for (n_frame_t _ = 0; _ < sum; _++) {
    ssize_t n = receive_data_frame(recv_fd, &data_frame, timeout);
    n_frame_t id = n_frame_to_id(data_frame.n_frame, input_frame.n_frame);
    if (n <= 0 || data_frame.n_file != input_frame.n_file ||
        id >= input_frame.n_frame || data_frame.flag != TP_FLAG_DATA ||
        memcmp(data_frame.header, tp_header, sizeof(tp_header)))
      continue;
    memcpy(&input_data_frames[id], &data_frame, sizeof(data_frame));
  }
  // update sum
  sum = count_unreceived_data_frames(input_data_frames, input_frame.n_frame);
  return sum;
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
  struct termios oldattr = init_tty(fd);

  fd_to_epoll_fds(fd, &send_fd, &recv_fd);
  syslog(LOG_NOTICE, "%s: initial successfully, data will be saved to %s",
         opt.tty, opt.out_dir);
  n_file_t *n_files = malloc(sizeof(n_file_t) * opt.number);

  // send data
  syslog(LOG_NOTICE, "drop %zd bytes", receive_and_drop(recv_fd, opt.timeout));
  syslog(LOG_NOTICE, "=== send data ===");
  frame_t input_frame, output_frame = {.address = TP_ADDRESS_MASTER};
  for (n_file_t k = 0; k < opt.number; k++) {
    // prepare to send data
    output_frame.frame_type = TP_FRAME_TYPE_SEND;
    int fd_file = open(opt.files[k], O_RDONLY);
    if (fd_file == -1) {
    error:
      syslog(LOG_ERR, "%s: %s", opt.files[k], strerror(errno));
      // skip to next picture
      continue;
    }
    struct stat status;
    if (fstat(fd_file, &status) == -1)
      goto error;
    data_frame_t *output_data_frames;
    // update n_file, n_frame
    if (opt.binary) {
      output_frame.n_frame = status.st_size / sizeof(data_frame_t);
      output_data_frames = malloc(status.st_size);
      if (output_data_frames == NULL)
        err(errno, NULL);
      uint8_t *p = (uint8_t *)output_data_frames;
      for (n_frame_t _ = 0; _ < output_frame.n_frame; _++) {
        ssize_t n = read(fd_file, p, sizeof(data_frame_t));
        p += n;
      }
      n_files[k] = be32toh(output_data_frames[0].n_file);
    } else {
      output_frame.n_frame = (status.st_size - 1) / TP_FRAME_DATA_LEN_MAX + 1;
      n_files[k] = k;
      output_data_frames =
          alloc_data_frames(output_frame.n_frame, k, NULL, fd_file,
                            TP_FLAG_1_YUV420, status.st_size);
      if (output_data_frames == NULL)
        err(errno, NULL);
    }
    output_frame.n_file = n_files[k];
    if (opt.dump) {
      char *filename =
          malloc((strlen(opt.out_dir) + sizeof("XX.dat") - 1) * sizeof(char));
      sprintf(filename, "%s/%d.dat", opt.out_dir, k);
      int fd_dat = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
      if (fd_dat == -1)
        err(errno, NULL);
      for (n_frame_t i = 0; i < output_frame.n_frame; i++)
        write_data_frame(fd_dat, &output_data_frames[i]);
      if (close(fd_dat) == -1)
        err(errno, NULL);
      syslog(LOG_NOTICE, "%s has been dumped to %s", opt.files[k], filename);
      free(filename);
    }
    if (close(fd_file) == -1)
      syslog(LOG_ERR, "%s: %s", opt.files[k], strerror(errno));

    query_status(send_fd, &output_frame, opt.timeout, recv_fd, &input_frame,
                 LOOP_PERIOD);
    // query status to ensure unreceived
    if (input_frame.status != TP_STATUS_UNRECEIVED)
      // skip to next picture
      goto free;

    do {
      // request to send data
      output_frame.frame_type = TP_FRAME_TYPE_SEND;
      syslog(LOG_NOTICE,
             "request to send yuv %d with %d frames to correct %d missing "
             "frames",
             output_frame.n_file, output_frame.n_frame,
             output_frame.n_frame - input_frame.n_frame);
      send_and_receive_frame(send_fd, &output_frame, opt.timeout, recv_fd,
                             &input_frame, LOOP_PERIOD);

      // send data frames
      for (n_frame_t i = 0; i < output_frame.n_frame; i++) {
        // cppcheck-suppress moduloofone
        if (i % SAFE_FRAMES == SAFE_FRAMES - 1)
          usleep(opt.safe_time);
        if (opt.binary)
          send_data_frame_directly(send_fd, &output_data_frames[i],
                                   opt.timeout);
        else
          send_data_frame(send_fd, &output_data_frames[i], opt.timeout);
      }

      // update input_frame
      query_status(send_fd, &output_frame, opt.timeout, recv_fd, &input_frame,
                   LOOP_PERIOD);
    } while (input_frame.status == TP_STATUS_UNRECEIVED);

    // complete
  free:
    free(output_data_frames);
  }

  // receive data
  syslog(LOG_NOTICE, "=== receive data ===");
  for (n_file_t k = 0; k < opt.number; k++) {
    // block until query status is processed
    output_frame.n_file = n_files[k];
    do {
      query_status(send_fd, &output_frame, opt.timeout, recv_fd, &input_frame,
                   LOOP_PERIOD);
    } while (input_frame.status != TP_STATUS_PROCESSED);

    // prepare to receive data, set output_frame
    data_frame_t *input_data_frames =
        calloc(input_frame.n_frame, sizeof(data_frame_t));
    if (input_data_frames == NULL)
      err(errno, "%s", opt.files[k]);
    // sum of unreceived frames
    n_frame_t sum = 0;

    do {
      // request to receive data
      output_frame.frame_type = TP_FRAME_TYPE_RECV;
      syslog(LOG_NOTICE, "request to receive data %d with %d frames",
             output_frame.n_file, input_frame.n_frame);
      send_and_receive_frame(send_fd, &output_frame, opt.timeout, recv_fd,
                             &input_frame, LOOP_PERIOD);

      // receive data frames
      sum = input_frame.n_frame;
      n_frame_t new_sum = sum;
      usleep(opt.wait);
      do {
        sum = new_sum;
        new_sum = receive_data_frames(recv_fd, input_data_frames, input_frame,
                                      sum, opt.timeout);
        syslog(LOG_NOTICE, "%d frames is unreceived", new_sum);
      } while (new_sum < sum);
      syslog(LOG_NOTICE, "drop %zd bytes",
             receive_and_drop(recv_fd, opt.timeout));
    } while (sum > 0);

    // save file
    // TODO: multithread
    char *filename =
        malloc((strlen(opt.out_dir) + sizeof("XX.bin") - 1) * sizeof(char));
    sprintf(filename, "%s/%d.bin", opt.out_dir, k);
    if (dump_data_frames(input_data_frames, input_frame.n_frame, filename) ==
        -1)
      syslog(LOG_ERR, "%s: %s", filename, strerror(errno));
    else
      syslog(LOG_NOTICE, "%s has been encoded to %s", opt.files[k], filename);

    // complete
    free(input_data_frames);
    // set it to NULL to allow malloc it in next picture
    input_data_frames = NULL;
    free(filename);
  }

  free(n_files);
  tcsetattr(fd, TCSANOW, &oldattr);
  if (close(fd) == -1)
    err(errno, "%s", opt.tty);
  return EXIT_SUCCESS;
}
