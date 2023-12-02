#include "arithmetic_coding.h"

void BitOutputStream::write(char b) {
  // std::cout << b;
  if (b != 0 and b != 1)
    throw "Argument must be 0 or 1";
  m_currentbyte = (m_currentbyte << 1) | b;
  m_numbitsfilled += 1;
  if (m_numbitsfilled == 8) {
    const char *towrite = &m_currentbyte;
    m_bit_out.write(towrite, 1);
    m_currentbyte = 0;
    m_numbitsfilled = 0;
  }
}

void BitOutputStream::close() {
  while (m_numbitsfilled != 0)
    write(0);

  m_bit_out.close();
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
