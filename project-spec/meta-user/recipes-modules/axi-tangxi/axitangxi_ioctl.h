/**
 * @file axitangxi_ioctl.c
 * @date 2023/09/25
 * @author Wang Yang
 *
 * axi-tangxi内核接口文件，这包含IOCTL接口定义。这是从用户空间到AXI
 *TANGXI设备的接口。
 *
 * @bug
 **/

#ifndef AXITANGXI_IOCTL_H_
#define AXITANGXI_IOCTL_H_

#include <asm/ioctl.h> // IOCTL macros
#include <linux/types.h>

#ifndef __KERNEL__
#include <stdint.h>
#include <sys/types.h>
#endif

/*----------------------------------------------------------------------------
 * IOCTL Argument Definitions
 *----------------------------------------------------------------------------*/
// The default timeout for DMA is 10 seconds
#define AXI_TANGXI_TIMEOUT 10000

// 突发长度默认最大 1024*1024 /16
#define BURST_SIZE 65536

/* 设备节点名称 */
#define DEVICE_NAME "axi_tangxi"

// The standard path to the AXI DMA device
#define AXITX_DEV_PATH ("/dev/" DEVICE_NAME)

// 内核参数定义
struct axitangxi_transfer {

  // 必须配置
  size_t tx_data_size; // 发送数据大小
  size_t rx_data_size; // 接收数据大小

  size_t burst_size;  // 突发长度
  size_t burst_count; // 突发次数
  size_t burst_data;  // 一次突发数据大小

  uint32_t tx_data_ps_ptr; // 突发发送地址(PS DDR地址)
  uint32_t rx_data_ps_ptr; // 突发接收地址(PS DDR地址)

  uint32_t tx_data_pl_ptr; // 突发发送地址(PL DDR地址)
  uint32_t rx_data_pl_ptr; // 突发接收地址(PL DDR地址)

  uint8_t node; // 芯片node节点
};

struct axitangxi_transaction {
  // 必须配置
  size_t tx_data_size; // 发送数据大小
  size_t rx_data_size; // 接收数据大小

  size_t burst_size;  // 突发长度
  size_t burst_count; // 突发次数
  size_t burst_data;  // 一次突发数据大小

  uint32_t *tx_data_ps_ptr; // 突发发送地址(PS DDR地址)
  uint32_t *rx_data_ps_ptr; // 突发接收地址(PS DDR地址)

  uint32_t tx_data_pl_ptr; // 突发发送地址(PL DDR地址)
  uint32_t rx_data_pl_ptr; // 突发接收地址(PL DDR地址)

  uint8_t node; // 芯片node节点
};

// 网络加速器寄存器配置
struct network_acc_reg {
  uint32_t control;       // Control Register（不需要配置）
  uint32_t weight_addr;   // PL端DRAM权重存储起始地址
  uint32_t weight_size;   // PL端DRAM权重大小
  uint32_t quantify_addr; // PL端DRAM量化因子存储起始地址
  uint32_t quantify_size; // PL端DRAM量化因子大小
  uint32_t picture_addr;  // PL端DRAM图像数据存储起始地址
  uint32_t picture_size;  // PL端DRAM图像数据大小
  uint32_t trans_addr;    // PL端DRAM变换系数存储起始地址
  uint32_t trans_size;    // PL端DRAM变换系数大小（初始化为0）
  uint32_t entropy_addr;  // PL端DRAM熵参数存储起始地址
  uint32_t entropy_size;  // PL端DRAM熵参数大小（初始化为0）
};

/*----------------------------------------------------------------------------
 * IOCTL Interface
 *----------------------------------------------------------------------------*/

// The magic number used to distinguish IOCTL's for our device
#define AXITANGXI_IOCTL_MAGIC 'Y'

// The number of IOCTL's implemented, used for verification
#define AXITANGXI_NUM_IOCTLS 10

// PS DDR --> PL DDR --> PS DDR 回环
#define AXITANGXI_PSDDR_PLDDR_LOOPBACK                                         \
  _IOR(AXITANGXI_IOCTL_MAGIC, 0, struct axitangxi_transaction)

#define AXITANGXI_PSDDR_TANGXI_LOOPBACK                                        \
  _IOR(AXITANGXI_IOCTL_MAGIC, 1, struct axitangxi_transaction)

#define NETWORK_ACC_CONFIG                                                     \
  _IOR(AXITANGXI_IOCTL_MAGIC, 2, struct network_acc_reg)

#define NETWORK_ACC_START _IO(AXITANGXI_IOCTL_MAGIC, 3)

#define NETWORK_ACC_GET _IOR(AXITANGXI_IOCTL_MAGIC, 4, struct network_acc_reg)

#define PSDDR_TO_PLDDR                                                         \
  _IOR(AXITANGXI_IOCTL_MAGIC, 5, struct axitangxi_transaction)

#define PLDDR_TO_PSDDR                                                         \
  _IOR(AXITANGXI_IOCTL_MAGIC, 6, struct axitangxi_transaction)

#endif /* AXIDMA_IOCTL_H_ */
