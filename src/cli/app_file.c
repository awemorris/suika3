/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-private Noct application container writer.
 */

#include "app_file.h"

#include "bcback_file.h"
#include "bcback_private.h"
#include "bytecode.h"
#include "bytecode_file.h"

#include <noct/noct.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
#include <sys/stat.h>
#include <unistd.h>
#endif

struct bcback_app_blob {
	uint8_t *data;
	uint32_t size;
};

static bool bcback_app_array_size_valid(uint32_t count, size_t item_size);
static bool bcback_app_source_is_portable(const char *source);
static bool bcback_app_keep_embedded_source(const struct bytecode_file_module *module);
static bool bcback_app_prepare_source(const struct bcback_app_module *input, const char *temporary_base, struct bcback_app_blob *blob);
static bool bcback_app_prepare_bytecode(const struct bcback_app_module *input, const char *temporary_base, struct bcback_app_blob *blob);
static bool bcback_app_serialize_inspected_module(const struct bytecode_file_module *module, const char *logical_source, const char *temporary_base, uint8_t **data, uint32_t *size);
static bool bcback_app_build_inspected_function(const struct bytecode_file_function *function, const char *logical_source, const char *module_source, bool rewrite_source, struct lir_func *lir_function);
static void bcback_app_cleanup_inspected_functions(const struct bytecode_file_module *module, struct lir_func lir_function[]);
static bool bcback_app_contains_bytes(const uint8_t *data, uint32_t size, const char *needle);
static bool bcback_app_inspected_needs_rewrite(const struct bytecode_file_module *module, const char *logical_source);
static bool bcback_app_inspected_safe_to_rewrite(const struct bytecode_file_module *module);
static void bcback_app_cleanup_blobs(uint32_t count, struct bcback_app_blob blob[]);
static bool bcback_app_write_stream(FILE *stream, uint32_t module_count, const struct bcback_app_blob module[], uint32_t binding_count, const struct bcback_app_binding binding[], uint32_t root_count, const uint32_t root[]);
static bool bcback_app_stream_to_blob(FILE *stream, uint8_t **data, uint32_t *size);
static bool bcback_app_build_blob(const char *temporary_base, uint32_t module_count, const struct bcback_app_blob module[], uint32_t binding_count, const struct bcback_app_binding binding[], uint32_t root_count, const uint32_t root[], uint8_t **data, uint32_t *size);

/*
 * Writes one self-contained, strictly validated Noct application.
 */
bool
bcback_write_app(
	const char *output_path,
	uint32_t module_count,
	const struct bcback_app_module module[],
	uint32_t binding_count,
	const struct bcback_app_binding binding[],
	uint32_t root_count,
	const uint32_t root[])
{
	struct bcback_app_blob *module_blob;
	struct bytecode_file_app inspected;
	struct bytecode_file_error error;
	struct bcback_output output;
	uint8_t *app_data;
	uint32_t app_size;
	FILE *stream;
	uint32_t i;
	bool succeeded;

	memset(&inspected, 0, sizeof(inspected));
	memset(&output, 0, sizeof(output));
	module_blob = NULL;
	app_data = NULL;
	app_size = 0;
	succeeded = false;

	if (output_path == NULL || output_path[0] == '\0')
		goto cleanup;
	if (module_count == 0 || module == NULL)
		goto cleanup;
	if (binding_count > 0 && binding == NULL)
		goto cleanup;
	if (root_count == 0 || root == NULL)
		goto cleanup;
	if (!bcback_app_array_size_valid(
		module_count,
		sizeof(*module_blob))) {
		goto cleanup;
	}

	module_blob = noct_calloc(
		(size_t)module_count,
		sizeof(*module_blob));
	if (module_blob == NULL)
		goto cleanup;

	/* Prepare every canonical 1.1 record before opening the destination. */
	for (i = 0; i < module_count; i++) {
		if (!bcback_app_source_is_portable(module[i].logical_source)) {
			printf(N_TR("Error: Invalid application source name.\n"));
			goto cleanup;
		}

		if (module[i].source_text != NULL) {
			if (module[i].bytecode_data != NULL ||
			    module[i].bytecode_size != 0) {
				goto cleanup;
			}
			if (!bcback_app_prepare_source(
				&module[i],
				output_path,
				&module_blob[i])) {
				printf(
					N_TR("Error: Cannot prepare application module %s.\n"),
					module[i].logical_source);
				goto cleanup;
			}
		} else {
			if (module[i].bytecode_data == NULL ||
			    module[i].bytecode_size == 0) {
				goto cleanup;
			}
			if (!bcback_app_prepare_bytecode(
				&module[i],
				output_path,
				&module_blob[i])) {
				printf(
					N_TR("Error: Cannot prepare application module %s.\n"),
					module[i].logical_source);
				goto cleanup;
			}
		}
	}

	if (!bcback_app_build_blob(
		output_path,
		module_count,
		module_blob,
		binding_count,
		binding,
		root_count,
		root,
		&app_data,
		&app_size)) {
		printf(N_TR("Error: Cannot assemble application records.\n"));
		goto cleanup;
	}

	if (!bytecode_file_inspect_app(
		app_data,
		(size_t)app_size,
		&inspected,
		&error)) {
		printf(
			N_TR("Error: Invalid application at byte %lu: %s\n"),
			(unsigned long)error.offset,
			error.message);
		goto cleanup;
	}
	bytecode_file_cleanup_app(&inspected);

	if (!bcback_output_open(&output, output_path))
		goto cleanup;
	stream = bcback_output_get_stream(&output);

	if (fwrite(
		NOCT_APP_SHEBANG,
		1,
		strlen(NOCT_APP_SHEBANG),
		stream) != strlen(NOCT_APP_SHEBANG)) {
		goto cleanup;
	}
	if (fwrite(app_data, 1, app_size, stream) != app_size)
		goto cleanup;

#if defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
	if (fchmod(fileno(stream), 0755) != 0)
		goto cleanup;
#endif

	if (!bcback_output_commit(&output))
		goto cleanup;

	succeeded = true;

cleanup:
	bytecode_file_cleanup_app(&inspected);
	bcback_output_abort(&output);
	noct_free(app_data);
	bcback_app_cleanup_blobs(module_count, module_blob);
	noct_free(module_blob);

	return succeeded;
}

/* Check one count-sized table without overflowing size_t. */
static bool
bcback_app_array_size_valid(
	uint32_t count,
	size_t item_size)
{
	if (count == 0)
		return true;
	if (item_size > SIZE_MAX / (size_t)count)
		return false;

	return true;
}

/* Check one logical source name against the portable app identity policy. */
static bool
bcback_app_source_is_portable(
	const char *source)
{
	const char *component;
	const char *cursor;
	size_t length;

	if (source == NULL || source[0] == '\0')
		return false;
	if (source[0] == '/' || source[0] == '\\')
		return false;
	if (((source[0] >= 'A' && source[0] <= 'Z') ||
	     (source[0] >= 'a' && source[0] <= 'z')) &&
	    source[1] == ':') {
		return false;
	}
	if (strlen(source) >= 1024)
		return false;
	if (strchr(source, '\n') != NULL || strchr(source, '\r') != NULL)
		return false;

	component = source;
	cursor = source;

	/* Reject every parent-directory component in either separator spelling. */
	for (;;) {
		if (*cursor == '/' ||
		    *cursor == '\\' ||
		    *cursor == '\0') {
			length = (size_t)(cursor - component);
			if (length == 2 &&
			    component[0] == '.' &&
			    component[1] == '.') {
				return false;
			}
			if (*cursor == '\0')
				break;
			component = cursor + 1;
		}
		cursor++;
	}

	return true;
}

/* Check whether one bytecode record already has a portable source identity. */
static bool
bcback_app_keep_embedded_source(
	const struct bytecode_file_module *module)
{
	uint32_t i;

	if (!bcback_app_source_is_portable(module->source))
		return false;

	/* Require one identical source identity in every function record. */
	for (i = 0; i < module->function_count; i++) {
		if (module->function[i].source == NULL)
			return false;
		if (strcmp(module->function[i].source, module->source) != 0)
			return false;
	}

	return true;
}

/* Compile and serialize one source-backed application module. */
static bool
bcback_app_prepare_source(
	const struct bcback_app_module *input,
	const char *temporary_base,
	struct bcback_app_blob *blob)
{
	struct bcback_module module;
	bool succeeded;

	memset(&module, 0, sizeof(module));

	succeeded = bcback_build_module(
		input->logical_source,
		input->source_text,
		&module);
	if (succeeded) {
		succeeded = bcback_serialize_module(
			&module,
			temporary_base,
			&blob->data,
			&blob->size);
	}

	bcback_cleanup_module(&module);

	return succeeded;
}

/* Validate and canonically serialize one bytecode-backed app module. */
static bool
bcback_app_prepare_bytecode(
	const struct bcback_app_module *input,
	const char *temporary_base,
	struct bcback_app_blob *blob)
{
	struct bytecode_file_module module;
	struct bytecode_file_error error;
	const char *logical_source;
	bool succeeded;

	memset(&module, 0, sizeof(module));
	logical_source = input->logical_source;

	succeeded = bytecode_file_inspect_module(
		input->bytecode_data,
		(size_t)input->bytecode_size,
		&module,
		&error);
	if (succeeded && module.kind != BYTECODE_FILE_MODULE_1_1)
		succeeded = false;
	if (succeeded && bcback_app_keep_embedded_source(&module))
		logical_source = module.source;
	if (succeeded) {
		succeeded = bcback_app_serialize_inspected_module(
			&module,
			logical_source,
			temporary_base,
			&blob->data,
			&blob->size);
	}

	bytecode_file_cleanup_module(&module);

	return succeeded;
}

/* Convert and serialize one inspected module without exposing CLI types. */
static bool
bcback_app_serialize_inspected_module(
	const struct bytecode_file_module *module,
	const char *logical_source,
	const char *temporary_base,
	uint8_t **data,
	uint32_t *size)
{
	struct bcback_module canonical_module;
	struct lir_func *lir_function;
	struct lir_func **function_pointer;
	bool rewrite_source;
	bool succeeded;
	uint32_t i;

	memset(&canonical_module, 0, sizeof(canonical_module));
	lir_function = NULL;
	function_pointer = NULL;
	succeeded = false;

	assert(module != NULL);
	assert(logical_source != NULL);
	assert(temporary_base != NULL);
	assert(data != NULL);
	assert(size != NULL);

	*data = NULL;
	*size = 0;

	if (module->kind != BYTECODE_FILE_MODULE_1_1)
		goto cleanup;
	if (!bcback_app_source_is_portable(logical_source))
		goto cleanup;

	rewrite_source = bcback_app_inspected_needs_rewrite(
		module,
		logical_source);
	if (rewrite_source && !bcback_app_inspected_safe_to_rewrite(module))
		goto cleanup;

	if (module->function_count > 0) {
		if (!bcback_app_array_size_valid(
			module->function_count,
			sizeof(*lir_function))) {
			goto cleanup;
		}

		lir_function = noct_calloc(
			(size_t)module->function_count,
			sizeof(*lir_function));
		if (lir_function == NULL)
			goto cleanup;

		if (!bcback_app_array_size_valid(
			module->function_count,
			sizeof(*function_pointer))) {
			goto cleanup;
		}

		function_pointer = noct_calloc(
			(size_t)module->function_count,
			sizeof(*function_pointer));
		if (function_pointer == NULL)
			goto cleanup;
	}

	canonical_module.source = (char *)logical_source;
	canonical_module.require_count = module->require_count;
	canonical_module.require_name = module->require_name;
	canonical_module.function_count = module->function_count;
	canonical_module.function = function_pointer;

	/* Convert every CLI descriptor to the backend's file-independent LIR. */
	for (i = 0; i < module->function_count; i++) {
		if (!bcback_app_build_inspected_function(
			&module->function[i],
			logical_source,
			module->source,
			rewrite_source,
			&lir_function[i])) {
			goto cleanup;
		}

		function_pointer[i] = &lir_function[i];
	}

	succeeded = bcback_serialize_module(
		&canonical_module,
		temporary_base,
		data,
		size);

cleanup:
	bcback_app_cleanup_inspected_functions(module, lir_function);
	noct_free(function_pointer);
	noct_free(lir_function);

	return succeeded;
}

/* Convert one inspected function to a borrowed LIR descriptor. */
static bool
bcback_app_build_inspected_function(
	const struct bytecode_file_function *function,
	const char *logical_source,
	const char *module_source,
	bool rewrite_source,
	struct lir_func *lir_function)
{
	char *rewritten_name;
	size_t source_size;
	uint32_t i;

	rewritten_name = NULL;

	assert(function != NULL);
	assert(logical_source != NULL);
	assert(module_source != NULL);
	assert(lir_function != NULL);

	if (function->param_count > LIR_PARAM_SIZE)
		return false;

	if (rewrite_source && strncmp(function->name, "$init.", 6) == 0) {
		if (strcmp(function->name + 6, module_source) != 0)
			return false;

		source_size = strlen(logical_source);
		if (source_size > SIZE_MAX - 7)
			return false;

		rewritten_name = noct_malloc(source_size + 7);
		if (rewritten_name == NULL)
			return false;

		memcpy(rewritten_name, "$init.", 6);
		memcpy(
			rewritten_name + 6,
			logical_source,
			source_size + 1);
		lir_function->func_name = rewritten_name;
	} else {
		lir_function->func_name = function->name;
	}

	lir_function->file_name = (char *)logical_source;
	lir_function->param_count = function->param_count;

	/* Copy every inspected parameter into the fixed LIR descriptor. */
	for (i = 0; i < function->param_count; i++) {
		lir_function->param_name[i] = function->param_name[i];
		lir_function->param_type[i] = function->param_type[i];
		lir_function->param_packed_type[i] =
			function->param_packed_type[i];
		lir_function->param_restricted[i] =
			function->param_restricted[i];
	}

	lir_function->return_type = function->return_type;
	lir_function->return_packed_type = function->return_packed_type;
	lir_function->return_type_checked = function->return_type_checked;
	lir_function->has_vector_ops = function->has_vector_ops;
	lir_function->has_fma_ops = function->has_fma_ops;
	lir_function->is_fast = false;
	lir_function->fast_info = NULL;
#if defined(NOCT_USE_OPTIMIZER)
	lir_function->is_fast = function->is_fast;
	lir_function->fast_info = function->fast_info;
#endif
	lir_function->tmpvar_size = function->tmpvar_size;
	lir_function->bytecode = (uint8_t *)function->bytecode.data;
	lir_function->bytecode_size = function->bytecode.size;

	return true;
}

/* Release names allocated while converting inspected functions. */
static void
bcback_app_cleanup_inspected_functions(
	const struct bytecode_file_module *module,
	struct lir_func lir_function[])
{
	uint32_t i;

	if (lir_function == NULL)
		return;

	/* Release only initializer names rewritten for the portable source. */
	for (i = 0; i < module->function_count; i++) {
		if (lir_function[i].func_name != module->function[i].name)
			noct_free(lir_function[i].func_name);
	}
}

/* Search one bytecode span for a fixed non-empty ASCII token. */
static bool
bcback_app_contains_bytes(
	const uint8_t *data,
	uint32_t size,
	const char *needle)
{
	size_t needle_size;
	uint32_t i;

	needle_size = strlen(needle);
	if (needle_size == 0)
		return true;
	if ((size_t)size < needle_size)
		return false;

	/* Compare the token at every possible byte offset. */
	for (i = 0; (size_t)i <= (size_t)size - needle_size; i++) {
		if (memcmp(data + i, needle, needle_size) == 0)
			return true;
	}

	return false;
}

/* Test whether any serialized source identity needs canonicalization. */
static bool
bcback_app_inspected_needs_rewrite(
	const struct bytecode_file_module *module,
	const char *logical_source)
{
	uint32_t i;

	if (strcmp(module->source, logical_source) != 0)
		return true;

	/* Compare every function source with the portable module identity. */
	for (i = 0; i < module->function_count; i++) {
		if (strcmp(module->function[i].source, logical_source) != 0)
			return true;
	}

	return false;
}

/* Reject source rewrites that would require a bytecode symbol relinker. */
static bool
bcback_app_inspected_safe_to_rewrite(
	const struct bytecode_file_module *module)
{
	const struct bytecode_file_function *function;
	uint32_t i;

	/* Inspect every link name and raw opcode span conservatively. */
	for (i = 0; i < module->function_count; i++) {
		function = &module->function[i];
		if (strstr(function->name, "$static.") != NULL)
			return false;
		if (bcback_app_contains_bytes(
			function->bytecode.data,
			function->bytecode.size,
			"$static.")) {
			return false;
		}
		if (strncmp(function->name, "$init.", 6) == 0 &&
		    strcmp(function->name + 6, module->source) != 0) {
			return false;
		}
	}

	return true;
}

/* Release every prepared module blob. */
static void
bcback_app_cleanup_blobs(
	uint32_t count,
	struct bcback_app_blob blob[])
{
	uint32_t i;

	if (blob == NULL)
		return;

	/* Release each owned canonical module record. */
	for (i = 0; i < count; i++)
		noct_free(blob[i].data);
}

/* Write one raw app container without a shebang. */
static bool
bcback_app_write_stream(
	FILE *stream,
	uint32_t module_count,
	const struct bcback_app_blob module[],
	uint32_t binding_count,
	const struct bcback_app_binding binding[],
	uint32_t root_count,
	const uint32_t root[])
{
	uint32_t i;

	if (fprintf(
		stream,
		"Noct App 1.0\nNumber Of Modules\n%u\n"
		"Number Of Bindings\n%u\n",
		module_count,
		binding_count) < 0) {
		return false;
	}

	/* Write every require alias and its serialized module index. */
	for (i = 0; i < binding_count; i++) {
		if (binding[i].module_name == NULL ||
		    binding[i].module_name[0] == '\0') {
			return false;
		}
		if (strchr(binding[i].module_name, '\n') != NULL ||
		    strchr(binding[i].module_name, '\r') != NULL) {
			return false;
		}
		if (fprintf(
			stream,
			"%s\n%u\n",
			binding[i].module_name,
			binding[i].module_index) < 0) {
			return false;
		}
	}

	if (fprintf(
		stream,
		"Number Of Roots\n%u\n",
		root_count) < 0) {
		return false;
	}

	/* Write every explicit root index in command-line order. */
	for (i = 0; i < root_count; i++) {
		if (fprintf(stream, "%u\n", root[i]) < 0)
			return false;
	}

	if (fprintf(stream, "Modules\n") < 0)
		return false;

	/* Write each independently framed canonical 1.1 module record. */
	for (i = 0; i < module_count; i++) {
		if (module[i].data == NULL || module[i].size == 0)
			return false;
		if (fprintf(
			stream,
			"Module Bytecode Size\n%u\n",
			module[i].size) < 0) {
			return false;
		}
		if (fwrite(
			module[i].data,
			1,
			module[i].size,
			stream) != module[i].size) {
			return false;
		}
	}

	if (fprintf(stream, "End App\n") < 0)
		return false;

	return ferror(stream) == 0;
}

/* Copy one raw app stream into a checked owned byte blob. */
static bool
bcback_app_stream_to_blob(
	FILE *stream,
	uint8_t **data,
	uint32_t *size)
{
	long stream_size;
	size_t read_size;
	uint32_t blob_size;

	if (fflush(stream) != 0)
		return false;
	if (ferror(stream) != 0)
		return false;

	stream_size = ftell(stream);
	if (stream_size <= 0)
		return false;
	if (fseek(stream, 0, SEEK_SET) != 0)
		return false;

	read_size = (size_t)stream_size;
	if ((long)read_size != stream_size)
		return false;
	blob_size = (uint32_t)read_size;
	if ((size_t)blob_size != read_size)
		return false;

	*data = noct_malloc(read_size);
	if (*data == NULL)
		return false;
	if (fread(*data, 1, read_size, stream) != read_size)
		return false;

	*size = blob_size;

	return true;
}

/* Assemble one complete raw app blob for strict preflight validation. */
static bool
bcback_app_build_blob(
	const char *temporary_base,
	uint32_t module_count,
	const struct bcback_app_blob module[],
	uint32_t binding_count,
	const struct bcback_app_binding binding[],
	uint32_t root_count,
	const uint32_t root[],
	uint8_t **data,
	uint32_t *size)
{
	struct bcback_output output;
	FILE *stream;
	bool succeeded;

	memset(&output, 0, sizeof(output));
	*data = NULL;
	*size = 0;

	if (!bcback_output_open(&output, temporary_base))
		return false;
	stream = bcback_output_get_stream(&output);

	succeeded = bcback_app_write_stream(
		stream,
		module_count,
		module,
		binding_count,
		binding,
		root_count,
		root);
	if (succeeded)
		succeeded = bcback_app_stream_to_blob(stream, data, size);
	bcback_output_abort(&output);

	if (!succeeded) {
		noct_free(*data);
		*data = NULL;
		*size = 0;
	}

	return succeeded;
}
