/**
 * 跟iwave完全对应:
 * 需要求出每一个的概率的原因是需要总的频率和
 * 如果不求出就很难准确求出
 */
#include <syslog.h>

#include "arithmetic_coding.h"
#include "coding.h"
// 统计时间
#include <time.h>

bool write_bin(uint8_t *data_addr, size_t size, char file[]) {
  std::ofstream outputFile(file, std::ios::binary);

  if (!outputFile.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return false;
  }

  outputFile.write(reinterpret_cast<char *>(data_addr), size);
  outputFile.close();
  return true;
}

extern "C" size_t coding(gmm_t *gmm, int16_t *trans_addr, size_t trans_len,
                         uint8_t *data_addr, uint32_t low_bound,
                         uint32_t high_bound, uint32_t freqs_resolution) {
  BitOutputStream bout(data_addr);
  CountingBitOutputStream c_bit_out(bout);
  ArithmeticEncoder enc(c_bit_out);

  EncTable freqs_table(freqs_resolution, low_bound, high_bound);

  for (size_t i = 0; i < trans_len; i++, gmm++) {
    uint16_t m_probs[3] = {gmm->prob1, gmm->prob2, gmm->prob3};
    uint16_t m_means[3] = {gmm->mean1, gmm->mean2, gmm->mean3};
    uint16_t m_stds[3] = {gmm->std1, gmm->std2, gmm->std3};
    freqs_table.update(m_probs, m_means, m_stds);

    int symbol = trans_addr[i];
    freqs_table.get_bound(symbol);
    uint32_t total_freqs = freqs_table.total_freqs,
             symlow = freqs_table.sym_low, symhigh = freqs_table.sym_high;
    enc.write(total_freqs, symlow, symhigh, symbol);
  }

  enc.finish();
  c_bit_out.close();
  size_t size = bout.size;

  return size;
}
