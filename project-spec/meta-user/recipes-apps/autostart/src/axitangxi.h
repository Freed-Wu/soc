#ifndef AXITANGXI_H
#define AXITANGXI_H 1
#include "axitangxi_ioctl.h"

__BEGIN_DECLS

ssize_t ps_read_file(int fd_dev, char *filename, void *addr);

ssize_t pl_write(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size);
ssize_t pl_read(int fd_dev, void *ps_addr, uint32_t pl_addr, uint32_t size);
ssize_t pl_config(int fd_dev, char *filename, uint32_t pl_addr,
                  uint32_t *p_size);
void pl_run(int fd_dev, struct network_acc_reg *reg);
void pl_init(int fd_dev, struct network_acc_reg *reg, char *weight_filename,
             uint32_t weight_addr, char *quantify_filename,
             uint32_t quantify_addr);
void pl_get(int fd_dev, struct network_acc_reg *reg, void *trans_addr,
            void *entropy_addr);

ssize_t dump_mem(char *filename, void *ps_addr, size_t size);

__END_DECLS
#endif /* axitangxi.h */
