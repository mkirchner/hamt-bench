#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <uuid/uuid.h>

#include "rb.h"
#include "../words.h"
#include "../utils.h"


static int cmp_strcmp(const void *lhs, const void *rhs, void *rb_param)
{
    (void) rb_param;
    /* expects lhs and rhs to be pointers to 0-terminated strings */
    return strcmp((const char *)lhs, (const char *)rhs);
}

static void perf_query(const char *benchmark_id, const time_t timestamp, size_t table_size, size_t reps)
{
    char **words = NULL;
    char **shuffled = NULL;
    struct rb_table *t;

    words_load_numbers(&words, 0, table_size);

    /* load table */
    t = rb_create(cmp_strcmp, NULL, &rb_allocator_default);
    for (size_t i = 0; i < table_size; i++) {
        rb_insert(t, words[i]);
    };

    struct TimeInterval ti_query;
    double ns_per_query;
    for (size_t i = 0; i < reps; ++i) {
        shuffled = words_create_shuffled_refs(words, table_size);
        timer_start(&ti_query);
        for (size_t i = 0; i < table_size; i++) {
            rb_find(t, shuffled[i]);
        }
        timer_stop(&ti_query);
        words_free_refs(shuffled);
        ns_per_query = timer_nsec(&ti_query) / (double)table_size;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i, "query", table_size, ns_per_query);
    }
    /* cleanup */
    rb_destroy(t, NULL);
    words_free(words, table_size);
}

static void perf_insert(const char *benchmark_id, const time_t timestamp, size_t scale, size_t reps)
{
    char **words = NULL;
    char **new_words = NULL;
    char **shuffled = NULL;
    struct rb_table *t;

    words_load_numbers(&words, 0, scale);
    /* insert 1% of scale words for test */
    size_t n_insert = scale * 0.01;
    words_load_numbers(&new_words, scale+1, n_insert);

    struct TimeInterval ti_insert;
    for (size_t i = 0; i < reps; ++i) {
        /* create new HAMT */
        t = rb_create(cmp_strcmp, NULL, &rb_allocator_default);
        for (size_t i = 0; i < scale; i++) {
            rb_insert(t, words[i]);
        }
        /* shuffle input data */
        shuffled = words_create_shuffled_refs(new_words, scale);
        timer_start(&ti_insert);
        for (size_t i = 0; i < n_insert; i++) {
            rb_insert(t, new_words[i]);
        }
        timer_stop(&ti_insert);
        rb_destroy(t, NULL);
        words_free_refs(shuffled);
        double ns_per_insert = timer_nsec(&ti_insert) / (double) n_insert;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i, "insert", scale, ns_per_insert);
    }
    words_free(new_words, n_insert);
    words_free(words, scale);
}

static void perf_remove(const char *benchmark_id, const time_t timestamp, size_t scale, size_t reps)
{
    char **words = NULL;
    char **shuffled = NULL;
    struct rb_table *t;

    words_load_numbers(&words, 0, scale);
    /* remove 1% of scale words for test */
    size_t n_remove = scale * 0.01;

    struct TimeInterval ti_remove;
    for (size_t i = 0; i < reps; ++i) {
        /* create new HAMT */
        t = rb_create(cmp_strcmp, NULL, &rb_allocator_default);
        for (size_t i = 0; i < scale; i++) {
            rb_insert(t, words[i]);
        }
        /* shuffle input data */
        shuffled = words_create_shuffled_refs(words, scale);
        timer_start(&ti_remove);
        /* delete the first n_remove entries */
        for (size_t i = 0; i < n_remove; i++) {
            rb_delete(t, words[i]);
        }
        timer_stop(&ti_remove);
        rb_destroy(t, NULL);
        words_free_refs(shuffled);
        double ns_per_remove = timer_nsec(&ti_remove) / (double) n_remove;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i, "remove", scale, ns_per_remove);
    }
    words_free(words, scale);
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
