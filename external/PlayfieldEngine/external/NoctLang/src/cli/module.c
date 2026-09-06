/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-owned source and bytecode module graph.
 */

#include "module.h"

#include "ast.h"
#include "bytecode.h"
#include "bytecode_file.h"
#include "hir.h"
#if defined(NOCT_USE_OPTIMIZER)
#include "fast.h"
#include "hir_fast_checked.h"
#endif
#include "lir.h"
#include "noct.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_MODULE_PATH_MAX	64
#define CLI_MODULE_ERROR_SIZE	512

enum cli_module_state {
	CLI_MODULE_UNVISITED,
	CLI_MODULE_VISITING,
	CLI_MODULE_VISITED
};

struct cli_module_record {
	struct cli_module_artifact artifact;
	uint8_t *storage;
	size_t storage_size;
	uint32_t require_count;
	char **require_name;
	uint32_t function_count;
	char **function_name;
	struct bytecode_file_module bytecode;
	enum cli_module_state state;
	bool owns_storage;
};

struct cli_module_owned_binding {
	struct cli_module_binding binding;
	char *name;
};

static const char *cli_module_path[CLI_MODULE_PATH_MAX];
static uint32_t cli_module_path_count;
static struct cli_module_record *cli_module_record_table;
static uint32_t cli_module_record_count;
static uint32_t cli_module_record_capacity;
static struct cli_module_owned_binding *cli_module_binding_table;
static uint32_t cli_module_binding_count;
static uint32_t cli_module_binding_capacity;
static uint32_t *cli_module_postorder;
static uint32_t cli_module_postorder_count;
static uint32_t cli_module_postorder_capacity;
static uint32_t *cli_module_root;
static uint32_t cli_module_root_count;
static uint32_t cli_module_root_capacity;
static struct bytecode_file_app cli_module_app;
static uint32_t *cli_module_app_order;
static uint32_t cli_module_app_order_count;
static char *(*cli_module_require_resolver)(const char *module_name);
static enum cli_module_graph_mode cli_module_mode;
static bool cli_module_has_app;
static char cli_module_error[CLI_MODULE_ERROR_SIZE];

static void cli_module_clear_graph(void);
static void cli_module_set_error(const char *message, const char *detail);
static void cli_module_set_frontend_error(const char *file, int line, const char *message);
static void cli_module_set_bytecode_error(const char *path, const struct bytecode_file_error *error);
static bool cli_module_name_is_valid(const char *module_name);
static bool cli_module_try_path_list(const char *path_list, const char *module_name, char **resolved_path);
static bool cli_module_try_directory(const char *directory, size_t directory_length, const char *module_name, char **resolved_path);
static bool cli_module_try_suffix(const char *directory, size_t directory_length, const char *module_name, const char *suffix, char **resolved_path);
static bool cli_module_read_file(const char *path, uint8_t **storage, size_t *size);
static bool cli_module_has_suffix(const char *path, const char *suffix);
static bool cli_module_path_is_drive_qualified(const char *path);
static char *cli_module_copy_string(const char *text);
static char *cli_module_make_app_root_source(const char *path, uint32_t root_index);
static char *cli_module_make_app_required_source(const char *path, const char *module_name);
static char *cli_module_make_logical_source(const char *path, bool explicit_root, uint32_t root_index, const char *module_name);
static int cli_module_find_artifact(const char *path);
static int cli_module_find_logical_source(const char *source);
static int cli_module_find_binding(const char *name);
static bool cli_module_grow_records(void);
static bool cli_module_grow_bindings(void);
static bool cli_module_grow_indices(uint32_t **table, uint32_t *capacity, uint32_t count);
static bool cli_module_add_artifact(const char *path, bool explicit_root, uint32_t root_index, const char *module_name, uint32_t *index);
static bool cli_module_add_input_artifact(const char *path, const uint8_t *data, size_t size, uint32_t *index);
static bool cli_module_add_binding(const char *name, uint32_t artifact_index);
static bool cli_module_copy_requires(struct cli_module_record *record, uint32_t count, const char *const name[]);
static bool cli_module_copy_source_functions(struct cli_module_record *record);
static bool cli_module_copy_bytecode_functions(struct cli_module_record *record, const struct bytecode_file_module *module);
static bool cli_module_validate_symbols(void);
static bool cli_module_mark_reachable(uint32_t artifact_index, unsigned char reachable[]);
static bool cli_module_validate_reachable_symbols(const unsigned char reachable[]);
static bool cli_module_prepare_source(uint32_t artifact_index);
static bool cli_module_prepare_bytecode(uint32_t artifact_index, enum bytecode_file_kind kind);
static bool cli_module_classify(uint32_t artifact_index);
static bool cli_module_add_bytecode_prototypes(const struct bytecode_file_module *module);
static bool cli_module_prepare(uint32_t artifact_index);
static int cli_module_find_app_binding(const char *name);
static bool cli_module_visit_app(uint32_t module_index, unsigned char state[]);
static bool cli_module_build_app(const uint8_t *data, size_t size);
static bool cli_module_register_bytecode(struct rt_env *env, const struct bytecode_file_module *module);
static bool cli_module_make_lir(const struct bytecode_file_function *function, struct lir_func *lir);

/*
 * Resets the complete CLI module resolver and owned graph state.
 */
void
cli_module_reset(
	void)
{
	cli_module_clear_graph();
	cli_module_path_count = 0;
	cli_module_error[0] = '\0';
#if defined(NOCT_USE_OPTIMIZER)
	hir_fast_checked_reset_prototypes();
#endif
}

/*
 * Appends one colon-separated CLI module search path.
 */
bool
cli_module_add_path(
	const char *path_list)
{
	if (path_list == NULL || path_list[0] == '\0')
		return false;
	if (cli_module_path_count == CLI_MODULE_PATH_MAX)
		return false;

	cli_module_path[cli_module_path_count] = path_list;
	cli_module_path_count++;

	return true;
}

/*
 * Resolves one module using directory-major source-first precedence.
 */
char *
cli_module_resolve(
	const char *module_name)
{
	char *resolved_path;
	uint32_t i;

	if (!cli_module_name_is_valid(module_name))
		return NULL;

	if (cli_module_try_directory(".", 1, module_name, &resolved_path))
		return resolved_path;

	/* Search explicit path lists in command-line order. */
	for (i = 0; i < cli_module_path_count; i++) {
		if (cli_module_try_path_list(
			cli_module_path[i],
			module_name,
			&resolved_path)) {
			return resolved_path;
		}
	}

	return NULL;
}

/*
 * Builds a side-effect-free dependency graph and shared prototype registry.
 */
bool
cli_module_build_graph(
	enum cli_module_graph_mode mode,
	uint32_t root_count,
	const char *const root_path[],
	char *(*require_resolver)(const char *module_name))
{
	uint32_t artifact_index;
	uint32_t i;
	bool succeeded;

	cli_module_clear_graph();
	cli_module_error[0] = '\0';
#if defined(NOCT_USE_OPTIMIZER)
	hir_fast_checked_reset_prototypes();
#endif
	cli_module_mode = mode;
	cli_module_require_resolver = require_resolver;
	succeeded = false;

	if (root_count == 0 || root_path == NULL) {
		cli_module_set_error(N_TR("No input module was specified."), NULL);
		goto cleanup;
	}

	/* Add every explicit root before traversing dependency edges. */
	for (i = 0; i < root_count; i++) {
		if (root_path[i] == NULL || root_path[i][0] == '\0') {
			cli_module_set_error(N_TR("Invalid input module path."), NULL);
			goto cleanup;
		}
		if (cli_module_find_artifact(root_path[i]) >= 0) {
			if (mode == CLI_MODULE_GRAPH_APP) {
				cli_module_set_error(
					N_TR("Duplicate Noct App input: %s"),
					root_path[i]);
			} else {
				cli_module_set_error(
					N_TR("Duplicate input module path: %s"),
					root_path[i]);
			}
			goto cleanup;
		}
		if (!cli_module_add_artifact(
			root_path[i],
			true,
			i,
			NULL,
			&artifact_index)) {
			goto cleanup;
		}

		if (!cli_module_grow_indices(
			&cli_module_root,
			&cli_module_root_capacity,
			cli_module_root_count + 1)) {
			cli_module_set_error(N_TR("Out of memory building module graph."), NULL);
			goto cleanup;
		}
		cli_module_root[cli_module_root_count] = artifact_index;
		cli_module_root_count++;
	}

	/* Prepare each explicit root and its complete transitive closure. */
	for (i = 0; i < cli_module_root_count; i++) {
		if (!cli_module_prepare(cli_module_root[i]))
			goto cleanup;
	}
	if (!cli_module_validate_symbols())
		goto cleanup;

	succeeded = true;

cleanup:
	if (!succeeded) {
		char saved_error[CLI_MODULE_ERROR_SIZE];

		memcpy(saved_error, cli_module_error, sizeof(saved_error));
		cli_module_clear_graph();
#if defined(NOCT_USE_OPTIMIZER)
		hir_fast_checked_reset_prototypes();
#endif
		memcpy(cli_module_error, saved_error, sizeof(cli_module_error));
	}

	return succeeded;
}

/*
 * Builds the run graph around an input that the command has already read.
 */
bool
cli_module_build_input_graph(
	const char *root_path,
	const uint8_t *data,
	size_t size,
	char *(*require_resolver)(const char *module_name))
{
	enum bytecode_file_kind kind;
	uint32_t artifact_index;
	bool succeeded;

	cli_module_clear_graph();
	cli_module_error[0] = '\0';
#if defined(NOCT_USE_OPTIMIZER)
	hir_fast_checked_reset_prototypes();
#endif
	cli_module_mode = CLI_MODULE_GRAPH_RUN;
	cli_module_require_resolver = require_resolver;
	succeeded = false;

	if (root_path == NULL ||
	    root_path[0] == '\0' ||
	    data == NULL) {
		cli_module_set_error(N_TR("Invalid input module."), NULL);
		goto cleanup;
	}

	kind = bytecode_file_detect(data, size);
	if (kind == BYTECODE_FILE_APP_1_0) {
		if (!cli_module_build_app(data, size))
			goto cleanup;
		succeeded = true;
		goto cleanup;
	}
	if (kind == BYTECODE_FILE_APP_UNKNOWN) {
		cli_module_set_error(
			N_TR("Unsupported or malformed application version: %s"),
			root_path);
		goto cleanup;
	}

	if (!cli_module_add_input_artifact(
		root_path,
		data,
		size,
		&artifact_index)) {
		goto cleanup;
	}
	if (!cli_module_grow_indices(
		&cli_module_root,
		&cli_module_root_capacity,
		1)) {
		cli_module_set_error(N_TR("Out of memory building module graph."), NULL);
		goto cleanup;
	}
	cli_module_root[0] = artifact_index;
	cli_module_root_count = 1;

	if (!cli_module_prepare(artifact_index))
		goto cleanup;
	if (!cli_module_validate_symbols())
		goto cleanup;

	succeeded = true;

cleanup:
	if (!succeeded) {
		char saved_error[CLI_MODULE_ERROR_SIZE];

		memcpy(saved_error, cli_module_error, sizeof(saved_error));
		cli_module_clear_graph();
#if defined(NOCT_USE_OPTIMIZER)
		hir_fast_checked_reset_prototypes();
#endif
		memcpy(cli_module_error, saved_error, sizeof(cli_module_error));
	}

	return succeeded;
}

/*
 * Registers a fully inspected graph in dependency-first order.
 */
bool
cli_module_register_graph(
	struct rt_env *env)
{
	const struct cli_module_artifact *artifact;
	struct cli_module_record *record;
	uint32_t artifact_index;
	uint32_t i;

	if (env == NULL)
		return false;

	if (cli_module_has_app) {
		/* Register every embedded module in dependency order. */
		for (i = 0; i < cli_module_app_order_count; i++) {
			if (!cli_module_register_bytecode(
				env,
				&cli_module_app.module[
					cli_module_app_order[i]])) {
				return false;
			}
		}

		return true;
	}

	/* Register every filesystem artifact in dependency order. */
	for (i = 0; i < cli_module_postorder_count; i++) {
		artifact_index = cli_module_postorder[i];
		record = &cli_module_record_table[artifact_index];
		artifact = &record->artifact;

		if (artifact->kind == CLI_MODULE_SOURCE) {
			if (!noct_register_source(
				env,
				artifact->logical_source,
				(const char *)artifact->data)) {
				return false;
			}
		} else if (artifact->kind == CLI_MODULE_BYTECODE) {
			if (!cli_module_register_bytecode(env, &record->bytecode))
				return false;
		} else {
			return false;
		}
	}

	return true;
}

/*
 * Returns the number of owned graph artifacts.
 */
uint32_t
cli_module_get_artifact_count(
	void)
{
	return cli_module_record_count;
}

/*
 * Returns one borrowed graph artifact descriptor.
 */
const struct cli_module_artifact *
cli_module_get_artifact(
	uint32_t index)
{
	if (index >= cli_module_record_count)
		return NULL;

	return &cli_module_record_table[index].artifact;
}

/*
 * Returns the dependency-first artifact count.
 */
uint32_t
cli_module_get_postorder_count(
	void)
{
	return cli_module_postorder_count;
}

/*
 * Returns one artifact index from dependency-first order.
 */
uint32_t
cli_module_get_postorder_artifact(
	uint32_t index)
{
	assert(index < cli_module_postorder_count);

	return cli_module_postorder[index];
}

/*
 * Returns the number of module-name bindings.
 */
uint32_t
cli_module_get_binding_count(
	void)
{
	return cli_module_binding_count;
}

/*
 * Returns one borrowed module-name binding.
 */
const struct cli_module_binding *
cli_module_get_binding(
	uint32_t index)
{
	if (index >= cli_module_binding_count)
		return NULL;

	return &cli_module_binding_table[index].binding;
}

/*
 * Returns the number of explicit root modules.
 */
uint32_t
cli_module_get_root_count(
	void)
{
	return cli_module_root_count;
}

/*
 * Returns one explicit root artifact index.
 */
uint32_t
cli_module_get_root_artifact(
	uint32_t index)
{
	assert(index < cli_module_root_count);

	return cli_module_root[index];
}

/*
 * Returns the last graph-build diagnostic.
 */
const char *
cli_module_get_error(
	void)
{
	return cli_module_error;
}

/* Release every graph-owned artifact, binding, and index table. */
static void
cli_module_clear_graph(
	void)
{
	struct cli_module_record *record;
	uint32_t i;
	uint32_t j;

	/* Release every artifact and its detached metadata. */
	for (i = 0; i < cli_module_record_count; i++) {
		record = &cli_module_record_table[i];
		bytecode_file_cleanup_module(&record->bytecode);

		/* Release source require names copied out of the AST. */
		if (record->require_name != NULL) {
			/* Release each name copied before any partial failure. */
			for (j = 0; j < record->require_count; j++)
				free(record->require_name[j]);
		}
		free(record->require_name);

		/* Release every exact function link name copied for validation. */
		for (j = 0; j < record->function_count; j++)
			free(record->function_name[j]);
		free(record->function_name);
		free((char *)record->artifact.logical_source);
		free((char *)record->artifact.physical_path);
		if (record->owns_storage)
			free(record->storage);
	}
	free(cli_module_record_table);
	cli_module_record_table = NULL;
	cli_module_record_count = 0;
	cli_module_record_capacity = 0;

	/* Release every binding name owned by the graph. */
	for (i = 0; i < cli_module_binding_count; i++)
		free(cli_module_binding_table[i].name);
	free(cli_module_binding_table);
	cli_module_binding_table = NULL;
	cli_module_binding_count = 0;
	cli_module_binding_capacity = 0;

	free(cli_module_postorder);
	cli_module_postorder = NULL;
	cli_module_postorder_count = 0;
	cli_module_postorder_capacity = 0;

	free(cli_module_root);
	cli_module_root = NULL;
	cli_module_root_count = 0;
	cli_module_root_capacity = 0;

	bytecode_file_cleanup_app(&cli_module_app);
	memset(&cli_module_app, 0, sizeof(cli_module_app));
	free(cli_module_app_order);
	cli_module_app_order = NULL;
	cli_module_app_order_count = 0;
	cli_module_require_resolver = NULL;
	cli_module_has_app = false;
}

/* Store one fixed diagnostic with an optional string detail. */
static void
cli_module_set_error(
	const char *message,
	const char *detail)
{
	if (cli_module_error[0] != '\0')
		return;

	if (detail == NULL) {
		snprintf(
			cli_module_error,
			sizeof(cli_module_error),
			"%s",
			message);
	} else {
		snprintf(
			cli_module_error,
			sizeof(cli_module_error),
			message,
			detail);
	}
}

/* Preserve one exact frontend diagnostic across parser cleanup. */
static void
cli_module_set_frontend_error(
	const char *file,
	int line,
	const char *message)
{
	if (cli_module_error[0] != '\0')
		return;

	snprintf(
		cli_module_error,
		sizeof(cli_module_error),
		"%s:%d: %s",
		file != NULL ? file : "",
		line,
		message != NULL ? message : "Frontend compilation failed.");
}

/* Preserve one exact CLI bytecode inspector diagnostic. */
static void
cli_module_set_bytecode_error(
	const char *path,
	const struct bytecode_file_error *error)
{
	if (cli_module_error[0] != '\0')
		return;

	snprintf(
		cli_module_error,
		sizeof(cli_module_error),
		"Malformed bytecode module %s at byte %lu: %s",
		path,
		(unsigned long)error->offset,
		error->message);
}

/* Check whether a module name is safe to append to a directory. */
static bool
cli_module_name_is_valid(
	const char *module_name)
{
	const unsigned char *cursor;

	if (module_name == NULL || module_name[0] == '\0')
		return false;
	if (module_name[0] >= '0' && module_name[0] <= '9')
		return false;

	cursor = (const unsigned char *)module_name;

	/* Accept the same ASCII identifier characters as the lexer. */
	while (*cursor != '\0') {
		if (!(*cursor >= 'A' && *cursor <= 'Z') &&
		    !(*cursor >= 'a' && *cursor <= 'z') &&
		    !(*cursor >= '0' && *cursor <= '9') &&
		    *cursor != '_') {
			return false;
		}
		cursor++;
	}

	return true;
}

/* Search one colon-separated path list. */
static bool
cli_module_try_path_list(
	const char *path_list,
	const char *module_name,
	char **resolved_path)
{
	const char *start;
	const char *cursor;
	bool drive_colon;

	start = path_list;
	cursor = path_list;

	/* Search each non-empty directory in the path list. */
	for (;;) {
		drive_colon = false;
		if (*cursor == ':' &&
		    cursor == start + 1 &&
		    ((start[0] >= 'A' && start[0] <= 'Z') ||
		     (start[0] >= 'a' && start[0] <= 'z')) &&
		    (cursor[1] == '/' || cursor[1] == '\\')) {
			drive_colon = true;
		}

		if ((*cursor == ':' && !drive_colon) || *cursor == '\0') {
			if (cursor != start) {
				if (cli_module_try_directory(
					start,
					(size_t)(cursor - start),
					module_name,
					resolved_path)) {
					return true;
				}
			}

			if (*cursor == '\0')
				break;
			start = cursor + 1;
		}

		cursor++;
	}

	return false;
}

/* Search source suffixes and then bytecode within one directory. */
static bool
cli_module_try_directory(
	const char *directory,
	size_t directory_length,
	const char *module_name,
	char **resolved_path)
{
	if (cli_module_try_suffix(
		directory,
		directory_length,
		module_name,
		".noct",
		resolved_path)) {
		return true;
	}
	if (cli_module_try_suffix(
		directory,
		directory_length,
		module_name,
		".nct",
		resolved_path)) {
		return true;
	}
	if (cli_module_try_suffix(
		directory,
		directory_length,
		module_name,
		".nbc",
		resolved_path)) {
		return true;
	}

	return false;
}

/* Test one directory, module name, and suffix. */
static bool
cli_module_try_suffix(
	const char *directory,
	size_t directory_length,
	const char *module_name,
	const char *suffix,
	char **resolved_path)
{
	FILE *stream;
	char *candidate;
	char *output;
	size_t module_length;
	size_t suffix_length;
	size_t candidate_size;
	bool needs_separator;

	module_length = strlen(module_name);
	suffix_length = strlen(suffix);
	needs_separator = directory_length != 0 &&
		directory[directory_length - 1] != '/' &&
		directory[directory_length - 1] != '\\';

	if (directory_length > SIZE_MAX - module_length)
		return false;
	candidate_size = directory_length + module_length;
	if (needs_separator) {
		if (candidate_size == SIZE_MAX)
			return false;
		candidate_size++;
	}
	if (candidate_size > SIZE_MAX - suffix_length)
		return false;
	candidate_size += suffix_length;
	if (candidate_size == SIZE_MAX)
		return false;
	candidate_size++;

	candidate = malloc(candidate_size);
	if (candidate == NULL)
		return false;

	output = candidate;
	memcpy(output, directory, directory_length);
	output += directory_length;
	if (needs_separator) {
		*output = '/';
		output++;
	}
	memcpy(output, module_name, module_length);
	output += module_length;
	memcpy(output, suffix, suffix_length + 1);

	stream = fopen(candidate, "rb");
	if (stream == NULL) {
		free(candidate);
		return false;
	}
	if (fclose(stream) != 0) {
		free(candidate);
		return false;
	}

	*resolved_path = candidate;

	return true;
}

/* Read one complete binary artifact into independently owned memory. */
static bool
cli_module_read_file(
	const char *path,
	uint8_t **storage,
	size_t *size)
{
	FILE *stream;
	long length;
	size_t read_size;
	bool succeeded;

	*storage = NULL;
	*size = 0;
	succeeded = false;

	stream = fopen(path, "rb");
	if (stream == NULL) {
		cli_module_set_error(N_TR("Cannot open module %s."), path);
		return false;
	}

	if (fseek(stream, 0, SEEK_END) != 0)
		goto cleanup;
	length = ftell(stream);
	if (length < 0)
		goto cleanup;
	if (fseek(stream, 0, SEEK_SET) != 0)
		goto cleanup;

	read_size = (size_t)length;
	if ((long)read_size != length)
		goto cleanup;
	if (read_size == SIZE_MAX)
		goto cleanup;

	*storage = malloc(read_size + 1);
	if (*storage == NULL) {
		cli_module_set_error(N_TR("Out of memory reading module %s."), path);
		goto cleanup;
	}
	if (fread(*storage, 1, read_size, stream) != read_size)
		goto cleanup;
	(*storage)[read_size] = '\0';
	*size = read_size;
	succeeded = true;

cleanup:
	if (fclose(stream) != 0)
		succeeded = false;
	if (!succeeded) {
		free(*storage);
		*storage = NULL;
		*size = 0;
		cli_module_set_error(N_TR("Cannot read module %s."), path);
	}

	return succeeded;
}

/* Test one exact case-sensitive path suffix. */
static bool
cli_module_has_suffix(
	const char *path,
	const char *suffix)
{
	size_t path_length;
	size_t suffix_length;

	path_length = strlen(path);
	suffix_length = strlen(suffix);
	if (path_length < suffix_length)
		return false;

	return strcmp(path + path_length - suffix_length, suffix) == 0;
}

/* Check whether one path starts with a Windows drive qualification. */
static bool
cli_module_path_is_drive_qualified(
	const char *path)
{
	if (path[0] == '\0' || path[1] == '\0')
		return false;
	if (path[1] != ':')
		return false;
	if (path[0] >= 'A' && path[0] <= 'Z')
		return true;
	if (path[0] >= 'a' && path[0] <= 'z')
		return true;

	return false;
}

/* Copy one NUL-terminated string with checked allocation arithmetic. */
static char *
cli_module_copy_string(
	const char *text)
{
	char *copy;
	size_t length;

	length = strlen(text);
	if (length == SIZE_MAX)
		return NULL;

	copy = malloc(length + 1);
	if (copy == NULL)
		return NULL;
	memcpy(copy, text, length + 1);

	return copy;
}

/* Derive one portable logical name for an explicit app root. */
static char *
cli_module_make_app_root_source(
	const char *path,
	uint32_t root_index)
{
	const char *base;
	const char *component;
	const char *cursor;
	const char *slash;
	const char *backslash;
	char prefix[32];
	char *source;
	char *output;
	size_t path_length;
	size_t component_length;
	size_t prefix_length;
	size_t base_length;
	int prefix_result;

	path_length = strlen(path);
	if (path_length == 0 || path_length == SIZE_MAX)
		return NULL;
	if (strchr(path, '\n') != NULL || strchr(path, '\r') != NULL)
		return NULL;

	if (path[0] == '/' || path[0] == '\\' ||
	    cli_module_path_is_drive_qualified(path)) {
		base = path;
		slash = strrchr(path, '/');
		backslash = strrchr(path, '\\');
		if (slash != NULL)
			base = slash + 1;
		if (backslash != NULL && backslash + 1 > base)
			base = backslash + 1;
		if (base[0] == '\0')
			return NULL;

		prefix_result = snprintf(
			prefix,
			sizeof(prefix),
			"roots/%u/",
			root_index);
		if (prefix_result < 0 ||
		    (size_t)prefix_result >= sizeof(prefix)) {
			return NULL;
		}
		prefix_length = (size_t)prefix_result;
		base_length = strlen(base);
		if (prefix_length > SIZE_MAX - base_length)
			return NULL;
		if (prefix_length + base_length == SIZE_MAX)
			return NULL;

		source = malloc(prefix_length + base_length + 1);
		if (source == NULL)
			return NULL;
		memcpy(source, prefix, prefix_length);
		memcpy(source + prefix_length, base, base_length + 1);

		return source;
	}

	source = malloc(path_length + 1);
	if (source == NULL)
		return NULL;
	output = source;
	component = path;
	cursor = path;

	/* Normalize separators and remove empty and current-directory parts. */
	for (;;) {
		if (*cursor == '/' ||
		    *cursor == '\\' ||
		    *cursor == '\0') {
			component_length = (size_t)(cursor - component);
			if (component_length == 2 &&
			    component[0] == '.' &&
			    component[1] == '.') {
				free(source);
				return NULL;
			}
			if (component_length != 0 &&
			    !(component_length == 1 && component[0] == '.')) {
				if (output != source) {
					*output = '/';
					output++;
				}
				memcpy(output, component, component_length);
				output += component_length;
			}

			if (*cursor == '\0')
				break;
			component = cursor + 1;
		}
		cursor++;
	}

	if (output == source) {
		free(source);
		return NULL;
	}
	*output = '\0';

	return source;
}

/* Derive one deterministic portable name for a required app artifact. */
static char *
cli_module_make_app_required_source(
	const char *path,
	const char *module_name)
{
	const char *suffix;
	char *source;
	size_t prefix_length;
	size_t name_length;
	size_t suffix_length;
	size_t source_length;

	suffix = NULL;
	if (cli_module_has_suffix(path, ".noct"))
		suffix = ".noct";
	else if (cli_module_has_suffix(path, ".nct"))
		suffix = ".nct";
	else if (cli_module_has_suffix(path, ".nbc"))
		suffix = ".nbc";
	if (suffix == NULL || module_name == NULL)
		return NULL;

	prefix_length = strlen("modules/");
	name_length = strlen(module_name);
	suffix_length = strlen(suffix);
	if (prefix_length > SIZE_MAX - name_length)
		return NULL;
	source_length = prefix_length + name_length;
	if (source_length > SIZE_MAX - suffix_length)
		return NULL;
	source_length += suffix_length;
	if (source_length == SIZE_MAX)
		return NULL;

	source = malloc(source_length + 1);
	if (source == NULL)
		return NULL;
	memcpy(source, "modules/", prefix_length);
	memcpy(source + prefix_length, module_name, name_length);
	memcpy(source + prefix_length + name_length, suffix, suffix_length + 1);

	return source;
}

/* Derive one logical source identity for the active graph mode. */
static char *
cli_module_make_logical_source(
	const char *path,
	bool explicit_root,
	uint32_t root_index,
	const char *module_name)
{
	if (explicit_root)
		return cli_module_make_app_root_source(path, root_index);

	return cli_module_make_app_required_source(path, module_name);
}

/* Find an artifact by the resolver's exact constructed path identity. */
static int
cli_module_find_artifact(
	const char *path)
{
	uint32_t i;

	/* Search every already retained physical path. */
	for (i = 0; i < cli_module_record_count; i++) {
		if (strcmp(
			cli_module_record_table[i].artifact.physical_path,
			path) == 0) {
			return (int)i;
		}
	}

	return -1;
}

/* Find an artifact by its portable application source identity. */
static int
cli_module_find_logical_source(
	const char *source)
{
	uint32_t i;

	/* Compare each logical identity without normalizing it again. */
	for (i = 0; i < cli_module_record_count; i++) {
		if (strcmp(
			cli_module_record_table[i].artifact.logical_source,
			source) == 0) {
			return (int)i;
		}
	}

	return -1;
}

/* Find a require alias without invoking the resolver again. */
static int
cli_module_find_binding(
	const char *name)
{
	uint32_t i;

	/* Search every already resolved require alias. */
	for (i = 0; i < cli_module_binding_count; i++) {
		if (strcmp(cli_module_binding_table[i].name, name) == 0)
			return (int)i;
	}

	return -1;
}

/* Grow the artifact table for one additional record. */
static bool
cli_module_grow_records(
	void)
{
	struct cli_module_record *table;
	uint32_t capacity;

	if (cli_module_record_count < cli_module_record_capacity)
		return true;

	capacity = cli_module_record_capacity == 0 ?
		8 : cli_module_record_capacity * 2;
	if (capacity < cli_module_record_capacity)
		return false;
	if (sizeof(*table) > SIZE_MAX / (size_t)capacity)
		return false;

	table = realloc(
		cli_module_record_table,
		(size_t)capacity * sizeof(*table));
	if (table == NULL)
		return false;

	cli_module_record_table = table;
	cli_module_record_capacity = capacity;

	return true;
}

/* Grow the require-binding table for one additional alias. */
static bool
cli_module_grow_bindings(
	void)
{
	struct cli_module_owned_binding *table;
	uint32_t capacity;

	if (cli_module_binding_count < cli_module_binding_capacity)
		return true;

	capacity = cli_module_binding_capacity == 0 ?
		8 : cli_module_binding_capacity * 2;
	if (capacity < cli_module_binding_capacity)
		return false;
	if (sizeof(*table) > SIZE_MAX / (size_t)capacity)
		return false;

	table = realloc(
		cli_module_binding_table,
		(size_t)capacity * sizeof(*table));
	if (table == NULL)
		return false;

	cli_module_binding_table = table;
	cli_module_binding_capacity = capacity;

	return true;
}

/* Grow one uint32 index table to at least the requested count. */
static bool
cli_module_grow_indices(
	uint32_t **table,
	uint32_t *capacity,
	uint32_t count)
{
	uint32_t *new_table;
	uint32_t new_capacity;

	if (count <= *capacity)
		return true;

	new_capacity = *capacity == 0 ? 8 : *capacity;

	/* Double until the requested index count fits. */
	while (new_capacity < count) {
		if (new_capacity > UINT32_MAX / 2U)
			return false;
		new_capacity *= 2U;
	}
	if (sizeof(*new_table) > SIZE_MAX / (size_t)new_capacity)
		return false;

	new_table = realloc(
		*table,
		(size_t)new_capacity * sizeof(*new_table));
	if (new_table == NULL)
		return false;

	*table = new_table;
	*capacity = new_capacity;

	return true;
}

/* Read and retain one artifact without classifying its payload yet. */
static bool
cli_module_add_artifact(
	const char *path,
	bool explicit_root,
	uint32_t root_index,
	const char *module_name,
	uint32_t *index)
{
	struct cli_module_record *record;
	char *physical_path;
	char *logical_source;
	uint8_t *storage;
	size_t size;
	size_t path_length;

	physical_path = NULL;
	logical_source = NULL;
	storage = NULL;
	size = 0;

	if (!cli_module_grow_records()) {
		cli_module_set_error(N_TR("Out of memory building module graph."), NULL);
		return false;
	}
	if (!cli_module_read_file(path, &storage, &size))
		return false;

	path_length = strlen(path);
	if (path_length == SIZE_MAX)
		goto oom;
	physical_path = malloc(path_length + 1);
	if (physical_path == NULL)
		goto oom;
	memcpy(physical_path, path, path_length + 1);

	logical_source = cli_module_make_logical_source(
		path,
		explicit_root,
		root_index,
		module_name);
	if (logical_source == NULL) {
		free(physical_path);
		free(storage);
		cli_module_set_error(
			N_TR("Cannot derive a portable source name for %s."),
			path);
		return false;
	}
	if (cli_module_mode == CLI_MODULE_GRAPH_APP &&
	    cli_module_find_logical_source(logical_source) >= 0) {
		free(logical_source);
		free(physical_path);
		free(storage);
		cli_module_set_error(
			N_TR("Duplicate logical source name for %s."),
			path);
		return false;
	}

	record = &cli_module_record_table[cli_module_record_count];
	memset(record, 0, sizeof(*record));
	record->artifact.physical_path = physical_path;
	record->artifact.logical_source = logical_source;
	record->artifact.data = storage;
	record->artifact.data_size = size;
	record->artifact.is_explicit_root = explicit_root;
	record->storage = storage;
	record->storage_size = size;
	record->owns_storage = true;

	*index = cli_module_record_count;
	cli_module_record_count++;

	return true;

oom:
	free(logical_source);
	free(physical_path);
	free(storage);
	cli_module_set_error(N_TR("Out of memory building module graph."), NULL);

	return false;
}

/* Retain an already-read root without taking ownership of its byte buffer. */
static bool
cli_module_add_input_artifact(
	const char *path,
	const uint8_t *data,
	size_t size,
	uint32_t *index)
{
	struct cli_module_record *record;
	char *physical_path;
	char *logical_source;

	physical_path = NULL;
	logical_source = NULL;

	if (!cli_module_grow_records())
		goto oom;

	physical_path = cli_module_copy_string(path);
	if (physical_path == NULL)
		goto oom;
	logical_source = cli_module_make_logical_source(
		path,
		true,
		0,
		NULL);
	if (logical_source == NULL)
		goto oom;

	record = &cli_module_record_table[cli_module_record_count];
	memset(record, 0, sizeof(*record));
	record->artifact.physical_path = physical_path;
	record->artifact.logical_source = logical_source;
	record->artifact.data = data;
	record->artifact.data_size = size;
	record->artifact.is_explicit_root = true;
	record->storage = (uint8_t *)(void *)data;
	record->storage_size = size;
	record->owns_storage = false;

	*index = cli_module_record_count;
	cli_module_record_count++;

	return true;

oom:
	free(logical_source);
	free(physical_path);
	cli_module_set_error(N_TR("Out of memory building module graph."), NULL);

	return false;
}

/* Retain one unique require alias and its artifact index. */
static bool
cli_module_add_binding(
	const char *name,
	uint32_t artifact_index)
{
	struct cli_module_owned_binding *binding;
	char *copy;

	if (!cli_module_grow_bindings())
		return false;

	copy = malloc(strlen(name) + 1);
	if (copy == NULL)
		return false;
	strcpy(copy, name);

	binding = &cli_module_binding_table[cli_module_binding_count];
	memset(binding, 0, sizeof(*binding));
	binding->name = copy;
	binding->binding.module_name = copy;
	binding->binding.artifact_index = artifact_index;
	cli_module_binding_count++;

	return true;
}

/* Copy one require list beyond the parser or AST lifetime. */
static bool
cli_module_copy_requires(
	struct cli_module_record *record,
	uint32_t count,
	const char *const name[])
{
	uint32_t i;

	record->require_count = count;
	if (count == 0)
		return true;
	if (name == NULL)
		return false;
	if (sizeof(*record->require_name) > SIZE_MAX / (size_t)count)
		return false;

	record->require_name = calloc(
		(size_t)count,
		sizeof(*record->require_name));
	if (record->require_name == NULL)
		return false;

	/* Copy every require name in declaration order. */
	for (i = 0; i < count; i++) {
		record->require_name[i] = malloc(strlen(name[i]) + 1);
		if (record->require_name[i] == NULL)
			return false;
		strcpy(record->require_name[i], name[i]);
	}

	return true;
}

/* Copy every source AST function link name for closure validation. */
static bool
cli_module_copy_source_functions(
	struct cli_module_record *record)
{
	struct ast_func_list *function_list;
	struct ast_func *function;
	uint32_t count;
	uint32_t i;

	function_list = ast_get_func_list();
	count = 0;

	/* Count every user, static, and synthesized initializer function. */
	for (function = function_list->list;
	     function != NULL;
	     function = function->next) {
		if (count == UINT32_MAX)
			return false;
		count++;
	}

	record->function_count = count;
	if (count == 0)
		return true;
	if (sizeof(*record->function_name) > SIZE_MAX / (size_t)count)
		return false;

	record->function_name = calloc(
		(size_t)count,
		sizeof(*record->function_name));
	if (record->function_name == NULL)
		return false;

	function = function_list->list;

	/* Deep-copy names before releasing the AST arena. */
	for (i = 0; i < count; i++) {
		if (function == NULL || function->name == NULL)
			return false;
		record->function_name[i] = cli_module_copy_string(function->name);
		if (record->function_name[i] == NULL)
			return false;
		function = function->next;
	}

	return true;
}

/* Copy every inspected bytecode function link name for validation. */
static bool
cli_module_copy_bytecode_functions(
	struct cli_module_record *record,
	const struct bytecode_file_module *module)
{
	uint32_t i;

	record->function_count = module->function_count;
	if (module->function_count == 0)
		return true;
	if (sizeof(*record->function_name) >
	    SIZE_MAX / (size_t)module->function_count) {
		return false;
	}

	record->function_name = calloc(
		(size_t)module->function_count,
		sizeof(*record->function_name));
	if (record->function_name == NULL)
		return false;

	/* Deep-copy every exact name from the detached descriptor. */
	for (i = 0; i < module->function_count; i++) {
		record->function_name[i] = cli_module_copy_string(
			module->function[i].name);
		if (record->function_name[i] == NULL)
			return false;
	}

	return true;
}

/* Validate each independently loadable root closure's link names. */
static bool
cli_module_validate_symbols(
	void)
{
	unsigned char *reachable;
	uint32_t i;
	bool succeeded;

	reachable = calloc((size_t)cli_module_record_count, sizeof(*reachable));
	if (reachable == NULL) {
		cli_module_set_error(N_TR("Out of memory validating module graph."), NULL);
		return false;
	}
	succeeded = false;

	if (cli_module_mode == CLI_MODULE_GRAPH_APP) {
		/* Mark the union embedded in one application container. */
		for (i = 0; i < cli_module_root_count; i++) {
			if (!cli_module_mark_reachable(cli_module_root[i], reachable))
				goto cleanup;
		}
		if (!cli_module_validate_reachable_symbols(reachable))
			goto cleanup;
	} else {
		/* Validate every standalone root and its dependencies separately. */
		for (i = 0; i < cli_module_root_count; i++) {
			memset(
				reachable,
				0,
				(size_t)cli_module_record_count * sizeof(*reachable));
			if (!cli_module_mark_reachable(cli_module_root[i], reachable))
				goto cleanup;
			if (!cli_module_validate_reachable_symbols(reachable))
				goto cleanup;
		}
	}

	succeeded = true;

cleanup:
	free(reachable);

	return succeeded;
}

/* Mark one artifact's complete require closure. */
static bool
cli_module_mark_reachable(
	uint32_t artifact_index,
	unsigned char reachable[])
{
	struct cli_module_record *record;
	int binding_index;
	uint32_t dependency_index;
	uint32_t i;

	if (artifact_index >= cli_module_record_count)
		return false;
	if (reachable[artifact_index] != 0)
		return true;

	reachable[artifact_index] = 1;
	record = &cli_module_record_table[artifact_index];

	/* Follow every already resolved require binding. */
	for (i = 0; i < record->require_count; i++) {
		binding_index = cli_module_find_binding(record->require_name[i]);
		if (binding_index < 0)
			return false;
		dependency_index = cli_module_binding_table[
			(uint32_t)binding_index].binding.artifact_index;
		if (!cli_module_mark_reachable(dependency_index, reachable))
			return false;
	}

	return true;
}

/* Validate exact names inside one marked root closure. */
static bool
cli_module_validate_reachable_symbols(
	const unsigned char reachable[])
{
	struct cli_module_record *record;
	const char *name;
	uint32_t initializer_count;
	uint32_t i;
	uint32_t j;
	uint32_t previous_artifact;
	uint32_t previous_function;
	uint32_t limit;

	/* Check every artifact belonging to this root closure. */
	for (i = 0; i < cli_module_record_count; i++) {
		if (reachable[i] == 0)
			continue;

		record = &cli_module_record_table[i];
		initializer_count = 0;

		/* Compare each exact link name with every preceding function. */
		for (j = 0; j < record->function_count; j++) {
			name = record->function_name[j];
			if (strncmp(name, "$init.", 6) == 0)
				initializer_count++;

			/* Search preceding functions in this root closure. */
			for (previous_artifact = 0;
			     previous_artifact <= i;
			     previous_artifact++) {
				if (reachable[previous_artifact] == 0)
					continue;

				limit = cli_module_record_table[
					previous_artifact].function_count;
				if (previous_artifact == i)
					limit = j;

				/* Search every preceding name in this artifact. */
				for (previous_function = 0;
				     previous_function < limit;
				     previous_function++) {
					if (strcmp(
						cli_module_record_table[
							previous_artifact].function_name[
							previous_function],
						name) == 0) {
							cli_module_set_error(
								N_TR("Duplicate public symbol \"%s\" (Duplicate function in module closure)."),
								name);
						return false;
					}
				}
			}
		}

		if (initializer_count > 1) {
			cli_module_set_error(
				N_TR("Multiple initializers in module: %s"),
				record->artifact.physical_path);
			return false;
		}
	}

	return true;
}

/* Parse one source artifact and collect its external prototypes. */
static bool
cli_module_prepare_source(
	uint32_t artifact_index)
{
	struct cli_module_record *record;
	const char **require_name;
	uint32_t require_count;
	uint32_t i;
	bool succeeded;

	record = &cli_module_record_table[artifact_index];
	require_name = NULL;
	require_count = 0;
	succeeded = false;

	if (!ast_build(
		record->artifact.logical_source,
		(const char *)record->artifact.data)) {
		cli_module_set_frontend_error(
			ast_get_file_name(),
			ast_get_error_line(),
			ast_get_error_message());
		ast_cleanup();
		return false;
	}

#if defined(NOCT_USE_OPTIMIZER)
	if (!hir_fast_checked_collect_prototypes()) {
		cli_module_set_frontend_error(
			hir_get_file_name(),
			hir_get_error_line(),
			hir_get_error_message());
		goto cleanup;
	}
#endif
	if (!cli_module_copy_source_functions(record))
		goto cleanup;

	require_count = ast_get_require_count();
	if (require_count > 0) {
		if (sizeof(*require_name) > SIZE_MAX / (size_t)require_count)
			goto cleanup;

		require_name = malloc((size_t)require_count * sizeof(*require_name));
		if (require_name == NULL)
			goto cleanup;
	}

	/* Borrow every AST require name for one immediate deep copy. */
	for (i = 0; i < require_count; i++)
		require_name[i] = ast_get_require_name(i);

	if (!cli_module_copy_requires(record, require_count, require_name))
		goto cleanup;

	record->artifact.kind = CLI_MODULE_SOURCE;
	succeeded = true;

cleanup:
	free(require_name);
	ast_cleanup();

	if (!succeeded) {
		cli_module_set_error(
			N_TR("Cannot inspect source module %s."),
			record->artifact.physical_path);
	}

	return succeeded;
}

/* Parse one detached bytecode artifact and collect its prototypes. */
static bool
cli_module_prepare_bytecode(
	uint32_t artifact_index,
	enum bytecode_file_kind kind)
{
	struct cli_module_record *record;
	struct bytecode_file_error error;

	record = &cli_module_record_table[artifact_index];
	if (cli_module_mode == CLI_MODULE_GRAPH_APP &&
	    kind == BYTECODE_FILE_MODULE_1_0) {
		cli_module_set_error(
			N_TR("Legacy 1.0 bytecode cannot be embedded in an application: %s"),
			record->artifact.physical_path);
		return false;
	}

	if (!bytecode_file_inspect_module(
		record->artifact.data,
		record->artifact.data_size,
		&record->bytecode,
		&error)) {
		cli_module_set_bytecode_error(
			record->artifact.physical_path,
			&error);
		return false;
	}
	if (!cli_module_copy_bytecode_functions(record, &record->bytecode)) {
		cli_module_set_error(
			N_TR("Cannot collect bytecode link names from %s."),
			record->artifact.physical_path);
		return false;
	}

	if (!cli_module_add_bytecode_prototypes(&record->bytecode)) {
		cli_module_set_error(
			N_TR("Cannot collect bytecode prototypes from %s."),
			record->artifact.physical_path);
		return false;
	}
	if (!cli_module_copy_requires(
		record,
		record->bytecode.require_count,
		(const char *const *)record->bytecode.require_name)) {
		cli_module_set_error(N_TR("Out of memory building module graph."), NULL);
		return false;
	}

	record->artifact.kind = CLI_MODULE_BYTECODE;

	return true;
}

/* Classify one raw artifact after exact executable-shebang stripping. */
static bool
cli_module_classify(
	uint32_t artifact_index)
{
	struct cli_module_record *record;
	const uint8_t *payload;
	size_t payload_size;
	size_t shebang_size;
	enum bytecode_file_kind kind;
	uint32_t registration_size;
	bool has_shebang;

	record = &cli_module_record_table[artifact_index];
	payload = record->storage;
	payload_size = record->storage_size;
	shebang_size = strlen(NOCT_APP_SHEBANG);
	has_shebang = false;

	if (payload_size >= shebang_size &&
	    memcmp(payload, NOCT_APP_SHEBANG, shebang_size) == 0) {
		payload += shebang_size;
		payload_size -= shebang_size;
		has_shebang = true;
	}

	record->artifact.data = payload;
	record->artifact.data_size = payload_size;
	kind = bytecode_file_detect(payload, payload_size);

	if (cli_module_mode == CLI_MODULE_GRAPH_COMPILE &&
	    record->artifact.is_explicit_root &&
	    cli_module_has_suffix(record->artifact.physical_path, ".nbc")) {
		cli_module_set_error(
			N_TR("Bytecode input is not recompiled: %s"),
			record->artifact.physical_path);
		return false;
	}

	if (kind == BYTECODE_FILE_MODULE_UNKNOWN) {
		cli_module_set_error(
			N_TR("Unsupported or malformed bytecode version: %s"),
			record->artifact.physical_path);
		return false;
	}
	if (kind == BYTECODE_FILE_APP_UNKNOWN ||
	    kind == BYTECODE_FILE_APP_1_0) {
		cli_module_set_error(
			N_TR("Application container cannot be used as a module: %s"),
			record->artifact.physical_path);
		return false;
	}

	if (kind == BYTECODE_FILE_UNKNOWN) {
		if (memchr(payload, '\0', payload_size) != NULL) {
			cli_module_set_error(
				N_TR("NUL in source module: %s"),
				record->artifact.physical_path);
			return false;
		}

		return cli_module_prepare_source(artifact_index);
	}

	if (record->artifact.is_explicit_root &&
	    cli_module_mode == CLI_MODULE_GRAPH_COMPILE) {
		cli_module_set_error(
			N_TR("Bytecode input is not recompiled: %s"),
			record->artifact.physical_path);
		return false;
	}
	if (has_shebang && !record->artifact.is_explicit_root) {
		cli_module_set_error(
			N_TR("Executable bytecode wrapper cannot be required: %s"),
			record->artifact.physical_path);
		return false;
	}
	if (!bytecode_file_check_registration_size(
		payload_size,
		&registration_size)) {
		cli_module_set_error(
			N_TR("Bytecode module is too large: %s"),
			record->artifact.physical_path);
		return false;
	}

	return cli_module_prepare_bytecode(artifact_index, kind);
}

/* Add externally visible prototypes from one inspected bytecode module. */
static bool
cli_module_add_bytecode_prototypes(
	const struct bytecode_file_module *module)
{
#if defined(NOCT_USE_OPTIMIZER)
	const struct bytecode_file_function *function;
	const struct fast_signature *signature;
	uint32_t i;

	/* Add every non-static, non-initializer link name. */
	for (i = 0; i < module->function_count; i++) {
		function = &module->function[i];
		if (strncmp(function->name, "$static.", 8) == 0)
			continue;
		if (strncmp(function->name, "$init.", 6) == 0)
			continue;

		signature = function->is_fast ?
			fast_info_signature(function->fast_info) : NULL;
		if (!hir_fast_checked_add_prototype(
			function->name,
			function->is_fast,
			signature)) {
			return false;
		}
	}
#else
	UNUSED_PARAMETER(module);
#endif

	return true;
}

/* Prepare one artifact and append it after all of its dependencies. */
static bool
cli_module_prepare(
	uint32_t artifact_index)
{
	struct cli_module_record *record;
	const char *require_name;
	char *resolved_path;
	uint32_t dependency_index;
	uint32_t require_count;
	uint32_t i;
	int binding_index;
	int existing_index;

	record = &cli_module_record_table[artifact_index];
	if (record->state == CLI_MODULE_VISITED)
		return true;
	if (record->state == CLI_MODULE_VISITING) {
		cli_module_set_error(
			N_TR("Circular require involving %s."),
			record->artifact.physical_path);
		return false;
	}

	record->state = CLI_MODULE_VISITING;
	if (!cli_module_classify(artifact_index))
		return false;

	require_count = cli_module_record_table[artifact_index].require_count;

	/* Resolve and prepare every require edge in declaration order. */
	for (i = 0; i < require_count; i++) {
		require_name = cli_module_record_table[artifact_index].require_name[i];
		binding_index = cli_module_find_binding(require_name);
		if (binding_index >= 0) {
			dependency_index = cli_module_binding_table[
				(uint32_t)binding_index].binding.artifact_index;
		} else {
			if (cli_module_require_resolver == NULL) {
				cli_module_set_error(
					N_TR("No resolver is available for required module '%s'."),
					require_name);
				return false;
			}
			resolved_path = cli_module_require_resolver(require_name);
			if (resolved_path == NULL) {
				cli_module_set_error(
					N_TR("Cannot resolve required module '%s'."),
					require_name);
				return false;
			}

			existing_index = cli_module_find_artifact(resolved_path);
			if (existing_index >= 0) {
				dependency_index = (uint32_t)existing_index;
				free(resolved_path);
			} else {
				if (!cli_module_add_artifact(
					resolved_path,
					false,
					0,
					require_name,
					&dependency_index)) {
					free(resolved_path);
					return false;
				}
				free(resolved_path);
			}

			if (!cli_module_add_binding(require_name, dependency_index)) {
				cli_module_set_error(
					N_TR("Out of memory building module graph."),
					NULL);
				return false;
			}
		}

		if (!cli_module_prepare(dependency_index))
			return false;
	}

	if (!cli_module_grow_indices(
		&cli_module_postorder,
		&cli_module_postorder_capacity,
		cli_module_postorder_count + 1)) {
		cli_module_set_error(N_TR("Out of memory building module graph."), NULL);
		return false;
	}
	cli_module_postorder[cli_module_postorder_count] = artifact_index;
	cli_module_postorder_count++;
	cli_module_record_table[artifact_index].state = CLI_MODULE_VISITED;

	return true;
}

/* Find one embedded application binding without filesystem lookup. */
static int
cli_module_find_app_binding(
	const char *name)
{
	uint32_t i;

	/* Search every validated name-to-module binding. */
	for (i = 0; i < cli_module_app.binding_count; i++) {
		if (strcmp(cli_module_app.binding[i].module_name, name) == 0)
			return (int)cli_module_app.binding[i].module_index;
	}

	return -1;
}

/* Append one embedded module after recursively visiting its dependencies. */
static bool
cli_module_visit_app(
	uint32_t module_index,
	unsigned char state[])
{
	const struct bytecode_file_module *module;
	int dependency_index;
	uint32_t i;

	if (module_index >= cli_module_app.module_count)
		return false;
	if (state[module_index] == CLI_MODULE_VISITED)
		return true;
	if (state[module_index] == CLI_MODULE_VISITING)
		return false;

	state[module_index] = CLI_MODULE_VISITING;
	module = &cli_module_app.module[module_index];

	/* Visit each embedded dependency before appending this module. */
	for (i = 0; i < module->require_count; i++) {
		dependency_index = cli_module_find_app_binding(
			module->require_name[i]);
		if (dependency_index < 0)
			return false;
		if (!cli_module_visit_app((uint32_t)dependency_index, state))
			return false;
	}

	if (cli_module_app_order_count >= cli_module_app.module_count)
		return false;
	cli_module_app_order[cli_module_app_order_count] = module_index;
	cli_module_app_order_count++;
	state[module_index] = CLI_MODULE_VISITED;

	return true;
}

/* Inspect and order one self-contained application container. */
static bool
cli_module_build_app(
	const uint8_t *data,
	size_t size)
{
	struct bytecode_file_error error;
	unsigned char *state;
	uint32_t i;
	bool succeeded;

	state = NULL;
	succeeded = false;

	if (!bytecode_file_inspect_app(
		data,
		size,
		&cli_module_app,
		&error)) {
		cli_module_set_error(N_TR("Malformed application container."), NULL);
		goto cleanup;
	}

	cli_module_app_order = malloc(
		(size_t)cli_module_app.module_count *
		sizeof(*cli_module_app_order));
	if (cli_module_app_order == NULL) {
		cli_module_set_error(N_TR("Out of memory inspecting application."), NULL);
		goto cleanup;
	}

	state = calloc(
		(size_t)cli_module_app.module_count,
		sizeof(*state));
	if (state == NULL) {
		cli_module_set_error(N_TR("Out of memory inspecting application."), NULL);
		goto cleanup;
	}

	/* Traverse every application root through embedded bindings. */
	for (i = 0; i < cli_module_app.root_count; i++) {
		if (!cli_module_visit_app(cli_module_app.root_index[i], state)) {
			cli_module_set_error(
				N_TR("Invalid application dependency graph."),
				NULL);
			goto cleanup;
		}
	}
	if (cli_module_app_order_count != cli_module_app.module_count) {
		cli_module_set_error(
			N_TR("Application contains an unreachable module."),
			NULL);
		goto cleanup;
	}

	cli_module_has_app = true;
	succeeded = true;

cleanup:
	free(state);

	return succeeded;
}

/* Convert one inspected bytecode function to the runtime's LIR view. */
static bool
cli_module_make_lir(
	const struct bytecode_file_function *function,
	struct lir_func *lir)
{
	uint32_t i;

	if (function->param_count > LIR_PARAM_SIZE ||
	    function->param_count > NOCT_ARG_MAX) {
		return false;
	}

	memset(lir, 0, sizeof(*lir));
	lir->tmpvar_size = function->tmpvar_size;
	lir->bytecode_size = function->bytecode.size;
	lir->bytecode = (uint8_t *)(void *)function->bytecode.data;
	lir->file_name = function->source;
	lir->func_name = function->name;
	lir->param_count = function->param_count;

	/* Copy every inspected parameter contract into the descriptor. */
	for (i = 0; i < function->param_count; i++) {
		lir->param_name[i] = function->param_name[i];
		lir->param_type[i] = function->param_type[i];
		lir->param_packed_type[i] = function->param_packed_type[i];
		lir->param_restricted[i] = function->param_restricted[i];
	}
	lir->return_type = function->return_type;
	lir->return_packed_type = function->return_packed_type;
	lir->return_type_checked = function->return_type_checked;
	lir->has_vector_ops = function->has_vector_ops;
	lir->is_fast = function->is_fast;
#if defined(NOCT_USE_OPTIMIZER)
	lir->fast_info = function->is_fast ? function->fast_info : NULL;
#else
	lir->is_fast = false;
	lir->fast_info = NULL;
#endif
	lir->has_fma_ops = function->has_fma_ops;

	return true;
}

/* Register one inspected module without exposing its file record to core. */
static bool
cli_module_register_bytecode(
	struct rt_env *env,
	const struct bytecode_file_module *module)
{
	struct lir_func *function;
	size_t data_size;
	uint32_t registration_size;
	uint32_t i;
	bool succeeded;

	function = NULL;
	succeeded = false;

	if (module->function_count == 0)
		return true;
	if (sizeof(*function) > SIZE_MAX / (size_t)module->function_count)
		return false;
	data_size = (size_t)module->function_count * sizeof(*function);
	if (!bytecode_file_check_registration_size(
		data_size,
		&registration_size)) {
		return false;
	}

	function = calloc((size_t)module->function_count, sizeof(*function));
	if (function == NULL)
		return false;

	/* Build one file-independent descriptor for every function. */
	for (i = 0; i < module->function_count; i++) {
		if (!cli_module_make_lir(&module->function[i], &function[i]))
			goto cleanup;
	}

	succeeded = noct_register_bytecode(
		env,
		(uint8_t *)(void *)function,
		registration_size);

cleanup:
	free(function);

	return succeeded;
}
