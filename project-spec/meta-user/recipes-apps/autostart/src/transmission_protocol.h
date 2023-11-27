#ifndef TRANSMISSION_PROTOCOL_H
#define TRANSMISSION_PROTOCOL_H 1
#include <stdint.h>
#include <stdlib.h>

#include "coding.h"

__BEGIN_DECLS

#define TP_FRAME_DATA_LEN_MAX 512
#define TP_HEADER                                                              \
  { 0xEB, 0x90, 0xEB, 0x90 }

const uint8_t tp_header[] = TP_HEADER;

typedef enum : uint8_t {
  TP_ADDRESS_MASTER,
  TP_ADDRESS_SLAVE,
} address_t;
typedef enum : uint8_t {
  TP_FRAME_TYPE_REQUEST = 1,
  TP_FRAME_TYPE_ACK = 3,
  TP_FRAME_TYPE_NACK = 5,
} frame_type_t;
typedef uint32_t n_file_t;
typedef uint32_t n_frame_t;
typedef uint16_t data_len_t;
typedef struct {
  address_t address;
  frame_type_t frame_type;
  n_file_t n_file;
  n_frame_t n_frame;
  uint16_t check_sum;
} frame_t;
typedef struct {
  uint8_t header[sizeof(tp_header)];
  n_file_t n_file;
  n_frame_t n_frame;
  data_len_t data_len;
  uint8_t data[TP_FRAME_DATA_LEN_MAX];
  uint16_t check_sum;
} data_frame_t;
typedef struct {
  uint8_t *addr;
  size_t len;
} data_t;

size_t frame2bit_stream(frame_t frame, uint8_t *bit_stream);
frame_t *bit_stream2frame(uint8_t *bit_stream);

ssize_t send_frame(int, frame_t *);
ssize_t receive_frame(int, frame_t *);
ssize_t send_data_frame(int, data_frame_t *);
ssize_t receive_data_frame(int, data_frame_t *);
ssize_t data_frame_to_data_len(data_frame_t *, n_frame_t);
void data_frames_to_data(data_frame_t *, n_frame_t, uint8_t *, size_t);
void data_to_data_frames(data_frame_t *, n_frame_t, uint8_t *, size_t);
ssize_t data_to_yuv420(uint8_t *, uint8_t *, uint8_t *, size_t);
void entropy_to_gmm(uint16_t *, gmm_t *);

__END_DECLS
#endif /* transmission_protocol.h */
