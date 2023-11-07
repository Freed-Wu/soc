#ifndef CODING_H
#define CODING_H 1
#include <sys/cdefs.h>
__BEGIN_DECLS

#define OUTPUT "/tmp/output.bin"

double normal_cdf(double index, double mean, double std);
void *coding();

__END_DECLS
#endif /* coding.h */
