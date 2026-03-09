/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * cback: C translation backend
 */

#include "cback.h"
#include "lir.h"
#include "bytecode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/*
 * False assertion
 */

#define NEVER_COME_HERE		0

/*
 * Constant
 */

#define ARG_MAX			32

/*
 * Message
 */

static const char BROKEN_BYTECODE[] = "Broken bytecode.";

/*
 * Translated function names.
 */

#define FUNC_MAX	(4096)

struct c_func {
	char *name;
	int param_count;
	char *param_name[ARG_MAX];
};

static struct c_func func_table[FUNC_MAX];
static int func_count;

/*
 * Translation context.
 */

static FILE *fp;

/*
 * Forward declaration
 */
static bool cback_visit_bytecode(struct lir_func *func);
static bool cback_visit_op(struct lir_func *func, int *pc);
static bool cback_write_dll_init(void);

/*
 * Clear translator states.
 */
bool
cback_init(
	const char *fname)
{
	fp = fopen(fname, "w");
	if (fp == NULL) {
		printf("Failed to open file \"%s\".\n", fname);
		return false;
	}

	return true;
}

/*
 * Translate LIR to C.
 */
bool
cback_translate_func(
	struct lir_func *func)
{
	int i;

	/* Save a function name. */
	func_table[func_count].name = strdup(func->func_name);
	if (func_table[func_count].name == NULL) {
		printf("Out of memory.\n");
		return false;
	}
	func_table[func_count].param_count = func->param_count;
	for (i = 0; i < func->param_count; i++) {
		func_table[func_count].param_name[i] = strdup(func->param_name[i]);
		if (func_table[func_count].param_name[i] == NULL) {
			printf("Out of memory.\n");
			return false;
		}
	}
	func_count++;

	/* Put a prologue code. */
	fprintf(fp, "#include <noct/cback.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "bool L_%s(struct rt_env *env)\n", func->func_name);
	fprintf(fp, "{\n");
	fprintf(fp, "    struct rt_value tmpvar[%d];\n", func->tmpvar_size);
	fprintf(fp, "    env->frame->tmpvar = &tmpvar[0];\n");

	/* Visit a bytecode array. */
	if (!cback_visit_bytecode(func))
		return false;

	/* Put an epilogue code. */
	fprintf(fp, "    env->frame->tmpvar = NULL;\n");
	fprintf(fp, "    return true;\n");
	fprintf(fp, "}\n\n");

	return true;
}

/* Visit a bytecode array. */
static bool
cback_visit_bytecode(
	struct lir_func *func)
{
	int pc;

	pc = 0;
	while (pc < func->bytecode_size) {
		if (!cback_visit_op(func, &pc))
			return false;
	}

	return true;
}

/* Get a u8 from bytecode. */
#define GET_U8(v) if (!cback_get_u8(func, pc, v)) return false
static INLINE bool cback_get_u8(
        struct lir_func *func,
        int *pc,
        int *val)
{
        if (*pc + 1 > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *val = func->bytecode[*pc];     

        *pc = *pc + 1;

        return true;
}

/* Get a u16 from bytecode. */
#define GET_U16(v) if (!cback_get_u16(func, pc, v)) return false
static INLINE bool cback_get_u16(
        struct lir_func *func,
        int *pc,
        int *val)
{
        if (*pc + 2 > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc] << 8) |
                (uint32_t)func->bytecode[*pc + 1];

        *pc = *pc + 2;

        return true;
}

/* Get a u16 tmpvar index from bytecode. */
#define GET_TMPVAR(v) if (!cback_get_tmpvar(func, pc, v)) return false
static INLINE bool cback_get_tmpvar(
        struct lir_func *func,
        int *pc,
        int *val)
{
        if (*pc + 2 > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc] << 8) |
                (uint32_t)func->bytecode[*pc + 1];
        if (*val >= (uint32_t)func->tmpvar_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *pc = *pc + 2;

        return true;
}

/* Get a u32 from bytecode. */
#define GET_U32(v) if (!cback_get_u32(func, pc, v)) return false
static INLINE bool cback_get_u32(
        struct lir_func *func,
        int *pc,
        uint32_t *val)
{
        if (*pc + 4 > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc + 0] << 24) |
               ((uint32_t)func->bytecode[*pc + 1] << 16) |
               ((uint32_t)func->bytecode[*pc + 2] << 8) |
                (uint32_t)func->bytecode[*pc + 3];

        *pc = *pc + 4;

        return true;
}

/* Get a u32 address from bytecode. */
#define GET_ADDR(v) if (!cback_get_addr(func, pc, v)) return false
static INLINE bool cback_get_addr(
        struct lir_func *func,
        int *pc,
        uint32_t *val)
{
        if (*pc + 4 > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc + 0] << 24) |
               ((uint32_t)func->bytecode[*pc + 1] << 16) |
               ((uint32_t)func->bytecode[*pc + 2] << 8) |
                (uint32_t)func->bytecode[*pc + 3];

        if (*val > (uint32_t)func->bytecode_size + 1) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *pc = *pc + 4;

        return true;
}

/* Get a string from bytecode. */
#define GET_STRING(s, l, h) if (!cback_get_string(func, pc, s, l, h)) return false
static INLINE bool cback_get_string(
        struct lir_func *func,
        int *pc,
        const char **s,
        uint32_t *len,
        uint32_t *hash)
{
        if (*pc + 8 > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *len = ((uint32_t)func->bytecode[*pc + 0] << 24) |
                ((uint32_t)func->bytecode[*pc + 1] << 16) |
                ((uint32_t)func->bytecode[*pc + 2] << 8) |
                (uint32_t)func->bytecode[*pc + 3];

        *hash = ((uint32_t)func->bytecode[*pc + 4] << 24) |
                ((uint32_t)func->bytecode[*pc + 5] << 16) |
                ((uint32_t)func->bytecode[*pc + 6] << 8) |
                (uint32_t)func->bytecode[*pc + 7];

        if (*pc + 8 + *len > func->bytecode_size) {
                puts(BROKEN_BYTECODE);
                return false;
        }

        *s = (const char *)&func->bytecode[*pc + 8];

        *pc = *pc + 8 + *len;

        return true;
}

/* Put a label. */
#define LABEL(pc) \
	fprintf(fp, "L_pc_%d:\n", (pc));

/* Unary OP macro. */
#define UNARY_OP(helper)							\
	int dst, src;								\
	GET_TMPVAR(&dst);							\
	GET_TMPVAR(&src);							\
	fprintf(fp, "if (!" #helper "(env, (int)dst, (int)src))");		\
	fprintf(fp, "    return false;\n");

/* Binary OP macro. */
#define BINARY_OP(helper)							\
	int dst, src1, src2;							\
	GET_TMPVAR(&dst);							\
	GET_TMPVAR(&src1);							\
	GET_TMPVAR(&src2);							\
	fprintf(fp, "if (!" #helper "(env, (int)dst, (int)src1, (int)src2))");	\
	fprintf(fp, "    return false;\n");

/* Visit a LOP_LINEINFO instruction. */
static INLINE bool
cback_visit_lineinfo_op(
	struct lir_func *func,
	int *pc)
{
	uint32_t line;

	GET_U32(&line);

	fprintf(fp, "/* line: %d */\n", line);

	return true;
}

/* Visit a LOP_ASSIGN instruction. */
static INLINE bool
cback_visit_assign_op(
	struct lir_func *func,
	int *pc)
{
	int dst, src;

	LABEL(*pc);

	GET_TMPVAR(&dst);
	GET_TMPVAR(&src);

	fprintf(fp, "env->frame->tmpvar[dst] = tmpvar[src];\n");

	return true;
}

/* Visit a LOP_ICONST instruction. */
static INLINE bool
cback_visit_iconst_op(
	struct lir_func *func,
	int *pc)
{
	int dst;
	uint32_t val;

	LABEL(*pc);

	GET_TMPVAR(&dst);
	GET_U32(&val);

	fprintf(fp, "    env->frame->tmpvar[%d].type = NOCT_VALUE_INT;\n", dst);
	fprintf(fp, "    env->frame->tmpvar[%d].val.i = %d;\n", dst, val);

	return true;
}

/* Visit a LOP_FCONST instruction. */
static INLINE bool
cback_visit_fconst_op(
	struct lir_func *func,
	int *pc)
{
	int dst;
	uint32_t raw;
	float val;

	LABEL(*pc);

	GET_TMPVAR(&dst);
	GET_U32(&raw);

	val = *(float *)&raw;

	fprintf(fp, "    env->frame->tmpvar[%d].type = NOCT_VALUE_FLOAT;\n", dst);
	fprintf(fp, "    env->frame->tmpvar[%d].val.f = %ff;\n", dst, val);

	return true;
}

/* Visit a LOP_SCONST instruction. */
static INLINE bool
cback_visit_sconst_op(
	struct lir_func *func,
	int *pc)
{
	int dst;
	const char *s;
	uint32_t len, hash;

	LABEL(*pc);

	GET_TMPVAR(&dst);
	GET_STRING(&s, &len, &hash);

	fprintf(fp, "    if (!rt_make_string(env, &env->frame->tmpvar[%d], \"%s\"))\n", dst, s);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_ACONST instruction. */
static INLINE bool
cback_visit_aconst_op(
	struct lir_func *func,
	int *pc)
{
	int dst;

	LABEL(*pc);

	GET_TMPVAR(&dst);

	fprintf(fp, "    if (!rt_make_empty_array(env, &env->frame->tmpvar[%d]))\n", dst);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_DCONST instruction. */
static INLINE bool
cback_visit_dconst_op(
	struct lir_func *func,
	int *pc)
{
	int dst;

	LABEL(*pc);

	GET_TMPVAR(&dst);

	fprintf(fp, "    if (!rt_make_empty_dict(env, &env->frame->tmpvar[%d]))\n", dst);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_INC instruction. */
static INLINE bool
cback_visit_inc_op(
	struct lir_func *func,
	int *pc)
{
	int dst;

	LABEL(*pc);

	GET_TMPVAR(&dst);

	fprintf(fp, "    env->frame->tmpvar[%d].val.i++;\n", dst);

	return true;
}

/* Visit a LOP_ADD instruction. */
static INLINE bool
cback_visit_add_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_add_heler);
	return true;
}

/* Visit a LOP_SUB instruction. */
static INLINE bool
cback_visit_sub_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_sub_helper);
	return true;
}

/* Visit a LOP_MUL instruction. */
static INLINE bool
cback_visit_mul_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_mul_helper);
	return true;
}

/* Visit a LOP_DIV instruction. */
static INLINE bool
cback_visit_div_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_div_helper);
	return true;
}

/* Visit a LOP_MOD instruction. */
static INLINE bool
cback_visit_mod_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_mod_helper);
	return true;
}

/* Visit a LOP_AND instruction. */
static INLINE bool
cback_visit_and_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_and_helper);
	return true;
}

/* Visit a LOP_OR instruction. */
static INLINE bool
cback_visit_or_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_or_helper);
	return true;
}

/* Visit a LOP_XOR instruction. */
static INLINE bool
cback_visit_xor_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_xor_helper);
	return true;
}

/* Visit a LOP_XOR instruction. */
static INLINE bool
cback_visit_shl_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_shl_helper);
	return true;
}

/* Visit a LOP_XOR instruction. */
static INLINE bool
cback_visit_shr_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_shr_helper);
	return true;
}

/* Visit a LOP_NEG instruction. */
static INLINE bool
cback_visit_neg_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	UNARY_OP(rt_neg_helper);
	return true;
}

/* Visit a LOP_LT instruction. */
static INLINE bool
cback_visit_lt_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_lt_helper);
	return true;
}

/* Visit a LOP_LTE instruction. */
static INLINE bool
cback_visit_lte_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_lte_helper);
	return true;
}

/* Visit a LOP_GT instruction. */
static INLINE bool
cback_visit_gt_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_gt_helper);
	return true;
}

/* Visit a LOP_GTE instruction. */
static INLINE bool
cback_visit_gte_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_gte_helper);
	return true;
}

/* Visit a LOP_EQ instruction. */
static INLINE bool
cback_visit_eq_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_eq_helper);
	return true;
}

/* Visit a LOP_NEQ instruction. */
static INLINE bool
cback_visit_neq_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_neq_helper);
	return true;
}

/* Visit a LOP_STOREARRAY instruction. */
static INLINE bool
cback_visit_storearray_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_storearray_helper);
	return true;
}

/* Visit a LOP_LOADARRAY instruction. */
static INLINE bool
cback_visit_loadarray_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_loadarray_helper);
	return true;
}

/* Visit a LOP_LEN instruction. */
static INLINE bool
cback_visit_len_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	UNARY_OP(rt_len_helper);
	return true;
}

/* Visit a LOP_GETDICTKEYBYINDEX instruction. */
static INLINE bool
cback_visit_getdictkeybyindex_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_getdictkeybyindex_helper);
	return true;
}

/* Visit a LOP_GETDICTVALBYINDEX instruction. */
static INLINE bool
cback_visit_getdictvalbyindex_op(
	struct lir_func *func,
	int *pc)
{
	LABEL(*pc);
	BINARY_OP(rt_getdictvalbyindex_helper);
	return true;
}

/* Visit a LOP_LOADYMBOL instruction. */
static INLINE bool
cback_visit_loadsymbol_op(
	struct lir_func *func,
	int *pc)
{
	int dst;
	const char *symbol;
	uint32_t len, hash;

	LABEL(*pc);

	GET_TMPVAR(&dst);
	GET_STRING(&symbol, &len, &hash);

	fprintf(fp, "    if (!rt_loadsymbol_helper(env, %d, \"%s\"))\n", dst, symbol);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_STORESYMBOL instruction. */
static INLINE bool
cback_visit_storesymbol_op(
	struct lir_func *func,
	int *pc)
{
	const char *symbol;
	uint32_t len, hash;
	int src;

	LABEL(*pc);

	GET_STRING(&symbol, &len, &hash);
	GET_TMPVAR(&src);

	fprintf(fp, "    if (!rt_storesymbol_helper(env, \"%s\", %d))\n", symbol, src);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_LOADDOT instruction. */
static INLINE bool
cback_visit_loaddot_op(
	struct lir_func *func,
	int *pc)
{
	int dst, dict;
	const char *field;
	uint32_t len, hash;

	LABEL(*pc);

	GET_TMPVAR(&dst);
	GET_TMPVAR(&dict);
	GET_STRING(&field, &len, &hash);

	fprintf(fp, "    if (!rt_loaddot_helper(env, %d, %d, \"%s\"))\n", dst, dict, field);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_STOREDOT instruction. */
static INLINE bool
cback_visit_storedot_op(
	struct lir_func *func,
	int *pc)
{
	int src, dict;
	const char *field;
	uint32_t len, hash;

	LABEL(*pc);

	GET_TMPVAR(&dict);
	GET_STRING(&field, &len, &hash);
	GET_TMPVAR(&src);

	fprintf(fp, "    if (!rt_storedot_helper(env, %d, \"%s\", %d))\n", dict, field, src);
	fprintf(fp, "        return false;\n");

	return true;
}

/* Visit a LOP_CALL instruction. */
static INLINE bool
cback_visit_call_op(
	struct lir_func *func,
	int *pc)
{
	int dst_tmpvar;
	int func_tmpvar;
	int arg_count;
	int arg_tmpvar;
	int arg[ARG_MAX];
	int i;

	LABEL(*pc);

	GET_TMPVAR(&dst_tmpvar);
	GET_TMPVAR(&func_tmpvar);
	GET_U8(&arg_count);
	for (i = 0; i < arg_count; i++) {
		GET_TMPVAR(&arg_tmpvar);
		arg[i] = arg_tmpvar;
	}

	fprintf(fp, "    {\n");
	fprintf(fp, "        int arg[%d] = {", arg_count);
	for (i = 0; i < arg_count; i++)
		fprintf(fp, "%d,", arg[i]);
	fprintf(fp, "};\n");
	fprintf(fp, "        if (!rt_call_helper(env, %d, %d, %d, arg))\n", dst_tmpvar, func_tmpvar, arg_count);
	fprintf(fp, "            return false;\n");
	fprintf(fp, "    };\n");

	return true;
}

/* Visit a LOP_THISCALL instruction. */
static INLINE bool
cback_visit_thiscall_op(
	struct lir_func *func,
	int *pc)
{
	int dst_tmpvar;
	int obj_tmpvar;
	const char *name;
	uint32_t len, hash;
	int arg_count;
	int arg_tmpvar;
	int arg[ARG_MAX];
	int i;

	LABEL(*pc);

	GET_TMPVAR(&dst_tmpvar);
	GET_TMPVAR(&obj_tmpvar);
	GET_STRING(&name, &len, &hash);
	GET_U8(&arg_count);
	for (i = 0; i < arg_count; i++) {
		GET_TMPVAR(&arg_tmpvar);
		arg[i] = arg_tmpvar;
	}

	fprintf(fp, "    {\n");
	fprintf(fp, "        int arg[%d] = {", arg_count);
	for (i = 0; i < arg_count; i++)
		fprintf(fp, "%d,", arg[i]);
	fprintf(fp, "};\n");
	fprintf(fp, "        if (!rt_thiscall_helper(env, %d, %d, \"%s\", %d, arg))\n", dst_tmpvar, obj_tmpvar, name, arg_count);
	fprintf(fp, "            return false;\n");
	fprintf(fp, "    };\n");

	return true;
}

/* Visit a LOP_JMP instruction. */
static inline bool
cback_visit_jmp_op(
	struct lir_func *func,
	int *pc)
{
	uint32_t target;

	LABEL(*pc);

	GET_ADDR(&target);

	fprintf(fp, "    goto L_pc_%d;\n", target);

	return true;
}

/* Visit a LOP_JMPIFTRUE instruction. */
static bool
cback_visit_jmpiftrue_op(
	struct lir_func *func,
	int *pc)
{
	int src;
	uint32_t target;

	LABEL(*pc);

	GET_TMPVAR(&src);
	GET_ADDR(&target);

	fprintf(fp, "    if (env->frame->tmpvar[%d].val.i != 0)\n", src);
	fprintf(fp, "        goto L_pc_%d;\n", target);

	return true;
}

/* Visit a LOP_JMPIFFALSE instruction. */
static INLINE bool
cback_visit_jmpiffalse_op(
	struct lir_func *func,
	int *pc)
{
	int src;
	uint32_t target;

	LABEL(*pc);

	GET_TMPVAR(&src);
	GET_ADDR(&target);

	fprintf(fp, "    if (env->frame->tmpvar[%d].val.i == 0)\n", src);
	fprintf(fp, "        goto L_pc_%d;\n", target);

	return true;
}

/* Visit an instruction. */
static bool
cback_visit_op(
	struct lir_func *func,
	int *pc)
{
	int op;

	GET_U8(&op);

	switch (op) {
	case OP_NOP:
		/* NOP */
		(*pc)++;
		break;
	case OP_LINEINFO:
		if (!cback_visit_lineinfo_op(func, pc))
			return false;
		break;
	case OP_ASSIGN:
		if (!cback_visit_assign_op(func, pc))
			return false;
		break;
	case OP_ICONST:
		if (!cback_visit_iconst_op(func, pc))
			return false;
		break;
	case OP_FCONST:
		if (!cback_visit_fconst_op(func, pc))
			return false;
		break;
	case OP_SCONST:
		if (!cback_visit_sconst_op(func, pc))
			return false;
		break;
	case OP_ACONST:
		if (!cback_visit_aconst_op(func, pc))
			return false;
		break;
	case OP_DCONST:
		if (!cback_visit_dconst_op(func, pc))
			return false;
		break;
	case OP_INC:
		if (!cback_visit_inc_op(func, pc))
			return false;
		break;
	case OP_ADD:
		if (!cback_visit_add_op(func, pc))
			return false;
		break;
	case OP_SUB:
		if (!cback_visit_sub_op(func, pc))
			return false;
		break;
	case OP_MUL:
		if (!cback_visit_mul_op(func, pc))
			return false;
		break;
	case OP_DIV:
		if (!cback_visit_div_op(func, pc))
			return false;
		break;
	case OP_MOD:
		if (!cback_visit_mod_op(func, pc))
			return false;
		break;
	case OP_AND:
		if (!cback_visit_and_op(func, pc))
			return false;
		break;
	case OP_OR:
		if (!cback_visit_or_op(func, pc))
			return false;
		break;
	case OP_XOR:
		if (!cback_visit_xor_op(func, pc))
			return false;
		break;
	case OP_NEG:
		if (!cback_visit_neg_op(func, pc))
			return false;
		break;
	case OP_LT:
		if (!cback_visit_lt_op(func, pc))
			return false;
		break;
	case OP_LTE:
		if (!cback_visit_lte_op(func, pc))
			return false;
		break;
	case OP_GT:
		if (!cback_visit_gt_op(func, pc))
			return false;
		break;
	case OP_GTE:
		if (!cback_visit_gte_op(func, pc))
			return false;
		break;
	case OP_EQ:
		if (!cback_visit_eq_op(func, pc))
			return false;
		break;
	case OP_EQI:
		/* Same as EQ. EQI is an optimization hint for JIT-compiler. */
		if (!cback_visit_eq_op(func, pc))
			return false;
		break;
	case OP_NEQ:
		if (!cback_visit_neq_op(func, pc))
			return false;
		break;
	case OP_STOREARRAY:
		if (!cback_visit_storearray_op(func, pc))
			return false;
		break;
	case OP_LOADARRAY:
		if (!cback_visit_loadarray_op(func, pc))
			return false;
		break;
	case OP_LEN:
		if (!cback_visit_len_op(func, pc))
			return false;
		break;
	case OP_GETDICTKEYBYINDEX:
		if (!cback_visit_getdictkeybyindex_op(func, pc))
			return false;
		break;
	case OP_GETDICTVALBYINDEX:
		if (!cback_visit_getdictvalbyindex_op(func, pc))
			return false;
		break;
	case OP_LOADSYMBOL:
		if (!cback_visit_loadsymbol_op(func, pc))
			return false;
		break;
	case OP_STORESYMBOL:
		if (!cback_visit_storesymbol_op(func, pc))
			return false;
		break;
	case OP_LOADDOT:
		if (!cback_visit_loaddot_op(func, pc))
			return false;
		break;
	case OP_STOREDOT:
		if (!cback_visit_storedot_op(func, pc))
			return false;
		break;
	case OP_CALL:
		if (!cback_visit_call_op(func, pc))
			return false;
		break;
	case OP_THISCALL:
		if (!cback_visit_thiscall_op(func, pc))
			return false;
		break;
	case OP_JMP:
		if (!cback_visit_jmp_op(func, pc))
			return false;
		break;
	case OP_JMPIFTRUE:
		if (!cback_visit_jmpiftrue_op(func, pc))
			return false;
		break;
	case OP_JMPIFFALSE:
		if (!cback_visit_jmpiffalse_op(func, pc))
			return false;
		break;
	case OP_JMPIFEQ:
		/* Same as JMPIFTRUE. (JMPIFEQ is an optimization hint for JIT-compiler.) */
		if (!cback_visit_jmpiftrue_op(func, pc))
			return false;
		break;
	default:
		printf("Unknow opcode.");
		return false;
	}

	return true;
}


/*
 * Put a finalization code for a plugin.
 */
bool cback_finalize_dll(void)
{
	if (!cback_write_dll_init())
		return false;

	fclose(fp);
	fp = NULL;
	
	return true;
}

static bool cback_write_dll_init(void)
{
	int i, j;

	fprintf(fp, "bool init_aot_code(struct rt_env *env)\n");
	fprintf(fp, "{\n");
	for (i = 0; i < func_count; i++) {
		fprintf(fp, "    {\n");
		if (func_table[i].param_count > 0) {
			fprintf(fp, "        const char *params[] = {");
			for (j = 0; j < func_table[i].param_count; j++)
				fprintf(fp, "\"%s\",", func_table[i].param_name[i]);
			fprintf(fp, "};\n");
			fprintf(fp, "        if (!noct_register_cfunc(env, \"%s\", %d, params, L_%s, NULL))\n",
				func_table[i].name, func_table[i].param_count, func_table[i].name);
			fprintf(fp, "            return false;\n");
		} else {
			fprintf(fp, "        if (!noct_register_cfunc(env, \"%s\", 0, NULL, L_%s, NULL))\n",
				func_table[i].name, func_table[i].name);
			fprintf(fp, "            return false;\n");
		}
		fprintf(fp, "    }\n");
	}
	fprintf(fp, "    return true;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");

	return true;
}
