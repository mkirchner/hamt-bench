#include <glib.h>
#include <stdio.h>

#include <uuid/uuid.h>

#include "../numbers.h"
#include "../utils.h"

static void perf_insert(const char *benchmark_id, const time_t timestamp,
                        size_t scale, size_t reps)
{
    int *numbers = make_numbers(scale, 0);

    /* insert 1% of scale words for test */
    int n_insert = 0.01 * scale;
    int *new_numbers = make_numbers(n_insert, scale);

    struct TimeInterval ti_insert;
    GHashTable *ht;
    for (size_t i = 0; i < reps; ++i) {
        /* create new HAMT */
        ht = g_hash_table_new(g_int_hash, g_int_equal);
        for (size_t i = 0; i < scale; i++) {
            g_hash_table_insert(ht, &numbers[i], &numbers[i]);
        }
        /* shuffle input data */
        shuffle_numbers(new_numbers, n_insert);

        /* insert */
        timer_start(&ti_insert);
        for (size_t i = 0; i < n_insert; i++) {
            g_hash_table_insert(ht, &new_numbers[i], &new_numbers[i]);
        }
        timer_stop(&ti_insert);
        g_hash_table_destroy(ht);
        double ns_per_insert = timer_nsec(&ti_insert) / (double)n_insert;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "insert", scale, ns_per_insert);
    }
    free(new_numbers);
    free(numbers);
}

static void perf_query(const char *benchmark_id, const time_t timestamp,
                       size_t scale, size_t reps)
{
    int *numbers = make_numbers(scale, 0);
    int *query_numbers = make_numbers(scale, 0);

    /* load table */
    GHashTable *ht = g_hash_table_new(g_int_hash, g_int_equal);
    for (size_t i = 0; i < scale; i++) {
        g_hash_table_insert(ht, &numbers[i], &numbers[i]);
    }

    struct TimeInterval ti_query;
    double ns_per_query;
    for (size_t i = 0; i < reps; ++i) {
        shuffle_numbers(query_numbers, scale);
        timer_start(&ti_query);
        for (size_t i = 0; i < scale; i++) {
            g_hash_table_lookup(ht, &numbers[i]);
        }
        timer_stop(&ti_query);
        ns_per_query = timer_nsec(&ti_query) / (double)scale;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "query", scale, ns_per_query);
    }
    /* cleanup */
    g_hash_table_destroy(ht);
    free(query_numbers);
    free(numbers);
}

static void perf_remove(const char *benchmark_id, const time_t timestamp,
                        size_t scale, size_t reps)
{
    GHashTable *ht;

    int *numbers = make_numbers(scale, 0);
    int *rem_numbers = make_numbers(scale, 0);

    /* remove 1% of scale words for test */
    size_t n_remove = scale * 0.01;

    struct TimeInterval ti_remove;
    for (size_t i = 0; i < reps; ++i) {
        ht = g_hash_table_new(g_int_hash, g_int_equal);
        for (size_t i = 0; i < scale; i++) {
            g_hash_table_insert(ht, &numbers[i], &numbers[i]);
        }
        /* shuffle input data */
        shuffle_numbers(rem_numbers, scale);

        timer_start(&ti_remove);
        /* delete the first n_remove entries */
        for (size_t i = 0; i < n_remove; i++) {
            g_hash_table_remove(ht, &rem_numbers[i]);
        }
        timer_stop(&ti_remove);
        g_hash_table_destroy(ht);
        double ns_per_remove = timer_nsec(&ti_remove) / (double)n_remove;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "remove", scale, ns_per_remove);
    }
    free(rem_numbers);
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
        perf_query(benchmark_id, now, scale[i], 20);
    }
    for (size_t i = 0; i < n_scales; ++i) {
        perf_insert(benchmark_id, now, scale[i], 20);
    }
    for (size_t i = 0; i < n_scales; ++i) {
        perf_remove(benchmark_id, now, scale[i], 20);
    }
    return 0;
}
