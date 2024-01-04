#### 1. gmm_t

原本

```
typedef double prob_t;
typedef double mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
} gmm_t;
```

现在

```
typedef uint16_t prob_t;
typedef uint16_t mean_t;
typedef mean_t std_t;

typedef struct {
  prob_t prob1, prob2, prob3;
  mean_t mean1, mean2, mean3;
  std_t std1, std2, std3;
  uint32_t freqs_resolution;
} gmm_t;
```

1. 改变了prob_t，mean_t， std_t的数据类型。**从double变成uint_16**，这是因为整数运算，传入的九个参数从浮点数变为16bit整数。
2. **增加变量freqs_resolution**，作为分辨率。

#### 2. GmmTable

去掉了softmax，以及noraml_cdf。因为都涉及到浮点运算

```
 double normal_cdf(double index, double mean, double std);
```

**改成查表运算**。表存储在\*\*'src/data'\*\*目录下，**相对路径**如下：

```
    char exp_file_path[255] = "data/exp.bin";
    char cdf_file_path[255] = "data/cdf.bin";
```

总体设计为：

```
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
```

#### 3. 编码

下面是编码流程（可先不看）

```
extern "C" size_t coding(gmm_t* gmm,uint16_t* trans_addr,size_t trans_len,uint8_t* data_addr,uint32_t low_bound,uint32_t high_bound){
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
	// char outputFile[255]="D:/code/codec/IO/enc.bin";
	// write_bin(data_addr,size,outputFile);

	return size;
}
```

注意到下面被注释的代码，实际流程是，**1）码流写入到数组 2）数组数据写入到bin文件**。

之所以要bin文件，是因为为解码服务，**方便检测编解码一致性**。

```
	//下面两行代码是将字符数组转换为bin文件。
	// 实际中并不需要下面代码，这里得到bin文件，只是为了解码，从而验证编解码的正确性
	// char outputFile[255]="D:/code/codec/IO/enc.bin";
	// write_bin(data_addr,size,outputFile);
```

#### 4. 解码

下面是解码流程，具体为：从bin文件取数据，然后解码，写入txt文件。这里，每写入一个数据，间隔一个空格。

\*\*上述过程，仅为暂定（等实际接口）。\*\*可能后续不是写入txt文件，而是写入数组里。

```
extern "C" bool decoding(gmm_t* gmm,uint32_t low_bound,uint32_t high_bound,char binFile[],char decFile[]){
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
```

#### 5. 精度设计

##### 1. M441提案

###### 1.1. exp

- **纵轴 (y):** `int16`，范围 \[0, 20000\]
- **横轴 (x):** 区间 \[-11, 0\]，共有 11001 个点

###### 1.2. erf

- **纵轴 (y):** `int32`，范围 \[0, 99999\]
- **横轴 (x):** 区间 \[-4.5, 4.5\]，共有 45001 个点

##### 2. 航天

###### 2.1. exp

- **纵轴 (y):** `uint16`，范围 \[0, 65535\]
- **横轴 (x):** 区间 \[-12, 0\]，精度 1000，共有 12001 个点

代码:

```python
    k = int(65535 * exp_x)
    print(f'e^{x} = {exp_x}          {k}')
```

输出:

```
e^-12.0 = 6.144212353e-6          0.4088051689
e^-11.0 = 1.670170079e-5          1.111247662
```

###### 2.2. erf

- **纵轴 (y):** `uint32`，范围 \[0, UINT32_MAX\]
- **横轴 (x):** 区间 \[-5, 0\]，精度 10000，共有 50001 个点

代码:

```python
    k = int(1e6 * (1 - probability))
    print(f"Probability  {i} standard deviations (SymPy): {probability:.10f}     {k}")
```

输出:

```
Probability  4.5 standard deviations (SymPy): 0.9999932047     6.79534624947742
Probability  5.0 standard deviations (SymPy): 0.9999994267     0.573303143736048
```
