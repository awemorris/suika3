/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: C Translation Mode
 */

#include "cli-main.h"
#include "../backend/backend.h"

/* Bytecode File Header */
#define BYTECODE_HEADER		"Noct Bytecode"

/* i18n.c */
#if defined(NOCT_USE_TRANSLATION)
void noct_init_locale(void);
#endif

/* Forward declaration. */
static bool do_transpile_c(const char *out_file, int in_file_count, const char *in_file[]);
static bool add_file_hook_c(const char *fname);

/*
 * C Translation
 */

/*
 * The top level function for the C translation mode.
 */
int
command_transpile_c(
	int argc,
	char *argv[])
{
	int first;
	int optimize_level;
	bool lineinfo;
	enum cli_optimize_level_result optimize_result;

	/* Optional compiler diagnostics/settings before the output file. */
	first = 2;
	while (first < argc) {
		optimize_result = parse_optimize_level_option(
			argv[first], &optimize_level, &lineinfo);
		if (optimize_result == CLI_OPTIMIZE_LEVEL_VALID) {
			noct_cback_set_optimize_level(optimize_level);
			noct_cback_set_lineinfo(lineinfo);
			first++;
			continue;
		}
		if (optimize_result == CLI_OPTIMIZE_LEVEL_INVALID) {
			printf(N_TR("Invalid optimize-level option %s.\n"), argv[first]);
			return 1;
		}
		if (strcmp(argv[first], "--simd-info") == 0) {
			noct_cback_set_simd_info(true);
			first++;
			continue;
		}
		break;
	}

	if (argc < first + 2) {
		show_usage();
		return 1;
	}

	if (!do_transpile_c(argv[first], argc - first - 1, (const char **)&argv[first + 1]))
		return 1;

	return 0;
}

/* Do C translation. */
static bool
do_transpile_c(
	const char *out_file,
	int in_file_count,
	const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!noct_cback_start(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		/* Recursively add files. */
		if (!add_file(in_file[i], add_file_hook_c))
			return false;
	}

	/* Put a epilogue code. */
	if (!noct_cback_finalize())
		return false;

	return true;
}

/* "On file add" callback for the recursive file search. */
static bool
add_file_hook_c(
	const char *fname)
{
	char *data;
	size_t len;

	/* Load a file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Translate. */
	if (!noct_cback_translate(fname, data)) {
		free(data);
		return false;
	}

	free(data);
	return true;
}
