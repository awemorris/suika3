/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * CLI: Compile Mode
 */

#include "cli-main.h"

/* Forward declaration. */
static bool compile_source(const char *file_name);

/*
 * The top level function for the compile mode.
 */
int command_compile(int argc, char *argv[])
{
	int i;

	/* For each argument file. */
	for (i = 2; i < argc; i++) {
		/* Compile a source to bytecode. */
		if (!compile_source(argv[i]))
			return 1;
	}

	return 1;
}

/* Compile a source file. */
static bool compile_source(const char *file_name)
{
	char bc_fname[1024];
	FILE *fp;
	char *source_data, *dot;
	size_t source_length;
	int func_count, i, j;

	/* Load an argument source file. */
	if (!load_file_content(file_name, &source_data, &source_length))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(file_name, source_data)) {
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

	/* Make an output file name. (*.nb) */
	strcpy(bc_fname, file_name);
	dot = strstr(bc_fname, ".");
	if (dot != NULL)
		strcpy(dot, ".nb");
	else
		strcat(bc_fname, ".nb");

	/* Open an output .nb bytecode file. */
	fp = fopen(bc_fname, "wb");
	if (fp == NULL) {
		wide_printf(N_TR("Cannot open file %s.\n"), bc_fname);
		return false;
	}

	/* Put a file header. */
	func_count = hir_get_function_count();
	fprintf(fp, "Noct Bytecode 1.0\n");
	fprintf(fp, "Source\n");
	fprintf(fp, "%s\n", file_name);
	fprintf(fp, "Number Of Functions\n");
	fprintf(fp, "%d\n", func_count);

	/* For each HIR function. */
	for (i = 0; i < func_count; i++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(i);
		if (!lir_build(hfunc, &lfunc)) {
			wide_printf(N_TR("Error: %s: %d: %s\n"),
				    lir_get_file_name(),
				    lir_get_error_line(),
				    lir_get_error_message());
			return false;
		}

		/* Put a bytcode. */
		fprintf(fp, "Begin Function\n");
		fprintf(fp, "Name\n");
		fprintf(fp, "%s\n", lfunc->func_name);
		fprintf(fp, "Parameters\n");
		fprintf(fp, "%d\n", lfunc->param_count);
		for (j = 0; j < lfunc->param_count; j++)
			fprintf(fp, "%s\n", lfunc->param_name[j]);
		fprintf(fp, "Temporary Size\n");
		fprintf(fp, "%d\n", lfunc->tmpvar_size);
		fprintf(fp, "Bytecode Size\n");
		fprintf(fp, "%d\n", lfunc->bytecode_size);
		fwrite(lfunc->bytecode, (size_t)lfunc->bytecode_size, 1, fp);
		fprintf(fp, "\nEnd Function\n");

		/* Free a single LIR. */
		lir_cleanup(lfunc);
	}

	fclose(fp);

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	return true;
}
