/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>

static bool
run_vm(bool check_failure)
{
	NoctConfig config;
	NoctVM *vm;
	NoctEnv *env;
	NoctValue value;
	NoctValue ret;
	NoctFunc *func;
	bool loaded;
	bool present;
	int result;

	noct_set_default_config(&config);
	config.jit_enable = false;
	if (!noct_create_vm(&vm, &env, &config))
		return false;
	if (!noct_load_library(env, "good", false, &loaded) || !loaded ||
	    !noct_get_global(env, "dynlib_value", &value) ||
	    !noct_get_func(env, &value, &func) ||
	    !noct_call(env, func, 0, NULL, &ret) ||
	    !noct_get_int(env, &ret, &result) || result != 42)
		return false;
	if (check_failure) {
		if (noct_load_library(env, "fail", false, &loaded))
			return false;
		if (!noct_check_global(env, "dynlib_partial", &present) || present)
			return false;
		/* tryLoad suppresses absence only; a present malformed library fails. */
		if (noct_load_library(env, "missing_init", true, &loaded))
			return false;
	}
	return noct_destroy_vm(vm);
}

int
main(void)
{
	if (!run_vm(true) || !run_vm(false)) {
		fprintf(stderr, "dynamic-library host test failed\n");
		return 1;
	}
	return 0;
}
