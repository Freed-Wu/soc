/**
 * 跟iwave完全对应:
 * 需要求出每一个的概率的原因是需要总的频率和
 * 如果不求出就很难准确求出
 */
#include <syslog.h>

#include "arithmetic_coding.h"
#include "coding.h"

extern "C" size_t coding(gmm_t *gmm, uint16_t *symbol, size_t len,
                         uint8_t *bits, uint32_t low_bound,
                         uint32_t high_bound) {

  syslog(LOG_NOTICE, "begin coding");
  uint32_t freqs_resolution = gmm->freqs_resolution;
  BitOutputStream bout(bits);
  CountingBitOutputStream c_bit_out(bout);
  ArithmeticEncoder enc(c_bit_out);

  gmm_t *ggmm = gmm;
  for (size_t k = 0; k < len; k++, ggmm++) {
    uint16_t m_probs[3] = {ggmm->prob1, ggmm->prob2, ggmm->prob3};
    uint16_t m_means[3] = {ggmm->mean1, ggmm->mean2, ggmm->mean3};
    uint16_t m_stds[3] = {ggmm->std1, ggmm->std2, ggmm->std3};
    GmmTable freqs(m_probs, m_means, m_stds, freqs_resolution, low_bound,
                   high_bound);

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
  syslog(LOG_NOTICE, "end coding");
  return size;
}
