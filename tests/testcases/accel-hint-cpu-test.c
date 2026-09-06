/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Accelerator function hint HIR test.
 */

#include "ast.h"
#include "hir.h"
#include "parser.tab.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Source used to inspect named and anonymous function hints. */
static const char accel_hint_source[] =
	"func ordinary(): void {\n"
	"    var callback = () => { return; };\n"
	"    return;\n"
	"}\n"
	"__accel func accelerated(): void { return; }\n"
	"static __accel func accelerated_static(): void { return; }\n";

static bool check_named_function(uint32_t index, const char *name, bool is_static, bool is_accel);
static bool check_static_function(uint32_t index);
static bool check_anonymous_function(uint32_t index);

/*
 * Checks accelerator hints on constructed HIR functions.
 */
int
main(
	void)
{
	bool ast_started;
	bool hir_started;
	bool ok;

	ast_started = false;
	hir_started = false;
	ok = false;

	if (TOKEN_DUNDER_ACCEL != 330 ||
	    TOKEN_INTERNAL_ACCEL_PACKAGE != 331 ||
	    TOKEN_DUNDER_FAST != 332) {
		fprintf(stderr, "Accelerator token slots moved.\n");
		goto cleanup;
	}

	ast_started = true;
	if (!ast_build("accel-hint-cpu-test.noct", accel_hint_source)) {
		fprintf(stderr, "Failed to build AST: %s\n", ast_get_error_message());
		goto cleanup;
	}

	hir_started = true;
	if (!hir_build()) {
		fprintf(stderr, "Failed to build HIR: %s\n", hir_get_error_message());
		goto cleanup;
	}

	if (hir_get_function_count() != 4) {
		fprintf(stderr, "Unexpected HIR function count: %u\n",
			(unsigned)hir_get_function_count());
		goto cleanup;
	}

	if (!check_named_function(0, "ordinary", false, false))
		goto cleanup;
	if (!check_named_function(1, "accelerated", false, true))
		goto cleanup;
	if (!check_static_function(2))
		goto cleanup;
	if (!check_anonymous_function(3))
		goto cleanup;

	ok = true;

cleanup:
	if (hir_started)
		hir_cleanup();
	if (ast_started)
		ast_cleanup();

	if (!ok)
		return 1;

	puts("Accelerator hint HIR tests passed.");

	return 0;
}

/* Check one ordinary link name and its function flags. */
static bool
check_named_function(
	uint32_t index,
	const char *name,
	bool is_static,
	bool is_accel)
{
	struct hir_block *func;

	func = hir_get_function(index);
	if (strcmp(func->val.func.name, name) != 0) {
		fprintf(stderr, "Unexpected function name at %u: %s\n",
			(unsigned)index, func->val.func.name);
		return false;
	}
	if (func->val.func.is_static != is_static) {
		fprintf(stderr, "Unexpected static flag for %s.\n", name);
		return false;
	}
	if (func->val.func.is_accel != is_accel) {
		fprintf(stderr, "Unexpected accelerator flag for %s.\n", name);
		return false;
	}

	return true;
}

/* Check the mangled static function and its accelerator hint. */
static bool
check_static_function(
	uint32_t index)
{
	struct hir_block *func;

	func = hir_get_function(index);
	if (strncmp(func->val.func.name, "$static.", 8) != 0) {
		fprintf(stderr, "Unexpected static function name: %s\n",
			func->val.func.name);
		return false;
	}
	if (!func->val.func.is_static) {
		fprintf(stderr, "Static accelerator function lost its static flag.\n");
		return false;
	}
	if (!func->val.func.is_accel) {
		fprintf(stderr, "Static accelerator function lost its hint.\n");
		return false;
	}

	return true;
}

/* Check that an anonymous function never inherits an accelerator hint. */
static bool
check_anonymous_function(
	uint32_t index)
{
	struct hir_block *func;

	func = hir_get_function(index);
	if (strncmp(func->val.func.name, "$anon.", 6) != 0) {
		fprintf(stderr, "Unexpected anonymous function name: %s\n",
			func->val.func.name);
		return false;
	}
	if (func->val.func.is_accel) {
		fprintf(stderr, "Anonymous function inherited an accelerator hint.\n");
		return false;
	}

	return true;
}
