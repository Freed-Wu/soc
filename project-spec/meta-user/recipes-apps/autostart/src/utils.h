#ifndef UTILS_H
#define UTILS_H 1
#include <stdlib.h>

#include "config.h"
__BEGIN_DECLS

#define VERSION                                                                \
  PROJECT_VERSION "\n"                                                         \
                  "Copyright (C) 2023\n"                                       \
                  "Written by Wu Zhenyu <wuzhenyu@ustc.edu>\n"

enum LOG_LEVEL {
  LOG_VERBOSE,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_FATAL,
};

void print_log(char *, ...);
int print_help(const struct option *longopts, const char *arg0);
ssize_t dump_mem(char *filename, void *addr, size_t size);
void fd_to_epoll_fds(int fd, int *send_fd, int *recv_fd);

__END_DECLS
#endif /* utils.h */
