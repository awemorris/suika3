/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Verifies that the embedding runtime never invokes the CLI-owned resolver.
 */

#include <noct/noct.h>

#include <stdio.h>
#include <stdlib.h>

static uint32_t resolver_call_count;

static char *test_resolve_module(const char *module_name);

/*
 * Register one source unit containing an otherwise unused require.
 */
int
main(
	void)
{
	static const char source[] =
		"require absent;\n"
		"func main() {\n"
		"}\n";
	NoctConfig config;
	NoctVM *vm;
	NoctEnv *env;
	bool vm_created;
	bool succeeded;

	vm = NULL;
	env = NULL;
	vm_created = false;
	succeeded = false;

	noct_set_default_config(&config);
	config.require_resolver = test_resolve_module;
	if (!noct_create_vm(&vm, &env, &config)) {
		fprintf(stderr, "Cannot create test VM.\n");
		goto cleanup;
	}
	vm_created = true;

	if (!noct_register_source(env, "core-require-test.noct", source)) {
		const char *message;

		if (noct_get_error_message(env, &message))
			fprintf(stderr, "Registration failed: %s\n", message);
		goto cleanup;
	}
	if (resolver_call_count != 0) {
		fprintf(stderr, "Core runtime invoked require_resolver.\n");
		goto cleanup;
	}

	succeeded = true;

cleanup:
	if (vm_created && !noct_destroy_vm(vm))
		succeeded = false;

	return succeeded ? 0 : 1;
}

/* Fail visibly if core ever crosses the resolver ownership boundary. */
static char *
test_resolve_module(
	const char *module_name)
{
	UNUSED_PARAMETER(module_name);

	resolver_call_count++;

	return NULL;
}
