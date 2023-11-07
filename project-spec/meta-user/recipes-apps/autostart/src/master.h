#ifndef MASTER_H
#define MASTER_H 1
#include <sys/cdefs.h>
__BEGIN_DECLS

#include "transmission_protocol.h"

#define TP_ADDRESS TP_ADDRESS_MASTER
#ifndef TTY
#define TTY "/tmp/ttyS0"
#endif
#ifndef OUTPUT_DIR
#define OUTPUT_DIR "/tmp"
#endif

typedef struct {
  char *name;
  uint8_t *file;
  off_t size;
} img_t;

typedef struct {
  char *tty;
  char *output_dir;
  size_t img_number;
  img_t *imgs;
} opt_t;

const opt_t default_opt = {
    .tty = TTY,
    .output_dir = OUTPUT_DIR,
    .img_number = 0,
};
const frame_t default_frame = {
    .header = TP_HEADER,
    .address = TP_ADDRESS,
    .n_frame = 0,
};

__END_DECLS
#endif /* master.h */
