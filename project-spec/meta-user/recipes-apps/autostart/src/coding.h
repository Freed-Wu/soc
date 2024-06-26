#ifndef CODING_H
#define CODING_H 1
#include <stdint.h>
#include <stdlib.h>

#define AA

__BEGIN_DECLS

#define LOW_BOUND 0
#define HIGH_BOUND (1 << 16) - 1

typedef uint16_t prob_t;
typedef uint16_t mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
  uint32_t freqs_resolution;
} gmm_t;

size_t coding(gmm_t *gmm, uint16_t *symbol, size_t len, uint8_t *bits,
              uint32_t low_bound, uint32_t high_bound);

__END_DECLS
#endif /* coding.h */
