/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * LIR: Low-level intermediate representation
 */

#ifndef NOCT_LIR_H
#define NOCT_LIR_H

#include <noct/c89compat.h>

#define LIR_PARAM_SIZE		32

struct hir_block;

struct lir_func {
	char *file_name;
	char *func_name;
	int param_count;
	char *param_name[LIR_PARAM_SIZE];
	int tmpvar_size;
	int bytecode_size;
	uint8_t *bytecode;
};

/* Build a LIR function from a HIR function. */
bool lir_build(struct hir_block *hir_func, struct lir_func **lir_func);

/* Free a constructed LIR. */
void lir_cleanup(struct lir_func *func);

/* Get a file name. */
const char *lir_get_file_name(void);

/* Get an error line. */
int lir_get_error_line(void);

/* Get an error message. */
const char *lir_get_error_message(void);

/* Dump LIR. */
void lir_dump(struct lir_func *func);

#endif
