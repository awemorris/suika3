/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-owned source and bytecode module graph.
 */

#ifndef NOCT_CLI_MODULE_H
#define NOCT_CLI_MODULE_H

#include <noct/c89compat.h>

struct rt_env;

enum cli_module_artifact_kind {
	CLI_MODULE_SOURCE,
	CLI_MODULE_BYTECODE
};

enum cli_module_graph_mode {
	CLI_MODULE_GRAPH_RUN,
	CLI_MODULE_GRAPH_COMPILE,
	CLI_MODULE_GRAPH_APP
};

struct cli_module_artifact {
	const char *physical_path;
	const char *logical_source;
	const uint8_t *data;
	size_t data_size;
	enum cli_module_artifact_kind kind;
	bool is_explicit_root;
};

struct cli_module_binding {
	const char *module_name;
	uint32_t artifact_index;
};

void cli_module_reset(void);
bool cli_module_add_path(const char *path_list);
char *cli_module_resolve(const char *module_name);
bool cli_module_build_graph(
	enum cli_module_graph_mode mode,
	uint32_t root_count,
	const char *const root_path[],
	char *(*require_resolver)(const char *module_name));
bool cli_module_build_input_graph(
	const char *root_path,
	const uint8_t *data,
	size_t size,
	char *(*require_resolver)(const char *module_name));
bool cli_module_register_graph(struct rt_env *env);
uint32_t cli_module_get_artifact_count(void);
const struct cli_module_artifact *cli_module_get_artifact(
	uint32_t index);
uint32_t cli_module_get_postorder_count(void);
uint32_t cli_module_get_postorder_artifact(
	uint32_t index);
uint32_t cli_module_get_binding_count(void);
const struct cli_module_binding *cli_module_get_binding(
	uint32_t index);
uint32_t cli_module_get_root_count(void);
uint32_t cli_module_get_root_artifact(
	uint32_t index);
const char *cli_module_get_error(void);

#endif
