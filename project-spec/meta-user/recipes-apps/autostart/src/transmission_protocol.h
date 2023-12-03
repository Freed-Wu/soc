#ifndef TRANSMISSION_PROTOCOL_H
#define TRANSMISSION_PROTOCOL_H 1
#include <stdint.h>
#include <stdlib.h>

// 增加算术编码涉及到的头文件： "编解码器"，"GMM和频率表"
#include "GmmTable.h"

__BEGIN_DECLS

#define TP_FRAME_DATA_LEN_MAX 512

enum {
  TP_STATUS_NETWORK_ENCODING = 0x01,
  TP_STATUS_ENTROPY_ENCODING = 0x02,
};
enum {
  TP_ADDRESS_MASTER,
  TP_ADDRESS_SLAVE,
};
enum {
  TP_FRAME_TYPE_QUERY = 1,
  TP_FRAME_TYPE_SEND = 3,
  TP_FRAME_TYPE_ACK = 4,
  TP_FRAME_TYPE_NACK = 5,
  TP_FRAME_TYPE_RECV = 7,
};
typedef uint8_t address_t;
typedef uint8_t frame_type_t;
typedef uint32_t n_file_t;
typedef uint32_t n_frame_t;
typedef uint16_t data_len_t;
typedef uint64_t status_t;
typedef struct {
  address_t address;
  frame_type_t frame_type;
  union {
    status_t status;
    struct {
      n_file_t n_file;
      n_frame_t n_frame;
    };
  };
  uint16_t check_sum;
} __attribute__((packed)) frame_t;
typedef struct {
  uint8_t header[4];
  n_file_t n_file;
  n_frame_t n_frame;
  data_len_t data_len;
  uint8_t data[TP_FRAME_DATA_LEN_MAX];
  uint16_t check_sum;
} __attribute__((packed)) data_frame_t;

ssize_t send_frame(int, frame_t *, int);
ssize_t receive_frame(int, frame_t *, int);
ssize_t send_data_frame(int, data_frame_t *, int);
ssize_t receive_data_frame(int, data_frame_t *, int);

void init_data_frames(data_frame_t *, n_frame_t, n_file_t);
data_frame_t *alloc_data_frames(n_frame_t, n_file_t, uint8_t *, size_t);
size_t data_frame_to_data_len(data_frame_t *, n_frame_t);
void data_frames_to_data(data_frame_t *, n_frame_t, uint8_t *);
void data_to_data_frames(uint8_t *, size_t, data_frame_t *);
ssize_t data_to_yuv420(uint8_t *, uint8_t **, uint8_t **, size_t);
void entropy_to_gmm(uint16_t *, gmm_t *, size_t);

__END_DECLS
#endif /* transmission_protocol.h */
