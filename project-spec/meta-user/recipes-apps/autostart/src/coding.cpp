/**
 * 跟iwave完全对应:
 * 需要求出每一个的概率的原因是需要总的频率和
 * 如果不求出就很难准确求出
 */
#include <iostream>
#include <math.h>

#include "arithmetic_coding.h"
#include "coding.h"

#define OUTPUT "/tmp/output.bin"

double normal_cdf(double index, mean_t mean, std_t std) {
  return 1.0 / 2 * (1 + erf((index - mean) / std / sqrt(2)));
}

extern "C" size_t coding(gmm_t *gmm, uint16_t *symbol, size_t len,
                         uint8_t *bits) {
  double prob1 = gmm->prob1, prob2 = gmm->prob2, prob3 = gmm->prob3;
  double mean1 = gmm->mean1, mean2 = gmm->mean2, mean3 = gmm->mean3;
  double std1 = gmm->std1, std2 = gmm->std2, std3 = gmm->std3;

  double prob_all = exp(prob1) + exp(prob2) + exp(prob3);
  prob1 = exp(prob1) / prob_all;
  prob2 = exp(prob2) / prob_all;
  prob3 = exp(prob3) / prob_all;

  int freqs_resolution = 1e6;

  long long total_freqs = 0;

  std::fstream file_bin(OUTPUT, std::ios::out | std::ios::binary);
  BitOutputStream bit_out(file_bin);
  CountingBitOutputStream c_bit_out(bit_out);
  ArithmeticEncoder enc(c_bit_out);

  for (size_t k = 0; k < len; k++) {
    int index = symbol[k];

    int symlow;
    int symhigh;
    for (int i = LOW_BOUND; i <= HIGH_BOUND; i++) {
      float lowboundlow = prob1 * (normal_cdf(i - 0.5, mean1, std1)) +
                          prob2 * normal_cdf(i - 0.5, mean2, std2) +
                          prob3 * normal_cdf(i - 0.5, mean3, std3);
      float highboundhigh = prob1 * (normal_cdf(i + 0.5, mean1, std1)) +
                            prob2 * normal_cdf(i + 0.5, mean2, std2) +
                            prob3 * normal_cdf(i + 0.5, mean3, std3);
      if (i == index)
        symlow = total_freqs;
      total_freqs += int((highboundhigh - lowboundlow) * freqs_resolution);
      if (i == index)
        symhigh = total_freqs;
    }
    // 频率的总个数，当前符号的下积分，当前符号的上积分，当前符号
    enc.write(total_freqs, symlow, symhigh, index);
  }

  // 所有符号结束后，调用下面的两句话
  enc.finish();
  c_bit_out.close();
  return 0;

  // TODO: 解码端还没写（解码端需要频率表）
  // std::fstream file_bin_read("tmp.bin", std::ios::in | std::ios::binary);
  // BitInputStream bit_in(file_bin_read);
  // ArithmeticDecoder dec(bit_in);
}
