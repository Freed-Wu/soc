#pragma once

#include <algorithm>
#include <cstdint>
#include "BitIoStream.hpp"
#include "GmmTable.h"

class ArithmeticCoderBase {
	protected: int numStateBits;
		std::uint64_t fullRange;
		std::uint64_t halfRange;
		std::uint64_t quarterRange;
		std::uint64_t minimumRange;
		std::uint64_t maximumTotal;
		std::uint64_t stateMask;
		std::uint64_t low;
		std::uint64_t high;
	public: 
		explicit ArithmeticCoderBase(int numBits);
		virtual ~ArithmeticCoderBase() = 0;
		virtual void update(const GmmTable &freqs, std::uint32_t symbol);
		virtual void shift() = 0;
		virtual void underflow() = 0;
};


class ArithmeticDecoder final : private ArithmeticCoderBase {
	private: 
		BitInputStream &input;
		std::uint64_t code;
	public: 
		explicit ArithmeticDecoder(int numBits, BitInputStream &in);
		std::uint32_t read(const GmmTable &freqs);
		void shift() override;
		void underflow() override;
		int readCodeBit();
};



class ArithmeticEncoder final : private ArithmeticCoderBase {
	public: 
		BitOutputStream &output;
		unsigned long numUnderflow;
		explicit ArithmeticEncoder(int numBits, BitOutputStream &out);
		void write(const GmmTable &freqs, std::uint32_t symbol);
		void finish();
		void shift() override;
		void underflow() override;
};

bool write_bin(uint8_t* data_addr,size_t size,char file[]);

extern "C" size_t coding(gmm_t* gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound);

extern "C" bool decoding(gmm_t* gmm,uint32_t low_bound,uint32_t high_bound,char binFile[],char decFile[]);