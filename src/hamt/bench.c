#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <uuid/uuid.h>

#include "../../lib/hamt/include/hamt.h"
#include "../../lib/hamt/include/murmur3.h"
#include "../../lib/hamt/include/uh.h"
#include "../numbers.h"
#include "../utils.h"

#include "gc.h"
#define USE_DL_PREFIX
#include "../../lib/dlmalloc/malloc.h"

struct benchmark_config;
typedef void (*hamt_bench_fn)(const struct benchmark_config *cfg);
struct benchmark_config {
    char id[37];
    time_t ts;
    size_t scale;
    size_t n_repeats;
    struct hamt_allocator *allocator;
    hamt_key_hash_fn hash_fn;
    char *hash_fn_name;
    hamt_bench_fn bench_fn;
    char *bench_fn_name;
};

int *shuffle(int *array, size_t n)
{
    if (n > 1) {
        for (size_t i = 0; i < n - 1; ++i) {
            size_t j = drand48() * (i + 1);
            int tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
        }
    }
    return array;
}

static void *bdwgc_malloc(const ptrdiff_t size, void *ctx)
{
    (void)ctx;
    return GC_malloc(size);
}

static void *bdwgc_realloc(void *ptr, const ptrdiff_t old_size,
                           const ptrdiff_t new_size, void *ctx)
{
    (void)ctx;
    (void)old_size;
    return GC_realloc(ptr, new_size);
}

void bdwgc_free(void *ptr, const ptrdiff_t size, void *ctx)
{
    (void)ptr;
    (void)size;
    (void)ctx;
}

struct hamt_allocator hamt_allocator_bdwgc = {bdwgc_malloc, bdwgc_realloc,
                                              bdwgc_free};

static void *dlmalloc_malloc(const ptrdiff_t size, void *ctx)
{
    (void)ctx;
    return dlmalloc(size);
}

static void *dlmalloc_realloc(void *ptr, const ptrdiff_t old_size,
                              const ptrdiff_t new_size, void *ctx)
{
    (void)ctx;
    (void)old_size;
    return dlrealloc(ptr, new_size);
}

void dlmalloc_free(void *ptr, const ptrdiff_t size, void *ctx)
{
    (void)ctx;
    (void)size;
    dlfree(ptr);
}

struct hamt_allocator hamt_allocator_dlmalloc = {
    dlmalloc_malloc, dlmalloc_realloc, dlmalloc_free};

static uint32_t my_keyhash_int(const void *key, const size_t gen)
{
    uint32_t hash = murmur3_32((uint8_t *)key, sizeof(int), gen);
    return hash;
}

static int my_keycmp_int(const void *lhs, const void *rhs)
{
    /* expects lhs and rhs to be pointers to 0-terminated strings */
    const int *l = (const int *)lhs;
    const int *r = (const int *)rhs;

    if (*l > *r)
        return 1;
    return *l == *r ? 0 : -1;
}

static void perf_insert(const struct benchmark_config *cfg)
{
    struct hamt *t;

    int *numbers = make_numbers(cfg->scale, 0);

    /* insert 1% of scale words for test */
    int n_insert = 0.1 * cfg->scale;
    int *new_numbers = make_numbers(n_insert, cfg->scale);

    struct TimeInterval ti_insert;
    for (size_t i = 0; i < cfg->n_repeats; ++i) {
        /* create new struct hamt **/
        t = hamt_create(my_keyhash_int, my_keycmp_int, &hamt_allocator_default);
        for (size_t i = 0; i < cfg->scale; i++) {
            hamt_set(t, &numbers[i], &numbers[i]);
        }
        /* shuffle input data */
        shuffle_numbers(new_numbers, n_insert);

        /* insert */
        timer_start(&ti_insert);
        for (size_t i = 0; i < n_insert; i++) {
            hamt_set(t, &new_numbers[i], &new_numbers[i]);
        }
        timer_stop(&ti_insert);
        hamt_delete(t);
        double ns_per_insert = timer_nsec(&ti_insert) / (double)n_insert;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", cfg->ts, cfg->id, i, "insert",
               cfg->scale, ns_per_insert);
    }
    free(new_numbers);
    free(numbers);
}

static void perf_query(const struct benchmark_config *cfg)
{
    struct hamt *t;

    int *numbers = make_numbers(cfg->scale, 0);
    int *query_numbers = make_numbers(cfg->scale, 0);

    /* create and load table */
    t = hamt_create(my_keyhash_int, my_keycmp_int, &hamt_allocator_default);
    for (size_t i = 0; i < cfg->scale; i++) {
        hamt_set(t, &numbers[i], &numbers[i]);
    }

    struct TimeInterval ti_query;
    double ns_per_query;
    /* make a copy so we don't modify the data underlying
     * the struct hamt **/
    for (size_t i = 0; i < cfg->n_repeats; ++i) {
        /* shuffle query keys */
        shuffle_numbers(query_numbers, cfg->scale);
        timer_start(&ti_query);
        for (size_t i = 0; i < cfg->scale; i++) {
            hamt_get(t, &query_numbers[i]);
        }
        timer_stop(&ti_query);
        ns_per_query = timer_nsec(&ti_query) / (double)cfg->scale;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", cfg->ts, cfg->id, i, "query",
               cfg->scale, ns_per_query);
    }
    /* cleanup */
    hamt_delete(t);
    free(numbers);
    free(query_numbers);
}

static void perf_remove(const struct benchmark_config *cfg)
{
    struct hamt *t;

    int *numbers = make_numbers(cfg->scale, 0);
    int *rem_numbers = make_numbers(cfg->scale, 0);

    /* remove 1% of scale words for test */
    size_t n_remove = cfg->scale * 0.01;

    struct TimeInterval ti_remove;
    for (size_t i = 0; i < cfg->n_repeats; ++i) {
        /* create new struct hamt **/
        t = hamt_create(my_keyhash_int, my_keycmp_int, &hamt_allocator_default);
        for (size_t i = 0; i < cfg->scale; i++) {
            hamt_set(t, &numbers[i], &numbers[i]);
        }

        /* shuffle all query keys */
        shuffle_numbers(rem_numbers, cfg->scale);

        timer_start(&ti_remove);
        /* delete the first n_remove entries */
        for (size_t i = 0; i < n_remove; i++) {
            hamt_remove(t, &rem_numbers[i]);
        }
        timer_stop(&ti_remove);
        hamt_delete(t);
        double ns_per_remove = timer_nsec(&ti_remove) / (double)n_remove;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", cfg->ts, cfg->id, i, "remove",
               cfg->scale, ns_per_remove);
    }
    free(rem_numbers);
    free(numbers);
}

static void perf_persistent_insert(const struct benchmark_config *cfg)
{

    int *numbers = make_numbers(cfg->scale, 0);

    /* insert 1% of cfg->scale words for test */
    int n_insert = 0.1 * cfg->scale;
    int *new_numbers = make_numbers(n_insert, cfg->scale);

    struct TimeInterval ti_insert;
    for (size_t i = 0; i < cfg->n_repeats; ++i) {
        /* create new struct hamt **/
        struct hamt *t =
            hamt_create(my_keyhash_int, my_keycmp_int, &hamt_allocator_default);
        for (size_t i = 0; i < cfg->scale; i++) {
            hamt_set(t, &numbers[i], &numbers[i]);
        }
        /* shuffle input data */
        shuffle_numbers(new_numbers, n_insert);

        /* insert */
        const struct hamt *ct = t;
        timer_start(&ti_insert);
        for (size_t i = 0; i < n_insert; i++) {
            ct = hamt_pset(ct, &new_numbers[i], &new_numbers[i]);
        }
        timer_stop(&ti_insert);
        hamt_delete(t);
        double ns_per_insert = timer_nsec(&ti_insert) / (double)n_insert;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", cfg->ts, cfg->id, i,
               "persistent_insert", cfg->scale, ns_per_insert);
    }
    free(new_numbers);
    free(numbers);
}

static void perf_persistent_remove(const struct benchmark_config *cfg)
{
    struct hamt *t;

    int *numbers = make_numbers(cfg->scale, 0);
    int *rem_numbers = make_numbers(cfg->scale, 0);

    /* remove 1% of cfg->scale words for test */
    size_t n_remove = cfg->scale * 0.01;

    struct TimeInterval ti_remove;
    for (size_t i = 0; i < cfg->n_repeats; ++i) {
        /* create new struct hamt **/
        t = hamt_create(my_keyhash_int, my_keycmp_int, &hamt_allocator_default);
        for (size_t i = 0; i < cfg->scale; i++) {
            hamt_set(t, &numbers[i], &numbers[i]);
        }

        /* shuffle all query keys */
        shuffle_numbers(rem_numbers, cfg->scale);

        timer_start(&ti_remove);
        /* delete the first n_remove entries */
        const struct hamt *ct = t;
        for (size_t i = 0; i < n_remove; i++) {
            ct = hamt_premove(ct, &rem_numbers[i]);
        }
        timer_stop(&ti_remove);
        hamt_delete(t);
        double ns_per_remove = timer_nsec(&ti_remove) / (double)n_remove;
        printf("%ld,\"%s\",%lu,\"%s\",%lu,%f\n", cfg->ts, cfg->id, i,
               "persistent_remove", cfg->scale, ns_per_remove);
    }
    free(rem_numbers);
    free(numbers);
}

static uint32_t keyhash_murmur3(const void *key, const size_t gen)
{
    return murmur3_32((uint8_t *)key, strlen((const char *)key), gen);
}

static uint32_t keyhash_universal(const void *key, const size_t gen)
{
    return sedgewick_universal_hash((const char *)key, 0x8fffffff - (gen << 8));
}

struct ProgressBar {
    int count;
    int total;
};

char *progress_fmtstrs[] = {"%5.2f%% |%.*s\r", "%5.2f%% |%.*s|\n"};

void progress_print(const struct ProgressBar *pb)
{
    int width = 40;
    float p = (float)pb->count / (float)pb->total;
    int k = (int)(p * width);
    char *fmtstr = progress_fmtstrs[pb->count == pb->total ? 1 : 0];
    fprintf(stderr, "                                                 |\r");
    fprintf(stderr, fmtstr, 100 * p, k,
            "========================================");
    fflush(stderr);
}

void progress_advance(struct ProgressBar *pb, int count)
{
    pb->count += count;
    if (pb->count > pb->total)
        pb->count = pb->total;
}

void usage(const char *message)
{
    if (message)
        printf("%s\n\n", message);
    printf("usage: bench [-h] [-a <malloc|dlmalloc|bdwgc>] [-d]"
           " [-H <murmur3|uh>] [-q] [-w <n_warmup_runs>]\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    /* set up the experiment */
    struct benchmark_config base_cfg;

    size_t n_bench_fn = 5;
    hamt_bench_fn bench_fns[] = {perf_query, perf_insert, perf_remove,
                                 perf_persistent_insert,
                                 perf_persistent_remove};
    char *bench_fn_names[] = {"query", "insert", "remove", "pinsert",
                              "premove"};
    char *hash_fn_names[] = {"murmur3", "uh"};
    // defaults
    base_cfg.allocator = &hamt_allocator_default;
    base_cfg.hash_fn = keyhash_murmur3;
    bool dry_run = false;
    int warmup_runs = 0;
    bool quiet = false;
    char c;
    if (argc == 1) {
        usage(NULL);
    }
    while ((c = getopt(argc, argv, "a:dhH:qw:")) != -1) {
        switch (c) {
        case 'a':
            if (strncmp(optarg, "malloc", 6) == 0) {
                base_cfg.allocator = &hamt_allocator_default;
            } else if (strncmp(optarg, "dlmalloc", 8) == 0) {
                base_cfg.allocator = &hamt_allocator_dlmalloc;
            } else if (strncmp(optarg, "bdwgc", 5) == 0) {
                base_cfg.allocator = &hamt_allocator_bdwgc;
            } else {
                printf("Unknown parameter %s to -a.\n", optarg);
                usage(NULL);
            }
            break;
        case 'd':
            dry_run = true;
            break;
        case 'H':
            if (strncmp(optarg, "murmur3", 7) == 0) {
                base_cfg.hash_fn = keyhash_murmur3;
                base_cfg.hash_fn_name = hash_fn_names[0];
            } else if (strncmp(optarg, "uh", 2) == 0) {
                base_cfg.hash_fn = keyhash_universal;
                base_cfg.hash_fn_name = hash_fn_names[1];
            } else {
                printf("Unknown parameter %s to -a.\n", optarg);
                usage(NULL);
            }
            break;
        case 'h':
            usage(NULL);
            break;
        case 'q':
            quiet = true;
            break;
        case 'w':
            warmup_runs = atoi(optarg);
            break;
        case '?':
            printf("Unknown option\n");
            usage(NULL);
        }
    }

    /* generate a benchmark id */
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, base_cfg.id);
    /* get a timestamp */
    base_cfg.ts = time(0);

    /* initialize subsystems: garbage collection, RNG */
    if (base_cfg.allocator == &hamt_allocator_bdwgc) {
        GC_INIT();
    }
    srand(base_cfg.ts);

    size_t scale[] = {1e3, 1e4, 1e5, 1e6};
    size_t n_scales = 4;
    base_cfg.n_repeats = 20;

    /* create all configurations */
    const size_t MAX_CONFIGS = 1024; // hardcode for now
    struct benchmark_config cfgs[MAX_CONFIGS] = {0};
    size_t j = 0;
    for (size_t i = 0; i < n_scales; ++i) {
        base_cfg.scale = scale[i];
        for (size_t k = 0; k < n_bench_fn; k++) {
            base_cfg.bench_fn = bench_fns[k];
            base_cfg.bench_fn_name = bench_fn_names[k];
            cfgs[j++] = base_cfg;
        }
    }

    int *indexes = malloc(sizeof(int) * j);
    for (size_t k = 0; k < j; ++k)
        indexes[k] = k;

    /* create a randomized order */
    indexes = shuffle(indexes, j);

    if (dry_run) {
        printf("-=[ Experiments ]=-\n");
        printf("Warmup runs: %i\n", warmup_runs);
        for (size_t i = 0; i < j; i++) {
            printf("%3i %s\t%s\t%8lu %3lu\n", indexes[i], cfgs[i].bench_fn_name,
                   cfgs[i].hash_fn_name, cfgs[i].scale, cfgs[i].n_repeats);
        }
        goto exit;
    }
    /* run warmup cycles */
    if (warmup_runs) {
        for (size_t i = 0; i < warmup_runs; i++) {
            int k = indexes[i];
            cfgs[k].bench_fn(&cfgs[k]);
        }
    }
    /* run the performance measurements in randomized order */
    struct ProgressBar pb = {.count = 0, .total = j};
    for (size_t i = 0; i < j; i++) {
        if (!quiet)
            progress_print(&pb);
        int k = indexes[i];
        cfgs[k].bench_fn(&cfgs[k]);
        if (!quiet)
            progress_advance(&pb, 1);
    }
    if (!quiet)
        progress_print(&pb);
exit:
    free(indexes);
    return 0;
}
