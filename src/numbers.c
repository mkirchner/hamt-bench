#include "numbers.h"

#include <stdlib.h>

/*
 * Create an integer sequence k, k+1, ..., k + n.
 */
int *make_numbers(const size_t n, const size_t k)
{
    int *numbers = (int *)malloc(n * sizeof(int));
    if (numbers) {
        for (int i = 0; i < n; ++i)
            numbers[i] = k + i;
    }
    return numbers;
}

/*
 * Shuffle numbers in-place.
 */
int *shuffle_numbers(int *arr, size_t size)
{
    int tmp;
    for (size_t i = 0; i < size - 1; ++i) {
        size_t j = drand48() * (i + 1);
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
    return arr;
}
