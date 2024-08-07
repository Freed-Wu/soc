#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "axitangxi.h"
#include "axitangxi_ioctl.h"

/**
 * use `mmap()` to allocate a memory shared by kernel space and user space
 *
 * @param fd_dev file descriptor of device
 * @param size size of allocated bytes
 * @returns memory address
 */
void *ps_mmap(int fd_dev, size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dev, 0);
}

/**
 * initial the struct needed by reading/writing data between FPGA ROM and kernel
 * space
 *
 * @param trans struct needed to be initialized
 * @param ps_addr address of the memory allocated by `ps_mmap()`
 * @param pl_addr address of FPAG ROM
 * @param size size of allocated bytes which FPGA uses
 */
static void init_trans(struct axitangxi_transaction *trans, void *ps_addr,
                       uint32_t pl_addr, uint32_t size) {
  trans->tx_data_size = trans->rx_data_size = size;
  trans->burst_size = BURST_SIZE;
  trans->burst_data = 16 * BURST_SIZE;
  // ceil(a / b) = floor((a - 1) / b) + 1
  trans->burst_count = (size - 1) / trans->burst_data + 1;
  trans->tx_data_ps_ptr = trans->rx_data_ps_ptr = ps_addr;
  trans->rx_data_pl_ptr = trans->tx_data_pl_ptr = pl_addr;
}

/**
 * read or write data between FPGA ROM and kernel
 *
 * @param fd_dev file descriptor of device
 * @param ps_addr address of the memory allocated by `ps_mmap()`
 * @param pl_addr address of FPAG ROM
 * @param size size of allocated bytes which FPGA uses
 * @param request read or write
 * @returns size of reading/writing data if positive, error code if negative
 */
ssize_t pl_io(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size,
              unsigned long request) {
  if (ps_addr == NULL)
    ps_addr = ps_mmap(fd_dev, size);
  if (ps_addr == MAP_FAILED)
    return -1;
  struct axitangxi_transaction trans;
  init_trans(&trans, ps_addr, pl_addr, size);
  if (ioctl(fd_dev, request, &trans) == -1)
    return -1;
  return size;
}

/**
 * write data to FPGA ROM from kernel space
 *
 * @param fd_dev file descriptor of device
 * @param ps_addr address of the memory allocated by `ps_mmap()`
 * @param pl_addr address of FPAG ROM
 * @param size size of allocated bytes which FPGA uses
 * @returns size of reading/writing data if positive, error code if negative
 */
ssize_t pl_write(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size) {
  return pl_io(fd_dev, ps_addr, pl_addr, size, PSDDR_TO_PLDDR);
}

/**
 * read data from FPGA ROM to kernel space
 *
 * @param fd_dev file descriptor of device
 * @param ps_addr address of the memory allocated by `ps_mmap()`
 * @param pl_addr address of FPAG ROM
 * @param size size of allocated bytes which FPGA uses
 * @returns size of reading/writing data if positive, error code if negative
 */
ssize_t pl_read(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size) {
  return pl_io(fd_dev, ps_addr, pl_addr, size, PLDDR_TO_PSDDR);
}

/**
 * read data from a hard disk file to kernel space
 *
 * @param fd_dev file descriptor of device
 * @param filename file name
 * @param addr memory address
 * @returns size of reading/writing data if positive, error code if negative
 */
ssize_t ps_read_file(int fd_dev, const char *filename, void *addr) {
  int fd = open(filename, O_RDWR);
  if (fd == -1)
    return -1;
  struct stat status;
  if (fstat(fd, &status) == -1)
    return -1;
  addr = ps_mmap(fd_dev, status.st_size);
  if (addr == MAP_FAILED)
    return -1;
  ssize_t size = read(fd, addr, status.st_size);
  if (close(fd) == -1)
    return -1;
  return size;
}

/**
 * read data from a file then write it to FPGA ROM
 *
 * @param fd_dev file descriptor of device
 * @param filename file name
 * @param pl_addr address of FPAG ROM
 * @param p_size size of file
 * @returns size of reading/writing data if positive, error code if negative
 */
ssize_t pl_config(int fd_dev, const char *filename, uint32_t pl_addr,
                  uint32_t *p_size) {
  void *ps_addr = NULL;
  *p_size = ps_read_file(fd_dev, filename, ps_addr);
  if (*p_size == -1)
    return -2;
  ssize_t ssize = pl_write(fd_dev, ps_addr, pl_addr, *p_size);
  if (munmap(ps_addr, *p_size) == -1)
    return -3;
  return ssize;
}

/**
 * run network accelerator
 *
 * @param fd_dev file descriptor of device
 * @param reg network accelerator register configuration
 */
void pl_run(int fd_dev, struct network_acc_reg *reg) {
  if (ioctl(fd_dev, NETWORK_ACC_CONFIG, reg) == -1)
    err(errno, AXITX_DEV_PATH);
  if (ioctl(fd_dev, NETWORK_ACC_START) == -1)
    err(errno, AXITX_DEV_PATH);
}

/**
 * initialize network accelerator
 *
 * @param fd_dev file descriptor of device
 * @param reg network accelerator register configuration
 * @param weight_filename weight file name
 * @param weight_addr weight address of FPAG ROM
 * @param quantify_filename quantify file name
 * @param quantify_addr quantify address of FPAG ROM
 */
void pl_init(int fd_dev, struct network_acc_reg *reg,
             const char *weight_filename, uint32_t weight_addr,
             const char *quantify_filename, uint32_t quantify_addr) {
  if (pl_config(fd_dev, weight_filename, reg->weight_addr = weight_addr,
                &reg->weight_size) == -1)
    err(errno, "%s", weight_filename);
  // TODO: use quantify_filename to config builtin quantization coefficience
  // if (pl_config(fd_dev, quantify_filename, reg->quantify_addr =
  // quantify_addr,
  //               &reg->quantify_size) == -1)
  //   err(errno, "%s", quantify_filename);
  // reg->quantify_size /= 16;
}

/**
 * get results from network accelerator after `pl_run()`. It will **block**!
 * use int16_t to get original code
 *
 * @param fd_dev file descriptor of device
 * @param reg network accelerator register configuration
 * @param trans_addr transform coefficience address of kernel space, pass NULL
 * to allocate it
 * @param entropy_addr entropy coefficience address of kernel space, pass NULL
 * to allocate it
 */
void pl_get(int fd_dev, struct network_acc_reg *reg, int16_t *trans_addr,
            int16_t *entropy_addr) {
  if (ioctl(fd_dev, NETWORK_ACC_GET, reg) == -1)
    err(errno, AXITX_DEV_PATH);
  if (pl_read(fd_dev, trans_addr, reg->trans_addr, reg->trans_size) == -1)
    err(errno, AXITX_DEV_PATH);
  if (pl_read(fd_dev, entropy_addr, reg->entropy_addr, reg->entropy_size) == -1)
    err(errno, AXITX_DEV_PATH);
}
