#include <stdio.h>
#include <time.h>

#include "utils.h"

void timer_start(struct TimeInterval *ti)
{
    ti->sec = 0;
    ti->nsec = 0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ti->begin);
}

void timer_stop(struct TimeInterval *ti)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &ti->end);
    ti->sec += ti->end.tv_sec - ti->begin.tv_sec;
    ti->nsec += ti->end.tv_nsec - ti->begin.tv_nsec;
}

void timer_continue(struct TimeInterval *ti)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &ti->begin);
}

long timer_nsec(struct TimeInterval *ti)
{
    return ti->sec * 1000000000L + ti->nsec;
}

void print_timer(struct TimeInterval *ti, const time_t timestamp,
                 const char *benchmark_id, size_t ix, const char *tag)
{
    printf("%ld, %s, %lu, %s,%lu\n", timestamp, benchmark_id, ix, tag,
           ti->sec * 1000000000L + ti->nsec);
}
