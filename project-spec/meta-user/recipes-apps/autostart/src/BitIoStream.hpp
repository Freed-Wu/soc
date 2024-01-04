#pragma once

#include <istream>
#include <ostream>


class BitInputStream final {
	private: 
		std::istream &input;
		int currentByte;
		int numBitsRemaining; 
	public: 
		int read();
		int readNoEof();
		explicit BitInputStream(std::istream &in);
};

class BitOutputStream final {
	private: 
		int currentByte;
		int numBitsFilled;
	public:
		uint8_t* data_addr;
		size_t size=0;
		BitOutputStream(uint8_t* _addr);	
	    void write(int b);
		void finish();
};

