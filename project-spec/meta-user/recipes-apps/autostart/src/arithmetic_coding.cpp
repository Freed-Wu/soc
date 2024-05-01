#include <algorithm>
#include <cassert>
#include <iostream>

#include "arithmetic_coding.h"

void BitOutputStream::write(char b) {
  // std::cout << b;
  if (b != 0 and b != 1)
    throw "Argument must be 0 or 1";
  m_currentbyte = (m_currentbyte << 1) | b;
  m_numbitsfilled += 1;
  if (m_numbitsfilled == 8) {
    data_addr[size++] = static_cast<uint8_t>(m_currentbyte);
    m_currentbyte = 0;
    m_numbitsfilled = 0;
  }
}

void BitOutputStream::close() {
  while (m_numbitsfilled != 0)
    write(0);
}

int BitInputStream::read() {
  if (m_currentbyte == -1)
    return -1;

  if (m_numbitsremaining == 0) {
    char temp;
    if (m_bit_in.eof()) {
      m_currentbyte = -1;
      return -1;
    }
    m_bit_in.read(&temp, 1);
    m_currentbyte = (int)temp;
    m_numbitsremaining = 8;
  }
  assert(m_numbitsremaining > 0);
  m_numbitsremaining -= 1;
  return (m_currentbyte >> m_numbitsremaining) & 1;
}

int BitInputStream::read_no_eof() {
  int result = read();
  if (result != -1) {
    return result;
  } else {
    throw("EOFError");
  }
}

void BitInputStream::close() {
  m_bit_in.close();
  m_currentbyte = -1;
  m_numbitsremaining = 0;
}

void CountingBitOutputStream::write(char b) {
  m_num_bits++;
  m_bit_out.write(b);
}

void CountingBitOutputStream::close() {
  m_num_bits += (8 - (m_num_bits % 8)) % 8;
  m_bit_out.close();
}

bool readBinaryFile(char file_path[], void *&data, size_t &size) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << file_path << std::endl;
    return false;
  }

  // 获取文件大小
  file.seekg(0, std::ios::end);
  size = file.tellg();
  file.seekg(0, std::ios::beg);

  // 分配内存
  data = new char[size];

  // 读取文件数据到数组
  file.read(reinterpret_cast<char *>(data), size);
  file.close();

  return true;
}

// ****** GMM频率表
int64_t clamp(int64_t x, int64_t l, int64_t r) {
  if (x < l)
    return l;
  else if (x > r)
    return r;
  else
    return x;
}

uint32_t softmax(uint16_t probs[3], uint16_t exp_table[], int scale = 1000,
                 int x_bound = -12) {
  int mx = *(std::max_element(probs, probs + 3));
  uint32_t sum = 0;
  int base = -x_bound * scale;

  for (int i = 0; i < 3; i++) {
    // 将idx 数值限定在[x_bound,0]
    int idx = probs[i];
    idx -= mx;
    if (probs[i] < x_bound)
      probs[i] = x_bound;

    // 映射到exp
    idx = idx * scale + base;
    probs[i] = exp_table[idx];
    sum += probs[i];
  }
  return sum;
}

int cnt(int x, uint32_t cdf_table[], uint16_t probs[3], uint32_t prob_sum,
        uint16_t means[3], uint16_t stds[3], uint32_t resolution = 1e6,
        int scale = 10000, int x_bound = -5) {
  uint32_t y_bound = UINT32_MAX;
  int base = -x_bound * scale;
  uint64_t cnt = 0;

  for (int i = 0; i < 3; i++) {
    // int idx_l=((-0.5+x)-means[i])*scale/stds[i];
    int idx_l = ((x - means[i]) * scale - (scale >> 1)) / stds[i];
    idx_l = clamp(idx_l, -base, base);
    idx_l += base;

    // int idx_r=((0.5+x)-means[i])*scale/stds[i];
    int idx_r = ((x - means[i]) * scale + (scale >> 1)) / stds[i];
    idx_r = clamp(idx_r, -base, base);
    idx_r += base;
    // uint64_t tr=cdf_table[idx_r],tl=cdf_table[idx_l];
    uint32_t cdf = cdf_table[idx_r] - cdf_table[idx_l];
    // 分辨率*权值*概率
    cnt += uint64_t(1) * resolution * probs[i] * cdf;
  }

  cnt /= prob_sum; // probs[i]
  cnt /= y_bound;  // cdf
  return cnt;
}

GmmTable::GmmTable(uint16_t m_probs[3], uint16_t m_means[3], uint16_t m_stds[3],
                   uint32_t freqs_resolution, uint32_t _low_bound,
                   uint32_t _high_bound) {
  uint16_t *exp_table = nullptr;
  size_t exp_size = 0;

  uint32_t *cdf_table = nullptr;
  size_t cdf_size = 0;

  readBinaryFile(exp_file_path, reinterpret_cast<void *&>(exp_table), exp_size);
  readBinaryFile(cdf_file_path, reinterpret_cast<void *&>(cdf_table), cdf_size);

  low_bound = _low_bound;
  high_bound = _high_bound; // 左闭右闭

  symlow.resize(high_bound + 2);
  symhigh.resize(high_bound + 2);

  uint32_t prob_sum = softmax(m_probs, exp_table);

  for (uint32_t i = low_bound; i <= high_bound + 1; i++) {
    int dif =
        cnt(i, cdf_table, m_probs, prob_sum, m_means, m_stds, freqs_resolution);
    dif = std::max(dif, 1);
    symlow[i] = total_freqs;
    total_freqs += dif;
    symhigh[i] = total_freqs;
  }

  delete[] exp_table;
  delete[] cdf_table;
}

uint32_t GmmTable::getSymbolLimit() const {
  return static_cast<uint32_t>(symlow.size());
}

void ArithmeticCoderBase::update(long long total, long long symlow,
                                 long long symhigh, char symbol) {
  long long low = m_low;
  long long high = m_high;
  long long range = high - low;
  if (total > MAX_TOTAL)
    throw("Cannot code symbol because total is too large");
  long long newlow = low + symlow * range / total;
  long long newhigh = low + symhigh * range / total - 1;
  m_low = newlow;
  m_high = newhigh;

  while (((m_low ^ m_high) & TOP_MASK) == 0) {
    shift(); // 将low最高位写入码流，然后根据underflow继续写
    m_low = (m_low << 1) & MASK;
    m_high = ((m_high << 1) & MASK) | 1; // 最后补1
  }

  while ((m_low & ~m_high & SECOND_MASK) != 0) {
    underflow();
    m_low = (m_low << 1) & (MASK >> 1);
    m_high = ((m_high << 1) & (MASK >> 1)) | TOP_MASK | 1;
  }
}

void ArithmeticEncoder::write(long long total, long long symlow,
                              long long symhigh, char symbol) {
  update(total, symlow, symhigh, symbol);
}

void ArithmeticEncoder::finish() { m_c_bit_out.write(1); }

void ArithmeticEncoder::shift() {
  unsigned char bit = m_low >> (STATE_SIZE - 1);
  m_c_bit_out.write(bit);
  for (int i = 0; i < m_num_underflow; i++) {
    m_c_bit_out.write(bit ^ 1);
  }
  m_num_underflow = 0;
}

void ArithmeticEncoder::underflow() { m_num_underflow += 1; }
