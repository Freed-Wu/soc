### GmmTable.h

#### 1. Gmm结构体

  混合高斯模型。

  来自文件“coding.h"，现在搬到”GmmTable.h"
```

typedef double prob_t;
typedef double mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
  uint32_t freqs_resolution;
} gmm_t;
```

#### 2. GmmTable

  **由构造和函数GmmTable可知，gmm和上下边界得到频率表**
  频率表：编解码都需要频率表，特别是解码
```
class GmmTable{
public:
    vector<uint32_t> symlow,symhigh;
    uint64_t total_freqs=0;
    uint32_t low_bound=0,high_bound=65536;
public:
    double normal_cdf(double index, double mean, double std);
    GmmTable (gmm_t* gmm,uint32_t _low_bound=0,uint32_t _high_bound=65536);
    std::uint32_t getSymbolLimit() const;
};
```
### ArithmeticCoder.hpp
#### 1. 编码器ArithmeticCoder

#### 2. 解码器ArithmeticDecoder

**提供编码接口**

```
extern "C" size_t coding(gmm_t* gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound)
```

**提供解码接口**

```
extern "C" bool decoding(gmm_t* gmm,uint32_t low_bound,uint32_t high_bound,char binFile[],char decFile[])
```