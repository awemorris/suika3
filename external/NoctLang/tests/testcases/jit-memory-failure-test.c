/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2026, Awe Morris
 */

/* JIT publication and teardown failure propagation regression test. */

#include <noct/noct.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

static int fail_mprotect;
static int fail_munmap;
static unsigned mprotect_calls;
static unsigned munmap_calls;

int __real_mprotect(void *address, size_t size, int prot);
int __real_munmap(void *address, size_t size);

int
__wrap_mprotect(void *address, size_t size, int prot)
{
	mprotect_calls++;
	if (fail_mprotect) {
		errno = EACCES;
		return -1;
	}
	return __real_mprotect(address, size, prot);
}

int
__wrap_munmap(void *address, size_t size)
{
	munmap_calls++;
	if (fail_munmap) {
		errno = EIO;
		return -1;
	}
	return __real_munmap(address, size);
}

static bool
call_value(NoctEnv *env, const char *name, int expected)
{
	NoctValue result;
	int value;

	return noct_enter_vm(env, name, 0, NULL, &result) &&
	       noct_get_int(env, &result, &value) && value == expected;
}

static bool
create_vm(NoctVM **vm, NoctEnv **env)
{
	NoctConfig config;

	noct_set_default_config(&config);
	config.optimize_level = 0;
	config.line_info = false;
	config.jit_code_size = 4096;
	return noct_create_vm(vm, env, &config);
}

int
main(void)
{
	static const char protect_source[] =
	    "func protected_value(): int { return 73; }\n";
	static const char teardown_source[] =
	    "func teardown_value(): int { return 91; }\n";
	static const char protect_error[] = "JIT memory protection failed.";
	const char *message;
	NoctVM *vm;
	NoctEnv *env;
	unsigned before;

	if (!create_vm(&vm, &env)) {
		fprintf(stderr, "cannot create protection-failure VM\n");
		return 1;
	}
	fail_mprotect = 1;
	before = mprotect_calls;
	if (noct_register_source(env, "protect-failure.noct", protect_source) ||
	    mprotect_calls == before ||
	    !noct_get_error_message(env, &message) ||
	    strcmp(message, protect_error) != 0) {
		fprintf(stderr, "mprotect failure was not propagated\n");
		return 1;
	}
	fail_mprotect = 0;
	/* The failed RX publication must leave no callable native entry.  The
	 * already-registered function remains safe to run in the interpreter.
	 */
	if (!call_value(env, "protected_value", 73) || !noct_destroy_vm(vm)) {
		fprintf(stderr, "failed JIT entry was retained\n");
		return 1;
	}

	if (!create_vm(&vm, &env) ||
	    !noct_register_source(env, "unmap-failure.noct", teardown_source) ||
	    !call_value(env, "teardown_value", 91)) {
		fprintf(stderr, "cannot prepare teardown-failure VM\n");
		return 1;
	}
	fail_munmap = 1;
	before = munmap_calls;
	if (noct_destroy_vm(vm) || munmap_calls == before) {
		fprintf(stderr, "munmap failure was not propagated\n");
		return 1;
	}
	fail_munmap = 0;

	puts("JIT memory failure propagation passed.");
	return 0;
}
