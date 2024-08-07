#include "arithmetic_coding.h"

// ****** 码流IO: BitOutputStream 仅用作编码器
BitOutputStream::BitOutputStream(uint8_t *_addr)
    : currentByte(0), numBitsFilled(0), data_addr(_addr) {}

void BitOutputStream::write(int b) {
  if (b != 0 && b != 1)
    throw domain_error("Argument must be 0 or 1");
  currentByte = (currentByte << 1) | b;
  numBitsFilled++;
  if (numBitsFilled == 8) {
    if (numeric_limits<char>::is_signed)
      currentByte -= (currentByte >> 7) << 8;
    data_addr[size++] = static_cast<uint8_t>(currentByte);
    currentByte = 0;
    numBitsFilled = 0;
  }
}

void BitOutputStream::finish() {
  while (numBitsFilled != 0)
    write(0);
}

bool readBinaryFile(char file_path[], void *&data, size_t &size) {
  ifstream file(file_path, ios::binary);
  if (!file.is_open()) {
    cerr << "Error opening file: " << file_path << endl;
    return false;
  }
  // 获取文件大小
  file.seekg(0, ios::end);
  size = file.tellg();
  file.seekg(0, ios::beg);

  data = new char[size];
  file.read(reinterpret_cast<char *>(data), size);
  file.close();

  return true;
}

inline int64_t clamp(int64_t x, int64_t l, int64_t r) {
  if (x < l)
    return l;
  else if (x > r)
    return r;
  else
    return x;
}

int softmax(int probs[3], uint16_t exp_table[], int scale = 1000,
            int x_bound = -12) {
  int mx = *max_element(probs, probs + 3);
  int sum = 0;
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

EncTable::EncTable(uint32_t _freqs_resolution, int _low_bound,
                   int _high_bound) {
  readBinaryFile(exp_file_path, reinterpret_cast<void *&>(exp_table), exp_size);
  readBinaryFile(cdf_file_path, reinterpret_cast<void *&>(cdf_table), cdf_size);
  low_bound = _low_bound;
  high_bound = _high_bound;
  freqs_resolution = _freqs_resolution;
}

void EncTable::update(int m_probs[3], int m_means[3], int m_stds[3]) {
  probs = m_probs;
  means = m_means;
  stds = m_stds;
  prob_sum = softmax(m_probs, exp_table, exp_scale, exp_x_bound);
}

void EncTable::get_bound(int x) {
  uint32_t y_bound = UINT32_MAX;
  int base = -cdf_x_bound * cdf_scale;

  l_bound = 0, r_bound = 0;
  for (int i = 0; i < 3; i++) {
    int idx_l =
        (long long)((low_bound * scale_pred - means[i]) - scale_pred / 2) *
        cdf_scale / stds[i];
    idx_l = clamp(idx_l, -base, base);
    idx_l += base;

    int idx_r =
        (long long)((high_bound * scale_pred - means[i]) + scale_pred / 2) *
        cdf_scale / stds[i];
    idx_r = clamp(idx_r, -base, base);
    idx_r += base;

    l_bound +=
        uint64_t(1) * freqs_resolution * cdf_table[idx_l] / prob_sum * probs[i];
    r_bound +=
        uint64_t(1) * freqs_resolution * cdf_table[idx_r] / prob_sum * probs[i];
  }
  l_bound /= y_bound; // cdf
  r_bound /= y_bound; // cdf

  uint64_t low = 0, high = 0;
  for (int i = 0; i < 3; i++) {
    int idx_l = (long long)((x * scale_pred - means[i]) - scale_pred / 2) *
                cdf_scale / stds[i];
    idx_l = clamp(idx_l, -base, base);
    idx_l += base;

    int idx_r = (long long)((x * scale_pred - means[i]) + scale_pred / 2) *
                cdf_scale / stds[i];
    idx_r = clamp(idx_r, -base, base);
    idx_r += base;

    low +=
        uint64_t(1) * freqs_resolution * cdf_table[idx_l] / prob_sum * probs[i];
    high +=
        uint64_t(1) * freqs_resolution * cdf_table[idx_r] / prob_sum * probs[i];
  }
  low /= y_bound;  // cdf
  high /= y_bound; // cdf

  sym_low = low - l_bound + (x - low_bound); // (x-0.5) to (low-0.5) cumulative
                                             // sum & increment for each point
  sym_high =
      high - l_bound +
      (x - low_bound +
       1); // (x+0.5) to (low-0.5) cumulative sum & increment for each point
  total_freqs =
      r_bound - l_bound +
      (high_bound - low_bound +
       1); // (high+0.5) to (low-0.5) cumulative sum & increment for each point
}

EncTable::~EncTable() {
  delete[] exp_table;
  delete[] cdf_table;
}

// ****** 编码器
ArithmeticCoderBase::ArithmeticCoderBase(int numBits) {
  if (!(1 <= numBits && numBits <= 63))
    throw domain_error("State size out of range");
  numStateBits = numBits;
  fullRange = static_cast<decltype(fullRange)>(1) << numStateBits;
  halfRange = fullRange >> 1;      // Non-zero
  quarterRange = halfRange >> 1;   // Can be zero
  minimumRange = quarterRange + 2; // At least 2
  maximumTotal =
      min(numeric_limits<decltype(fullRange)>::max() / fullRange, minimumRange);
  stateMask = fullRange - 1;
  low = 0;
  high = stateMask;
}

ArithmeticCoderBase::~ArithmeticCoderBase() {}

void ArithmeticCoderBase::update(uint32_t total, uint32_t symLow,
                                 uint32_t symHigh) {
  uint64_t range = high - low + 1;
  uint64_t newLow = low + symLow * range / total;
  uint64_t newHigh = low + symHigh * range / total - 1;
  low = newLow;
  high = newHigh;

  while (((low ^ high) & halfRange) == 0) {
    shift();
    low = ((low << 1) & stateMask);
    high = ((high << 1) & stateMask) | 1;
  }

  while ((low & ~high & quarterRange) != 0) {
    underflow();
    low = (low << 1) ^ halfRange;
    high = ((high ^ halfRange) << 1) | halfRange | 1;
  }
}

ArithmeticEncoder::ArithmeticEncoder(int numBits, BitOutputStream &out)
    : ArithmeticCoderBase(numBits), output(out), numUnderflow(0) {}

void ArithmeticEncoder::write(uint32_t total, uint32_t symLow,
                              uint32_t symHigh) {
  update(total, symLow, symHigh);
}

void ArithmeticEncoder::finish() { output.write(1); }

void ArithmeticEncoder::shift() {
  int bit = static_cast<int>(low >> (numStateBits - 1));
  output.write(bit);

  for (; numUnderflow > 0; numUnderflow--)
    output.write(bit ^ 1);
}

void ArithmeticEncoder::underflow() { numUnderflow++; }
