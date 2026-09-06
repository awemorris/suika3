/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Bytecode backend.
 */

#include <noct/noct.h>

#include "ast.h"
#include "bcback_file.h"
#include "bcback_private.h"
#if defined(NOCT_USE_OPTIMIZER)
#include "fast.h"
#endif
#include "hir.h"
#include "lir.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum bcback_func_kind {
	BCBACK_FUNC_NORMAL = 0,
	BCBACK_FUNC_FAST = 1
};

enum bcback_state {
	BCBACK_IDLE,
	BCBACK_TEMP_OPEN,
	BCBACK_FAILED
};

static int bcback_optimize_level = 1;
static bool bcback_lineinfo = true;
static bool bcback_simd_info;
static enum bcback_state bcback_current_state;
static bool bcback_translated;
static struct bcback_output bcback_current_output;

static bool bcback_metadata_valid(const char *text, bool allow_empty);
static bool bcback_array_size_valid(uint32_t count, size_t item_size);
static bool bcback_write_header(FILE *stream, const char *source, uint32_t require_count, char *const require_name[], uint32_t function_count);
#if defined(NOCT_USE_OPTIMIZER)
static bool bcback_write_fast_signature(FILE *stream, const struct fast_signature *signature);
#endif
static bool bcback_write_lir_function(FILE *stream, const struct lir_func *function);
static bool bcback_write_module_stream(FILE *stream, const struct bcback_module *module);
static bool bcback_stream_to_blob(FILE *stream, uint8_t **data, uint32_t *size);

/*
 * Sets the optimization level used by subsequent bytecode translations.
 */
NOCT_DLL
void
noct_bcback_set_optimize_level(
	int level)
{
	bcback_optimize_level = level;
	bcback_lineinfo = level == 0;
}

/*
 * Selects whether subsequent bytecode contains source-line metadata.
 */
NOCT_DLL
void
noct_bcback_set_lineinfo(
	bool enable)
{
	bcback_lineinfo = enable;
}

/*
 * Selects vectorization diagnostics for subsequent bytecode translations.
 */
NOCT_DLL
void
noct_bcback_set_simd_info(
	bool enable)
{
	bcback_simd_info = enable;
}

/*
 * Starts one transactional standalone bytecode output.
 */
NOCT_DLL
bool
noct_bcback_start(
	const char *out_file_name)
{
	if (bcback_current_state != BCBACK_IDLE)
		return false;

	if (!bcback_output_open(&bcback_current_output, out_file_name)) {
		printf(N_TR("Failed to open file \"%s\".\n"), out_file_name);
		return false;
	}

	bcback_current_state = BCBACK_TEMP_OPEN;
	bcback_translated = false;

	return true;
}

/*
 * Translates one source module into the open standalone output.
 */
NOCT_DLL
bool
noct_bcback_translate(
	const char *source_file_name,
	const char *source_data)
{
	struct bcback_module module;
	FILE *stream;
	bool succeeded;

	memset(&module, 0, sizeof(module));

	if (bcback_current_state != BCBACK_TEMP_OPEN)
		return false;
	if (bcback_translated)
		return false;

	succeeded = bcback_build_module(
		source_file_name,
		source_data,
		&module);
	if (succeeded) {
		stream = bcback_output_get_stream(&bcback_current_output);
		succeeded = bcback_write_module_stream(stream, &module);
	}

	bcback_cleanup_module(&module);

	if (!succeeded) {
		bcback_current_state = BCBACK_FAILED;
		return false;
	}

	bcback_translated = true;

	return true;
}

/*
 * Commits one complete standalone bytecode output.
 */
NOCT_DLL
bool
noct_bcback_finalize(
	void)
{
	bool succeeded;

	if (bcback_current_state == BCBACK_FAILED) {
		bcback_abort();
		return false;
	}
	if (bcback_current_state != BCBACK_TEMP_OPEN)
		return false;
	if (!bcback_translated) {
		bcback_abort();
		return false;
	}

	succeeded = bcback_output_commit(&bcback_current_output);
	bcback_current_state = BCBACK_IDLE;
	bcback_translated = false;

	return succeeded;
}

/*
 * Builds one detached CPU bytecode module from source text.
 */
bool
bcback_build_module(
	const char *source_name,
	const char *source_text,
	struct bcback_module *module)
{
	struct hir_block *hir_function;
	struct lir_func *lir_function;
	uint32_t i;
	bool hir_started;
	bool succeeded;

	assert(module != NULL);

	memset(module, 0, sizeof(*module));
	hir_started = false;
	succeeded = false;

	if (!bcback_metadata_valid(source_name, false)) {
		printf(N_TR("Error: Invalid bytecode source name.\n"));
		return false;
	}
	if (source_text == NULL)
		return false;

	if (!ast_build(source_name, source_text)) {
		printf(
			N_TR("Error: %s:%d: %s\n"),
			ast_get_file_name(),
			ast_get_error_line(),
			ast_get_error_message());
		ast_cleanup();
		return false;
	}

	module->source = noct_strdup(source_name);
	if (module->source == NULL)
		goto cleanup;

	module->require_count = ast_get_require_count();
	if (module->require_count > 0) {
		if (!bcback_array_size_valid(
			module->require_count,
			sizeof(*module->require_name))) {
			goto cleanup;
		}

		module->require_name = noct_calloc(
			(size_t)module->require_count,
			sizeof(*module->require_name));
		if (module->require_name == NULL)
			goto cleanup;
	}

	/* Retain require names before the AST arena is released. */
	for (i = 0; i < module->require_count; i++) {
		module->require_name[i] = noct_strdup(ast_get_require_name(i));
		if (module->require_name[i] == NULL)
			goto cleanup;
	}

	hir_started = true;
	if (!hir_build()) {
		printf(
			N_TR("Error: %s:%d: %s\n"),
			hir_get_file_name(),
			hir_get_error_line(),
			hir_get_error_message());
		goto cleanup;
	}
	module->function_count = hir_get_function_count();
	if (module->function_count > 0) {
		if (!bcback_array_size_valid(
			module->function_count,
			sizeof(*module->function))) {
			goto cleanup;
		}

		module->function = noct_calloc(
			(size_t)module->function_count,
			sizeof(*module->function));
		if (module->function == NULL)
			goto cleanup;
	}

	lir_set_optimize_level(bcback_optimize_level);
	lir_set_lineinfo(bcback_lineinfo);

	/* Lower every HIR function into an independently owned LIR function. */
	for (i = 0; i < module->function_count; i++) {
		hir_function = hir_get_function(i);
		if (!hir_optimize_func(
			hir_function,
			bcback_optimize_level,
			bcback_simd_info,
			NULL,
			NULL)) {
			printf(N_TR("Error: %s\n"), hir_get_error_message());
			goto cleanup;
		}

		lir_function = NULL;
		if (!lir_build(hir_function, &lir_function)) {
			printf(
				N_TR("Error: %s:%d: %s\n"),
				lir_get_file_name(),
				lir_get_error_line(),
				lir_get_error_message());
			goto cleanup;
		}
		module->function[i] = lir_function;
	}

	succeeded = true;

cleanup:
	if (hir_started)
		hir_cleanup();
	ast_cleanup();

	if (!succeeded)
		bcback_cleanup_module(module);

	return succeeded;
}

/*
 * Releases one detached CPU bytecode module.
 */
void
bcback_cleanup_module(
	struct bcback_module *module)
{
	uint32_t i;

	if (module == NULL)
		return;

	/* Release every detached LIR function. */
	if (module->function != NULL) {
		/* Release each successfully lowered function exactly once. */
		for (i = 0; i < module->function_count; i++) {
			if (module->function[i] != NULL)
				lir_cleanup(module->function[i]);
		}
	}
	noct_free(module->function);

	/* Release every copied require name. */
	if (module->require_name != NULL) {
		/* Release each name copied before any partial failure. */
		for (i = 0; i < module->require_count; i++)
			noct_free(module->require_name[i]);
	}
	noct_free(module->require_name);
	noct_free(module->source);

	memset(module, 0, sizeof(*module));
}

/*
 * Serializes one detached source module into an owned canonical 1.1 blob.
 */
bool
bcback_serialize_module(
	const struct bcback_module *module,
	const char *temporary_base,
	uint8_t **data,
	uint32_t *size)
{
	struct bcback_output output;
	FILE *stream;
	bool succeeded;

	memset(&output, 0, sizeof(output));

	assert(module != NULL);
	assert(temporary_base != NULL);
	assert(data != NULL);
	assert(size != NULL);

	*data = NULL;
	*size = 0;

	if (!bcback_output_open(&output, temporary_base))
		return false;
	stream = bcback_output_get_stream(&output);

	succeeded = bcback_write_module_stream(stream, module);
	if (succeeded)
		succeeded = bcback_stream_to_blob(stream, data, size);
	bcback_output_abort(&output);

	if (!succeeded) {
		noct_free(*data);
		*data = NULL;
		*size = 0;
	}

	return succeeded;
}

/*
 * Aborts the current standalone output transaction.
 */
void
bcback_abort(
	void)
{
	bcback_output_abort(&bcback_current_output);
	bcback_current_state = BCBACK_IDLE;
	bcback_translated = false;
}

/* Check one string against the unescaped metadata-line contract. */
static bool
bcback_metadata_valid(
	const char *text,
	bool allow_empty)
{
	size_t length;

	if (text == NULL)
		return false;
	if (!allow_empty && text[0] == '\0')
		return false;

	length = strlen(text);
	if (length >= 1024)
		return false;
	if (strchr(text, '\n') != NULL)
		return false;
	if (strchr(text, '\r') != NULL)
		return false;

	return true;
}

/* Check one count-sized table without overflowing size_t. */
static bool
bcback_array_size_valid(
	uint32_t count,
	size_t item_size)
{
	if (count == 0)
		return true;
	if (item_size > SIZE_MAX / (size_t)count)
		return false;

	return true;
}

/* Write the canonical 1.1 module header and require list. */
static bool
bcback_write_header(
	FILE *stream,
	const char *source,
	uint32_t require_count,
	char *const require_name[],
	uint32_t function_count)
{
	uint32_t i;

	if (!bcback_metadata_valid(source, false))
		return false;
	if (require_count > 0 && require_name == NULL)
		return false;

	if (fprintf(
		stream,
		"Noct Bytecode 1.1\nSource\n%s\nNumber Of Requires\n%u\n",
		source,
		require_count) < 0) {
		return false;
	}

	/* Write require names in their source declaration order. */
	for (i = 0; i < require_count; i++) {
		if (!bcback_metadata_valid(require_name[i], false))
			return false;
		if (fprintf(stream, "%s\n", require_name[i]) < 0)
			return false;
	}

	if (fprintf(
		stream,
		"Number Of Functions\n%u\n",
		function_count) < 0) {
		return false;
	}

	return true;
}

/* Write one exact sparse fast-function signature. */
#if defined(NOCT_USE_OPTIMIZER)
static bool
bcback_write_fast_signature(
	FILE *stream,
	const struct fast_signature *signature)
{
	const struct fast_param_contract *parameter;
	const struct fast_extent *extent;
	uint32_t parameter_index;
	uint32_t axis;

	if (!signature->valid)
		return false;
	if (signature->version != NOCT_FAST_SIGNATURE_VERSION)
		return false;
	if (!fast_signature_valid(signature))
		return false;
	if (signature->param_count != 0 && signature->param == NULL)
		return false;

	if (fprintf(
		stream,
		"Fast Signature\n%u\n%d\n%u\n%d\n",
		signature->version,
		signature->valid ? 1 : 0,
		signature->param_count,
		signature->return_type) < 0) {
		return false;
	}

	/* Write every parameter contract in declaration order. */
	for (parameter_index = 0;
	     parameter_index < signature->param_count;
	     parameter_index++) {
		parameter = &signature->param[parameter_index];
		if (parameter->rank > NOCT_FAST_RANK_MAX)
			return false;
		if (parameter->rank != 0 && parameter->extent == NULL)
			return false;

		if (fprintf(
			stream,
			"%d\n%d\n%d\n%u\n",
			parameter->value_type,
			parameter->packed_type,
			parameter->restricted ? 1 : 0,
			parameter->rank) < 0) {
			return false;
		}

		/* Write the exact expression for every shaped axis. */
		for (axis = 0; axis < parameter->rank; axis++) {
			extent = &parameter->extent[axis];
			if (fprintf(stream, "%d\n", extent->kind) < 0)
				return false;

			if (extent->kind == FAST_EXTENT_CONST) {
				if (fprintf(
					stream,
					"%lld\n",
					(long long)extent->value.constant) < 0) {
					return false;
				}
			} else if (extent->kind == FAST_EXTENT_PARAM) {
				if (fprintf(
					stream,
					"%u\n",
					extent->value.param_index) < 0) {
					return false;
				}
			} else {
				return false;
			}
		}
	}

	return true;
}
#endif

/* Write one canonical function from an owned LIR descriptor. */
static bool
bcback_write_lir_function(
	FILE *stream,
	const struct lir_func *function)
{
	uint32_t i;
	bool any;

	if (!bcback_metadata_valid(function->func_name, false))
		return false;
	if (!bcback_metadata_valid(function->file_name, false))
		return false;
	if (function->param_count > NOCT_ARG_MAX)
		return false;

	if (fprintf(
		stream,
		"Begin Function\nName\n%s\nSource\n%s\nParameters\n%u\n",
		function->func_name,
		function->file_name,
		function->param_count) < 0) {
		return false;
	}

	/* Write every parameter name in declaration order. */
	for (i = 0; i < function->param_count; i++) {
		if (!bcback_metadata_valid(function->param_name[i], false))
			return false;
		if (fprintf(stream, "%s\n", function->param_name[i]) < 0)
			return false;
	}

	any = false;

	/* Detect whether ordinary parameter types carry any information. */
	for (i = 0; i < function->param_count; i++) {
		if (function->param_type[i] >= 0)
			any = true;
	}
	if (any) {
		if (fprintf(stream, "Parameter Types\n") < 0)
			return false;

		/* Write one ordinary type for every parameter. */
		for (i = 0; i < function->param_count; i++) {
			if (fprintf(stream, "%d\n", function->param_type[i]) < 0)
				return false;
		}
	}

	any = false;

	/* Detect whether Packed parameter types carry any information. */
	for (i = 0; i < function->param_count; i++) {
		if (function->param_packed_type[i] >= 0)
			any = true;
	}
	if (any) {
		if (fprintf(stream, "Parameter Packed Types\n") < 0)
			return false;

		/* Write one Packed type for every parameter. */
		for (i = 0; i < function->param_count; i++) {
			if (fprintf(
				stream,
				"%d\n",
				function->param_packed_type[i]) < 0) {
				return false;
			}
		}
	}

	any = false;

	/* Detect whether any parameter carries the restricted contract. */
	for (i = 0; i < function->param_count; i++) {
		if (function->param_restricted[i])
			any = true;
	}
	if (any) {
		if (fprintf(stream, "Parameter Restricted\n") < 0)
			return false;

		/* Write one restricted flag for every parameter. */
		for (i = 0; i < function->param_count; i++) {
			if (fprintf(
				stream,
				"%d\n",
				function->param_restricted[i] ? 1 : 0) < 0) {
				return false;
			}
		}
	}

	if (function->return_type >= 0 || function->is_fast) {
		if (fprintf(
			stream,
			"Return Type\n%d\n%d\n%d\n",
			function->return_type,
			function->return_packed_type,
			function->return_type_checked ? 1 : 0) < 0) {
			return false;
		}
	}
	if (function->has_vector_ops) {
		if (fprintf(stream, "Vector Ops\n1\n") < 0)
			return false;
	}
	if (function->is_fast) {
#if defined(NOCT_USE_OPTIMIZER)
		const struct fast_signature *signature;

		signature = fast_info_signature(function->fast_info);
		if (signature == NULL)
			return false;
		if (fprintf(
			stream,
			"Function Kind\n%d\n",
			BCBACK_FUNC_FAST) < 0) {
			return false;
		}
		if (!bcback_write_fast_signature(
			stream,
			signature)) {
			return false;
		}
#else
		return false;
#endif
	}
	if (function->has_fma_ops) {
		if (fprintf(stream, "FMA Ops\n1\n") < 0)
			return false;
	}

	if (fprintf(
		stream,
		"Temporary Size\n%u\nBytecode Size\n%u\n",
		function->tmpvar_size,
		function->bytecode_size) < 0) {
		return false;
	}
	if (function->bytecode_size != 0) {
		if (fwrite(
			function->bytecode,
			1,
			function->bytecode_size,
			stream) != function->bytecode_size) {
			return false;
		}
	}
	if (fprintf(stream, "\nEnd Function\n") < 0)
		return false;

	return true;
}

/* Write one detached source module in canonical 1.1 form. */
static bool
bcback_write_module_stream(
	FILE *stream,
	const struct bcback_module *module)
{
	uint32_t i;

	if (!bcback_write_header(
		stream,
		module->source,
		module->require_count,
		module->require_name,
		module->function_count)) {
		return false;
	}

	/* Write every lowered function in source order. */
	for (i = 0; i < module->function_count; i++) {
		if (!bcback_write_lir_function(stream, module->function[i]))
			return false;
	}

	return ferror(stream) == 0;
}

/* Copy one complete temporary stream into a checked owned byte blob. */
static bool
bcback_stream_to_blob(
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
