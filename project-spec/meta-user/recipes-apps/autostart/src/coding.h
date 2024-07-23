#ifndef CODING_H
#define CODING_H 1
#include <stdint.h>
#include <stdlib.h>

#define AA

__BEGIN_DECLS

typedef int prob_t;
typedef int mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
} gmm_t;

extern  size_t coding(gmm_t* gmm,int16_t* trans_addr,size_t trans_len,uint8_t* data_addr,int16_t low_bound,int16_t high_bound);

__END_DECLS
#endif /* coding.h */
