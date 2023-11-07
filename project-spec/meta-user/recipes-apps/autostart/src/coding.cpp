// 跟iwave完全对应:需要求出每一个的概率的原因是需要总的频率和，如果不求出就很难准确求出
#include "coding.h"
#include "arithmetic_coding.h"
#include <iostream>
#include <math.h>

double normal_cdf(double index, double mean, double std) {
  return 1.0 / 2 * (1 + erf((index - mean) / std / sqrt(2)));
}

extern "C" void *coding() {
  double prob1, prob2, prob3; // 权重
  double mean1, mean2, mean3; //
  double std1, std2, std3;
  int index; // 待编码符号
  int low_bound, high_bound;

  index = 1, low_bound = 0, high_bound = 10;
  mean1 = 0.2, std1 = 0.4;
  mean2 = 0.3, std2 = 0.1;
  mean3 = 0.1, std3 = 0.3;
  prob1 = 0.1, prob2 = 0.6, prob3 = 0.3;

  std::fstream file_bin(OUTPUT, std::ios::out | std::ios::binary);

  double prob_all = exp(prob1) + exp(prob2) + exp(prob3);
  prob1 = exp(prob1) / prob_all;
  prob2 = exp(prob2) / prob_all;
  prob3 = exp(prob3) / prob_all;

  int freqs_resolution = 1e6; //

  BitOutputStream bit_out(file_bin);
  CountingBitOutputStream c_bit_out(bit_out);
  ArithmeticEncoder enc(c_bit_out);

  // normal_cdf(-0.5, mean1, std1);
  // float symlow = prob1 * (normal_cdf(index - 0.5, mean1, std1)) + prob2 *
  // normal_cdf(index - 0.5, mean2, std2) + prob3 * normal_cdf(index - 0.5,
  // mean3, std3); float symhigh = prob1 * (normal_cdf(index + 0.5, mean1,
  // std1)) + prob2 * normal_cdf(index + 0.5, mean2, std2) + prob3 *
  // normal_cdf(index + 0.5, mean3, std3); int symlow_int = int(symlow *
  // freqs_resolution); int symhigh_int = int(symhigh * freqs_resolution);

  long long total_freqs = 0;
  int symlow;
  int symhigh;
  for (int i = low_bound; i <= high_bound; i++) {
    float lowboundlow = prob1 * (normal_cdf(i - 0.5, mean1, std1)) +
                        prob2 * normal_cdf(i - 0.5, mean2, std2) +
                        prob3 * normal_cdf(i - 0.5, mean3, std3);
    float highboundhigh = prob1 * (normal_cdf(i + 0.5, mean1, std1)) +
                          prob2 * normal_cdf(i + 0.5, mean2, std2) +
                          prob3 * normal_cdf(i + 0.5, mean3, std3);
    if (i == index) {
      symlow = total_freqs;
    }
    total_freqs += int((highboundhigh - lowboundlow) * freqs_resolution);
    if (i == index) {
      symhigh = total_freqs;
    }
    // std::cout << int((highboundhigh - lowboundlow) * freqs_resolution) <<
    // std::endl;
  }
  // std::cout << total_freqs<<" "<< symlow<< " "<< symhigh <<" "<< index;
  enc.write(
      total_freqs, symlow, symhigh,
      index); // 输入为频率的总个数，当前符号的下积分，当前符号的上积分，当前符号

  // 所有符号结束后，调用下面的两句话
  enc.finish();
  c_bit_out.close();

  // TODO 解码端还没写（解码端需要频率表）
  // std::fstream file_bin_read("tmp.bin", std::ios::in | std::ios::binary);
  // BitInputStream bit_in(file_bin_read);
  // ArithmeticDecoder dec(bit_in);
}
