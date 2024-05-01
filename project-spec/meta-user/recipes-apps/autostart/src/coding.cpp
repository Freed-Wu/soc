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
                         uint8_t *bits, uint32_t low_bound,
                         uint32_t high_bound) {

  uint16_t m_probs[3] = {gmm->prob1, gmm->prob2, gmm->prob3};
  uint16_t m_means[3] = {gmm->mean1, gmm->mean2, gmm->mean3};
  uint16_t m_stds[3] = {gmm->std1, gmm->std2, gmm->std3};
  uint32_t freqs_resolution = gmm->freqs_resolution;
  GmmTable freqs(m_probs, m_means, m_stds, freqs_resolution, low_bound,
                 high_bound);

  BitOutputStream bout(bits);
  CountingBitOutputStream c_bit_out(bout);
  ArithmeticEncoder enc(c_bit_out);

  for (size_t k = 0; k < len; k++) {
    int index = symbol[k];

    uint32_t total_freqs = freqs.total_freqs;
    uint32_t symlow = freqs.symlow[index];
    uint32_t symhigh = freqs.symhigh[index];
    enc.write(total_freqs, symlow, symhigh, index);
  }

  // 所有符号结束后，调用下面的两句话
  enc.finish();
  c_bit_out.close();
  size_t size = bout.size;
  return size;

  // TODO: 解码端还没写（解码端需要频率表）
  // std::fstream file_bin_read("tmp.bin", std::ios::in | std::ios::binary);
  // BitInputStream bit_in(file_bin_read);
  // ArithmeticDecoder dec(bit_in);
}
