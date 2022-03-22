#ifndef HAMT_BENCH_UTILS
#define HAMT_BENCH_UTILS

#include <stdio.h>
#include <time.h>

struct TimeInterval {
    struct timespec begin, end;
    time_t sec;
    long nsec;
};


void timer_start(struct TimeInterval *ti);
void timer_stop(struct TimeInterval *ti);
void timer_continue(struct TimeInterval *ti);
long timer_nsec(struct TimeInterval *ti);
void print_timer(struct TimeInterval *ti, const time_t timestamp, const char *benchmark_id, size_t ix, const char *tag);

#endif
