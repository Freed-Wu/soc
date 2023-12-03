#pragma once

#include <cstdint>
<<<<<<< HEAD
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
    using namespace std;


=======
#include <math.h>
#include <vector>
using namespace std;

>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
typedef double prob_t;
typedef double mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
<<<<<<< HEAD
  uint32_t freqs_resolution = 1e6;
} gmm_t;

class GmmTable {
public:
  vector<uint32_t> symlow, symhigh;
  uint64_t total_freqs = 0;
  uint32_t low_bound = 0, high_bound = 65536;

public:
  double normal_cdf(double index, double mean, double std);
  GmmTable(gmm_t *_gmm, uint32_t _low_bound = 0, uint32_t _high_bound = 65536);
  std::uint32_t getSymbolLimit() const;
};

=======
  uint32_t freqs_resolution;
} gmm_t;

class GmmTable {
public:
  vector<uint32_t> symlow, symhigh;
  uint64_t total_freqs = 0;
  uint32_t low_bound = 0, high_bound = 65536;

public:
  double normal_cdf(double index, double mean, double std);
  GmmTable(gmm_t *gmm, uint32_t _low_bound = 0, uint32_t _high_bound = 65536);
  std::uint32_t getSymbolLimit() const;
};
>>>>>>> be19f6e4c39d9cd7312f25b2c2d291aea1a494f1
