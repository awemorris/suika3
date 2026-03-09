/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * CLI: Run Mode
 */

#include "cli-main.h"

/*
 * Top level function for the run mode.
 */
int command_run(int argc, char *argv[])
{
	NoctVM *vm;
	NoctEnv *env;
	NoctValue ret;
	int file_arg;
	int i;
	char *data;
	size_t len;
	int arg_count;
	NoctValue *arg_value;

	/* Parse options. */
	file_arg = 1;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[1], "--disable-jit") == 0) {
			noct_conf_jit_enable = false;
			file_arg++;
			continue;
		}
		if (strcmp(argv[1], "--force-jit") == 0) {
			noct_conf_jit_threshold = 0;
			file_arg++;
			continue;
		}

		wide_printf(N_TR("Unknown option %s.\n"), argv[1]);
		return 1;
	}

	/* Check if a file is specified. */
	if (file_arg == argc) {
		/* No file specified, enter REPL. */
		if (argc == 1)
			return command_repl();
		return 1;
	}

	/* Create a runtime. */
	if (!noct_create_vm(&vm, &env))
		return 1;

	/* Register libraries. */
	NOCT_REGISTER_ALL_APIS(env);

	/* Register FFI functions. */
	if (!register_cli_ffi(env))
		return 1;

	/* Load a file content. */
	if (!load_file_content(argv[i], &data, &len))
		return 1;

	/* Check for the bytecode header. */
	if (strncmp(data, NOCT_BYTECODE_HEADER, strlen(NOCT_BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, argv[i], data)) {
			const char *file, *msg;
			int line;
			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return 1;
		}
	} else {
		/* It's a bytecode file. */
		if (!noct_register_bytecode(env, (void *)data, len)) {
			const char *file, *msg;
			int line;
			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return 1;
		}
	}

	/* Make the arguments for "main()". */
	arg_count = argc - file_arg - 1;
	if (arg_count > 0) {
		arg_value = malloc(sizeof(NoctValue) * arg_count);
		if (arg_value == NULL)
			return 1;
		for (i = 0; i < arg_count; i++) {
			if (!noct_make_string(env, &arg_value[i], argv[file_arg + i + 1]))
				return 1;
		}
	} else {
		arg_value = NULL;
	}

	/* Run the "main()" function. */
	if (!noct_enter_vm(env, "main", arg_count, arg_value, &ret)) {
		const char *file, *msg;
		int line;
		noct_get_error_file(env, &file);
		noct_get_error_line(env, &line);
		noct_get_error_message(env, &msg);
		wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
		return 1;
	}

	/* Destroy the runtime. */
	if (!noct_destroy_vm(vm))
		return 1;

	return 0;
}
