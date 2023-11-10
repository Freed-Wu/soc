/**
 * @file axi_tangxi.c
 * @date 2023/09/25
 * @author Wang Yang
 *
 * axi-tangxi内核驱动文件，实现axi-tangxi基本寄存器配置
 *
 * @bug
 **/

// Kernel dependencies
#include <asm/io.h>
#include <linux/delay.h> // Milliseconds to jiffies conversion
#include <linux/wait.h>  // Completion related functions

#include "axitangxi_dev.h"
#include "axitangxi_ioctl.h" // IOCTL interface definition and types

// 引入需要映射的寄存器虚拟地址定义
extern void __iomem *dma_mm2s_ps_addr;
extern void __iomem *dma_mm2s_ps_size;
extern void __iomem *noc_data_pkg_addr;
extern void __iomem *noc_data_pkg_ctl;
extern void __iomem *noc_ctl_pkg_ctl;
extern void __iomem *csram_write_ctl;
extern void __iomem *dma_s2mm_ps_addr;
extern void __iomem *dma_s2mm_ps_size;
extern void __iomem *result_read_addr;
extern void __iomem *result_read_ctl;
extern void __iomem *tx_riscv_reset_ctl;
extern void __iomem *dma_mm2s_pl_addr;
extern void __iomem *dma_mm2s_pl_size;
extern void __iomem *dma_s2mm_pl_addr;
extern void __iomem *dma_s2mm_pl_size;

// 定义网络加速器需要映射的寄存器虚拟地址
extern void __iomem *network_acc_control;
extern void __iomem *network_acc_weight_addr;
extern void __iomem *network_acc_weight_size;
extern void __iomem *network_acc_quantify_addr;
extern void __iomem *network_acc_quantify_size;
extern void __iomem *network_acc_picture_addr;
extern void __iomem *network_acc_picture_size;
extern void __iomem *network_acc_trans_addr;
extern void __iomem *network_acc_trans_size;
extern void __iomem *network_acc_entropy_addr;
extern void __iomem *network_acc_entropy_size;

// 初始化中断参数
struct axitangxi_irq_data axitangxi_irq_data = {0};

/**
 * @brief PS DDR --> PL DDR
 *
 * @param axi_args axi传输相关参数
 */
void axitangxi_psddr_plddr(struct axitangxi_transfer *axitangxi_trans) {
  uint32_t data_remaining = axitangxi_trans->tx_data_size;
  uint32_t burst_size = axitangxi_trans->burst_size;
  uint32_t burst_data = axitangxi_trans->burst_data;

  uint32_t tx_data_ps_next = axitangxi_trans->tx_data_ps_ptr;
  uint32_t rx_data_pl_next = axitangxi_trans->rx_data_pl_ptr;

  struct completion comp;
  unsigned long timeout, time_remain;

  // axitangxi_info("PS DDR --> PL DDR 开始配置！\n");

  int count = 0;
  while (data_remaining > 0) {
    // 初始化 completion
    init_completion(&comp);
    axitangxi_irq_data.comp = &comp;
    // 最后一次突发
    if (data_remaining < burst_data) {
      if ((data_remaining & ADDR_ALIGN_BASE_16_MASK) != 0x00) {
        // 对齐16字节
        data_remaining +=
            (ADDR_ALIGN_BASE_16 - (data_remaining & ADDR_ALIGN_BASE_16_MASK));
      }
      burst_size = data_remaining >> 4;
      burst_data = data_remaining;
    }
    data_remaining -= burst_data;
    // if(data_remaining == 0) {
    // 	axitangxi_irq_data.last_s2mm_pl_done = true;
    // }
    // PS DDR data--> PL DDR
    mm2s_ps_data_s2mm_pl(tx_data_ps_next, rx_data_pl_next, burst_size);
    // 等待中断信号
    tx_data_ps_next = tx_data_ps_next + burst_data;
    rx_data_pl_next = rx_data_pl_next + burst_data;
    count++;
    axitangxi_info(
        "PS DDR --> PL DDR: 第 %d 次突发，突发长度：%d, 剩下数据：%dB\n", count,
        burst_size, data_remaining);
    // 等待一次传输完成
    timeout = msecs_to_jiffies(AXI_TANGXI_TIMEOUT);
    // wait S2MM_PL_DONE
    time_remain = wait_for_completion_timeout(axitangxi_irq_data.comp, timeout);
    if (time_remain == 0) {
      axitangxi_err(
          "axitangxi_psddr_plddr transaction 第 %d 次突发 timed out.\n", count);
      // FIXME: should be a bug, shouldn't return any value
      // return -ETIME;
    }
    printk("time_remain_1: %ld\n", time_remain);
  }
  // axitangxi_info("PS DDR --> PL DDR 配置完成！\n");
}

/**
 * @brief PL DDR --> PS DDR
 *
 * @param axi_args axi传输相关参数
 */
void axitangxi_plddr_psddr(struct axitangxi_transfer *axitangxi_trans) {

  uint32_t data_remaining = axitangxi_trans->rx_data_size;
  uint32_t burst_size = axitangxi_trans->burst_size;
  uint32_t burst_data = axitangxi_trans->burst_data;

  uint32_t rx_data_ps_next = axitangxi_trans->rx_data_ps_ptr;
  uint32_t tx_data_pl_next = axitangxi_trans->tx_data_pl_ptr;

  struct completion comp;
  unsigned long timeout, time_remain;

  int count = 0;
  while (data_remaining > 0) {
    init_completion(&comp);
    axitangxi_irq_data.comp = &comp;
    // 最后一次突发
    if (data_remaining < burst_data) {
      if ((data_remaining & ADDR_ALIGN_BASE_16_MASK) != 0x00) {
        // 对齐16字节
        data_remaining +=
            (ADDR_ALIGN_BASE_16 - (data_remaining & ADDR_ALIGN_BASE_16_MASK));
      }
      burst_size = data_remaining >> 4;
      burst_data = data_remaining;
    }
    data_remaining -= burst_data;
    // if(data_remaining == 0) {
    // 	axitangxi_irq_data.last_s2mm_ps_done = true;
    // }
    // PL DDR --> PS ddr
    s2mm_ps_data_mm2s_pl(rx_data_ps_next, tx_data_pl_next, burst_size);
    // 等待中断信号
    rx_data_ps_next = rx_data_ps_next + burst_data;
    tx_data_pl_next = tx_data_pl_next + burst_data;
    count++;
    axitangxi_info(
        "PL DDR --> PS DDR: 第 %d 次突发，突发长度：%d, 剩下数据：%d B\n",
        count, burst_size, data_remaining);
    // wait S2MM_PS_DONE
    // 等待一次传输完成
    timeout = msecs_to_jiffies(AXI_TANGXI_TIMEOUT);
    time_remain = wait_for_completion_timeout(axitangxi_irq_data.comp, timeout);
    if (time_remain == 0) {
      axitangxi_err(
          "axitangxi_psddr_plddr transaction 第 %d 次突发 timed out.\n", count);
      // FIXME: should be a bug, shouldn't return any value
      // return -ETIME;
    }
    printk("time_remain_1: %ld\n", time_remain);
  }

  // printf("PL DDR --> PS DDR 配置完成！\n");
}

/**
 * @brief PS DRAM --> PL DRAM --> PS DRAM回环
 *
 * @param dev
 * @param trans
 * @return int
 */
int axitangxi_psddr_plddr_loopback(struct axitangxi_device *dev,
                                   struct axitangxi_transaction *trans) {

  struct axitangxi_transfer axitx_trans;

  axitx_trans.tx_data_size = trans->tx_data_size;
  axitx_trans.rx_data_size = trans->rx_data_size;
  axitx_trans.burst_size = trans->burst_size;
  axitx_trans.burst_count = trans->burst_count;
  axitx_trans.burst_data = trans->burst_data;
  axitx_trans.rx_data_ps_ptr =
      axitangxi_uservirt_to_phys(dev, trans->rx_data_ps_ptr);
  axitx_trans.tx_data_pl_ptr = axitx_trans.rx_data_pl_ptr =
      axitx_trans.tx_data_ps_ptr =
          axitangxi_uservirt_to_phys(dev, trans->tx_data_ps_ptr);
  // axitx_trans.rx_data_pl_ptr = axitx_trans.rx_data_ps_ptr;
  axitx_trans.node = trans->node;

  printk("tx_data_ps_ptr: %#x, rx_data_ps_ptr: %#x, tx_data_pl_ptr: %#x, "
         "rx_data_pl_ptr: %#x, burst_size: %ld, burst_data: %ld\n",
         axitx_trans.tx_data_ps_ptr, axitx_trans.rx_data_ps_ptr,
         axitx_trans.tx_data_pl_ptr, axitx_trans.rx_data_pl_ptr,
         axitx_trans.burst_size, axitx_trans.burst_data);

  int i;
  for (i = 0; i < 100; i++) {
    if (trans->tx_data_ps_ptr[i] != i + 1) {
      printk("data error! trans->tx_data_ps_ptr[%d]: %d\n", i,
             trans->tx_data_ps_ptr[i]);
    }
  }

  // PS DDR --> PL DDR
  axitangxi_psddr_plddr(&axitx_trans);

  axitangxi_plddr_psddr(&axitx_trans);

  for (i = 0; i < 10; i++) {
    if (trans->rx_data_ps_ptr[i] != i + 1) {
      printk("data error! trans->rx_data_ps_ptr[%d]: %d\n", i,
             trans->rx_data_ps_ptr[i]);
    }
  }

  uint32_t *tx_ptr = phys_to_virt(axitx_trans.tx_data_ps_ptr);
  uint32_t *rx_ptr = phys_to_virt(axitx_trans.rx_data_ps_ptr);

  printk("tx_ptr: %p, rx_ptr: %p", tx_ptr, rx_ptr);

  return 0;
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
                         uint32_t burst_size, uint32_t chip_position,
                         uint8_t node) {
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
  writel(tx_data_ps_addr, dma_mm2s_ps_addr);
  // DMA_MM2S_PS_SIZE
  writel(burst_size - 1, dma_mm2s_ps_size);

  // NOC_DATA_PKG_ADDR
  writel(noc_data_size_addr, noc_data_pkg_addr);
  // NOC_DATA_PKG_CTL
  writel(noc_data_node_type_cmd, noc_data_pkg_ctl);
}

/**
 * @brief tangxi --> PL Read Result (默认存储BRAM，首地址256) --> PS ddr
 *
 * @param rx_data_ps_addr 写PS ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void s2mm_ps_data_tangxi(uint32_t rx_data_ps_addr, uint32_t burst_size) {
  // RESULT_READ_ADDR 与NOC_DATA_PKG_ADDR addr一致
  uint32_t noc_data_size_addr = ((burst_size - 1) << 20) | 0x00100;

  writel(noc_data_size_addr, result_read_addr);
  // RESULT_READ_CTL
  writel(burst_size - 1, result_read_ctl);

  // DMA_S2MM_PS_ADDR
  writel(rx_data_ps_addr, dma_s2mm_ps_addr);

  // DMA_S2MM_PS_SIZE
  writel(burst_size - 1, dma_s2mm_ps_size);
}

/**
 * @brief PS DDR --> PL DDR
 *
 * @param tx_data_ps_addr 读取ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param rx_data_pl_addr 写入pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void mm2s_ps_data_s2mm_pl(uint32_t tx_data_ps_addr, uint32_t rx_data_pl_addr,
                          uint32_t burst_size) {
  printk("DMA_MM2S_PS_ADDR: %p\n", dma_mm2s_ps_addr);
  // DMA_MM2S_PS_ADDR
  writel(tx_data_ps_addr, dma_mm2s_ps_addr);

  printk("DMA_MM2S_PS_SIZE: %p\n", dma_mm2s_ps_size);
  // DMA_MM2S_PS_SIZE
  writel(burst_size - 1, dma_mm2s_ps_size);

  printk("DMA_S2MM_PL_ADDR: %p\n", dma_s2mm_pl_addr);
  // DMA_S2MM_PL_ADDR
  writel(rx_data_pl_addr, dma_s2mm_pl_addr);

  printk("DMA_S2MM_PL_SIZE: %p\n", dma_s2mm_pl_size);
  // DMA_S2MM_PL_SIZE
  writel(burst_size - 1, dma_s2mm_pl_size);
}

/**
 * @brief PL DDR --> PS DDR
 *
 * @param rx_data_ps_addr 写入ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param tx_data_pl_addr 读取pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void s2mm_ps_data_mm2s_pl(uint32_t rx_data_ps_addr, uint32_t tx_data_pl_addr,
                          uint32_t burst_size) {
  // DMA_MM2S_PL_ADDR
  writel(tx_data_pl_addr, dma_mm2s_pl_addr);
  // DMA_MM2S_PL_SIZE
  writel(burst_size - 1, dma_mm2s_pl_size);

  // DMA_S2MM_PS_ADDR
  writel(rx_data_ps_addr, dma_s2mm_ps_addr);
  // DMA_S2MM_PS_SIZE
  writel(burst_size - 1, dma_s2mm_ps_size);
}

/**
 * @brief 批量PS DDR --> tangxi --> PL DDR
 *
 * @param axitangxi_trans
 */
void batch_mm2s_ps_data_tangxi(struct axitangxi_transfer *axitangxi_trans) {
  uint32_t data_remaining = axitangxi_trans->tx_data_size;

  uint32_t tx_data_ps_next = (uintptr_t)axitangxi_trans->tx_data_ps_ptr;
  uint32_t rx_data_pl_next = axitangxi_trans->rx_data_pl_ptr;

  uint32_t burst_size = axitangxi_trans->burst_size;
  uint32_t burst_data = axitangxi_trans->burst_data;

  // XTime_GetTime(&start_t);
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
    }
    // PS DDR data--> PL --> tangxi, 0:第0个chip, 3:第三个node, 256:req
    // data存储地址256
    mm2s_ps_data_tangxi(tx_data_ps_next, rx_data_pl_next, burst_size, 0, 3);
    // 等待中断信号
    tx_data_ps_next = tx_data_ps_next + burst_data;
    data_remaining -= burst_data;
  }
}

/**
 * @brief 批量PL DDR --> PS DDR
 *
 * @param axitangxi_trans
 */
void batch_s2mm_ps_data_mm2s_pl(struct axitangxi_transfer *axitangxi_trans) {
  uint32_t data_remaining = axitangxi_trans->rx_data_size;

  uint32_t rx_data_ps_next = (uintptr_t)axitangxi_trans->rx_data_ps_ptr;
  uint32_t tx_data_pl_next = axitangxi_trans->rx_data_pl_ptr;

  uint32_t burst_size = axitangxi_trans->burst_size;
  uint32_t burst_data = axitangxi_trans->burst_data;
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
    }
    s2mm_ps_data_mm2s_pl(rx_data_ps_next, tx_data_pl_next, burst_size);
    rx_data_ps_next = rx_data_ps_next + burst_data;
    data_remaining -= burst_data;
  }
}

/**
 * @brief PS DRAM --> PL --> tangxi --> PL --> PS DRAM 回环
 *
 * @param axitangxi_trans
 * @return int
 */
int axi_tangxi_loopback(struct axitangxi_transfer *axitangxi_trans) {
  // PS DDR --> tangxi --> PL DDR
  batch_mm2s_ps_data_tangxi(axitangxi_trans);

  udelay(2000);

  batch_s2mm_ps_data_mm2s_pl(axitangxi_trans);

  udelay(2000);

  return 0;
}

void network_acc_config_dev(uint32_t weight_addr, uint32_t weight_size,
                            uint32_t quantify_addr, uint32_t quantify_size,
                            uint32_t picture_addr, uint32_t picture_size) {
  writel(weight_addr, network_acc_weight_addr);
  writel(weight_size, network_acc_weight_size);
  writel(quantify_addr, network_acc_quantify_addr);
  writel(quantify_size, network_acc_quantify_size);
  writel(picture_addr, network_acc_picture_addr);
  writel(picture_size, network_acc_picture_size);

  printk("network_acc_config_dev successful");
}

/**
 * @brief
 *
 * @param dev
 * @param trans
 * @return int
 */
int acc_config(struct axitangxi_device *dev, struct network_acc_reg *args) {
  network_acc_config_dev(args->weight_addr, args->weight_size,
                         args->quantify_addr, args->quantify_size,
                         args->picture_addr, args->picture_size);
  printk("acc_config_successful");
  return 0;
};

/**
 * @brief PS DRAM --> PL DRAM --> PS DRAM回环
 *
 * @param dev
 * @param trans
 * @return int
 */
int acc_start() {
  uint32_t args_control = 1;
  writel(args_control, network_acc_control);
  // udelay(2000);
  // args_control = 0;
  // writel(args_control, network_acc_control);

  printk("network_acc_start successful");
  return 0;
};

int acc_complete() {

  struct completion my_completion;
  init_completion(&my_completion);

  axitangxi_irq_data.comp = &my_completion;
  // 等待中断响应
  unsigned long timeout = msecs_to_jiffies(10000);
  unsigned long time_remain =
      wait_for_completion_timeout(axitangxi_irq_data.comp, timeout);
  if (time_remain == 0) {
    axitangxi_err("network_acc_done timed out.\n");
    return -ETIME;
  }
  printk("network_acc_done time_remain: %ld\n", time_remain);

  return 0;
}

// /**
//  * @brief PS DRAM --> PL DRAM --> PS DRAM回环
//  *
//  * @param dev
//  * @param trans
//  * @return int
//  */
uint32_t acc_get_trans_addr(struct axitangxi_device *dev) {

  uint32_t rect = readl(network_acc_trans_addr);
  printk("acc_get_trans_addr successful");

  return rect;
};
// 2
uint32_t acc_get_trans_size(struct axitangxi_device *dev) {

  uint32_t rect = readl(network_acc_trans_size);
  printk("acc_get_trans_size successful");

  return rect;
};
// 3
uint32_t acc_get_entropy_addr(struct axitangxi_device *dev) {

  uint32_t rect = readl(network_acc_entropy_addr);

  printk("acc_get_entropy_addr successful");

  return rect;
};
// 4
uint32_t acc_get_entropy_size(struct axitangxi_device *dev) {

  uint32_t rect = readl(network_acc_entropy_size);

  printk("acc_get_entropy_size successful");

  return rect;
};

int psddr_to_plddr(struct axitangxi_device *dev,
                   struct axitangxi_transaction *trans) {

  struct axitangxi_transfer axitx_trans;

  axitx_trans.tx_data_size = trans->tx_data_size;
  axitx_trans.rx_data_size = trans->rx_data_size;
  axitx_trans.burst_size = trans->burst_size;
  axitx_trans.burst_count = trans->burst_count;
  axitx_trans.burst_data = trans->burst_data;
  // PL doesn't have MMU
  axitx_trans.rx_data_pl_ptr = trans->rx_data_pl_ptr;
  // PS has MMU
  axitx_trans.tx_data_ps_ptr =
      axitangxi_uservirt_to_phys(dev, trans->tx_data_ps_ptr);
  // axitx_trans.rx_data_pl_ptr = axitx_trans.rx_data_ps_ptr;
  axitx_trans.node = trans->node;

  axitangxi_psddr_plddr(&axitx_trans);

  printk("tx_data_ps_ptr: %#x, rx_data_pl_ptr: %#x, burst_size: %ld, "
         "burst_data: %ld \n",
         axitx_trans.tx_data_ps_ptr, axitx_trans.rx_data_pl_ptr,
         axitx_trans.burst_size, axitx_trans.burst_data);

  uint32_t *tx_ptr = phys_to_virt(axitx_trans.tx_data_ps_ptr);
  uint32_t *rx_ptr = phys_to_virt(axitx_trans.rx_data_pl_ptr);

  printk("tx_ptr: %p, rx_ptr: %p", tx_ptr, rx_ptr);

  return 0;
}

int plddr_to_psddr(struct axitangxi_device *dev,
                   struct axitangxi_transaction *trans) {

  struct axitangxi_transfer axitx_trans;

  axitx_trans.tx_data_size = trans->tx_data_size;
  axitx_trans.rx_data_size = trans->rx_data_size;
  axitx_trans.burst_size = trans->burst_size;
  axitx_trans.burst_count = trans->burst_count;
  axitx_trans.burst_data = trans->burst_data;
  axitx_trans.tx_data_pl_ptr = trans->tx_data_pl_ptr;
  axitx_trans.rx_data_ps_ptr =
      axitangxi_uservirt_to_phys(dev, trans->rx_data_ps_ptr);
  // axitx_trans.rx_data_pl_ptr = axitx_trans.rx_data_ps_ptr;
  axitx_trans.node = trans->node;

  axitangxi_plddr_psddr(&axitx_trans);

  printk("tx_data_pl_ptr: %#x, rx_data_ps_ptr: %#x, burst_size: %ld, "
         "burst_data: %ld \n",
         axitx_trans.tx_data_pl_ptr, axitx_trans.rx_data_ps_ptr,
         axitx_trans.burst_size, axitx_trans.burst_data);

  uint32_t *tx_ptr = phys_to_virt(axitx_trans.tx_data_pl_ptr);
  uint32_t *rx_ptr = phys_to_virt(axitx_trans.rx_data_ps_ptr);

  printk("tx_ptr: %p, rx_ptr: %p", tx_ptr, rx_ptr);

  return 0;
}
