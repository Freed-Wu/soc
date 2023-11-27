#ifndef AXITANGXI_H
#define AXITANGXI_H 1
#include "axitangxi_ioctl.h"

__BEGIN_DECLS

void *ps_mmap(int fd_dev, size_t size);
ssize_t ps_read_file(int fd_dev, char *filename, void *addr);

ssize_t pl_write(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size);
ssize_t pl_read(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size);
ssize_t pl_config(int fd_dev, char *filename, uint32_t pl_addr,
                  uint32_t *p_size);
void pl_run(int fd_dev, struct network_acc_reg *reg);
void pl_init(int fd_dev, struct network_acc_reg *reg, char *weight_filename,
             uint32_t weight_addr, char *quantify_filename,
             uint32_t quantify_addr);
void pl_get(int fd_dev, struct network_acc_reg *reg, uint16_t *trans_addr,
            uint16_t *entropy_addr);

uint16_t complete_to_original16(uint16_t code);
void complete_to_original16s(uint16_t *code, size_t len);

ssize_t dump_mem(char *filename, void *ps_addr, size_t size);

__END_DECLS
#endif /* axitangxi.h */
