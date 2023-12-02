#include"GmmTable.h"
#include <iostream>
#include <math.h>

Gmm initGmm(double* _probs, double* _means, double* _stds,int _len, uint32_t _freqs_resolution) {
    Gmm gmm;
    gmm.probs = _probs;
    gmm.means = _means;
    gmm.stds = _stds;
    gmm.freqs_resolution = _freqs_resolution;
    gmm.len = _len;
   
    double prob_all=0.0;

    for(int i=0;i<_len;i++)
        prob_all+= exp(gmm.probs[i]);
    
    for(int i=0;i<_len;i++)
        gmm.probs[i]/=prob_all;

    return gmm;
}


double GmmTable::normal_cdf(double index, double mean, double std) {
    return 1.0 / 2 * (1 + erf((index - mean) / std / sqrt(2)));
}


GmmTable::GmmTable (Gmm _gmm,uint32_t _low_bound,uint32_t _high_bound){
    gmm=_gmm;
    low_bound=_low_bound;
    high_bound=_high_bound;//左闭右闭
    uint32_t freqs_resolution = gmm.freqs_resolution; 

    symlow.resize(high_bound + 2);
    symhigh.resize(high_bound + 2);
    
    double* m_probs=gmm.probs;
    double* m_means=gmm.means;
    double* m_stds=gmm.stds;
    int n_gauss=gmm.len;

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

