/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * bcback: Bytecode Backend
 */

#include <noct/noct.h>
#include "ast.h"
#include "hir.h"
#include "lir.h"
#include "bytecode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

static FILE *fp;

/*
 * Start the BC backend.
 */
NOCT_DLL
bool
noct_bcback_start(
	const char *out_file_name)
{
	fp = fopen(out_file_name, "wb");
	if (fp == NULL) {
		printf("Failed to open file \"%s\".\n", out_file_name);
		return false;
	}

	return true;
}

/*
 * Translate a file using BC backend.
 */
NOCT_DLL
bool
noct_bcback_translate(
	const char *source_file_name,
	const char *source_data)
{
	uint32_t func_count, i, j;

	/* Do parse, build AST. */
	if (!ast_build(source_file_name, source_data)) {
		printf(N_TR("Error: %s:%d: %s\n"),
		       ast_get_file_name(),
		       ast_get_error_line(),
		       ast_get_error_message());
		return false;
	}

	/* Put a file header. */
	func_count = hir_get_function_count();
	fprintf(fp, "Noct Bytecode 1.0\n");
	fprintf(fp, "Source\n");
	fprintf(fp, "%s\n", source_file_name);
	fprintf(fp, "Number Of Functions\n");
	fprintf(fp, "%d\n", func_count);

	/* Transform AST to HIR. */
	if (!hir_build()) {
		printf(N_TR("Error: %s:%d: %s\n"),
		       hir_get_file_name(),
		       hir_get_error_line(),
		       hir_get_error_message());
		return false;
	}

	/* For each HIR function. */
	for (i = 0; i < func_count; i++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(i);
		if (!lir_build(hfunc, &lfunc)) {
			printf(N_TR("Error: %s:%d: %s\n"),
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

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	return true;
}

/*
 * Finalize the BC backend.
 */
NOCT_DLL
bool
noct_bcback_finalize(void)
{
	if (fp == NULL)
		return false;

	fclose(fp);
	fp = NULL;

	return true;
}
