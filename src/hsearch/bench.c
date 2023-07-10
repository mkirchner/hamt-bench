#include <search.h>
#include <stdio.h>
#include <string.h>

#include <uuid/uuid.h>

#include "../utils.h"
#include "../words.h"

static void perf_insert(const char *benchmark_id, const time_t timestamp,
                        size_t scale, size_t reps)
{
    char **words = NULL;
    char **new_words = NULL;
    char **shuffled = NULL;

    words_load_numbers(&words, 0, scale);
    /* insert 1% of scale words for test */
    size_t n_insert = scale * 0.01;
    words_load_numbers(&new_words, scale + 1, n_insert);

    struct TimeInterval ti_insert;
    ENTRY item;
    for (size_t i = 0; i < reps; ++i) {
        hcreate(2 * scale);
        for (size_t i = 0; i < scale; i++) {
            item.key = strdup(words[i]); /* hdestroy will free the keys */
            item.data = words[i];
            if (!hsearch(item, ENTER)) {
                printf("Failed to insert all items.\n");
                exit(1);
            }
        }
        /* shuffle input data */
        shuffled = words_create_shuffled_refs(new_words, scale);
        /* duplicate keys since hdestroy() will free() the keys and we don't
         * want to measure strdup performance in the hot loop */
        char *duped_keys[n_insert];
        for (size_t i = 0; i < n_insert; i++) {
            duped_keys[i] = strdup(new_words[i]);
        }

        timer_start(&ti_insert);
        for (size_t i = 0; i < n_insert; i++) {
            item.key = duped_keys[i];
            item.data = new_words[i];
            if (!hsearch(item, ENTER)) {
                printf("Failed to insert all items.\n");
                exit(1);
            }
        }
        timer_stop(&ti_insert);

        hdestroy();
        words_free_refs(shuffled);
        double ns_per_insert = timer_nsec(&ti_insert) / (double)n_insert;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "insert", scale, ns_per_insert);
    }

    words_free(new_words, n_insert);
    words_free(words, scale);
}

static void perf_query(const char *benchmark_id, const time_t timestamp,
                       size_t scale, size_t reps)
{
    char **words = NULL;
    char **shuffled = NULL;
    ENTRY item;

    words_load_numbers(&words, 0, scale);

    /* load table */
    hcreate(2 * scale); /* make sure we don't need to resize */
    for (size_t i = 0; i < scale; i++) {
        item.key = strdup(words[i]); /* hdestroy will free the keys */
        item.data = words[i];
        if (!hsearch(item, ENTER)) {
            printf("Failed to insert all items.\n");
            exit(1);
        }
    }

    struct TimeInterval ti_query;
    double ns_per_query;
    for (size_t i = 0; i < reps; ++i) {
        shuffled = words_create_shuffled_refs(words, scale);
        timer_start(&ti_query);
        for (size_t i = 0; i < scale; i++) {
            item.key = words[i];
            hsearch(item, FIND);
        }
        timer_stop(&ti_query);
        words_free_refs(shuffled);
        ns_per_query = timer_nsec(&ti_query) / (double)scale;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", timestamp, benchmark_id, i,
               "query", scale, ns_per_query);
    }
    /* cleanup */
    hdestroy();

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
