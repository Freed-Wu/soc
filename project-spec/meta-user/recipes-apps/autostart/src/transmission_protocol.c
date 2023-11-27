#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "crc.h"
#include "transmission_protocol.h"

static size_t frame_without_check_sum2bit_stream(frame_t frame,
                                                 uint8_t *bit_stream) {
  uint8_t *p_bit_stream = bit_stream;
  memcpy(p_bit_stream, &frame.address, sizeof(frame.address));
  p_bit_stream += sizeof(frame.address);
  memcpy(p_bit_stream, &frame.frame_type, sizeof(frame.frame_type));
  p_bit_stream += sizeof(frame.frame_type);
  memcpy(p_bit_stream, &frame.n_file, sizeof(frame.n_file));
  p_bit_stream += sizeof(frame.n_file);
  memcpy(p_bit_stream, &frame.n_frame, sizeof(frame.n_frame));
  p_bit_stream += sizeof(frame.n_frame);
  return p_bit_stream - bit_stream;
}

size_t frame2bit_stream(frame_t frame, uint8_t *bit_stream) {
  size_t size = frame_without_check_sum2bit_stream(frame, bit_stream);
  uint16_t check_sum = crc16(bit_stream, size);
  uint8_t *p_bit_stream = bit_stream;
  p_bit_stream += size;
  memcpy(p_bit_stream, &check_sum, sizeof(check_sum));
  size += sizeof(check_sum);
  return size;
}

frame_t *bit_stream2frame(uint8_t *bit_stream) {
  frame_t *p_frame = (frame_t *)malloc(sizeof(frame_t));
  uint8_t *p_bit_stream = bit_stream;
  p_bit_stream += sizeof(tp_header);
  memcpy(&p_frame->address, p_bit_stream, sizeof(p_frame->address));
  p_bit_stream += sizeof(p_frame->address);
  memcpy(&p_frame->frame_type, p_bit_stream, sizeof(p_frame->frame_type));
  p_bit_stream += sizeof(p_frame->frame_type);
  memcpy(&p_frame->n_file, p_bit_stream, sizeof(p_frame->n_file));
  p_bit_stream += sizeof(p_frame->n_file);
  memcpy(&p_frame->n_frame, p_bit_stream, sizeof(p_frame->n_frame));
  p_bit_stream += sizeof(p_frame->n_frame);
  memcpy(&p_frame->check_sum, p_bit_stream, sizeof(p_frame->check_sum));
  uint8_t *temp = malloc(p_bit_stream - bit_stream);
  if (temp == NULL)
    err(errno, NULL);
  size_t size = frame_without_check_sum2bit_stream(*p_frame, temp);
  uint16_t check_sum = crc16(temp, size);
  if (p_frame->check_sum != check_sum) {
    printf("incorrect check sum: %d is not %d!\n", p_frame->check_sum,
           check_sum);
    return NULL;
  }
  return p_frame;
}

ssize_t send_frame(int fd, frame_t *p_frame) {
  uint8_t bit_stream[sizeof(frame_t)];
  if (frame2bit_stream(*p_frame, bit_stream) != sizeof(frame_t)) {
    return -1;
  }
  return write(fd, bit_stream, sizeof(frame_t));
}

ssize_t receive_frame(int fd, frame_t *p_frame) {
  uint8_t temp[sizeof(frame_t)];
  ssize_t n = read(fd, temp, sizeof(frame_t));
  if (n < sizeof(frame_t))
    return -1;
  frame_type_t frame_type;
  memcpy(&frame_type, temp + sizeof(temp) - sizeof(frame_type),
         sizeof(frame_type));
  uint8_t buffer[sizeof(frame_t)];
  uint8_t *p_buffer = buffer;
  memcpy(buffer, temp, sizeof(temp));
  p_buffer += sizeof(temp);
  memcpy(p_frame, bit_stream2frame(buffer), sizeof(*p_frame));
  if (p_frame == NULL)
    return 0;
  return n;
}

ssize_t send_data_frame(int fd, data_frame_t *p_frame) { return 0; }

ssize_t receive_data_frame(int fd, data_frame_t *p_frame) { return 0; }

ssize_t data_frame_to_data_len(data_frame_t *data_frames, n_frame_t n_frame) {
  return 0;
};

void data_frame_to_data(data_frame_t *data_frames, n_frame_t n_frame,
                        uint8_t *addr, size_t len){};

ssize_t data_to_yuv420(uint8_t *y, uint8_t *u, uint8_t *v, size_t yuv_len) {
  return 0;
};

void entropy_to_gmm(uint16_t *entropy_addr, gmm_t *gmm) {
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
