#include "transmission_protocol.h"
#include "crc.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static size_t frame_without_check_sum2bit_stream(frame_t frame,
                                                 uint8_t *bit_stream) {
  size_t size;
  switch (frame.frame_type) {
  case TP_FRAME_TYPE_CONTROL:
    size = TP_FRAME_SIZE_CONTROL;
  case TP_FRAME_TYPE_REQUEST_DATA:
    size = TP_FRAME_SIZE_REQUEST_DATA;
  case TP_FRAME_TYPE_TRANSPORT_DATA:
    size = TP_FRAME_SIZE_TRANSPORT_DATA;
  }
  uint8_t *p_bit_stream = bit_stream;
  memcpy(p_bit_stream, frame.header, sizeof(frame.header));
  p_bit_stream += sizeof(frame.header);
  memcpy(p_bit_stream, &frame.address, sizeof(frame.address));
  p_bit_stream += sizeof(frame.address);
  memcpy(p_bit_stream, &frame.frame_type, sizeof(frame.frame_type));
  p_bit_stream += sizeof(frame.frame_type);
  switch (frame.frame_type) {
  case TP_FRAME_TYPE_CONTROL:
    memcpy(p_bit_stream, &frame.cmd_id, sizeof(frame.cmd_id));
    p_bit_stream += sizeof(frame.cmd_id);
    break;
  default:
    memcpy(p_bit_stream, &frame.n_file, sizeof(frame.n_file));
    p_bit_stream += sizeof(frame.n_file);
  }
  memcpy(p_bit_stream, &frame.n_frame, sizeof(frame.n_frame));
  p_bit_stream += sizeof(frame.n_frame);
  switch (frame.frame_type) {
  case TP_FRAME_TYPE_TRANSPORT_DATA:
    memcpy(p_bit_stream, &frame.data_len, sizeof(frame.data_len));
    p_bit_stream += sizeof(frame.data_len);
    memcpy(p_bit_stream, &frame.data, frame.data_len);
    p_bit_stream += TP_FRAME_DATA_LEN_MAX;
  }
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
  memcpy(p_frame->header, p_bit_stream, sizeof(tp_header));
  if (memcmp(p_frame->header, tp_header, sizeof(tp_header)) != 0) {
    printf("wrong header: %s is not %s!\n", p_frame->header, tp_header);
    return NULL;
  }
  p_bit_stream += sizeof(tp_header);
  memcpy(&p_frame->address, p_bit_stream, sizeof(p_frame->address));
  p_bit_stream += sizeof(p_frame->address);
  memcpy(&p_frame->frame_type, p_bit_stream, sizeof(p_frame->frame_type));
  p_bit_stream += sizeof(p_frame->frame_type);
  switch (p_frame->frame_type) {
  case TP_FRAME_TYPE_CONTROL:
    memcpy(&p_frame->cmd_id, p_bit_stream, sizeof(p_frame->cmd_id));
    p_bit_stream += sizeof(p_frame->cmd_id);
    break;
  case TP_FRAME_TYPE_TRANSPORT_DATA:
    memcpy(&p_frame->data_len, p_bit_stream, sizeof(p_frame->data_len));
    p_bit_stream += sizeof(p_frame->data_len);
    memcpy(p_frame->data, p_bit_stream, p_frame->data_len);
    p_bit_stream += TP_FRAME_DATA_LEN_MAX;
  case TP_FRAME_TYPE_REQUEST_DATA:
    memcpy(&p_frame->n_file, p_bit_stream, sizeof(p_frame->n_file));
    p_bit_stream += sizeof(p_frame->n_file);
    break;
  default:
    printf("unimplemented frame type: %d!\n", p_frame->frame_type);
    return NULL;
  }
  memcpy(&p_frame->n_frame, p_bit_stream, sizeof(p_frame->n_frame));
  p_bit_stream += sizeof(p_frame->n_frame);
  memcpy(&p_frame->check_sum, p_bit_stream, sizeof(p_frame->check_sum));
  uint8_t *temp = malloc(p_bit_stream - bit_stream);
  if (temp == NULL) {
    perror("temp");
    return NULL;
  }
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
  size_t size;
  switch (p_frame->frame_type) {
  case TP_FRAME_TYPE_REQUEST_DATA:
    size = TP_FRAME_SIZE_REQUEST_DATA;
    break;
  case TP_FRAME_TYPE_TRANSPORT_DATA:
    size = TP_FRAME_SIZE_TRANSPORT_DATA;
    break;
  case TP_FRAME_TYPE_CONTROL:
    size = TP_FRAME_SIZE_CONTROL;
    break;
  }
  uint8_t *bit_stream = malloc(size);
  if (frame2bit_stream(*p_frame, bit_stream) != size) {
    return -1;
  }
  return write(fd, bit_stream, size);
}

ssize_t receive_frame(int fd, frame_t *p_frame) {
  uint8_t temp[TP_FRAME_SIZE_CMD_TYPE];
  ssize_t n = read(fd, temp, TP_FRAME_SIZE_CMD_TYPE);
  if (n < TP_FRAME_SIZE_CMD_TYPE)
    return -1;
  frame_type_t frame_type;
  memcpy(&frame_type, temp + sizeof(temp) - sizeof(frame_type),
         sizeof(frame_type));
  size_t size;
  switch (frame_type) {
  case TP_FRAME_TYPE_CONTROL:
    size = TP_FRAME_SIZE_CONTROL;
  case TP_FRAME_TYPE_REQUEST_DATA:
    size = TP_FRAME_SIZE_REQUEST_DATA;
  case TP_FRAME_TYPE_TRANSPORT_DATA:
    size = TP_FRAME_SIZE_TRANSPORT_DATA;
  }
  uint8_t *buffer = malloc(size);
  uint8_t *p_buffer = buffer;
  memcpy(buffer, temp, sizeof(temp));
  p_buffer += sizeof(temp);
  n += read(fd, p_buffer, size - TP_FRAME_SIZE_CMD_TYPE);
  if (n < TP_FRAME_SIZE_CMD_TYPE)
    return -1;
  memcpy(p_frame, bit_stream2frame(buffer), sizeof(*p_frame));
  if (p_frame == NULL)
    return 0;
  return n;
}

void wait_frame(int fd, frame_t *p_input_frame, frame_t *p_output_frame) {
  ssize_t n = 0;
  p_output_frame->frame_type = TP_FRAME_TYPE_CONTROL;
  p_output_frame->cmd_id = TP_CONTROL_RESEND;
  do {
    n = receive_frame(fd, p_input_frame);
    switch (n) {
    case -1:
      break;
    case 0:
      send_frame(fd, p_input_frame);
      p_output_frame->n_frame++;
    }
  } while (n <= 0);
}
