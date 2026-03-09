/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

#include "cli-main.h"

/* Forward declaration. */
static bool do_transpile_elisp(const char *out_file, int in_file_count, const char *in_file[]);
static bool add_file_hook_elisp(const char *fname);

/*
 * Emacs Lisp Translation
 */

/*
 * The top level function for the Emacs Lisp translation.
 */
int command_transpile_elisp(int argc, char *argv[])
{
	if (argc < 4) {
		show_usage();
		return 1;
	}

	if (!do_transpile_elisp(argv[2], argc - 3, (const char **)&argv[3]))
		return 1;

	return 0;
}

/* Do Emacs Lisp translation. */
static bool do_transpile_elisp(const char *out_file, int in_file_count, const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!elback_init(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		if (!add_file(in_file[i], add_file_hook_elisp))
			return false;
	}

	/* Put a epilogue code. */
	if (!elback_finalize())
		return false;

	return true;
}

/* "On file add" callback for Emacs Lisp translation. */
static bool add_file_hook_elisp(const char *fname)
{
	char *data;
	size_t len;
	int func_count, j;

	/* Load a file. */
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

		/* Put Emacs Lisp. */
		hfunc = hir_get_function(j);
		if (!elback_translate_func(hfunc))
			return false;
	}

	/* Free entire HIR. */
	hir_cleanup();

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	return true;
}
