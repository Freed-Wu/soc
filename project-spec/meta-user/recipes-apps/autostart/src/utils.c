#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

uint8_t *mirror_padding(bool is_uv, ssize_t yuv_len, int16_t *dst, uint8_t *src,
                        size_t len) {
  // k yuv_len p dst
  int16_t *p_yuv;
  uint8_t *p = src;
  int height = ((2160 / 64 + 1) * 64 - 2160) / 2, width = 3840;
  if (is_uv) {
    height /= 2;
    width /= 2;
  }
  if (yuv_len == 3840 * 2160) {
    p_yuv = dst + (height - 1) * width;
    for (int i = height - 1; i >= 0; i--) {
      for (int j = 0; j < width; j++) {
        *p_yuv++ = *p++;
      }
      p_yuv -= 3840 * 2;
    }
  }
  p_yuv = dst;
  p = src;
  for (int i = 0; i < len; ++i)
    *p_yuv++ = *p++;
  uint8_t *p_end = p;
  if (yuv_len == 3840 * 2160) {
    p_yuv = dst + (height - 1) * width;
    for (int i = height - 1; i >= 0; i--) {
      for (int j = 0; j < width; j++) {
        *p_yuv++ = *p++;
      }
      p -= 3840 * 2;
    }
  }
  return p_end;
}

int print_help(const struct option *longopts, const char *arg0) {
  unsigned int i = 0;
  char *base = strdup(arg0);
  printf("%s", basename(base));
  free(base);
  struct option o = longopts[i];
  while (o.name != NULL) {
    char *name = malloc(strlen(o.name) + strlen("(|0)"));
    char *value = malloc(strlen(o.name) + strlen("( )"));
    char *meta = malloc(strlen(o.name));
    char *str = meta;
    if (name == NULL || value == NULL || meta == NULL)
      return EXIT_FAILURE;

    if (isascii(o.val))
      sprintf(name, "(--%s|-%c)", o.name, (char)o.val);
    else
      sprintf(name, "--%s", o.name);

    sprintf(meta, "%s", o.name);
    do
      *str = (char)toupper(*str);
    while (*str++);

    if (o.has_arg == required_argument)
      sprintf(value, " %s", meta);
    else if (o.has_arg == optional_argument)
      sprintf(value, "( %s)", meta);
    else
      value[0] = '\0';

    printf(" [%s%s]", name, value);

    free(name);
    free(value);
    free(meta);

    o = longopts[++i];
  }
  return EXIT_SUCCESS;
}

int mkdir_p(const char *path, mode_t mode) {
  char *dir = strdup(path);
  dirname(dir);
  struct stat st;
  if (stat(dir, &st) == -1) {
    return mkdir(dir, mode);
  }
  return 0;
}

/**
 * for debug
 */
ssize_t dump_mem(char *filename, void *addr, size_t size) {
  if (mkdir_p(filename, 0755) == -1)
    return -2;
  int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd == -1)
    return -3;
  ssize_t _size = write(fd, addr, size);
  if (close(fd) == -1)
    return -4;
  return _size;
}

void fd_to_epoll_fds(int fd, int *send_fd, int *recv_fd) {
  if (send_fd != NULL) {
    *send_fd = epoll_create(1);
    struct epoll_event send_event = {.events = EPOLLOUT, .data.fd = fd};
    epoll_ctl(*send_fd, EPOLL_CTL_ADD, fd, &send_event);
  }

  if (recv_fd != NULL) {
    *recv_fd = epoll_create(1);
    struct epoll_event recv_event = {.events = EPOLLIN, .data.fd = fd};
    epoll_ctl(*recv_fd, EPOLL_CTL_ADD, fd, &recv_event);
  }
}
