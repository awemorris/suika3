/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Compare ordinary func/JIT and raw __gpu func CNN forward steady-state time. */

#define _POSIX_C_SOURCE 200809L

#include <noct/noct.h>

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#define CALIBRATION_NS UINT64_C(500000000)

static bool single_invocation;

struct result {
	const char *mode;
	int iterations;
	int samples;
	uint64_t best;
	uint64_t median;
	uint64_t worst;
};

static char *
read_file(const char *path)
{
	FILE *fp;
	long length;
	char *text;

	fp = fopen(path, "rb");
	if (fp == NULL || fseek(fp, 0, SEEK_END) != 0) {
		if (fp != NULL) fclose(fp);
		return NULL;
	}
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

static bool
call_iterations(NoctEnv *env, const char *name, int iterations,
		NoctValue *arg, NoctValue *ret)
{
	if (single_invocation)
		return noct_enter_vm(env, name, 0, NULL, ret);
	return noct_make_int(env, arg, iterations) &&
		noct_enter_vm(env, name, 1, arg, ret);
}

static bool
measure(NoctEnv *env, const char *name, int iterations, NoctValue *arg,
	NoctValue *ret, uint64_t *elapsed)
{
	uint64_t begin;
	uint64_t end;

	begin = monotonic_ns();
	if (begin == 0 || !call_iterations(env, name, iterations, arg, ret))
		return false;
	end = monotonic_ns();
	if (end == 0 || end < begin)
		return false;
	*elapsed = end - begin;
	return true;
}

static int
scaled_iterations(int iterations, uint64_t desired, uint64_t elapsed)
{
	long double estimate;

	if (elapsed == 0)
		return 1;
	estimate = (long double)iterations * (long double)desired /
		(long double)elapsed;
	if (estimate < 1.0L)
		return 1;
	if (estimate > (long double)INT_MAX)
		return INT_MAX;
	return (int)(estimate + 0.5L);
}

static bool
calibrate(NoctEnv *env, const char *name, int initial, uint64_t target_ns,
	  NoctValue *arg, NoctValue *ret, int *iterations)
{
	int current = initial;
	uint64_t elapsed;

	for (;;) {
		if (!measure(env, name, current, arg, ret, &elapsed))
			return false;
		fprintf(stderr, "calibrate,%s,%d,%.3f ms\n", name, current,
			(double)elapsed / 1000000.0);
		if (elapsed >= CALIBRATION_NS || current == INT_MAX) {
			*iterations = scaled_iterations(current, target_ns, elapsed);
			return true;
		}
		if (current > INT_MAX / 8)
			current = INT_MAX;
		else
			current *= 8;
	}
}

static bool
verify(NoctEnv *env, const char *name, NoctValue *ret)
{
	int valid;
	if (!noct_enter_vm(env, name, 0, NULL, ret) ||
	    !noct_get_int(env, ret, &valid))
		return false;
	if (valid != 1) {
		fprintf(stderr, "%s verification failed\n", name);
		return false;
	}
	return true;
}

static bool
run_mode(NoctEnv *env, const char *mode, const char *run_name,
	 const char *verify_name, int initial, uint64_t target_ns,
	 uint64_t warmup_ns, int samples, NoctValue *arg, NoctValue *ret,
	 struct result *result)
{
	uint64_t *elapsed;
	uint64_t warm_elapsed;
	int iterations;
	int warm_iterations;
	int i;

	if (single_invocation) {
		elapsed = malloc((size_t)samples * sizeof(*elapsed));
		if (elapsed == NULL)
			return false;
		for (i = 0; i < samples; i++) {
			if (!measure(env, run_name, 1, arg, ret, &elapsed[i]) ||
			    !verify(env, verify_name, ret)) {
				free(elapsed);
				return false;
			}
			fprintf(stderr, "sample,%s,%d/%d,one-call,%.3f s\n",
				mode, i + 1, samples,
				(double)elapsed[i] / 1000000000.0);
		}
		qsort(elapsed, (size_t)samples, sizeof(*elapsed), compare_u64);
		result->mode = mode;
		result->iterations = 1;
		result->samples = samples;
		result->best = elapsed[0];
		result->median = elapsed[samples / 2];
		result->worst = elapsed[samples - 1];
		free(elapsed);
		return true;
	}

	if (!calibrate(env, run_name, initial, target_ns, arg, ret, &iterations))
		return false;
	warm_iterations = scaled_iterations(iterations, warmup_ns, target_ns);
	fprintf(stderr, "warmup,%s,%d,target %.1f s\n", mode,
		warm_iterations, (double)warmup_ns / 1000000000.0);
	if (!measure(env, run_name, warm_iterations, arg, ret, &warm_elapsed) ||
	    !verify(env, verify_name, ret))
		return false;
	fprintf(stderr, "warmup-complete,%s,%.3f s\n", mode,
		(double)warm_elapsed / 1000000000.0);
	/* Recalculate at the post-warmup clock/thermal state. */
	iterations = scaled_iterations(warm_iterations, target_ns, warm_elapsed);
	elapsed = malloc((size_t)samples * sizeof(*elapsed));
	if (elapsed == NULL)
		return false;
	for (i = 0; i < samples; i++) {
		if (!measure(env, run_name, iterations, arg, ret, &elapsed[i]) ||
		    !verify(env, verify_name, ret)) {
			free(elapsed);
			return false;
		}
		fprintf(stderr, "sample,%s,%d/%d,%d,%.3f s\n", mode,
			i + 1, samples, iterations,
			(double)elapsed[i] / 1000000000.0);
	}
	qsort(elapsed, (size_t)samples, sizeof(*elapsed), compare_u64);
	result->mode = mode;
	result->iterations = iterations;
	result->samples = samples;
	result->best = elapsed[0];
	result->median = elapsed[samples / 2];
	result->worst = elapsed[samples - 1];
	free(elapsed);
	return true;
}

static void
print_result(const struct result *r)
{
	double ns_per_forward = (double)r->median / (double)r->iterations;
	printf("%s,%d,%d,%.6f,%.6f,%.6f,%.3f,%.3f\n", r->mode,
		r->iterations, r->samples, (double)r->best / 1000000.0,
		(double)r->median / 1000000.0,
		(double)r->worst / 1000000.0, ns_per_forward,
		1000000000.0 / ns_per_forward);
}

int
main(int argc, char *argv[])
{
	NoctConfig config;
	NoctVM *vm = NULL;
	NoctEnv *env = NULL;
	NoctValue arg = {0};
	NoctValue ret = {0};
	struct result cpu;
	struct result gpu;
	char *source;
	double target_seconds;
	double warmup_seconds;
	uint64_t target_ns;
	uint64_t warmup_ns;
	int samples;
	const char *backend;
	const char *gpu_mode;
	bool skip_cpu;
	int status = 1;

	if (argc != 10) {
		fprintf(stderr, "usage: %s TARGET_SECONDS WARMUP_SECONDS SAMPLES source.noct setup cpu-forward cpu-verify gpu-forward gpu-verify\n",
			argv[0]);
		return 2;
	}
	target_seconds = strtod(argv[1], NULL);
	warmup_seconds = strtod(argv[2], NULL);
	samples = atoi(argv[3]);
	if (target_seconds <= 0.0 || warmup_seconds <= 0.0 || samples <= 0 ||
	    (samples & 1) == 0)
		return 2;
	target_ns = (uint64_t)(target_seconds * 1000000000.0);
	warmup_ns = (uint64_t)(warmup_seconds * 1000000000.0);
	single_invocation = getenv("NOCT_BENCH_SINGLE_INVOCATION") != NULL;
	skip_cpu = getenv("NOCT_BENCH_SKIP_CPU") != NULL;
	backend = getenv("NOCT_BENCH_ACCEL_BACKEND");
	if (backend != NULL && strcmp(backend, "vulkan") == 0) {
		gpu_mode = "gpu-vulkan";
	} else {
		gpu_mode = "gpu-opengl-es";
	}
	source = read_file(argv[4]);
	if (source == NULL) {
		fprintf(stderr, "cannot read %s\n", argv[4]);
		return 1;
	}

	noct_set_default_config(&config);
	{
		const char *model = getenv("NOCT_BENCH_OBJECT_MODEL");
		if (model != NULL) {
			if (strcmp(model, "0") == 0)
				config.object_model = NOCT_OBJECT_MODEL_SINGLE;
			else if (strcmp(model, "1") == 0)
				config.object_model = NOCT_OBJECT_MODEL_MULTI;
			else {
				fprintf(stderr,
					"NOCT_BENCH_OBJECT_MODEL must be 0 or 1\n");
				free(source);
				return 2;
			}
		}
	}
	config.jit_enable = true;
	config.optimize_level = 2;
	config.accel_enable = true;
	config.accel_backend = backend != NULL && strcmp(backend, "vulkan") == 0 ?
		NOCT_ACCEL_BACKEND_VULKAN : NOCT_ACCEL_BACKEND_OPENGL;
	if (!noct_create_vm(&vm, &env, &config) ||
	    !noct_pin_local(env, 2, &arg, &ret) ||
	    !noct_register_source(env, argv[4], source) ||
	    !noct_enter_vm(env, argv[5], 0, NULL, &ret)) {
		if (env != NULL) print_vm_error(env);
		goto cleanup;
	}

	/* First calls finish JIT and accelerator pipeline compilation. */
	if ((!skip_cpu &&
	     (!call_iterations(env, argv[6], 1, &arg, &ret) ||
	      !verify(env, argv[7], &ret))) ||
	    !call_iterations(env, argv[8], 1, &arg, &ret) ||
	    !verify(env, argv[9], &ret) ||
	    (!skip_cpu && !run_mode(env, "cpu-jit", argv[6], argv[7],
		1024, target_ns, warmup_ns, samples, &arg, &ret, &cpu)) ||
	    !run_mode(env, gpu_mode, argv[8], argv[9],
		1, target_ns, warmup_ns, samples, &arg, &ret, &gpu)) {
		print_vm_error(env);
		goto cleanup;
	}

	printf("mode,iterations,samples,best_ms,median_ms,worst_ms,median_ns_per_forward,forwards_per_second\n");
	if (!skip_cpu)
		print_result(&cpu);
	print_result(&gpu);
	if (!skip_cpu) {
		printf("comparison,cpu_ns_per_forward,gpu_ns_per_forward,gpu_speedup\n");
		printf("cpu-vs-gpu,%.3f,%.3f,%.6f\n",
			(double)cpu.median / (double)cpu.iterations,
			(double)gpu.median / (double)gpu.iterations,
			((double)cpu.median / (double)cpu.iterations) /
			((double)gpu.median / (double)gpu.iterations));
	}
	status = 0;

cleanup:
	if (vm != NULL) (void)noct_destroy_vm(vm);
	free(source);
	return status;
}
