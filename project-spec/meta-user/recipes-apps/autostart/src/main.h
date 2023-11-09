#ifndef MAIN_H
#define MAIN_H 1
#include <sys/cdefs.h>
__BEGIN_DECLS

#include "transmission_protocol.h"

typedef struct {
  char *tty;
  char *weight;
  char *quantization_coefficience;
} opt_t;

const frame_t default_frame = {
    .header = TP_HEADER,
    .address = TP_ADDRESS_SLAVE,
    .frame_type = TP_FRAME_TYPE_REQUEST_DATA,
    .n_file = 0,
    .n_frame = 0,
};

__END_DECLS
#endif /* main.h */
