/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
make_vm(NoctVM **vm, NoctEnv **env)
{
	NoctConfig config;
	noct_set_default_config(&config);
	config.jit_enable = false;
	if (!noct_create_vm(vm, env, &config)) return 0;
	if (!noct_register_api_file(*env)) {
		(void)noct_destroy_vm(*vm);
		return 0;
	}
	return 1;
}

int
main(int argc, char *argv[])
{
	NoctVM *vm_a, *vm_b, *vm;
	NoctEnv *env_a, *env_b, *env;
	NoctValue args[2], handle, file_handle, ignored;
	const char *message;
	int i;

	if (argc != 3) return 2;
	if (!make_vm(&vm_a, &env_a) || !make_vm(&vm_b, &env_b)) return 3;
	if (!noct_register_source(env_a, "owner-a.noct",
		"func makeHandle(path, hash) { return Weights.open(path, hash); }\n"
		"func makeFile(path) { return File.open(path, \"rb\"); }") ||
	    !noct_register_source(env_b, "owner-b.noct",
		"func closeForeign(handle) { Weights.close(handle); }\n"
		"func closeForeignFile(handle) { File.close(handle); }")) return 4;
	if (!noct_make_string(env_a, &args[0], argv[1]) ||
	    !noct_make_string(env_a, &args[1], argv[2]) ||
	    !noct_enter_vm(env_a, "makeHandle", 2, args, &handle)) return 5;
	if (noct_enter_vm(env_b, "closeForeign", 1, &handle, &ignored)) return 6;
	if (!noct_get_error_message(env_b, &message) ||
	    strstr(message, "different VM") == NULL) return 7;
	if (!noct_enter_vm(env_a, "makeFile", 1, &args[0], &file_handle)) return 11;
	if (noct_enter_vm(env_b, "closeForeignFile", 1, &file_handle, &ignored))
		return 12;
	if (!noct_get_error_message(env_b, &message) ||
	    strstr(message, "different VM") == NULL) return 13;
	if (!noct_destroy_vm(vm_b) || !noct_destroy_vm(vm_a)) return 8;

	for (i = 0; i < 40; i++) {
		if (!make_vm(&vm, &env)) return 9;
		if (!noct_register_source(env, "teardown.noct",
			"func leaveOpen(path, hash) { Weights.open(path, hash); }") ||
		    !noct_make_string(env, &args[0], argv[1]) ||
		    !noct_make_string(env, &args[1], argv[2]) ||
		    !noct_enter_vm(env, "leaveOpen", 2, args, &ignored) ||
		    !noct_destroy_vm(vm)) return 10;
	}
	puts("weights owner/teardown tests passed");
	return 0;
}
