#ifndef UTILS_H
#define UTILS_H 1
#include <stdlib.h>

#include "config.h"
__BEGIN_DECLS

#define VERSION                                                                \
  PROJECT_VERSION "\n"                                                         \
                  "Copyright (C) 2023\n"                                       \
                  "Written by Wu Zhenyu <wuzhenyu@ustc.edu>\n"

int print_help(const struct option *longopts, const char *arg0);
ssize_t dump_mem(char *filename, void *addr, size_t size);

__END_DECLS
#endif /* utils.h */
