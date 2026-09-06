/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-private inspection of Noct bytecode files.
 */

#ifndef NOCT_BYTECODE_FILE_H
#define NOCT_BYTECODE_FILE_H

#include <noct/noct.h>

#define BYTECODE_FILE_ERROR_SIZE	256
#define NOCT_APP_SHEBANG		"#!/usr/bin/noct\n"

enum bytecode_file_kind {
	BYTECODE_FILE_UNKNOWN,
	BYTECODE_FILE_MODULE_UNKNOWN,
	BYTECODE_FILE_MODULE_1_0,
	BYTECODE_FILE_MODULE_1_1,
	BYTECODE_FILE_APP_UNKNOWN,
	BYTECODE_FILE_APP_1_0
};

struct bytecode_file_span {
	const uint8_t *data;
	uint32_t size;
};

struct bytecode_file_function {
	char *name;
	char *source;
	uint32_t param_count;
	char **param_name;
	int *param_type;
	int *param_packed_type;
	bool *param_restricted;
	int return_type;
	int return_packed_type;
	bool return_type_checked;
	bool has_vector_ops;
	bool has_fma_ops;
	bool is_fast;
	void *fast_info;
	uint32_t tmpvar_size;
	struct bytecode_file_span bytecode;
};

struct bytecode_file_module {
	enum bytecode_file_kind kind;
	char *source;
	uint32_t require_count;
	char **require_name;
	uint32_t function_count;
	struct bytecode_file_function *function;
};

struct bytecode_file_binding {
	char *module_name;
	uint32_t module_index;
};

struct bytecode_file_app {
	uint32_t module_count;
	struct bytecode_file_module *module;
	uint32_t binding_count;
	struct bytecode_file_binding *binding;
	uint32_t root_count;
	uint32_t *root_index;
};

struct bytecode_file_error {
	size_t offset;
	char message[BYTECODE_FILE_ERROR_SIZE];
};

enum bytecode_file_kind bytecode_file_detect(
	const uint8_t *data,
	size_t size);

bool bytecode_file_check_registration_size(
	size_t size,
	uint32_t *size_out);

bool bytecode_file_inspect_module(
	const uint8_t *data,
	size_t size,
	struct bytecode_file_module *module,
	struct bytecode_file_error *error);

bool bytecode_file_inspect_app(
	const uint8_t *data,
	size_t size,
	struct bytecode_file_app *app,
	struct bytecode_file_error *error);

void bytecode_file_cleanup_module(
	struct bytecode_file_module *module);

void bytecode_file_cleanup_app(
	struct bytecode_file_app *app);

#endif
