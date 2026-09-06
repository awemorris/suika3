/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Accelerator eligibility analysis and deterministic GPU source emission. */

#include "ast.h"
#include "hir.h"
#include "hir_opt.h"
#include "hir_opt_parallel.h"
#include "gpu_ir.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_GLSL_MAX 65536

enum accel_rejection {
	ACCEL_REJECT_NONE,
	ACCEL_REJECT_PARAMETER,
	ACCEL_REJECT_LOOP_SHAPE,
	ACCEL_REJECT_BODY,
	ACCEL_REJECT_ACCESS,
	ACCEL_REJECT_GLOBAL_RESOURCE,
	ACCEL_REJECT_EXPRESSION,
	ACCEL_REJECT_SIZE
};

struct accel_emit {
	struct hir_block *func;
	struct hir_block *loop;
	struct accel_kernel *kernel;
	char text[ACCEL_GLSL_MAX];
	size_t size;
	int reason;
	int min_index;
	bool analysis_only;
	bool hlsl;
	bool shifted_read[NOCT_ARG_MAX];
	bool written[NOCT_ARG_MAX];
	uint32_t local_buffer_count;
	const char *local_buffer_symbol[NOCT_ARG_MAX];
	int local_buffer_type[NOCT_ARG_MAX];
	const struct hir_expr *local_buffer_length[NOCT_ARG_MAX];
};

static const char *accel_reason_name(int reason);
static bool accel_put(struct accel_emit *ctx, const char *format, ...);
static int accel_param_index(struct accel_emit *ctx, const char *symbol);
static bool accel_is_int_zero(struct hir_expr *expr);
static bool accel_is_counter(struct accel_emit *ctx, struct hir_expr *expr);
static bool accel_index_offset(struct accel_emit *ctx, struct hir_expr *expr,
			      int *offset);
static void accel_record_range(struct accel_emit *ctx, int param, int offset);
static bool accel_emit_term(struct accel_emit *ctx, struct hir_term *term,
	int expected_type);
static const char *accel_binary_op(int type);
static const char *accel_glsl_type(int packed_type);
static bool accel_emit_expr(struct accel_emit *ctx, struct hir_expr *expr,
			    int expected_type);
static bool accel_emit_stmt(struct accel_emit *ctx, struct hir_stmt *stmt,
			    int indent);
static bool accel_is_condition(struct hir_expr *expr);
static bool accel_expr_uses_counter(struct accel_emit *ctx,
	struct hir_expr *expr);
static bool accel_int_literal(struct hir_expr *expr, int *value);
static int accel_condition_min_index(struct accel_emit *ctx,
	struct hir_expr *expr);
static bool accel_emit_if_chain(struct accel_emit *ctx,
	struct hir_block *block, int packed_type, int indent,
	int *statement_count);
static bool accel_emit_blocks(struct accel_emit *ctx, struct hir_block *block,
			      int packed_type, int indent,
			      int *statement_count);
static bool accel_emit_body(struct accel_emit *ctx, int indent);
static bool accel_build_shader(struct accel_emit *ctx,
			       struct accel_kernel *kernel, bool hlsl);
static int accel_element_width(int packed_type);
static bool accel_store_hlsl(struct accel_kernel *kernel,
			     const char *source, size_t source_size);
static bool accel_build_program(struct hir_block *func,
				struct accel_kernel **outer_kernel,
				struct hir_block **loop,
				uint32_t kernel_count);
static bool accel_is_local_buffer_prologue(struct hir_block *func,
					   struct hir_stmt *stmt);
static bool accel_is_reduction_decl_tail(struct hir_block *func,
					 struct hir_stmt *stmt);
static struct accel_kernel *accel_build_doall_kernel(
	struct hir_block *func, struct hir_block *loop, const char *name);
static int accel_try_build_dosum(struct hir_block *func, bool accel_info);
static int accel_try_build_dosum_at(struct hir_block *func, bool accel_info,
				    uint32_t target_index,
				    bool reuse_program);

bool
hir_opt_accel_func(
	struct hir_block *func_block,
	bool accel_info)
{
	struct accel_kernel *kernel[ACCEL_PROGRAM_MAX_KERNELS];
	struct hir_block *loop[ACCEL_PROGRAM_MAX_KERNELS];
	struct hir_block *block;
	struct accel_emit ctx;
	const char *kind;
	char kernel_name[256];
	char ir_error[128];
	uint32_t loop_count;
	uint32_t k;
	uint32_t i;
	bool shape_ok;

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);

	if (!func_block->val.func.is_accel)
		return true;
	{
		int dosum_status;

		dosum_status = accel_try_build_dosum(func_block, accel_info);
		if (dosum_status > 0)
			return true;
		if (dosum_status < 0) {
			hir_set_error(func_block->line,
				      "Out of memory or invalid descriptor while lowering DOSUM.");
			return false;
		}
	}
	memset(kernel, 0, sizeof(kernel));
	loop_count = 0;
	shape_ok = true;
	block = func_block->val.func.inner;
	while (block != NULL) {
		if (block->type == HIR_BLOCK_FOR) {
			if (loop_count >= ACCEL_PROGRAM_MAX_KERNELS) {
				shape_ok = false;
				break;
			}
			loop[loop_count++] = block;
		} else if (block->type == HIR_BLOCK_BASIC &&
			   block->val.basic.stmt_list != NULL &&
			   !accel_is_local_buffer_prologue(
				   func_block, block->val.basic.stmt_list)) {
			shape_ok = false;
			break;
		}
		if (block->stop)
			break;
		block = block->succ;
	}
	if (loop_count == 0)
		shape_ok = false;
	if (!shape_ok)
		loop_count = 1;
	for (k = 0; k < loop_count; k++) {
		kernel[k] = noct_calloc(1, sizeof(*kernel[k]));
		if (kernel[k] == NULL)
			goto failed;
		kernel[k]->output_param = -1;
		kernel[k]->dispatch_param = -1;
		kernel[k]->param_count = func_block->val.func.param_count;
		if (loop_count > 1)
			snprintf(kernel_name, sizeof(kernel_name), "%s$doall%u",
				 func_block->val.func.name, (unsigned int)k);
		else
			snprintf(kernel_name, sizeof(kernel_name), "%s",
				 func_block->val.func.name);
		kernel[k]->name = noct_strdup(kernel_name);
		kernel[k]->source_name = noct_strdup(func_block->val.func.file_name);
		if (kernel[k]->name == NULL || kernel[k]->source_name == NULL)
			goto failed;
		memset(&ctx, 0, sizeof(ctx));
		ctx.func = func_block;
		ctx.kernel = kernel[k];
		if (!shape_ok) {
			ctx.reason = ACCEL_REJECT_LOOP_SHAPE;
		} else {
			ctx.loop = loop[k];
			(void)accel_build_shader(&ctx, kernel[k], false);
		}
		if (ctx.reason == ACCEL_REJECT_NONE) {
			kernel[k]->eligible = true;
			kernel[k]->source_line = ctx.loop->line;
			if (!gpu_ir_finalize_kernel(kernel[k], ctx.text, ctx.size,
						    ir_error, sizeof(ir_error)))
				goto failed;
			memset(&ctx, 0, sizeof(ctx));
			ctx.func = func_block;
			ctx.loop = loop[k];
			ctx.kernel = kernel[k];
			kernel[k]->param_count = func_block->val.func.param_count;
			kernel[k]->output_param = -1;
			kernel[k]->dispatch_param = -1;
			memset(kernel[k]->param_effect, 0,
			       sizeof(kernel[k]->param_effect));
			memset(kernel[k]->param_range, 0,
			       sizeof(kernel[k]->param_range));
			if (!accel_build_shader(&ctx, kernel[k], true) ||
			    !accel_store_hlsl(kernel[k], ctx.text, ctx.size))
				goto failed;
		} else {
			kernel[k]->eligible = false;
			kernel[k]->rejection_reason = ctx.reason;
			shape_ok = false;
			loop_count = k + 1;
			break;
		}
	}
	if (!shape_ok) {
		int reject_line;
		int reject_reason;
		char reject_message[256];

		reject_line = kernel[loop_count - 1] != NULL ?
			kernel[loop_count - 1]->source_line : func_block->line;
		reject_reason = kernel[loop_count - 1] != NULL ?
			kernel[loop_count - 1]->rejection_reason :
			ACCEL_REJECT_LOOP_SHAPE;
		snprintf(reject_message, sizeof(reject_message),
			 "GPU-only __accel func '%s' cannot be lowered: %s.",
			 func_block->val.func.name,
			 accel_reason_name(reject_reason));
		hir_set_error(reject_line, reject_message);
		if (accel_info)
			fprintf(stderr, "ACCEL: %s:%d: rejected (%s)\n",
				func_block->val.func.file_name, reject_line,
				accel_reason_name(reject_reason));
		goto failed;
	}
	func_block->val.func.accel_kernel = kernel[0];
	if (!accel_build_program(func_block, kernel, loop, loop_count)) {
		hir_set_error(func_block->line,
			      "Failed to build GPU-only accelerator program descriptor.");
		goto failed_owned;
	}
	if (accel_info) {
		for (k = 0; k < loop_count; k++) {
			if (kernel[k]->eligible) {
				int kind_param;

				kind_param = kernel[k]->output_param;
				if (kind_param < 0) {
					for (i = 0; i < kernel[k]->param_count; i++)
						if (kernel[k]->param_transport[i] != ACCEL_TRANSPORT_SCALAR) {
							kind_param = (int)i;
							break;
						}
				}
				kind = kind_param >= 0 ?
					accel_glsl_type(kernel[k]->param_packed_type[kind_param]) : "unknown";
				fprintf(stderr, "ACCEL: %s:%d: accelerator kernel generated (%s32, 1D)\n",
					kernel[k]->source_name, kernel[k]->source_line,
					strcmp(kind, "float") == 0 ? "float" : kind);
				fprintf(stderr, "ACCEL: %s:%d: DOALL %s; strategy %s\n",
					kernel[k]->source_name, kernel[k]->source_line,
					"yes", "parallel block=64");
			}
		}
	}
	if (getenv("NOCT_ACCEL_DEBUG") != NULL) {
		for (k = 0; k < loop_count; k++)
			if (kernel[k]->eligible)
				fprintf(stderr, "%s", kernel[k]->glsl);
	}
	for (k = 1; k < loop_count; k++)
		accel_kernel_free(kernel[k]);
	return true;

failed_owned:
	func_block->val.func.accel_kernel = NULL;
failed:
	for (k = 0; k < ACCEL_PROGRAM_MAX_KERNELS; k++)
		accel_kernel_free(kernel[k]);
	return false;
}

static const char *
accel_reason_name(
	int reason)
{
	switch (reason) {
	case ACCEL_REJECT_PARAMETER:
		return "unsupported parameter contract";
	case ACCEL_REJECT_LOOP_SHAPE:
		return "unsupported loop shape";
	case ACCEL_REJECT_BODY:
		return "unsupported kernel body";
	case ACCEL_REJECT_ACCESS:
		return "invalid accelerator buffer access";
	case ACCEL_REJECT_GLOBAL_RESOURCE:
		return "direct __accel resource dependency; pass it through a _ptr parameter";
	case ACCEL_REJECT_EXPRESSION:
		return "unsupported expression";
	case ACCEL_REJECT_SIZE:
		return "generated shader is too large";
	default:
		return "unknown pattern";
	}
}

static bool
accel_store_hlsl(
	struct accel_kernel *kernel,
	const char *source,
	size_t source_size)
{
	char *copy;

	copy = noct_malloc(source_size + 1);
	if (copy == NULL)
		return false;
	memcpy(copy, source, source_size);
	copy[source_size] = '\0';
	noct_free(kernel->hlsl);
	kernel->hlsl = copy;
	kernel->hlsl_size = source_size;
	return true;
}

static bool
accel_put(
	struct accel_emit *ctx,
	const char *format,
	...)
{
	va_list ap;
	int written;

	if (ctx->reason != ACCEL_REJECT_NONE)
		return false;
	if (ctx->analysis_only)
		return true;
	va_start(ap, format);
	written = vsnprintf(ctx->text + ctx->size,
			    sizeof(ctx->text) - ctx->size, format, ap);
	va_end(ap);
	if (written < 0 || (size_t)written >= sizeof(ctx->text) - ctx->size) {
		ctx->reason = ACCEL_REJECT_SIZE;
		return false;
	}
	ctx->size += (size_t)written;
	return true;
}

static int
accel_param_index(
	struct accel_emit *ctx,
	const char *symbol)
{
	uint32_t i;

	for (i = 0; i < ctx->func->val.func.param_count; i++) {
		if (strcmp(ctx->func->val.func.param_name[i], symbol) == 0)
			return (int)i;
	}
	for (i = 0; i < ctx->local_buffer_count; i++) {
		if (strcmp(ctx->local_buffer_symbol[i], symbol) == 0)
			return (int)(ctx->func->val.func.param_count + i);
	}
	return -1;
}

static bool
accel_is_int_zero(
	struct hir_expr *expr)
{
	return expr != NULL && expr->type == HIR_EXPR_TERM &&
		expr->val.term.term != NULL &&
		expr->val.term.term->type == HIR_TERM_INT &&
		expr->val.term.term->val.i == 0;
}

static bool
accel_is_counter(
	struct accel_emit *ctx,
	struct hir_expr *expr)
{
	return expr != NULL && expr->type == HIR_EXPR_TERM &&
		expr->val.term.term != NULL &&
		expr->val.term.term->type == HIR_TERM_SYMBOL &&
		strcmp(expr->val.term.term->val.symbol,
		       ctx->loop->val.for_.counter_symbol) == 0;
}

static bool
accel_index_offset(struct accel_emit *ctx, struct hir_expr *expr, int *offset)
{
	struct hir_expr *left;
	struct hir_expr *right;
	int distance;

	if (accel_is_counter(ctx, expr)) {
		*offset = 0;
		return true;
	}
	if (expr != NULL && expr->type == HIR_EXPR_TERM &&
	    expr->val.term.term != NULL &&
	    expr->val.term.term->type == HIR_TERM_INT &&
	    expr->val.term.term->val.i == 0) {
		/*
		 * Element zero is the device-side publication slot for a prior
		 * DOSUM.  The caller restricts this form to persistent _ptr data;
		 * DOSUM program validation requires that result buffer to exist.
		 */
		*offset = expr->val.term.term->val.i;
		return true;
	}
	if (expr == NULL ||
	    (expr->type != HIR_EXPR_MINUS && expr->type != HIR_EXPR_PLUS))
		return false;
	left = expr->val.binary.expr[0];
	right = expr->val.binary.expr[1];
	if (!accel_is_counter(ctx, left) || right == NULL ||
	    right->type != HIR_EXPR_TERM || right->val.term.term == NULL ||
	    right->val.term.term->type != HIR_TERM_INT)
		return false;
	distance = right->val.term.term->val.i;
	if (distance < 0)
		return false;
	if (expr->type == HIR_EXPR_MINUS) {
		if (distance > ctx->min_index)
			return false;
		*offset = -distance;
	} else {
		*offset = distance;
	}
	return true;
}

static void
accel_record_range(struct accel_emit *ctx, int param, int offset)
{
	struct accel_param_range *range;
	int64_t min_offset;

	range = &ctx->kernel->param_range[param];
	min_offset = offset < 0 ? 0 : offset;
	if (!range->has_access) {
		range->has_access = true;
		range->min_offset = min_offset;
		range->max_offset = offset;
	} else {
		if (min_offset < range->min_offset) range->min_offset = min_offset;
		if (offset > range->max_offset) range->max_offset = offset;
	}
}

static bool
accel_emit_term(
	struct accel_emit *ctx,
	struct hir_term *term,
	int expected_type)
{
	int param;
	int actual_type;
	const char *constructor;

	switch (term->type) {
	case HIR_TERM_INT:
		if (expected_type == NOCT_PACKED_UINT32)
			return accel_put(ctx, "uint(%d)", term->val.i);
		if (expected_type == NOCT_PACKED_FLOAT32)
			return accel_put(ctx, "float(%d)", term->val.i);
		return accel_put(ctx, "int(%d)", term->val.i);
	case HIR_TERM_FLOAT:
		return accel_put(ctx, "float(%.9g)", (double)term->val.f);
	case HIR_TERM_SYMBOL:
		if (strcmp(term->val.symbol,
			   ctx->loop->val.for_.counter_symbol) == 0) {
			if (expected_type == NOCT_PACKED_UINT32)
				return accel_put(ctx, "i");
			constructor = accel_glsl_type(expected_type);
			if (constructor == NULL) break;
			return accel_put(ctx, "%s(i)", constructor);
		}
		param = accel_param_index(ctx, term->val.symbol);
		if (param >= 0 &&
		    ctx->kernel->param_transport[param] == ACCEL_TRANSPORT_SCALAR &&
		    (ctx->kernel->param_type[param] == NOCT_VALUE_INT ||
		     ctx->kernel->param_type[param] == NOCT_VALUE_FLOAT)) {
			actual_type = ctx->kernel->param_type[param] == NOCT_VALUE_FLOAT ?
				NOCT_PACKED_FLOAT32 : NOCT_PACKED_INT32;
			if (actual_type == expected_type)
				return accel_put(ctx, ctx->hlsl ? "pc_p%d" : "pc.p%d",
						 param);
			constructor = accel_glsl_type(expected_type);
			if (constructor == NULL) break;
			return accel_put(ctx, ctx->hlsl ? "%s(pc_p%d)" :
					 "%s(pc.p%d)", constructor, param);
		}
		break;
	default:
		break;
	}
	ctx->reason = ACCEL_REJECT_EXPRESSION;
	return false;
}

static const char *
accel_binary_op(
	int type)
{
	switch (type) {
	case HIR_EXPR_LT: return "<";
	case HIR_EXPR_LTE: return "<=";
	case HIR_EXPR_GT: return ">";
	case HIR_EXPR_GTE: return ">=";
	case HIR_EXPR_EQ: return "==";
	case HIR_EXPR_NEQ: return "!=";
	case HIR_EXPR_PLUS: return "+";
	case HIR_EXPR_MINUS: return "-";
	case HIR_EXPR_MUL: return "*";
	case HIR_EXPR_DIV: return "/";
	case HIR_EXPR_MOD: return "%";
	case HIR_EXPR_AND: return "&";
	case HIR_EXPR_OR: return "|";
	case HIR_EXPR_XOR: return "^";
	case HIR_EXPR_SHL: return "<<";
	case HIR_EXPR_SHR: return ">>";
	case HIR_EXPR_LAND: return "&&";
	case HIR_EXPR_LOR: return "||";
	default: return NULL;
	}
}

static bool
accel_emit_expr(
	struct accel_emit *ctx,
	struct hir_expr *expr,
	int expected_type)
{
	const char *op;
	struct hir_expr *base;
	int param;
	int offset;
	bool constant_index;
	bool convert;
	const char *constructor;

	if (expr == NULL) {
		ctx->reason = ACCEL_REJECT_EXPRESSION;
		return false;
	}
	if (expr->type == HIR_EXPR_TERM)
		return accel_emit_term(ctx, expr->val.term.term, expected_type);
	if (expr->type == HIR_EXPR_PAR) {
		if (!accel_put(ctx, "(")) return false;
		if (!accel_emit_expr(ctx, expr->val.unary.expr, expected_type)) return false;
		return accel_put(ctx, ")");
	}
	if (expr->type == HIR_EXPR_NEG || expr->type == HIR_EXPR_NOT) {
		if (!accel_put(ctx, expr->type == HIR_EXPR_NEG ? "(-" : "(!"))
			return false;
		if (!accel_emit_expr(ctx, expr->val.unary.expr, expected_type))
			return false;
		return accel_put(ctx, ")");
	}
	if (expr->type == HIR_EXPR_SUBSCR) {
		base = expr->val.binary.expr[0];
		constant_index = expr->val.binary.expr[1] != NULL &&
			expr->val.binary.expr[1]->type == HIR_EXPR_TERM &&
			expr->val.binary.expr[1]->val.term.term != NULL &&
			expr->val.binary.expr[1]->val.term.term->type ==
				HIR_TERM_INT;
		if (base == NULL || base->type != HIR_EXPR_TERM ||
		    base->val.term.term->type != HIR_TERM_SYMBOL) {
			ctx->reason = ACCEL_REJECT_ACCESS;
			return false;
		}
		param = accel_param_index(ctx, base->val.term.term->val.symbol);
		if (param < 0 && ast_is_accel_resource_symbol(
		    base->val.term.term->val.symbol)) {
			ctx->reason = ACCEL_REJECT_GLOBAL_RESOURCE;
			return false;
		}
		if (param < 0 ||
		    (ctx->kernel->param_transport[param] != ACCEL_TRANSPORT_COPY_IN &&
		     ctx->kernel->param_transport[param] != ACCEL_TRANSPORT_DEVICE_PTR)) {
			ctx->reason = ACCEL_REJECT_ACCESS;
			return false;
		}
		if (!accel_index_offset(ctx, expr->val.binary.expr[1], &offset)) {
			ctx->reason = ACCEL_REJECT_ACCESS;
			return false;
		}
		if (constant_index && ctx->kernel->param_transport[param] !=
		    ACCEL_TRANSPORT_DEVICE_PTR) {
			ctx->reason = ACCEL_REJECT_ACCESS;
			return false;
		}
		if (ctx->kernel->param_transport[param] ==
		    ACCEL_TRANSPORT_DEVICE_PTR && offset != 0)
			ctx->shifted_read[param] = true;
		ctx->kernel->param_effect[param] |= ACCEL_EFFECT_READ;
		if (!constant_index)
			accel_record_range(ctx, param, offset);
		convert = ctx->kernel->param_packed_type[param] != expected_type;
		constructor = convert ? accel_glsl_type(expected_type) : NULL;
		if (convert && constructor == NULL) {
			ctx->reason = ACCEL_REJECT_EXPRESSION;
			return false;
		}
		if (convert && !accel_put(ctx, "%s(", constructor)) return false;
		if (!accel_put(ctx, ctx->hlsl ? "p%d[" : "p%d.words[", param))
			return false;
		if (!accel_emit_expr(ctx, expr->val.binary.expr[1], NOCT_PACKED_UINT32))
			return false;
		return accel_put(ctx, convert ? "])" : "]");
	}
	if (expr->type == HIR_EXPR_CALL) {
		int intrinsic;
		int source_type;
		const char *constructor;

		intrinsic = hir_get_intrinsic_call(expr);
		if (intrinsic == HIR_INTRINSIC_FLOAT_FROM) {
			if (expected_type != NOCT_PACKED_FLOAT32) {
				ctx->reason = ACCEL_REJECT_EXPRESSION;
				return false;
			}
			constructor = "float";
			source_type = NOCT_PACKED_INT32;
		} else if (intrinsic == HIR_INTRINSIC_INT_FROM) {
			if (expected_type != NOCT_PACKED_INT32) {
				ctx->reason = ACCEL_REJECT_EXPRESSION;
				return false;
			}
			constructor = "int";
			source_type = NOCT_PACKED_FLOAT32;
		} else {
			ctx->reason = ACCEL_REJECT_EXPRESSION;
			return false;
		}
		if (!accel_put(ctx, "%s(", constructor)) return false;
		if (!accel_emit_expr(ctx, expr->val.call.arg[0], source_type))
			return false;
		return accel_put(ctx, ")");
	}
	op = accel_binary_op(expr->type);
	if (op != NULL) {
		if (expected_type == NOCT_PACKED_FLOAT32 &&
		    (expr->type == HIR_EXPR_MOD || expr->type == HIR_EXPR_AND ||
		     expr->type == HIR_EXPR_OR || expr->type == HIR_EXPR_XOR ||
		     expr->type == HIR_EXPR_SHL || expr->type == HIR_EXPR_SHR)) {
			ctx->reason = ACCEL_REJECT_EXPRESSION;
			return false;
		}
		if (!accel_put(ctx, "(")) return false;
		if (!accel_emit_expr(ctx, expr->val.binary.expr[0], expected_type))
			return false;
		if (!accel_put(ctx, " %s ", op)) return false;
		if (!accel_emit_expr(ctx, expr->val.binary.expr[1], expected_type))
			return false;
		return accel_put(ctx, ")");
	}
	ctx->reason = ACCEL_REJECT_EXPRESSION;
	return false;
}

static bool
accel_emit_stmt(
	struct accel_emit *ctx,
	struct hir_stmt *stmt,
	int indent)
{
	struct hir_expr *base;
	int param;
	int packed_type;

	if (stmt->lhs == NULL || stmt->rhs == NULL ||
	    stmt->lhs->type != HIR_EXPR_SUBSCR) {
		ctx->reason = ACCEL_REJECT_BODY;
		return false;
	}
	base = stmt->lhs->val.binary.expr[0];
	if (base == NULL || base->type != HIR_EXPR_TERM ||
	    base->val.term.term->type != HIR_TERM_SYMBOL ||
	    !accel_is_counter(ctx, stmt->lhs->val.binary.expr[1])) {
		ctx->reason = ACCEL_REJECT_ACCESS;
		return false;
	}
	param = accel_param_index(ctx, base->val.term.term->val.symbol);
	if (param < 0 && ast_is_accel_resource_symbol(
	    base->val.term.term->val.symbol)) {
		ctx->reason = ACCEL_REJECT_GLOBAL_RESOURCE;
		return false;
	}
	if (param < 0 ||
	    (ctx->kernel->param_transport[param] != ACCEL_TRANSPORT_COPY_OUT &&
	     ctx->kernel->param_transport[param] != ACCEL_TRANSPORT_DEVICE_PTR)) {
		ctx->reason = ACCEL_REJECT_ACCESS;
		return false;
	}
	packed_type = ctx->kernel->param_packed_type[param];
	ctx->written[param] = true;
	ctx->kernel->param_effect[param] |= ACCEL_EFFECT_WRITE;
	accel_record_range(ctx, param, 0);
	if (!accel_put(ctx, ctx->hlsl ? "%*sp%d[i] = " :
					 "%*sp%d.words[i] = ", indent, "", param))
		return false;
	if (!accel_emit_expr(ctx, stmt->rhs, packed_type)) return false;
	return accel_put(ctx, ";\n");
}

static bool
accel_is_condition(
	struct hir_expr *expr)
{
	if (expr == NULL)
		return false;
	switch (expr->type) {
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		return true;
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
		return accel_is_condition(expr->val.binary.expr[0]) &&
			accel_is_condition(expr->val.binary.expr[1]);
	case HIR_EXPR_NOT:
		return accel_is_condition(expr->val.unary.expr);
	default:
		return false;
	}
}

static bool
accel_expr_uses_counter(
	struct accel_emit *ctx,
	struct hir_expr *expr)
{
	if (expr == NULL)
		return false;
	if (accel_is_counter(ctx, expr))
		return true;
	switch (expr->type) {
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
		return accel_expr_uses_counter(ctx, expr->val.unary.expr);
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
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
	case HIR_EXPR_SUBSCR:
		return accel_expr_uses_counter(ctx, expr->val.binary.expr[0]) ||
			accel_expr_uses_counter(ctx, expr->val.binary.expr[1]);
	default:
		return false;
	}
}

static bool
accel_int_literal(
	struct hir_expr *expr,
	int *value)
{
	if (expr == NULL || expr->type != HIR_EXPR_TERM ||
	    expr->val.term.term == NULL ||
	    expr->val.term.term->type != HIR_TERM_INT)
		return false;
	*value = expr->val.term.term->val.i;
	return true;
}

static int
accel_condition_min_index(
	struct accel_emit *ctx,
	struct hir_expr *expr)
{
	struct hir_expr *left;
	struct hir_expr *right;
	int bound;
	int a;
	int b;

	if (expr == NULL)
		return ctx->min_index;
	if (expr->type == HIR_EXPR_LAND) {
		a = accel_condition_min_index(ctx, expr->val.binary.expr[0]);
		b = accel_condition_min_index(ctx, expr->val.binary.expr[1]);
		return a > b ? a : b;
	}
	if (expr->type != HIR_EXPR_LT && expr->type != HIR_EXPR_LTE &&
	    expr->type != HIR_EXPR_GT && expr->type != HIR_EXPR_GTE &&
	    expr->type != HIR_EXPR_EQ && expr->type != HIR_EXPR_NEQ)
		return ctx->min_index;
	left = expr->val.binary.expr[0];
	right = expr->val.binary.expr[1];
	bound = ctx->min_index;
	if (accel_is_counter(ctx, left) && accel_int_literal(right, &a)) {
		if (expr->type == HIR_EXPR_GT && a >= 0 && a < INT_MAX)
			bound = a + 1;
		else if (expr->type == HIR_EXPR_GTE && a >= 0)
			bound = a;
		else if (expr->type == HIR_EXPR_NEQ && a == 0)
			bound = 1;
	} else if (accel_int_literal(left, &a) && accel_is_counter(ctx, right)) {
		if (expr->type == HIR_EXPR_LT && a >= 0 && a < INT_MAX)
			bound = a + 1;
		else if (expr->type == HIR_EXPR_LTE && a >= 0)
			bound = a;
	}
	return bound > ctx->min_index ? bound : ctx->min_index;
}

static bool
accel_emit_if_chain(
	struct accel_emit *ctx,
	struct hir_block *block,
	int packed_type,
	int indent,
	int *statement_count)
{
	struct hir_block *chain;
	int old_min_index;
	int before_count;
	int condition_type;
	bool first;
	bool saw_else;

	first = true;
	saw_else = false;
	chain = block;
	while (chain != NULL) {
		old_min_index = ctx->min_index;
		if (chain->val.if_.cond != NULL) {
			if (saw_else || !accel_is_condition(chain->val.if_.cond)) {
				ctx->reason = ACCEL_REJECT_BODY;
				return false;
			}
			if (!accel_put(ctx, first ? "%*sif (" : "%*s} else if (",
				       indent, ""))
				return false;
			condition_type = accel_expr_uses_counter(
				ctx, chain->val.if_.cond) ?
				NOCT_PACKED_UINT32 : packed_type;
			if (!accel_emit_expr(ctx, chain->val.if_.cond,
					     condition_type) ||
			    !accel_put(ctx, ") {\n"))
				return false;
			ctx->min_index = accel_condition_min_index(
				ctx, chain->val.if_.cond);
		} else {
			if (first || chain->val.if_.chain_next != NULL) {
				ctx->reason = ACCEL_REJECT_BODY;
				return false;
			}
			if (!accel_put(ctx, "%*s} else {\n", indent, ""))
				return false;
			saw_else = true;
		}
		before_count = *statement_count;
		if (!accel_emit_blocks(ctx, chain->val.if_.inner, packed_type,
				       indent + 4, statement_count))
			return false;
		ctx->min_index = old_min_index;
		if (*statement_count == before_count) {
			ctx->reason = ACCEL_REJECT_BODY;
			return false;
		}
		first = false;
		chain = chain->val.if_.chain_next;
	}
	if (!saw_else) {
		ctx->reason = ACCEL_REJECT_BODY;
		return false;
	}
	return accel_put(ctx, "%*s}\n", indent, "");
}

static bool
accel_emit_blocks(
	struct accel_emit *ctx,
	struct hir_block *block,
	int packed_type,
	int indent,
	int *statement_count)
{
	struct hir_stmt *stmt;

	while (block != NULL) {
		if (block->type == HIR_BLOCK_BASIC) {
			stmt = block->val.basic.stmt_list;
			while (stmt != NULL) {
				if (!accel_emit_stmt(ctx, stmt, indent)) return false;
				(*statement_count)++;
				stmt = stmt->next;
			}
		} else if (block->type == HIR_BLOCK_IF) {
			if (!accel_emit_if_chain(ctx, block, packed_type, indent,
						 statement_count))
				return false;
		} else {
			ctx->reason = ACCEL_REJECT_BODY;
			return false;
		}
		if (block->stop)
			break;
		block = block->succ;
	}
	return true;
}

static bool
accel_emit_body(
	struct accel_emit *ctx,
	int indent)
{
	uint32_t i;
	int packed_type;
	int statement_count;

	packed_type = -1;
	for (i = 0; i < ctx->kernel->param_count; i++) {
		if (ctx->kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_OUT ||
		    ctx->kernel->param_transport[i] == ACCEL_TRANSPORT_DEVICE_PTR) {
			packed_type = ctx->kernel->param_packed_type[i];
			break;
		}
	}
	statement_count = 0;
	ctx->min_index = 0;
	if (!accel_emit_blocks(ctx, ctx->loop->val.for_.inner, packed_type, indent,
			       &statement_count))
		return false;
	if (statement_count == 0) {
		ctx->reason = ACCEL_REJECT_BODY;
		return false;
	}
	return true;
}

static const char *
accel_glsl_type(
	int packed_type)
{
	switch (packed_type) {
	case NOCT_PACKED_INT32: return "int";
	case NOCT_PACKED_UINT32: return "uint";
	case NOCT_PACKED_FLOAT32: return "float";
	default: return NULL;
	}
}

static bool
accel_build_shader(
	struct accel_emit *ctx,
	struct accel_kernel *kernel,
	bool hlsl)
{
	struct hir_block *block;
	struct hir_expr *stop;
	uint32_t i;
	int stop_param;
	const char *type;
	const char *qualifier;
	struct hir_memory_catalog catalog;
	struct hir_loop_summary *summary;
	struct hir_doall_result doall;
	struct hir_local *local;

	ctx->hlsl = hlsl;
	kernel->descriptor_version = 3;
	kernel->func_kind = NOCT_FUNC_ACCEL;

	for (i = 0; i < ctx->func->val.func.param_count; i++) {
		kernel->param_type[i] = ctx->func->val.func.param_type[i];
		kernel->param_packed_type[i] =
			ctx->func->val.func.param_packed_type[i];
		kernel->param_transport[i] =
			ctx->func->val.func.param_accel_transport[i];
		kernel->param_access[i] = kernel->param_transport[i];
		kernel->param_effect[i] =
			kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_IN ?
			ACCEL_EFFECT_READ :
			kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_OUT ?
			ACCEL_EFFECT_WRITE : ACCEL_EFFECT_NONE;
		kernel->param_range[i].status =
			kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR ?
			ACCEL_RANGE_NOT_APPLICABLE : ACCEL_RANGE_COMPLETE;
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_OUT) {
			kernel->output_param = (int)i;
		}
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR) {
			if (!ctx->func->val.func.param_restricted[i] ||
			    accel_glsl_type(kernel->param_packed_type[i]) == NULL) {
				ctx->reason = ACCEL_REJECT_PARAMETER;
				return false;
			}
		} else if (kernel->param_type[i] != NOCT_VALUE_INT &&
			   kernel->param_type[i] != NOCT_VALUE_FLOAT) {
			ctx->reason = ACCEL_REJECT_PARAMETER;
			return false;
		}
	}
	ctx->local_buffer_count = 0;
	local = ctx->func->val.func.local;
	while (local != NULL) {
		if (!local->is_parameter &&
		    local->storage_class == HIR_LOCAL_STORAGE_LOGICAL_BUFFER) {
			if (kernel->param_count >= NOCT_ARG_MAX ||
			    ctx->local_buffer_count >= NOCT_ARG_MAX ||
			    accel_glsl_type(local->declared_packed_type) == NULL ||
			    local->initializer == NULL ||
			    local->initializer->type != HIR_EXPR_THISCALL ||
			    local->initializer->val.thiscall.arg_count != 1) {
				ctx->reason = ACCEL_REJECT_PARAMETER;
				return false;
			}
			ctx->local_buffer_symbol[ctx->local_buffer_count] =
				local->symbol;
			ctx->local_buffer_type[ctx->local_buffer_count] =
				local->declared_packed_type;
			ctx->local_buffer_length[ctx->local_buffer_count] =
				local->initializer->val.thiscall.arg[0];
			kernel->param_type[kernel->param_count] = NOCT_VALUE_PACKED;
			kernel->param_packed_type[kernel->param_count] =
				local->declared_packed_type;
			kernel->param_transport[kernel->param_count] =
				ACCEL_TRANSPORT_DEVICE_PTR;
			kernel->param_access[kernel->param_count] =
				ACCEL_TRANSPORT_DEVICE_PTR;
			kernel->param_effect[kernel->param_count] = ACCEL_EFFECT_NONE;
			kernel->param_range[kernel->param_count].status =
				ACCEL_RANGE_COMPLETE;
			kernel->param_count++;
			ctx->local_buffer_count++;
		}
		local = local->next;
	}

	if (ctx->loop == NULL) {
		block = ctx->func->val.func.inner;
		while (block != NULL) {
			if (block->type == HIR_BLOCK_FOR) {
				if (ctx->loop != NULL) {
					ctx->reason = ACCEL_REJECT_LOOP_SHAPE;
					return false;
				}
				ctx->loop = block;
			} else if (block->type == HIR_BLOCK_BASIC &&
				   block->val.basic.stmt_list != NULL) {
				ctx->reason = ACCEL_REJECT_BODY;
				return false;
			}
			if (block->stop) break;
			block = block->succ;
		}
	}
	if (ctx->loop == NULL || !ctx->loop->val.for_.is_ranged ||
	    !accel_is_int_zero(ctx->loop->val.for_.start)) {
		ctx->reason = ACCEL_REJECT_LOOP_SHAPE;
		return false;
	}
	stop = ctx->loop->val.for_.stop;
	if (stop == NULL || stop->type != HIR_EXPR_TERM ||
	    (stop->val.term.term->type != HIR_TERM_SYMBOL &&
	     stop->val.term.term->type != HIR_TERM_INT)) {
		ctx->reason = ACCEL_REJECT_LOOP_SHAPE;
		return false;
	}
	if (stop->val.term.term->type == HIR_TERM_SYMBOL) {
		stop_param = accel_param_index(ctx,
					       stop->val.term.term->val.symbol);
		if (stop_param < 0 ||
		    ctx->func->val.func.param_type[stop_param] != NOCT_VALUE_INT) {
			ctx->reason = ACCEL_REJECT_LOOP_SHAPE;
			return false;
		}
		kernel->dispatch_param = stop_param;
	} else {
		if (stop->val.term.term->val.i < 0) {
			ctx->reason = ACCEL_REJECT_LOOP_SHAPE;
			return false;
		}
		kernel->dispatch_param = -1;
	}

	/* Validate and summarize effects before choosing the launch strategy. */
	ctx->analysis_only = true;
	if (!accel_emit_body(ctx, 4))
		return false;
	ctx->analysis_only = false;
	if (!hir_memory_catalog_build_accel(ctx->func, &catalog) ||
	    !hir_loop_analyze(ctx->func, ctx->loop, &catalog, &summary))
		return false;
	if (!hir_doall_classify(summary, &doall)) {
		hir_loop_summary_free(summary);
		return false;
	}
	hir_loop_summary_free(summary);
	if (doall.classification != HIR_PAR_CLASS_DOALL) {
		ctx->reason = ACCEL_REJECT_ACCESS;
		return false;
	}
	kernel->parallel_mode = ACCEL_PARALLEL_DOALL;

	if (hlsl) {
		if (!accel_put(ctx, "// Noct HLSL SM 5.1\n"))
			return false;
	} else if (!accel_put(ctx, "#version 450\n"
				     "layout(local_size_x = %d, local_size_y = 1, local_size_z = 1) in;\n",
				     kernel->parallel_mode == ACCEL_PARALLEL_DOALL ? 64 : 1))
		return false;
	for (i = 0; i < kernel->param_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) continue;
		type = accel_glsl_type(kernel->param_packed_type[i]);
		if (hlsl) {
			if (!accel_put(ctx,
				"RWStructuredBuffer<%s> p%u : register(u%u);\n",
				type, i, i)) return false;
		} else {
			qualifier = kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_IN ?
				"readonly" : kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_OUT ?
				"writeonly" : "";
			if (!accel_put(ctx,
				"layout(set = 0, binding = %u, std430) %s buffer P%u { %s words[]; } p%u;\n",
				i, qualifier, i, type, i)) return false;
		}
	}
	if (!accel_put(ctx, hlsl ? "cbuffer NoctPush : register(b0) {\n"
					"    uint element_count;\n"
					"    uint dispatch_stride;\n" :
					"layout(push_constant) uniform PushConstants {\n"
					"    uint element_count;\n")) return false;
	for (i = 0; i < kernel->param_count; i++) {
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR) continue;
		type = kernel->param_type[i] == NOCT_VALUE_FLOAT ? "float" : "int";
		if (!accel_put(ctx, hlsl ? "    %s pc_p%u;\n" :
					 "    %s p%u;\n", type, i)) return false;
	}
	if (!accel_put(ctx, hlsl ?
			"};\n[numthreads(64, 1, 1)]\n"
			"void main(uint3 dispatch_id : SV_DispatchThreadID) {\n" :
			"} pc;\nvoid main() {\n")) return false;
	if (!accel_put(ctx, hlsl ?
			"    uint i = dispatch_id.x;\n"
			"    uint stride = dispatch_stride;\n"
			"    for (; i < element_count; i += stride) {\n" :
			"    uint i = gl_GlobalInvocationID.x;\n"
			"    uint stride = gl_NumWorkGroups.x * gl_WorkGroupSize.x;\n"
			"    for (; i < pc.element_count; i += stride) {\n"))
		return false;
	if (!accel_emit_body(ctx, 8)) return false;
	return accel_put(ctx, "    }\n}\n");
}

static int
accel_element_width(
	int packed_type)
{
	if (packed_type == NOCT_PACKED_INT32 ||
	    packed_type == NOCT_PACKED_UINT32 ||
	    packed_type == NOCT_PACKED_FLOAT32)
		return 4;
	return 0;
}

static bool
accel_is_local_buffer_prologue(
	struct hir_block *func,
	struct hir_stmt *stmt)
{
	struct hir_local *local;

	while (stmt != NULL) {
		local = func->val.func.local;
		while (local != NULL) {
			if (local->declaration_stmt == stmt && !local->is_parameter &&
			    local->storage_class == HIR_LOCAL_STORAGE_LOGICAL_BUFFER)
				break;
			local = local->next;
		}
		if (local == NULL)
			return false;
		stmt = stmt->next;
	}
	return true;
}

static bool
accel_is_reduction_decl_tail(
	struct hir_block *func,
	struct hir_stmt *stmt)
{
	struct hir_local *local;
	const struct hir_expr *initializer;
	const struct hir_term *term;

	while (stmt != NULL) {
		local = func->val.func.local;
		while (local != NULL) {
			if (local->declaration_stmt == stmt &&
			    local->declaration_kind == HIR_LOCAL_DECL_VAR)
				break;
			local = local->next;
		}
		if (local == NULL ||
		    (local->declared_scalar_kind != HIR_DECL_SCALAR_INT32 &&
		     local->declared_scalar_kind != HIR_DECL_SCALAR_UINT32 &&
		     local->declared_scalar_kind != HIR_DECL_SCALAR_FLOAT32))
			return false;
		initializer = local->initializer;
		if (initializer == NULL || initializer->type != HIR_EXPR_TERM ||
		    initializer->val.term.term == NULL)
			return false;
		term = initializer->val.term.term;
		if ((term->type != HIR_TERM_INT || term->val.i != 0) &&
		    (term->type != HIR_TERM_FLOAT || term->val.f != 0.0f))
			return false;
		stmt = stmt->next;
	}
	return true;
}

static bool
accel_build_program(
	struct hir_block *func,
	struct accel_kernel **outer_kernel,
	struct hir_block **loop,
	uint32_t kernel_count)
{
	struct accel_program *program;
	struct accel_kernel *internal;
	struct accel_buffer_desc *buffer;
	struct accel_program_step *step;
	int buffer_map[NOCT_ARG_MAX];
	int expr_map[NOCT_ARG_MAX];
	uint32_t buffer_count;
	uint32_t expr_count;
	uint32_t i;
	uint32_t k;
	unsigned int aggregate_effect;
	struct hir_local *local;
	uint32_t local_index;
	int param_index;
	int length_param;
	const struct hir_expr *length_expr;
	const char *length_symbol;
	const struct hir_expr *stop_expr;
	bool defined;
	char error[128];

	program = noct_calloc(1, sizeof(*program));
	if (program == NULL)
		return false;
	program->descriptor_version = ACCEL_PROGRAM_VERSION;
	program->name = noct_strdup(func->val.func.name);
	program->source_name = noct_strdup(func->val.func.file_name);
	program->source_line = outer_kernel[0]->source_line;
	program->outer_param_count = func->val.func.param_count;
	program->expr = noct_calloc(ACCEL_PROGRAM_MAX_EXPRS,
				    sizeof(*program->expr));
	program->buffer = noct_calloc(ACCEL_PROGRAM_MAX_BUFFERS,
				      sizeof(*program->buffer));
	program->kernel = noct_calloc(kernel_count, sizeof(*program->kernel));
	program->step = noct_calloc(kernel_count, sizeof(*program->step));
	if (program->name == NULL || program->source_name == NULL ||
	    program->expr == NULL || program->buffer == NULL ||
	    program->kernel == NULL || program->step == NULL)
		goto failed;
	buffer_count = 0;
	expr_count = 0;
	for (i = 0; i < NOCT_ARG_MAX; i++) {
		buffer_map[i] = -1;
		expr_map[i] = -1;
	}
	for (i = 0; i < func->val.func.param_count; i++) {
		aggregate_effect = 0;
		for (k = 0; k < kernel_count; k++)
			aggregate_effect |= outer_kernel[k]->param_effect[i];
		program->outer_param_effect[i] = aggregate_effect;
		program->outer_param_range[i] = outer_kernel[0]->param_range[i];
		for (k = 1; k < kernel_count; k++) {
			if (outer_kernel[k]->param_range[i].has_access) {
				if (!program->outer_param_range[i].has_access ||
				    outer_kernel[k]->param_range[i].min_offset <
				    program->outer_param_range[i].min_offset)
					program->outer_param_range[i].min_offset =
						outer_kernel[k]->param_range[i].min_offset;
				if (!program->outer_param_range[i].has_access ||
				    outer_kernel[k]->param_range[i].max_offset >
				    program->outer_param_range[i].max_offset)
					program->outer_param_range[i].max_offset =
						outer_kernel[k]->param_range[i].max_offset;
				program->outer_param_range[i].has_access = true;
			}
		}
		if (outer_kernel[0]->param_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			expr_map[i] = (int)expr_count;
			program->expr[expr_count].op = ACCEL_EXPR_SCALAR_ARG;
			program->expr[expr_count].ref = (int)i;
			expr_count++;
			continue;
		}
		buffer_map[i] = (int)buffer_count;
		buffer = &program->buffer[buffer_count];
		buffer->id = (int)buffer_count;
		buffer->name = noct_strdup(func->val.func.param_name[i]);
		if (buffer->name == NULL)
			goto failed;
		buffer->source_line = outer_kernel[0]->source_line;
		buffer->outer_param = (int)i;
		buffer->element_kind = outer_kernel[0]->param_packed_type[i];
		buffer->element_width = accel_element_width(buffer->element_kind);
		buffer->length_expr = (int)expr_count;
		buffer->read_start_expr = -1;
		buffer->read_end_expr = -1;
		buffer->write_start_expr = -1;
		buffer->write_end_expr = -1;
		buffer->first_step = 0;
		buffer->last_step = 0;
		buffer->initially_defined =
			(aggregate_effect & ACCEL_EFFECT_READ) != 0;
		buffer->upload = aggregate_effect != ACCEL_EFFECT_WRITE &&
			outer_kernel[0]->param_transport[i] != ACCEL_TRANSPORT_DEVICE_PTR;
		buffer->download = (aggregate_effect & ACCEL_EFFECT_WRITE) != 0 &&
			outer_kernel[0]->param_transport[i] != ACCEL_TRANSPORT_DEVICE_PTR;
		if (outer_kernel[0]->param_transport[i] == ACCEL_TRANSPORT_COPY_IN)
			buffer->origin = ACCEL_BUFFER_HOST_IN;
		else if (outer_kernel[0]->param_transport[i] ==
			 ACCEL_TRANSPORT_COPY_OUT)
			buffer->origin = ACCEL_BUFFER_HOST_OUT;
		else
			buffer->origin = ACCEL_BUFFER_DEVICE_PTR;
		program->expr[expr_count].op = ACCEL_EXPR_BUFFER_LENGTH;
		program->expr[expr_count].ref = (int)buffer_count;
		expr_count++;
		buffer_count++;
	}
	local_index = 0;
	local = func->val.func.local;
	while (local != NULL) {
		if (local->is_parameter ||
		    local->storage_class != HIR_LOCAL_STORAGE_LOGICAL_BUFFER) {
			local = local->next;
			continue;
		}
		param_index = (int)(func->val.func.param_count + local_index);
		if (param_index >= NOCT_ARG_MAX ||
		    buffer_count >= ACCEL_PROGRAM_MAX_BUFFERS)
			goto failed;
		aggregate_effect = 0;
		for (k = 0; k < kernel_count; k++)
			aggregate_effect |= outer_kernel[k]->param_effect[param_index];
		buffer_map[param_index] = (int)buffer_count;
		buffer = &program->buffer[buffer_count];
		buffer->id = (int)buffer_count;
		buffer->name = noct_strdup(local->symbol);
		if (buffer->name == NULL)
			goto failed;
		buffer->source_line = local->declaration_line;
		buffer->origin = ACCEL_BUFFER_LOCAL;
		buffer->outer_param = -1;
		buffer->element_kind = local->declared_packed_type;
		buffer->element_width = accel_element_width(buffer->element_kind);
		length_expr = outer_kernel[0]->param_count > (uint32_t)param_index ?
			local->initializer->val.thiscall.arg[0] : NULL;
		length_symbol = length_expr != NULL &&
			length_expr->type == HIR_EXPR_TERM &&
			length_expr->val.term.term->type == HIR_TERM_SYMBOL ?
			length_expr->val.term.term->val.symbol : NULL;
		length_param = -1;
		if (length_symbol != NULL) {
			for (i = 0; i < func->val.func.param_count; i++) {
				if (strcmp(func->val.func.param_name[i],
					   length_symbol) == 0) {
					length_param = (int)i;
					break;
				}
			}
		}
		if (length_param >= 0 && expr_map[length_param] >= 0) {
			buffer->length_expr = expr_map[length_param];
		} else if (length_expr != NULL &&
			   length_expr->type == HIR_EXPR_TERM &&
			   length_expr->val.term.term->type == HIR_TERM_INT) {
			if (expr_count >= ACCEL_PROGRAM_MAX_EXPRS)
				goto failed;
			buffer->length_expr = (int)expr_count;
			program->expr[expr_count].op = ACCEL_EXPR_CONST;
			program->expr[expr_count].value =
				length_expr->val.term.term->val.i;
			expr_count++;
		} else {
			goto failed;
		}
		buffer->read_start_expr = -1;
		buffer->read_end_expr = -1;
		buffer->write_start_expr = -1;
		buffer->write_end_expr = -1;
		buffer->first_step = -1;
		buffer->last_step = -1;
		defined = false;
		for (k = 0; k < kernel_count; k++) {
			unsigned int effect;

			effect = outer_kernel[k]->param_effect[param_index];
			if (effect == ACCEL_EFFECT_NONE)
				continue;
			if (buffer->first_step < 0)
				buffer->first_step = (int)k;
			buffer->last_step = (int)k;
			if ((effect & ACCEL_EFFECT_READ) != 0 && !defined)
				goto failed;
			if ((effect & ACCEL_EFFECT_WRITE) != 0) {
				if (length_param < 0 ||
				    outer_kernel[k]->dispatch_param != length_param)
					goto failed;
				defined = true;
			}
		}
		buffer->initially_defined = false;
		buffer->upload = false;
		buffer->download = false;
		buffer_count++;
		local_index++;
		local = local->next;
	}
	program->expr_count = expr_count;
	program->buffer_count = buffer_count;
	for (k = 0; k < kernel_count; k++) {
		internal = accel_kernel_clone(outer_kernel[k]);
		if (internal == NULL)
			goto failed;
		internal->func_kind = NOCT_FUNC_GPU;
		for (i = 0; i < internal->param_count; i++) {
			if (internal->param_transport[i] != ACCEL_TRANSPORT_SCALAR) {
				internal->param_transport[i] = ACCEL_TRANSPORT_DEVICE_PTR;
				internal->param_access[i] = ACCEL_TRANSPORT_DEVICE_PTR;
			}
		}
		program->kernel[k] = internal;
		program->kernel_count++;
		step = &program->step[k];
		step->kind = ACCEL_STEP_DOALL_DISPATCH;
		step->source_line = outer_kernel[k]->source_line;
		step->kernel = (int)k;
		stop_expr = loop[k]->val.for_.stop;
		if (outer_kernel[k]->dispatch_param >= 0) {
			step->trip_expr =
				expr_map[outer_kernel[k]->dispatch_param];
		} else if (stop_expr != NULL &&
			   stop_expr->type == HIR_EXPR_TERM &&
			   stop_expr->val.term.term->type == HIR_TERM_INT &&
			   expr_count < ACCEL_PROGRAM_MAX_EXPRS) {
			step->trip_expr = (int)expr_count;
			program->expr[expr_count].op = ACCEL_EXPR_CONST;
			program->expr[expr_count].value =
				stop_expr->val.term.term->val.i;
			expr_count++;
			program->expr_count = expr_count;
		} else {
			goto failed;
		}
		step->block_size = 64;
		step->binding_count = outer_kernel[k]->param_count;
		for (i = 0; i < outer_kernel[k]->param_count; i++) {
			step->binding[i].kernel_param = (int)i;
			if (outer_kernel[k]->param_transport[i] ==
			    ACCEL_TRANSPORT_SCALAR) {
				step->binding[i].kind = ACCEL_BIND_SCALAR_EXPR;
				step->binding[i].value = expr_map[i];
			} else {
				step->binding[i].kind = ACCEL_BIND_BUFFER;
				step->binding[i].value = buffer_map[i];
			}
		}
	}
	program->step_count = kernel_count;
	if (!accel_program_validate(program, error, sizeof(error)))
		goto failed;
	func->val.func.accel_program = program;
	return true;

failed:
	accel_program_free(program);
	return false;
}

static struct accel_kernel *
accel_build_doall_kernel(
	struct hir_block *func,
	struct hir_block *loop,
	const char *name)
{
	struct accel_kernel *kernel;
	struct accel_emit ctx;
	char error[128];

	kernel = noct_calloc(1, sizeof(*kernel));
	if (kernel == NULL)
		return NULL;
	kernel->output_param = -1;
	kernel->dispatch_param = -1;
	kernel->param_count = func->val.func.param_count;
	kernel->name = noct_strdup(name);
	kernel->source_name = noct_strdup(func->val.func.file_name);
	if (kernel->name == NULL || kernel->source_name == NULL) {
		accel_kernel_free(kernel);
		return NULL;
	}
	memset(&ctx, 0, sizeof(ctx));
	ctx.func = func;
	ctx.loop = loop;
	ctx.kernel = kernel;
	(void)accel_build_shader(&ctx, kernel, false);
	if (ctx.reason != ACCEL_REJECT_NONE) {
		accel_kernel_free(kernel);
		return NULL;
	}
	kernel->eligible = true;
	kernel->source_line = loop->line;
	if (!gpu_ir_finalize_kernel(kernel, ctx.text, ctx.size,
				    error, sizeof(error))) {
		accel_kernel_free(kernel);
		return NULL;
	}
	memset(&ctx, 0, sizeof(ctx));
	ctx.func = func;
	ctx.loop = loop;
	ctx.kernel = kernel;
	kernel->param_count = func->val.func.param_count;
	kernel->output_param = -1;
	kernel->dispatch_param = -1;
	memset(kernel->param_effect, 0, sizeof(kernel->param_effect));
	memset(kernel->param_range, 0, sizeof(kernel->param_range));
	if (!accel_build_shader(&ctx, kernel, true) ||
	    !accel_store_hlsl(kernel, ctx.text, ctx.size)) {
		accel_kernel_free(kernel);
		return NULL;
	}
	return kernel;
}

/* Build the initial map kernel and the reusable fold kernel for DOSUM. */
static int
accel_try_build_dosum_at(
	struct hir_block *func,
	bool accel_info,
	uint32_t target_index,
	bool reuse_program)
{
	struct hir_block *block;
	struct hir_block *loop;
	struct hir_block *all_loop[ACCEL_PROGRAM_MAX_KERNELS];
	struct hir_block *loop_list[ACCEL_PROGRAM_MAX_KERNELS];
	struct accel_kernel *prefix_kernel[ACCEL_PROGRAM_MAX_KERNELS];
	struct hir_stmt *post;
	struct hir_expr *post_base;
	const char *post_symbol;
	struct hir_memory_catalog catalog;
	struct hir_loop_summary *summary;
	struct hir_loop_summary *candidate_summary;
	struct hir_dosum_result dosum;
	struct hir_dosum_result candidate_dosum;
	struct hir_local *local;
	struct accel_kernel *map_kernel;
	struct accel_kernel *fold_kernel;
	struct accel_kernel *shell;
	struct accel_program *program;
	struct accel_kernel **new_kernel;
	struct accel_program_step *new_step;
	struct accel_program_step *reduction_step;
	struct accel_emit ctx;
	const char *type;
	const char *zero;
	uint32_t i;
	uint32_t k;
	uint32_t loop_count;
	uint32_t prefix_count;
	uint32_t dosum_step_index;
	uint32_t reduction_count;
	uint32_t local_count;
	uint32_t scratch_param;
	uint32_t output_param;
	uint32_t expr_count;
	uint32_t buffer_count;
	uint32_t base_kernel_count;
	uint32_t base_step_count;
	uint32_t map_kernel_index;
	int expr_map[NOCT_ARG_MAX];
	int buffer_map[NOCT_ARG_MAX];
	int trip_expr;
	int scratch_length_expr;
	char name[256];
	char error[128];

	memset(prefix_kernel, 0, sizeof(prefix_kernel));
	error[0] = '\0';
	map_kernel = NULL;
	fold_kernel = NULL;
	shell = NULL;
	program = NULL;
	loop = NULL;
	summary = NULL;
	loop_count = 0;
	block = func->val.func.inner;
	while (block != NULL) {
		if (block->type == HIR_BLOCK_FOR) {
			if (loop_count >= ACCEL_PROGRAM_MAX_KERNELS)
				return 0;
			all_loop[loop_count++] = block;
		}
		if (block->stop) break;
		block = block->succ;
	}
	if (loop_count == 0)
		return 0;
	if (!hir_memory_catalog_build_accel(func, &catalog))
		return 0;
	prefix_count = 0;
	dosum_step_index = 0;
	reduction_count = 0;
	for (k = 0; k < loop_count; k++) {
		candidate_summary = NULL;
		if (!hir_loop_analyze(func, all_loop[k], &catalog,
				      &candidate_summary))
			return -1;
		if (!hir_dosum_classify(candidate_summary, &candidate_dosum)) {
			hir_loop_summary_free(candidate_summary);
			return -1;
		}
		if (candidate_dosum.classification == HIR_PAR_CLASS_DOSUM) {
			if (reduction_count == target_index) {
				loop = all_loop[k];
				summary = candidate_summary;
				dosum = candidate_dosum;
				dosum_step_index = prefix_count + target_index;
			} else {
				hir_loop_summary_free(candidate_summary);
			}
			reduction_count++;
		} else {
			loop_list[prefix_count++] = all_loop[k];
			hir_loop_summary_free(candidate_summary);
		}
	}
	if (loop == NULL) {
		return 0;
	}
	/* Publish each reduction through the immediately following element zero. */
	post = loop->succ != NULL && loop->succ->type == HIR_BLOCK_BASIC ?
		loop->succ->val.basic.stmt_list : NULL;
	if (post == NULL || !accel_is_reduction_decl_tail(func, post->next) ||
	    post->lhs == NULL ||
	    post->lhs->type != HIR_EXPR_SUBSCR || post->rhs == NULL ||
	    post->rhs->type != HIR_EXPR_TERM ||
	    post->rhs->val.term.term->type != HIR_TERM_SYMBOL ||
	    strcmp(post->rhs->val.term.term->val.symbol,
		   dosum.accumulator_symbol) != 0 ||
	    post->lhs->val.binary.expr[1]->type != HIR_EXPR_TERM ||
	    post->lhs->val.binary.expr[1]->val.term.term->type != HIR_TERM_INT ||
	    post->lhs->val.binary.expr[1]->val.term.term->val.i != 0) {
		hir_loop_summary_free(summary);
		return target_index == 0 ? 0 : -1;
	}
	post_base = post->lhs->val.binary.expr[0];
	post_symbol = post_base != NULL && post_base->type == HIR_EXPR_TERM &&
		post_base->val.term.term->type == HIR_TERM_SYMBOL ?
		post_base->val.term.term->val.symbol : NULL;
	output_param = NOCT_ARG_MAX;
	for (i = 0; i < func->val.func.param_count; i++)
		if (post_symbol != NULL &&
		    strcmp(func->val.func.param_name[i], post_symbol) == 0)
			output_param = i;
	if (output_param >= func->val.func.param_count ||
	    (func->val.func.param_accel_transport[output_param] !=
		ACCEL_TRANSPORT_COPY_OUT &&
	     func->val.func.param_accel_transport[output_param] !=
		ACCEL_TRANSPORT_DEVICE_PTR)) {
		hir_loop_summary_free(summary);
		return target_index == 0 ? 0 : -1;
	}
	if ((dosum.value_type == HIR_DECL_SCALAR_INT32 &&
	     func->val.func.param_packed_type[output_param] !=
		NOCT_PACKED_INT32) ||
	    (dosum.value_type == HIR_DECL_SCALAR_UINT32 &&
	     func->val.func.param_packed_type[output_param] !=
		NOCT_PACKED_UINT32) ||
	    (dosum.value_type == HIR_DECL_SCALAR_FLOAT32 &&
	     func->val.func.param_packed_type[output_param] !=
		NOCT_PACKED_FLOAT32)) {
		hir_loop_summary_free(summary);
		return target_index == 0 ? 0 : -1;
	}
	type = accel_glsl_type(func->val.func.param_packed_type[output_param]);
	if (type == NULL) {
		hir_loop_summary_free(summary);
		return target_index == 0 ? 0 : -1;
	}
	zero = strcmp(type, "float") == 0 ? "0.0" :
		strcmp(type, "uint") == 0 ? "uint(0)" : "int(0)";
	for (k = 0; !reuse_program && k < prefix_count; k++) {
		snprintf(name, sizeof(name), "%s$doall%u",
			 func->val.func.name, (unsigned int)k);
		prefix_kernel[k] = accel_build_doall_kernel(
			func, loop_list[k], name);
		if (prefix_kernel[k] == NULL) {
			if (getenv("NOCT_ACCEL_DEBUG") != NULL)
				fprintf(stderr,
					"ACCEL: DOSUM DOALL kernel %u failed at line %d\n",
					(unsigned int)k, loop_list[k]->line);
			goto failed;
		}
	}
	map_kernel = noct_calloc(1, sizeof(*map_kernel));
	fold_kernel = noct_calloc(1, sizeof(*fold_kernel));
	program = noct_calloc(1, sizeof(*program));
	if (map_kernel == NULL || fold_kernel == NULL || program == NULL)
		goto failed;
	snprintf(name, sizeof(name), "%s$dosum%u_map", func->val.func.name,
		 (unsigned int)target_index);
	map_kernel->name = noct_strdup(name);
	map_kernel->source_name = noct_strdup(func->val.func.file_name);
	map_kernel->descriptor_version = 3;
	map_kernel->func_kind = NOCT_FUNC_GPU;
	map_kernel->eligible = true;
	map_kernel->parallel_mode = ACCEL_PARALLEL_DOALL;
	map_kernel->source_line = loop->line;
	map_kernel->dispatch_param = -1;
	map_kernel->output_param = -1;
	map_kernel->param_count = func->val.func.param_count;
	if (map_kernel->name == NULL ||
	    map_kernel->source_name == NULL) goto failed;
	for (i = 0; i < func->val.func.param_count; i++) {
		map_kernel->param_type[i] = func->val.func.param_type[i];
		map_kernel->param_packed_type[i] =
			func->val.func.param_packed_type[i];
		map_kernel->param_transport[i] =
			func->val.func.param_accel_transport[i];
		map_kernel->param_access[i] = map_kernel->param_transport[i];
		map_kernel->param_range[i].status =
			map_kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR ?
			ACCEL_RANGE_NOT_APPLICABLE : ACCEL_RANGE_COMPLETE;
	}
	if (loop->val.for_.stop == NULL ||
	    loop->val.for_.stop->type != HIR_EXPR_TERM ||
	    loop->val.for_.stop->val.term.term->type != HIR_TERM_SYMBOL)
		goto failed;
	for (i = 0; i < func->val.func.param_count; i++)
		if (strcmp(func->val.func.param_name[i],
			   loop->val.for_.stop->val.term.term->val.symbol) == 0)
			map_kernel->dispatch_param = (int)i;
	if (map_kernel->dispatch_param < 0)
		goto failed;
	memset(&ctx, 0, sizeof(ctx));
	ctx.func = func;
	ctx.loop = loop;
	ctx.kernel = map_kernel;
	local_count = 0;
	local = func->val.func.local;
	while (local != NULL) {
		if (!local->is_parameter &&
		    local->storage_class == HIR_LOCAL_STORAGE_LOGICAL_BUFFER) {
			if (map_kernel->param_count >= NOCT_ARG_MAX ||
			    local_count >= NOCT_ARG_MAX)
				goto failed;
			ctx.local_buffer_symbol[local_count] = local->symbol;
			ctx.local_buffer_type[local_count] =
				local->declared_packed_type;
			ctx.local_buffer_length[local_count] =
				local->initializer->val.thiscall.arg[0];
			map_kernel->param_type[map_kernel->param_count] =
				NOCT_VALUE_PACKED;
			map_kernel->param_packed_type[map_kernel->param_count] =
				local->declared_packed_type;
			map_kernel->param_transport[map_kernel->param_count] =
				ACCEL_TRANSPORT_DEVICE_PTR;
			map_kernel->param_access[map_kernel->param_count] =
				ACCEL_TRANSPORT_DEVICE_PTR;
			map_kernel->param_range[map_kernel->param_count].status =
				ACCEL_RANGE_COMPLETE;
			map_kernel->param_count++;
			local_count++;
		}
		local = local->next;
	}
	ctx.local_buffer_count = local_count;
	scratch_param = map_kernel->param_count;
	if (scratch_param >= NOCT_ARG_MAX)
		goto failed;
	map_kernel->param_type[scratch_param] = NOCT_VALUE_PACKED;
	map_kernel->param_packed_type[scratch_param] =
		func->val.func.param_packed_type[output_param];
	map_kernel->param_transport[scratch_param] = ACCEL_TRANSPORT_DEVICE_PTR;
	map_kernel->param_access[scratch_param] = ACCEL_TRANSPORT_DEVICE_PTR;
	map_kernel->param_effect[scratch_param] = ACCEL_EFFECT_WRITE;
	map_kernel->param_count++;
	if (!accel_put(&ctx, "#version 450\nlayout(local_size_x = 64) in;\n"))
		goto failed;
	for (i = 0; i < scratch_param; i++) {
		const char *pt;

		if (map_kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
			continue;
		pt = accel_glsl_type(map_kernel->param_packed_type[i]);
		if (pt == NULL || !accel_put(&ctx,
			"layout(set=0,binding=%u,std430) buffer P%u { %s words[]; } p%u;\n",
			i, i, pt, i)) goto failed;
	}
	if (!accel_put(&ctx,
		"layout(set=0,binding=%u,std430) writeonly buffer PR { %s words[]; } pr;\n"
		"layout(push_constant) uniform PushConstants { uint element_count;\n",
		scratch_param, type)) goto failed;
	for (i = 0; i < func->val.func.param_count; i++) {
		if (map_kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR &&
		    !accel_put(&ctx, "    %s p%u;\n",
			map_kernel->param_type[i] == NOCT_VALUE_FLOAT ? "float" : "int", i))
			goto failed;
	}
	if (!accel_put(&ctx,
		"} pc;\nshared %s partial[64];\nvoid main(){\n"
		" uint lid=gl_LocalInvocationID.x; uint gid=gl_GlobalInvocationID.x;\n"
		" %s value=%s; if(gid<pc.element_count){ uint i=gid; value=",
		type, type, zero) ||
	    !accel_emit_expr(&ctx, (struct hir_expr *)dosum.mapped_expr,
			    func->val.func.param_packed_type[output_param]) ||
	    !accel_put(&ctx,
		";} partial[lid]=value; barrier();\n"
		" if(lid<32u) partial[lid]+=partial[lid+32u]; barrier();\n"
		" if(lid<16u) partial[lid]+=partial[lid+16u]; barrier();\n"
		" if(lid<8u) partial[lid]+=partial[lid+8u]; barrier();\n"
		" if(lid<4u) partial[lid]+=partial[lid+4u]; barrier();\n"
		" if(lid<2u) partial[lid]+=partial[lid+2u]; barrier();\n"
		" if(lid<1u) partial[lid]+=partial[lid+1u]; barrier();\n"
		" if(lid==0u) pr.words[gl_WorkGroupID.x]=partial[0];\n}\n"))
		goto failed;
	if (!gpu_ir_finalize_kernel(map_kernel, ctx.text, ctx.size,
				    error, sizeof(error)))
		goto failed;
	memset(ctx.text, 0, sizeof(ctx.text));
	ctx.size = 0;
	ctx.hlsl = true;
	if (!accel_put(&ctx, "// Noct HLSL SM 5.1\n"))
		goto failed;
	for (i = 0; i < scratch_param; i++) {
		const char *pt;

		if (map_kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
			continue;
		pt = accel_glsl_type(map_kernel->param_packed_type[i]);
		if (pt == NULL || !accel_put(&ctx,
			"RWStructuredBuffer<%s> p%u : register(u%u);\n",
			pt, i, i)) goto failed;
	}
	if (!accel_put(&ctx,
		"RWStructuredBuffer<%s> pr : register(u%u);\n"
		"cbuffer NoctPush : register(b0) { uint element_count; uint dispatch_stride;\n",
		type, scratch_param)) goto failed;
	for (i = 0; i < func->val.func.param_count; i++) {
		if (map_kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR &&
		    !accel_put(&ctx, "    %s pc_p%u;\n",
			map_kernel->param_type[i] == NOCT_VALUE_FLOAT ? "float" : "int", i))
			goto failed;
	}
	if (!accel_put(&ctx,
		"};\ngroupshared %s partial[64];\n[numthreads(64,1,1)]\n"
		"void main(uint3 dispatch_id:SV_DispatchThreadID,uint3 group_id:SV_GroupID,uint3 local_id:SV_GroupThreadID){\n"
		" uint lid=local_id.x; uint gid=dispatch_id.x;\n"
		" %s value=%s; for(uint i=gid;i<element_count;i+=dispatch_stride){ value+=",
		type, type, zero) ||
	    !accel_emit_expr(&ctx, (struct hir_expr *)dosum.mapped_expr,
			    func->val.func.param_packed_type[output_param]) ||
	    !accel_put(&ctx,
		";} partial[lid]=value; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid<32u) partial[lid]+=partial[lid+32u]; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid<16u) partial[lid]+=partial[lid+16u]; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid<8u) partial[lid]+=partial[lid+8u]; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid<4u) partial[lid]+=partial[lid+4u]; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid<2u) partial[lid]+=partial[lid+2u]; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid<1u) partial[lid]+=partial[lid+1u]; GroupMemoryBarrierWithGroupSync();\n"
		" if(lid==0u) pr[group_id.x]=partial[0];\n}\n") ||
	    !accel_store_hlsl(map_kernel, ctx.text, ctx.size))
		goto failed;
	memset(ctx.text, 0, sizeof(ctx.text));
	ctx.size = 0;
	for (i = 0; i < map_kernel->param_count; i++)
		if (map_kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR)
			map_kernel->param_transport[i] = ACCEL_TRANSPORT_DEVICE_PTR;

	snprintf(name, sizeof(name), "%s$dosum%u_fold", func->val.func.name,
		 (unsigned int)target_index);
	fold_kernel->name = noct_strdup(name);
	fold_kernel->source_name = noct_strdup(func->val.func.file_name);
	fold_kernel->descriptor_version = 3;
	fold_kernel->func_kind = NOCT_FUNC_GPU;
	fold_kernel->eligible = true;
	fold_kernel->parallel_mode = ACCEL_PARALLEL_DOALL;
	fold_kernel->source_line = loop->line;
	fold_kernel->dispatch_param = 2;
	fold_kernel->output_param = 1;
	fold_kernel->param_count = 3;
	for (i = 0; i < 2; i++) {
		fold_kernel->param_type[i] = NOCT_VALUE_PACKED;
		fold_kernel->param_packed_type[i] =
			func->val.func.param_packed_type[output_param];
		fold_kernel->param_transport[i] = ACCEL_TRANSPORT_DEVICE_PTR;
		fold_kernel->param_access[i] = ACCEL_TRANSPORT_DEVICE_PTR;
	}
	fold_kernel->param_effect[0] = ACCEL_EFFECT_READ;
	fold_kernel->param_effect[1] = ACCEL_EFFECT_WRITE;
	fold_kernel->param_type[2] = NOCT_VALUE_INT;
	fold_kernel->param_packed_type[2] = -1;
	fold_kernel->param_transport[2] = ACCEL_TRANSPORT_SCALAR;
	fold_kernel->param_access[2] = ACCEL_TRANSPORT_SCALAR;
	if (fold_kernel->name == NULL || fold_kernel->source_name == NULL)
		goto failed;
	memset(&ctx, 0, sizeof(ctx));
	if (!accel_put(&ctx,
		"#version 450\nlayout(local_size_x=64) in;\n"
		"layout(set=0,binding=0,std430) readonly buffer A{%s words[];} a;\n"
		"layout(set=0,binding=1,std430) writeonly buffer B{%s words[];} b;\n"
		"layout(push_constant) uniform PushConstants{uint element_count; int p2;}pc;\n"
		"shared %s partial[64]; void main(){uint lid=gl_LocalInvocationID.x;"
		"uint gid=gl_GlobalInvocationID.x;%s v=%s;if(gid<pc.element_count)v=a.words[gid];"
		"partial[lid]=v;barrier();"
		"if(lid<32u)partial[lid]+=partial[lid+32u];barrier();"
		"if(lid<16u)partial[lid]+=partial[lid+16u];barrier();"
		"if(lid<8u)partial[lid]+=partial[lid+8u];barrier();"
		"if(lid<4u)partial[lid]+=partial[lid+4u];barrier();"
		"if(lid<2u)partial[lid]+=partial[lid+2u];barrier();"
		"if(lid<1u)partial[lid]+=partial[lid+1u];barrier();"
		"if(lid==0u)b.words[gl_WorkGroupID.x]=partial[0];}\n",
		type, type, type, type, zero))
		goto failed;
	if (!gpu_ir_finalize_kernel(fold_kernel, ctx.text, ctx.size,
				    error, sizeof(error)))
		goto failed;
	memset(ctx.text, 0, sizeof(ctx.text));
	ctx.size = 0;
	ctx.hlsl = true;
	if (!accel_put(&ctx,
		"// Noct HLSL SM 5.1\n"
		"RWStructuredBuffer<%s> a : register(u0);\n"
		"RWStructuredBuffer<%s> b : register(u1);\n"
		"cbuffer NoctPush : register(b0){uint element_count;uint dispatch_stride;int pc_p2;};\n"
		"groupshared %s partial[64];\n[numthreads(64,1,1)]\n"
		"void main(uint3 dispatch_id:SV_DispatchThreadID,uint3 group_id:SV_GroupID,uint3 local_id:SV_GroupThreadID){"
		"uint lid=local_id.x;uint gid=dispatch_id.x;%s v=%s;if(gid<element_count)v=a[gid];"
		"partial[lid]=v;GroupMemoryBarrierWithGroupSync();"
		"if(lid<32u)partial[lid]+=partial[lid+32u];GroupMemoryBarrierWithGroupSync();"
		"if(lid<16u)partial[lid]+=partial[lid+16u];GroupMemoryBarrierWithGroupSync();"
		"if(lid<8u)partial[lid]+=partial[lid+8u];GroupMemoryBarrierWithGroupSync();"
		"if(lid<4u)partial[lid]+=partial[lid+4u];GroupMemoryBarrierWithGroupSync();"
		"if(lid<2u)partial[lid]+=partial[lid+2u];GroupMemoryBarrierWithGroupSync();"
		"if(lid<1u)partial[lid]+=partial[lid+1u];GroupMemoryBarrierWithGroupSync();"
		"if(lid==0u)b[group_id.x]=partial[0];}\n",
		 type, type, type, type, zero) ||
	    !accel_store_hlsl(fold_kernel, ctx.text, ctx.size))
		goto failed;
	memset(ctx.text, 0, sizeof(ctx.text));
	ctx.size = 0;

	for (i = 0; i < NOCT_ARG_MAX; i++) {
		expr_map[i] = -1;
		buffer_map[i] = -1;
	}
	if (reuse_program) {
		accel_program_free(program);
		program = func->val.func.accel_program;
		func->val.func.accel_program = NULL;
		if (program == NULL)
			goto failed;
		base_kernel_count = program->kernel_count;
		base_step_count = program->step_count;
		if (base_kernel_count > ACCEL_PROGRAM_MAX_KERNELS - 2 ||
		    base_step_count >= ACCEL_PROGRAM_MAX_STEPS ||
		    dosum_step_index > base_step_count)
			goto failed;
		new_kernel = noct_realloc(program->kernel,
			(sizeof(*program->kernel) * (base_kernel_count + 2)));
		if (new_kernel == NULL)
			goto failed;
		program->kernel = new_kernel;
		new_step = noct_realloc(program->step,
			(sizeof(*program->step) * (base_step_count + 1)));
		if (new_step == NULL)
			goto failed;
		program->step = new_step;
		if (dosum_step_index < base_step_count)
			memmove(&program->step[dosum_step_index + 1],
				&program->step[dosum_step_index],
				(sizeof(*program->step) *
				 (base_step_count - dosum_step_index)));
		memset(&program->step[dosum_step_index], 0,
		       sizeof(*program->step));
		expr_count = program->expr_count;
		buffer_count = program->buffer_count;
		for (i = 0; i < expr_count; i++) {
			if (program->expr[i].op == ACCEL_EXPR_SCALAR_ARG &&
			    program->expr[i].ref >= 0 &&
			    program->expr[i].ref < NOCT_ARG_MAX)
				expr_map[program->expr[i].ref] = (int)i;
		}
		local_count = 0;
		for (i = 0; i < buffer_count; i++) {
			if (program->buffer[i].outer_param >= 0)
				buffer_map[program->buffer[i].outer_param] = (int)i;
			else if (program->buffer[i].origin == ACCEL_BUFFER_LOCAL) {
				buffer_map[func->val.func.param_count + local_count] =
					(int)i;
				local_count++;
			}
		}
	} else if (prefix_count == 0) {
		base_kernel_count = 0;
		base_step_count = 0;
		program->descriptor_version = ACCEL_PROGRAM_VERSION;
		program->name = noct_strdup(func->val.func.name);
		program->source_name = noct_strdup(func->val.func.file_name);
		program->source_line = loop->line;
		program->outer_param_count = func->val.func.param_count;
		program->expr = noct_calloc(ACCEL_PROGRAM_MAX_EXPRS,
					     sizeof(*program->expr));
		program->buffer = noct_calloc(ACCEL_PROGRAM_MAX_BUFFERS,
					       sizeof(*program->buffer));
		program->kernel = noct_calloc(2, sizeof(*program->kernel));
		program->step = noct_calloc(1, sizeof(*program->step));
		if (program->name == NULL || program->source_name == NULL ||
		    program->expr == NULL || program->buffer == NULL ||
		    program->kernel == NULL || program->step == NULL)
			goto failed;
		expr_count = 0;
		buffer_count = 0;
		for (i = 0; i < func->val.func.param_count; i++) {
		if (func->val.func.param_accel_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			expr_map[i] = (int)expr_count;
			program->expr[expr_count].op = ACCEL_EXPR_SCALAR_ARG;
			program->expr[expr_count].ref = (int)i;
			expr_count++;
		} else {
			struct accel_buffer_desc *bd;

			bd = &program->buffer[buffer_count];
			buffer_map[i] = (int)buffer_count;
			bd->id = (int)buffer_count;
			bd->name = noct_strdup(func->val.func.param_name[i]);
			if (bd->name == NULL)
				goto failed;
			bd->outer_param = (int)i;
			bd->element_kind = func->val.func.param_packed_type[i];
			bd->element_width = 4;
			bd->length_expr = (int)expr_count;
			program->expr[expr_count].op = ACCEL_EXPR_BUFFER_LENGTH;
			program->expr[expr_count].ref = (int)buffer_count;
			expr_count++;
			bd->origin = i == output_param ?
				ACCEL_BUFFER_HOST_OUT : ACCEL_BUFFER_HOST_IN;
			bd->upload = i != output_param;
			bd->download = i == output_param;
			buffer_count++;
		}
		}
	} else {
		base_kernel_count = prefix_count;
		base_step_count = prefix_count;
		accel_program_free(program);
		program = NULL;
		if (!accel_build_program(func, prefix_kernel, loop_list,
					 prefix_count))
			goto failed;
		program = func->val.func.accel_program;
		func->val.func.accel_program = NULL;
		new_kernel = noct_realloc(program->kernel,
			(sizeof(*program->kernel) * (prefix_count + 2)));
		if (new_kernel == NULL)
			goto failed;
		program->kernel = new_kernel;
		new_step = noct_realloc(program->step,
			(sizeof(*program->step) * (prefix_count + 1)));
		if (new_step == NULL)
			goto failed;
		program->step = new_step;
		if (dosum_step_index < prefix_count)
			memmove(&program->step[dosum_step_index + 1],
				&program->step[dosum_step_index],
				(sizeof(*program->step) *
				 (prefix_count - dosum_step_index)));
		memset(&program->step[dosum_step_index], 0,
		       sizeof(*program->step));
		expr_count = program->expr_count;
		buffer_count = program->buffer_count;
		for (i = 0; i < expr_count; i++) {
			if (program->expr[i].op == ACCEL_EXPR_SCALAR_ARG &&
			    program->expr[i].ref >= 0 &&
			    program->expr[i].ref < NOCT_ARG_MAX)
				expr_map[program->expr[i].ref] = (int)i;
		}
		local_count = 0;
		for (i = 0; i < buffer_count; i++) {
			if (program->buffer[i].outer_param >= 0)
				buffer_map[program->buffer[i].outer_param] = (int)i;
			else if (program->buffer[i].origin == ACCEL_BUFFER_LOCAL) {
				buffer_map[func->val.func.param_count + local_count] =
					(int)i;
				local_count++;
			}
		}
	}
	trip_expr = expr_map[map_kernel->dispatch_param];
	if (trip_expr < 0)
		goto failed;
	if (expr_count >= ACCEL_PROGRAM_MAX_EXPRS ||
	    buffer_count > ACCEL_PROGRAM_MAX_BUFFERS - 2)
		goto failed;
	scratch_length_expr = (int)expr_count;
	program->expr[expr_count].op = ACCEL_EXPR_CEIL_DIV_CONST;
	program->expr[expr_count].left = trip_expr;
	program->expr[expr_count].value = 64;
	expr_count++;
	for (i = 0; i < 2; i++) {
		struct accel_buffer_desc *bd;

		bd = &program->buffer[buffer_count];
		bd->id = (int)buffer_count;
		snprintf(name, sizeof(name), "$dosum%u_scratch%u",
			 (unsigned int)target_index, (unsigned int)i);
		bd->name = noct_strdup(name);
		if (bd->name == NULL)
			goto failed;
		bd->origin = ACCEL_BUFFER_SCRATCH;
		bd->outer_param = -1;
		bd->element_kind =
			func->val.func.param_packed_type[output_param];
		bd->element_width = 4;
		bd->length_expr = scratch_length_expr;
		buffer_count++;
		program->buffer_count = buffer_count;
	}
	program->expr_count = expr_count;
	program->buffer_count = buffer_count;
	map_kernel_index = base_kernel_count;
	program->kernel[map_kernel_index] = map_kernel;
	program->kernel[map_kernel_index + 1] = fold_kernel;
	program->kernel_count = base_kernel_count + 2;
	map_kernel = NULL;
	fold_kernel = NULL;
	reduction_step = &program->step[dosum_step_index];
	reduction_step->kind = ACCEL_STEP_DOSUM_REDUCTION;
	reduction_step->source_line = loop->line;
	reduction_step->kernel = (int)map_kernel_index;
	reduction_step->fold_kernel = (int)(map_kernel_index + 1);
	reduction_step->trip_expr = trip_expr;
	reduction_step->block_size = 64;
	reduction_step->result_buffer = buffer_map[output_param];
	reduction_step->scratch_buffer = (int)(buffer_count - 2);
	reduction_step->scratch_buffer2 = (int)(buffer_count - 1);
	reduction_step->reduction_operator = ACCEL_REDUCTION_ADD;
	reduction_step->reduction_type =
		func->val.func.param_packed_type[output_param];
	reduction_step->binding_count =
		program->kernel[map_kernel_index]->param_count;
	for (i = 0; i < scratch_param; i++) {
		reduction_step->binding[i].kernel_param = (int)i;
		if (i < func->val.func.param_count &&
		    func->val.func.param_accel_transport[i] ==
			ACCEL_TRANSPORT_SCALAR) {
			reduction_step->binding[i].kind =
				ACCEL_BIND_SCALAR_EXPR;
			reduction_step->binding[i].value = expr_map[i];
		} else {
			reduction_step->binding[i].kind = ACCEL_BIND_BUFFER;
			reduction_step->binding[i].value = buffer_map[i];
		}
	}
	reduction_step->binding[i].kernel_param = (int)i;
	reduction_step->binding[i].kind = ACCEL_BIND_BUFFER;
	reduction_step->binding[i].value = reduction_step->scratch_buffer;
	program->step_count = base_step_count + 1;
	program->outer_param_effect[output_param] |= ACCEL_EFFECT_WRITE;
	program->buffer[buffer_map[output_param]].download =
		func->val.func.param_accel_transport[output_param] !=
		ACCEL_TRANSPORT_DEVICE_PTR;
	program->buffer[buffer_map[output_param]].upload = false;
	for (i = 0; i < func->val.func.param_count; i++) {
		if (func->val.func.param_accel_transport[i] !=
		    ACCEL_TRANSPORT_SCALAR &&
		    (program->kernel[map_kernel_index]->param_effect[i] &
		     ACCEL_EFFECT_READ) != 0) {
			program->outer_param_effect[i] |= ACCEL_EFFECT_READ;
			program->buffer[buffer_map[i]].upload =
				func->val.func.param_accel_transport[i] !=
				ACCEL_TRANSPORT_DEVICE_PTR;
			program->buffer[buffer_map[i]].initially_defined = true;
		}
	}
	if (!accel_program_validate(program, error, sizeof(error)))
		goto failed;
	if (!reuse_program) {
		shell = accel_kernel_clone(program->kernel[0]);
		if (shell == NULL)
			goto failed;
		shell->func_kind = NOCT_FUNC_ACCEL;
		shell->param_count = func->val.func.param_count;
		shell->output_param = (int)output_param;
		shell->dispatch_param = program->kernel[0]->dispatch_param;
		func->val.func.accel_kernel = shell;
	}
	func->val.func.accel_program = program;
	if (accel_info) {
		fprintf(stderr,
			"ACCEL: %s:%d: accelerator kernel generated (%s32, DOSUM)\n",
			func->val.func.file_name, loop->line,
			strcmp(type, "float") == 0 ? "float" : type);
		fprintf(stderr,
			"ACCEL: %s:%d: DOSUM additive reduction; block=64 multi-stage\n",
			func->val.func.file_name, loop->line);
	}
	for (k = 0; k < prefix_count; k++)
		accel_kernel_free(prefix_kernel[k]);
	hir_loop_summary_free(summary);
	return 1;
failed:
	if (getenv("NOCT_ACCEL_DEBUG") != NULL)
		fprintf(stderr,
			"ACCEL: DOSUM lowering failed (prefix=%u, error=%s)\n",
			(unsigned int)prefix_count,
			error[0] != '\0' ? error : "unspecified");
	for (k = 0; k < prefix_count; k++)
		accel_kernel_free(prefix_kernel[k]);
	accel_kernel_free(map_kernel);
	accel_kernel_free(fold_kernel);
	accel_program_free(program);
	hir_loop_summary_free(summary);
	return -1;
}

static int
accel_try_build_dosum(
	struct hir_block *func,
	bool accel_info)
{
	uint32_t target_index;
	int status;

	for (target_index = 0;
	     target_index < ACCEL_PROGRAM_MAX_STEPS;
	     target_index++) {
		status = accel_try_build_dosum_at(
			func, accel_info, target_index, target_index != 0);
		if (status < 0)
			return -1;
		if (status == 0)
			return target_index == 0 ? 0 : 1;
	}
	return -1;
}
