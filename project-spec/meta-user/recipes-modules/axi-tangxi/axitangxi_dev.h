/**
 * @file axitangxi_dev.h
 * @date 2023/09/25
 * @author Wang Yang
 *
 * axi-tangxi内核驱动库文件
 *
 * @bug
 **/

#ifndef AXITANGXI_DEV_H_
#define AXITANGXI_DEV_H_

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/list.h> // Linked list definitions and functions
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/types.h>

// #include <linux/dmaengine.h>        // Definitions for DMA structures and
// types
#include <linux/dma-mapping.h> // DMA shared buffers interface

#include "axitangxi_ioctl.h" // IOCTL argument structures

// 定义需要控制的寄存器地址
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

// 网络加速器
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

// 中断信号
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
#define ADDR_ALIGN_BASE_4096_MASK (ADDR_ALIGN_BASE_4096 - 1)
#define ADDR_ALIGNED_4096(addr)                                                \
  (UINTPTR)(((addr) + ADDR_ALIGN_BASE_4096_MASK) & (~ADDR_ALIGN_BASE_4096_MASK))

// 16字节对齐
#define ADDR_ALIGN_BASE_16 0x0010
#define ADDR_ALIGN_BASE_16_MASK (ADDR_ALIGN_BASE_16 - 1)
#define ADDR_ALIGNED_16(addr)                                                  \
  (UINTPTR)(((addr) + ADDR_ALIGN_BASE_16_MASK) & (~ADDR_ALIGN_BASE_16_MASK))

// Default minor number for the device
#define MINOR_NUMBER 0
// The default number of character devices for DMA
#define NUM_DEVICES 1

// Truncates the full __FILE__ path, only displaying the basename
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Convenient macros for printing out messages to the kernel log buffer
#define axitangxi_err(fmt, ...)                                                \
  printk(KERN_ERR DEVICE_NAME ": %s: %s: %d: " fmt, __FILENAME__, __func__,    \
         __LINE__, ##__VA_ARGS__)
#define axitangxi_info(fmt, ...)                                               \
  printk(KERN_ERR DEVICE_NAME ": %s: %s: %d: " fmt, __FILENAME__, __func__,    \
         __LINE__, ##__VA_ARGS__)

/* 把驱动代码中会用到的数据打包进设备结构体 */
struct axitangxi_device {
  /** 字符设备框架 **/

  int num_devices;        // The number of devices
  unsigned int minor_num; // The minor number of the device
  char *chrdev_name;      // The name of the character device
  dev_t devid;            // 设备号
  struct cdev cdev;       // 字符设备
  struct class *class;    // 类
  struct device *device;  // 设备
  struct device_node *nd; // 设备树的设备节点

  /** 中断 **/
  unsigned int irq;        // 中断号
  unsigned int regdata[2]; // 寄存器
  /** 定时器 **/
  // struct timer_list  timer;             //定时器

  struct platform_device *pdev; // The platform device from the device tree

  struct list_head addr_list; // List of allocated DMA buffers
};

// 中断同步信号
struct axitangxi_irq_data {
  bool last_mm2s_ps_done;       // 最后一次读PS DDR传输完成
  bool last_s2mm_ps_done;       // 最后一次写PS DDR传输完成
  bool last_mm2s_pl_done;       // 最后一次读PL DDR传输完成
  bool last_s2mm_pl_done;       // 最后一次写PL DDR传输完成
  bool last_req_data_done;      // 最后一次NI发包传输完成
  bool last_read_result_done;   // 最后一次read result传输完成
  bool network_acc_handle_done; // 网络加速器处理完成

  struct completion *comp; // For sync, the notification to kernel
};

// Function prototypes
int axitangxi_chrdev_init(struct axitangxi_device *dev);
void axitangxi_chrdev_exit(struct axitangxi_device *dev);
// Function Prototypes
int axitangxi_dma_init(struct platform_device *pdev,
                       struct axitangxi_device *dev);

/**
 * @brief 用户虚拟地址转化成物理地址
 *
 * @param dev
 * @param user_addr
 * @return uint32_t
 */
uint32_t axitangxi_uservirt_to_phys(struct axitangxi_device *dev,
                                    void *user_addr);

/**
 * @brief PS DRAM --> PL DRAM --> PS DRAM回环
 *
 * @param dev
 * @param trans
 * @return int
 */
int axitangxi_psddr_plddr_loopback(struct axitangxi_device *dev,
                                   struct axitangxi_transaction *trans);

/**
 * @brief PS DDR --> PL DDR
 *
 * @param tx_data_ps_addr 读取ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param rx_data_pl_addr 写入pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void mm2s_ps_data_s2mm_pl(uint32_t tx_data_ps_addr, uint32_t rx_data_pl_addr,
                          uint32_t burst_size);

/**
 * @brief PL DDR --> PS DDR
 *
 * @param rx_data_ps_addr 写入ps ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param tx_data_pl_addr 读取pl ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void s2mm_ps_data_mm2s_pl(uint32_t rx_data_ps_addr, uint32_t tx_data_pl_addr,
                          uint32_t burst_size);

/**
 * @brief tangxi --> PL Read Result (默认存储BRAM，首地址256) --> PS ddr
 *
 * @param rx_data_ps_addr 写PS ddr数据首地址:0x00000000 - 0x7FFFFFFF
 * @param burst_size 突发长度：0-127
 */
void s2mm_ps_data_tangxi(uint32_t rx_data_ps_addr, uint32_t burst_size);

// 网络加速器

void network_acc_config_dev(uint32_t weight_addr, uint32_t weight_size,
                            uint32_t quantify_addr, uint32_t quantify_size,
                            uint32_t picture_addr, uint32_t picture_size);

int acc_config(struct axitangxi_device *dev, struct network_acc_reg *args);

int acc_start(void);

int acc_complete(void);

uint32_t acc_get_trans_addr(struct axitangxi_device *dev);
uint32_t acc_get_trans_size(struct axitangxi_device *dev);
uint32_t acc_get_entropy_addr(struct axitangxi_device *dev);
uint32_t acc_get_entropy_size(struct axitangxi_device *dev);

int psddr_to_plddr(struct axitangxi_device *dev,
                   struct axitangxi_transaction *trans);

int plddr_to_psddr(struct axitangxi_device *dev,
                   struct axitangxi_transaction *trans);
#endif /* AXITANGXI_H_ */
