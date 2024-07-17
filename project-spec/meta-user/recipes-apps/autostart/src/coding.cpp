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

extern "C" size_t coding(gmm_t *gmm, uint16_t *trans_addr, size_t trans_len,
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

double test_one(char *input_path, char *bin_path, bool is_bin = false) {
  uint16_t *enc_data;
  size_t data_len;
  uint32_t data_max, data_min;

  read_txt(input_path, enc_data, data_len, data_max, data_min);
  uint16_t dif = data_max - data_min;
  // 输出文件名，数据长度，数据最大值，数据最小值
  printf("input_path: %s\n", input_path);
  printf("  data_len: %ld\n", data_len);
  printf("  data_max: %d\n", data_max);
  printf("  data_min: %d\n", data_min);

  gmm_t *gmm1 = (gmm_t *)malloc(data_len * sizeof(gmm_t));
  for (size_t i = 0; i < data_len; i++) {
    gmm1[i].prob1 = 1;
    gmm1[i].prob2 = 2;
    gmm1[i].prob3 = 1;
    gmm1[i].mean1 = data_min + rand() % dif;
    gmm1[i].mean2 = data_min + rand() % dif;
    gmm1[i].mean3 = data_min + rand() % dif;
    gmm1[i].std1 = 10;
    gmm1[i].std2 = 10;
    gmm1[i].std3 = 10;
  }

  uint8_t *data1 = (uint8_t *)malloc(data_len * 5 * sizeof(uint8_t));
  uint32_t freqs_resolution = 1000000;
  // 统计编码时间
  clock_t start, end;
  start = clock();
  int bin_len1 = coding(gmm1, enc_data, data_len, data1, data_min, data_max,
                        freqs_resolution);
  end = clock();
  printf("  encode_time: %f\n", (double)(end - start) / CLOCKS_PER_SEC);

  if (is_bin)
    write_bin(data1, bin_len1, bin_path);

  return (double)(end - start) / CLOCKS_PER_SEC;
}
