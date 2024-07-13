/**
 * slave's private header
 */
#ifndef MAIN_H
#define MAIN_H 1
#include <stdbool.h>

#include "transmission_protocol.h"
__BEGIN_DECLS

typedef struct {
  char *tty;
  char *out_dir;
  char **files;
  n_file_t number;
  char *weight;
  char *quantization_coefficience;
  bool dry_run;
  bool binary;
  int timeout;
  unsigned int safe_time;
  unsigned int level : 3;
} opt_t;
typedef struct {
  uint8_t *addr;
  size_t len;
} data_t;
// when len == total_len, this pictures is received successfully
typedef struct {
  data_frame_t *addr;
  n_frame_t len;
  n_frame_t total_len;
} data_frame_info_t;

__END_DECLS
#endif /* main.h */
