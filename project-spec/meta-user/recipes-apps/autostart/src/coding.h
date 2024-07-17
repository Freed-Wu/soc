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
} gmm_t;


extern  size_t coding(gmm_t* gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound,uint32_t freqs_resolution);

double test_one(char *input_path, char *bin_path, bool is_bin );

__END_DECLS
#endif /* coding.h */
