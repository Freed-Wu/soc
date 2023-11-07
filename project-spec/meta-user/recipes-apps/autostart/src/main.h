#ifndef MAIN_H
#define MAIN_H 1
#include <sys/cdefs.h>
__BEGIN_DECLS

#include "transmission_protocol.h"

#define TP_ADDRESS TP_ADDRESS_SLAVE
#ifndef TTY
#define TTY "/tmp/ttyS1"
#endif
#ifndef OUTPUT_DIR
#define OUTPUT_DIR "/tmp"
#endif

typedef struct {
  char *tty;
  char *output_dir;
} opt_t;

const opt_t default_opt = {
    .tty = TTY,
    .output_dir = OUTPUT_DIR,
};

const frame_t default_frame = {
    .header = TP_HEADER,
    .address = TP_ADDRESS,
    .frame_type = TP_FRAME_TYPE_REQUEST_DATA,
    .n_file = 0,
    .n_frame = 0,
};

__END_DECLS
#endif /* main.h */
