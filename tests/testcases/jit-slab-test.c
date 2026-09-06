/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Per-VM JIT slab ownership regression test. */

#include <noct/noct.h>

#include <stdio.h>

static bool
call_int(NoctEnv *env, const char *name, int expected)
{
	NoctValue ret;
	int value;

	if (!noct_enter_vm(env, name, 0, NULL, &ret) ||
	    !noct_get_int(env, &ret, &value))
		return false;
	return value == expected;
}

int
main(void)
{
	static const char source1[] =
		"func value(): int { return 11; }\n";
	static const char source2[] =
		"func value(): int { return 22; }\n";
	static const char source3[] =
		"func later(): int { return 33; }\n";
	NoctConfig config;
	NoctVM *vm1;
	NoctVM *vm2;
	NoctEnv *env1;
	NoctEnv *env2;

	noct_set_default_config(&config);
	config.optimize_level = 1;
	config.line_info = false;
	config.jit_code_size = 4096;
	if (!noct_create_vm(&vm1, &env1, &config) ||
	    !noct_create_vm(&vm2, &env2, &config)) {
		fprintf(stderr, "cannot create VMs\n");
		return 1;
	}
	if (!noct_register_source(env1, "vm1.noct", source1) ||
	    !noct_register_source(env2, "vm2.noct", source2) ||
	    !call_int(env1, "value", 11) ||
	    !call_int(env2, "value", 22)) {
		fprintf(stderr, "independent JIT execution failed\n");
		return 1;
	}
	/* Registering after the first commit must allocate/write only fresh RW
	 * pages while the older function remains executable. */
	if (!noct_register_source(env2, "later.noct", source3) ||
	    !call_int(env2, "value", 22) ||
	    !call_int(env2, "later", 33)) {
		fprintf(stderr, "incremental slab publication failed\n");
		return 1;
	}
	if (!noct_destroy_vm(vm1) || !call_int(env2, "value", 22) ||
	    !call_int(env2, "later", 33)) {
		fprintf(stderr, "destroying VM1 invalidated VM2 JIT code\n");
		return 1;
	}
	if (!noct_destroy_vm(vm2))
		return 1;
	puts("JIT slab VM isolation passed.");
	return 0;
}
