/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: Compile Mode
 */

#include "cli-main.h"
#include "app_file.h"
#include "bcback_private.h"
#include "bytecode_file.h"
#include "module.h"
#include "../backend/backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool compile_has_suffix(const char *path, const char *suffix);
static char *compile_copy_path(const char *path);
static char *compile_make_output_path(const char *input_path);
static void compile_cleanup_outputs(uint32_t count, char *output[]);
static bool compile_preflight_outputs(uint32_t input_count, char *const input[], char ***output);
static bool compile_check_app_output(const char *output, uint32_t root_count, char *const root[]);
static bool compile_standalone(uint32_t root_count, char *const root[], char *const output[]);
static bool compile_application(const char *output_path);

/*
 * Compiles source modules or one self-contained CPU application.
 */
int
command_compile(
	int argc,
	char *argv[])
{
	char **output;
	char *app_output;
	char **root;
	int first;
	int optimize_level;
	uint32_t root_count;
	bool app_mode;
	bool lineinfo;
	bool succeeded;
	enum cli_optimize_level_result optimize_result;

	output = NULL;
	app_output = NULL;
	root = NULL;
	first = 2;
	root_count = 0;
	app_mode = false;
	succeeded = false;
	cli_module_reset();

	/* Parse every compiler option before the first positional argument. */
	while (first < argc) {
		optimize_result = parse_optimize_level_option(
			argv[first],
			&optimize_level,
			&lineinfo);
		if (optimize_result == CLI_OPTIMIZE_LEVEL_VALID) {
			noct_bcback_set_optimize_level(optimize_level);
			noct_bcback_set_lineinfo(lineinfo);
			first++;
			continue;
		}
		if (optimize_result == CLI_OPTIMIZE_LEVEL_INVALID) {
			printf(N_TR("Invalid optimize-level option %s.\n"), argv[first]);
			goto cleanup;
		}
		if (strcmp(argv[first], "--simd-info") == 0) {
			noct_bcback_set_simd_info(true);
			first++;
			continue;
		}
		if (strcmp(argv[first], "--app") == 0) {
			if (app_mode) {
				printf(N_TR("The --app option may be specified only once.\n"));
				goto cleanup;
			}
			app_mode = true;
			first++;
			continue;
		}
		if (strncmp(argv[first], "--path=", 7) == 0) {
			if (!cli_module_add_path(argv[first] + 7)) {
				printf(N_TR("Invalid module path option %s.\n"), argv[first]);
				goto cleanup;
			}
			first++;
			continue;
		}
		if (strcmp(argv[first], "--gpu") == 0 ||
		    strncmp(argv[first], "--gpu=", 6) == 0 ||
		    strcmp(argv[first], "--gpu-list") == 0) {
			printf(
				N_TR("GPU acceleration is available only when running Noct source.\n"));
			goto cleanup;
		}
		break;
	}

	if (app_mode) {
		if (argc - first < 2) {
			show_usage();
			goto cleanup;
		}

		app_output = compile_copy_path(argv[first]);
		if (app_output == NULL) {
			printf(N_TR("Cannot allocate application output path.\n"));
			goto cleanup;
		}

		root = argv + first + 1;
		root_count = (uint32_t)(argc - first - 1);
		if (!compile_check_app_output(app_output, root_count, root))
			goto cleanup;

		if (!cli_module_build_graph(
			CLI_MODULE_GRAPH_APP,
			root_count,
			(const char *const *)root,
			cli_module_resolve)) {
			printf(N_TR("%s\n"), cli_module_get_error());
			goto cleanup;
		}

		succeeded = compile_application(app_output);
		goto cleanup;
	}

	if (argc <= first) {
		show_usage();
		goto cleanup;
	}

	root = argv + first;
	root_count = (uint32_t)(argc - first);
	if (!compile_preflight_outputs(root_count, root, &output))
		goto cleanup;

	if (!cli_module_build_graph(
		CLI_MODULE_GRAPH_COMPILE,
		root_count,
		(const char *const *)root,
		cli_module_resolve)) {
		printf(N_TR("%s\n"), cli_module_get_error());
		goto cleanup;
	}

	succeeded = compile_standalone(root_count, root, output);

cleanup:
	compile_cleanup_outputs(root_count, output);
	free(app_output);
	cli_module_reset();

	return succeeded ? 0 : 1;
}

/* Test one exact case-sensitive path suffix. */
static bool
compile_has_suffix(
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

/* Copy one output path with checked allocation arithmetic. */
static char *
compile_copy_path(
	const char *path)
{
	char *copy;
	size_t length;

	if (path == NULL)
		return NULL;

	length = strlen(path);
	if (length == SIZE_MAX)
		return NULL;

	copy = malloc(length + 1);
	if (copy == NULL)
		return NULL;
	memcpy(copy, path, length + 1);

	return copy;
}

/* Derive the canonical .nbc output path for one source input. */
static char *
compile_make_output_path(
	const char *input_path)
{
	char *output_path;
	size_t input_length;
	size_t base_length;
	size_t output_length;

	if (compile_has_suffix(input_path, ".nbc"))
		return NULL;

	input_length = strlen(input_path);
	if (compile_has_suffix(input_path, ".noct")) {
		base_length = input_length - strlen(".noct");
	} else if (compile_has_suffix(input_path, ".nct")) {
		base_length = input_length - strlen(".nct");
	} else {
		base_length = input_length;
	}

	if (base_length > SIZE_MAX - strlen(".nbc"))
		return NULL;
	output_length = base_length + strlen(".nbc");
	if (output_length == SIZE_MAX)
		return NULL;

	output_path = malloc(output_length + 1);
	if (output_path == NULL)
		return NULL;
	memcpy(output_path, input_path, base_length);
	memcpy(output_path + base_length, ".nbc", strlen(".nbc") + 1);

	return output_path;
}

/* Release every precomputed standalone output path. */
static void
compile_cleanup_outputs(
	uint32_t count,
	char *output[])
{
	uint32_t i;

	if (output == NULL)
		return;

	/* Release each path owned by the command. */
	for (i = 0; i < count; i++)
		free(output[i]);
	free(output);
}

/* Precompute and collision-check every standalone output path. */
static bool
compile_preflight_outputs(
	uint32_t input_count,
	char *const input[],
	char ***output)
{
	char **table;
	uint32_t i;
	uint32_t j;

	*output = NULL;
	if (input_count == 0)
		return false;
	if (sizeof(*table) > SIZE_MAX / (size_t)input_count)
		return false;

	table = calloc((size_t)input_count, sizeof(*table));
	if (table == NULL) {
		printf(N_TR("Cannot allocate bytecode output paths.\n"));
		return false;
	}

	/* Build every final name before any writer can be opened. */
	for (i = 0; i < input_count; i++) {
		if (compile_has_suffix(input[i], ".nbc")) {
			printf(N_TR("Bytecode input is not recompiled: %s\n"), input[i]);
			goto failure;
		}

		table[i] = compile_make_output_path(input[i]);
		if (table[i] == NULL) {
			printf(N_TR("Cannot allocate output path for %s.\n"), input[i]);
			goto failure;
		}
	}

	/* Reject two inputs that map to the same final bytecode path. */
	for (i = 0; i < input_count; i++) {
		for (j = i + 1; j < input_count; j++) {
			if (strcmp(table[i], table[j]) == 0) {
				printf(
					N_TR("Inputs %s and %s map to the same output %s.\n"),
					input[i],
					input[j],
					table[i]);
				goto failure;
			}
		}
	}

	/* Reject every generated path that aliases any positional input. */
	for (i = 0; i < input_count; i++) {
		for (j = 0; j < input_count; j++) {
			if (strcmp(table[i], input[j]) == 0) {
				printf(
					N_TR("Output %s for input %s collides with input %s.\n"),
					table[i],
					input[i],
					input[j]);
				goto failure;
			}
		}
	}

	*output = table;

	return true;

failure:
	compile_cleanup_outputs(input_count, table);

	return false;
}

/* Check the application output against every positional root path. */
static bool
compile_check_app_output(
	const char *output,
	uint32_t root_count,
	char *const root[])
{
	uint32_t i;

	/* Reject exact output/input collisions before reading the graph. */
	for (i = 0; i < root_count; i++) {
		if (strcmp(output, root[i]) == 0) {
			printf(
				N_TR("Application output %s collides with input %s.\n"),
				output,
				root[i]);
			return false;
		}
	}

	return true;
}

/* Compile every explicit source root into its precomputed .nbc path. */
static bool
compile_standalone(
	uint32_t root_count,
	char *const root[],
	char *const output[])
{
	const struct cli_module_artifact *artifact;
	uint32_t artifact_index;
	uint32_t i;

	UNUSED_PARAMETER(root);

	/* Translate explicit roots after the complete closure was validated. */
	for (i = 0; i < root_count; i++) {
		artifact_index = cli_module_get_root_artifact(i);
		artifact = cli_module_get_artifact(artifact_index);
		if (artifact == NULL || artifact->kind != CLI_MODULE_SOURCE)
			return false;

		if (!noct_bcback_start(output[i]))
			return false;
		if (!noct_bcback_translate(
			artifact->physical_path,
			(const char *)artifact->data)) {
			bcback_abort();
			return false;
		}
		if (!noct_bcback_finalize())
			return false;
	}

	return true;
}

/* Serialize the dependency-first graph into one self-contained .nap. */
static bool
compile_application(
	const char *output_path)
{
	const struct cli_module_artifact *artifact;
	const struct cli_module_binding *graph_binding;
	struct bcback_app_module *module;
	struct bcback_app_binding *binding;
	uint32_t *artifact_to_module;
	uint32_t *root;
	uint32_t artifact_count;
	uint32_t module_count;
	uint32_t binding_count;
	uint32_t root_count;
	uint32_t artifact_index;
	uint32_t bytecode_size;
	uint32_t i;
	bool succeeded;

	module = NULL;
	binding = NULL;
	artifact_to_module = NULL;
	root = NULL;
	artifact_count = cli_module_get_artifact_count();
	module_count = cli_module_get_postorder_count();
	binding_count = cli_module_get_binding_count();
	root_count = cli_module_get_root_count();
	succeeded = false;

	if (artifact_count == 0 || module_count != artifact_count)
		goto cleanup;

	/* Preserve every graph input if the requested output names it exactly. */
	for (i = 0; i < artifact_count; i++) {
		artifact = cli_module_get_artifact(i);
		if (artifact == NULL || artifact->physical_path == NULL)
			goto cleanup;
		if (strcmp(output_path, artifact->physical_path) == 0) {
			printf(
				N_TR("Application output %s collides with input %s.\n"),
				output_path,
				artifact->physical_path);
			goto cleanup;
		}
	}

	if (sizeof(*module) > SIZE_MAX / (size_t)module_count)
		goto cleanup;
	if (sizeof(*artifact_to_module) > SIZE_MAX / (size_t)artifact_count)
		goto cleanup;
	if (binding_count > 0 &&
	    sizeof(*binding) > SIZE_MAX / (size_t)binding_count) {
		goto cleanup;
	}
	if (sizeof(*root) > SIZE_MAX / (size_t)root_count)
		goto cleanup;

	module = calloc((size_t)module_count, sizeof(*module));
	if (module == NULL)
		goto cleanup;
	artifact_to_module = malloc(
		(size_t)artifact_count * sizeof(*artifact_to_module));
	if (artifact_to_module == NULL)
		goto cleanup;
	if (binding_count > 0) {
		binding = calloc((size_t)binding_count, sizeof(*binding));
		if (binding == NULL)
			goto cleanup;
	}
	root = malloc((size_t)root_count * sizeof(*root));
	if (root == NULL)
		goto cleanup;

	/* Mark every artifact as absent from the serialized index space. */
	for (i = 0; i < artifact_count; i++)
		artifact_to_module[i] = UINT32_MAX;

	/* Describe every graph artifact in dependency-first record order. */
	for (i = 0; i < module_count; i++) {
		artifact_index = cli_module_get_postorder_artifact(i);
		if (artifact_index >= artifact_count)
			goto cleanup;
		artifact = cli_module_get_artifact(artifact_index);
		if (artifact == NULL)
			goto cleanup;

		artifact_to_module[artifact_index] = i;
		module[i].logical_source = artifact->logical_source;
		if (artifact->kind == CLI_MODULE_SOURCE) {
			module[i].source_text = (const char *)artifact->data;
		} else if (artifact->kind == CLI_MODULE_BYTECODE) {
			if (!bytecode_file_check_registration_size(
				artifact->data_size,
				&bytecode_size)) {
				goto cleanup;
			}
			module[i].bytecode_data = artifact->data;
			module[i].bytecode_size = bytecode_size;
		} else {
			goto cleanup;
		}
	}

	/* Remap every require alias into the serialized module index space. */
	for (i = 0; i < binding_count; i++) {
		graph_binding = cli_module_get_binding(i);
		if (graph_binding == NULL ||
		    graph_binding->artifact_index >= artifact_count) {
			goto cleanup;
		}
		artifact_index = graph_binding->artifact_index;
		if (artifact_to_module[artifact_index] == UINT32_MAX)
			goto cleanup;

		binding[i].module_name = graph_binding->module_name;
		binding[i].module_index = artifact_to_module[artifact_index];
	}

	/* Remap explicit roots while preserving command-line order. */
	for (i = 0; i < root_count; i++) {
		artifact_index = cli_module_get_root_artifact(i);
		if (artifact_index >= artifact_count)
			goto cleanup;
		if (artifact_to_module[artifact_index] == UINT32_MAX)
			goto cleanup;
		root[i] = artifact_to_module[artifact_index];
	}

	succeeded = bcback_write_app(
		output_path,
		module_count,
		module,
		binding_count,
		binding,
		root_count,
		root);

cleanup:
	free(root);
	free(artifact_to_module);
	free(binding);
	free(module);

	if (!succeeded)
		printf(N_TR("Cannot create application %s.\n"), output_path);

	return succeeded;
}
