#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "crc.h"
#include "transmission_protocol.h"

ssize_t send_frame(int fd, frame_t *frame) {
  frame->check_sum = crc16((uint8_t *)frame, sizeof(*frame) - sizeof(uint16_t));
  return write(fd, frame, sizeof(*frame));
}

ssize_t send_data_frame(int fd, data_frame_t *frame) {
  frame->check_sum = crc16((uint8_t *)frame, sizeof(*frame) - sizeof(uint16_t));
  return write(fd, frame, sizeof(*frame));
}

ssize_t receive_frame(int fd, frame_t *frame) {
  __typeof__(frame) temp;
  ssize_t n = read(fd, temp, sizeof(*frame));
  if (n < sizeof(*frame) ||
      crc16((uint8_t *)temp, sizeof(*frame) - sizeof(uint16_t)) !=
          temp->check_sum)
    return -1;
  memcpy(frame, temp, sizeof(*frame));
  return n;
}

ssize_t receive_data_frame(int fd, data_frame_t *frame) {
  __typeof__(frame) temp;
  ssize_t n = read(fd, temp, sizeof(*frame));
  if (n < sizeof(*frame) ||
      crc16((uint8_t *)temp, sizeof(*frame) - sizeof(uint16_t)) !=
          temp->check_sum ||
      memcmp(temp->header, tp_header, sizeof(tp_header)))
    return -1;
  memcpy(frame, temp, sizeof(*frame));
  return n;
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
  for (int i = 0; i < n_frame; i++) {
    memcpy(p, data_frames[i].data, data_frames[i].data_len);
    p += data_frames[i].data_len;
  }
};

void data_to_data_frames(uint8_t *addr, size_t len, data_frame_t *data_frames) {
  uint8_t *p = addr;
  size_t n_frame = (len - 1) / TP_FRAME_DATA_LEN_MAX + 1;
  int i;
  for (i = 0; i < n_frame - 1; i++) {
    memcpy(p, data_frames[i].data, TP_FRAME_DATA_LEN_MAX);
    data_frames[i].data_len = TP_FRAME_DATA_LEN_MAX;
    data_frames[i].check_sum = crc16((uint8_t *)&data_frames[i],
                                     sizeof(data_frame_t) - sizeof(uint16_t));
    p += TP_FRAME_DATA_LEN_MAX;
  }
  size_t last_len = len - i * TP_FRAME_DATA_LEN_MAX;
  memcpy(p, data_frames[i].data, last_len);
  data_frames[i].data_len = last_len;
  memset(data_frames[i].data + last_len, 0, TP_FRAME_DATA_LEN_MAX - last_len);
  data_frames[i].check_sum = crc16((uint8_t *)&data_frames[i],
                                   sizeof(data_frame_t) - sizeof(uint16_t));
};

ssize_t data_to_yuv420(uint8_t *y, uint8_t **u, uint8_t **v, size_t yuv_len) {
  size_t v_len = yuv_len / (4 + 1 + 1);
  *u = y + v_len * 4;
  *v = *u + v_len;
  return v_len;
};

void entropy_to_gmm(uint16_t *entropy_addr, gmm_t *gmm) {
  // TODO: scale and bias are known by Yu Yue
#define scale 1.0
#define bias 0
  gmm->mean1 = entropy_addr[1] / scale + bias;
  gmm->mean2 = entropy_addr[2] / scale + bias;
  gmm->mean3 = entropy_addr[3] / scale + bias;
  gmm->std1 = entropy_addr[4] / scale + bias;
  gmm->std2 = entropy_addr[5] / scale + bias;
  gmm->std3 = entropy_addr[6] / scale + bias;
  gmm->prob1 = entropy_addr[7] / scale + bias;
  gmm->prob2 = entropy_addr[8] / scale + bias;
  gmm->prob3 = entropy_addr[9] / scale + bias;
}
