#ifndef AXI_TANGXI_API
#define AXI_TANGXI_API

#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define INTERRUPT_ID XPAR_FABRIC_TX_AXI_IF_AND_NETWORK_0_TANGXI_IRQ_INTR
#define DDR_BASE_ADDR XPAR_PSU_DDR_0_S_AXI_BASEADDR
// #define  TX_DATA_BASE (DDR_BASE_ADDR + 0x10000000)
// #define  RX_DATA_BASE (DDR_BASE_ADDR + 0x11000000)

#define DMA_MM2S_PS_ADDR 0x80000000
#define DMA_MM2S_PS_SIZE 0x80000004
#define NOC_DATA_PKG_ADDR 0x80000008
#define NOC_DATA_PKG_CTL 0x8000000C
#define NOC_CTL_PKG_CTL 0x80000010
#define CSRAM_WRITE_CTL 0x80000014
#define DMA_S2MM_PS_ADDR 0x80000018
#define DMA_S2MM_PS_SIZE 0x8000001C
#define RESULT_READ_ADDR 0x80000020
#define RESULT_READ_CTL 0x80000024
#define TX_RISCV_RESET_CTL 0x80000028
#define PL_IRQ_TASK_NUM 0x8000002C
#define DMA_MM2S_PL_ADDR 0x80000030
#define DMA_MM2S_PL_SIZE 0x80000034
#define DMA_S2MM_PL_ADDR 0x80000038
#define DMA_S2MM_PL_SIZE 0x8000003C
#define NODE_OCCUPY_BEGIN 0x80001000 // 1KB 'h1000-'h13FC
#define TASK_RECORD_BEGIN 0x80001400 // 1KB 'h1400-'h17FC

/*变换网络*/
#define ENGINE_CR 0x80000100
#define ENGINE_WA 0x80000104
#define ENGINE_WS 0x80000108
#define ENGINE_QA 0x8000010C
#define ENGINE_QS 0x80000110
#define ENGINE_PA 0x80000114
#define ENGINE_PS 0x80000118
#define ENGINE_TA 0x8000011C
#define ENGINE_TS 0x80000120
#define ENGINE_EA 0x80000124
#define ENGINE_ES 0x80000128

#define BURST_SIZE                                                             \
  65536 // 突发长度16位，最大表示65536长度, 65536*128 bit = 1MB  = size/16

#define MM2S_PS_DONE 256     // MM2S_PS传输完成
#define S2MM_PS_DONE 257     // S2MM_PS传输完成
#define MM2S_PL_DONE 258     // MM2S_PL传输完成
#define S2MM_PL_DONE 259     // S2MM_PL传输完成
#define REQ_DATA_DONE 260    // NI发包完成（req data）
#define READ_RESULT_DONE 261 // Read Result传输完成
#define DDR_RW_DONE 262      // DDR_RW读写任务完成
#define NETWORK_ACC_DONE 263 // 网络加速器处理完成

// 4K字节对齐
#define ADDR_ALIGN_BASE_4096 0x1000
#define ADDR_ALIGN_BASE_4096_MASK 0x0FFF
#define ADDR_ALIGNED_4096(addr)                                                \
  (UINTPTR)(((addr) + ADDR_ALIGN_BASE_4096_MASK) & (~ADDR_ALIGN_BASE_4096_MASK))

// 16字节对齐
#define ADDR_ALIGN_BASE_16 0x0010
#define ADDR_ALIGN_BASE_16_MASK 0x000F
#define ADDR_ALIGNED_16(addr)                                                  \
  (UINTPTR)(((addr) + ADDR_ALIGN_BASE_16_MASK) & (~ADDR_ALIGN_BASE_16_MASK))

typedef struct {
  // 必须配置
  int tx_data_size; // 发送数据大小
  int rx_data_size; // 接收数据大小

  int burst_size;           // 突发长度
  int burst_count;          // 突发次数
  int burst_data;           // 一次突发数据大小
  uint32_t *tx_data_ps_ptr; // 突发发送地址(PS DDR地址)
  uint32_t *rx_data_ps_ptr; // 突发接收地址(PS DDR地址)
  uint32_t *tx_data_pl_ptr; // 突发发送地址(PL DDR地址)
  uint32_t *rx_data_pl_ptr; // 突发接收地址(PL DDR地址)

  uint8_t node; // 芯片node节点
} AxiTangxi_args;

// 变换网络寄存器结构体
typedef struct {
  uint32_t control;
  uint32_t weight_addr;
  uint32_t weight_size;
  uint32_t quantify_addr;
  uint32_t quantify_size;
  uint32_t picture_addr;
  uint32_t picture_size;
  uint32_t trans_addr;
  uint32_t trans_size;
  uint32_t entropy_addr;
  uint32_t entropy_size;
} iwave_args;

// 向特定地址写入一个 32 位无符号整数
void WR_u32(uint32_t data, UINTPTR addr);
// 从特定地址读取一个 32 位无符号整数的数据
uint32_t RD_u32(UINTPTR addr);

// 读取AXI-TANGXI控制系统中断号
uint32_t get_irq();
// 清除AXI-TANGXI控制系统中断
void clean_irq();
// 裸机专用函数，断开XScuGic中断控制器
void disable_axi_intr();
// 设置中断
int set_intc();

// 初始化通用参数与输入数据地址
int init_axi(AxiTangxi_args *axi_args);

// ps ddr --> pl --> tangxi
void mm2s_ps_data_tangxi(uint32_t tx_data_ps_addr, uint32_t rx_data_pl_addr,
                         int burst_size, uint32_t chip_position, uint8_t node);

void batch_mm2s_ps_data_tangxi(AxiTangxi_args *axi_args);

// tangxi --> pl --> ps ddr
void s2mm_ps_data_tangxi(uint32_t rx_data_ps_addr, int burst_size);
// ps ddr --> pl ddr
void mm2s_ps_data_s2mm_pl(uint32_t tx_data_ps_addr, uint32_t rx_data_pl_addr,
                          int burst_size);
// pl ddr --> ps ddr
void s2mm_ps_data_mm2s_pl(uint32_t rx_data_ps_addr, uint32_t tx_data_pl_addr,
                          int burst_size);

// PL DDR --> PS DDR
void batch_s2mm_ps_data_mm2s_pl(AxiTangxi_args *axi_args);

// PS DDR --> PL DDR
void batch_mm2s_ps_data_s2mm_pl(AxiTangxi_args *axi_args);

// ps ddr --> pl --> tangxi --> pl --> ps ddr 回环
int axi_tangxi_loopback(AxiTangxi_args axi_args);
// ps ddr --> pl ddr
int axi_ps_pl_ddr(AxiTangxi_args axi_args);
int axi_pl_ps_ddr(AxiTangxi_args axi_args);

// 变换网络
void iwave_args_config(iwave_args *args);
void get_iwave_args(iwave_args *args);
void iwave_start(iwave_args *args);

#endif
