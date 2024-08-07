/**
 * 跟iwave完全对应:
 * 需要求出每一个的概率的原因是需要总的频率和
 * 如果不求出就很难准确求出
 */
#include "coding.h"
#include "arithmetic_coding.h"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <random>
#include <stddef.h>
#include <string>

extern "C" CodingResult codings(gmm_t *gmms[SUB_CNT], int16_t *datas[SUB_CNT],
                                size_t *lens, int gmm_scale) {
  size_t sum_len = accumulate(lens, lens + SUB_CNT, 0);
  uint8_t *bins = (uint8_t *)malloc((sum_len * 8) * sizeof(uint8_t));

  BitOutputStream bout(bins + SUB_CNT * 2 *
                                  sizeof(int16_t)); // 从第52=SUB_CNT*2*2B开始
  ArithmeticEncoder enc(32, bout);

  for (int i = 0; i < SUB_CNT; i++) {
    int len = lens[i];
    gmm_t *gmm = gmms[i];
    int16_t *data = datas[i];
    int16_t xmax = *max_element(data, data + len);
    int16_t xmin = *min_element(data, data + len);
    *reinterpret_cast<int16_t *>(bins + i * 2 * sizeof(int16_t)) = xmin;
    *reinterpret_cast<int16_t *>(bins + (i * 2 + 1) * sizeof(int16_t)) = xmax;

    if (xmin >= xmax)
      continue;
    EncTable freqs_table(1000000, xmin, xmax);
    freqs_table.scale_pred = gmm_scale;
    for (int p = 0; p < len; p++, gmm++) {
      int m_probs[3] = {gmm->prob1, gmm->prob2, gmm->prob3};
      int m_means[3] = {gmm->mean1, gmm->mean2, gmm->mean3};
      int m_stds[3] = {gmm->std1, gmm->std2, gmm->std3};
      freqs_table.update(m_probs, m_means, m_stds);

      freqs_table.get_bound(data[p]);
      int total_freqs = freqs_table.total_freqs, symlow = freqs_table.sym_low,
          symhigh = freqs_table.sym_high;
      enc.write(total_freqs, symlow, symhigh);
    }
  }

  enc.finish();
  bout.finish();

  CodingResult result = {bins, bout.size + SUB_CNT * 2 * sizeof(int16_t)};
  return result;
}
