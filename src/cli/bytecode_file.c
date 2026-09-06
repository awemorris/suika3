/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-private inspection of Noct bytecode files.
 */

#include "bytecode_file.h"

#if defined(NOCT_USE_OPTIMIZER)
#include "fast.h"
#endif
#include "lir.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define BYTECODE_FILE_LINE_SIZE		1024
#define BYTECODE_FILE_MODULE_PREFIX	"Noct Bytecode "
#define BYTECODE_FILE_MODULE_1_0_MAGIC	"Noct Bytecode 1.0\n"
#define BYTECODE_FILE_MODULE_1_1_MAGIC	"Noct Bytecode 1.1\n"
#define BYTECODE_FILE_APP_PREFIX	"Noct App "
#define BYTECODE_FILE_APP_1_0_MAGIC	"Noct App 1.0\n"
#define BYTECODE_FILE_MAX_RECORD_COUNT	4096U
#define BYTECODE_FILE_FAST_SIGNATURE_VERSION	1U
#define BYTECODE_FILE_FAST_RANK_MAX	8U
#define BYTECODE_FILE_FAST_EXTENT_CONST	1
#define BYTECODE_FILE_FAST_EXTENT_PARAM	2
#define BYTECODE_FILE_FAST_RETURN_VOID	(-2)

enum bytecode_file_optional_section {
	BYTECODE_FILE_PARAM_TYPES = 1U << 0,
	BYTECODE_FILE_PARAM_PACKED_TYPES = 1U << 1,
	BYTECODE_FILE_PARAM_RESTRICTED = 1U << 2,
	BYTECODE_FILE_RETURN_TYPE = 1U << 3,
	BYTECODE_FILE_VECTOR_OPS = 1U << 4,
	BYTECODE_FILE_FMA_OPS = 1U << 5,
	BYTECODE_FILE_FUNCTION_KIND = 1U << 6,
	BYTECODE_FILE_FAST_SIGNATURE = 1U << 7
};

struct bytecode_file_parser {
	const uint8_t *data;
	size_t size;
	size_t pos;
	size_t limit;
	char line[BYTECODE_FILE_LINE_SIZE];
	struct bytecode_file_error *error;
};

static bool bytecode_file_has_prefix(const uint8_t *data, size_t size, const char *prefix);
static void bytecode_file_reset_error(struct bytecode_file_error *error);
static bool bytecode_file_fail(struct bytecode_file_parser *parser, const char *message);
static bool bytecode_file_read_line(struct bytecode_file_parser *parser);
static bool bytecode_file_expect_line(struct bytecode_file_parser *parser, const char *expected);
static bool bytecode_file_parse_u32(const char *text, uint32_t *value);
static bool bytecode_file_parse_i64(const char *text, int64_t *value);
static bool bytecode_file_parse_int(const char *text, int *value);
static bool bytecode_file_count_valid(struct bytecode_file_parser *parser, uint32_t count, size_t minimum_size);
static void *bytecode_file_allocate_array(uint32_t count, size_t item_size);
static bool bytecode_file_copy_text(const char *text, char **copy);
static bool bytecode_file_read_text(struct bytecode_file_parser *parser, bool allow_empty, char **text);
static bool bytecode_file_identifier(const char *text);
static bool bytecode_file_read_identifier(struct bytecode_file_parser *parser, char **identifier);
static void bytecode_file_cleanup_function(struct bytecode_file_function *function);
static bool bytecode_file_parse_fast_signature(struct bytecode_file_parser *parser, struct bytecode_file_function *function);
static bool bytecode_file_validate_fast_metadata(const struct bytecode_file_function *function, unsigned int sections);
static unsigned int bytecode_file_get_optional_section(const char *line);
static bool bytecode_file_parse_param_section(struct bytecode_file_parser *parser, struct bytecode_file_function *function, unsigned int section);
static bool bytecode_file_parse_return_section(struct bytecode_file_parser *parser, struct bytecode_file_function *function);
static bool bytecode_file_parse_flag_section(struct bytecode_file_parser *parser, bool *flag);
static bool bytecode_file_parse_kind_section(struct bytecode_file_parser *parser, struct bytecode_file_function *function);
static bool bytecode_file_parse_function(struct bytecode_file_parser *parser, const char *module_source, struct bytecode_file_function *function);
static bool bytecode_file_parse_module(struct bytecode_file_parser *parser, size_t limit, enum bytecode_file_kind required_kind, struct bytecode_file_module *module);
static bool bytecode_file_find_binding(const struct bytecode_file_app *app, const char *name, uint32_t *module_index);
static bool bytecode_file_function_name_seen(const struct bytecode_file_app *app, uint32_t module_index, uint32_t function_index);
static bool bytecode_file_module_is_root(const struct bytecode_file_app *app, uint32_t module_index);
static bool bytecode_file_validate_app_directory(struct bytecode_file_parser *parser, const struct bytecode_file_app *app);
static bool bytecode_file_validate_app_symbols(struct bytecode_file_parser *parser, const struct bytecode_file_app *app);
static bool bytecode_file_validate_app_cycle(struct bytecode_file_parser *parser, const struct bytecode_file_app *app);
static bool bytecode_file_validate_app_reachability(struct bytecode_file_parser *parser, const struct bytecode_file_app *app);
static bool bytecode_file_validate_app(struct bytecode_file_parser *parser, const struct bytecode_file_app *app);
static bool bytecode_file_parse_app(struct bytecode_file_parser *parser, struct bytecode_file_app *app);

/*
 * Detects the bytecode file family and known version.
 */
enum bytecode_file_kind
bytecode_file_detect(
	const uint8_t *data,
	size_t size)
{
	if (bytecode_file_has_prefix(
		data,
		size,
		BYTECODE_FILE_MODULE_1_0_MAGIC)) {
		return BYTECODE_FILE_MODULE_1_0;
	}
	if (bytecode_file_has_prefix(
		data,
		size,
		BYTECODE_FILE_MODULE_1_1_MAGIC)) {
		return BYTECODE_FILE_MODULE_1_1;
	}
	if (bytecode_file_has_prefix(
		data,
		size,
		BYTECODE_FILE_APP_1_0_MAGIC)) {
		return BYTECODE_FILE_APP_1_0;
	}
	if (bytecode_file_has_prefix(
		data,
		size,
		BYTECODE_FILE_MODULE_PREFIX)) {
		return BYTECODE_FILE_MODULE_UNKNOWN;
	}
	if (bytecode_file_has_prefix(
		data,
		size,
		BYTECODE_FILE_APP_PREFIX)) {
		return BYTECODE_FILE_APP_UNKNOWN;
	}

	return BYTECODE_FILE_UNKNOWN;
}

/*
 * Checks whether an artifact size fits the public registration interface.
 */
bool
bytecode_file_check_registration_size(
	size_t size,
	uint32_t *size_out)
{
	assert(size_out != NULL);

	if (size == 0)
		return false;

	*size_out = (uint32_t)size;
	if ((size_t)*size_out != size)
		return false;

	return true;
}

/*
 * Inspects one raw Noct bytecode module.
 */
bool
bytecode_file_inspect_module(
	const uint8_t *data,
	size_t size,
	struct bytecode_file_module *module,
	struct bytecode_file_error *error)
{
	struct bytecode_file_parser parser;
	enum bytecode_file_kind kind;

	assert(module != NULL);
	assert(error != NULL);

	memset(module, 0, sizeof(*module));
	bytecode_file_reset_error(error);

	if (data == NULL) {
		error->offset = 0;
		strcpy(error->message, "Bytecode data is null.");
		return false;
	}

	kind = bytecode_file_detect(data, size);
	if (kind != BYTECODE_FILE_MODULE_1_0 &&
	    kind != BYTECODE_FILE_MODULE_1_1) {
		error->offset = 0;
		strcpy(error->message, "Unsupported or malformed bytecode version.");
		return false;
	}

	parser.data = data;
	parser.size = size;
	parser.pos = 0;
	parser.limit = size;
	parser.line[0] = '\0';
	parser.error = error;

	if (!bytecode_file_parse_module(&parser, size, kind, module)) {
		bytecode_file_cleanup_module(module);
		return false;
	}

	return true;
}

/*
 * Inspects one raw Noct application container.
 */
bool
bytecode_file_inspect_app(
	const uint8_t *data,
	size_t size,
	struct bytecode_file_app *app,
	struct bytecode_file_error *error)
{
	struct bytecode_file_parser parser;

	assert(app != NULL);
	assert(error != NULL);

	memset(app, 0, sizeof(*app));
	bytecode_file_reset_error(error);

	if (data == NULL) {
		error->offset = 0;
		strcpy(error->message, "Application data is null.");
		return false;
	}
	if (bytecode_file_detect(data, size) != BYTECODE_FILE_APP_1_0) {
		error->offset = 0;
		strcpy(error->message, "Unsupported or malformed application version.");
		return false;
	}

	parser.data = data;
	parser.size = size;
	parser.pos = 0;
	parser.limit = size;
	parser.line[0] = '\0';
	parser.error = error;

	if (!bytecode_file_parse_app(&parser, app)) {
		bytecode_file_cleanup_app(app);
		return false;
	}

	return true;
}

/*
 * Releases an inspected bytecode module.
 */
void
bytecode_file_cleanup_module(
	struct bytecode_file_module *module)
{
	uint32_t i;

	if (module == NULL)
		return;

	noct_free(module->source);

	/* Release every owned require name. */
	if (module->require_name != NULL) {
		/* Release names allocated before any partial parse failure. */
		for (i = 0; i < module->require_count; i++)
			noct_free(module->require_name[i]);
	}
	noct_free(module->require_name);

	/* Release every owned function descriptor. */
	if (module->function != NULL) {
		/* Release descriptors allocated before any partial parse failure. */
		for (i = 0; i < module->function_count; i++)
			bytecode_file_cleanup_function(&module->function[i]);
	}
	noct_free(module->function);

	memset(module, 0, sizeof(*module));
}

/*
 * Releases an inspected application container.
 */
void
bytecode_file_cleanup_app(
	struct bytecode_file_app *app)
{
	uint32_t i;

	if (app == NULL)
		return;

	/* Release every embedded module descriptor. */
	if (app->module != NULL) {
		/* Release records allocated before any partial parse failure. */
		for (i = 0; i < app->module_count; i++)
			bytecode_file_cleanup_module(&app->module[i]);
	}
	noct_free(app->module);

	/* Release every owned binding name. */
	if (app->binding != NULL) {
		/* Release names allocated before any partial parse failure. */
		for (i = 0; i < app->binding_count; i++)
			noct_free(app->binding[i].module_name);
	}
	noct_free(app->binding);

	noct_free(app->root_index);
	memset(app, 0, sizeof(*app));
}

/* Test whether an explicit byte span begins with a fixed string. */
static bool
bytecode_file_has_prefix(
	const uint8_t *data,
	size_t size,
	const char *prefix)
{
	size_t prefix_size;

	if (data == NULL || prefix == NULL)
		return false;

	prefix_size = strlen(prefix);
	if (size < prefix_size)
		return false;
	if (memcmp(data, prefix, prefix_size) != 0)
		return false;

	return true;
}

/* Clear a caller-owned diagnostic before one inspection. */
static void
bytecode_file_reset_error(
	struct bytecode_file_error *error)
{
	assert(error != NULL);

	error->offset = 0;
	error->message[0] = '\0';
}

/* Record the first precise parser failure. */
static bool
bytecode_file_fail(
	struct bytecode_file_parser *parser,
	const char *message)
{
	size_t length;

	assert(parser != NULL);
	assert(parser->error != NULL);
	assert(message != NULL);

	if (parser->error->message[0] != '\0')
		return false;

	parser->error->offset = parser->pos;
	length = strlen(message);
	if (length >= sizeof(parser->error->message))
		length = sizeof(parser->error->message) - 1;
	memcpy(parser->error->message, message, length);
	parser->error->message[length] = '\0';

	return false;
}

/* Read one LF-terminated metadata line without accepting CR or NUL. */
static bool
bytecode_file_read_line(
	struct bytecode_file_parser *parser)
{
	size_t length;
	uint8_t ch;

	assert(parser != NULL);

	length = 0;

	/* Consume bytes through the required LF terminator. */
	while (parser->pos < parser->limit) {
		ch = parser->data[parser->pos];
		parser->pos++;

		if (ch == '\n') {
			parser->line[length] = '\0';
			return true;
		}
		if (ch == '\0')
			return bytecode_file_fail(parser, "NUL in bytecode metadata.");
		if (ch == '\r')
			return bytecode_file_fail(parser, "CR in bytecode metadata.");
		if (length >= sizeof(parser->line) - 1)
			return bytecode_file_fail(parser, "Bytecode metadata line is too long.");

		parser->line[length] = (char)ch;
		length++;
	}

	return bytecode_file_fail(parser, "Truncated bytecode metadata line.");
}

/* Read and compare one exact metadata line. */
static bool
bytecode_file_expect_line(
	struct bytecode_file_parser *parser,
	const char *expected)
{
	if (!bytecode_file_read_line(parser))
		return false;
	if (strcmp(parser->line, expected) != 0)
		return bytecode_file_fail(parser, "Unexpected bytecode metadata section.");

	return true;
}

/* Parse one strict unsigned 32-bit decimal. */
static bool
bytecode_file_parse_u32(
	const char *text,
	uint32_t *value)
{
	uint32_t result;
	uint32_t digit;

	assert(value != NULL);

	if (text == NULL || text[0] == '\0')
		return false;

	result = 0;

	/* Accumulate every digit with an explicit overflow check. */
	while (*text != '\0') {
		if (*text < '0' || *text > '9')
			return false;

		digit = (uint32_t)(*text - '0');
		if (result > (UINT32_MAX - digit) / 10U)
			return false;

		result = result * 10U + digit;
		text++;
	}

	*value = result;

	return true;
}

/* Parse one strict signed 64-bit decimal. */
static bool
bytecode_file_parse_i64(
	const char *text,
	int64_t *value)
{
	uint64_t result;
	uint64_t limit;
	uint64_t digit;
	bool negative;

	assert(value != NULL);

	if (text == NULL || text[0] == '\0')
		return false;

	negative = false;
	if (*text == '-') {
		negative = true;
		text++;
		if (*text == '\0')
			return false;
	}

	limit = (uint64_t)INT64_MAX;
	if (negative)
		limit++;
	result = 0;

	/* Accumulate the magnitude without overflowing its unsigned form. */
	while (*text != '\0') {
		if (*text < '0' || *text > '9')
			return false;

		digit = (uint64_t)(unsigned int)(*text - '0');
		if (result > (limit - digit) / 10U)
			return false;

		result = result * 10U + digit;
		text++;
	}

	if (!negative) {
		*value = (int64_t)result;
	} else if (result == limit) {
		*value = INT64_MIN;
	} else {
		*value = -(int64_t)result;
	}

	return true;
}

/* Parse one strict signed native integer. */
static bool
bytecode_file_parse_int(
	const char *text,
	int *value)
{
	int64_t result;

	assert(value != NULL);

	if (!bytecode_file_parse_i64(text, &result))
		return false;
	if (result < (int64_t)INT_MIN || result > (int64_t)INT_MAX)
		return false;

	*value = (int)result;

	return true;
}

/* Bound one untrusted record count by input size and validation cost. */
static bool
bytecode_file_count_valid(
	struct bytecode_file_parser *parser,
	uint32_t count,
	size_t minimum_size)
{
	size_t remaining;

	assert(minimum_size > 0);

	if (count > BYTECODE_FILE_MAX_RECORD_COUNT)
		return false;
	if (parser->pos > parser->limit)
		return false;

	remaining = parser->limit - parser->pos;
	if ((size_t)count > remaining / minimum_size)
		return false;

	return true;
}

/* Allocate one zero-cleared count-sized table safely. */
static void *
bytecode_file_allocate_array(
	uint32_t count,
	size_t item_size)
{
	assert(item_size > 0);

	if (count == 0)
		return NULL;
	if ((size_t)count > SIZE_MAX / item_size)
		return NULL;

	return noct_calloc((size_t)count, item_size);
}

/* Copy one validated metadata line into owned memory. */
static bool
bytecode_file_copy_text(
	const char *text,
	char **copy)
{
	size_t length;

	assert(text != NULL);
	assert(copy != NULL);

	length = strlen(text);
	*copy = noct_malloc(length + 1);
	if (*copy == NULL)
		return false;

	memcpy(*copy, text, length + 1);

	return true;
}

/* Read one metadata string into owned memory. */
static bool
bytecode_file_read_text(
	struct bytecode_file_parser *parser,
	bool allow_empty,
	char **text)
{
	assert(text != NULL);

	if (!bytecode_file_read_line(parser))
		return false;
	if (!allow_empty && parser->line[0] == '\0')
		return bytecode_file_fail(parser, "Empty bytecode metadata name.");
	if (!bytecode_file_copy_text(parser->line, text))
		return bytecode_file_fail(parser, "Out of memory inspecting bytecode.");

	return true;
}

/* Test whether one string uses the source-language identifier alphabet. */
static bool
bytecode_file_identifier(
	const char *text)
{
	const unsigned char *cursor;

	if (text == NULL || text[0] == '\0')
		return false;

	cursor = (const unsigned char *)text;

	/* Check every byte against the lexer symbol alphabet. */
	while (*cursor != '\0') {
		if ((*cursor < (unsigned char)'a' ||
		     *cursor > (unsigned char)'z') &&
		    (*cursor < (unsigned char)'A' ||
		     *cursor > (unsigned char)'Z') &&
		    (*cursor < (unsigned char)'0' ||
		     *cursor > (unsigned char)'9') &&
		    *cursor != (unsigned char)'_') {
			return false;
		}
		cursor++;
	}

	return true;
}

/* Read one owned module identifier. */
static bool
bytecode_file_read_identifier(
	struct bytecode_file_parser *parser,
	char **identifier)
{
	if (!bytecode_file_read_text(parser, false, identifier))
		return false;
	if (!bytecode_file_identifier(*identifier))
		return bytecode_file_fail(parser, "Invalid bytecode module identifier.");

	return true;
}

/* Release one partially or completely inspected function. */
static void
bytecode_file_cleanup_function(
	struct bytecode_file_function *function)
{
	uint32_t i;

	if (function == NULL)
		return;

	noct_free(function->name);
	noct_free(function->source);

	/* Release every owned parameter name. */
	for (i = 0; i < function->param_count; i++)
		noct_free(function->param_name != NULL ? function->param_name[i] : NULL);
	noct_free(function->param_name);
	noct_free(function->param_type);
	noct_free(function->param_packed_type);
	noct_free(function->param_restricted);
#if defined(NOCT_USE_OPTIMIZER)
	fast_info_free(function->fast_info);
#endif

	memset(function, 0, sizeof(*function));
}

/* Read and validate one exact fast signature section. */
static bool
bytecode_file_parse_fast_signature(
	struct bytecode_file_parser *parser,
	struct bytecode_file_function *function)
{
#if defined(NOCT_USE_OPTIMIZER)
	struct fast_signature *signature;
	struct fast_param_contract *contract;
	struct fast_extent *extent;
	uint32_t unsigned_value;
	int signed_value;
	int64_t signed_long;
	uint32_t i;
	uint32_t axis;

	signature = NULL;

	if (function->fast_info != NULL)
		return bytecode_file_fail(parser, "Duplicate fast signature data.");

	signature = noct_malloc(sizeof(*signature));
	if (signature == NULL) {
		return bytecode_file_fail(
			parser,
			"Out of memory inspecting fast signature.");
	}
	fast_signature_init(signature);
	function->fast_info = signature;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &unsigned_value))
		return bytecode_file_fail(parser, "Invalid fast signature version.");
	if (unsigned_value != BYTECODE_FILE_FAST_SIGNATURE_VERSION)
		return bytecode_file_fail(parser, "Unsupported fast signature version.");
	signature->version = unsigned_value;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &unsigned_value))
		return bytecode_file_fail(parser, "Invalid fast signature validity flag.");
	if (unsigned_value != 1)
		return bytecode_file_fail(parser, "Invalid fast signature validity flag.");

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &unsigned_value))
		return bytecode_file_fail(parser, "Invalid fast signature parameter count.");
	if (unsigned_value != function->param_count ||
	    unsigned_value > NOCT_ARG_MAX) {
		return bytecode_file_fail(parser, "Fast signature parameter count mismatch.");
	}
	signature->param_count = unsigned_value;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_int(parser->line, &signature->return_type))
		return bytecode_file_fail(parser, "Invalid fast signature return type.");

	signature->param = bytecode_file_allocate_array(
		signature->param_count,
		sizeof(*signature->param));
	if (signature->param_count > 0 && signature->param == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting fast signature.");
	}

	/* Read every parameter contract and its exact-rank extent table. */
	for (i = 0; i < signature->param_count; i++) {
		contract = &signature->param[i];

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_int(parser->line, &contract->value_type)) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature parameter type.");
		}

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_int(parser->line, &contract->packed_type)) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature Packed type.");
		}

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(parser->line, &unsigned_value)) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature restricted flag.");
		}
		if (unsigned_value > 1) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature restricted flag.");
		}
		contract->restricted = unsigned_value != 0;

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(parser->line, &contract->rank))
			return bytecode_file_fail(parser, "Invalid fast signature rank.");
		if (contract->rank > BYTECODE_FILE_FAST_RANK_MAX)
			return bytecode_file_fail(parser, "Fast signature rank is too large.");

		contract->extent = bytecode_file_allocate_array(
			contract->rank,
			sizeof(*contract->extent));
		if (contract->rank > 0 && contract->extent == NULL) {
			return bytecode_file_fail(
				parser,
				"Out of memory inspecting fast signature.");
		}

		/* Read every constant or parameter-dependent extent. */
		for (axis = 0; axis < contract->rank; axis++) {
			extent = &contract->extent[axis];

			if (!bytecode_file_read_line(parser))
				return false;
			if (!bytecode_file_parse_int(parser->line, &signed_value)) {
				return bytecode_file_fail(
					parser,
					"Invalid fast signature extent kind.");
			}
			extent->kind = signed_value;

			if (!bytecode_file_read_line(parser))
				return false;

			if (extent->kind == BYTECODE_FILE_FAST_EXTENT_CONST) {
				if (!bytecode_file_parse_i64(
					parser->line,
					&signed_long)) {
					return bytecode_file_fail(
						parser,
						"Invalid constant fast extent.");
				}
				extent->value.constant = signed_long;
			} else if (extent->kind == BYTECODE_FILE_FAST_EXTENT_PARAM) {
				if (!bytecode_file_parse_u32(
					parser->line,
					&unsigned_value)) {
					return bytecode_file_fail(
						parser,
						"Invalid parameter fast extent.");
				}
				extent->value.param_index = unsigned_value;
			} else {
				return bytecode_file_fail(
					parser,
					"Unsupported fast signature extent kind.");
			}
		}
	}

	signature->valid = true;
	if (!fast_signature_valid(signature))
		return bytecode_file_fail(parser, "Invalid fast signature layout.");

	return true;
#else
	uint32_t param_count;
	uint32_t rank;
	uint32_t unsigned_value;
	int signed_value;
	int64_t signed_long;
	uint32_t i;
	uint32_t axis;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &unsigned_value))
		return bytecode_file_fail(parser, "Invalid fast signature version.");
	if (unsigned_value != BYTECODE_FILE_FAST_SIGNATURE_VERSION)
		return bytecode_file_fail(parser, "Unsupported fast signature version.");

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &unsigned_value)) {
		return bytecode_file_fail(
			parser,
			"Invalid fast signature validity flag.");
	}
	if (unsigned_value != 1U) {
		return bytecode_file_fail(
			parser,
			"Invalid fast signature validity flag.");
	}

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &param_count)) {
		return bytecode_file_fail(
			parser,
			"Invalid fast signature parameter count.");
	}
	if (param_count != function->param_count || param_count > NOCT_ARG_MAX) {
		return bytecode_file_fail(
			parser,
			"Fast signature parameter count mismatch.");
	}

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_int(parser->line, &signed_value))
		return bytecode_file_fail(parser, "Invalid fast signature return type.");

	/* Consume and validate the optimizer-owned contract representation. */
	for (i = 0; i < param_count; i++) {
		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_int(parser->line, &signed_value)) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature parameter type.");
		}

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_int(parser->line, &signed_value)) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature Packed type.");
		}

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(parser->line, &unsigned_value)) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature restricted flag.");
		}
		if (unsigned_value > 1U) {
			return bytecode_file_fail(
				parser,
				"Invalid fast signature restricted flag.");
		}

		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(parser->line, &rank)) {
			return bytecode_file_fail(parser, "Invalid fast signature rank.");
		}
		if (rank > BYTECODE_FILE_FAST_RANK_MAX)
			return bytecode_file_fail(parser, "Fast signature rank is too large.");

		/* Consume every exact-shape extent in this parameter. */
		for (axis = 0; axis < rank; axis++) {
			if (!bytecode_file_read_line(parser))
				return false;
			if (!bytecode_file_parse_int(parser->line, &signed_value)) {
				return bytecode_file_fail(
					parser,
					"Invalid fast signature extent kind.");
			}
			if (!bytecode_file_read_line(parser))
				return false;

			if (signed_value == BYTECODE_FILE_FAST_EXTENT_CONST) {
				if (!bytecode_file_parse_i64(parser->line, &signed_long)) {
					return bytecode_file_fail(
						parser,
						"Invalid constant fast extent.");
				}
			} else if (signed_value == BYTECODE_FILE_FAST_EXTENT_PARAM) {
				if (!bytecode_file_parse_u32(
					parser->line,
					&unsigned_value)) {
					return bytecode_file_fail(
						parser,
						"Invalid parameter fast extent.");
				}
				if (unsigned_value >= param_count) {
					return bytecode_file_fail(
						parser,
						"Invalid parameter fast extent.");
				}
			} else {
				return bytecode_file_fail(
					parser,
					"Unsupported fast signature extent kind.");
			}
		}
	}

	return true;
#endif
}

/* Cross-check a fast signature against ordinary function metadata. */
static bool
bytecode_file_validate_fast_metadata(
	const struct bytecode_file_function *function,
	unsigned int sections)
{
#if defined(NOCT_USE_OPTIMIZER)
	const struct fast_signature *signature;
	const struct fast_param_contract *contract;
	uint32_t i;

	signature = fast_info_signature(function->fast_info);
	if (!function->is_fast)
		return false;
	if ((sections & BYTECODE_FILE_FAST_SIGNATURE) == 0)
		return false;
	if (!signature->valid)
		return false;
	if (!fast_signature_valid(signature))
		return false;
	if (signature->param_count != function->param_count)
		return false;
	if (function->param_count > 0 &&
	    (sections & BYTECODE_FILE_PARAM_TYPES) == 0) {
		return false;
	}
	if ((sections & BYTECODE_FILE_RETURN_TYPE) == 0)
		return false;
	if (signature->return_type != function->return_type)
		return false;
	if (function->return_packed_type != -1)
		return false;
	if (function->return_type == BYTECODE_FILE_FAST_RETURN_VOID &&
	    function->return_type_checked) {
		return false;
	}

	/* Match every ordinary parameter entry to its exact contract. */
	for (i = 0; i < function->param_count; i++) {
		contract = &signature->param[i];

		if (contract->value_type != function->param_type[i])
			return false;
		if (contract->packed_type != function->param_packed_type[i])
			return false;
		if (contract->restricted != function->param_restricted[i])
			return false;
	}

	return true;
#else
	if (!function->is_fast)
		return false;
	if ((sections & BYTECODE_FILE_FAST_SIGNATURE) == 0)
		return false;
	if (function->param_count > 0 &&
	    (sections & BYTECODE_FILE_PARAM_TYPES) == 0) {
		return false;
	}
	if ((sections & BYTECODE_FILE_RETURN_TYPE) == 0)
		return false;
	if (function->return_packed_type != -1)
		return false;
	if (function->return_type == BYTECODE_FILE_FAST_RETURN_VOID &&
	    function->return_type_checked) {
		return false;
	}

	return true;
#endif
}

/* Map one optional section label to its duplicate-check bit. */
static unsigned int
bytecode_file_get_optional_section(
	const char *line)
{
	if (strcmp(line, "Parameter Types") == 0)
		return BYTECODE_FILE_PARAM_TYPES;
	if (strcmp(line, "Parameter Packed Types") == 0)
		return BYTECODE_FILE_PARAM_PACKED_TYPES;
	if (strcmp(line, "Parameter Restricted") == 0)
		return BYTECODE_FILE_PARAM_RESTRICTED;
	if (strcmp(line, "Return Type") == 0)
		return BYTECODE_FILE_RETURN_TYPE;
	if (strcmp(line, "Vector Ops") == 0)
		return BYTECODE_FILE_VECTOR_OPS;
	if (strcmp(line, "FMA Ops") == 0)
		return BYTECODE_FILE_FMA_OPS;
	if (strcmp(line, "Function Kind") == 0)
		return BYTECODE_FILE_FUNCTION_KIND;
	if (strcmp(line, "Fast Signature") == 0)
		return BYTECODE_FILE_FAST_SIGNATURE;

	return 0;
}

/* Read one parameter-wide optional metadata section. */
static bool
bytecode_file_parse_param_section(
	struct bytecode_file_parser *parser,
	struct bytecode_file_function *function,
	unsigned int section)
{
	uint32_t value;
	int signed_value;
	uint32_t i;

	/* Read one metadata entry for every parameter. */
	for (i = 0; i < function->param_count; i++) {
		if (!bytecode_file_read_line(parser))
			return false;

		if (section == BYTECODE_FILE_PARAM_RESTRICTED) {
			if (!bytecode_file_parse_u32(parser->line, &value)) {
				return bytecode_file_fail(
					parser,
					"Invalid parameter restricted flag.");
			}
			if (value > 1) {
				return bytecode_file_fail(
					parser,
					"Invalid parameter restricted flag.");
			}
			function->param_restricted[i] = value != 0;
		} else {
			if (!bytecode_file_parse_int(parser->line, &signed_value)) {
				return bytecode_file_fail(
					parser,
					"Invalid parameter type metadata.");
			}

			if (section == BYTECODE_FILE_PARAM_TYPES)
				function->param_type[i] = signed_value;
			else
				function->param_packed_type[i] = signed_value;
		}
	}

	return true;
}

/* Read the three-value return metadata section. */
static bool
bytecode_file_parse_return_section(
	struct bytecode_file_parser *parser,
	struct bytecode_file_function *function)
{
	uint32_t flag;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_int(parser->line, &function->return_type))
		return bytecode_file_fail(parser, "Invalid function return type.");

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_int(parser->line, &function->return_packed_type))
		return bytecode_file_fail(parser, "Invalid function return Packed type.");

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &flag))
		return bytecode_file_fail(parser, "Invalid return type checked flag.");
	if (flag > 1)
		return bytecode_file_fail(parser, "Invalid return type checked flag.");
	function->return_type_checked = flag != 0;

	return true;
}

/* Read one strict Boolean optional metadata section. */
static bool
bytecode_file_parse_flag_section(
	struct bytecode_file_parser *parser,
	bool *flag)
{
	uint32_t value;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &value))
		return bytecode_file_fail(parser, "Invalid bytecode metadata flag.");
	if (value > 1)
		return bytecode_file_fail(parser, "Invalid bytecode metadata flag.");

	*flag = value != 0;

	return true;
}

/* Read the function-kind metadata section. */
static bool
bytecode_file_parse_kind_section(
	struct bytecode_file_parser *parser,
	struct bytecode_file_function *function)
{
	uint32_t kind;

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &kind))
		return bytecode_file_fail(parser, "Invalid bytecode function kind.");
	if (kind > 1)
		return bytecode_file_fail(parser, "Unsupported bytecode function kind.");

	function->is_fast = kind == 1;

	return true;
}

/* Read one complete function descriptor and its borrowed opcode span. */
static bool
bytecode_file_parse_function(
	struct bytecode_file_parser *parser,
	const char *module_source,
	struct bytecode_file_function *function)
{
	unsigned int sections;
	unsigned int section;
	uint32_t value;
	uint32_t i;

	function->return_type = -1;
	function->return_packed_type = -1;

	if (!bytecode_file_expect_line(parser, "Begin Function"))
		return false;
	if (!bytecode_file_expect_line(parser, "Name"))
		return false;
	if (!bytecode_file_read_text(parser, false, &function->name))
		return false;

	if (!bytecode_file_read_line(parser))
		return false;
	if (strcmp(parser->line, "Source") == 0) {
		if (!bytecode_file_read_text(parser, false, &function->source))
			return false;
		if (!bytecode_file_read_line(parser))
			return false;
	} else {
		if (!bytecode_file_copy_text(module_source, &function->source)) {
			return bytecode_file_fail(
				parser,
				"Out of memory inspecting bytecode.");
		}
	}

	if (strcmp(parser->line, "Parameters") != 0)
		return bytecode_file_fail(parser, "Missing function parameters section.");

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &function->param_count))
		return bytecode_file_fail(parser, "Invalid function parameter count.");
	if (function->param_count > NOCT_ARG_MAX)
		return bytecode_file_fail(parser, "Too many function parameters.");

	function->param_name = bytecode_file_allocate_array(
		function->param_count,
		sizeof(*function->param_name));
	if (function->param_count > 0 && function->param_name == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting bytecode.");
	}
	function->param_type = bytecode_file_allocate_array(
		function->param_count,
		sizeof(*function->param_type));
	if (function->param_count > 0 && function->param_type == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting bytecode.");
	}
	function->param_packed_type = bytecode_file_allocate_array(
		function->param_count,
		sizeof(*function->param_packed_type));
	if (function->param_count > 0 && function->param_packed_type == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting bytecode.");
	}
	function->param_restricted = bytecode_file_allocate_array(
		function->param_count,
		sizeof(*function->param_restricted));
	if (function->param_count > 0 && function->param_restricted == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting bytecode.");
	}

	/* Establish the absent-metadata defaults for every parameter. */
	for (i = 0; i < function->param_count; i++) {
		function->param_type[i] = -1;
		function->param_packed_type[i] = -1;
	}

	/* Read every declared parameter name. */
	for (i = 0; i < function->param_count; i++) {
		if (!bytecode_file_read_text(
			parser,
			false,
			&function->param_name[i])) {
			return false;
		}
		if (!bytecode_file_identifier(function->param_name[i])) {
			return bytecode_file_fail(
				parser,
				"Invalid function parameter identifier.");
		}
	}

	sections = 0;
	if (!bytecode_file_read_line(parser))
		return false;

	/* Read optional metadata sections until the required local-size section. */
	while (strcmp(parser->line, "Temporary Size") != 0) {
		section = bytecode_file_get_optional_section(parser->line);
		if (section == 0)
			return bytecode_file_fail(parser, "Unknown function metadata section.");
		if ((sections & section) != 0)
			return bytecode_file_fail(parser, "Duplicate function metadata section.");
		sections |= section;

		if (section == BYTECODE_FILE_PARAM_TYPES ||
		    section == BYTECODE_FILE_PARAM_PACKED_TYPES ||
		    section == BYTECODE_FILE_PARAM_RESTRICTED) {
			if (!bytecode_file_parse_param_section(
				parser,
				function,
				section)) {
				return false;
			}
		} else if (section == BYTECODE_FILE_RETURN_TYPE) {
			if (!bytecode_file_parse_return_section(parser, function))
				return false;
		} else if (section == BYTECODE_FILE_VECTOR_OPS) {
			if (!bytecode_file_parse_flag_section(
				parser,
				&function->has_vector_ops)) {
				return false;
			}
		} else if (section == BYTECODE_FILE_FMA_OPS) {
			if (!bytecode_file_parse_flag_section(
				parser,
				&function->has_fma_ops)) {
				return false;
			}
		} else if (section == BYTECODE_FILE_FUNCTION_KIND) {
			if (!bytecode_file_parse_kind_section(parser, function))
				return false;
		} else {
			if (!bytecode_file_parse_fast_signature(parser, function))
				return false;
		}

		if (!bytecode_file_read_line(parser))
			return false;
	}

	if (function->is_fast) {
		if (!bytecode_file_validate_fast_metadata(function, sections)) {
			return bytecode_file_fail(
				parser,
				"Fast signature does not match function metadata.");
		}
#if !defined(NOCT_USE_OPTIMIZER)
		return bytecode_file_fail(
			parser,
			"Optimized __fast bytecode requires optimizer support.");
#endif
	} else if ((sections & BYTECODE_FILE_FAST_SIGNATURE) != 0) {
		return bytecode_file_fail(
			parser,
			"Fast signature is present on a normal function.");
	}

	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &function->tmpvar_size))
		return bytecode_file_fail(parser, "Invalid function temporary size.");
	if (function->tmpvar_size <= function->param_count ||
	    function->tmpvar_size > LIR_TMPVAR_MAX) {
		return bytecode_file_fail(parser, "Function temporary size is out of range.");
	}

	if (!bytecode_file_expect_line(parser, "Bytecode Size"))
		return false;
	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &value))
		return bytecode_file_fail(parser, "Invalid function bytecode size.");

	if (parser->pos > parser->limit)
		return bytecode_file_fail(parser, "Invalid bytecode parser position.");
	if ((size_t)value > parser->limit - parser->pos)
		return bytecode_file_fail(parser, "Truncated function bytecode payload.");

	function->bytecode.data = parser->data + parser->pos;
	function->bytecode.size = value;
	parser->pos += (size_t)value;

	if (parser->pos >= parser->limit)
		return bytecode_file_fail(parser, "Missing function bytecode terminator.");
	if (parser->data[parser->pos] != (uint8_t)'\n')
		return bytecode_file_fail(parser, "Invalid function bytecode terminator.");
	parser->pos++;

	if (!bytecode_file_expect_line(parser, "End Function"))
		return false;

	return true;
}

/* Read one exact raw module within the parser's input buffer. */
static bool
bytecode_file_parse_module(
	struct bytecode_file_parser *parser,
	size_t limit,
	enum bytecode_file_kind required_kind,
	struct bytecode_file_module *module)
{
	size_t outer_limit;
	enum bytecode_file_kind kind;
	uint32_t count;
	uint32_t i;
	bool succeeded;

	if (limit > parser->limit || parser->pos > limit)
		return bytecode_file_fail(parser, "Invalid embedded module boundary.");

	outer_limit = parser->limit;
	parser->limit = limit;
	succeeded = false;

	kind = bytecode_file_detect(
		parser->data + parser->pos,
		parser->limit - parser->pos);
	if (kind != required_kind) {
		bytecode_file_fail(parser, "Unexpected embedded bytecode version.");
		goto done;
	}

	if (kind == BYTECODE_FILE_MODULE_1_0) {
		if (!bytecode_file_expect_line(parser, "Noct Bytecode 1.0"))
			goto done;
	} else {
		if (!bytecode_file_expect_line(parser, "Noct Bytecode 1.1"))
			goto done;
	}
	module->kind = kind;

	if (!bytecode_file_expect_line(parser, "Source"))
		goto done;
	if (!bytecode_file_read_text(parser, false, &module->source))
		goto done;

	if (kind == BYTECODE_FILE_MODULE_1_1) {
		if (!bytecode_file_expect_line(parser, "Number Of Requires"))
			goto done;
		if (!bytecode_file_read_line(parser))
			goto done;
		if (!bytecode_file_parse_u32(parser->line, &module->require_count)) {
			bytecode_file_fail(parser, "Invalid bytecode require count.");
			goto done;
		}
		if (!bytecode_file_count_valid(
			parser,
			module->require_count,
			2)) {
			bytecode_file_fail(parser, "Bytecode require count is too large.");
			goto done;
		}

		module->require_name = bytecode_file_allocate_array(
			module->require_count,
			sizeof(*module->require_name));
		if (module->require_count > 0 && module->require_name == NULL) {
			bytecode_file_fail(parser, "Out of memory inspecting bytecode.");
			goto done;
		}

		/* Read every required module identifier in declaration order. */
		for (i = 0; i < module->require_count; i++) {
			if (!bytecode_file_read_identifier(
				parser,
				&module->require_name[i])) {
				goto done;
			}
		}
	}

	if (!bytecode_file_expect_line(parser, "Number Of Functions"))
		goto done;
	if (!bytecode_file_read_line(parser))
		goto done;
	if (!bytecode_file_parse_u32(parser->line, &count)) {
		bytecode_file_fail(parser, "Invalid bytecode function count.");
		goto done;
	}
	if (!bytecode_file_count_valid(parser, count, 1)) {
		bytecode_file_fail(parser, "Bytecode function count is too large.");
		goto done;
	}
	module->function_count = count;

	module->function = bytecode_file_allocate_array(
		module->function_count,
		sizeof(*module->function));
	if (module->function_count > 0 && module->function == NULL) {
		bytecode_file_fail(parser, "Out of memory inspecting bytecode.");
		goto done;
	}

	/* Read every function descriptor in its serialized order. */
	for (i = 0; i < module->function_count; i++) {
		if (!bytecode_file_parse_function(
			parser,
			module->source,
			&module->function[i])) {
			goto done;
		}
	}

	if (parser->pos != parser->limit) {
		bytecode_file_fail(parser, "Trailing bytes after bytecode module.");
		goto done;
	}

	succeeded = true;

done:
	parser->limit = outer_limit;

	return succeeded;
}

/* Resolve one application binding without filesystem fallback. */
static bool
bytecode_file_find_binding(
	const struct bytecode_file_app *app,
	const char *name,
	uint32_t *module_index)
{
	uint32_t i;

	assert(app != NULL);
	assert(name != NULL);
	assert(module_index != NULL);

	/* Search the already uniqueness-checked binding directory. */
	for (i = 0; i < app->binding_count; i++) {
		if (strcmp(app->binding[i].module_name, name) == 0) {
			*module_index = app->binding[i].module_index;
			return true;
		}
	}

	return false;
}

/* Test whether an earlier function already owns one exact link name. */
static bool
bytecode_file_function_name_seen(
	const struct bytecode_file_app *app,
	uint32_t module_index,
	uint32_t function_index)
{
	const char *name;
	uint32_t i;
	uint32_t j;
	uint32_t limit;

	name = app->module[module_index].function[function_index].name;

	/* Compare the name with functions in every preceding module. */
	for (i = 0; i <= module_index; i++) {
		limit = app->module[i].function_count;
		if (i == module_index)
			limit = function_index;

		/* Compare the name with every preceding function in this module. */
		for (j = 0; j < limit; j++) {
			if (strcmp(app->module[i].function[j].name, name) == 0)
				return true;
		}
	}

	return false;
}

/* Test whether one module belongs to the explicit root list. */
static bool
bytecode_file_module_is_root(
	const struct bytecode_file_app *app,
	uint32_t module_index)
{
	uint32_t i;

	/* Search every explicit root index. */
	for (i = 0; i < app->root_count; i++) {
		if (app->root_index[i] == module_index)
			return true;
	}

	return false;
}

/* Validate binding and root directory invariants. */
static bool
bytecode_file_validate_app_directory(
	struct bytecode_file_parser *parser,
	const struct bytecode_file_app *app)
{
	uint32_t module_index;
	uint32_t i;
	uint32_t j;

	if (app->module_count == 0)
		return bytecode_file_fail(parser, "Application has no modules.");
	if (app->root_count == 0)
		return bytecode_file_fail(parser, "Application has no roots.");

	/* Check every binding name, index, and uniqueness constraint. */
	for (i = 0; i < app->binding_count; i++) {
		if (app->binding[i].module_index >= app->module_count)
			return bytecode_file_fail(parser, "Application binding index is out of range.");

		/* Compare this name with every later binding name. */
		for (j = i + 1; j < app->binding_count; j++) {
			if (strcmp(
				app->binding[i].module_name,
				app->binding[j].module_name) == 0) {
				return bytecode_file_fail(
					parser,
					"Duplicate application binding name.");
			}
		}
	}

	/* Check every explicit root index and reject duplicates. */
	for (i = 0; i < app->root_count; i++) {
		if (app->root_index[i] >= app->module_count)
			return bytecode_file_fail(parser, "Application root index is out of range.");

		/* Compare this root with every later root index. */
		for (j = i + 1; j < app->root_count; j++) {
			if (app->root_index[i] == app->root_index[j])
				return bytecode_file_fail(parser, "Duplicate application root index.");
		}
	}

	/* Require an in-container binding for every module dependency. */
	for (i = 0; i < app->module_count; i++) {
		/* Resolve every require edge through the binding directory. */
		for (j = 0; j < app->module[i].require_count; j++) {
			if (!bytecode_file_find_binding(
				app,
				app->module[i].require_name[j],
				&module_index)) {
				return bytecode_file_fail(
					parser,
					"Missing application require binding.");
			}
		}
	}

	return true;
}

/* Validate closure-wide function link names and entry-point ownership. */
static bool
bytecode_file_validate_app_symbols(
	struct bytecode_file_parser *parser,
	const struct bytecode_file_app *app)
{
	const char *name;
	uint32_t main_count;
	uint32_t main_module;
	uint32_t init_count;
	uint32_t i;
	uint32_t j;

	main_count = 0;
	main_module = 0;

	/* Validate functions in every module record. */
	for (i = 0; i < app->module_count; i++) {
		init_count = 0;

		/* Check every exact function link name in this module. */
		for (j = 0; j < app->module[i].function_count; j++) {
			name = app->module[i].function[j].name;
			if (bytecode_file_function_name_seen(app, i, j)) {
				return bytecode_file_fail(
					parser,
					"Duplicate application function link name.");
			}

			if (strncmp(name, "$init.", 6) == 0)
				init_count++;
			if (strcmp(name, "main") == 0) {
				main_count++;
				main_module = i;
			}
		}

		if (init_count > 1) {
			return bytecode_file_fail(
				parser,
				"Application module has multiple initializers.");
		}
	}

	if (main_count != 1)
		return bytecode_file_fail(parser, "Application must contain exactly one main function.");
	if (!bytecode_file_module_is_root(app, main_module))
		return bytecode_file_fail(parser, "Application main function is not owned by a root module.");

	return true;
}

/* Reject cycles in the application require graph. */
static bool
bytecode_file_validate_app_cycle(
	struct bytecode_file_parser *parser,
	const struct bytecode_file_app *app)
{
	uint8_t *color;
	uint32_t *stack;
	uint32_t *next_require;
	uint32_t start;
	uint32_t depth;
	uint32_t current;
	uint32_t target;
	uint32_t edge;
	bool succeeded;

	color = NULL;
	stack = NULL;
	next_require = NULL;
	succeeded = false;

	color = bytecode_file_allocate_array(
		app->module_count,
		sizeof(*color));
	if (color == NULL) {
		bytecode_file_fail(parser, "Out of memory validating application graph.");
		goto cleanup;
	}
	stack = bytecode_file_allocate_array(
		app->module_count,
		sizeof(*stack));
	if (stack == NULL) {
		bytecode_file_fail(parser, "Out of memory validating application graph.");
		goto cleanup;
	}
	next_require = bytecode_file_allocate_array(
		app->module_count,
		sizeof(*next_require));
	if (next_require == NULL) {
		bytecode_file_fail(parser, "Out of memory validating application graph.");
		goto cleanup;
	}

	/* Start an iterative depth-first traversal at every unvisited module. */
	for (start = 0; start < app->module_count; start++) {
		if (color[start] != 0)
			continue;

		depth = 0;
		stack[0] = start;
		next_require[0] = 0;
		color[start] = 1;

		/* Traverse require edges until this search stack is empty. */
		while (true) {
			current = stack[depth];
			edge = next_require[depth];

			if (edge >= app->module[current].require_count) {
				color[current] = 2;
				if (depth == 0)
					break;
				depth--;
				continue;
			}

			next_require[depth]++;
			if (!bytecode_file_find_binding(
				app,
				app->module[current].require_name[edge],
				&target)) {
				bytecode_file_fail(parser, "Missing application require binding.");
				goto cleanup;
			}

			if (color[target] == 1) {
				bytecode_file_fail(parser, "Cycle in application require graph.");
				goto cleanup;
			}
			if (color[target] == 2)
				continue;

			depth++;
			stack[depth] = target;
			next_require[depth] = 0;
			color[target] = 1;
		}
	}

	succeeded = true;

cleanup:
	noct_free(next_require);
	noct_free(stack);
	noct_free(color);

	return succeeded;
}

/* Reject module records unreachable from the explicit roots. */
static bool
bytecode_file_validate_app_reachability(
	struct bytecode_file_parser *parser,
	const struct bytecode_file_app *app)
{
	uint8_t *visited;
	uint32_t *queue;
	uint32_t head;
	uint32_t tail;
	uint32_t current;
	uint32_t target;
	uint32_t i;
	bool succeeded;

	visited = NULL;
	queue = NULL;
	head = 0;
	tail = 0;
	succeeded = false;

	visited = bytecode_file_allocate_array(
		app->module_count,
		sizeof(*visited));
	if (visited == NULL) {
		bytecode_file_fail(parser, "Out of memory validating application graph.");
		goto cleanup;
	}
	queue = bytecode_file_allocate_array(
		app->module_count,
		sizeof(*queue));
	if (queue == NULL) {
		bytecode_file_fail(parser, "Out of memory validating application graph.");
		goto cleanup;
	}

	/* Seed the traversal with every distinct explicit root. */
	for (i = 0; i < app->root_count; i++) {
		current = app->root_index[i];
		visited[current] = 1;
		queue[tail] = current;
		tail++;
	}

	/* Visit every dependency reachable from the root queue. */
	while (head < tail) {
		current = queue[head];
		head++;

		/* Follow every require edge owned by this module. */
		for (i = 0; i < app->module[current].require_count; i++) {
			if (!bytecode_file_find_binding(
				app,
				app->module[current].require_name[i],
				&target)) {
				bytecode_file_fail(parser, "Missing application require binding.");
				goto cleanup;
			}
			if (visited[target] != 0)
				continue;

			visited[target] = 1;
			queue[tail] = target;
			tail++;
		}
	}

	/* Ensure the container has no unreachable module record. */
	for (i = 0; i < app->module_count; i++) {
		if (visited[i] == 0) {
			bytecode_file_fail(parser, "Unreachable application module.");
			goto cleanup;
		}
	}

	succeeded = true;

cleanup:
	noct_free(queue);
	noct_free(visited);

	return succeeded;
}

/* Validate all cross-record application invariants. */
static bool
bytecode_file_validate_app(
	struct bytecode_file_parser *parser,
	const struct bytecode_file_app *app)
{
	uint32_t total_function_count;
	uint32_t total_require_count;
	uint32_t i;

	total_function_count = 0;
	total_require_count = 0;

	/* Bound every closure-wide quadratic directory validation. */
	for (i = 0; i < app->module_count; i++) {
		if (app->module[i].function_count >
		    BYTECODE_FILE_MAX_RECORD_COUNT - total_function_count) {
			return bytecode_file_fail(
				parser,
				"Application has too many functions.");
		}
		total_function_count += app->module[i].function_count;

		if (app->module[i].require_count >
		    BYTECODE_FILE_MAX_RECORD_COUNT - total_require_count) {
			return bytecode_file_fail(
				parser,
				"Application has too many require edges.");
		}
		total_require_count += app->module[i].require_count;
	}

	if (!bytecode_file_validate_app_directory(parser, app))
		return false;
	if (!bytecode_file_validate_app_symbols(parser, app))
		return false;
	if (!bytecode_file_validate_app_cycle(parser, app))
		return false;
	if (!bytecode_file_validate_app_reachability(parser, app))
		return false;

	return true;
}

/* Read one complete application container and all embedded modules. */
static bool
bytecode_file_parse_app(
	struct bytecode_file_parser *parser,
	struct bytecode_file_app *app)
{
	uint32_t module_size;
	size_t module_limit;
	uint32_t i;

	if (!bytecode_file_expect_line(parser, "Noct App 1.0"))
		return false;

	if (!bytecode_file_expect_line(parser, "Number Of Modules"))
		return false;
	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &app->module_count))
		return bytecode_file_fail(parser, "Invalid application module count.");
	if (!bytecode_file_count_valid(parser, app->module_count, 1))
		return bytecode_file_fail(parser, "Application module count is too large.");
	app->module = bytecode_file_allocate_array(
		app->module_count,
		sizeof(*app->module));
	if (app->module_count > 0 && app->module == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting application.");
	}

	if (!bytecode_file_expect_line(parser, "Number Of Bindings"))
		return false;
	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &app->binding_count))
		return bytecode_file_fail(parser, "Invalid application binding count.");
	if (!bytecode_file_count_valid(parser, app->binding_count, 4))
		return bytecode_file_fail(parser, "Application binding count is too large.");
	app->binding = bytecode_file_allocate_array(
		app->binding_count,
		sizeof(*app->binding));
	if (app->binding_count > 0 && app->binding == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting application.");
	}

	/* Read every binding name and zero-origin module index. */
	for (i = 0; i < app->binding_count; i++) {
		if (!bytecode_file_read_identifier(
			parser,
			&app->binding[i].module_name)) {
			return false;
		}
		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(
			parser->line,
			&app->binding[i].module_index)) {
			return bytecode_file_fail(parser, "Invalid application binding index.");
		}
	}

	if (!bytecode_file_expect_line(parser, "Number Of Roots"))
		return false;
	if (!bytecode_file_read_line(parser))
		return false;
	if (!bytecode_file_parse_u32(parser->line, &app->root_count))
		return bytecode_file_fail(parser, "Invalid application root count.");
	if (!bytecode_file_count_valid(parser, app->root_count, 2))
		return bytecode_file_fail(parser, "Application root count is too large.");
	app->root_index = bytecode_file_allocate_array(
		app->root_count,
		sizeof(*app->root_index));
	if (app->root_count > 0 && app->root_index == NULL) {
		return bytecode_file_fail(parser, "Out of memory inspecting application.");
	}

	/* Read every explicit root index in command-line order. */
	for (i = 0; i < app->root_count; i++) {
		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(parser->line, &app->root_index[i]))
			return bytecode_file_fail(parser, "Invalid application root index.");
	}

	if (!bytecode_file_expect_line(parser, "Modules"))
		return false;

	/* Parse each declared-size embedded Noct Bytecode 1.1 record. */
	for (i = 0; i < app->module_count; i++) {
		if (!bytecode_file_expect_line(parser, "Module Bytecode Size"))
			return false;
		if (!bytecode_file_read_line(parser))
			return false;
		if (!bytecode_file_parse_u32(parser->line, &module_size))
			return bytecode_file_fail(parser, "Invalid embedded module size.");
		if (module_size == 0)
			return bytecode_file_fail(parser, "Empty embedded module.");
		if (parser->pos > parser->limit)
			return bytecode_file_fail(parser, "Invalid application parser position.");
		if ((size_t)module_size > parser->limit - parser->pos)
			return bytecode_file_fail(parser, "Truncated embedded module.");

		module_limit = parser->pos + (size_t)module_size;
		if (!bytecode_file_parse_module(
			parser,
			module_limit,
			BYTECODE_FILE_MODULE_1_1,
			&app->module[i])) {
			return false;
		}
	}

	if (!bytecode_file_expect_line(parser, "End App"))
		return false;
	if (parser->pos != parser->size)
		return bytecode_file_fail(parser, "Trailing bytes after application container.");

	if (!bytecode_file_validate_app(parser, app))
		return false;

	return true;
}
