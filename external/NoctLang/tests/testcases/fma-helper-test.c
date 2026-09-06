/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Direct portable VFMA helper and alias-semantics test. */

#include <noct/noct.h>
#include <noct/aot.h>
#include "runtime.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t
float_bits(float value)
{
	uint32_t bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static int
same_result(float actual, float expected)
{
	if (isnan(expected))
		return isnan(actual);
	return float_bits(actual) == float_bits(expected);
}

static int
run_alias_case(NoctEnv *env, int dst, const float *a, const float *b,
	       const float *c)
{
	float actual[4];
	int lane;

	memcpy(env->vreg[0], a, 16);
	memcpy(env->vreg[1], b, 16);
	memcpy(env->vreg[2], c, 16);
	if (!noct_ex_vfmaf32x4_helper(env, dst, 0, (1 << 8) | 2))
		return 0;
	memcpy(actual, env->vreg[dst], 16);
	for (lane = 0; lane < 4; lane++) {
		float expected = fmaf(a[lane], b[lane], c[lane]);
		if (!same_result(actual[lane], expected)) {
			fprintf(stderr,
				"alias %d lane %d: got %08x expected %08x\n",
				dst, lane, (unsigned)float_bits(actual[lane]),
				(unsigned)float_bits(expected));
			return 0;
		}
	}
	return 1;
}

int
main(void)
{
	NoctConfig config;
	NoctVM *vm;
	NoctEnv *env;
	float a[4], b[4], c[4], same[4], actual[4];
	int dst, lane;

	/* Lane zero distinguishes fused from separately rounded arithmetic. */
	a[0] = 0x1.000002p0f;
	b[0] = 0x1.000002p0f;
	c[0] = -0x1.000004p0f;
	a[1] = 0.0f; b[1] = -2.0f; c[1] = -0.0f;
	a[2] = -0.0f; b[2] = -0.0f; c[2] = 0.0f;
	a[3] = 0x1p-126f; b[3] = 0.5f; c[3] = 0x1p-149f;

	noct_set_default_config(&config);
	config.jit_enable = false;
	if (!noct_create_vm(&vm, &env, &config))
		return 1;
	for (dst = 0; dst < 4; dst++) {
		if (!run_alias_case(env, dst, a, b, c)) {
			noct_destroy_vm(vm);
			return 1;
		}
	}

	/* Complete alias: vd == va == vb == vc. */
	same[0] = 1.5f;
	same[1] = -2.0f;
	same[2] = INFINITY;
	same[3] = NAN;
	memcpy(env->vreg[0], same, 16);
	if (!noct_ex_vfmaf32x4_helper(env, 0, 0, 0)) {
		noct_destroy_vm(vm);
		return 1;
	}
	memcpy(actual, env->vreg[0], 16);
	for (lane = 0; lane < 4; lane++) {
		float expected = fmaf(same[lane], same[lane], same[lane]);
		if (!same_result(actual[lane], expected)) {
			noct_destroy_vm(vm);
			return 1;
		}
	}
	noct_destroy_vm(vm);
	puts("VFMA helper tests passed.");
	return 0;
}
