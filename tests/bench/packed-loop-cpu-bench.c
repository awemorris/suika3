/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Measure one already-JIT-compiled Noct function call. */

#define _POSIX_C_SOURCE 200809L

#include <noct/noct.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

static char *
read_file(const char *path)
{
	FILE *fp;
	long length;
	char *text;

	fp = fopen(path, "rb");
	if (fp == NULL || fseek(fp, 0, SEEK_END) != 0)
		return NULL;
	length = ftell(fp);
	if (length < 0 || fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return NULL;
	}
	text = malloc((size_t)length + 1);
	if (text == NULL || fread(text, 1, (size_t)length, fp) !=
	    (size_t)length) {
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
print_error(NoctEnv *env)
{
	const char *file = "?";
	const char *message = "?";
	int line = 0;

	(void)noct_get_error_file(env, &file);
	(void)noct_get_error_line(env, &line);
	(void)noct_get_error_message(env, &message);
	fprintf(stderr, "%s:%d: %s\n", file, line, message);
}

static bool
verify(NoctEnv *env, const char *name, NoctValue *ret)
{
	int value;

	return noct_enter_vm(env, name, 0, NULL, ret) &&
		noct_get_int(env, ret, &value) && value == 1;
}

int
main(int argc, char *argv[])
{
	NoctConfig config;
	NoctVM *vm = NULL;
	NoctEnv *env = NULL;
	NoctValue ret = {0};
	char *source = NULL;
	uint64_t *sample = NULL;
	uint64_t begin;
	uint64_t end;
	int samples;
	int i;
	int status = 1;

	if (argc != 6 || (samples = atoi(argv[5])) <= 0 ||
	    (samples & 1) == 0) {
		fprintf(stderr,
			"usage: %s source.noct setup run verify odd-samples\n",
			argv[0]);
		return 2;
	}
	source = read_file(argv[1]);
	sample = malloc((size_t)samples * sizeof(*sample));
	if (source == NULL || sample == NULL)
		goto cleanup;

	noct_set_default_config(&config);
	config.jit_enable = true;
	config.optimize_level = 2;
	if (!noct_create_vm(&vm, &env, &config) ||
	    !noct_pin_local(env, 1, &ret) ||
	    !noct_register_source(env, argv[1], source) ||
	    !noct_enter_vm(env, argv[2], 0, NULL, &ret) ||
	    !noct_enter_vm(env, argv[3], 0, NULL, &ret) ||
	    !verify(env, argv[4], &ret)) {
		if (env != NULL)
			print_error(env);
		goto cleanup;
	}

	for (i = 0; i < samples; i++) {
		begin = monotonic_ns();
		if (begin == 0 ||
		    !noct_enter_vm(env, argv[3], 0, NULL, &ret) ||
		    (end = monotonic_ns()) == 0 || end < begin ||
		    !verify(env, argv[4], &ret)) {
			print_error(env);
			goto cleanup;
		}
		sample[i] = end - begin;
	}
	qsort(sample, (size_t)samples, sizeof(*sample), compare_u64);
	printf("samples,best_ms,median_ms,worst_ms\n");
	printf("%d,%.6f,%.6f,%.6f\n", samples,
		(double)sample[0] / 1000000.0,
		(double)sample[samples / 2] / 1000000.0,
		(double)sample[samples - 1] / 1000000.0);
	status = 0;

cleanup:
	if (vm != NULL)
		(void)noct_destroy_vm(vm);
	free(sample);
	free(source);
	return status;
}
