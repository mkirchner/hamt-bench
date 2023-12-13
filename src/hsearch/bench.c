#include <search.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <uuid/uuid.h>

#include "../utils.h"
#include "../numbers.h"

static int make_string_keys(char ***keys, size_t n, int *numbers)
{
    *keys = (char **) malloc(sizeof(char*) * n);
    for (size_t i = 0; i < n; ++i) {
        if (asprintf(&(*keys)[i], "%d", numbers[i]) < 0) {
            return -1;
        }
    }
    return 0;
}


static void perf_insert(const char *benchmark_id, const time_t timestamp,
                        size_t scale, size_t reps)
{

    int *numbers = make_numbers(scale, 0);
    /*
     * hsearch(3) has hard-coded key and value types:
     *
     *   The hsearch() function is a hash-table search routine. [...]
     *   The item argument is a structure of type ENTRY (defined
     *   in the <search.h> header) containing two pointers: item.key points to
     *   the comparison key (a char *), and item.data (a void *) points to any
     *   other data to be associated with that key.  The comparison function
     *   used by hsearch() is strcmp(3).
     * 
     * We create a char* array of keys, where the key is simply
     * the number as a string.
     *
     * In terms of memory management: hdestroy(3) will clean up the
     * array items (see below); all we need to do is to free the array
     * itself.
     */
    char **keys;
    make_string_keys(&keys, scale, numbers);

    /* insert 1% of scale items for test */
    int n_insert = 0.01 * scale;
    int *new_numbers = make_numbers(n_insert, scale);
    char **new_keys;

    struct TimeInterval ti_insert;
    ENTRY item;
    for (size_t i = 0; i < reps; ++i) {
        hcreate(2 * scale);
        for (size_t j = 0; j < scale; ++j) {
            /* 
             * Note that keys must be malloc(3)-allocated for hsearch(3):
             *
             *   The comparison key (passed to hsearch() as item.key)
             *   must be allocated using malloc(3) if action is ENTER
             *   and hdestroy() is called.
             * 
             * We duplicate the string keys with strdup(3) since we need to
             * create and destroy the hash table `reps` times. hdestroy(3) will
             * then clean up the duplicates and we only need to clean up the
             * original keys in the array and the array itself.
             */
            item.key = strdup(keys[j]);
            item.data = &numbers[j];
            if (!hsearch(item, ENTER)) {
                printf("Failed to insert item with key: %s\n", item.key);
                exit(1);
            }
        }
        /* shuffle input data */
        shuffle_numbers(new_numbers, n_insert);
        /* create the string keys required for hsearch */
        make_string_keys(&new_keys, n_insert, new_numbers);

        timer_start(&ti_insert);
        for (size_t i = 0; i < n_insert; i++) {
            item.key = new_keys[i];
            item.data = &new_numbers[i];
            if (!hsearch(item, ENTER)) {
                printf("Failed to insert key: %s.\n", item.key);
                exit(1);
            }
        }
        timer_stop(&ti_insert);

        /* cleanup in every iteration */
        hdestroy();  // delete hash table and all keys
        free(new_keys);  // delete the key array

        /* timing */
        double ns_per_insert = timer_nsec(&ti_insert) / (double)n_insert;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "insert", scale, ns_per_insert);
    }

    /* cleanup */
    for (size_t i = 0; i < scale; ++i) {
        free(keys[i]);
    }
    free(keys);
    free(numbers);
    free(new_numbers);
}

static void perf_query(const char *benchmark_id, const time_t timestamp,
                       size_t scale, size_t reps)
{
    int *numbers = make_numbers(scale, 0);
    /*
     * hsearch(3) has hard-coded key and value types: see perf_query
     * implementation for additional comments
     */
    char **keys;
    make_string_keys(&keys, scale, numbers);

    /* query 5% of scale items for test */
    int n_query = 0.05 * scale;
    int *new_numbers = make_numbers(n_query, scale);
    char **new_keys;

    ENTRY item;
    /* load table */
    hcreate(2 * scale); /* make sure we don't need to resize */
    for (size_t i = 0; i < scale; i++) {
        item.key = keys[i]; /* hdestroy will free the keys */
        item.data = &numbers[i];
        if (!hsearch(item, ENTER)) {
            printf("Failed to insert item: %s.\n", item.key);
            exit(1);
        }
    }

    struct TimeInterval ti_query;
    double ns_per_query;
    for (size_t i = 0; i < reps; ++i) {
        /* shuffle input data */
        shuffle_numbers(new_numbers, n_query);
        /* create the string keys required for hsearch */
        make_string_keys(&new_keys, n_query, new_numbers);

        timer_start(&ti_query);
        for (size_t j = 0; j < n_query; j++) {
            item.key = new_keys[j];
            hsearch(item, FIND);
        }
        timer_stop(&ti_query);
        for (size_t j = 0; j < n_query; ++j) {
            free(new_keys[j]);
        }
        free(new_keys);
        ns_per_query = timer_nsec(&ti_query) / (double)scale;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "query", scale, ns_per_query);
    }
    /* cleanup */
    hdestroy();
    free(keys);
    free(numbers);
}
int main(int argc, char **argv)
{
    /* generate a benchmark id */
    uuid_t uuid;
    uuid_generate_random(uuid);
    char benchmark_id[37];
    uuid_unparse_lower(uuid, benchmark_id);

    /* get a timestamp */
    time_t now = time(0);

    size_t scale[] = {1e3, 1e4, 1e5, 1e6};
    size_t n_scales = 4;

    /* run the performance measurements */
    srand(now);
    for (size_t i = 0; i < n_scales; ++i) {
        perf_insert(benchmark_id, now, scale[i], 20);
    }
    for (size_t i = 0; i < n_scales; ++i) {
        perf_query(benchmark_id, now, scale[i], 20);
    }
    /*
    for (size_t i = 0; i < n_scales; ++i) {
        perf_remove(benchmark_id, now, scale[i], 20);
    }
    */
    return 0;
}
