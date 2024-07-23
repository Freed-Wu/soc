/**
 * 跟iwave完全对应:
 * 需要求出每一个的概率的原因是需要总的频率和
 * 如果不求出就很难准确求出
 */
#include <syslog.h>         
#include <cstring>
#include <cstdint>
#include <string>
#include <stddef.h> 
#include <ctime>
#include <dirent.h>
#include <random>
#include "coding.h"  
#include "arithmetic_coding.h"

extern "C" size_t coding(gmm_t* gmm,int16_t* trans_addr,size_t trans_len,uint8_t* data_addr,int16_t low_bound,int16_t high_bound){
	BitOutputStream bout(data_addr);	
	ArithmeticEncoder enc(32, bout);

	uint32_t freqs_resolution=1000000;
	EncTable freqs_table(freqs_resolution,low_bound,high_bound);

	for(size_t i=0;i<trans_len;i++,gmm++){
		int m_probs[3]={gmm->prob1, gmm->prob2,  gmm->prob3};
		int m_means[3]={gmm->mean1, gmm->mean2, gmm->mean3};
		int m_stds[3]={gmm->std1, gmm->std2, gmm->std3};
		freqs_table.update(m_probs,m_means,m_stds);

		int symbol = trans_addr[i];
		freqs_table.get_bound(symbol);
		int total_freqs=freqs_table.total_freqs,symlow=freqs_table.sym_low,symhigh=freqs_table.sym_high;
		enc.write(total_freqs, symlow, symhigh);
	}

	enc.finish();
	bout.finish();
	size_t size=bout.size;

	return size;
}
