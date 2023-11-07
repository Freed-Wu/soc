#pragma once
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>

class BitOutputStream {
public:
  BitOutputStream(std::fstream &file)
      : m_bit_out(file), m_currentbyte(0), m_numbitsfilled(0){};
  void write(char);
  void close();

private:
  char m_currentbyte;  // The accumulated bits for the current byte, always in
                       // the range [0x00, 0xFF]
  int m_numbitsfilled; // Number of accumulated bits in the current byte, always
                       // between 0 and 7 (inclusive)
  std::fstream &m_bit_out;
};

class BitInputStream {
public:
  BitInputStream(std::fstream &file)
      : m_bit_in(file), m_currentbyte(0), m_numbitsremaining(0){};
  int read();
  int read_no_eof();
  void close();

private:
  int m_currentbyte; // The accumulated bits for the current byte, always in the
                     // range [0x00, 0xFF]
  int m_numbitsremaining; // Number of accumulated bits in the current byte,
                          // always between 0 and 7 (inclusive)
  std::fstream &m_bit_in;
};

class CountingBitOutputStream {
public:
  CountingBitOutputStream(BitOutputStream &bit_out)
      : m_bit_out(bit_out), m_num_bits(0){};
  void close();
  void write(char);

private:
  int m_num_bits;
  BitOutputStream &m_bit_out;
};

// class FrequencyTable
//{
// public:
//	virtual void get_symbol_limit() = 0;
//	virtual void get() = 0;
//	virtual void set() = 0;
//	virtual void increment() = 0;
//	virtual void get_total() = 0;
//	virtual void get_low() = 0;
//	virtual void get_high() = 0;
// };
//
// class SimpleFrequencyTable: public FrequencyTable
//{
//	SimpleFrequencyTable()
// };

class ArithmeticCoderBase {
public:
  // Number of bits for 'low' and 'high'. Configurable and must be at least 1.
  long long STATE_SIZE = 32;
  long long symbol1 = 1;

  // Maximum range during coding (trivial), i.e. 1000...000.
  long long MAX_RANGE = symbol1 << STATE_SIZE;

  // Minimum range during coding (non-trivial), i.e. 010...010.
  long long MIN_RANGE = (MAX_RANGE >> 2) + 2;

  // Maximum allowed total frequency at all times during coding.  Ϊʲô
  long long MAX_TOTAL = MIN_RANGE;

  // Mask of STATE_SIZE ones, i.e. 111...111.
  long long MASK = MAX_RANGE - 1;

  // Mask of the top bit at width STATE_SIZE, i.e. 100...000.
  long long TOP_MASK = MAX_RANGE >> 1;

  // Mask of the second highest bit at width STATE_SIZE, i.e. 010...000.
  long long SECOND_MASK = TOP_MASK >> 1;

  ArithmeticCoderBase() : m_low(0), m_high(MASK){};
  void update(long long total, long long low, long long high, char symbol);
  virtual void shift() = 0;
  virtual void underflow() = 0;

  long long m_low;  // Low end of this arithmetic coder's current range.
                    // Conceptually has an infinite number of trailing 0s.
  long long m_high; // High end of this arithmetic coder's current range.
                    // Conceptually has an infinite number of trailing 1s.
};

class ArithmeticEncoder : public ArithmeticCoderBase {
public:
  ArithmeticEncoder(CountingBitOutputStream &c_bit_out)
      : m_c_bit_out(c_bit_out), m_num_underflow(0){};
  void write(long long, long long, long long, char);
  void finish();
  void shift();
  void underflow();

private:
  CountingBitOutputStream &m_c_bit_out;
  long long m_num_underflow;
};

class ArithmeticDecoder : public ArithmeticCoderBase {
public:
  ArithmeticDecoder(ArithmeticCoderBase &bitin);
  int read();

private:
  ArithmeticCoderBase &m_bitin;
  long long code;
};
