#ifndef TRANSMISSION_PROTOCOL_H
#define TRANSMISSION_PROTOCOL_H 1
#include <sys/cdefs.h>
__BEGIN_DECLS

#define TP_HEADER                                                              \
  { 0xEB, 0x90, 0xEB, 0x90 }

#define TP_ADDRESS_MASTER 0
#define TP_ADDRESS_SLAVE 1

#define TP_FRAME_TYPE_CONTROL 1
#define TP_FRAME_TYPE_REQUEST_DATA 3
#define TP_FRAME_TYPE_TRANSPORT_DATA 5

#define TP_FRAME_SIZE_CONTROL 16
#define TP_FRAME_SIZE_REQUEST_DATA 16
#define TP_FRAME_SIZE_TRANSPORT_DATA 530
#define TP_FRAME_SIZE_MAX TP_FRAME_SIZE_TRANSPORT_DATA
#define TP_FRAME_SIZE_CMD_TYPE 8

#define TP_FRAME_DATA_LEN_MAX 512

#define TP_CONTROL_RESEND 0

#include <stdint.h>
#include <stdlib.h>

const uint8_t tp_header[] = TP_HEADER;

typedef uint16_t address_t;
typedef uint16_t frame_type_t;
typedef uint32_t n_file_t;
typedef uint32_t cmd_id_t;
typedef uint16_t n_frame_t;
typedef uint16_t data_len_t;

typedef struct {
  uint8_t header[sizeof(tp_header)];
  address_t address;
  frame_type_t frame_type;
  union {
    n_file_t n_file;
    cmd_id_t cmd_id;
  };
  n_frame_t n_frame;
  data_len_t data_len;
  uint8_t data[512];
  uint16_t check_sum;
} frame_t;

frame_t *bit_stream2frame(uint8_t *);
size_t frame2bit_stream(frame_t, uint8_t *);
ssize_t send_frame(int, frame_t *);
ssize_t receive_frame(int, frame_t *);
void wait_frame(int, frame_t *, frame_t *);

__END_DECLS
#endif /* transmission_protocol.h */
