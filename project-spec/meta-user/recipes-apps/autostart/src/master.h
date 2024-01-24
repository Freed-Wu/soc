/**
 * master's private header
 */
#ifndef MASTER_H
#define MASTER_H 1
#include <sys/cdefs.h>

#include "transmission_protocol.h"
__BEGIN_DECLS

typedef struct {
  char *tty;
  char *out_dir;
  char **files;
  n_file_t number;
  int timeout;
  unsigned int safe_time;
  unsigned int level : 3;
  bool binary;
  bool dump;
} opt_t;

__END_DECLS
#endif /* master.h */
