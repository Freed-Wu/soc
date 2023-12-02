#pragma once

#include <cstdint>
#include <vector>
#include<math.h>
using namespace std;

// 定义结构体
typedef struct {
    double* probs;
    double* means;
    double* stds;
    int len;
    uint32_t freqs_resolution;
} Gmm;

Gmm initGmm(double* _probs, double* _means, double* _stds,int _len, uint32_t _freqs_resolution);

class GmmTable{
public:
    vector<double> m_probs,m_means,m_stds;
    vector<uint32_t> symlow,symhigh;
    uint64_t total_freqs=0;
    uint32_t low_bound=0,high_bound=65536;
    Gmm gmm;
public:
    double normal_cdf(double index, double mean, double std);
    GmmTable (Gmm _gmm,uint32_t _low_bound=0,uint32_t _high_bound=65536);
    std::uint32_t getSymbolLimit() const;
};

