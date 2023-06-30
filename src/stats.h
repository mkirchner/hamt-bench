#ifndef STATS_C
#define STATS_C

#include <stddef.h>

int cmp_double(const void *lhs, const void *rhs);
double trimmed_mean(double *arr, size_t arrsize, double p);

#endif /* STATS_C */
