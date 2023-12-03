/**
 * master's private header
 */
#ifndef MASTER_H
#define MASTER_H 1
#include <sys/cdefs.h>

#include "utils.h"
#include "transmission_protocol.h"
__BEGIN_DECLS

typedef struct {
  char *tty;
  char *out_dir;
  char **files;
  n_file_t number;
  enum LOG_LEVEL level;
} opt_t;

ssize_t dump_data_frames(data_frame_t *, n_frame_t, char *);

__END_DECLS
#endif /* master.h */
