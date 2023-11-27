#ifndef MAIN_H
#define MAIN_H 1
#include <sys/cdefs.h>
__BEGIN_DECLS

typedef struct {
  char *tty;
  char *weight;
  char *quantization_coefficience;
} opt_t;

__END_DECLS
#endif /* main.h */
