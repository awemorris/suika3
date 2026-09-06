/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Measure DRAW_IMAGE_ALPHA across a fixed pixel count and one call boundary. */

#define _POSIX_C_SOURCE 200809L

#include <noct/noct.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#define MEASURE_WARMUPS 5

static char *
read_file(const char *path)
{
	FILE *fp;
	long length;
	char *text;

	fp = fopen(path, "rb");
	if (fp == NULL || fseek(fp, 0, SEEK_END) != 0) {
		if (fp != NULL)
			fclose(fp);
		return NULL;
	}
	length = ftell(fp);
	if (length < 0 || fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return NULL;
	}
	text = malloc((size_t)length + 1);
	if (text == NULL) {
		fclose(fp);
		return NULL;
	}
	if (fread(text, 1, (size_t)length, fp) != (size_t)length) {
		free(text);
		fclose(fp);
		return NULL;
	}
	text[length] = '\0';
	fclose(fp);
	return text;
}

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

static void
print_vm_error(NoctEnv *env)
{
	const char *file = "?";
	const char *message = "?";
	int line = 0;

	(void)noct_get_error_file(env, &file);
	(void)noct_get_error_line(env, &line);
	(void)noct_get_error_message(env, &message);
	fprintf(stderr, "%s:%d: %s\n", file, line, message);
}

int
main(int argc, char *argv[])
{
	NoctConfig config;
	NoctVM *vm;
	NoctEnv *env;
	NoctValue func_value;
	NoctValue arg[4];
	NoctValue ret;
	NoctFunc *func;
	char *source;
	uint32_t *dst;
	uint32_t *initial_dst;
	uint32_t *src;
	uint64_t *elapsed;
	uint64_t begin;
	uint64_t end;
	uint64_t median2;
	size_t i;
	int level;
	int samples;
	int iterations;
	int sample;
	int result = 1;
	size_t pixels;

	if (argc != 5) {
		fprintf(stderr, "usage: %s LEVEL SAMPLES PIXELS blend-alpha.noct\n",
			argv[0]);
		return 2;
	}
	level = atoi(argv[1]);
	samples = atoi(argv[2]);
	iterations = atoi(argv[3]);
	if ((level != 0 && level != 2 && level != 3) ||
	    samples <= 0 || iterations <= 0)
		return 2;
	pixels = (size_t)iterations;
	memset(&func_value, 0, sizeof(func_value));
	memset(arg, 0, sizeof(arg));
	memset(&ret, 0, sizeof(ret));

	source = read_file(argv[4]);
	dst = malloc(pixels * sizeof(*dst));
	initial_dst = malloc(pixels * sizeof(*initial_dst));
	src = malloc(pixels * sizeof(*src));
	elapsed = malloc((size_t)samples * sizeof(*elapsed));
	if (source == NULL || dst == NULL || initial_dst == NULL || src == NULL ||
	    elapsed == NULL) {
		fprintf(stderr, "allocation failed\n");
		goto cleanup_buffers;
	}
	for (i = 0; i < pixels; i++) {
		uint32_t x = (uint32_t)i;
		initial_dst[i] = UINT32_C(0xff000000) |
			(((x * 17u) & 255u) << 16) |
			(((x * 29u) & 255u) << 8) |
			((x * 43u) & 255u);
		src[i] = (((x * 7u) & 255u) << 24) |
			(((x * 11u) & 255u) << 16) |
			(((x * 13u) & 255u) << 8) |
			((x * 19u) & 255u);
	}
	memcpy(dst, initial_dst, pixels * sizeof(*dst));

	noct_set_default_config(&config);
	config.jit_enable = true;
	config.optimize_level = level;
	config.lineinfo = level == 0;
	if (!noct_create_vm(&vm, &env, &config)) {
		fprintf(stderr, "noct_create_vm failed\n");
		goto cleanup_buffers;
	}
	if (!noct_pin_local(env, 6, &func_value, &arg[0], &arg[1], &arg[2],
			    &arg[3], &ret) ||
	    !noct_register_source(env, argv[4], source) ||
	    !noct_get_global(env, "blend_alpha", &func_value) ||
	    !noct_get_func(env, &func_value, &func) ||
	    !noct_make_packed(env, &arg[0], NOCT_PACKED_UINT32,
			      pixels * sizeof(*dst), pixels, dst, NULL, NULL) ||
	    !noct_make_packed(env, &arg[1], NOCT_PACKED_UINT32,
			      pixels * sizeof(*src), pixels, src, NULL, NULL) ||
	    !noct_make_int(env, &arg[2], 128) ||
	    !noct_make_int(env, &arg[3], 1) ||
	    !noct_call(env, func, 4, arg, &ret)) {
		print_vm_error(env);
		goto cleanup_vm;
	}

	/* JIT build/commit and one execution are complete before timing. */
	if (!noct_make_int(env, &arg[3], iterations))
		goto cleanup_vm;
	for (sample = 0; sample < MEASURE_WARMUPS; sample++) {
		memcpy(dst, initial_dst, pixels * sizeof(*dst));
		if (!noct_call(env, func, 4, arg, &ret)) {
			print_vm_error(env);
			goto cleanup_vm;
		}
	}
	for (sample = 0; sample < samples; sample++) {
		memcpy(dst, initial_dst, pixels * sizeof(*dst));
		begin = monotonic_ns();
		if (begin == 0 || !noct_call(env, func, 4, arg, &ret)) {
			print_vm_error(env);
			goto cleanup_vm;
		}
		end = monotonic_ns();
		if (end == 0)
			goto cleanup_vm;
		elapsed[sample] = end - begin;
		fprintf(stderr, "sample,%d,%.3f\n", sample + 1,
			(double)elapsed[sample] / 1000000.0);
	}

	qsort(elapsed, (size_t)samples, sizeof(*elapsed), compare_u64);
	if ((samples & 1) != 0)
		median2 = elapsed[samples / 2] * 2;
	else
		median2 = elapsed[samples / 2 - 1] + elapsed[samples / 2];
	printf("optimize_level,samples,iterations,best_ms,median_ms,checksum\n");
	printf("%d,%d,%d,%.6f,%.6f,%" PRIu32 "\n", level, samples, iterations,
		(double)elapsed[0] / 1000000.0,
		(double)median2 / 2000000.0,
		dst[0] ^ dst[pixels - 1]);
	result = 0;

cleanup_vm:
	(void)noct_destroy_vm(vm);
cleanup_buffers:
	free(elapsed);
	free(src);
	free(initial_dst);
	free(dst);
	free(source);
	return result;
}
