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

#if defined(NOCT_TARGET_WINDOWS)
#include <windows.h>
#endif

static NoctVM *vm;
static NoctEnv *env;
static NoctConfig config;
static NoctValue arg;
static int file_arg;
static int prog_arg;
static size_t param_count;
static bool is_oneliner;

static bool parse_options(int argc, char *argv[]);
static bool load_program(int argc, char *argv[]);
static bool load_args(int argc, char *argv[]);
static bool check_params(void);

/*
 * Top level function for the run mode.
 */
int command_run(int argc, char *argv[])
{
	NoctValue ret;

	noct_set_default_config(&config);

	/* Parse options. */
	if (!parse_options(argc, argv))
		return 1;

	/* Check if a file is specified. */
	if (file_arg == argc && !is_oneliner) {
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
	if (!noct_register_api_system(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
	if (!noct_register_api_console(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
	if (!noct_register_api_file(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}

	/* Register native functions. */
	if (!register_cli_cfunc(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return 1;
	}

	/* Load program. */
	if (!load_program(argc, argv))
		return 1;

	/* Load arguments. */
	if (!load_args(argc, argv))
		return 1;

	/* Check main parameters. */
	if (!check_params())
		return 1;

	/* Run the "main()" function. */
	if (!noct_enter_vm(env,
			   "main",
			   param_count == 0 ? 0 : 1,
			   &arg,
			   &ret)) {
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

static bool
parse_options(
	int argc,
	char *argv[])
{
	int i;

	file_arg = 1;
	is_oneliner = false;
	for (i = 1; i < argc; i++) {
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
		if (strncmp(argv[i], "--one-line", 10) == 0 ||
		    strcmp(argv[i], "-e") == 0) {
			if (argc <= (int)i + 1) {
				wide_printf(N_TR("Specify a command.\n"));
				return 1;
			}
			is_oneliner = true;
			prog_arg = i + 1;
			i++;
			file_arg++;
			continue;
		}

		wide_printf(N_TR("Unknown option %s.\n"), argv[1]);
		return false;
	}

	return true;
}

static bool
load_program(
	int argc,
	char *argv[])
{
	static char entire[32768];
	char *data;
	size_t len;

	UNUSED_PARAMETER(argc);

	/* Load an one liner if exists. */
	if (is_oneliner) {
		/* Make a function. */
		snprintf(entire, sizeof(entire), "func main() { %s; }", argv[prog_arg]);
		if (!noct_register_source(env, "oneliner", entire)) {
			const char *file, *msg;
			int line;

			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return false;
		}
		return true;
	}

	/* Load a file content. */
	if (!load_file_content(argv[file_arg], &data, &len))
		return false;

	/* Check for the bytecode header. */
	if (strncmp(data, NOCT_BYTECODE_HEADER, strlen(NOCT_BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, argv[file_arg], data)) {
			const char *file, *msg;
			int line;

			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			free(data);
			return false;
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
			return false;
		}
	}

	free(data);
	return true;
}

static bool
load_args(
	int argc,
	char *argv[])
{
	NoctValue val;

	/* Make the arguments for "main()". */
        if (!noct_make_empty_array(env, &arg))
                return false;

#if defined(NOCT_TARGET_WINDOWS)
	{
		int i;
		size_t index;

		index = 0;
		for (i = file_arg + 1; i < __argc; i++) {
			const wchar_t *wstr = __wargv[i];
			char *utf8_buf = NULL;
			int size_needed = 0;

			size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
			if (size_needed <= 0)
				return false;

			utf8_buf = malloc(size_needed);
			if (!utf8_buf)
				return false;

			WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8_buf, size_needed, NULL, NULL);

			if (!noct_set_array_elem_make_string(env, &arg, index++, &val, utf8_buf)) {
				free(utf8_buf);
				return false;
			}
			free(utf8_buf);
		}
	}
#else
	{
		int i;
		size_t index;

		index = 0;
		for (i = file_arg + 1; i < argc; i++) {
			if (!noct_set_array_elem_make_string(env, &arg, index++, &val, argv[i]))
				return false;
		}

	}
#endif

	return true;
}

static bool
check_params(void)
{
        NoctValue main_val;
	NoctFunc *main_func;

	/* Check for main(). */
	if (!noct_get_global(env, "main", &main_val)) {
		wide_printf(N_TR("main() is not defined\n"));
		return false;
	}

	if (!noct_get_func(env, &main_val, &main_func))
		return false;

	if (!noct_get_func_param_count(env, main_func, &param_count))
		return false;

	return true;
}
