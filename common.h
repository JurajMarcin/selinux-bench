#ifndef COMMON_H
#define COMMON_H

#include <time.h>

struct bench_clock {
	unsigned long long runs;
	unsigned long long total;
	unsigned long long min;
	unsigned long long max;
};

unsigned long long bench_clock_add(struct timespec *before, struct timespec *after,
		struct bench_clock *bench_clock);

void bench_clock_print(struct bench_clock *bench_clock);

void bench_block_printf(struct bench_clock *bench_clock, const char *fmt);

#endif // !COMMON_H
