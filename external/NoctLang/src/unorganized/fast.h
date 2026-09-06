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

#define NOCT_FAST_SIGNATURE_VERSION 1
#define NOCT_FAST_RANK_MAX 8
#define NOCT_FAST_RETURN_VOID (-2)

enum fast_extent_kind {
	FAST_EXTENT_NONE = 0,
	FAST_EXTENT_CONST,
	FAST_EXTENT_PARAM
};

struct fast_extent {
	int kind;
	int64_t constant;
	int param_index;
};

struct fast_param_contract {
	int value_type;
	int packed_type;
	bool restricted;
	int rank;
	struct fast_extent extent[NOCT_FAST_RANK_MAX];
};

struct fast_signature {
	int version;
	bool valid;
	uint32_t param_count;
	struct fast_param_contract param[NOCT_ARG_MAX];
	int return_type;
};

void fast_signature_init(struct fast_signature *sig);

/* Copy the base spelling before an optional shape into base. */
bool fast_annotation_base(const char *annotation, char *base,
			  size_t base_size, bool *has_shape);

/* Build and validate a complete function contract. */
bool fast_signature_build(struct fast_signature *sig, int func_kind,
			  uint32_t param_count,
			  const char *const *param_name,
			  const char *const *param_annotation,
			  const int *param_type,
			  const int *param_packed_type,
			  const bool *param_restricted,
			  const char *return_annotation,
			  int return_type,
			  char *error, size_t error_size);

bool fast_signature_equal(const struct fast_signature *a,
			  const struct fast_signature *b);

#endif
