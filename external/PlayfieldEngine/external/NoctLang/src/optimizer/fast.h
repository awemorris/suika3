/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Noct __fast function contracts.
 */

#ifndef NOCT_FAST_H
#define NOCT_FAST_H

#include <noct/noct.h>

#if defined(NOCT_USE_OPTIMIZER)

#define NOCT_FAST_SIGNATURE_VERSION	1
#define NOCT_FAST_RANK_MAX		8
#define NOCT_FAST_RETURN_VOID		(-2)

enum fast_extent_kind {
	FAST_EXTENT_NONE = 0,
	FAST_EXTENT_CONST,
	FAST_EXTENT_PARAM
};

struct rt_env;
struct rt_func;

struct fast_extent {
	int kind;
	union {
		int64_t constant;
		uint32_t param_index;
	} value;
};

struct fast_param_contract {
	int value_type;
	int packed_type;
	bool restricted;
	uint32_t rank;
	/* Owned exact-rank table, or NULL for a primitive parameter. */
	struct fast_extent *extent;
};

struct fast_signature {
	uint32_t version;
	bool valid;
	uint32_t param_count;
	/* Owned exact-count table, or NULL for an empty signature. */
	struct fast_param_contract *param;
	int return_type;
};

/*
 * Initializes an empty signature before its first build or clone.
 */
void
fast_signature_init(
	struct fast_signature *signature);

/*
 * Releases a signature and restores its empty state.
 */
void
fast_signature_free(
	struct fast_signature *signature);

/*
 * Copies the base spelling before an optional shape.
 */
bool
fast_annotation_base(
	const char *annotation,
	char *base,
	size_t base_size,
	bool *has_shape);

/*
 * Builds and validates a complete function contract.
 *
 * The destination must have been initialized.  A failed build leaves it
 * unchanged.  A successful non-fast build stores an empty signature and
 * allocates no contract tables.
 */
bool
fast_signature_build(
	struct fast_signature *signature,
	bool is_fast,
	uint32_t param_count,
	const char *const *param_name,
	const char *const *param_annotation,
	const int *param_type,
	const int *param_packed_type,
	const bool *param_restricted,
	const char *return_annotation,
	int return_type,
	char *error,
	size_t error_size);

/*
 * Clones a signature without changing the destination on failure.
 *
 * The destination must have been initialized.
 */
bool
fast_signature_clone(
	struct fast_signature *destination,
	const struct fast_signature *source);

/*
 * Validates the complete in-memory signature representation.
 */
bool
fast_signature_valid(
	const struct fast_signature *signature);

/* Clones optimizer-owned metadata through an opaque boundary. */
void *
fast_info_clone(
	const void *fast_info);

/* Releases optimizer-owned metadata through an opaque boundary. */
void
fast_info_free(
	void *fast_info);

/* Gets the immutable signature stored in opaque optimizer metadata. */
const struct fast_signature *
fast_info_signature(
	const void *fast_info);

/* Restores an optimized generated function's runtime contract. */
bool
fast_mark_runtime_func(
	struct rt_func *func,
	uint32_t tmpvar_size,
	int return_type,
	uint32_t param_count,
	const int *value_type,
	const int *packed_type,
	const int *restricted,
	const uint32_t *rank,
	const int *extent_kind,
	const int64_t *extent_value);

/* Checks an optimized function call against its opaque entry contract. */
bool
fast_check_runtime_call(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count);

bool
fast_mark_func(
	NoctFunc *func,
	uint32_t tmpvar_size,
	int return_type,
	uint32_t param_count,
	const int *value_type,
	const int *packed_type,
	const int *restricted,
	const uint32_t *rank,
	const int *extent_kind,
	const int64_t *extent_value);

#endif /* defined(NOCT_USE_OPTIMIZER) */

#endif
