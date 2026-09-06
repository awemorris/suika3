/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* C reference for bench/multi-doall.noct. */

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#define ELEMENTS 4194304

static uint64_t
monotonic_ns(void)
{
#if defined(__APPLE__)
	static mach_timebase_info_data_t timebase;
	uint64_t ticks;

	if (timebase.denom == 0 && mach_timebase_info(&timebase) != KERN_SUCCESS)
		return 0;
	ticks = mach_absolute_time();
	return ticks * (uint64_t)timebase.numer / (uint64_t)timebase.denom;
#else
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return 0;
	return (uint64_t)ts.tv_sec * UINT64_C(1000000000) +
		(uint64_t)ts.tv_nsec;
#endif
}

static int
compare_u64(const void *a, const void *b)
{
	uint64_t av = *(const uint64_t *)a;
	uint64_t bv = *(const uint64_t *)b;
	return av < bv ? -1 : av != bv;
}

#define DO_STAGE(source, destination, xor_value, add_value)             \
	for (i = 0; i < count; i++) {                                    \
		int32_t in = input[i];                                      \
		int32_t divisor = (in & 7) + 1;                             \
		destination[i] = ((((source[i] ^ (xor_value)) + in) * 3) /  \
			divisor + (add_value)) & 16777215;                     \
	}

__attribute__((noinline)) static void
multi_doall(const int32_t *restrict input, int32_t *restrict tmp1,
	    int32_t *restrict tmp2, int32_t *restrict output, int count)
{
	int i;

	DO_STAGE(input, tmp1, 90, 1);
	DO_STAGE(tmp1, tmp2, 91, 2);
	DO_STAGE(tmp2, tmp1, 92, 3);
	DO_STAGE(tmp1, tmp2, 93, 4);
	DO_STAGE(tmp2, tmp1, 94, 5);
	DO_STAGE(tmp1, tmp2, 95, 6);
	DO_STAGE(tmp2, tmp1, 96, 7);
	DO_STAGE(tmp1, output, 97, 8);
}

int
main(int argc, char *argv[])
{
	int32_t *input;
	int32_t *tmp1;
	int32_t *tmp2;
	int32_t *output;
	uint64_t *sample;
	uint64_t begin;
	uint64_t end;
	int samples = argc == 2 ? atoi(argv[1]) : 51;
	int i;

	if (samples <= 0 || (samples & 1) == 0)
		return 2;
	input = malloc((size_t)ELEMENTS * sizeof(*input));
	tmp1 = malloc((size_t)ELEMENTS * sizeof(*tmp1));
	tmp2 = malloc((size_t)ELEMENTS * sizeof(*tmp2));
	output = malloc((size_t)ELEMENTS * sizeof(*output));
	sample = malloc((size_t)samples * sizeof(*sample));
	if (input == NULL || tmp1 == NULL || tmp2 == NULL ||
	    output == NULL || sample == NULL)
		return 1;
	for (i = 0; i < ELEMENTS; i++)
		input[i] = i;
	multi_doall(input, tmp1, tmp2, output, ELEMENTS);
	if (output[0] != 670832 || output[2097152] != 2767984 ||
	    output[4194303] != 2517264)
		return 1;
	for (i = 0; i < samples; i++) {
		begin = monotonic_ns();
		multi_doall(input, tmp1, tmp2, output, ELEMENTS);
		end = monotonic_ns();
		if (begin == 0 || end < begin || output[0] != 670832)
			return 1;
		sample[i] = end - begin;
	}
	qsort(sample, (size_t)samples, sizeof(*sample), compare_u64);
	printf("samples,best_ms,median_ms,worst_ms\n");
	printf("%d,%.6f,%.6f,%.6f\n", samples,
		(double)sample[0] / 1000000.0,
		(double)sample[samples / 2] / 1000000.0,
		(double)sample[samples - 1] / 1000000.0);
	free(sample);
	free(output);
	free(tmp2);
	free(tmp1);
	free(input);
	return 0;
}
