/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Private bytecode backend module representation.
 */

#ifndef NOCT_BCBACK_PRIVATE_H
#define NOCT_BCBACK_PRIVATE_H

#include "lir.h"

struct bcback_module {
	char *source;
	uint32_t require_count;
	char **require_name;
	uint32_t function_count;
	struct lir_func **function;
};

bool bcback_build_module(
	const char *source_name,
	const char *source_text,
	struct bcback_module *module);
void bcback_cleanup_module(
	struct bcback_module *module);
bool bcback_serialize_module(
	const struct bcback_module *module,
	const char *temporary_base,
	uint8_t **data,
	uint32_t *size);
void bcback_abort(void);

#endif
