/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_CLI_MAIN_H
#define NOCT_CLI_MAIN_H

#include <noct/noct.h>

#include "module.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

enum cli_optimize_level_result {
	CLI_OPTIMIZE_LEVEL_NOT_MATCHED,
	CLI_OPTIMIZE_LEVEL_VALID,
	CLI_OPTIMIZE_LEVEL_INVALID
};

void
show_usage(void);

enum cli_optimize_level_result
parse_optimize_level_option(
	const char *arg,
	int *level,
	bool *lineinfo);

bool
load_file_content(
	const char *fname,
	char **data,
	size_t *size);

int
wide_printf(
	const char *format,
	...);

bool
add_file(
	const char *fname,
	bool (*add_file_hook)(const char *));

int
command_compile(
	int argc,
	char *argv[]);

int
command_transpile_c(
	int argc,
	char *argv[]);

int
command_transpile_elisp(
	int argc,
	char *argv[]);

int
command_transpile_scheme(
	int argc,
	char *argv[]);

int
command_run(
	int argc,
	char *argv[]);

int
command_repl(void);

bool
register_cli_cfunc(
	NoctEnv *env);

#endif
