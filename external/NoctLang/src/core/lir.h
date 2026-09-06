/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * LIR: Low-level Intermediate Representation
 */

#ifndef NOCT_LIR_H
#define NOCT_LIR_H

#include <noct/noct.h>

#define LIR_PARAM_SIZE		32
#define LIR_TMPVAR_MAX		128

struct hir_block;

struct lir_func {
	/* Size of tmpvar. */
	uint32_t tmpvar_size;

	/* Size of bytecode array. */
	uint32_t bytecode_size;

	/* Bytecode array. */
	uint8_t *bytecode;

	/* Source file name. */
	char *file_name;

	/* Function name. */
	char *func_name;

	/* Parameter count. */
	uint32_t param_count;

	/* Parameter names. */
	char *param_name[LIR_PARAM_SIZE];

	/* Parameter type annotation. */
	int param_type[LIR_PARAM_SIZE];

	/* Pckaed parameter type annotation. */
	int param_packed_type[LIR_PARAM_SIZE];

	/* Restricted Packed parameter type annotation. */
	bool param_restricted[LIR_PARAM_SIZE];

	/* Return type annotation. */
	int return_type;

	/* Packed return type annotation. */
	int return_packed_type;

	/* The bytecode enforces the declared return type on every edge. */
	bool return_type_checked;

	/* The bytecode contains at least one vector operation. */
	bool has_vector_ops;

	/* Did the optimizer commit the __fast contract? */
	bool is_fast;

	/* Optimizer-owned entry contract. */
	void *fast_info;

	/* The bytecode contains a fused multiply-add operation. */
	bool has_fma_ops;
};

/*
 * Build a LIR function from a HIR function.
 */
bool
lir_build(
	struct hir_block *hir_func,
	struct lir_func **lir_func);

/*
 * Free a constructed LIR.
 */
void
lir_cleanup(
	struct lir_func *func);

/*
 * Get a file name.
 */
const char *
lir_get_file_name(void);

/*
 * Get an error line.
 */
int
lir_get_error_line(void);

/*
 * Get an error message.
 */
const char *
lir_get_error_message(void);

/*
 * Set the optimization level.
 */
void
lir_set_optimize_level(int level);

/*
 * Enable or disable lineinfo ops.
 */
void
lir_set_lineinfo(bool enable);

/*
 * Dump LIR.
 */
void
lir_dump(
	struct lir_func *func);

#endif
