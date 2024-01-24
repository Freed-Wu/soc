/**
 * @file axitangxi_test.c
 * @date 2023/09/25
 * @author Wang Yang
 *
 * axi-tangxi应用测试文件。
 *
 * @bug
 **/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Strlen function

#include <errno.h>     // Error codes
#include <fcntl.h>     // Flags for open()
#include <getopt.h>    // Option parsing
#include <sys/stat.h>  // Open() system call
#include <sys/types.h> // Types for open()
#include <unistd.h>    // Close() system call

#include "libaxitangxi.h" // Interface to the AXI DMA

/**
 * @brief 校验数据
 *
 * @param dev
 */
static void verify_data(axitangxi_dev_t dev) {
  int len = dev->tx_data_size / sizeof(int);
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (dev->rx_data_ps_ptr[i] != dev->tx_data_ps_ptr[i]) {
      count++;
      printf("different data! : tx address %#x: %u; rx address %#x: %u\n",
             dev->tx_data_ps_ptr + i, dev->tx_data_ps_ptr[i],
             dev->rx_data_ps_ptr + i, dev->rx_data_ps_ptr[i]);
    }
  }
  printf("all different data count %u\n", count);
}

/*----------------------------------------------------------------------------
 * Main Function
 *----------------------------------------------------------------------------*/

int main(int argc, char **argv) {
  int rc;
  int i;

  /* 1. 初始化axitangxi设备 */
  axitangxi_dev_t axitangxi_dev;

  axitangxi_dev = axitangxi_dev_init();
  if (axitangxi_dev == NULL) {
    fprintf(stderr, "Failed to initialize the AXI DMA device.\n");
    rc = 1;
    goto ret;
  }

  // 分配内存来存储weight文件内容
  uint32_t weight_tx_size = 1024 * 1024;
  uint32_t *weight_tx_buffer = axitangxi_malloc(axitangxi_dev, weight_tx_size);
  if (weight_tx_buffer == NULL) {
    printf("Failed to allocate weight_tx_buffer memory\n");
    goto free_weight_tx_buffer;
  }
  // 内存清除1
  memset(weight_tx_buffer, 1, weight_tx_size);

  // 分配内存来存储weight文件内容(返回)
  uint32_t weight_rx_size = 1024 * 1024;
  uint32_t *weight_rx_buffer = axitangxi_malloc(axitangxi_dev, weight_rx_size);
  if (weight_rx_buffer == NULL) {
    printf("Failed to allocate weight_rx_buffer memory\n");
    goto free_weight_rx_buffer;
  }
  // 内存清除0
  memset(weight_rx_buffer, 0, weight_rx_size);

  printf("%x\n", weight_tx_buffer);
  for (i = 0; i < 10; i++) {
    printf("%x ", *(weight_tx_buffer + i));
  }
  printf("\n");
  printf("%x\n", weight_rx_buffer);
  for (i = 0; i < 10; i++) {
    printf("%x ", *(weight_rx_buffer + i));
  }
  printf("\n");

  /* 2. 初始化network_acc设备 */
  network_acc_dev_t network_acc_dev;

  network_acc_dev = network_acc_dev_init();
  if (network_acc_dev == NULL) {
    fprintf(stderr, "Failed to initialize the acc device.\n");
    rc = 1;
    goto ret;
  }

  /* 3. 读取权重数据 ，从文件读取到内存中*/

  // 定义文件指针
  FILE *file;

  // 文件路径，根据路径进行设置
  const char *filename = "/mnt/bin_w16.txt";

  // 打开文件以进行读取
  file = fopen(filename, "rb");

  if (file == NULL) {
    perror("无法打开文件");
    return 1;
  }

  // 获取文件大小
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // 读取文件内容到内存
  size_t result = fread(weight_tx_buffer, 1, file_size, file);

  if (result != file_size) {
    perror("读取文件失败");
    return 1;
  }
  // 关闭文件
  fclose(file);
  // 打印内存数据
  for (i = 0; i < 10; i++) {
    printf("%x ", *(weight_tx_buffer + i));
  }
  printf("\n");
  for (i = 0; i < 10; i++) {
    printf("%x ", *(weight_rx_buffer + i));
  }
  printf("\n");

  /* 4. 权重数据传输 */
  axitangxi_dev->tx_data_size = WEIGHT_SIZE;
  axitangxi_dev->rx_data_size = WEIGHT_SIZE;
  axitangxi_dev->tx_data_ps_ptr = weight_tx_buffer;
  axitangxi_dev->rx_data_pl_ptr = WEIGHT_ADDR;

  rc = psddr_to_plddr(axitangxi_dev);

  /* 5. 权重数据校验 */
  axitangxi_dev->tx_data_size = WEIGHT_SIZE;
  axitangxi_dev->rx_data_size = WEIGHT_SIZE;
  axitangxi_dev->tx_data_pl_ptr = WEIGHT_ADDR;
  axitangxi_dev->rx_data_ps_ptr = weight_rx_buffer;

  rc = plddr_to_psddr(axitangxi_dev);
  // 打印接收内存数据
  for (i = 0; i < 10; i++) {
    printf("%x ", *(weight_tx_buffer + i));
  }
  printf("\n");
  for (i = 0; i < 10; i++) {
    printf("%x ", *(weight_rx_buffer + i));
  }
  printf("\n");

  /* 6. 初始化network_acc数据 */
  rc = network_acc_data_init(network_acc_dev);
  if (rc != 0) {
    printf("Failed to axitangxi_data_init\n");
    goto ret;
  }
  // 网络加速器配置
  rc = acc_config_test(network_acc_dev);
  // 网络加速器开始运行
  rc = acc_start_test(network_acc_dev);
  // 网络加速器获取结果（与start信号同时执行，不要加断点）
  rc = acc_get_test(network_acc_dev);

free_weight_tx_buffer:
  axitangxi_free(axitangxi_dev, weight_tx_buffer, weight_tx_size);
free_weight_rx_buffer:
  axitangxi_free(axitangxi_dev, weight_rx_buffer, weight_rx_size);
destroy_axitangxi:
  axitangxi_destroy(axitangxi_dev);
destroy_network_acc:
  network_acc_destroy(network_acc_dev);
ret:
  return rc;
}
