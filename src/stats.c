#include "stats.h"

#include <stdlib.h>
#include <string.h>

int cmp_double(const void *lhs, const void *rhs)
{
    const double *l = (const double *)lhs;
    const double *r = (const double *)rhs;
    if (*l < *r)
        return -1;
    return *l == *r ? 0 : 1;
}

double trimmed_mean(double *arr, size_t arrsize, double p)
{
    double *copy = (double *)malloc(arrsize * sizeof(double));
    memcpy(copy, arr, arrsize * sizeof(double));
    qsort(copy, arrsize, sizeof(double), cmp_double);
    size_t lower_ix = (int)(p * arrsize);
    size_t upper_ix = arrsize - lower_ix;
    double sum = 0.0;
    for (size_t i = lower_ix; i < upper_ix; ++i) {
        // printf("copy[%lu] = %0.2f\n", i, copy[i]);
        sum += copy[i];
    }
    free(copy);
    return sum / (upper_ix - lower_ix);
}
