#include "common.h"

#include <ctype.h>
#include <error.h>
#include <stdio.h>

unsigned long long bench_clock_add(struct timespec *before, struct timespec *after,
		struct bench_clock *bench_clock)
{
	unsigned long long diff = (after->tv_sec * 1000000000 + after->tv_nsec)
		- (before->tv_sec * 1000000000 + before->tv_nsec);

	bench_clock->total += diff;
	
	if (bench_clock->runs == 0 || bench_clock->min > diff)
		bench_clock->min = diff;
	
	if (bench_clock->runs == 0 || bench_clock->max < diff)
		bench_clock->max = diff;

	bench_clock->runs++;

	return diff;
}

void bench_clock_print(struct bench_clock *bench_clock)
{
	bench_block_printf(bench_clock, "Runs: %R\nTotal: %T\nMin: %N\nAvg: %A\nMax: %X\n");
}

void bench_block_printf(struct bench_clock *bench_clock, const char *fmt)
{
	if (bench_clock->runs == 0)
		return;
	while (*fmt != '\0') {
		if (*fmt == '%') {
			fmt++;
			const char *num_fmt = isupper(*fmt) ? "%'llu" : "%llu";
			switch (tolower(*fmt)) {
			case '%':
				putchar('%');
				break;
			case 'r':
				printf(num_fmt, bench_clock->runs);
				break;
			case 't':
				printf(num_fmt, bench_clock->total);
				break;
			case 'n':
				printf(num_fmt, bench_clock->min);
				break;
			case 'a':
				printf(num_fmt, bench_clock->total / bench_clock->runs);
				break;
			case 'x':
				printf(num_fmt, bench_clock->max);
				break;
			default:
				error(0, 0, "Unknown format mark '%%%c'", *fmt);
				break;
			}
		} else if (*fmt == '\\') {
			fmt++;
			switch (*fmt) {
			case '\\':
			case '\'':
			case '\"':
			case '\?':
				putchar(*fmt);
				break;
			case 'a':
				putchar('\a');
				break;
			case 'b':
				putchar('\b');
				break;
			case 'f':
				putchar('\f');
				break;
			case 'n':
				putchar('\n');
				break;
			case 'r':
				putchar('\r');
				break;
			case 't':
				putchar('\t');
				break;
			case 'v':
				putchar('\v');
				break;
			}
		} else {
			putchar(*fmt);
		}
		fmt++;
	}
}
