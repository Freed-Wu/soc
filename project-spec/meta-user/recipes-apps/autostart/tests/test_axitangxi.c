#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/axitangxi.h"

// 权重、因子、图片的地址
#define WEIGHT_ADDR 0x10000000
#define QUANTIFY_ADDR 0x10010000
#define PICTURE_BASE_ADDR 0x20000000

int main(int argc, const char *argv[]) {
  int ret = EXIT_SUCCESS;
  int fd_dev = open(AXITX_DEV_PATH, O_RDWR);
  if (fd_dev == -1)
    // https://mesonbuild.com/Unit-tests.html#skipped-tests-and-hard-errors
    err(77, AXITX_DEV_PATH);
  struct network_acc_reg reg;
  pl_init(fd_dev, &reg, "/usr/share/autostart/weight.bin", WEIGHT_ADDR, "",
          QUANTIFY_ADDR);

  /*int fd_yuv = open(argv[1], O_RDONLY | O_NONBLOCK);*/
  /*if (fd_yuv == -1)*/
  /*  err(errno, "%s", argv[1]);*/
  struct stat status;
  /*if (fstat(fd_yuv, &status) == -1)*/
  /*  err(errno, "%s", argv[1]);*/
  status.st_size = 1024;
  int16_t *yuv_addr = ps_mmap(fd_dev, status.st_size);
  memset(yuv_addr, 1, status.st_size);
  /*if (read(fd_yuv, yuv_addr, status.st_size) != status.st_size)*/
  /*  err(errno, "%s", argv[1]);*/
  /*if (close(fd_yuv) == -1)*/
  /*  err(errno, "%s", argv[1]);*/

  reg.picture_size = status.st_size;
  if (pl_write(fd_dev, (void **)&yuv_addr, reg.picture_addr = PICTURE_BASE_ADDR,
               status.st_size) == -1)
    err(errno, AXITX_DEV_PATH);
  munmap(yuv_addr, status.st_size);

  int16_t *yuv2_addr = ps_mmap(fd_dev, status.st_size);
  if (pl_read(fd_dev, (void **)&yuv2_addr, reg.picture_addr, status.st_size) ==
      -1)
    err(errno, AXITX_DEV_PATH);

  for (int i = 0; i < status.st_size; i++) {
    if (yuv_addr[i] != yuv2_addr[i]) {
      fprintf(stderr, "PL read/write inconsistent");
      ret = EXIT_FAILURE;
    }
  }

  int16_t *trans_addr = NULL, *entropy_addr = NULL;
  pl_run(fd_dev, &reg);
  pl_get(fd_dev, &reg, &trans_addr, &entropy_addr);

  int fd_trans = open("trans.bin", O_RDWR | O_CREAT | O_NONBLOCK, 0644);
  if (fd_trans == -1)
    err(errno, "trans.bin");
  int size = write(fd_trans, trans_addr, reg.trans_size * 2);
  if (size != reg.trans_size * 2)
    fprintf(stderr, "trans.bin: %x/%x data is written", size,
            reg.trans_size * 2);
  if (close(fd_trans) == -1)
    err(errno, "trans.bin");

  int fd_entropy = open("entropy.bin", O_RDWR | O_CREAT | O_NONBLOCK, 0644);
  if (fd_entropy == -1)
    err(errno, "entropy.bin");
  size = write(fd_entropy, entropy_addr, reg.entropy_size * 2);
  if (size != reg.entropy_size * 2)
    fprintf(stderr, "entropy.bin: %x/%x data is written", size,
            reg.entropy_size * 2);
  if (write(fd_entropy, entropy_addr, reg.entropy_size * 2) !=
      reg.entropy_size * 2)
    err(errno, "entropy.bin");
  if (close(fd_entropy) == -1)
    err(errno, "entropy.bin");

  if (close(fd_dev) == -1)
    err(errno, NULL);
  return ret;
}
