### GmmTable.h
1. Gmm结构体
混合高斯模型。用写构体写，方便C调用
```
typedef struct {
    double* probs;
    double* means;
    double* stds;
    size_t len;//数组长度
    uint32_t freqs_resolution;
} Gmm;
```
初始函数
```
Gmm initGmm(double* _probs, double* _means, double* _stds,size_t _len, uint32_t _freqs_resolution) 
```
2. GmmTable
频率表：编解码都需要频率表，特别是解码
初始函数
```
GmmTable (Gmm _gmm,uint32_t _low_bound,uint32_t _high_bound)
```
### ArithmeticCoder.hpp
1. 编码器ArithmeticCoder
2. 解码器ArithmeticDecoder

**提供编码接口**

```
extern "C" size_t coding(Gmm gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound)
```

**提供解码接口**

```
extern "C" bool decoding(Gmm gmm,uint32_t low_bound,uint32_t high_bound,char binFile[],char decFile[])
```

## test.cpp

模拟整个编码和解码过程：生成数据，编码数据，解码数据

**Cmaketxts**

```
cmake_minimum_required(VERSION 3.10)

project(ArithmeticCoding)

add_executable(test 
    test.cpp 
    ArithmeticCoder.cpp  
    GmmTable.cpp
)



set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

