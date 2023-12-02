#include <limits>
#include <stdexcept>
#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "GmmTable.h"
#include <stddef.h> 
using std::uint32_t;
using std::uint64_t;


ArithmeticCoderBase::ArithmeticCoderBase(int numBits) {
	if (!(1 <= numBits && numBits <= 63))
		throw std::domain_error("State size out of range");
	numStateBits = numBits;
	fullRange = static_cast<decltype(fullRange)>(1) << numStateBits;
	halfRange = fullRange >> 1;  // Non-zero
	quarterRange = halfRange >> 1;  // Can be zero
	minimumRange = quarterRange + 2;  // At least 2
	maximumTotal = std::min(std::numeric_limits<decltype(fullRange)>::max() / fullRange, minimumRange);
	stateMask = fullRange - 1;
	low = 0;
	high = stateMask;
}


ArithmeticCoderBase::~ArithmeticCoderBase() {}


void ArithmeticCoderBase::update(const GmmTable &freqs, uint32_t symbol) {
	// State check
	if (low >= high || (low & stateMask) != low || (high & stateMask) != high)
		throw std::logic_error("Assertion error: Low or high out of range");
	uint64_t range = high - low + 1;
	if (!(minimumRange <= range && range <= fullRange))
		throw std::logic_error("Assertion error: Range out of range");
	
	// Frequency table values check
	uint32_t total = freqs.total_freqs;
	uint32_t symLow = freqs.symlow[symbol];
	uint32_t symHigh = freqs.symhigh[symbol];
	if (symLow == symHigh)
		throw std::invalid_argument("Symbol has zero frequency");
	if (total > maximumTotal)
		throw std::invalid_argument("Cannot code symbol because total is too large");
	
	// Update range
	uint64_t newLow  = low + symLow  * range / total;
	uint64_t newHigh = low + symHigh * range / total - 1;
	low = newLow;
	high = newHigh;
	
	// While low and high have the same top bit value, shift them out
	while (((low ^ high) & halfRange) == 0) {
		shift();
		low  = ((low  << 1) & stateMask);
		high = ((high << 1) & stateMask) | 1;
	}
	// Now low's top bit must be 0 and high's top bit must be 1
	
	// While low's top two bits are 01 and high's are 10, delete the second highest bit of both
	while ((low & ~high & quarterRange) != 0) {
		underflow();
		low = (low << 1) ^ halfRange;
		high = ((high ^ halfRange) << 1) | halfRange | 1;
	}
}


ArithmeticDecoder::ArithmeticDecoder(int numBits, BitInputStream &in) :
		ArithmeticCoderBase(numBits),
		input(in),
		code(0) {
	for (int i = 0; i < numStateBits; i++)
		code = code << 1 | readCodeBit();
}


uint32_t ArithmeticDecoder::read(const GmmTable &freqs) {
	// Translate from coding range scale to frequency table scale
	uint32_t total = freqs.total_freqs;
	if (total > maximumTotal)
		throw std::invalid_argument("Cannot decode symbol because total is too large");
	uint64_t range = high - low + 1;
	uint64_t offset = code - low;// 200-100=100
	uint64_t value = ((offset + 1) * total - 1) / range;  //eg 100 100/288 A:90-101 B:102-110
	if (value * range / total > offset)
		throw std::logic_error("Assertion error");
	if (value >= total)
		throw std::logic_error("Assertion error");
	
	// A kind of binary search. Find highest symbol such that freqs.getLow(symbol) <= value.
	uint32_t start = 0;
	uint32_t end = freqs.getSymbolLimit();
	while (end - start > 1) {
		uint32_t middle = (start + end) >> 1;
		if (freqs.symlow[middle] > value)
			end = middle;
		else
			start = middle;
	}
	if (start + 1 != end)
		throw std::logic_error("Assertion error");
	
	uint32_t symbol = start;
	if (!(freqs.symlow[symbol] * range / total <= offset && offset < freqs.symhigh[symbol] * range / total))
		throw std::logic_error("Assertion error");
	update(freqs, symbol);
	if (!(low <= code && code <= high))
		throw std::logic_error("Assertion error: Code out of range");
	return symbol;
}


void ArithmeticDecoder::shift() {
	code = ((code << 1) & stateMask) | readCodeBit();
}


void ArithmeticDecoder::underflow() {
	code = (code & halfRange) | ((code << 1) & (stateMask >> 1)) | readCodeBit();
}


int ArithmeticDecoder::readCodeBit() {
	int temp = input.read();
	if (temp == -1)
		temp = 0;
	return temp;
}


ArithmeticEncoder::ArithmeticEncoder(int numBits, BitOutputStream &out) :
	ArithmeticCoderBase(numBits),
	output(out),
	numUnderflow(0) {}


void ArithmeticEncoder::write(const GmmTable &freqs, uint32_t symbol) {
	update(freqs, symbol);
}


void ArithmeticEncoder::finish() {
	output.write(1);
}


void ArithmeticEncoder::shift() {
	int bit = static_cast<int>(low >> (numStateBits - 1));
	output.write(bit);
	
	// Write out the saved underflow bits
	for (; numUnderflow > 0; numUnderflow--)
		output.write(bit ^ 1);
}


void ArithmeticEncoder::underflow() {
	if (numUnderflow == std::numeric_limits<decltype(numUnderflow)>::max())
		throw std::overflow_error("Maximum underflow reached");
	numUnderflow++;
}


bool write_bin(uint8_t* data_addr,size_t size,char file[]){
	std::ofstream outputFile(file, std::ios::binary);

    if (!outputFile.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return false;
    }

    outputFile.write(reinterpret_cast<char*>(data_addr), size);
    outputFile.close();
    return true;
}

extern "C" size_t coding(Gmm gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound){
	//根据GMM得到对应的频率表
    GmmTable freqs(gmm,low_bound,high_bound);
	
	BitOutputStream bout(data_addr);	

	//读取数据。根据频率表进行算术编码
	ArithmeticEncoder enc(32, bout);

	for(int i=0;i<trans_len;i++){
		int symbol = trans_addr[i];
		if (symbol == std::char_traits<char>::eof())
			break;
		if (!(low_bound <= symbol && symbol <= high_bound))
			throw std::logic_error("Assertion error");
		enc.write(freqs, static_cast<uint32_t>(symbol));
	}

	// EOF，值域属于low_bound-high_bound，故而high_bound+1是终止符号	
	enc.write(freqs, high_bound+1);  
	enc.finish();
	bout.finish();
	size_t size=bout.size;


	//下面两行代码是将字符数组转换为bin文件。
	// 实际中并不需要下面代码，这里得到bin文件，只是为了解码，从而验证编解码的正确性
	char outputFile[255]="IO/enc.bin";
	write_bin(data_addr,size,outputFile);

	return size;
}

extern "C" bool decoding(Gmm gmm,uint32_t low_bound,uint32_t high_bound,char binFile[],char decFile[]){
	//根据GMM得到对应的频率表
	GmmTable freqs(gmm,low_bound,high_bound);

	std::ifstream in(binFile, std::ios::binary);
	std::ofstream out(decFile, std::ios::binary);
	BitInputStream bin(in);
	try {
		ArithmeticDecoder dec(32, bin);
		while (true) {
			uint32_t symbol = dec.read(freqs);
			if (symbol > high_bound)  // EOF symbol
				break;
			int b = static_cast<int>(symbol);
			out << b << ' ';
		}
		return EXIT_SUCCESS;
		
	} catch (const char *msg) {
		std::cerr << msg << std::endl;
		return EXIT_FAILURE;
	}
}