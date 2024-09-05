#ifndef CODING_H
#define CODING_H 1
#include <stdint.h>
#include <stdlib.h>

#define AA

__BEGIN_DECLS

typedef int prob_t;
typedef int mean_t;
typedef mean_t std_t;
#define SUB_CNT 39
typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
} gmm_t;

typedef struct {
  uint8_t *data; // 编码后的数据指针
  size_t length; // 数据长度
} CodingResult;

extern CodingResult codings(gmm_t *gmms[SUB_CNT], int16_t *data[SUB_CNT],
                            size_t *lens, int gmm_scale);

__END_DECLS
#endif /* coding.h */
