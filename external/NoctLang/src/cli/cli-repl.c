/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * CLI: REPL Mode
 */

#include "cli-main.h"

/* Forward declaration. */
static bool run_repl(void);
static bool is_multiline_start(const char *text, bool *is_func);
static bool accept_multiline(const char *text);

int command_repl(void)
{
	if (!run_repl())
		return 1;

	return 0;
}

static bool run_repl(void)
{		
	NoctVM *vm;
	NoctEnv *env;

	/* Create a runtime. */
	if (!noct_create_vm(&vm, &env))
		return false;

	/* Register libraries. */
	NOCT_REGISTER_ALL_APIS(env);

	/* Register FFI functions. */
	if (!register_cli_ffi(env))
		return false;

	wide_printf(N_TR("Noct Programming Language\n"));
	wide_printf(N_TR("Entering REPL mode.\n"));
	wide_printf("\n");

	/* Prompt. */
	while (1) {
		char line[4096];
		char entire[32768];
		char *start;
		bool is_func;
		NoctValue ret, zero;

		memset(line, 0, sizeof(line));
		memset(entire, 0, sizeof(entire));

		/* Show the prompt and get an input. */
		wide_printf("> ");
		if (fgets(line, sizeof(line) - 1, stdin) == NULL)
			break;

		/* Check if it is multiple lines. */
		if (!is_multiline_start(line, &is_func)) {
			/* Single line. */

			/* Make a function. */
			strncpy(entire, "func repl() {", sizeof(entire) - 1);
			strncat(entire, line, sizeof(entire) - 1);
			strncat(entire, "; }", sizeof(entire) - 1);

			/* Compile the source. */
			if (!noct_register_source(env, "REPL", entire)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				continue;
			}

			/* Run the "repl()" function. */
			if (!noct_enter_vm(env, "repl", 0, NULL, &ret)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				continue;
			}
		} else {
			/* Multiple lines. */

			/* Make a function if the block is not a function. */
			if (!is_func)
				strncpy(entire, "func repl() {", sizeof(entire) - 1);
			else
				strcpy(entire, "");
			start = &entire[strlen(entire)];

			/* Show the multiline prompt and get inputs until the block ends. */
			strncat(entire, line, sizeof(entire) - 1);
			while (!accept_multiline(start)) {
				wide_printf(". ");
				if (fgets(line, sizeof(line) - 1, stdin) == NULL)
					break;
				strncat(entire, line, sizeof(entire) - 1);
			}

			/* Terminate the synthetic function if the block is not a function. */
			if (!is_func)
				strncat(entire, "}", sizeof(entire) - 1);

			/* Compile the source. */
			if (!noct_register_source(env, "REPL", entire)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				continue;
			}

			/* If the block is not a function, run the synthetic function. */
			if (!is_func) {
				/* Run the "repl()" function. */
				if (!noct_enter_vm(env, "repl", 0, NULL, &ret)) {
					const char *file, *msg;
					int line;
					noct_get_error_file(env, &file);
					noct_get_error_line(env, &line);
					noct_get_error_message(env, &msg);
					wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
					continue;
				}
			}
		}

		/* Make the "repl()" function updatable. */
		noct_make_int(env, &zero, 0);
		if (!noct_set_global(env, "repl", &zero))
			return false;
	}

	/* Destroy the runtime. */
	if (!noct_destroy_vm(vm))
		return false;

	return true;
}

/* Check for the start of the multiple line mode. */
static bool is_multiline_start(const char *text, bool *is_func)
{
	const char *s;

	s = text;
	while (*s == ' ')
		s++;

	*is_func = false;
	if (strncmp(s, "func", 4) == 0) {
		if (s[4] == ' ' || s[4] == '\t' || s[4] == '\n' || s[4] == '(') {
			*is_func = true;
			return true;
		}
	}
	if (strncmp(s, "if", 2) == 0) {
		if (s[2] == ' ' || s[2] == '\t' || s[2] == '\n' || s[2] == '(')
			return true;
	}
	if (strncmp(s, "for", 3) == 0) {
		if (s[3] == ' ' || s[3] == '\t' || s[3] == '\n' || s[3] == '(')
			return true;
	}
	if (strncmp(s, "while", 5) == 0) {
		if (s[5] == ' ' || s[5] == '\t' || s[5] == '\n' || s[5] == '(')
			return true;
	}

	return false;
}

/* Check for the end of the multiple line mode. */
static bool accept_multiline(const char *text)
{
	int open, close;
	const char *s;

	open = 0;
	close = 0;
	s = text;
	while(*s) {
		if (*s == '{')
			open++;
		else if (*s == '}')
			close++;
		s++;
	}

	if (open > 0 && open == close)
		return true; /* Matched. */

	return false;
}
