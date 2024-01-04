#pragma once

#include <cstdint>
#include <vector>
#include<math.h>
using namespace std;


// typedef double prob_t;
// typedef double mean_t;
// typedef mean_t std_t;

typedef uint16_t prob_t;
typedef uint16_t mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
  uint32_t freqs_resolution;
} gmm_t;

class GmmTable{
public:
    vector<uint32_t> symlow,symhigh;
    uint64_t total_freqs=0;
    uint32_t low_bound=0,high_bound=65536;
    char exp_file_path[255] = "data/exp.bin";
    char cdf_file_path[255] = "data/cdf.bin";
public:
    GmmTable (gmm_t* gmm,uint32_t _low_bound,uint32_t _high_bound);
    std::uint32_t getSymbolLimit() const;
};

