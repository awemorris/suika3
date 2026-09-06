/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-private Noct application container writer.
 */

#ifndef NOCT_BCBACK_APP_H
#define NOCT_BCBACK_APP_H

#include <noct/c89compat.h>

struct bcback_app_module {
	const char *logical_source;
	const char *source_text;
	const uint8_t *bytecode_data;
	uint32_t bytecode_size;
};

struct bcback_app_binding {
	const char *module_name;
	uint32_t module_index;
};

bool bcback_write_app(
	const char *output_path,
	uint32_t module_count,
	const struct bcback_app_module module[],
	uint32_t binding_count,
	const struct bcback_app_binding binding[],
	uint32_t root_count,
	const uint32_t root[]);

#endif
