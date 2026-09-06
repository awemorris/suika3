/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Optimizer-independent validation and GLSL extraction for raw GPU funcs. */

#include <noct/noct.h>
#include "ast.h"
#include "hir.h"
#include "accel_ops.h"
#include "gpu_ir.h"

#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPU_LOCAL_MAX 256
#define GPU_SCOPE_MAX 16
#define GPU_LOOP_MAX 8
#define GPU_SOURCE_MAX (1024u * 1024u)

struct gpu_local {
	const char *source_name;
	int type;
	int scope_depth;
	bool is_var;
	bool active;
};

struct gpu_emit {
	struct hir_block *func;
	bool hlsl;
	char *text;
	size_t size, capacity;
	char *error;
	size_t error_size;
	uint32_t shared_count;
	const char *shared_name[NOCT_ARG_MAX];
	int shared_type[NOCT_ARG_MAX];
	uint32_t shared_length[NOCT_ARG_MAX];
	struct gpu_local local[GPU_LOCAL_MAX];
	uint32_t local_count;
	int scope_depth;
	int loop_depth;
	bool used_math[ACCEL_MATH_ID_LIMIT];
	bool has_barrier;
	bool has_nonfinal_return;
};

static void
gpu_error(struct gpu_emit *e, const char *s)
{
	if (e->error_size == 0) return;
	strncpy(e->error, s, e->error_size - 1);
	e->error[e->error_size - 1] = '\0';
}

static void
gpu_error_name(struct gpu_emit *e, const char *fmt, const char *name)
{
	char msg[256];
	snprintf(msg, sizeof(msg), fmt, name);
	gpu_error(e, msg);
}

static bool
gpu_put(struct gpu_emit *e, const char *fmt, ...)
{
	va_list ap, cp;
	int n;
	size_t cap;
	char *p;
	char limit_error[160];
	va_start(ap, fmt);
	va_copy(cp, ap);
	n = vsnprintf(NULL, 0, fmt, cp);
	va_end(cp);
	if (n < 0 || (size_t)n > SIZE_MAX - e->size - 1 ||
	    e->size + (size_t)n + 1 > GPU_SOURCE_MAX) {
		va_end(ap);
		snprintf(limit_error, sizeof(limit_error),
			 "Raw GPU kernel exceeds the source-size limit (%lu + %d bytes).",
			 (unsigned long)e->size, n);
		gpu_error(e, limit_error); return false;
	}
	if (e->size + (size_t)n + 1 > e->capacity) {
		cap = e->capacity == 0 ? 1024 : e->capacity;
		while (cap < e->size + (size_t)n + 1) {
			if (cap == GPU_SOURCE_MAX) {
				va_end(ap); gpu_error(e, "Raw GPU kernel exceeds the source-size limit."); return false;
			}
			if (cap > GPU_SOURCE_MAX / 2) cap = GPU_SOURCE_MAX;
			else cap *= 2;
		}
		p = noct_realloc(e->text, cap);
		if (p == NULL) {
			va_end(ap); gpu_error(e, "Out of memory building raw GPU kernel."); return false;
		}
		e->text = p; e->capacity = cap;
	}
	vsnprintf(e->text + e->size, e->capacity - e->size, fmt, ap);
	va_end(ap); e->size += (size_t)n; return true;
}

static bool
gpu_reserved_name(const char *name)
{
	return strcmp(name, "Accel") == 0 || strcmp(name, "syncthreads") == 0 ||
	       strcmp(name, "threadIdx") == 0 || strcmp(name, "blockIdx") == 0 ||
	       strcmp(name, "blockDim") == 0 || strcmp(name, "gridDim") == 0 ||
	       strcmp(name, "globalIdx") == 0;
}

static int
gpu_param(struct gpu_emit *e, const char *name)
{
	uint32_t i;
	for (i = 0; i < e->func->val.func.param_count; i++)
		if (strcmp(e->func->val.func.param_name[i], name) == 0) return (int)i;
	return -1;
}

static int
gpu_shared(struct gpu_emit *e, const char *name)
{
	uint32_t i;
	for (i = 0; i < e->shared_count; i++)
		if (strcmp(e->shared_name[i], name) == 0) return (int)i;
	return -1;
}

static int
gpu_local(struct gpu_emit *e, const char *name)
{
	int i;
	for (i = (int)e->local_count - 1; i >= 0; i--)
		if (e->local[i].active &&
		    strcmp(e->local[i].source_name, name) == 0) return i;
	return -1;
}

static bool
gpu_scope_push(struct gpu_emit *e)
{
	if (e->scope_depth + 1 >= GPU_SCOPE_MAX) {
		gpu_error(e, "Raw GPU lexical-scope nesting limit exceeded."); return false;
	}
	e->scope_depth++;
	return true;
}

static void
gpu_scope_pop(struct gpu_emit *e)
{
	uint32_t i;
	for (i = 0; i < e->local_count; i++)
		if (e->local[i].active &&
		    e->local[i].scope_depth == e->scope_depth)
			e->local[i].active = false;
	e->scope_depth--;
}

static int
gpu_add_local(struct gpu_emit *e, const char *name, int type, bool is_var)
{
	uint32_t i;
	if (gpu_reserved_name(name)) {
		gpu_error_name(e, "'%s' is a reserved name inside __gpu func.", name);
		return -1;
	}
	for (i = 0; i < e->local_count; i++) {
		if (e->local[i].active &&
		    e->local[i].scope_depth == e->scope_depth &&
		    strcmp(e->local[i].source_name, name) == 0) {
			gpu_error_name(e, "Duplicate raw GPU local '%s'.", name);
			return -1;
		}
	}
	if (e->local_count >= GPU_LOCAL_MAX) {
		gpu_error(e, "Too many raw GPU local variables."); return -1;
	}
	i = e->local_count++;
	e->local[i].source_name = name;
	e->local[i].type = type;
	e->local[i].scope_depth = e->scope_depth;
	e->local[i].is_var = is_var;
	e->local[i].active = true;
	return (int)i;
}

static const char *
gpu_packed_type(int type)
{
	switch (type) {
	case NOCT_PACKED_INT32: return "int";
	case NOCT_PACKED_UINT32: return "uint";
	case NOCT_PACKED_FLOAT32: return "float";
	default: return NULL;
	}
}

static const char *
gpu_op(int type)
{
	switch (type) {
	case AST_EXPR_LT: return "<"; case AST_EXPR_LTE: return "<=";
	case AST_EXPR_GT: return ">"; case AST_EXPR_GTE: return ">=";
	case AST_EXPR_EQ: return "=="; case AST_EXPR_NEQ: return "!=";
	case AST_EXPR_PLUS: return "+"; case AST_EXPR_MINUS: return "-";
	case AST_EXPR_MUL: return "*"; case AST_EXPR_DIV: return "/";
	case AST_EXPR_MOD: return "%"; case AST_EXPR_AND: return "&";
	case AST_EXPR_OR: return "|"; case AST_EXPR_LAND: return "&&";
	case AST_EXPR_LOR: return "||"; case AST_EXPR_XOR: return "^";
	case AST_EXPR_SHL: return "<<"; case AST_EXPR_SHR: return ">>";
	default: return NULL;
	}
}

static bool
gpu_is_compare(int type)
{
	return type == AST_EXPR_LT || type == AST_EXPR_LTE ||
	       type == AST_EXPR_GT || type == AST_EXPR_GTE ||
	       type == AST_EXPR_EQ || type == AST_EXPR_NEQ;
}

static bool
gpu_is_logic(int type)
{
	return type == AST_EXPR_LAND || type == AST_EXPR_LOR;
}

static bool
gpu_is_bitwise(int type)
{
	return type == AST_EXPR_MOD || type == AST_EXPR_AND ||
	       type == AST_EXPR_OR || type == AST_EXPR_XOR ||
	       type == AST_EXPR_SHL || type == AST_EXPR_SHR;
}

static bool
gpu_is_int_literal(struct ast_expr *x)
{
	if (x == NULL) return false;
	if (x->type == AST_EXPR_TERM && x->val.term.term != NULL &&
	    x->val.term.term->type == AST_TERM_INT) return true;
	return x->type == AST_EXPR_NEG && x->val.unary.expr != NULL &&
	       x->val.unary.expr->type == AST_EXPR_TERM &&
	       x->val.unary.expr->val.term.term != NULL &&
	       x->val.unary.expr->val.term.term->type == AST_TERM_INT;
}

static bool
gpu_intrinsic_name(struct gpu_emit *e, struct ast_expr *x, const char **name)
{
	struct ast_expr *o;
	const char *n;
	o = x->val.dot.obj;
	if (o == NULL || o->type != AST_EXPR_TERM || o->val.term.term == NULL ||
	    o->val.term.term->type != AST_TERM_SYMBOL ||
	    strcmp(x->val.dot.symbol, "x") != 0) {
		gpu_error(e, "Raw GPU intrinsics support only .x."); return false;
	}
	n = o->val.term.term->val.symbol;
	if (strcmp(n, "threadIdx") == 0)
		*name = e->hlsl ? "noct_group_thread_id.x" :
			"gl_LocalInvocationID.x";
	else if (strcmp(n, "blockIdx") == 0)
		*name = e->hlsl ? "noct_group_id.x" : "gl_WorkGroupID.x";
	else if (strcmp(n, "blockDim") == 0)
		*name = e->hlsl ? "pc_block_size" : "gl_WorkGroupSize.x";
	else if (strcmp(n, "gridDim") == 0)
		*name = e->hlsl ? "pc_grid_size" : "gl_NumWorkGroups.x";
	else if (strcmp(n, "globalIdx") == 0)
		*name = e->hlsl ? "noct_dispatch_thread_id.x" :
			"gl_GlobalInvocationID.x";
	else { gpu_error(e, "Unsupported property in raw GPU function."); return false; }
	return true;
}

static const struct accel_op_desc *
gpu_math_call(struct gpu_emit *e, struct ast_expr *x, bool diagnose_unknown)
{
	struct ast_expr *fn;
	struct ast_expr *obj;
	struct ast_term *term;
	const struct accel_op_desc *op;
	if (x == NULL || x->type != AST_EXPR_CALL) return NULL;
	fn = x->val.call.func;
	if (fn == NULL || fn->type != AST_EXPR_DOT) return NULL;
	obj = fn->val.dot.obj;
	if (obj == NULL || obj->type != AST_EXPR_TERM) return NULL;
	term = obj->val.term.term;
	if (term == NULL || term->type != AST_TERM_SYMBOL ||
	    strcmp(term->val.symbol, "Accel") != 0) return NULL;
	op = accel_math_lookup_member(fn->val.dot.symbol);
	if (op == NULL && diagnose_unknown)
		gpu_error_name(e, "Unknown GPU math operation 'Accel.%s'.",
			       fn->val.dot.symbol);
	return op;
}

static bool
gpu_float32_bits_call(struct ast_expr *x)
{
	struct ast_expr *fn;
	struct ast_expr *obj;
	struct ast_term *term;
	if (x == NULL || x->type != AST_EXPR_CALL) return false;
	fn = x->val.call.func;
	if (fn == NULL || fn->type != AST_EXPR_DOT ||
	    strcmp(fn->val.dot.symbol, "float32FromBits") != 0) return false;
	obj = fn->val.dot.obj;
	if (obj == NULL || obj->type != AST_EXPR_TERM) return false;
	term = obj->val.term.term;
	return term != NULL && term->type == AST_TERM_SYMBOL &&
	       strcmp(term->val.symbol, "Accel") == 0;
}

static bool
gpu_float32_bits_literal(struct gpu_emit *e, struct ast_expr *x, uint32_t *bits)
{
	struct ast_term *term;
	int64_t value;
	bool negative;
	negative = false;
	if (x != NULL && x->type == AST_EXPR_NEG) {
		negative = true;
		x = x->val.unary.expr;
	}
	if (x == NULL || x->type != AST_EXPR_TERM || x->val.term.term == NULL) {
		gpu_error(e, "Accel.float32FromBits() requires a 32-bit integer literal.");
		return false;
	}
	term = x->val.term.term;
	if (term->type == AST_TERM_INT) value = term->val.i;
	else if (term->type == AST_TERM_LONG) value = term->val.l;
	else {
		gpu_error(e, "Accel.float32FromBits() requires a 32-bit integer literal.");
		return false;
	}
	if (negative) {
		if (value < 0 || value > 2147483648LL) {
			gpu_error(e, "Accel.float32FromBits() literal is out of range.");
			return false;
		}
		value = -value;
		*bits = (uint32_t)value;
		return true;
	}
	if (value < 0 || value > 4294967295LL) {
		gpu_error(e, "Accel.float32FromBits() literal is out of range.");
		return false;
	}
	*bits = (uint32_t)value;
	return true;
}

static int
gpu_arg_count(struct ast_arg_list *list)
{
	struct ast_expr *arg;
	int count;
	count = 0;
	for (arg = list != NULL ? list->list : NULL; arg != NULL; arg = arg->next)
		count++;
	return count;
}

static bool gpu_expr_type(struct gpu_emit *, struct ast_expr *, int, int *);

static bool
gpu_symbol_type(struct gpu_emit *e, const char *name, int expected, int *type)
{
	int p;
	int local;
	int actual;
	p = gpu_param(e, name);
	if (p >= 0) {
		if (e->func->val.func.param_accel_transport[p] !=
		    ACCEL_TRANSPORT_SCALAR) {
			gpu_error(e, "A raw GPU pointer must be subscripted."); return false;
		}
		actual = e->func->val.func.param_type[p] == NOCT_VALUE_FLOAT ?
			 NOCT_PACKED_FLOAT32 : NOCT_PACKED_INT32;
	} else {
		local = gpu_local(e, name);
		if (local < 0) {
			gpu_error_name(e, "Unknown or out-of-scope raw GPU symbol '%s'.", name);
			return false;
		}
		actual = e->local[local].type;
	}
	if (expected != 0 && actual != expected) {
		gpu_error(e, "Raw GPU expression type does not match its context."); return false;
	}
	*type = actual;
	return true;
}

static bool
gpu_subscript_type(struct gpu_emit *e, struct ast_expr *x, int expected,
		   int *type)
{
	struct ast_expr *base;
	int p;
	int shared;
	int index_type;
	int actual;
	base = x->val.binary.expr[0];
	if (base == NULL || base->type != AST_EXPR_TERM ||
	    base->val.term.term == NULL ||
	    base->val.term.term->type != AST_TERM_SYMBOL) {
		gpu_error(e, "Raw GPU subscript base must be a _ptr parameter or shared array."); return false;
	}
	p = gpu_param(e, base->val.term.term->val.symbol);
	shared = gpu_shared(e, base->val.term.term->val.symbol);
	if (p < 0 && shared >= 0) actual = e->shared_type[shared];
	else if (p >= 0 && e->func->val.func.param_accel_transport[p] ==
		 ACCEL_TRANSPORT_DEVICE_PTR)
		actual = e->func->val.func.param_packed_type[p];
	else {
		gpu_error(e, "Raw GPU subscript base must be a _ptr parameter or shared array."); return false;
	}
	if (!gpu_expr_type(e, x->val.binary.expr[1], NOCT_PACKED_INT32,
			   &index_type)) return false;
	if (expected != 0 && actual != expected) {
		gpu_error(e, "Raw GPU expression type does not match its context."); return false;
	}
	*type = actual;
	return true;
}

static bool
gpu_binary_types(struct gpu_emit *e, struct ast_expr *x, int expected,
		 int *operand_type, int *result_type)
{
	int left_type;
	int right_type;
	int selected;
	if (gpu_is_logic(x->type)) {
		if (!gpu_expr_type(e, x->val.binary.expr[0], NOCT_PACKED_INT32,
				   &left_type) ||
		    !gpu_expr_type(e, x->val.binary.expr[1], NOCT_PACKED_INT32,
				   &right_type)) return false;
		*operand_type = NOCT_PACKED_INT32;
		*result_type = NOCT_PACKED_INT32;
		return true;
	}
	if (!gpu_is_compare(x->type) && expected != 0) {
		if (gpu_is_bitwise(x->type) && expected == NOCT_PACKED_FLOAT32) {
			gpu_error(e, "Raw GPU bitwise operands must be integers."); return false;
		}
		if (!gpu_expr_type(e, x->val.binary.expr[0], expected,
				   &left_type) ||
		    !gpu_expr_type(e, x->val.binary.expr[1], expected,
				   &right_type)) return false;
		*operand_type = expected;
		*result_type = expected;
		return true;
	}
	if (!gpu_expr_type(e, x->val.binary.expr[0], 0, &left_type) ||
	    !gpu_expr_type(e, x->val.binary.expr[1], 0, &right_type)) return false;
	if (left_type == NOCT_PACKED_FLOAT32 || right_type == NOCT_PACKED_FLOAT32)
		selected = NOCT_PACKED_FLOAT32;
	else if (left_type == NOCT_PACKED_UINT32 ||
		 right_type == NOCT_PACKED_UINT32)
		selected = NOCT_PACKED_UINT32;
	else
		selected = NOCT_PACKED_INT32;
	if (left_type != right_type &&
	    !((left_type == NOCT_PACKED_INT32 &&
	       gpu_is_int_literal(x->val.binary.expr[0])) ||
	      (right_type == NOCT_PACKED_INT32 &&
	       gpu_is_int_literal(x->val.binary.expr[1])))) {
		gpu_error(e, "Raw GPU binary operands have incompatible types."); return false;
	}
	if (gpu_is_bitwise(x->type) && selected == NOCT_PACKED_FLOAT32) {
		gpu_error(e, "Raw GPU bitwise operands must be integers."); return false;
	}
	*operand_type = selected;
	*result_type = gpu_is_compare(x->type) ? NOCT_PACKED_INT32 : selected;
	if (expected != 0 && *result_type != expected) {
		gpu_error(e, "Raw GPU expression type does not match its context."); return false;
	}
	return true;
}

static bool
gpu_expr_type(struct gpu_emit *e, struct ast_expr *x, int expected, int *type)
{
	struct ast_term *t;
	struct ast_expr *arg;
	const struct accel_op_desc *op;
	const char *glsl;
	int actual;
	int operand_type;
	int count;
	uint32_t float_bits;
	if (x == NULL) { gpu_error(e, "Missing raw GPU expression."); return false; }
	if (x->type == AST_EXPR_TERM) {
		t = x->val.term.term;
		if (t == NULL) { gpu_error(e, "Missing raw GPU term."); return false; }
		if (t->type == AST_TERM_INT) {
			actual = expected != 0 ? expected : NOCT_PACKED_INT32;
			*type = actual; return true;
		}
		if (t->type == AST_TERM_FLOAT) {
			if (expected != 0 && expected != NOCT_PACKED_FLOAT32) {
				gpu_error(e, "Raw GPU expression type does not match its context."); return false;
			}
			*type = NOCT_PACKED_FLOAT32; return true;
		}
		if (t->type == AST_TERM_SYMBOL)
			return gpu_symbol_type(e, t->val.symbol, expected, type);
		gpu_error(e, "Unsupported raw GPU literal."); return false;
	}
	if (x->type == AST_EXPR_DOT) {
		if (!gpu_intrinsic_name(e, x, &glsl)) return false;
		if (expected != 0 && expected != NOCT_PACKED_INT32) {
			gpu_error(e, "Raw GPU expression type does not match its context."); return false;
		}
		*type = NOCT_PACKED_INT32; return true;
	}
	if (x->type == AST_EXPR_PAR)
		return gpu_expr_type(e, x->val.par.expr, expected, type);
	if (x->type == AST_EXPR_NEG) {
		if (!gpu_expr_type(e, x->val.unary.expr, expected, &actual)) return false;
		if (actual == NOCT_PACKED_UINT32) {
			gpu_error(e, "Raw GPU unary minus does not accept uint32."); return false;
		}
		*type = actual; return true;
	}
	if (x->type == AST_EXPR_NOT) {
		if (!gpu_expr_type(e, x->val.unary.expr, NOCT_PACKED_INT32,
				   &actual)) return false;
		if (expected != 0 && expected != NOCT_PACKED_INT32) {
			gpu_error(e, "Raw GPU expression type does not match its context."); return false;
		}
		*type = NOCT_PACKED_INT32; return true;
	}
	if (x->type == AST_EXPR_SUBSCR)
		return gpu_subscript_type(e, x, expected, type);
	if (x->type == AST_EXPR_CALL) {
		if (gpu_float32_bits_call(x)) {
			count = gpu_arg_count(x->val.call.arg_list);
			if (count != 1) {
				gpu_error(e, "Accel.float32FromBits() expects 1 int32 argument.");
				return false;
			}
			arg = x->val.call.arg_list != NULL ?
			      x->val.call.arg_list->list : NULL;
			if (!gpu_float32_bits_literal(e, arg, &float_bits)) return false;
			if (expected != 0 && expected != NOCT_PACKED_FLOAT32) {
				gpu_error(e, "Raw GPU expression type does not match its context.");
				return false;
			}
			*type = NOCT_PACKED_FLOAT32;
			return true;
		}
		op = gpu_math_call(e, x, true);
		if (op == NULL) {
			if (e->error_size == 0 || e->error[0] == '\0')
				gpu_error(e, "Ordinary calls are unsupported inside __gpu func.");
			return false;
		}
		count = gpu_arg_count(x->val.call.arg_list);
		if (count != op->arity) {
			char msg[256];
			snprintf(msg, sizeof(msg), "%s() expects %d float32 argument(s).",
				 op->source_spelling, op->arity);
			gpu_error(e, msg); return false;
		}
		if ((op->capabilities & ACCEL_OP_CAP_RAW_GLSL) == 0 ||
		    accel_op_glsl_function(op) == NULL) {
			gpu_error_name(e,
				"GPU math operation '%s' is registered but not supported in this stage.",
				op->source_spelling);
			return false;
		}
		for (arg = x->val.call.arg_list != NULL ?
		     x->val.call.arg_list->list : NULL; arg != NULL; arg = arg->next)
			if (!gpu_expr_type(e, arg, NOCT_PACKED_FLOAT32, &actual))
				return false;
		if (expected != 0 && expected != NOCT_PACKED_FLOAT32) {
			gpu_error(e, "Raw GPU expression type does not match its context."); return false;
		}
		*type = NOCT_PACKED_FLOAT32; return true;
	}
	if (gpu_op(x->type) != NULL) {
		if (!gpu_binary_types(e, x, expected, &operand_type, &actual))
			return false;
		*type = actual; return true;
	}
	gpu_error(e, "Unsupported expression in raw GPU function."); return false;
}

static bool gpu_expr(struct gpu_emit *, struct ast_expr *, int);

static bool
gpu_put_float(struct gpu_emit *e, float value)
{
	char text[64];
	char *p;
	if (value != value || value > FLT_MAX || value < -FLT_MAX) {
		gpu_error(e, "Non-finite raw GPU source literal is unsupported."); return false;
	}
	snprintf(text, sizeof(text), "%.9g", (double)value);
	for (p = text; *p != '\0'; p++) if (*p == ',') *p = '.';
	if (strchr(text, '.') == NULL && strchr(text, 'e') == NULL &&
	    strchr(text, 'E') == NULL)
		return gpu_put(e, "%s.0", text);
	return gpu_put(e, "%s", text);
}

static bool
gpu_symbol_expr(struct gpu_emit *e, const char *name)
{
	int p;
	int local;
	p = gpu_param(e, name);
	if (p >= 0) return gpu_put(e, e->hlsl ? "pc_p%d" : "pc.p%d", p);
	local = gpu_local(e, name);
	if (local < 0) {
		gpu_error_name(e, "Unknown or out-of-scope raw GPU symbol '%s'.", name);
		return false;
	}
	return gpu_put(e, "v%d", local);
}

static bool
gpu_expr(struct gpu_emit *e, struct ast_expr *x, int expected)
{
	struct ast_term *t;
	struct ast_expr *base;
	struct ast_expr *arg;
	const struct accel_op_desc *math_op;
	const char *op;
	const char *glsl;
	const char *fn;
	int p;
	int shared;
	int actual;
	int operand_type;
	int result_type;
	uint32_t float_bits;
	actual = expected;
	if (x->type == AST_EXPR_TERM) {
		t = x->val.term.term;
		if (t->type == AST_TERM_INT) {
			if (actual == NOCT_PACKED_UINT32) return gpu_put(e, "uint(%d)", t->val.i);
			if (actual == NOCT_PACKED_FLOAT32) return gpu_put(e, "float(%d)", t->val.i);
			return gpu_put(e, "%d", t->val.i);
		}
		if (t->type == AST_TERM_FLOAT) return gpu_put_float(e, t->val.f);
		return gpu_symbol_expr(e, t->val.symbol);
	}
	if (x->type == AST_EXPR_DOT) {
		if (!gpu_intrinsic_name(e, x, &glsl)) return false;
		return gpu_put(e, "int(%s)", glsl);
	}
	if (x->type == AST_EXPR_PAR)
		return gpu_put(e, "(") && gpu_expr(e, x->val.par.expr, expected) && gpu_put(e, ")");
	if (x->type == AST_EXPR_NEG || x->type == AST_EXPR_NOT)
		return gpu_put(e, x->type == AST_EXPR_NEG ? "(-" : "(!") &&
			gpu_expr(e, x->val.unary.expr, expected) && gpu_put(e, ")");
	if (x->type == AST_EXPR_SUBSCR) {
		base = x->val.binary.expr[0];
		p = gpu_param(e, base->val.term.term->val.symbol);
		shared = gpu_shared(e, base->val.term.term->val.symbol);
		if (p < 0 && shared >= 0)
			return gpu_put(e, "s%d[", shared) &&
				gpu_expr(e, x->val.binary.expr[1], NOCT_PACKED_INT32) && gpu_put(e, "]");
		e->func->val.func.param_accel_effect[p] |= ACCEL_EFFECT_READ;
		return gpu_put(e, e->hlsl ? "p%d[" : "p%d.words[", p) &&
			gpu_expr(e, x->val.binary.expr[1], NOCT_PACKED_INT32) && gpu_put(e, "]");
	}
	if (x->type == AST_EXPR_CALL) {
		if (gpu_float32_bits_call(x)) {
			arg = x->val.call.arg_list != NULL ?
			      x->val.call.arg_list->list : NULL;
			if (!gpu_float32_bits_literal(e, arg, &float_bits)) return false;
			return gpu_put(e, e->hlsl ? "asfloat(0x%08xu)" :
			       "uintBitsToFloat(0x%08xu)",
			       (unsigned int)float_bits);
		}
		math_op = gpu_math_call(e, x, false);
		fn = accel_op_glsl_function(math_op);
		if (!gpu_put(e, "%s(", fn)) return false;
		for (arg = x->val.call.arg_list != NULL ?
		     x->val.call.arg_list->list : NULL; arg != NULL; arg = arg->next) {
			if (arg != x->val.call.arg_list->list && !gpu_put(e, ", ")) return false;
			if (!gpu_expr(e, arg, NOCT_PACKED_FLOAT32)) return false;
		}
		if (math_op->function_id > ACCEL_MATH_NONE &&
		    math_op->function_id < ACCEL_MATH_ID_LIMIT)
			e->used_math[math_op->function_id] = true;
		return gpu_put(e, ")");
	}
	op = gpu_op(x->type);
	if (op != NULL) {
		if (gpu_is_compare(x->type) || gpu_is_logic(x->type) ||
		    expected == 0) {
			if (!gpu_binary_types(e, x, expected, &operand_type,
					      &result_type)) return false;
		} else {
			operand_type = expected;
			result_type = expected;
		}
		return gpu_put(e, "(") &&
			gpu_expr(e, x->val.binary.expr[0], operand_type) &&
			gpu_put(e, " %s ", op) &&
			gpu_expr(e, x->val.binary.expr[1], operand_type) &&
			gpu_put(e, ")");
	}
	gpu_error(e, "Unsupported expression in raw GPU function."); return false;
}

static bool
gpu_store_lhs(struct gpu_emit *e, struct ast_expr *x, int *type)
{
	struct ast_expr *base;
	const char *name;
	int p;
	int shared;
	int local;
	int index_type;
	if (x == NULL) {
		gpu_error(e, "Raw GPU assignment target is missing."); return false;
	}
	if (x->type == AST_EXPR_TERM && x->val.term.term != NULL &&
	    x->val.term.term->type == AST_TERM_SYMBOL) {
		name = x->val.term.term->val.symbol;
		local = gpu_local(e, name);
		if (local < 0) {
			gpu_error_name(e, "Assignment to undeclared or out-of-scope raw GPU local '%s'.", name);
			return false;
		}
		if (!e->local[local].is_var) {
			gpu_error_name(e, "Raw GPU local '%s' is immutable.", name);
			return false;
		}
		*type = e->local[local].type;
		return gpu_put(e, "v%d", local);
	}
	if (x->type != AST_EXPR_SUBSCR) {
		gpu_error(e, "Raw GPU assignment target is unsupported."); return false;
	}
	base = x->val.binary.expr[0];
	if (base == NULL || base->type != AST_EXPR_TERM ||
	    base->val.term.term == NULL ||
	    base->val.term.term->type != AST_TERM_SYMBOL) {
		gpu_error(e, "Raw GPU store base must be a _ptr parameter or shared array."); return false;
	}
	name = base->val.term.term->val.symbol;
	p = gpu_param(e, name);
	shared = gpu_shared(e, name);
	if (p < 0 && shared >= 0) {
		*type = e->shared_type[shared];
		if (!gpu_expr_type(e, x->val.binary.expr[1],
				   NOCT_PACKED_INT32, &index_type)) return false;
		return gpu_put(e, "s%d[", shared) &&
			gpu_expr(e, x->val.binary.expr[1], NOCT_PACKED_INT32) && gpu_put(e, "]");
	}
	if (p < 0 || e->func->val.func.param_accel_transport[p] !=
	    ACCEL_TRANSPORT_DEVICE_PTR) {
		gpu_error(e, "Raw GPU store base must be a _ptr parameter or shared array."); return false;
	}
	*type = e->func->val.func.param_packed_type[p];
	if (!gpu_expr_type(e, x->val.binary.expr[1], NOCT_PACKED_INT32,
			   &index_type)) return false;
	e->func->val.func.param_accel_effect[p] |= ACCEL_EFFECT_WRITE;
	return gpu_put(e, e->hlsl ? "p%d[" : "p%d.words[", p) &&
		gpu_expr(e, x->val.binary.expr[1], NOCT_PACKED_INT32) && gpu_put(e, "]");
}

static bool
gpu_const_int(struct gpu_emit *e, struct ast_expr *x, int *value)
{
	struct ast_term *term;
	if (x != NULL && x->type == AST_EXPR_TERM &&
	    x->val.term.term != NULL) {
		term = x->val.term.term;
		if (term->type == AST_TERM_INT) {
			*value = term->val.i;
			return true;
		}
		if (term->type == AST_TERM_LONG) {
			gpu_error(e, "Raw GPU loop bound exceeds the int range."); return false;
		}
	}
	if (x != NULL && x->type == AST_EXPR_NEG &&
	    x->val.unary.expr != NULL &&
	    x->val.unary.expr->type == AST_EXPR_TERM &&
	    x->val.unary.expr->val.term.term != NULL &&
	    x->val.unary.expr->val.term.term->type == AST_TERM_INT) {
		*value = -x->val.unary.expr->val.term.term->val.i;
		return true;
	}
	gpu_error(e, "Raw GPU ranged-for bounds must be compile-time int constants.");
	return false;
}

static bool gpu_stmts(struct gpu_emit *, struct ast_stmt_list *, int);

static bool
gpu_branch(struct gpu_emit *e, struct ast_stmt_list *list, int indent)
{
	bool ok;
	if (!gpu_scope_push(e)) return false;
	ok = gpu_stmts(e, list, indent);
	gpu_scope_pop(e);
	return ok;
}

static bool
gpu_stmts(struct gpu_emit *e, struct ast_stmt_list *list, int indent)
{
	struct ast_stmt *s;
	struct ast_stmt *previous;
	struct ast_term *name;
	const char *tn;
	int type;
	int local;
	int start;
	int stop;
	int cond_type;
	if (list == NULL) return true;
	previous = NULL;
	for (s = list->list; s != NULL; s = s->next) {
		switch (s->type) {
		case AST_STMT_EMPTY:
			break;
		case AST_STMT_ASSIGN:
			if (s->val.assign.is_shared) {
				if (indent != 4) {
					gpu_error(e, "__shared declarations must be at function scope."); return false;
				}
				break;
			}
			if ((s->val.assign.is_var || s->val.assign.is_let) &&
			    s->val.assign.lhs != NULL &&
			    s->val.assign.lhs->type == AST_EXPR_TERM) {
				name = s->val.assign.lhs->val.term.term;
				tn = s->val.assign.type_name;
				if (name == NULL || name->type != AST_TERM_SYMBOL || tn == NULL ||
				    (strcmp(tn, "int") != 0 && strcmp(tn, "float") != 0)) {
					gpu_error(e, "Raw GPU locals require int or float type annotations."); return false;
				}
				type = strcmp(tn, "float") == 0 ?
					NOCT_PACKED_FLOAT32 : NOCT_PACKED_INT32;
				if (!gpu_expr_type(e, s->val.assign.rhs, type, &cond_type)) return false;
				local = gpu_add_local(e, name->val.symbol, type,
						      s->val.assign.is_var);
				if (local < 0) return false;
				if (!gpu_put(e, "%*s%s v%d = ", indent, "", tn, local) ||
				    !gpu_expr(e, s->val.assign.rhs, type) ||
				    !gpu_put(e, ";\n")) return false;
			} else {
				if (!gpu_put(e, "%*s", indent, "") ||
				    !gpu_store_lhs(e, s->val.assign.lhs, &type) ||
				    !gpu_expr_type(e, s->val.assign.rhs, type,
						   &cond_type) ||
				    !gpu_put(e, " = ") ||
				    !gpu_expr(e, s->val.assign.rhs, type) ||
				    !gpu_put(e, ";\n")) return false;
			}
			break;
		case AST_STMT_IF:
			if (!gpu_expr_type(e, s->val.if_.cond, NOCT_PACKED_INT32,
					   &cond_type) ||
			    !gpu_put(e, "%*sif (", indent, "") ||
			    !gpu_expr(e, s->val.if_.cond, NOCT_PACKED_INT32) ||
			    !gpu_put(e, ") {\n") ||
			    !gpu_branch(e, s->val.if_.stmt_list, indent + 4) ||
			    !gpu_put(e, "%*s}\n", indent, "")) return false;
			break;
		case AST_STMT_ELIF:
			if (previous == NULL ||
			    (previous->type != AST_STMT_IF &&
			     previous->type != AST_STMT_ELIF)) {
				gpu_error(e, "else-if block appeared without a raw GPU if block."); return false;
			}
			if (!gpu_expr_type(e, s->val.elif.cond, NOCT_PACKED_INT32,
					   &cond_type) ||
			    !gpu_put(e, "%*selse if (", indent, "") ||
			    !gpu_expr(e, s->val.elif.cond, NOCT_PACKED_INT32) ||
			    !gpu_put(e, ") {\n") ||
			    !gpu_branch(e, s->val.elif.stmt_list, indent + 4) ||
			    !gpu_put(e, "%*s}\n", indent, "")) return false;
			break;
		case AST_STMT_ELSE:
			if (previous == NULL ||
			    (previous->type != AST_STMT_IF &&
			     previous->type != AST_STMT_ELIF)) {
				gpu_error(e, "else block appeared without a raw GPU if block."); return false;
			}
			if (!gpu_put(e, "%*selse {\n", indent, "") ||
			    !gpu_branch(e, s->val.else_.stmt_list, indent + 4) ||
			    !gpu_put(e, "%*s}\n", indent, "")) return false;
			break;
		case AST_STMT_FOR:
			if (!s->val.for_.is_range) {
				gpu_error(e, "Collection iteration is unsupported inside __gpu func."); return false;
			}
			if (!gpu_const_int(e, s->val.for_.start, &start) ||
			    !gpu_const_int(e, s->val.for_.stop, &stop)) return false;
			if (start < 0 || stop < 0) {
				gpu_error(e, "Raw GPU ranged-for bounds must be non-negative."); return false;
			}
			if (start > stop) {
				gpu_error(e, "Raw GPU ranged-for start must not exceed stop."); return false;
			}
			if (e->loop_depth >= GPU_LOOP_MAX) {
				gpu_error(e, "Raw GPU loop-nesting limit exceeded."); return false;
			}
			if (!gpu_scope_push(e)) return false;
			e->loop_depth++;
			local = gpu_add_local(e, s->val.for_.counter_symbol,
					      NOCT_PACKED_INT32, false);
			if (local < 0) {
				e->loop_depth--; gpu_scope_pop(e); return false;
			}
			if (!gpu_put(e, "%*sfor (int v%d = %d; v%d < %d; v%d++) {\n",
				     indent, "", local, start, local, stop, local) ||
			    !gpu_stmts(e, s->val.for_.stmt_list, indent + 4) ||
			    !gpu_put(e, "%*s}\n", indent, "")) {
				e->loop_depth--; gpu_scope_pop(e); return false;
			}
			e->loop_depth--;
			gpu_scope_pop(e);
			break;
		case AST_STMT_RETURN:
			if (s->val.return_.has_value) {
				gpu_error(e, "Raw GPU functions cannot return a value."); return false;
			}
			if (indent != 4 || s->next != NULL) e->has_nonfinal_return = true;
			if (!gpu_put(e, "%*sreturn;\n", indent, "")) return false;
			break;
		case AST_STMT_EXPR:
			if (s->val.expr.expr == NULL ||
			    s->val.expr.expr->type != AST_EXPR_CALL ||
			    s->val.expr.expr->val.call.arg_list != NULL ||
			    s->val.expr.expr->val.call.func == NULL ||
			    s->val.expr.expr->val.call.func->type != AST_EXPR_TERM ||
			    s->val.expr.expr->val.call.func->val.term.term == NULL ||
			    s->val.expr.expr->val.call.func->val.term.term->type != AST_TERM_SYMBOL ||
			    strcmp(s->val.expr.expr->val.call.func->val.term.term->val.symbol,
				   "syncthreads") != 0) {
				gpu_error(e, "Only syncthreads() is allowed as a raw GPU expression statement.");
				return false;
			}
			if (indent != 4) {
				gpu_error(e, "syncthreads() must be at top-level uniform control flow.");
				return false;
			}
			e->has_barrier = true;
			if (!gpu_put(e, e->hlsl ?
				"%*sGroupMemoryBarrierWithGroupSync();\n" :
				"%*smemoryBarrierShared(); barrier();\n", indent, ""))
				return false;
			break;
		default:
			gpu_error(e, "Unsupported statement in raw GPU function."); return false;
		}
		previous = s;
	}
	return true;
}

static bool
gpu_collect_shared(struct gpu_emit *body, struct ast_func *afunc)
{
	struct ast_stmt *stmt;
	struct ast_expr *lhs;
	struct ast_expr *rhs;
	struct ast_expr *fn;
	struct ast_expr *arg;
	const char *shared_type_name;
	bool saw_executable;
	saw_executable = false;
	for (stmt = afunc->stmt_list != NULL ? afunc->stmt_list->list : NULL;
	     stmt != NULL; stmt = stmt->next) {
		if (stmt->type != AST_STMT_EMPTY &&
		    !(stmt->type == AST_STMT_ASSIGN && stmt->val.assign.is_shared))
			saw_executable = true;
		if (stmt->type != AST_STMT_ASSIGN || !stmt->val.assign.is_shared)
			continue;
		if (saw_executable) {
			gpu_error(body, "__shared declarations must precede executable statements."); return false;
		}
		if (body->shared_count == NOCT_ARG_MAX) {
			gpu_error(body, "Too many raw GPU shared declarations."); return false;
		}
		lhs = stmt->val.assign.lhs; rhs = stmt->val.assign.rhs;
		if (lhs == NULL || lhs->type != AST_EXPR_TERM || lhs->val.term.term == NULL ||
		    lhs->val.term.term->type != AST_TERM_SYMBOL || rhs == NULL ||
		    rhs->type != AST_EXPR_CALL || rhs->val.call.arg_list == NULL ||
		    rhs->val.call.arg_list->list == NULL ||
		    rhs->val.call.arg_list->list->next != NULL) {
			gpu_error(body, "Invalid __shared declaration."); return false;
		}
		if (gpu_reserved_name(lhs->val.term.term->val.symbol) ||
		    gpu_param(body, lhs->val.term.term->val.symbol) >= 0 ||
		    gpu_shared(body, lhs->val.term.term->val.symbol) >= 0) {
			gpu_error_name(body, "Invalid or duplicate raw GPU shared name '%s'.",
				       lhs->val.term.term->val.symbol);
			return false;
		}
		fn = rhs->val.call.func; arg = rhs->val.call.arg_list->list;
		if (fn == NULL || fn->type != AST_EXPR_DOT || fn->val.dot.obj == NULL ||
		    fn->val.dot.obj->type != AST_EXPR_TERM ||
		    fn->val.dot.obj->val.term.term == NULL ||
		    fn->val.dot.obj->val.term.term->type != AST_TERM_SYMBOL ||
		    strcmp(fn->val.dot.obj->val.term.term->val.symbol, "Accel") != 0 ||
		    arg->type != AST_EXPR_TERM || arg->val.term.term == NULL ||
		    arg->val.term.term->type != AST_TERM_INT ||
		    arg->val.term.term->val.i <= 0) {
			gpu_error(body, "__shared requires Accel.int32/uint32/float32(positive constant)."); return false;
		}
		shared_type_name = fn->val.dot.symbol;
		if (strcmp(shared_type_name, "int32") == 0)
			body->shared_type[body->shared_count] = NOCT_PACKED_INT32;
		else if (strcmp(shared_type_name, "uint32") == 0)
			body->shared_type[body->shared_count] = NOCT_PACKED_UINT32;
		else if (strcmp(shared_type_name, "float32") == 0)
			body->shared_type[body->shared_count] = NOCT_PACKED_FLOAT32;
		else {
			gpu_error(body, "__shared supports int32, uint32, or float32."); return false;
		}
		body->shared_name[body->shared_count] = lhs->val.term.term->val.symbol;
		body->shared_length[body->shared_count] =
			(uint32_t)arg->val.term.term->val.i;
		body->shared_count++;
	}
	return true;
}

bool
hir_gpu_build_kernel(struct hir_block *func, struct ast_func *afunc,
		     char *error, size_t error_size)
{
	struct gpu_emit body, hbody, out, hout;
	struct accel_kernel *k;
	const struct accel_op_desc *math_op;
	const char *helper;
	const char *type;
	uint32_t i;
	memset(&body, 0, sizeof(body));
	memset(&hbody, 0, sizeof(hbody));
	memset(&out, 0, sizeof(out));
	memset(&hout, 0, sizeof(hout));
	body.func = func; body.error = error; body.error_size = error_size;
	if (error_size != 0) error[0] = '\0';
	for (i = 0; i < func->val.func.param_count; i++) {
		if (gpu_reserved_name(func->val.func.param_name[i])) {
			gpu_error_name(&body, "'%s' is a reserved name inside __gpu func.",
				       func->val.func.param_name[i]);
			return false;
		}
		if (func->val.func.param_accel_transport[i] ==
		    ACCEL_TRANSPORT_DEVICE_PTR) {
			if (gpu_packed_type(func->val.func.param_packed_type[i]) == NULL) {
				gpu_error(&body, "Raw GPU _ptr requires int32, uint32, or float32 elements."); return false;
			}
			func->val.func.param_accel_effect[i] = ACCEL_EFFECT_NONE;
		} else if (func->val.func.param_type[i] != NOCT_VALUE_INT &&
			   func->val.func.param_type[i] != NOCT_VALUE_FLOAT) {
			gpu_error(&body, "Raw GPU scalar parameters require int or float annotations."); return false;
		}
	}
	if (!gpu_collect_shared(&body, afunc)) return false;
	if (!gpu_stmts(&body, afunc->stmt_list, 4)) {
		noct_free(body.text); return false;
	}
	if (body.has_barrier && body.has_nonfinal_return) {
		gpu_error(&body, "Raw GPU functions containing syncthreads() cannot return early.");
		noct_free(body.text); return false;
	}
	hbody.func = func; hbody.hlsl = true;
	hbody.error = error; hbody.error_size = error_size;
	hbody.shared_count = body.shared_count;
	memcpy(hbody.shared_name, body.shared_name, sizeof(body.shared_name));
	memcpy(hbody.shared_type, body.shared_type, sizeof(body.shared_type));
	memcpy(hbody.shared_length, body.shared_length, sizeof(body.shared_length));
	if (!gpu_stmts(&hbody, afunc->stmt_list, 4)) {
		noct_free(body.text); noct_free(hbody.text); return false;
	}
	if (hbody.has_barrier && hbody.has_nonfinal_return) {
		gpu_error(&hbody, "Raw GPU functions containing syncthreads() cannot return early.");
		noct_free(body.text); noct_free(hbody.text); return false;
	}
	out.func = func; out.error = error; out.error_size = error_size;
	out.shared_count = body.shared_count;
	memcpy(out.shared_name, body.shared_name, sizeof(body.shared_name));
	memcpy(out.shared_type, body.shared_type, sizeof(body.shared_type));
	memcpy(out.shared_length, body.shared_length, sizeof(body.shared_length));
	memcpy(out.used_math, body.used_math, sizeof(body.used_math));
	if (!gpu_put(&out, "#version 450\nlayout(local_size_x = __NOCT_LOCAL_SIZE_X__, local_size_y = 1, local_size_z = 1) in;\n")) goto fail;
	for (i = 1; i < ACCEL_MATH_ID_LIMIT; i++) {
		if (!out.used_math[i]) continue;
		math_op = accel_math_lookup_id((int)i);
		helper = accel_op_glsl_helper(math_op);
		if (helper != NULL && !gpu_put(&out, "%s", helper)) goto fail;
	}
	for (i = 0; i < func->val.func.param_count; i++) {
		if (func->val.func.param_accel_transport[i] !=
		    ACCEL_TRANSPORT_DEVICE_PTR) continue;
		type = gpu_packed_type(func->val.func.param_packed_type[i]);
		if (!gpu_put(&out,
			     "layout(set = 0, binding = %u, std430) buffer P%u { %s words[]; } p%u;\n",
			     i, i, type, i)) goto fail;
	}
	for (i = 0; i < out.shared_count; i++) {
		type = gpu_packed_type(out.shared_type[i]);
		if (!gpu_put(&out, "shared %s s%u[%u];\n", type, i,
			     out.shared_length[i])) goto fail;
	}
	if (!gpu_put(&out,
		     "layout(push_constant) uniform PushConstants {\n    uint grid_size;\n")) goto fail;
	for (i = 0; i < func->val.func.param_count; i++) {
		if (func->val.func.param_accel_transport[i] !=
		    ACCEL_TRANSPORT_SCALAR) continue;
		type = func->val.func.param_type[i] == NOCT_VALUE_FLOAT ?
			"float" : "int";
		if (!gpu_put(&out, "    %s p%u;\n", type, i)) goto fail;
	}
	if (!gpu_put(&out, "} pc;\nvoid main() {\n%s}\n",
		     body.text != NULL ? body.text : "")) goto fail;
	hout.func = func; hout.hlsl = true;
	hout.error = error; hout.error_size = error_size;
	hout.shared_count = hbody.shared_count;
	memcpy(hout.shared_name, hbody.shared_name, sizeof(hbody.shared_name));
	memcpy(hout.shared_type, hbody.shared_type, sizeof(hbody.shared_type));
	memcpy(hout.shared_length, hbody.shared_length, sizeof(hbody.shared_length));
	memcpy(hout.used_math, hbody.used_math, sizeof(hbody.used_math));
	if (!gpu_put(&hout, "// Noct HLSL SM 5.1\n")) goto fail_hlsl;
	for (i = 1; i < ACCEL_MATH_ID_LIMIT; i++) {
		if (!hout.used_math[i]) continue;
		math_op = accel_math_lookup_id((int)i);
		helper = accel_op_glsl_helper(math_op);
		if (helper != NULL && !gpu_put(&hout, "%s", helper)) goto fail_hlsl;
	}
	for (i = 0; i < func->val.func.param_count; i++) {
		if (func->val.func.param_accel_transport[i] !=
		    ACCEL_TRANSPORT_DEVICE_PTR) continue;
		type = gpu_packed_type(func->val.func.param_packed_type[i]);
		if (!gpu_put(&hout,
			"RWStructuredBuffer<%s> p%u : register(u%u);\n",
			type, i, i)) goto fail_hlsl;
	}
	for (i = 0; i < hout.shared_count; i++) {
		type = gpu_packed_type(hout.shared_type[i]);
		if (!gpu_put(&hout, "groupshared %s s%u[%u];\n", type, i,
			     hout.shared_length[i])) goto fail_hlsl;
	}
	if (!gpu_put(&hout, "cbuffer NoctPush : register(b0) {\n"
		     "    uint pc_grid_size;\n    uint pc_block_size;\n"))
		goto fail_hlsl;
	for (i = 0; i < func->val.func.param_count; i++) {
		if (func->val.func.param_accel_transport[i] !=
		    ACCEL_TRANSPORT_SCALAR) continue;
		type = func->val.func.param_type[i] == NOCT_VALUE_FLOAT ?
			"float" : "int";
		if (!gpu_put(&hout, "    %s pc_p%u;\n", type, i)) goto fail_hlsl;
	}
	if (!gpu_put(&hout,
		"};\n[numthreads(NOCT_LOCAL_SIZE_X, 1, 1)]\n"
		"void main(uint3 noct_group_thread_id : SV_GroupThreadID,\n"
		"          uint3 noct_group_id : SV_GroupID,\n"
		"          uint3 noct_dispatch_thread_id : SV_DispatchThreadID) {\n"
		"%s}\n", hbody.text != NULL ? hbody.text : ""))
		goto fail_hlsl;
	noct_free(body.text);
	noct_free(hbody.text);
	body.text = NULL;
	hbody.text = NULL;
	k = noct_calloc(1, sizeof(*k));
	if (k == NULL) {
		gpu_error(&out, "Out of memory creating raw GPU descriptor."); goto fail_out;
	}
	k->descriptor_version = 3; k->func_kind = NOCT_FUNC_GPU; k->eligible = true;
	k->name = noct_strdup(func->val.func.name);
	k->source_name = noct_strdup(func->val.func.file_name);
	k->param_count = func->val.func.param_count;
	k->output_param = -1; k->dispatch_param = -1;
	for (i = 0; i < k->param_count; i++) {
		k->param_type[i] = func->val.func.param_type[i];
		k->param_packed_type[i] = func->val.func.param_packed_type[i];
		k->param_transport[i] = func->val.func.param_accel_transport[i];
		k->param_effect[i] = func->val.func.param_accel_effect[i];
		k->param_access[i] = k->param_transport[i];
	}
	if (k->name == NULL || k->source_name == NULL) {
		accel_kernel_free(k);
		gpu_error(&out, "Out of memory creating raw GPU descriptor.");
		goto fail_out;
	}
	if (!gpu_ir_finalize_kernel(k, out.text, out.size,
				    error, error_size)) {
		accel_kernel_free(k);
		goto fail_out;
	}
	k->hlsl = hout.text;
	k->hlsl_size = hout.size;
	hout.text = NULL;
	noct_free(out.text);
	func->val.func.accel_kernel = k; return true;
fail:
fail_hlsl:
	noct_free(body.text);
	noct_free(hbody.text);
fail_out:
	noct_free(out.text);
	noct_free(hout.text);
	return false;
}
