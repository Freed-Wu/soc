#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include "GmmTable.h"
#include <stddef.h> 

// 模拟输入数据,数值0-65535
size_t trans_len = 640;
uint16_t trans_addr [640];	
//模拟输出数据,由于是字符数组，0-255，故八位
uint8_t data_addr[2400];

//模拟GMM：权重，均值，方差，分辨率
double probs[]={0.1,0.15,0.25,0.25,0.15,0.1};
double means[]={0,10000,16384,32768,43690,65536};
double stds[] ={5000,15000,20000,40000,15000,5000};
uint32_t freqs_resolution = 1e9;
// 上下界
uint16_t low_bound=0;
uint16_t high_bound=65535;

int main(int argc, char *argv[]) {

	// 写入生成数据。
	cin>>low_bound>>high_bound;
	for (uint16_t i = 0; i < trans_len; i++) 
		trans_addr[i] = 1*(high_bound-low_bound)*i/trans_len+low_bound;


	//初始化混合高斯模型
	Gmm gmm =initGmm(probs,means,stds,6,freqs_resolution);


    //*****编码******
    // 编码结果顺便写入"IO\enc.bin",实际是不会写入的
    size_t len=coding(gmm,trans_addr,trans_len,data_addr,low_bound,high_bound);

	char *binFile  = argv[2];
	char *decFile = argv[3];
    //*****解码******
	bool ans=decoding(gmm,low_bound,high_bound,binFile,decFile);

	return 0;
}