#include "sleep.h"
#include "xil_cache.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xstatus.h"
#include "xtime_l.h"

#include <stdio.h>
#include <stdlib.h>

#include "axi_tangxi.h"

/**********************************Function
 * Prototypes***************************************/

static void interrupt_handler(void *callback);
static int initInterruptSystem(XScuGic *intc);
static void disableInterruptSystem(XScuGic *intc);

/**********************************Variable
 * Definitions***************************************/

static XScuGic INTC; // 创建XScuGic中断控制器实例
XTime start_t, end_t;
uint8_t ps_conf_done, pl_conf_done, ps_transfer_done, pl_transfer_done;

/**********************************Function
 * Definitions***************************************/

void WR_u32(uint32_t data, UINTPTR addr) {
  volatile uint32_t *LocalAddr = (volatile uint32_t *)addr;
  *LocalAddr = data;
}

uint32_t RD_u32(UINTPTR addr) {
  volatile uint32_t *LocalAddr = (volatile uint32_t *)addr;
  return *LocalAddr;
}

uint32_t get_irq() {
  volatile uint32_t *LocalAddr = (volatile uint32_t *)PL_IRQ_TASK_NUM;
  return *LocalAddr;
}

void clean_irq() {
  volatile uint32_t *LocalAddr = (volatile uint32_t *)PL_IRQ_TASK_NUM;
  *LocalAddr = 0;
}

// 初始化中断函数
static int initInterruptSystem(XScuGic *intc) {
  int status;
  XScuGic_Config *intc_config;

  // 初始化中断控制器
  intc_config = XScuGic_LookupConfig(INTC_DEVICE_ID);
  if (NULL == intc_config) {
    return XST_FAILURE;
  }
  status =
      XScuGic_CfgInitialize(intc, intc_config, intc_config->CpuBaseAddress);
  if (status != XST_SUCCESS) {
    return XST_FAILURE;
  }

  // XScuGic_SetPriorityTriggerType(intc, INTERRUPT_ID, 0x0, 0x3);
  // 设置中断处理函数
  status = XScuGic_Connect(intc, INTERRUPT_ID,
                           (Xil_InterruptHandler)interrupt_handler, NULL);
  if (status != XST_SUCCESS) {
    return status;
  }
  XScuGic_Enable(intc, INTERRUPT_ID);

  // 连接中断处理函数
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                               (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                               intc);
  // 使能全局中断
  Xil_ExceptionEnable();

  return XST_SUCCESS;
}

// 中断处理函数
void interrupt_handler(void *callback) {
  XTime_GetTime(&end_t);
  //	double elapsed_time = ((double)(end_t - start_t) * 1e6) /
  // COUNTS_PER_SECOND; printf("send done! elapsed_time: %.2f us.\n",
  // elapsed_time);

  uint32_t irq = get_irq();
  printf("\n irq = %d\n", irq);
  switch (irq) {

  case MM2S_PS_DONE:
    // PL读PS DDR结束
    printf("MM2S_PS_DONE, irq = %d\n", irq);
    break;
  case S2MM_PS_DONE:
    // PL写PS DDR结束：代表整个接收数据流程结束
    if (pl_conf_done) {
      pl_transfer_done = 1;
      printf("S2MM_PS_DONE, irq = %d\n", irq);
    }
    break;
  case MM2S_PL_DONE:
    // PL读PL DDR结束
    printf("MM2S_PL_DONE, irq = %d\n", irq);
    break;
  case S2MM_PL_DONE:
    // tangxi 写 PL DDR结束
    printf("S2MM_PL_DONE, irq = %d\n", irq);
    break;
  case REQ_DATA_DONE:
    // NI发包完成（req data），PL读PS
    // DDR的数发送到芯片完成，代表整个发送数据完成
    if (ps_conf_done) {
      ps_transfer_done = 1;
    }
    printf("REQ_DATA_DONE, irq = %d\n", irq);
    break;
  case READ_RESULT_DONE:
    // Read Result传输完成，tangxi芯片写 PL BRAM完成
    printf("READ_RESULT_DONE, irq = %d\n", irq);
    break;
  case NETWORK_ACC_DONE:
    // NETWORK DONE
    printf("NETWORK DONE, irq = %d\n", irq);
    break;
  }
  clean_irq();
}

static void disableInterruptSystem(XScuGic *intc) {
  XScuGic_Disconnect(intc, INTERRUPT_ID);
}

/**
 * @brief PS DDR data--> PL --> tangxi
 *
 * @param tx_data_ps_addr 读ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param rx_data_pl_addr 芯片写回pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 * @param chip_position chip号:00-01-10
 * @param node node节点号:0-255
 */
void mm2s_ps_data_tangxi(uint32_t tx_data_ps_addr, uint32_t rx_data_pl_addr,
                         int burst_size, uint32_t chip_position, uint8_t node) {
  /**
   *
   * NOC_DATA_PKG_ADDR
   * 0xnnnaaaaa  [29:20][19:0] : node_addr
   * n - node[29:20] → [9:8]表示chip_position(FPGA为2'b11,发送的包不能是2'b11),
   * [7:0]表示此包的目的节点(0-255) a - addr[19:0] → 表示写地址 0x00300100:node
   * 0x003, addr 0x00100 bram 256-4096,pl ddr 4096+
   */
  uint32_t noc_data_size_addr =
      (chip_position << 28) | (node << 20) | rx_data_pl_addr;

  /**
   * NOC_DATA_PKG_CTL
   * 0bssssssssssssssssttcccccccc  [25:10][9:8][7:0] : size_type_cmd
   * s - size[25:10] → 表示写大小（控制包为size不用管，数据包size表示数据长度）
   * t - type[9:8] → 表示此包的类型, 00:data, 01:ctrl
   * c - cmd[7:0] → 对应类型的具体命令(控制指令)
           > cmd[7:6] → ctrl_cmd , 00:reset, 01:start, 10:end, 11:riscv change
   de_mode(=addr[15]) > cmd[5:3] → sync_cmd > cmd[2:0] → ddr_cmd , 001:read,
   010:write, 100:refresh
   * 0b00 00000010:type 00, cmd 00000010
   */
  uint32_t noc_data_node_type_cmd = ((burst_size - 1) << 10) | 0b0000000010;

  // DMA_MM2S_PS_ADDR
  WR_u32(tx_data_ps_addr, (UINTPTR)DMA_MM2S_PS_ADDR);
  // DMA_MM2S_PS_SIZE
  WR_u32(burst_size - 1, (UINTPTR)DMA_MM2S_PS_SIZE);

  // NOC_DATA_PKG_ADDR
  WR_u32(noc_data_size_addr, (UINTPTR)NOC_DATA_PKG_ADDR);
  // NOC_DATA_PKG_CTL
  WR_u32(noc_data_node_type_cmd, (UINTPTR)NOC_DATA_PKG_CTL);
}

/**
 * @brief tangxi --> PL Read Result (默认存储BRAM，首地址256) --> PS ddr
 *
 * @param rx_data_ps_addr 写PS ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void s2mm_ps_data_tangxi(uint32_t rx_data_ps_addr, int burst_size) {
  // RESULT_READ_ADDR 与NOC_DATA_PKG_ADDR addr一致
  uint32_t noc_data_size_addr = ((burst_size - 1) << 20) | 0x00100;
  WR_u32((UINTPTR)noc_data_size_addr, (UINTPTR)RESULT_READ_ADDR);
  // RESULT_READ_CTL
  WR_u32(burst_size - 1, (UINTPTR)RESULT_READ_CTL);

  // DMA_S2MM_PS_ADDR
  WR_u32((UINTPTR)rx_data_ps_addr, (UINTPTR)DMA_S2MM_PS_ADDR);

  // DMA_S2MM_PS_SIZE
  WR_u32(burst_size - 1, (UINTPTR)DMA_S2MM_PS_SIZE);
}

/**
 * @brief PS DDR --> PL DDR
 *
 * @param tx_data_ps_addr 读取ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param rx_data_pl_addr 写入pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void mm2s_ps_data_s2mm_pl(uint32_t tx_data_ps_addr, uint32_t rx_data_pl_addr,
                          int burst_size) {
  // DMA_MM2S_PS_ADDR
  WR_u32(tx_data_ps_addr, (UINTPTR)DMA_MM2S_PS_ADDR);
  // DMA_MM2S_PS_SIZE
  WR_u32(burst_size - 1, (UINTPTR)DMA_MM2S_PS_SIZE);

  // DMA_S2MM_PL_ADDR
  WR_u32((UINTPTR)rx_data_pl_addr, (UINTPTR)DMA_S2MM_PL_ADDR);
  // DMA_S2MM_PL_SIZE
  WR_u32(burst_size - 1, (UINTPTR)DMA_S2MM_PL_SIZE);
}

/**
 * @brief PL DDR --> PS DDR
 *
 * @param rx_data_ps_addr 写入ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param tx_data_pl_addr 读取pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void s2mm_ps_data_mm2s_pl(uint32_t rx_data_ps_addr, uint32_t tx_data_pl_addr,
                          int burst_size) {
  // DMA_MM2S_PL_ADDR
  WR_u32((UINTPTR)tx_data_pl_addr, (UINTPTR)DMA_MM2S_PL_ADDR);
  // DMA_MM2S_PL_SIZE
  WR_u32(burst_size - 1, (UINTPTR)DMA_MM2S_PL_SIZE);

  // DMA_S2MM_PS_ADDR
  WR_u32((UINTPTR)rx_data_ps_addr, (UINTPTR)DMA_S2MM_PS_ADDR);
  // DMA_S2MM_PS_SIZE
  WR_u32(burst_size - 1, (UINTPTR)DMA_S2MM_PS_SIZE);
}

// 批量PS DDR --> tangxi --> PL DDR
void batch_mm2s_ps_data_tangxi(AxiTangxi_args *axi_args) {
  uint32_t data_remaining = axi_args->tx_data_size;

  uint32_t tx_data_ps_next = (UINTPTR)axi_args->tx_data_ps_ptr;
  uint32_t rx_data_pl_next = (UINTPTR)axi_args->rx_data_pl_ptr;

  uint32_t burst_size = axi_args->burst_size;
  uint32_t burst_data = axi_args->burst_data;

  XTime_GetTime(&start_t);
  while (data_remaining > 0) {
    // 最后一次突发
    if (data_remaining < burst_data) {
      if ((data_remaining & ADDR_ALIGN_BASE_16_MASK) != 0x00) {
        // 对齐16字节
        data_remaining +=
            (ADDR_ALIGN_BASE_16 - (data_remaining & ADDR_ALIGN_BASE_16_MASK));
      }
      burst_size = data_remaining >> 4;
      burst_data = data_remaining;
      ps_conf_done = 1;
    }
    // PS DDR data--> PL --> tangxi, 0:第0个chip, 3:第三个node, 256:req
    // data存储地址256
    mm2s_ps_data_tangxi((UINTPTR)tx_data_ps_next, (UINTPTR)rx_data_pl_next,
                        burst_size, 0, 3);
    // 等待中断信号
    tx_data_ps_next = tx_data_ps_next + burst_data;
    data_remaining -= burst_data;
  }
}

// 批量PS DDR --> PL DDR
void batch_mm2s_ps_data_s2mm_pl(AxiTangxi_args *axi_args) {
  uint32_t data_remaining = axi_args->tx_data_size;

  uint32_t tx_data_ps_next = (UINTPTR)axi_args->tx_data_ps_ptr;
  uint32_t rx_data_pl_next = (UINTPTR)axi_args->rx_data_pl_ptr;

  uint32_t burst_size = axi_args->burst_size;
  uint32_t burst_data = axi_args->burst_data;
  while (data_remaining > 0) {
    if (data_remaining < burst_data) {
      // 如果剩余的数据小于一次性突发数据，修改突发长度
      if ((data_remaining & ADDR_ALIGN_BASE_16_MASK) != 0x00) {
        // 对齐16字节
        data_remaining +=
            (ADDR_ALIGN_BASE_16 - (data_remaining & ADDR_ALIGN_BASE_16_MASK));
      }

      burst_size = data_remaining >> 4;
      burst_data = data_remaining;
      pl_conf_done = 1;
    }
    mm2s_ps_data_s2mm_pl((UINTPTR)tx_data_ps_next, (UINTPTR)rx_data_pl_next,
                         burst_size);
    tx_data_ps_next = tx_data_ps_next + burst_data;
    data_remaining -= burst_data;
  }
}

// 批量PL DDR --> PS DDR
void batch_s2mm_ps_data_mm2s_pl(AxiTangxi_args *axi_args) {
  uint32_t data_remaining = axi_args->rx_data_size;

  uint32_t rx_data_ps_next = (UINTPTR)axi_args->rx_data_ps_ptr;
  uint32_t tx_data_pl_next = (UINTPTR)axi_args->tx_data_pl_ptr;

  uint32_t burst_size = axi_args->burst_size;
  uint32_t burst_data = axi_args->burst_data;
  while (data_remaining > 0) {
    if (data_remaining < burst_data) {
      // 如果剩余的数据小于一次性突发数据，修改突发长度
      if ((data_remaining & ADDR_ALIGN_BASE_16_MASK) != 0x00) {
        // 对齐16字节
        data_remaining +=
            (ADDR_ALIGN_BASE_16 - (data_remaining & ADDR_ALIGN_BASE_16_MASK));
      }

      burst_size = data_remaining >> 4; //!!!
      burst_data = data_remaining;
      pl_conf_done = 1;
    }
    s2mm_ps_data_mm2s_pl((UINTPTR)rx_data_ps_next, (UINTPTR)tx_data_pl_next,
                         burst_size);
    rx_data_ps_next = rx_data_ps_next + burst_data;
    data_remaining -= burst_data;
  }
}

int axi_tangxi_loopback(AxiTangxi_args axi_args) {
  // PS DDR --> tangxi --> PL DDR
  batch_mm2s_ps_data_tangxi(&axi_args);

  // ps最后一组数据发送完成，从PL DDR取数
  if (ps_transfer_done) {
    batch_s2mm_ps_data_mm2s_pl(&axi_args);
  }
  if (pl_transfer_done) {
    XTime_GetTime(&end_t);
    double elapsed_time = ((double)(end_t - start_t) * 1e6) / COUNTS_PER_SECOND;
    printf("send done! elapsed_time: %.2f us.\n", elapsed_time);

    // 刷新Data Cache
    // Xil_DCacheFlushRange((UINTPTR) axi_args.rx_data_ps_ptr,
    // axi_args.tx_data_size);

    int len = axi_args.tx_data_size / sizeof(int);
    int count = 0;
    for (int i = 0; i < len; i++) {
      if (axi_args.rx_data_ps_ptr[i] != axi_args.tx_data_ps_ptr[i]) {
        count++;
        xil_printf(
            "different data! : tx address 0x%x: %u; rx address 0x%x: %u\n",
            axi_args.tx_data_ps_ptr + i, axi_args.tx_data_ps_ptr[i],
            axi_args.rx_data_ps_ptr + i, axi_args.rx_data_ps_ptr[i]);
      }
    }
    xil_printf("all different data count %u\n", count);
    //	for (int i = len - 25; i < len ; i++) {
    //		xil_printf("Data at address 0x%x: %u\n", axi_args.rx_data_ps_ptr
    //+ i, axi_args.rx_data_ps_ptr[i]);
    //	}
    //	for (int i = len - 25; i < len + 2; i++) {
    //		xil_printf("Data at address 0x%x: %u\n", axi_args.tx_data_ps_ptr
    //+ i, axi_args.tx_data_ps_ptr[i]);
    //	}

    // disableInterruptSystem(&INTC);
  }

  return 0;
}

int init_axi(AxiTangxi_args *axi_args) {
  //	int status;

  // int burst_count_temp = 0;
  //  禁用缓存，确保数据直接写入DDR
  // Xil_DCacheDisable();
  ps_conf_done = 0;
  pl_conf_done = 0;
  ps_transfer_done = 0;
  pl_transfer_done = 0;

  // 突发长度
  axi_args->burst_size = BURST_SIZE;
  // 突发次数
  axi_args->burst_count =
      (axi_args->tx_data_size - 1) / (axi_args->burst_size * 16) + 1;
  // 一次突发大小
  axi_args->burst_data = BURST_SIZE * 16;

  // UINTPTR tx_data_ps_addr = (UINTPTR)data;
  //  ps ddr传输首地址
  // axi_args->tx_data_ps_ptr = (uint32_t *)tx_data_ps_addr;

  // 接收首地址默认加0x1000000(16MB数据空间)
  UINTPTR rx_data_ps_addr = ADDR_ALIGNED_4096(
      (UINTPTR)axi_args->tx_data_ps_ptr + axi_args->tx_data_size + 0x1000000);
  // ps ddr接收首地址
  axi_args->rx_data_ps_ptr = (uint32_t *)rx_data_ps_addr;

  // pl ddr传输/接收首地址(自定义，默认与ps ddr传输首地址相同)
  axi_args->tx_data_pl_ptr = axi_args->rx_data_pl_ptr;

  //	if ((uint64_t)tx_data_ps_addr > 0xffffffff || (uint64_t)rx_data_ps_addr
  //> 0xffffffff)
  //	{
  //		printf("Error: Failed to initialize ps addr, tx_data_addr: %ld;
  // rx_data_addr: %ld\n", (long)(tx_data_ps_addr), (long)(rx_data_ps_addr));
  //		return XST_FAILURE;
  //	}
  //	printf("tx_data_ps_addr: 0x%x \nrx_data_ps_addr: 0x%x\ntx_data_pl_addr:
  // 0x%x \nrx_data_pl_addr: 0x%x\n", 		   axi_args->tx_data_ps_ptr,
  // axi_args->rx_data_ps_ptr, axi_args->tx_data_pl_ptr,
  // axi_args->rx_data_pl_ptr);

  // 刷新Data Cache
  // Xil_DCacheFlushRange((UINTPTR) axi_args.tx_data_ps_ptr,
  // axi_args.tx_data_size);

  return 0;
}

int set_intc() {
  int status;
  // 设置中断
  status = initInterruptSystem(&INTC);
  if (status != XST_SUCCESS) {
    xil_printf("Error: Failed to initialize interrupt system\r\n");
    return XST_FAILURE;
  }
  return status;
}

/**
 * @brief PS DDR --> PL DDR
 *
 * @param axi_args axi传输相关参数
 */
int axi_ps_pl_ddr(AxiTangxi_args axi_args) {
  //	int status;

  int burst_count_temp = 0;

  XTime_GetTime(&start_t);
  while (burst_count_temp < axi_args.burst_count) {

    uint32_t tx_data_ps_next =
        (UINTPTR)axi_args.tx_data_ps_ptr + BURST_SIZE * 16 * burst_count_temp;
    uint32_t tx_data_pl_next =
        (UINTPTR)axi_args.rx_data_pl_ptr + BURST_SIZE * 16 * burst_count_temp;
    // PS DDR data--> PL DDR
    mm2s_ps_data_s2mm_pl(tx_data_ps_next, tx_data_pl_next, BURST_SIZE);
    burst_count_temp++;
  }

  XTime_GetTime(&end_t);
  double elapsed_time = ((double)(end_t - start_t) * 1e6) / COUNTS_PER_SECOND;
  printf("send done! elapsed_time: %.2f us.\n", elapsed_time);

  return 0;
}

/**
 * @brief PL DDR --> PS DDR
 *
 * @param axi_args axi传输相关参数
 */
int axi_pl_ps_ddr(AxiTangxi_args axi_args) {
  //	int status;

  int burst_count_temp = 0;

  XTime_GetTime(&start_t);
  while (burst_count_temp < axi_args.burst_count) {
    uint32_t tx_data_pl_next =
        (UINTPTR)axi_args.tx_data_pl_ptr + BURST_SIZE * 16 * burst_count_temp;
    uint32_t rx_data_ps_next =
        (UINTPTR)axi_args.rx_data_ps_ptr + BURST_SIZE * 16 * burst_count_temp;
    // PL DDR --> PS ddr
    s2mm_ps_data_mm2s_pl(rx_data_ps_next, tx_data_pl_next, BURST_SIZE);
    burst_count_temp++;
  }

  XTime_GetTime(&end_t);
  double elapsed_time = ((double)(end_t - start_t) * 1e6) / COUNTS_PER_SECOND;
  printf("send done! elapsed_time: %.2f us.\n", elapsed_time);

  // 刷新Data Cache
  // Xil_DCacheFlushRange((UINTPTR) axi_args.rx_data_ps_ptr,
  // axi_args.tx_data_size);

  int len = axi_args.tx_data_size / sizeof(int);
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (axi_args.rx_data_ps_ptr[i] != axi_args.tx_data_ps_ptr[i]) {
      count++;
      xil_printf("different data! : tx address 0x%x: %u; rx address 0x%x: %u\n",
                 axi_args.tx_data_ps_ptr + i, axi_args.tx_data_ps_ptr[i],
                 axi_args.rx_data_ps_ptr + i, axi_args.rx_data_ps_ptr[i]);
    }
  }
  xil_printf("all different data count %u\n", count);
  //	for (int i = len - 25; i < len ; i++) {
  //		xil_printf("Data at address 0x%x: %u\n", axi_args.rx_data_ps_ptr
  //+ i, axi_args.rx_data_ps_ptr[i]);
  //	}
  //	for (int i = len - 25; i < len + 2; i++) {
  //		xil_printf("Data at address 0x%x: %u\n", axi_args.tx_data_ps_ptr
  //+ i, axi_args.tx_data_ps_ptr[i]);
  //	}

  return 0;
}

void disable_axi_intr() { disableInterruptSystem(&INTC); }

/**变换网络**/
// 配置变换网络寄存器
void iwave_args_config(iwave_args *args) {
  WR_u32(args->weight_addr, (UINTPTR)ENGINE_WA);
  WR_u32(args->weight_size, (UINTPTR)ENGINE_WS);
  WR_u32(args->quantify_addr, (UINTPTR)ENGINE_QA);
  WR_u32(args->quantify_size, (UINTPTR)ENGINE_QS);
  WR_u32(args->picture_addr, (UINTPTR)ENGINE_PA);
  WR_u32(args->picture_size, (UINTPTR)ENGINE_PS);
}
// 获取变换系数和熵参数大小
void get_iwave_args(iwave_args *args) {
  args->trans_addr = RD_u32((UINTPTR)ENGINE_TA);
  args->trans_size = RD_u32((UINTPTR)ENGINE_TS);
  args->entropy_addr = RD_u32((UINTPTR)ENGINE_EA);
  args->entropy_size = RD_u32((UINTPTR)ENGINE_ES);
}
// 变换网络启动
void iwave_start(iwave_args *args) {
  args->control = 1;
  WR_u32(args->control, (UINTPTR)ENGINE_CR);
  //	usleep(1);
  //	args->control = 0;
  //	WR_u32(args->control, (UINTPTR)ENGINE_CR);
  // clean_irq();
  // control信号何时复位？用IRQ_TASK_NUM寄存器存储变换网络状态，要等到结束之后复位控制，清除中断？
  // 经确认：开始信号是脉冲信号，没有返回的信号。
}
