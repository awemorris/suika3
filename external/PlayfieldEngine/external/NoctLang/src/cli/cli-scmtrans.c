/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include "cli-main.h"

/* Forward declaration. */
static bool do_transpile_scheme(const char *out_file, int in_file_count, const char *in_file[]);
static bool add_file_hook_scheme(const char *fname);

/*
 * Emacs Lisp Translation
 */

/*
 * The top level function for the Scheme translation.
 */
int command_transpile_scheme(int argc, char *argv[])
{
	if (argc < 4) {
		show_usage();
		return 1;
	}

	if (!do_transpile_scheme(argv[2], argc - 3, (const char **)&argv[3]))
		return 1;

	return 0;
}

/* Do Scheme translation. */
static bool do_transpile_scheme(const char *out_file, int in_file_count, const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!noct_scmback_start(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		if (!add_file(in_file[i], add_file_hook_scheme))
			return false;
	}

	/* Put a epilogue code. */
	if (!noct_scmback_finalize())
		return false;

	return true;
}

/* "On file add" callback for Scheme translation. */
static bool add_file_hook_scheme(const char *fname)
{
	char *data;
	size_t len;

	/* Load a file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Translate. */
	if (!noct_scmback_translate(fname, data)) {
		free(data);
		return false;
	}

	free(data);
	return true;
}
