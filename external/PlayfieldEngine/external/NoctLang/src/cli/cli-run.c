/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: Run Mode
 */

#include <noct/noct.h>
#include "cli-main.h"

/*
 * Top level function for the run mode.
 */
int command_run(int argc, char *argv[])
{
	NoctConfig config;
	NoctVM *vm;
	NoctEnv *env;
	NoctValue ret;
	uint32_t file_arg;
	uint32_t i;
	char *data;
	size_t len;
	uint32_t arg_count;
	NoctValue *arg_value;

	noct_set_default_config(&config);

	/* Parse options. */
	file_arg = 1;
	for (i = 1; i < (uint32_t)argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[i], "--disable-jit") == 0) {
			config.jit_enable = false;
			file_arg++;
			continue;
		}
		if (strcmp(argv[i], "--force-jit") == 0) {
			config.jit_threshold = 0;
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--jit-threshold=", 16) == 0) {
			config.jit_threshold = atoi(argv[i] + 16);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--optimize-level=", 17) == 0) {
			config.optimize_level = atoi(argv[i] + 17);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-nursery-size=", 18) == 0) {
			config.gc_nursery_size = (size_t)atoi(argv[i] + 18);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-graduate-size=", 21) == 0) {
			config.gc_graduate_size = (size_t)atoi(argv[i] + 21);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-tenure-size=", 17) == 0) {
			config.gc_tenure_size = (size_t)atoi(argv[i] + 17);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-lop-threshold=", 18) == 0) {
			config.gc_lop_threshold = (size_t)atoi(argv[i] + 18);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-promotion-threshold=", 25) == 0) {
			config.gc_promotion_threshold = (size_t)atoi(argv[i] + 25);
			file_arg++;
			continue;
		}

		wide_printf(N_TR("Unknown option %s.\n"), argv[1]);
		return 1;
	}

	/* Check if a file is specified. */
	if (file_arg == (uint32_t)argc) {
		/* No file specified, enter REPL. */
		if (argc == 1) {
#if defined(NOCT_USE_REPL)
			return command_repl();
#else
			show_usage();
			return 1;
#endif
		}
		return 1;
	}

	/* Create a runtime. */
	if (!noct_create_vm(&vm, &env, &config)) {
		wide_printf(N_TR("Out of memory.\n"));
		return 1;
	}

	/* Register libraries. */
	if (!noct_register_api_math(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
	if (!noct_register_api_system(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
	if (!noct_register_api_console(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}

	/* Register native functions. */
	if (!register_cli_cfunc(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return 1;
	}

	for (i = file_arg; i < (uint32_t)argc; i++) {
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
				free(data);
				return 1;
			}
		} else {
			/* It's a bytecode file. */
			if (!noct_register_bytecode(env, (void *)data, (uint32_t)len)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				free(data);
				return 1;
			}
		}

		free(data);
	}

	/* Make the arguments for "main()". */
	arg_count = (uint32_t)argc - file_arg - 1;
	if (arg_count > 0) {
		arg_value = malloc(sizeof(NoctValue) * arg_count);
		if (arg_value == NULL)
			return 1;
		for (i = 0; i < arg_count; i++) {
			if (!noct_make_string(env, &arg_value[i], argv[file_arg + i + 1])) {
				wide_printf(N_TR("Out of memory.\n"));
				return 1;
			}
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
