/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include "cli-main.h"
#include "../backend/backend.h"

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
	if (!noct_elback_start(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		/* A "--ns-map=FILE" pseudo-input loads the namespace map. */
		if (strncmp(in_file[i], "--ns-map=", 9) == 0) {
			if (!noct_elback_load_ns_map(in_file[i] + 9))
				return false;
			continue;
		}
		if (!add_file(in_file[i], add_file_hook_elisp))
			return false;
	}

	/* Put a epilogue code. */
	if (!noct_elback_finalize())
		return false;

	return true;
}

/* "On file add" callback for Emacs Lisp translation. */
static bool add_file_hook_elisp(const char *fname)
{
	char *data;
	size_t len;

	/* Load a file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Translate. */
	if (!noct_elback_translate(fname, data)) {
		free(data);
		return false;
	}

	free(data);
	return true;
}
