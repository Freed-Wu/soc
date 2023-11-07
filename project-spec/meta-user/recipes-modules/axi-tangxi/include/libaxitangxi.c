/**
 * @file libaxitangxi.c
 * @date 2023/09/25
 * @author Wang Yang
 *
 * axi-tangxi设备用户库文件
 *
 * @bug
 **/

#include <assert.h>
#include <errno.h>  // Error codes
#include <fcntl.h>  // Flags for open()
#include <signal.h> // Signal handling functions
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // Memset and memcpy functions
#include <sys/ioctl.h> // IOCTL system call
#include <sys/mman.h>  // Mmap system call
#include <sys/stat.h>  // Open() system call
#include <sys/types.h> // Types for open()
#include <unistd.h>    // Close() system call

#include "axitangxi_ioctl.h" // The IOCTL interface to AXI TANGXI
#include "libaxitangxi.h"    // Local definitions

// axitangxi结构，确认是否已经open
struct axitangxi_dev axitangxi_dev = {0};

/**
 * @brief 初始化AXI TANGXI设备
 *
 * @return struct axitangxi_dev*
 */
struct axitangxi_dev *axitangxi_dev_init() {
  assert(!axitangxi_dev.initialized);

  // Open the AXI DMA device
  axitangxi_dev.fd = open("/dev/axi_tangxi", O_RDWR | O_EXCL);
  if (axitangxi_dev.fd < 0) {
    printf("Error opening AXI Tangxi device");
    fprintf(stderr, "Expected the AXI Tangxi device at the path `%s`\n",
            "/dev/axi_tangxi");
    return NULL;
  }

  // Return the AXI DMA device to the user
  axitangxi_dev.initialized = true;
  return &axitangxi_dev;
}

/**
 * @brief 关闭AXI TANGXI设备
 *
 * @param dev
 */
void axitangxi_destroy(axitangxi_dev_t dev) {
  // Close the AXI TANGXI device
  if (close(dev->fd) < 0) {
    perror("Failed to close the AXI TANGXI device");
    assert(false);
  }

  // Free the device structure
  axitangxi_dev.initialized = false;
  return;
}

// network_acc结构，确认是否已经open
struct network_acc_dev network_acc_dev = {0};

/**
 * @brief 初始化network_acc设备
 *
 * @return struct network_acc*
 */
struct network_acc_dev *network_acc_dev_init() {
  assert(!network_acc_dev.initialized);

  // Open the device
  network_acc_dev.fd = open("/dev/axi_tangxi", O_RDWR | O_EXCL);
  if (network_acc_dev.fd < 0) {
    printf("Error opening AXI Tangxi device");
    fprintf(stderr, "Expected the AXI Tangxi device at the path `%s`\n",
            "/dev/axi_tangxi");
    return NULL;
  }

  // Return the AXI DMA device to the user
  network_acc_dev.initialized = true;
  return &network_acc_dev;
}

/**
 * @brief 关闭network_acc设备
 *
 * @param dev
 */
void network_acc_destroy(network_acc_dev_t dev) {
  // Close the AXI device
  if (close(dev->fd) < 0) {
    perror("Failed to close the AXI TANGXI device");
    assert(false);
  }

  // Free the device structure
  network_acc_dev.initialized = false;
  return;
}

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
                        uint32_t *tx_buf, uint32_t *rx_buf) {

  // assert(burst_size <= 65536);
  // 突发长度
  dev->burst_size = BURST_SIZE;
  // 传输数据大小
  dev->tx_data_size = tx_size;
  // 接收数据大小
  dev->rx_data_size = rx_size;

  // 突发次数
  dev->burst_count = (dev->tx_data_size - 1) / (dev->burst_size * 16) + 1;
  // 一次突发大小
  dev->burst_data = dev->burst_size * 16;

  // UINTPTR tx_data_ps_addr = (UINTPTR) tx_buf;
  // ps ddr传输首地址
  dev->tx_data_ps_ptr = tx_buf;

  // 接收首地址默认加0x1000000(16MB数据空间)
  // UINTPTR rx_data_ps_addr = ADDR_ALIGNED_4096(tx_data_ps_addr +
  // dev->tx_data_size + 0x1000000); ps ddr接收首地址
  dev->rx_data_ps_ptr = rx_buf;

  // pl ddr传输/接收首地址(自定义，默认与ps ddr传输首地址相同)
  dev->tx_data_pl_ptr = dev->rx_data_pl_ptr = (uintptr_t)dev->tx_data_ps_ptr;

  printf("tx_data_ps_addr: %#x \nrx_data_ps_addr: %#x\ntx_data_pl_addr: %#x "
         "\nrx_data_pl_addr: %#x\n",
         (uintptr_t)dev->tx_data_ps_ptr, (uintptr_t)dev->rx_data_ps_ptr,
         dev->tx_data_pl_ptr, dev->rx_data_pl_ptr);

  return 0;
}

/**
 * @brief 分配适合与AXI TANGXI驱动程序一起使用的内存区域。
 *        注意，这是一个相当昂贵的操作，应该在初始化时完成。
 *
 * @param dev
 * @param size
 * @param offset
 * @return void*
 */
void *axitangxi_malloc(axitangxi_dev_t dev, size_t size) {
  void *addr;

  // 将文件映射到内存区域
  addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, 0);
  if (addr == MAP_FAILED) {
    return NULL;
  }

  return addr;
}

/**
 * @brief 释放通过调用axitangxi_malloc分配的内存区域。
 *        这里传入的大小必须与调用时使用的大小匹配，否则该函数将抛出异常。
 *
 * @param dev
 * @param addr
 * @param size
 */
void axitangxi_free(axitangxi_dev_t dev, void *addr, size_t size) {
  // Silence the compiler
  (void)dev;

  if (munmap(addr, size) < 0) {
    perror("Failed to free the AXI TANGXI memory mapped region");
    assert(false);
  }

  return;
}

/**
 * @brief PS DDR <--->  PL DDR 回环
 *
 * @param dev
 * @return int
 */
int axitangxi_psddr_plddr_loopback(axitangxi_dev_t dev) {
  int rc;
  struct axitangxi_transaction trans;

  trans.tx_data_size = dev->tx_data_size;
  trans.rx_data_size = dev->rx_data_size;
  trans.burst_size = dev->burst_size;
  trans.burst_count = dev->burst_count;
  trans.burst_data = dev->burst_data;
  trans.tx_data_ps_ptr = dev->tx_data_ps_ptr;
  trans.rx_data_ps_ptr = dev->rx_data_ps_ptr;
  trans.tx_data_pl_ptr = dev->tx_data_pl_ptr;
  trans.rx_data_pl_ptr = dev->rx_data_pl_ptr;
  trans.node = dev->node;

  // 调用内核函数
  rc = ioctl(dev->fd, AXITANGXI_PSDDR_PLDDR_LOOPBACK, &trans);
  if (rc < 0) {
    perror("Failed to perform the AXITANGXI_PSDDR_PLDDR_LOOPBACK transfer");
  }

  return rc;
}

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
int ps_to_pl_data_init(axitangxi_dev_t dev, size_t tx_size, size_t rx_size,
                       uint32_t *tx_buf, uint32_t rx_buf) {

  // assert(burst_size <= 65536);
  // 突发长度
  dev->burst_size = BURST_SIZE;
  // 传输数据大小
  dev->tx_data_size = tx_size;
  // 接收数据大小
  dev->rx_data_size = rx_size;

  // 突发次数
  dev->burst_count = (dev->tx_data_size - 1) / (dev->burst_size * 16) + 1;
  // 一次突发大小
  dev->burst_data = dev->burst_size * 16;

  // ps ddr传输首地址
  dev->tx_data_ps_ptr = tx_buf;

  // ps ddr接收首地址
  dev->rx_data_pl_ptr = rx_buf;

  printf("tx_data_ps_addr: %#x \nrx_data_pl_addr: %#x\n",
         (uintptr_t)dev->tx_data_ps_ptr, dev->rx_data_pl_ptr);

  return 0;
}

/**
 * @brief PS DDR <--->  PL DDR
 *
 * @param dev
 * @return int
 */
int psddr_to_plddr(axitangxi_dev_t dev) {
  int rc;
  struct axitangxi_transaction trans;

  // 传输数据大小
  size_t tx_size = dev->tx_data_size;
  // 接收数据大小
  size_t rx_size = dev->rx_data_size;

  // 传输空间
  uint32_t *tx_buffer = dev->tx_data_ps_ptr;
  // 接收地址
  uint32_t rx_buffer = dev->rx_data_pl_ptr;

  ps_to_pl_data_init(dev, tx_size, rx_size, tx_buffer, rx_buffer);

  trans.tx_data_size = dev->tx_data_size;
  trans.rx_data_size = dev->rx_data_size;
  trans.burst_size = dev->burst_size;
  trans.burst_count = dev->burst_count;
  trans.burst_data = dev->burst_data;
  trans.tx_data_ps_ptr = dev->tx_data_ps_ptr;
  trans.rx_data_pl_ptr = dev->rx_data_pl_ptr;
  trans.node = dev->node;

  // 调用内核函数
  rc = ioctl(dev->fd, PSDDR_TO_PLDDR, &trans);
  if (rc < 0) {
    perror("Failed to perform the PSDDR_TO_PLDDR transfer");
  }

  return rc;
}

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
int pl_to_ps_data_init(axitangxi_dev_t dev, size_t tx_size, size_t rx_size,
                       uint32_t tx_buf, uint32_t *rx_buf) {

  // assert(burst_size <= 65536);
  // 突发长度
  dev->burst_size = BURST_SIZE;
  // 传输数据大小
  dev->tx_data_size = tx_size;
  // 接收数据大小
  dev->rx_data_size = rx_size;

  // 突发次数
  dev->burst_count = (dev->tx_data_size - 1) / (dev->burst_size * 16) + 1;
  // 一次突发大小
  dev->burst_data = dev->burst_size * 16;

  // ps ddr传输首地址
  dev->tx_data_pl_ptr = tx_buf;

  // ps ddr接收首地址
  dev->rx_data_ps_ptr = rx_buf;

  printf("tx_data_pl_addr: %#x \nrx_data_ps_addr: %#x\n",
         (uintptr_t)dev->tx_data_pl_ptr, dev->rx_data_ps_ptr);

  return 0;
}

/**
 * @brief PL DDR <--->  PS DDR
 *
 * @param dev
 * @return int
 */
int plddr_to_psddr(axitangxi_dev_t dev) {
  int rc;
  struct axitangxi_transaction trans;

  // 传输数据大小
  size_t tx_size = dev->tx_data_size;
  // 接收数据大小
  size_t rx_size = dev->rx_data_size;

  // 接收空间
  uint32_t *rx_buffer = dev->rx_data_ps_ptr;
  // 传输地址
  uint32_t tx_buffer = dev->tx_data_pl_ptr;

  pl_to_ps_data_init(dev, tx_size, rx_size, tx_buffer, rx_buffer);

  trans.tx_data_size = dev->tx_data_size;
  trans.rx_data_size = dev->rx_data_size;
  trans.burst_size = dev->burst_size;
  trans.burst_count = dev->burst_count;
  trans.burst_data = dev->burst_data;
  trans.rx_data_ps_ptr = dev->rx_data_ps_ptr;
  trans.tx_data_pl_ptr = dev->tx_data_pl_ptr;
  trans.node = dev->node;

  // 调用内核函数
  rc = ioctl(dev->fd, PLDDR_TO_PSDDR, &trans);
  if (rc < 0) {
    perror("Failed to perform the PLDDR_TO_PSDDR transfer");
  }

  return rc;
}

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
int network_acc_data_init(network_acc_dev_t dev) {

  dev->weight_addr = WEIGHT_ADDR;
  dev->weight_size = WEIGHT_SIZE;
  dev->quantify_addr = QUANTIFY_ADDR;
  dev->quantify_size = QUANTIFY_SIZE;
  dev->picture_addr = PICTURE_BASE_ADDR;
  dev->picture_size = PICTURE_BASE_SIZE;

  printf("weight_addr: %#x \n weight_size: %#x\n quantify_addr: %#x \n "
         "quantify_size: %#x\n ",
         dev->weight_addr, dev->weight_size, dev->quantify_addr,
         dev->quantify_size);

  return 0;
}

int acc_config_test(network_acc_dev_t dev) {
  int rc;
  struct network_acc_reg iargs;

  iargs.weight_addr = dev->weight_addr;
  iargs.weight_size = dev->weight_size;
  iargs.quantify_addr = dev->quantify_addr;
  iargs.quantify_size = dev->quantify_size;
  iargs.picture_addr = dev->picture_addr;
  iargs.picture_size = dev->picture_size;

  // 调用内核函数
  rc = ioctl(dev->fd, NETWORK_ACC_CONFIG, &iargs);
  if (rc < 0) {
    perror("Failed to perform the NETWORK_ACC_CONFIG ");
  }

  return rc;
}

/**
 * @brief PS DDR <--->  PL DDR 回环
 *
 * @param dev
 * @return int
 */
int acc_start_test(network_acc_dev_t dev) {
  int rc;
  // 调用内核函数
  rc = ioctl(dev->fd, NETWORK_ACC_START);
  if (rc < 0) {
    perror("Failed to perform the NETWORK_ACC_START ");
  }
  return rc;
}

/**
 * @brief PS DDR <--->  PL DDR 回环
 *
 * @param dev
 * @return int
 */
int acc_get_test(network_acc_dev_t dev) {
  int rc;
  struct network_acc_reg iargs;

  // 调用内核函数
  rc = ioctl(dev->fd, NETWORK_ACC_GET, &iargs);
  if (rc < 0) {
    perror("Failed to perform the NETWORK_ACC_GET ");
  }

  dev->trans_addr = iargs.trans_addr;
  dev->trans_size = iargs.trans_size;
  dev->entropy_addr = iargs.entropy_addr;
  dev->entropy_size = iargs.entropy_size;

  printf("trans_addr: %#x \n trans_size: %#x\n entropy_addr: %#x \n "
         "entropy_size: %#x\n ",
         dev->trans_addr, dev->trans_size, dev->entropy_addr,
         dev->entropy_size);

  return rc;
}
