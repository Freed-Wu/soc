#include <limits>
#include <stdexcept>
#include <string>
#include "BitIoStream.hpp"


BitInputStream::BitInputStream(std::istream &in) :
	input(in),
	currentByte(0),
	numBitsRemaining(0) {}
	
	
int BitInputStream::read() {
	if (currentByte == std::char_traits<char>::eof())
		return -1;
	if (numBitsRemaining == 0) {
		currentByte = input.get();  // Note: istream.get() returns int, not char
		if (currentByte == std::char_traits<char>::eof())
			return -1;
		if (!(0 <= currentByte && currentByte <= 255))
			throw std::logic_error("Assertion error");
		numBitsRemaining = 8;
	}
	if (numBitsRemaining <= 0)
		throw std::logic_error("Assertion error");
	numBitsRemaining--;
	return (currentByte >> numBitsRemaining) & 1;
}


int BitInputStream::readNoEof() {
	int result = read();
	if (result != -1)
		return result;
	else
		throw std::runtime_error("End of stream");
}



BitOutputStream::BitOutputStream(uint8_t* _addr) :
	currentByte(0),
	numBitsFilled(0) ,
	data_addr(_addr) {}

void BitOutputStream::write(int b) {
	if (b != 0 && b != 1)
		throw std::domain_error("Argument must be 0 or 1");
	currentByte = (currentByte << 1) | b;
	numBitsFilled++;
	if (numBitsFilled == 8) {
		// Note: ostream.put() takes char, which may be signed/unsigned
		if (std::numeric_limits<char>::is_signed)
			currentByte -= (currentByte >> 7) << 8;
		data_addr[size++]=static_cast<uint8_t>(currentByte);
		currentByte = 0;
		numBitsFilled = 0;
	}
}


void BitOutputStream::finish() {
	while (numBitsFilled != 0)
		write(0);
}
