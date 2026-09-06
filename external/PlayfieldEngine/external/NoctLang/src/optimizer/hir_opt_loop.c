/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral source-HIR loop effect collection.
 */

#include "hir_opt_parallel.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define HIR_ANALYZE_MAX_VISITED 1024

struct hir_analyze_ctx {
	struct hir_loop_summary *summary;
	const struct hir_memory_catalog *catalog;
	const struct hir_stmt *stmt;
	int line;
	uint32_t visited_count;
	struct hir_block *visited[HIR_ANALYZE_MAX_VISITED];
};

static int hir_packed_width(int kind);
static const char *hir_term_symbol(const struct hir_expr *expr);
static void hir_mark_unknown(struct hir_analyze_ctx *ctx, int reason);
static bool hir_collect_expr(struct hir_analyze_ctx *ctx, const struct hir_expr *expr, bool write);
static bool hir_collect_block(struct hir_analyze_ctx *ctx, struct hir_block *block);
static bool hir_block_within(const struct hir_block *block, const struct hir_block *loop);
static bool hir_block_seen(struct hir_analyze_ctx *ctx, struct hir_block *block);
static bool hir_validate_affine_invariants(struct hir_analyze_ctx *ctx);
static struct hir_scalar_effect *hir_scalar_get(struct hir_analyze_ctx *ctx, const char *symbol);
static bool hir_record_scalar(struct hir_analyze_ctx *ctx, const char *symbol, bool write);
static bool hir_record_access(struct hir_analyze_ctx *ctx, const char *symbol, const struct hir_expr *index_expr, bool write);
static bool hir_record_call(struct hir_analyze_ctx *ctx, const struct hir_expr *expr, bool pure);
static bool hir_collect_top_loops(struct hir_block *func, struct hir_block *block, const struct hir_memory_catalog *catalog, FILE *fp, const char *prefix, struct hir_block **visited, uint32_t *visited_count, uint32_t *loop_count);

/*
 * Initializes a memory-object catalog.
 */
void
hir_memory_catalog_init(
	struct hir_memory_catalog *catalog)
{
	assert(catalog != NULL);

	memset(catalog, 0, sizeof(*catalog));
}

/*
 * Adds one memory object to a catalog.
 */
bool
hir_memory_catalog_add(
	struct hir_memory_catalog *catalog,
	const struct hir_memory_object *object)
{
	uint32_t i;

	if (catalog == NULL)
		return false;
	if (object == NULL)
		return false;
	if (object->symbol == NULL)
		return false;
	if (catalog->count >= HIR_PARALLEL_MAX_OBJECTS)
		return false;

	/* Reject a second object with the same symbol. */
	for (i = 0; i < catalog->count; i++) {
		if (strcmp(catalog->object[i].symbol, object->symbol) == 0)
			return false;
	}

	catalog->object[catalog->count] = *object;
	catalog->count++;

	return true;
}

/*
 * Finds a memory object by symbol.
 */
const struct hir_memory_object *
hir_memory_catalog_find(
	const struct hir_memory_catalog *catalog,
	const char *symbol)
{
	uint32_t i;

	if (catalog == NULL)
		return NULL;
	if (symbol == NULL)
		return NULL;

	/* Search the catalog for the requested symbol. */
	for (i = 0; i < catalog->count; i++) {
		if (strcmp(catalog->object[i].symbol, symbol) == 0)
			return &catalog->object[i];
	}

	return NULL;
}

/*
 * Builds the memory-object catalog for one HIR function.
 */
bool
hir_memory_catalog_build_func(
	struct hir_block *func,
	struct hir_memory_catalog *catalog)
{
	struct hir_memory_object object;
	struct hir_local *local;
	uint32_t i;
	int param;

	if (func == NULL)
		return false;
	if (func->type != HIR_BLOCK_FUNC)
		return false;
	if (catalog == NULL)
		return false;

	hir_memory_catalog_init(catalog);
	catalog->allow_non_affine_reads = true;

	/* Add every declared logical buffer to the catalog. */
	for (local = func->val.func.local; local != NULL; local = local->next) {
		if (local->storage_class != HIR_LOCAL_STORAGE_LOGICAL_BUFFER)
			continue;
		if (local->declared_packed_type < 0)
			continue;

		memset(&object, 0, sizeof(object));
		object.id = (int)catalog->count;
		object.symbol = local->symbol;
		object.source_line = local->declaration_line;
		object.element_kind = local->declared_packed_type;
		object.element_width = hir_packed_width(local->declared_packed_type);
		object.readable = true;
		object.writable = true;
		object.alias_class = local->index;
		object.length_expr = NULL;

		if (local->initializer != NULL) {
			if (local->initializer->type == HIR_EXPR_THISCALL &&
			    local->initializer->val.thiscall.arg_count == 1) {
				object.length_expr =
					local->initializer->val.thiscall.arg[0];
			}
		}

		if (local->is_parameter) {
			object.storage = HIR_MEMORY_STORAGE_PARAMETER;
			object.alias_kind = HIR_ALIAS_MAY_ALIAS;
			param = -1;

			/* Find the parameter slot for this buffer. */
			for (i = 0; i < func->val.func.param_count; i++) {
				if (strcmp(
					    func->val.func.param_name[i],
					    local->symbol) == 0) {
					param = (int)i;
					break;
				}
			}

			if (param >= 0) {
				if (func->val.func.param_restricted[param]) {
					object.alias_kind =
						HIR_ALIAS_CHECKED_NOALIAS;
				}
			}
		} else {
			object.storage = HIR_MEMORY_STORAGE_LOCAL;
			object.alias_kind = HIR_ALIAS_UNIQUE;
		}

		if (!hir_memory_catalog_add(catalog, &object))
			return false;
	}

	return true;
}

/*
 * Collects the effects of one HIR loop.
 */
bool
hir_loop_analyze(
	struct hir_block *func,
	struct hir_block *loop,
	const struct hir_memory_catalog *catalog,
	struct hir_loop_summary **summary)
{
	struct hir_loop_summary *result;
	struct hir_analyze_ctx ctx;

	if (summary == NULL)
		return false;

	*summary = NULL;

	if (func == NULL)
		return false;
	if (func->type != HIR_BLOCK_FUNC)
		return false;
	if (loop == NULL)
		return false;
	if (catalog == NULL)
		return false;

	result = noct_calloc(1, sizeof(*result));
	if (result == NULL)
		return false;

	result->func = func;
	result->loop = loop;
	result->catalog = catalog;
	result->line = loop->line;
	result->analysis_status = HIR_ANALYSIS_COMPLETE;
	result->analysis_reason = HIR_PAR_REASON_NONE;
	result->parallel_class = HIR_PAR_CLASS_UNCLASSIFIED;
	result->parallel_reason = HIR_PAR_REASON_NONE;
	result->range_status = HIR_RANGE_UNCHECKED;
	result->range_reason = HIR_PAR_REASON_NONE;

	if (loop->type != HIR_BLOCK_FOR) {
		result->analysis_status = HIR_ANALYSIS_UNKNOWN;
		result->analysis_reason = HIR_PAR_REASON_NOT_RANGED_LOOP;
		*summary = result;
		return true;
	}
	if (!loop->val.for_.is_ranged) {
		result->analysis_status = HIR_ANALYSIS_UNKNOWN;
		result->analysis_reason = HIR_PAR_REASON_NOT_RANGED_LOOP;
		*summary = result;
		return true;
	}
	result->counter_symbol = loop->val.for_.counter_symbol;
	result->start = loop->val.for_.start;
	result->stop = loop->val.for_.stop;

	memset(&ctx, 0, sizeof(ctx));
	ctx.summary = result;
	ctx.catalog = catalog;
	ctx.line = loop->line;

	if (!hir_collect_block(&ctx, loop->val.for_.inner)) {
		hir_loop_summary_free(result);
		return false;
	}
	if (!hir_validate_affine_invariants(&ctx)) {
		hir_loop_summary_free(result);
		return false;
	}

	*summary = result;

	return true;
}

/*
 * Frees one loop-analysis summary.
 */
void
hir_loop_summary_free(
	struct hir_loop_summary *summary)
{
	noct_free(summary);
}

/*
 * Writes one loop-analysis summary.
 */
void
hir_loop_summary_dump(
	FILE *fp,
	const struct hir_loop_summary *summary,
	const char *prefix)
{
	struct hir_doall_result doall;
	struct hir_dosum_result dosum;
	const char *class_name;
	const char *class_reason;
	const char *display_prefix;
	const char *status_name;

	if (fp == NULL)
		return;
	if (summary == NULL)
		return;

	class_name = "unknown";
	class_reason = hir_parallel_reason_string(summary->analysis_reason);

	if (hir_doall_classify(summary, &doall)) {
		if (doall.classification == HIR_PAR_CLASS_DOALL)
			class_name = "doall";
		else if (doall.classification == HIR_PAR_CLASS_DEPENDENT)
			class_name = "dependent";

		class_reason = hir_parallel_reason_string(doall.reason);

		if (doall.classification == HIR_PAR_CLASS_DEPENDENT) {
			if (hir_dosum_classify(summary, &dosum)) {
				if (dosum.classification == HIR_PAR_CLASS_DOSUM) {
					class_name = "dosum";
					class_reason =
						hir_parallel_reason_string(dosum.reason);
				}
			}
		}
	}

	if (prefix != NULL)
		display_prefix = prefix;
	else
		display_prefix = "parallel-analysis";

	if (summary->analysis_status == HIR_ANALYSIS_COMPLETE)
		status_name = "complete";
	else
		status_name = "unknown";

	fprintf(
		fp,
		"%s line=%d status=%s reason=%s class=%s class-reason=%s accesses=%u scalars=%u calls=%u",
		display_prefix,
		summary->line,
		status_name,
		hir_parallel_reason_string(summary->analysis_reason),
		class_name,
		class_reason,
		(unsigned int)summary->access_count,
		(unsigned int)summary->scalar_count,
		(unsigned int)summary->call_count);
	fprintf(
		fp,
		" alias-guards=%u",
		(unsigned int)doall.alias_requirement_count);
	fputc('\n', fp);
}

/*
 * Returns the diagnostic name for a parallel-analysis reason.
 */
const char *
hir_parallel_reason_string(
	int reason)
{

	/* Map every analysis reason to its stable diagnostic spelling. */
	switch (reason) {
	case HIR_PAR_REASON_NONE:
		return "none";
	case HIR_PAR_REASON_INVALID_ARGUMENT:
		return "invalid-argument";
	case HIR_PAR_REASON_NOT_RANGED_LOOP:
		return "not-ranged-loop";
	case HIR_PAR_REASON_NESTED_LOOP:
		return "nested-loop";
	case HIR_PAR_REASON_WHILE_LOOP:
		return "while-loop";
	case HIR_PAR_REASON_UNKNOWN_MEMORY:
		return "unknown-memory";
	case HIR_PAR_REASON_NON_AFFINE_INDEX:
		return "non-affine-index";
	case HIR_PAR_REASON_UNKNOWN_CALL:
		return "unknown-call";
	case HIR_PAR_REASON_ACCESS_LIMIT:
		return "access-limit";
	case HIR_PAR_REASON_SCALAR_LIMIT:
		return "scalar-limit";
	case HIR_PAR_REASON_CALL_LIMIT:
		return "call-limit";
	case HIR_PAR_REASON_OBJECT_LIMIT:
		return "object-limit";
	case HIR_PAR_REASON_DEPENDENCE_LIMIT:
		return "dependence-limit";
	case HIR_PAR_REASON_SCALAR_CARRIED:
		return "scalar-carried";
	case HIR_PAR_REASON_OUTER_SCALAR_WRITE:
		return "outer-scalar-write";
	case HIR_PAR_REASON_MEMORY_RAW:
		return "memory-raw";
	case HIR_PAR_REASON_MEMORY_WAR:
		return "memory-war";
	case HIR_PAR_REASON_MEMORY_WAW:
		return "memory-waw";
	case HIR_PAR_REASON_MAY_ALIAS:
		return "may-alias";
	case HIR_PAR_REASON_REDUCTION_SHAPE:
		return "reduction-shape";
	case HIR_PAR_REASON_REDUCTION_IDENTITY:
		return "reduction-identity";
	case HIR_PAR_REASON_REDUCTION_TYPE:
		return "reduction-type";
	case HIR_PAR_REASON_REDUCTION_EFFECT:
		return "reduction-effect";
	case HIR_PAR_REASON_REDUCTION_PATH:
		return "reduction-path";
	case HIR_PAR_REASON_OUT_OF_MEMORY:
		return "out-of-memory";
	case HIR_PAR_REASON_INTERNAL:
		return "internal";
	default:
		return "unknown-reason";
	}
}

/*
 * Writes parallel-analysis diagnostics for one HIR function.
 */
bool
hir_parallel_diagnose_func(
	struct hir_block *func,
	FILE *fp,
	const char *prefix)
{
	struct hir_memory_catalog catalog;
	struct hir_block *visited[HIR_ANALYZE_MAX_VISITED];
	uint32_t visited_count;
	uint32_t loop_count;

	if (func == NULL)
		return false;
	if (func->type != HIR_BLOCK_FUNC)
		return false;
	if (fp == NULL)
		return false;

	if (!hir_memory_catalog_build_func(func, &catalog))
		return false;

	visited_count = 0;
	loop_count = 0;

	return hir_collect_top_loops(
		func,
		func->val.func.inner,
		&catalog,
		fp,
		prefix,
		visited,
		&visited_count,
		&loop_count);
}

static int
hir_packed_width(
	int kind)
{

	/* Map each packed element kind to its byte width. */
	switch (kind) {
	case NOCT_PACKED_INT8:
	case NOCT_PACKED_UINT8:
		return 1;
	case NOCT_PACKED_INT16:
	case NOCT_PACKED_UINT16:
		return 2;
	case NOCT_PACKED_INT32:
	case NOCT_PACKED_UINT32:
	case NOCT_PACKED_FLOAT32:
		return 4;
	case NOCT_PACKED_INT64:
	case NOCT_PACKED_UINT64:
	case NOCT_PACKED_FLOAT64:
		return 8;
	default:
		return -1;
	}
}

static const char *
hir_term_symbol(
	const struct hir_expr *expr)
{
	if (expr == NULL)
		return NULL;
	if (expr->type != HIR_EXPR_TERM)
		return NULL;
	if (expr->val.term.term == NULL)
		return NULL;
	if (expr->val.term.term->type != HIR_TERM_SYMBOL)
		return NULL;

	return expr->val.term.term->val.symbol;
}

static void
hir_mark_unknown(
	struct hir_analyze_ctx *ctx,
	int reason)
{
	if (ctx->summary->analysis_status == HIR_ANALYSIS_COMPLETE) {
		ctx->summary->analysis_status = HIR_ANALYSIS_UNKNOWN;
		ctx->summary->analysis_reason = reason;
	}
}

static struct hir_scalar_effect *
hir_scalar_get(
	struct hir_analyze_ctx *ctx,
	const char *symbol)
{
	uint32_t i;
	struct hir_scalar_effect *effect;
	struct hir_local *local;

	/* Find an existing scalar-effect record. */
	for (i = 0; i < ctx->summary->scalar_count; i++) {
		if (strcmp(ctx->summary->scalar[i].symbol, symbol) == 0)
			return &ctx->summary->scalar[i];
	}

	if (ctx->summary->scalar_count >= HIR_PARALLEL_MAX_SCALARS) {
		hir_mark_unknown(ctx, HIR_PAR_REASON_SCALAR_LIMIT);
		return NULL;
	}

	effect = &ctx->summary->scalar[ctx->summary->scalar_count++];
	memset(effect, 0, sizeof(*effect));
	effect->symbol = symbol;
	effect->is_counter = false;
	if (ctx->summary->counter_symbol != NULL) {
		if (strcmp(symbol, ctx->summary->counter_symbol) == 0)
			effect->is_counter = true;
	}

	local = ctx->summary->func->val.func.local;

	/* Locate the declaration associated with this symbol. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0) {
			effect->declared_inside_loop = false;
			if (local->declaration_stmt != NULL) {
				if (local->declaration_stmt == ctx->stmt)
					effect->declared_inside_loop = true;
			}
			break;
		}
		local = local->next;
	}

	return effect;
}

static bool
hir_record_scalar(
	struct hir_analyze_ctx *ctx,
	const char *symbol,
	bool write)
{
	struct hir_scalar_effect *effect;
	struct hir_local *local;

	effect = hir_scalar_get(ctx, symbol);
	if (effect == NULL)
		return true;

	if (write) {
		effect->writes++;
	} else {
		effect->reads++;
		if (effect->writes == 0)
			effect->read_before_write = true;
	}

	local = ctx->summary->func->val.func.local;

	/* Mark a scalar declared by the current statement. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0) {
			if (local->declaration_stmt == ctx->stmt)
				effect->declared_inside_loop = true;
		}
		local = local->next;
	}

	return true;
}

static bool
hir_record_access(
	struct hir_analyze_ctx *ctx,
	const char *symbol,
	const struct hir_expr *index_expr,
	bool write)
{
	const struct hir_memory_object *object;
	struct hir_memory_access *access;

	object = hir_memory_catalog_find(ctx->catalog, symbol);
	if (object == NULL) {
		hir_mark_unknown(ctx, HIR_PAR_REASON_UNKNOWN_MEMORY);
		return true;
	}

	if (ctx->summary->access_count >= HIR_PARALLEL_MAX_ACCESSES) {
		hir_mark_unknown(ctx, HIR_PAR_REASON_ACCESS_LIMIT);
		return true;
	}

	access = &ctx->summary->access[ctx->summary->access_count++];
	memset(access, 0, sizeof(*access));
	if (write)
		access->kind = HIR_MEMORY_WRITE;
	else
		access->kind = HIR_MEMORY_READ;
	access->object_id = object->id;
	access->element_kind = object->element_kind;
	access->line = ctx->line;

	if (!hir_opt_normalize_index(
		    index_expr,
		    ctx->summary->counter_symbol,
		    &access->index)) {
		return false;
	}

	if (access->index.kind == HIR_AFFINE_UNKNOWN) {
		if (write || !ctx->catalog->allow_non_affine_reads) {
			hir_mark_unknown(
				ctx,
				HIR_PAR_REASON_NON_AFFINE_INDEX);
		}
	}

	return true;
}

static bool
hir_record_call(
	struct hir_analyze_ctx *ctx,
	const struct hir_expr *expr,
	bool pure)
{
	struct hir_call_effect *call;

	if (ctx->summary->call_count >= HIR_PARALLEL_MAX_CALLS) {
		hir_mark_unknown(ctx, HIR_PAR_REASON_CALL_LIMIT);
		return true;
	}

	call = &ctx->summary->call[ctx->summary->call_count++];
	call->expr = expr;
	call->line = ctx->line;
	call->is_pure = pure;
	if (!pure)
		hir_mark_unknown(ctx, HIR_PAR_REASON_UNKNOWN_CALL);

	return true;
}

static bool
hir_collect_expr(
	struct hir_analyze_ctx *ctx,
	const struct hir_expr *expr,
	bool write)
{
	uint32_t i;
	const char *symbol;
	bool access_write;
	bool pure;

	if (expr == NULL)
		return true;

	/* Collect the effects contributed by this expression shape. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		symbol = hir_term_symbol(expr);
		if (symbol != NULL)
			return hir_record_scalar(ctx, symbol, write);
		return true;
	case HIR_EXPR_SUBSCR:
		symbol = hir_term_symbol(expr->val.binary.expr[0]);
		if (symbol == NULL) {
			hir_mark_unknown(ctx, HIR_PAR_REASON_UNKNOWN_MEMORY);
			if (!hir_collect_expr(ctx, expr->val.binary.expr[0], false))
				return false;
		} else {
			if (!hir_record_access(
				    ctx,
				    symbol,
				    expr->val.binary.expr[1],
				    write)) {
				return false;
			}
		}

		return hir_collect_expr(ctx, expr->val.binary.expr[1], false);
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PSTORE8:
	case HIR_EXPR_PSTORE16:
	case HIR_EXPR_PSTORE32:
	case HIR_EXPR_PSTORE64:
	case HIR_EXPR_PSTOREF32:
		symbol = hir_term_symbol(expr->val.binary.expr[0]);
		if (symbol == NULL) {
			hir_mark_unknown(ctx, HIR_PAR_REASON_UNKNOWN_MEMORY);
			return true;
		}

		access_write = false;

		/* Distinguish packed stores from packed loads. */
		switch (expr->type) {
		case HIR_EXPR_PSTORE8:
		case HIR_EXPR_PSTORE16:
		case HIR_EXPR_PSTORE32:
		case HIR_EXPR_PSTORE64:
		case HIR_EXPR_PSTOREF32:
			access_write = true;
			break;
		default:
			break;
		}

		if (!hir_record_access(
			    ctx,
			    symbol,
			    expr->val.binary.expr[1],
			    access_write)) {
			return false;
		}

		return hir_collect_expr(ctx, expr->val.binary.expr[1], false);
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		if (!hir_collect_expr(ctx, expr->val.binary.expr[0], false))
			return false;
		return hir_collect_expr(ctx, expr->val.binary.expr[1], false);
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PAR:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		return hir_collect_expr(ctx, expr->val.unary.expr, false);
	case HIR_EXPR_VINDUCTF32:
		if (!hir_collect_expr(ctx, expr->val.binary.expr[0], false))
			return false;
		return hir_collect_expr(ctx, expr->val.binary.expr[1], false);
	case HIR_EXPR_DOT:
		return hir_collect_expr(ctx, expr->val.dot.obj, false);
	case HIR_EXPR_CALL:
		pure = hir_get_intrinsic_call(expr) != HIR_INTRINSIC_NONE;
		if (!hir_record_call(ctx, expr, pure))
			return false;
		if (!hir_collect_expr(ctx, expr->val.call.func, false))
			return false;

		/* Collect effects from every ordinary call argument. */
		for (i = 0; i < expr->val.call.arg_count; i++) {
			if (!hir_collect_expr(ctx, expr->val.call.arg[i], false))
				return false;
		}

		return true;
	case HIR_EXPR_THISCALL:
		if (!hir_record_call(ctx, expr, false))
			return false;
		if (!hir_collect_expr(ctx, expr->val.thiscall.obj, false))
			return false;

		/* Collect effects from every method-call argument. */
		for (i = 0; i < expr->val.thiscall.arg_count; i++) {
			if (!hir_collect_expr(ctx, expr->val.thiscall.arg[i], false))
				return false;
		}

		return true;
	case HIR_EXPR_ARRAY:

		/* Collect effects from every array element. */
		for (i = 0; i < expr->val.array.elem_count; i++) {
			if (!hir_collect_expr(ctx, expr->val.array.elem[i], false))
				return false;
		}

		return true;
	case HIR_EXPR_DICT:

		/* Collect effects from every dictionary value. */
		for (i = 0; i < expr->val.dict.kv_count; i++) {
			if (!hir_collect_expr(ctx, expr->val.dict.value[i], false))
				return false;
		}

		return true;
	case HIR_EXPR_NEW:
		return hir_collect_expr(ctx, expr->val.new_.init, false);
	case HIR_EXPR_CAPTURE:
		if (!hir_collect_expr(ctx, expr->val.capture.expr, false))
			return false;

		return hir_record_scalar(ctx, expr->val.capture.symbol, true);
	case HIR_EXPR_SELECT:
		if (!hir_collect_expr(ctx, expr->val.select.cond, false))
			return false;
		if (!hir_collect_expr(ctx, expr->val.select.if_true, false))
			return false;

		return hir_collect_expr(ctx, expr->val.select.if_false, false);
	case HIR_EXPR_PMASKSTORE32:
		symbol = hir_term_symbol(expr->val.mask_store.base);
		if (symbol == NULL) {
			hir_mark_unknown(ctx, HIR_PAR_REASON_UNKNOWN_MEMORY);
			return true;
		}

		if (!hir_record_access(
			    ctx,
			    symbol,
			    expr->val.mask_store.offset,
			    true)) {
			return false;
		}
		if (!hir_collect_expr(ctx, expr->val.mask_store.offset, false))
			return false;

		return hir_collect_expr(ctx, expr->val.mask_store.mask, false);
	case HIR_EXPR_PGATHER32:
		symbol = hir_term_symbol(expr->val.gather.base);
		if (symbol == NULL) {
			hir_mark_unknown(ctx, HIR_PAR_REASON_UNKNOWN_MEMORY);
			return true;
		}

		if (!hir_record_access(
			    ctx,
			    symbol,
			    expr->val.gather.index,
			    false)) {
			return false;
		}
		if (!hir_collect_expr(ctx, expr->val.gather.index, false))
			return false;

		return true;
	default:
		hir_mark_unknown(ctx, HIR_PAR_REASON_INTERNAL);
		return true;
	}
}

static bool
hir_block_within(
	const struct hir_block *block,
	const struct hir_block *loop)
{

	/* Walk from the block to each enclosing parent. */
	while (block != NULL) {
		if (block == loop)
			return true;
		block = block->parent;
	}

	return false;
}

static bool
hir_block_seen(
	struct hir_analyze_ctx *ctx,
	struct hir_block *block)
{
	uint32_t i;

	/* Search the blocks already visited by this analysis. */
	for (i = 0; i < ctx->visited_count; i++) {
		if (ctx->visited[i] == block)
			return true;
	}

	if (ctx->visited_count >= HIR_ANALYZE_MAX_VISITED) {
		hir_mark_unknown(ctx, HIR_PAR_REASON_INTERNAL);
		return true;
	}

	ctx->visited[ctx->visited_count++] = block;

	return false;
}

static bool
hir_validate_affine_invariants(
	struct hir_analyze_ctx *ctx)
{
	struct hir_memory_access *access;
	struct hir_affine_index *index;
	uint32_t i;
	uint32_t j;

	/* Validate every invariant symbol used by a memory access. */
	for (i = 0; i < ctx->summary->access_count; i++) {
		access = &ctx->summary->access[i];
		index = &access->index;
		if (index->invariant_symbol == NULL)
			continue;

		/* Reject an invariant that is written inside the loop. */
		for (j = 0; j < ctx->summary->scalar_count; j++) {
			if (strcmp(
				    ctx->summary->scalar[j].symbol,
				    index->invariant_symbol) != 0) {
				continue;
			}
			if (ctx->summary->scalar[j].writes != 0) {
				index->kind = HIR_AFFINE_UNKNOWN;
				index->invariant_symbol = NULL;
				index->invariant_sign = 1;

				if (access->kind == HIR_MEMORY_WRITE ||
				    !ctx->catalog->allow_non_affine_reads) {
					hir_mark_unknown(
						ctx,
						HIR_PAR_REASON_NON_AFFINE_INDEX);
				}
				break;
			}
		}
	}

	return true;
}

static bool
hir_collect_block(
	struct hir_analyze_ctx *ctx,
	struct hir_block *block)
{
	struct hir_stmt *stmt;
	struct hir_block *chain;

	if (block == NULL)
		return true;
	if (block == ctx->summary->loop)
		return true;
	if (!hir_block_within(block, ctx->summary->loop))
		return true;
	if (hir_block_seen(ctx, block))
		return true;

	/* Collect the effects owned by this block shape. */
	switch (block->type) {
	case HIR_BLOCK_BASIC:
		stmt = block->val.basic.stmt_list;

		/* Collect both sides of every statement. */
		while (stmt != NULL) {
			ctx->stmt = stmt;
			ctx->line = stmt->line;

			if (!hir_collect_expr(ctx, stmt->rhs, false))
				return false;
			if (!hir_collect_expr(ctx, stmt->lhs, true))
				return false;

			stmt = stmt->next;
		}
		break;
	case HIR_BLOCK_IF:
		chain = block;

		/* Collect every arm in the conditional chain. */
		while (chain != NULL) {
			ctx->stmt = NULL;
			ctx->line = chain->line;

			if (!hir_collect_expr(ctx, chain->val.if_.cond, false))
				return false;
			if (!hir_collect_block(ctx, chain->val.if_.inner))
				return false;

			chain = chain->val.if_.chain_next;
		}
		break;
	case HIR_BLOCK_FOR:
		ctx->summary->has_nested_loop = true;
		hir_mark_unknown(ctx, HIR_PAR_REASON_NESTED_LOOP);

		if (!hir_collect_expr(ctx, block->val.for_.start, false))
			return false;
		if (!hir_collect_expr(ctx, block->val.for_.stop, false))
			return false;
		if (!hir_collect_expr(ctx, block->val.for_.collection, false))
			return false;
		if (!hir_collect_block(ctx, block->val.for_.inner))
			return false;
		break;
	case HIR_BLOCK_WHILE:
		ctx->summary->has_while_loop = true;
		hir_mark_unknown(ctx, HIR_PAR_REASON_WHILE_LOOP);

		if (!hir_collect_expr(ctx, block->val.while_.cond, false))
			return false;
		if (!hir_collect_block(ctx, block->val.while_.inner))
			return false;
		break;
	case HIR_BLOCK_FUNC:
	case HIR_BLOCK_END:
		hir_mark_unknown(ctx, HIR_PAR_REASON_INTERNAL);
		break;
	default:
		hir_mark_unknown(ctx, HIR_PAR_REASON_INTERNAL);
		break;
	}

	return hir_collect_block(
		ctx,
		block->succ);
}

static bool
hir_collect_top_loops(
	struct hir_block *func,
	struct hir_block *block,
	const struct hir_memory_catalog *catalog,
	FILE *fp,
	const char *prefix,
	struct hir_block **visited,
	uint32_t *visited_count,
	uint32_t *loop_count)
{
	uint32_t i;
	struct hir_loop_summary *summary;
	struct hir_block *chain;

	if (block == NULL)
		return true;

	/* Stop when this traversal has already visited the block. */
	for (i = 0; i < *visited_count; i++) {
		if (visited[i] == block)
			return true;
	}

	if (*visited_count >= HIR_ANALYZE_MAX_VISITED)
		return false;

	visited[(*visited_count)++] = block;

	if (block->type == HIR_BLOCK_FOR) {
		if (block->parent == func) {
			if (*loop_count >= HIR_PARALLEL_MAX_LOOPS)
				return false;

			(*loop_count)++;

			if (!hir_loop_analyze(func, block, catalog, &summary))
				return false;

			hir_loop_summary_dump(fp, summary, prefix);
			hir_loop_summary_free(summary);
		}
	}

	if (block->type == HIR_BLOCK_IF) {
		chain = block;

		/* Collect loops from every conditional arm. */
		while (chain != NULL) {
			if (!hir_collect_top_loops(
				    func,
				    chain->val.if_.inner,
				    catalog,
				    fp,
				    prefix,
				    visited,
				    visited_count,
				    loop_count)) {
				return false;
			}
			chain = chain->val.if_.chain_next;
		}
	} else if (block->type == HIR_BLOCK_FOR) {
		if (!hir_collect_top_loops(
			    func,
			    block->val.for_.inner,
			    catalog,
			    fp,
			    prefix,
			    visited,
			    visited_count,
			    loop_count)) {
			return false;
		}
	} else if (block->type == HIR_BLOCK_WHILE) {
		if (!hir_collect_top_loops(
			    func,
			    block->val.while_.inner,
			    catalog,
			    fp,
			    prefix,
			    visited,
			    visited_count,
			    loop_count)) {
			return false;
		}
	}

	return hir_collect_top_loops(
		func,
		block->succ,
		catalog,
		fp,
		prefix,
		visited,
		visited_count,
		loop_count);
}
