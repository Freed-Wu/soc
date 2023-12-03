<<<<<<< HEAD
#include"GmmTable.h"
#include <iostream>
#include <math.h>


double GmmTable::normal_cdf(double index, double mean, double std) {
    return 1.0 / 2 * (1 + erf((index - mean) / std / sqrt(2)));
}


GmmTable::GmmTable (gmm_t* gmm,uint32_t _low_bound,uint32_t _high_bound){
    low_bound=_low_bound;
    high_bound=_high_bound;//左闭右闭
    uint32_t freqs_resolution = gmm->freqs_resolution; 

    symlow.resize(high_bound + 2);
    symhigh.resize(high_bound + 2);

    int n_gauss=3;
    double m_probs[3]={gmm->prob1,gmm->prob2, gmm->prob3};
    double prob_all=0.0;

    for(int i=0;i<n_gauss;i++)
        prob_all+= exp(m_probs[i]);
    for(int i=0;i<n_gauss;i++)
        m_probs[i]/=prob_all;

    double m_means[3]={gmm->mean1,gmm->mean2,gmm->mean3};
    double m_stds[3]={gmm->std1,gmm->std2,gmm->std3};
    

    for(int i=low_bound;i<=high_bound+1;i++){
        double lowboundlow=0.,highboundhigh=0.;
        
        for(int j=0;j<n_gauss;j++){
            lowboundlow+= m_probs[j]*normal_cdf(-0.5+i,m_means[j],m_stds[j]);
            highboundhigh+= m_probs[j]*normal_cdf(0.5+i,m_means[j],m_stds[j]);
        }

        symlow[i]=total_freqs;
        int dif=int((highboundhigh-lowboundlow)*freqs_resolution);
        dif=max(dif,1);
        total_freqs+=dif;
        symhigh[i]=total_freqs;
    }
}

uint32_t GmmTable::getSymbolLimit() const{
	return static_cast<uint32_t>(symlow.size());
}

=======
#include "GmmTable.h"
#include <iostream>
#include <math.h>

double GmmTable::normal_cdf(double index, double mean, double std) {
  return 1.0 / 2 * (1 + erf((index - mean) / std / sqrt(2)));
}

GmmTable::GmmTable(gmm_t *gmm, uint32_t _low_bound, uint32_t _high_bound) {
  low_bound = _low_bound;
  high_bound = _high_bound; // 左闭右闭
  uint32_t freqs_resolution = gmm->freqs_resolution;

  symlow.resize(high_bound + 2);
  symhigh.resize(high_bound + 2);

  double m_probs[3] = {gmm->prob1, gmm->prob2, gmm->prob3};
  double m_means[3] = {gmm->mean1, gmm->mean2, gmm->mean3};
  double m_stds[3] = {gmm->std1, gmm->std2, gmm->std3};
  int n_gauss = 3;

  for (int i = low_bound; i <= high_bound + 1; i++) {
    double lowboundlow = 0., highboundhigh = 0.;

    for (int j = 0; j < n_gauss; j++) {
      lowboundlow += m_probs[j] * normal_cdf(-0.5 + i, m_means[j], m_stds[j]);
      highboundhigh += m_probs[j] * normal_cdf(0.5 + i, m_means[j], m_stds[j]);
    }

    symlow[i] = total_freqs;
    int dif = int((highboundhigh - lowboundlow) * freqs_resolution);
    dif = max(dif, 1);
    total_freqs += dif;
    symhigh[i] = total_freqs;
  }
}

uint32_t GmmTable::getSymbolLimit() const {
  return static_cast<uint32_t>(symlow.size());
}
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
