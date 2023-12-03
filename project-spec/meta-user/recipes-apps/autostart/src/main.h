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
  char *weight;
  char *quantization_coefficience;
  bool dry_run;
  unsigned int level : 3;
} opt_t;
typedef struct {
  uint8_t *addr;
  size_t len;
} data_t;

size_t process_data_frames(int, data_frame_t *, n_frame_t,
                           struct network_acc_reg, uint8_t *);

__END_DECLS
#endif /* main.h */
