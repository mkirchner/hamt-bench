#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uuid/uuid.h>

#include <gperftools/profiler.h>

#include "../../../hamt/include/hamt.h"
#include "../../../hamt/include/murmur3.h"
#include "../words.h"
#include "../utils.h"

#include "gc.h"

void nop(void *_) { return; }

struct HamtAllocator hamt_allocator_gc = {GC_malloc, GC_realloc, nop};

static uint32_t my_keyhash_string(const void *key, const size_t gen)
{
    uint32_t hash = murmur3_32((uint8_t *)key, strlen((const char *)key), gen);
    return hash;
}

static int my_keycmp_string(const void *lhs, const void *rhs)
{
    /* expects lhs and rhs to be pointers to 0-terminated strings */
    size_t nl = strlen((const char *)lhs);
    size_t nr = strlen((const char *)rhs);
    return strncmp((const char *)lhs, (const char *)rhs, nl > nr ? nl : nr);
}

static void profile_insert(const char *benchmark_id, size_t scale)
{
    char **words = NULL;
    char **new_words = NULL;
    char **shuffled = NULL;
    HAMT t;

    words_load_numbers(&words, 0, scale);
    /* insert 10% of scale words for profiling */
    size_t n_insert = scale * 0.1;
    words_load_numbers(&new_words, scale+1, n_insert);

    /* create HAMT */
    t = hamt_create(my_keyhash_string, my_keycmp_string,
                    &hamt_allocator_default);
    for (size_t i = 0; i < scale; i++) {
        hamt_set(t, words[i], words[i]);
    }
    /* shuffle input data */
    shuffled = words_create_shuffled_refs(new_words, scale);
    /* create profiling filename */
    char *filename;
    asprintf(&filename, "hamt-insert-transient-%s.prof", benchmark_id);
    ProfilerStart(filename);
    for (size_t i = 0; i < n_insert; i++) {
        hamt_set(t, new_words[i], new_words[i]);
    }
    ProfilerStop();
    free(filename);
    hamt_delete(t);
    words_free_refs(shuffled);

    /*
     * persistent
     */
    /* create new HAMT */
    t = hamt_create(my_keyhash_string, my_keycmp_string,
                    &hamt_allocator_gc);
    for (size_t i = 0; i < scale; i++) {
        hamt_set(t, words[i], words[i]);
    }
    /* shuffle input data */
    shuffled = words_create_shuffled_refs(new_words, scale);
    asprintf(&filename, "hamt-insert-persistent-%s.prof", benchmark_id);
    ProfilerStart(filename);
    for (size_t i = 0; i < n_insert; i++) {
        t = hamt_pset(t, new_words[i], new_words[i]);
    }
    ProfilerStop();
    free(filename);
    hamt_delete(t);
    words_free_refs(shuffled);

    words_free(new_words, n_insert);
    words_free(words, scale);
}

static void profile_query(const char *benchmark_id, size_t scale)
{
    char **words = NULL;
    HAMT t;

    words_load_numbers(&words, 0, scale);

    /* load table */
    t = hamt_create(my_keyhash_string, my_keycmp_string,
                    &hamt_allocator_default);
    for (size_t i = 0; i < scale; i++) {
        hamt_set(t, words[i], words[i]);
    }

    /* query the entire tree, 10 times */
    char **shuffled[10];
    for (size_t i = 0; i < 10; ++i) {
        shuffled[i] = words_create_shuffled_refs(words, scale);
    }
    char *filename;
    asprintf(&filename, "hamt-query-%s.prof", benchmark_id);
    ProfilerStart(filename);
    for (size_t i = 0; i < 10; ++i) {
        for (size_t i = 0; i < scale; i++) {
            hamt_get(t, words[i]);
        }
    }
    ProfilerStop();
    /* cleanup */
    for (size_t i = 0; i < 10; ++i) {
        words_free_refs(shuffled[i]);
    }
    hamt_delete(t);

    words_free(words, scale);
}

int main(int argc, char **argv)
{
    /* initialize garbage collection */
    GC_INIT();

    /* generate a benchmark id */
    uuid_t uuid;
    uuid_generate_random(uuid);
    char benchmark_id[37];
    uuid_unparse_lower(uuid, benchmark_id);

    /* get a timestamp */
    time_t now = time(0);

    /* run the performance measurements */
    srand(now);
    profile_insert(benchmark_id, 1e6);
    profile_query(benchmark_id, 1e6);
    printf("%s\n", benchmark_id);
    return 0;
}
