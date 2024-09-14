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
#include <wordexp.h>

#include "axitangxi.h"
#include "coding.h"
#include "main.h"
#include "utils.h"

// 权重、因子、图片的地址
#define WEIGHT_ADDR 0x10000000
#define QUANTIFY_ADDR 0x10010000
#define PICTURE_BASE_ADDR 0x20000000
#define SCALE 0.00667200749740045

extern const uint8_t tp_header[4];
// status of program, not the status of every picture
status_t status;
// maximum of n_file_t
#define PICTURES_NUMBER_MAX 8
// picture status: unreceived -> received (processing) -> processed
// look up len to see if a picture have been processed
data8_t bit_streams[PICTURES_NUMBER_MAX];
// look up len == total_len to see if a picture have been received
data_frame_info_t data_frame_infos[PICTURES_NUMBER_MAX];

static status_t get_status(n_file_t n_file) {
  if (n_file >= PICTURES_NUMBER_MAX)
    return TP_STATUS_UNRECEIVED;
  if (bit_streams[n_file].len > 0)
    return TP_STATUS_PROCESSED;
  // empty file like /dev/null will be unreceived forever
  if (data_frame_infos[n_file].len == data_frame_infos[n_file].total_len &&
      data_frame_infos[n_file].total_len)
    return TP_STATUS_PROCESSING;
  return TP_STATUS_UNRECEIVED;
}

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
  opt->tty = "/dev/ttyPS1";
  opt->weight = "/usr/share/autostart/weight.bin";
  opt->quantization_coefficience = "/usr/share/autostart/quantify.bin";
  opt->dry_run = false;
  opt->level = LOG_NOTICE;
  opt->timeout = -1;
  opt->safe_time = 3;
}

static void deinit_opt(opt_t *opt) {
  if (strcmp(opt->out_dir, "."))
    free(opt->out_dir);
  free(opt->files);
}

static char shortopts[] = "hVvqbdt:T:S:w:c:o:";
static struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"binary", no_argument, NULL, 'b'},
    {"dry-run", no_argument, NULL, 'd'},
    {"tty", required_argument, NULL, 't'},
    // milli second
    {"timeout", required_argument, NULL, 'T'},
    // micro second
    {"safe-time", required_argument, NULL, 'S'},
    {"weight", required_argument, NULL, 'w'},
    {"quantization-coefficience", required_argument, NULL, 'c'},
    {"out-dir", required_argument, NULL, 'o'},
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
      opt->dry_run = true;
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
    case 'w':
      opt->weight = optarg;
      break;
    case 'c':
      opt->quantization_coefficience = optarg;
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

static size_t process_data_frames(int fd, data_frame_t *input_data_frames,
                                  n_frame_t n_frame, struct network_acc_reg reg,
                                  uint8_t **p_addr) {
  // convert data frames to yuv
  ssize_t yuv_len = data_frame_to_data_len(input_data_frames, n_frame);
  uint8_t *addr = malloc(yuv_len);
  data_frames_to_data(input_data_frames, n_frame, addr);
  ssize_t u_len = yuv_len / (4 + 1 + 1);
  data_t yuv[3] = {
      // uint16_t to uint8_t
      // len to size
      {.addr = ps_mmap(fd, u_len * 4 * 2), .len = u_len * 4},
      {.addr = ps_mmap(fd, u_len * 2), .len = u_len},
      {.addr = ps_mmap(fd, u_len * 2), .len = u_len},
  };
  uint8_t *p = addr;
  for (int k = 0; k < 3; k++) {
    int16_t *p_yuv = yuv[k].addr;
    for (total_data_len_t i = 0; i < yuv[k].len; ++i)
      *p_yuv++ = *p++;
  }
  free(addr);

  // neural encoding yuv to y', u', v'
  data_t trans[3] = {};
  data_t entropy[3] = {};
  for (int k = 0; k < 3; k++) {
    syslog(LOG_NOTICE, "start to encode yuv channel %d", k);
    if (pl_write(fd, yuv[k].addr, reg.picture_addr = PICTURE_BASE_ADDR,
                 // uint16_t to uint8_t
                 2 * yuv[k].len) == -1)
      err(errno, AXITX_DEV_PATH);
    // uint16_t to uint8_t
    if (munmap(yuv[k].addr, 2 * yuv[k].len) == -1)
      err(errno, AXITX_DEV_PATH);
    // uint16_t to uint8_t
    reg.picture_size = 2 * yuv[k].len;
    status |= TP_STATUS_NETWORK_ENCODING;
    pl_run(fd, &reg);
    status &= ~TP_STATUS_NETWORK_ENCODING;

    status |= TP_STATUS_NETWORK_ENCODING;
    // TODO: multithread
    syslog(LOG_NOTICE, "wait yuv channel %d to be encoded", k);
    pl_get(fd, &reg, trans[k].addr, entropy[k].addr);
    status &= ~TP_STATUS_NETWORK_ENCODING;

    trans[k].len = reg.trans_size;
    entropy[k].len = reg.entropy_size;
  }
  syslog(LOG_NOTICE, "all yuv channels are encoded");

  // entropy encoding y', u', v'
  status |= TP_STATUS_ENTROPY_ENCODING;
  size_t lens[SUB_CNT] = {
      32400,  32400,  32400,   32400,   129600,  129600, 129600, 518400,
      518400, 518400, 2073600, 2073600, 2073600, 8160,   8160,   8160,
      8160,   32640,  32640,   32640,   130560,  130560, 130560, 522240,
      522240, 522240, 8160,    8160,    8160,    8160,   32640,  32640,
      32640,  130560, 130560,  130560,  522240,  522240, 522240};
  gmm_t *gmms[SUB_CNT];
  int16_t *data[SUB_CNT];
  for (int k = 0; k < 3; k++) {
    size_t gmm_len = entropy[k].len / 9;
    gmm_t *gmm = malloc(gmm_len * sizeof(gmm_t));
    for (size_t i = 0; i < 5; i++) {
      gmm[i].mean1 = 1;
      syslog(LOG_NOTICE, "success for %zd mean", i);
      int16_t x = entropy[k].addr[i * 9];
      gmm[i].mean1 = x;
      int16_t x2 = entropy[k].addr[i * 9 + 1];
      gmm[i].mean2 = x2;
      /*gmm[i].mean2 = entropy[k].addr[i * 9 + 1];*/
      /*gmm[i].mean3 = entropy[k].addr[i * 9 + 2];*/
      /*gmm[i].std1 = entropy[k].addr[i * 9 + 3];*/
      /*gmm[i].std2 = entropy[k].addr[i * 9 + 4];*/
      /*gmm[i].std3 = entropy[k].addr[i * 9 + 5];*/
      /*gmm[i].prob1 = entropy[k].addr[i * 9 + 6];*/
      /*gmm[i].prob2 = entropy[k].addr[i * 9 + 7];*/
      /*gmm[i].prob3 = entropy[k].addr[i * 9 + 8];*/
      syslog(LOG_NOTICE, "success for %zd", i);
    }
    int16_t x = entropy[k].addr[0];
    syslog(LOG_NOTICE, "success for %zd mean1", 0);
    gmm[0].mean1 = 1;
    syslog(LOG_NOTICE, "success for %zd mean1", 0);
    x = entropy[k].addr[1];
    syslog(LOG_NOTICE, "success for %d ?", x);
    gmm[0].mean2 = x;
    syslog(LOG_NOTICE, "success for %zd mean2", 0);
    gmm[0].mean3 = entropy[k].addr[2];
    syslog(LOG_NOTICE, "success for %zd mean3", 0);
    entropy_to_gmm(entropy[k].addr, gmm, gmm_len);
    size_t ptr = 0;
    for (int i = 13 * k; i < 13 * (k + 1); i++) {
      data[i] = trans[k].addr + ptr;
      gmms[i] = gmm + ptr;
      ptr += lens[i];
    }
    if (ptr != gmm_len || ptr != trans[k].len)
      syslog(LOG_NOTICE, "yuv channel %d error", k);
  }
  syslog(LOG_NOTICE, "all yuv channels are entropy encoded");
  // ******** gmm_scale由硬件给出
  int gmm_scale = 10000;
  // ********
  CodingResult enc_res = codings(gmms, data, lens, gmm_scale);
  uint8_t *enc_data = enc_res.data;
  size_t len = enc_res.length;
  status &= ~TP_STATUS_ENTROPY_ENCODING;

  if (*p_addr == NULL) {
    *p_addr = malloc(len);
    if (*p_addr == NULL)
      err(errno, NULL);
  }
  p = *p_addr;
  memcpy(p, enc_data, len);
  free(enc_data);
  return len;
}

static size_t process_data_frames_dry_run(int fd,
                                          data_frame_t *input_data_frames,
                                          n_frame_t n_frame,
                                          struct network_acc_reg reg,
                                          uint8_t **p_addr, bool dry_run) {
  size_t len;
  if (dry_run) {
    len = data_frame_to_data_len(input_data_frames, n_frame);
    uint8_t *p = *p_addr = malloc(len * sizeof(uint8_t));
    for (n_frame_t i = 0; i < n_frame; i++) {
      memcpy(p, input_data_frames[i].data, input_data_frames[i].data_len);
      p += input_data_frames[i].data_len;
    }
  } else
    // TODO: multithread
    len = process_data_frames(fd, input_data_frames, n_frame, reg, p_addr);
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
    syslog(LOG_NOTICE, AXITX_DEV_PATH " init successfully");
  }

  for (n_file_t k = 0; k < opt.number; k++) {
    char *filename = opt.files[k];
    data_frame_t *data_frames;
    n_file_t n_file = k;
    n_frame_t n_frame;
    int ret_status =
        get_data_frames(filename, &n_file, &n_frame, &data_frames, opt.binary);
    // cannot open filename
    if (ret_status == 1) {
      syslog(LOG_ERR, "%s: %s", filename, strerror(errno));
      // skip to next picture
      continue;
      // cannot close filename
    } else if (ret_status == 2) {
      syslog(LOG_ERR, "%s: %s", filename, strerror(errno));
    } else if (ret_status == 3) {
      err(errno, NULL);
    }
    syslog(LOG_NOTICE, "start to encode %s", opt.files[k]);
    bit_streams[n_file].len =
        process_data_frames_dry_run(fd_dev, data_frames, n_frame, reg,
                                    &bit_streams[n_file].addr, opt.dry_run);
    // save file
    // TODO: multithread
    filename =
        malloc((strlen(opt.out_dir) + sizeof("XX.bin") - 1) * sizeof(char));
    sprintf(filename, "%s/%d.bin", opt.out_dir, k);
    if (dump_mem(filename, bit_streams[n_file].addr, bit_streams[n_file].len) ==
        -1)
      syslog(LOG_ERR, "%s: %s", filename, strerror(errno));
    else
      syslog(LOG_NOTICE, "%s has been encoded to %s", opt.files[k], filename);
    free(filename);
  }

  int fd, send_fd, recv_fd;
  struct termios oldattr;
  if (!opt.number) {
    if ((fd = open(opt.tty, O_RDWR | O_NOCTTY)) == -1)
      err(errno, "%s", opt.tty);
    fd_to_epoll_fds(fd, &send_fd, &recv_fd);
    oldattr = init_tty(fd);
    syslog(LOG_NOTICE, "%s: initial successfully%s", opt.tty,
           opt.dry_run ? " (dry run)" : "");
  }

  frame_t input_frame, output_frame = {.address = TP_ADDRESS_SLAVE};
  ssize_t n;
  while (!opt.number) {
    do {
      // receive forever
      memset(&input_frame, 0, sizeof(input_frame));
      syslog(LOG_NOTICE, "wait a frame");
      n = receive_frame(recv_fd, &input_frame, -1);
    } while (n <= 0 || input_frame.address != TP_ADDRESS_MASTER);

    switch (input_frame.frame_type) {
    case TP_FRAME_TYPE_QUERY:
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.status = get_status(input_frame.n_file);
      if (input_frame.n_file < PICTURES_NUMBER_MAX)
        output_frame.n_frame = data_frame_infos[input_frame.n_file].len;
      else
        output_frame.n_frame = 0;
      syslog(LOG_NOTICE, "%s to response query file %u with status %x",
             send_frame(send_fd, &output_frame, opt.timeout) > 0 ? "succeed"
                                                                 : "failed",
             output_frame.n_file, output_frame.status);
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
      ret = send_frame(send_fd, &output_frame, opt.timeout) > 0;
      syslog(LOG_NOTICE, "%s to response to receive yuv %u with %u frames",
             ret ? "succeed" : "failed", output_frame.n_file,
             output_frame.n_frame);
      if (!ret)
        break;

      // prepare to receive data frames, set data_frame_infos
      // override if processed
      if (get_status(input_frame.n_file) == TP_STATUS_PROCESSED) {
        free(data_frame_infos[input_frame.n_file].addr);
        data_frame_infos[input_frame.n_file].addr = NULL;
      }
      // every picture only malloc once!
      if (data_frame_infos[input_frame.n_file].addr == NULL) {
        data_frame_infos[input_frame.n_file].addr =
            calloc(input_frame.n_frame, sizeof(data_frame_t));
        if (data_frame_infos[input_frame.n_file].addr == NULL) {
          syslog(LOG_ERR, "malloc failed");
          err(errno, NULL);
        }
      }
      data_frame_infos[input_frame.n_file].total_len = input_frame.n_frame;
      data_frame_t *input_data_frames =
          data_frame_infos[input_frame.n_file].addr;

      n_frame_t sum = receive_data_frames(recv_fd, input_data_frames,
                                          input_frame, opt.timeout);
      data_frame_infos[input_frame.n_file].len =
          data_frame_infos[input_frame.n_file].total_len - sum;
      syslog(LOG_NOTICE, "%u incorrect frames need to be corrected", sum);

      // process data frames
      if (sum == 0) {
        bit_streams[input_frame.n_file].len = process_data_frames_dry_run(
            fd_dev, input_data_frames, input_frame.n_frame, reg,
            &bit_streams[input_frame.n_file].addr, opt.dry_run);
      }
      break;

    case TP_FRAME_TYPE_RECV:
      // check
      if (input_frame.n_file >= PICTURES_NUMBER_MAX) {
        syslog(LOG_ERR, "picture %u exceeds maximum: %d\n", input_frame.n_file,
               PICTURES_NUMBER_MAX);
        break;
      }
      // pictures haven't been encoded
      if (bit_streams[input_frame.n_file].len == 0) {
        syslog(LOG_ERR, "picture %u haven't been encoded\n",
               input_frame.n_file);
        break;
      }
      // alloc output_data_frames
      output_frame.frame_type = input_frame.frame_type;
      output_frame.n_file = input_frame.n_file;
      output_frame.n_frame = bit_streams[input_frame.n_file].len == 0
                                 ? 0
                                 : (bit_streams[input_frame.n_file].len - 1) /
                                           TP_FRAME_DATA_LEN_MAX +
                                       1;
      data_frame_t *output_data_frames =
          alloc_data_frames(output_frame.n_frame, output_frame.n_file,
                            bit_streams[input_frame.n_file].addr,
                            bit_streams[input_frame.n_file].len, TP_FLAG_DATA,
                            bit_streams[input_frame.n_file].len);
      if (output_data_frames == NULL) {
        syslog(LOG_ERR, "%s", strerror(errno));
        break;
      }

      // response to receive data
      ret = send_frame(send_fd, &output_frame, opt.timeout);
      syslog(LOG_NOTICE, "%s to response to send data %u with %u frames",
             ret ? "succeed" : "failed", output_frame.n_file,
             output_frame.n_frame);
      if (!ret)
        break;

      // send data
      sleep(5);
      size_t safe_frames = sysconf(_SC_PAGESIZE) / sizeof(data_frame_t);
      for (n_frame_t i = 0; i < output_frame.n_frame; i++) {
        if (i % safe_frames == safe_frames - 1)
          usleep(opt.safe_time);
        send_data_frame(send_fd, &output_data_frames[i], opt.timeout);
      }
      syslog(LOG_NOTICE, "send data %u with %u frames", output_frame.n_file,
             output_frame.n_frame);

      // complete
      free(output_data_frames);
      break;

    default:
      syslog(LOG_ERR, "Unknown frame type: %d\n", input_frame.frame_type);
    }
  }

  for (n_file_t i = 0; i < PICTURES_NUMBER_MAX; i++) {
    if (bit_streams[i].addr != NULL)
      free(bit_streams[i].addr);
    if (data_frame_infos[i].addr != NULL)
      free(data_frame_infos[i].addr);
  }
  deinit_opt(&opt);
  if (!opt.number) {
    tcsetattr(fd, TCSANOW, &oldattr);
    if (close(fd) == -1)
      err(errno, "%s", opt.tty);
  }
  if (!opt.dry_run && close(fd_dev) == -1)
    err(errno, AXITX_DEV_PATH);
  return EXIT_SUCCESS;
}
