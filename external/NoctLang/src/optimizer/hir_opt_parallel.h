/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral HIR loop analysis and parallel classification.
 */

#ifndef NOCT_HIR_PARALLEL_H
#define NOCT_HIR_PARALLEL_H

#include "hir_opt_analysis.h"

#include <stdio.h>

#define HIR_PARALLEL_MAX_LOOPS		32
#define HIR_PARALLEL_MAX_OBJECTS	64
#define HIR_PARALLEL_MAX_ACCESSES	256
#define HIR_PARALLEL_MAX_SCALARS	256
#define HIR_PARALLEL_MAX_CALLS		64
#define HIR_PARALLEL_MAX_DEPENDENCES	256

enum hir_parallel_reason {
	HIR_PAR_REASON_NONE,
	HIR_PAR_REASON_INVALID_ARGUMENT,
	HIR_PAR_REASON_NOT_RANGED_LOOP,
	HIR_PAR_REASON_NESTED_LOOP,
	HIR_PAR_REASON_WHILE_LOOP,
	HIR_PAR_REASON_UNKNOWN_MEMORY,
	HIR_PAR_REASON_NON_AFFINE_INDEX,
	HIR_PAR_REASON_UNKNOWN_CALL,
	HIR_PAR_REASON_ACCESS_LIMIT,
	HIR_PAR_REASON_SCALAR_LIMIT,
	HIR_PAR_REASON_CALL_LIMIT,
	HIR_PAR_REASON_OBJECT_LIMIT,
	HIR_PAR_REASON_DEPENDENCE_LIMIT,
	HIR_PAR_REASON_SCALAR_CARRIED,
	HIR_PAR_REASON_OUTER_SCALAR_WRITE,
	HIR_PAR_REASON_MEMORY_RAW,
	HIR_PAR_REASON_MEMORY_WAR,
	HIR_PAR_REASON_MEMORY_WAW,
	HIR_PAR_REASON_MAY_ALIAS,
	HIR_PAR_REASON_REDUCTION_SHAPE,
	HIR_PAR_REASON_REDUCTION_IDENTITY,
	HIR_PAR_REASON_REDUCTION_TYPE,
	HIR_PAR_REASON_REDUCTION_EFFECT,
	HIR_PAR_REASON_REDUCTION_PATH,
	HIR_PAR_REASON_OUT_OF_MEMORY,
	HIR_PAR_REASON_INTERNAL
};

enum hir_analysis_status {
	HIR_ANALYSIS_COMPLETE,
	HIR_ANALYSIS_UNKNOWN
};

enum hir_parallel_class {
	HIR_PAR_CLASS_UNCLASSIFIED,
	HIR_PAR_CLASS_DOALL,
	HIR_PAR_CLASS_DOSUM,
	HIR_PAR_CLASS_DEPENDENT,
	HIR_PAR_CLASS_UNKNOWN
};

enum hir_range_status {
	HIR_RANGE_UNCHECKED,
	HIR_RANGE_COMPLETE,
	HIR_RANGE_UNAVAILABLE
};

enum hir_memory_storage {
	HIR_MEMORY_STORAGE_PARAMETER,
	HIR_MEMORY_STORAGE_LOCAL,
	HIR_MEMORY_STORAGE_REDUCTION
};

enum hir_alias_kind {
	HIR_ALIAS_MAY_ALIAS,
	HIR_ALIAS_UNIQUE,
	HIR_ALIAS_CHECKED_NOALIAS
};

enum hir_memory_access_kind {
	HIR_MEMORY_READ,
	HIR_MEMORY_WRITE
};

enum hir_dependence_kind {
	HIR_DEP_RAW,
	HIR_DEP_WAR,
	HIR_DEP_WAW
};

struct hir_memory_object {
	int id;
	const char *symbol;
	int source_line;
	int element_kind;
	int element_width;
	int storage;
	int alias_kind;
	int alias_class;
	bool readable;
	bool writable;
	const struct hir_expr *length_expr;
};

struct hir_memory_catalog {
	uint32_t count;
	bool allow_non_affine_reads;
	struct hir_memory_object object[HIR_PARALLEL_MAX_OBJECTS];
};

struct hir_memory_access {
	int kind;
	int object_id;
	int element_kind;
	int line;
	struct hir_affine_index index;
};

struct hir_scalar_effect {
	const char *symbol;
	uint32_t reads;
	uint32_t writes;
	bool read_before_write;
	bool declared_inside_loop;
	bool is_counter;
};

struct hir_call_effect {
	const struct hir_expr *expr;
	int line;
	bool is_pure;
};

struct hir_dependence {
	int kind;
	uint32_t first_access;
	uint32_t second_access;
	bool distance_known;
	int distance;
};

struct hir_loop_summary {
	struct hir_block *func;
	struct hir_block *loop;
	const struct hir_memory_catalog *catalog;
	const char *counter_symbol;
	const struct hir_expr *start;
	const struct hir_expr *stop;
	int line;
	int analysis_status;
	int analysis_reason;
	int parallel_class;
	int parallel_reason;
	int range_status;
	int range_reason;
	bool has_nested_loop;
	bool has_while_loop;
	uint32_t access_count;
	struct hir_memory_access access[HIR_PARALLEL_MAX_ACCESSES];
	uint32_t scalar_count;
	struct hir_scalar_effect scalar[HIR_PARALLEL_MAX_SCALARS];
	uint32_t call_count;
	struct hir_call_effect call[HIR_PARALLEL_MAX_CALLS];
	uint32_t dependence_count;
	struct hir_dependence dependence[HIR_PARALLEL_MAX_DEPENDENCES];
};

struct hir_doall_result {
	int classification;
	int reason;
	uint32_t dependence_count;
	struct hir_dependence dependence[HIR_PARALLEL_MAX_DEPENDENCES];
	uint32_t alias_requirement_count;
	struct hir_alias_requirement {
		int first_object_id;
		int second_object_id;
	} alias_requirement[HIR_PARALLEL_MAX_DEPENDENCES];
};

enum hir_reduction_operator {
	HIR_REDUCTION_NONE,
	HIR_REDUCTION_ADD
};

struct hir_dosum_result {
	int classification;
	int reason;
	int operator_;
	int value_type;
	const char *accumulator_symbol;
	const struct hir_expr *identity;
	const struct hir_expr *mapped_expr;
	const struct hir_expr *trip_expr;
	bool post_loop_use;
	int line;
};

void
hir_memory_catalog_init(
	struct hir_memory_catalog *catalog);

bool
hir_memory_catalog_add(
	struct hir_memory_catalog *catalog,
	const struct hir_memory_object *object);

const struct hir_memory_object *
hir_memory_catalog_find(
	const struct hir_memory_catalog *catalog,
	const char *symbol);

bool
hir_memory_catalog_build_func(
	struct hir_block *func,
	struct hir_memory_catalog *catalog);

bool
hir_loop_analyze(
	struct hir_block *func,
	struct hir_block *loop,
	const struct hir_memory_catalog *catalog,
	struct hir_loop_summary **summary);

void
hir_loop_summary_free(
	struct hir_loop_summary *summary);

void
hir_loop_summary_dump(
	FILE *fp,
	const struct hir_loop_summary *summary,
	const char *prefix);

const char *
hir_parallel_reason_string(
	int reason);

bool
hir_doall_classify(
	const struct hir_loop_summary *summary,
	struct hir_doall_result *result);

bool
hir_doall_classify_memory(
	const struct hir_loop_summary *summary,
	struct hir_doall_result *result);

bool
hir_dosum_classify(
	const struct hir_loop_summary *summary,
	struct hir_dosum_result *result);

/* Developer-only diagnostics; does not mutate the input HIR. */
bool
hir_parallel_diagnose_func(
	struct hir_block *func,
	FILE *fp,
	const char *prefix);

#endif
