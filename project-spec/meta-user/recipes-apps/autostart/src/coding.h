#ifndef CODING_H
#define CODING_H 1
#include <stdint.h>
#include <stdlib.h>

__BEGIN_DECLS

#define LOW_BOUND 0
#define HIGH_BOUND (1 << 16) - 1

typedef double prob_t;
typedef double mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
} gmm_t;

double normal_cdf(double index, mean_t mean, std_t std);
size_t coding(gmm_t gmm, uint16_t *symbol, size_t len, uint8_t *bits);

__END_DECLS
#endif /* coding.h */
