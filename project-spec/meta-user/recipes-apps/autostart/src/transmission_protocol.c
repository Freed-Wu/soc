#include <endian.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <syslog.h>
#include <unistd.h>

#include "crc.h"
#include "transmission_protocol.h"

#define COUNT 10

const uint8_t tp_header[] = {0x3A, 0x62, 0x04, 0x3F};
n_total_frame_t n_total_frame = {.uint24 = 0};

char *bin_to_str(uint8_t const *bin, size_t size) {
  char *str = malloc(size * 3 + 1);
  char *p = str;
  for (int i = 0; i < size; i++) {
    sprintf(p, "%02x ", bin[i]);
    p += 3;
  }
  return str;
}

n_frame_t id_to_n_frame(n_frame_t id, n_frame_t len) {
  if (len == 1)
    return id;
  // count n_frame from 1 not 0
  return ++id;
}

n_frame_t n_frame_to_id(n_frame_t n_frame, n_frame_t len) {
  if (len == 1)
    return n_frame;
  // count id from 0 not 1
  return --n_frame;
}

n_frame_t count_unreceived_data_frames(data_frame_t *data_frames,
                                       n_frame_t n_frame) {
  n_frame_t sum = 0;
  for (n_frame_t i = 0; i < n_frame; i++)
    if (data_frames[i].data_len == 0)
      sum++;
  return sum;
}

/**
 * Remember:
 * tcsetattr(fd, TCSANOW, &oldattr);
 * before exit()
 */
struct termios init_tty(int fd) {
  struct termios newattr, oldattr;
  tcgetattr(fd, &oldattr);
  newattr = oldattr;
  // https://stackoverflow.com/questions/22075741/weird-character-substitution-between-pseudo-terminal-and-serial-device
  cfmakeraw(&newattr);
  if (cfsetispeed(&newattr, TP_BAUD_RATE) == -1)
    err(errno, NULL);
  if (cfsetospeed(&newattr, TP_BAUD_RATE) == -1)
    err(errno, NULL);

  // 8O1
  // 8
  newattr.c_cflag &= ~CSIZE;
  newattr.c_cflag |= CS8;
  // O
  newattr.c_cflag |= PARENB;
  newattr.c_cflag |= PARODD;
  newattr.c_iflag |= INPCK;
  // 1
  newattr.c_cflag &= ~CSTOPB;
  if (tcsetattr(fd, TCSANOW, &newattr) == -1)
    err(errno, NULL);
  return oldattr;
}

ssize_t write_frame(int fd, const frame_t *frame) {
  frame_t temp = *frame;
  temp.n_file = htobe32(frame->n_file);
  temp.n_frame = htobe16(frame->n_frame);
  temp.status = htobe16(frame->status);
  temp.check_sum = crc16((uint8_t *)&temp, sizeof(temp) - sizeof(uint16_t));

  ssize_t n = write(fd, &temp, sizeof(temp));
  if (n < sizeof(temp))
    err(errno, NULL);
  char *str = bin_to_str((uint8_t *)&temp, n);
  syslog(LOG_INFO, "send: %s", str);
  free(str);
  return n;
}

ssize_t send_frame(int fd, const frame_t *frame, int timeout) {
  struct epoll_event event;
  int num = epoll_wait(fd, &event, 1, timeout);
  if (num < 1)
    return -1;

  return write_frame(event.data.fd, frame);
}

ssize_t write_data_frame(int fd, const data_frame_t *frame) {
  data_frame_t temp = *frame;
  temp.n_total_frame.uint24 = htobe32(frame->n_total_frame.uint24) >> 8;
  temp.n_file = htobe32(frame->n_file);
  temp.n_frame = htobe16(frame->n_frame);
  temp.total_data_len = htobe32(frame->total_data_len);
  temp.data_len = htobe16(frame->data_len);
  temp.check_sum = crc16((uint8_t *)&temp, sizeof(temp) - sizeof(uint16_t));

  ssize_t n = write(fd, &temp, sizeof(temp));
  if (n < sizeof(temp))
    err(errno, NULL);
  char *str = bin_to_str((uint8_t *)&temp, n);
  syslog(LOG_DEBUG, "send: %s", str);
  free(str);
  return n;
}

ssize_t send_data_frame(int fd, data_frame_t *frame, int timeout) {
  struct epoll_event event;
  int num = epoll_wait(fd, &event, 1, timeout);
  if (num < 1)
    return -1;

  frame->n_total_frame.uint24 = n_total_frame.uint24++;
  return write_data_frame(event.data.fd, frame);
}

ssize_t send_data_frame_directly(int fd, const data_frame_t *frame,
                                 int timeout) {
  struct epoll_event event;
  int num = epoll_wait(fd, &event, 1, timeout);
  if (num < 1)
    return -1;
  return write(event.data.fd, frame, sizeof(*frame));
}

/**
 * Slave waits master's request. If not received, just wait forever.
 * However, master sends a request and waits slave's response. If not received,
 * it shouldn't wait forever but resend a request. So a timeout is necessary.
 *
 * https://stackoverflow.com/questions/32537792/why-i-only-get-one-character-at-one-time-when-i-use-read
 */
ssize_t receive_frame(int fd, frame_t *frame, int timeout) {
  __typeof__(frame) temp = malloc(sizeof(*frame));

  struct epoll_event event;
  int num = epoll_wait(fd, &event, 1, timeout);
  ssize_t n = -1;
  if (num < 1) {
    syslog(LOG_INFO, "timeout overrun %d ms", timeout);
    goto free_temp;
  }
  n = read(event.data.fd, temp, sizeof(*frame));
  char *str = bin_to_str((uint8_t *)temp, n);
  if (n < sizeof(*frame)) {
    syslog(LOG_INFO, "receive not enough: %s", str);
    n = -2;
    goto free;
  }
  if (crc16((uint8_t *)temp, sizeof(*frame) - sizeof(uint16_t)) !=
      temp->check_sum) {
    syslog(LOG_INFO, "receive incorrect CRC: %s", str);
    n = -3;
    goto free;
  }
  syslog(LOG_INFO, "receive correctly: %s", str);
  memcpy(frame, temp, sizeof(*frame));

  frame->n_file = be32toh(frame->n_file);
  frame->n_frame = be16toh(frame->n_frame);
  frame->status = be16toh(frame->status);

free:
  free(str);
free_temp:
  free(temp);
  return n;
}

n_frame_t receive_data_frames(int recv_fd, data_frame_t *input_data_frames,
                              frame_t input_frame, int timeout) {
  struct epoll_event event;
  int num = epoll_wait(recv_fd, &event, 1, timeout);
  if (num < 1) {
    syslog(LOG_INFO, "timeout overrun %d ms", timeout);
    return -1;
  }
  size_t len = sizeof(data_frame_t) * input_frame.n_frame;
  data_frame_t *temp = calloc(len, sizeof(uint8_t));
  ssize_t n = 0;
  size_t count = 0;
  syslog(LOG_NOTICE, "begin to read");
  do {
    ssize_t size = read(event.data.fd, (uint8_t *)temp + n, len - n);
    if (size > 0) {
      count = 0;
      n += size;
    } else {
      count++;
      syslog(LOG_WARNING, "receive %zd frames + %zd bytes after %zu failure",
             n / sizeof(data_frame_t), n % sizeof(data_frame_t), count);
    }
  } while (n < len && count < COUNT);
  syslog(LOG_NOTICE, "finish to read");
  if (count == COUNT)
    syslog(LOG_INFO, "timeout overrun %zu times", count);

  n_frame_t sum = 0;
  for (n_frame_t i = 0; i < input_frame.n_frame; i++) {
    if (input_data_frames[i].data_len > 0)
      continue;
    if (memcmp(temp[i].header, tp_header, sizeof(tp_header)) ||
        crc16((uint8_t *)&temp[i], sizeof(data_frame_t) - sizeof(uint16_t)) !=
            temp[i].check_sum) {
      sum++;
      continue;
    }
    memcpy(&input_data_frames[i], &temp[i], sizeof(data_frame_t));

    input_data_frames[i].n_total_frame.uint24 =
        be32toh(input_data_frames[i].n_total_frame.uint24) >> 8;
    input_data_frames[i].n_file = be32toh(input_data_frames[i].n_file);
    input_data_frames[i].n_frame = be16toh(input_data_frames[i].n_frame);
    input_data_frames[i].total_data_len =
        be32toh(input_data_frames[i].total_data_len);
    input_data_frames[i].data_len = be16toh(input_data_frames[i].data_len);
  }
  free(temp);
  return sum;
}

size_t data_frame_to_data_len(data_frame_t *data_frames, n_frame_t n_frame) {
  size_t len = 0;
  for (int i = 0; i < n_frame; i++)
    len += data_frames[i].data_len;
  return len;
};

void data_frames_to_data(data_frame_t *data_frames, n_frame_t n_frame,
                         uint8_t *addr) {
  uint8_t *p = addr;
  for (n_frame_t i = 0; i < n_frame; i++) {
    memcpy(p, data_frames[i].data, data_frames[i].data_len);
    p += data_frames[i].data_len;
  }
};

void data_to_data_frames(uint8_t *addr, size_t len, data_frame_t *data_frames) {
  n_frame_t i = 0;
  uint8_t *p = addr;
  size_t n_frame = len == 0 ? 0 : (len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
  for (; i < n_frame - 1; i++) {
    memcpy(data_frames[i].data, p, TP_FRAME_DATA_LEN_MAX);
    data_frames[i].data_len = TP_FRAME_DATA_LEN_MAX;
    p += TP_FRAME_DATA_LEN_MAX;
  }
  data_frames[i].data_len = len - i * TP_FRAME_DATA_LEN_MAX;
  memcpy(data_frames[i].data, p, data_frames[i].data_len);
  memset(data_frames[i].data + data_frames[i].data_len, 0,
         TP_FRAME_DATA_LEN_MAX - data_frames[i].data_len);
};

void fd_to_data_frames(int fd, data_frame_t *data_frames, n_frame_t n_frame) {
  n_frame_t i = 0;
  ssize_t size;
  for (n_frame_t i = 0; i < n_frame; i++) {
    size = read(fd, data_frames[i].data, TP_FRAME_DATA_LEN_MAX);
    if (size == -1)
      err(errno, NULL);
    data_frames[i].data_len = size;
  }
  memset(data_frames[i].data + size, 0, TP_FRAME_DATA_LEN_MAX - size);
}

ssize_t data_to_yuv420(int16_t *y, int16_t **u, int16_t **v, size_t yuv_len) {
  size_t v_len = yuv_len / (4 + 1 + 1);
  *u = y + v_len * 4;
  *v = *u + v_len;
  return v_len;
};

/**
 * dequantize:
 *  output = input
 *  bias = 0
 */
void entropy_to_gmm(int16_t *entropy_addr, gmm_t *gmm, size_t len) {

  for (gmm_t *p = gmm; gmm + len - p >= 0; p++) {
    p->mean1 = entropy_addr[0];
    p->mean2 = entropy_addr[1];
    p->mean3 = entropy_addr[2];
    p->std1 = entropy_addr[3];
    p->std2 = entropy_addr[4];
    p->std3 = entropy_addr[5];
    p->prob1 = entropy_addr[6];
    p->prob2 = entropy_addr[7];
    p->prob3 = entropy_addr[8];
  }
}

/**
 * data, data_len and check_sum should be set in data_to_data_frames()
 */
void init_data_frames(data_frame_t *data_frames, n_frame_t n_frame,
                      n_file_t n_file, flag_t flag,
                      total_data_len_t total_data_len) {
  for (int i = 0; i < n_frame; i++) {
    memcpy(data_frames[i].header, tp_header, sizeof(tp_header));
    data_frames[i].flag = flag;
    data_frames[i].n_file = n_file;
    data_frames[i].n_frame = id_to_n_frame(i, n_frame);
    data_frames[i].total_data_len = total_data_len;
  }
}

/**
 * if addr is NULL, len is fd
 */
data_frame_t *alloc_data_frames(n_frame_t n_frame, n_file_t n_file,
                                uint8_t *addr, size_t len, flag_t flag,
                                total_data_len_t total_data_len) {
  data_frame_t *data_frames = malloc(n_frame * sizeof(data_frame_t));
  if (data_frames == NULL)
    return NULL;
  init_data_frames(data_frames, n_frame, n_file, flag, total_data_len);
  if (addr == NULL)
    fd_to_data_frames(len, data_frames, n_frame);
  else
    data_to_data_frames(addr, len, data_frames);
  return data_frames;
}

int get_data_frames(char *filename, n_file_t *const n_file,
                    n_frame_t *const n_frame,
                    data_frame_t **p_output_data_frames, bool binary) {
  int fd_file = open(filename, O_RDONLY);
  if (fd_file == -1)
    return 1;

  struct stat status;
  if (fstat(fd_file, &status) == -1)
    return 1;
  // update n_file, n_frame
  if (binary) {
    *n_frame = status.st_size / sizeof(data_frame_t);
    *p_output_data_frames = malloc(status.st_size);
    if (*p_output_data_frames == NULL)
      err(errno, NULL);
    uint8_t *p = *(uint8_t **)p_output_data_frames;
    for (n_frame_t _ = 0; _ < *n_frame; _++) {
      ssize_t n = read(fd_file, p, sizeof(data_frame_t));
      p += n;
    }
    *n_file = be32toh((*p_output_data_frames)[0].n_file);
  } else {
    *n_frame = status.st_size == 0
                   ? 0
                   : (status.st_size - 1) / TP_FRAME_DATA_LEN_MAX + 1;
    *p_output_data_frames = alloc_data_frames(*n_frame, *n_file, NULL, fd_file,
                                              TP_FLAG_1_YUV420, status.st_size);
    if (*p_output_data_frames == NULL)
      return 3;
  }

  if (close(fd_file) == -1)
    return 2;
  return 0;
}
