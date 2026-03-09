/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * CLI: C Translation Mode
 */

#include "cli-main.h"

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
int command_transpile_c(int argc, char *argv[])
{
	if (argc < 4) {
		show_usage();
		return 1;
	}

	if (!do_transpile_c(argv[2], argc - 3, (const char **)&argv[3]))
		return 1;

	return 0;
}

/* Do C translation. */
static bool do_transpile_c(const char *out_file, int in_file_count, const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!cback_init(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		/* Recursively add files. */
		if (!add_file(in_file[i], add_file_hook_c))
			return false;
	}

	/* Put a epilogue code. */
	if (!cback_finalize_dll())
		return false;

	return true;
}

/* "On file add" callback for the recursive file search. */
static bool add_file_hook_c(const char *fname)
{
	char *data;
	size_t len;
	int func_count, j;

	/* Load an argument file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(fname, data)) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    ast_get_file_name(),
			    ast_get_error_line(),
			    ast_get_error_message());
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    hir_get_file_name(),
			    hir_get_error_line(),
			    hir_get_error_message());
		return false;
	}

	/* For each HIR function. */
	func_count = hir_get_function_count();
	for (j = 0; j < func_count; j++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(j);
		if (!lir_build(hfunc, &lfunc)) {
			wide_printf(N_TR("Error: %s: %d: %s\n"),
				    lir_get_file_name(),
				    lir_get_error_line(),
				    lir_get_error_message());
			return false;;
		}

		/* Put a C function. */
		if (!cback_translate_func(lfunc))
			return false;

		/* Free a single LIR. */
		lir_cleanup(lfunc);
	}

	/* Free intermediated. */
	hir_cleanup();
	ast_cleanup();

	return true;
}
