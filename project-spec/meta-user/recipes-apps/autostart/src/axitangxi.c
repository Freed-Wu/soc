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

void *ps_mmap(int fd_dev, size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dev, 0);
}

static void init_trans(struct axitangxi_transaction *trans, void *ps_addr,
                       uint32_t pl_addr, uint32_t size) {
  trans->tx_data_size = trans->rx_data_size = size;
  trans->burst_size = BURST_SIZE;
  trans->burst_data = 16 * BURST_SIZE;
  // ceil(a / b) = floor((a - 1) / b) + 1
  trans->burst_count = (size - 1) / (BURST_SIZE * 16) + 1;
  trans->tx_data_ps_ptr = trans->rx_data_ps_ptr = ps_addr;
  trans->rx_data_pl_ptr = trans->tx_data_pl_ptr = pl_addr;
}

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

ssize_t pl_write(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size) {
  return pl_io(fd_dev, ps_addr, pl_addr, size, PSDDR_TO_PLDDR);
}

ssize_t pl_read(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size) {
  return pl_io(fd_dev, ps_addr, pl_addr, size, PLDDR_TO_PSDDR);
}

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

void pl_run(int fd_dev, struct network_acc_reg *reg) {
  if (ioctl(fd_dev, NETWORK_ACC_CONFIG, reg) == -1)
    err(errno, AXITX_DEV_PATH);
  if (ioctl(fd_dev, NETWORK_ACC_START) == -1)
    err(errno, AXITX_DEV_PATH);
}

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
}

uint16_t complete_to_original16(uint16_t code) {
  if ((code >> 15) & 1)
    code = ~(code & 0x7FFF) + 1;
  return code;
}

void complete_to_original16s(uint16_t *code, size_t len) {
  for (uint16_t *p_code = code; p_code < code + len; p_code++)
    *p_code = complete_to_original16(*p_code);
}

/**
 * will block!
 */
void pl_get(int fd_dev, struct network_acc_reg *reg, uint16_t *trans_addr,
            uint16_t *entropy_addr) {
  if (ioctl(fd_dev, NETWORK_ACC_GET, reg) == -1)
    err(errno, AXITX_DEV_PATH);
  if (pl_read(fd_dev, trans_addr, reg->trans_addr, reg->trans_size) == -1)
    err(errno, AXITX_DEV_PATH);
  if (pl_read(fd_dev, entropy_addr, reg->entropy_addr, reg->entropy_size) == -1)
    err(errno, AXITX_DEV_PATH);
  complete_to_original16s(trans_addr, reg->trans_size);
  complete_to_original16s(entropy_addr, reg->entropy_size);
}
