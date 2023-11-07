/**
 * @file libaxitangxi.h
 * @date 2023/09/25
 * @author Wang Yang
 *
 * axi-tangxi设备用户库文件
 *
 * @bug
 **/

#ifndef LIBAXITANGXI_H
#define LIBAXITANGXI_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "axitangxi_ioctl.h"

// 突发长度默认最大 1024*1024 /16
#define BURST_SIZE 65536

// 权重、因子、图片的地址大小
#define WEIGHT_ADDR 0x10000000
#define WEIGHT_SIZE 0x1000

#define QUANTIFY_ADDR 0x10010000
#define QUANTIFY_SIZE 0x1000

#define PICTURE_BASE_ADDR 0x20000000
#define PICTURE_BASE_SIZE 0x2000

// axi-tangxi设备参数结构体
struct axitangxi_dev {
  bool initialized; // 指示该结构体的初始化
  int fd;           // 设备的文件描述符

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

typedef struct axitangxi_dev *axitangxi_dev_t;

/**
 * @brief 初始化AXI TANGXI设备
 *
 * @return struct axitangxi_dev*
 */
struct axitangxi_dev *axitangxi_dev_init();

/**
 * @brief 关闭AXI TANGXI设备
 *
 * @param dev
 */
void axitangxi_destroy(axitangxi_dev_t dev);

// 网络加速器寄存器配置
struct network_acc_dev {
  bool initialized; // 指示该结构体的初始化
  int fd;

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
};

typedef struct network_acc_dev *network_acc_dev_t;

/**
 * @brief 初始化NETWORK_ACC设备
 *
 * @return struct network_acc_dev*
 */
struct network_acc_dev *network_acc_dev_init();

/**
 * @brief 关闭NETWORK_ACC设备
 *
 * @param dev
 */
void network_acc_destroy(network_acc_dev_t dev);

/**
 * @brief 初始化AXI TANGXI设备需要的参数
 *
 * @param dev
 * @param tx_size
 * @param rx_size
 * @param tx_buf
 * @param rx_buf
 * @return int
 */
int axitangxi_data_init(axitangxi_dev_t dev, size_t tx_size, size_t rx_size,
                        uint32_t *tx_buf, uint32_t *rx_buf);

/**
 * @brief 分配适合与AXI
 * TANGXI驱动程序一起使用的内存区域。（目前一次性分配最大4MB内存）
 *        注意，这是一个相当昂贵的操作，应该在初始化时完成。
 *
 * @param dev
 * @param size
 * @param offset
 * @return void*
 */
void *axitangxi_malloc(axitangxi_dev_t dev, size_t size);

/**
 * @brief 释放通过调用axitangxi_malloc分配的内存区域。
 *        这里传入的大小必须与调用时使用的大小匹配，否则该函数将抛出异常。
 *
 * @param dev
 * @param addr
 * @param size
 */
void axitangxi_free(axitangxi_dev_t dev, void *addr, size_t size);

/**
 * @brief PS DDR <--->  PL DDR 回环
 *
 * @param dev
 * @return int
 */
int axitangxi_psddr_plddr_loopback(axitangxi_dev_t dev);

int ps_to_pl_data_init(axitangxi_dev_t dev, size_t tx_size, size_t rx_size,
                       uint32_t *tx_buf, uint32_t rx_buf);

int psddr_to_plddr(axitangxi_dev_t dev);

int pl_to_ps_data_init(axitangxi_dev_t dev, size_t tx_size, size_t rx_size,
                       uint32_t tx_buf, uint32_t *rx_buf);

int plddr_to_psddr(axitangxi_dev_t dev);

// 网络加速器寄存器初始化
int network_acc_data_init(network_acc_dev_t dev);
/*
 *网络加速器的三个功能函数：
 */
// 配置寄存器
int acc_config_test(network_acc_dev_t dev);
// 启动加速器
int acc_start_test(network_acc_dev_t dev);
// 获取处理结果地址
int acc_get_test(network_acc_dev_t dev);

#endif
