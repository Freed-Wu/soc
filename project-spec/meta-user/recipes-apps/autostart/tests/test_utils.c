#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../src/main.h"

#define PROJECT_VERSION __FILE__
#include "../src/utils.h"

int main(int argc, char *argv[]) {
  uint8_t addr[3840 * 2160];
  for (int j = 0; j < 2160; j++) {
    for (int i = 0; i < 3840; i++) {
      addr[i * 2160 + j] = (i + j) % 256;
    }
  }
  uint8_t *p = addr;
  size_t real_yuv_len = 3840 * (2160 / 64 + 1) * 64;
  ssize_t u_len = real_yuv_len / (4 + 1 + 1);
  data_t yuv[3] = {
      // uint16_t to uint8_t
      // len to size
      {.addr = malloc(u_len * 4 * 2), .len = u_len * 4},
      {.addr = malloc(u_len * 2), .len = u_len},
      {.addr = malloc(u_len * 2), .len = u_len},
  };
  for (int k = 0; k < 3; k++) {
    p = mirror_padding(k > 1, 3840 * 2160, yuv[k].addr, p, yuv[k].len);
  }
  return EXIT_SUCCESS;
}
