#pragma once

#include <algorithm>
#include <cstdint>
#include "BitIoStream.hpp"
#include "GmmTable.h"

/* 
 * Provides the state and behaviors that arithmetic coding encoders and decoders share.
 */
class ArithmeticCoderBase {
	protected: int numStateBits;
	
	// Maximum range (high+1-low) during coding (trivial), which is 2^numStateBits = 1000...000.
	protected: std::uint64_t fullRange;
	
	// The top bit at width numStateBits, which is 0100...000.
	protected: std::uint64_t halfRange;
	
	// The second highest bit at width numStateBits, which is 0010...000. This is zero when numStateBits=1.
	protected: std::uint64_t quarterRange;
	
	// Minimum range (high+1-low) during coding (non-trivial), which is 0010...010.
	protected: std::uint64_t minimumRange;
	
	// Maximum allowed total from a frequency table at all times during coding.
	protected: std::uint64_t maximumTotal;
	
	// Bit mask of numStateBits ones, which is 0111...111.
	protected: std::uint64_t stateMask;
	
	
	/*---- State fields ----*/
	
	// Low end of this arithmetic coder's current range. Conceptually has an infinite number of trailing 0s.
	protected: std::uint64_t low;
	
	// High end of this arithmetic coder's current range. Conceptually has an infinite number of trailing 1s.
	protected: std::uint64_t high;
	
	
	/*---- Constructor ----*/
	
	// Constructs an arithmetic coder, which initializes the code range.
	public: explicit ArithmeticCoderBase(int numBits);
	
	
	public: virtual ~ArithmeticCoderBase() = 0;

	protected: virtual void update(const GmmTable &freqs, std::uint32_t symbol);
	
	
	// Called to handle the situation when the top bit of 'low' and 'high' are equal.
	protected: virtual void shift() = 0;
	
	
	// Called to handle the situation when low=01(...) and high=10(...).
	protected: virtual void underflow() = 0;
	
};


class ArithmeticDecoder final : private ArithmeticCoderBase {

	private: BitInputStream &input;

	private: std::uint64_t code;

	public: explicit ArithmeticDecoder(int numBits, BitInputStream &in);
	
	public: std::uint32_t read(const GmmTable &freqs);
	
	protected: void shift() override;
	
	protected: void underflow() override;

	private: int readCodeBit();
	
};



class ArithmeticEncoder final : private ArithmeticCoderBase {

	private: BitOutputStream &output;

	private: unsigned long numUnderflow;
	
	public: explicit ArithmeticEncoder(int numBits, BitOutputStream &out);
	
	public: void write(const GmmTable &freqs, std::uint32_t symbol);

	public: void finish();
	
	protected: void shift() override;
	
	protected: void underflow() override;
};

bool write_bin(uint8_t* data_addr,size_t size,char file[]);

extern "C" size_t coding(Gmm gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound);

extern "C" bool decoding(Gmm gmm,uint32_t low_bound,uint32_t high_bound,char binFile[],char decFile[]);