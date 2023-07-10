#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/stats.c"

MU_TEST_CASE(test_trimmed_mean)
{
    printf(". testing trimmed mean\n");
    double numbers[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    struct {
        double trim;
        double expected_mean;
    } test_cases[4] = {{0.0, 0.4}, {0.1, 0.375}, {0.2, 1.0 / 3.0}, {0.3, 0.25}};
    for (size_t i = 0; i < 4; ++i) {
        double mean = trimmed_mean(numbers, 10, test_cases[i].trim);
        MU_ASSERT(mean == test_cases[i].expected_mean,
                  "Error in trimmed mean calculation");
    }
    return 0;
}

int mu_tests_run = 0;

MU_TEST_SUITE(test_suite)
{
    MU_RUN_TEST(test_trimmed_mean);
    return 0;
}

int main()
{
    printf("---=[ Hash array mapped trie benchmarking suite tests\n");
    char *result = test_suite();
    if (result != 0) {
        printf("%s\n", result);
    } else {
        printf("All tests passed.\n");
    }
    printf("Tests run: %d\n", mu_tests_run);
    return result != 0;
}
