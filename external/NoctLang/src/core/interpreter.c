/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Bytecode Interpreter
 */

#include <noct/noct.h>
#include <noct/executor.h>
#include "runtime.h"
#include "bytecode.h"
#include "objectmodel.h"

#include <stdio.h>
#include <string.h>

/* Debug trace */
#if 0
#define DEBUG_TRACE(pc, op)     printf("[TRACE] pc=%d, opcode=%s\n", pc, op)
#else
#define DEBUG_TRACE(pc, op)
#endif

/* False assertion */
#define NOT_IMPLEMENTED         0
#define NEVER_COME_HERE         0

/* Message. */
#define BROKEN_BYTECODE         "Broken bytecode."

/* Unary OP macro */
#define UNARY_OP(helper)                                        \
        int dst, src;                                           \
                                                                \
        GET_TMPVAR(&dst);                                       \
        GET_TMPVAR(&src);                                       \
        if (!helper(env, (int)dst, (int)src))                   \
                return false;                                   \
        return true

/* Binary OP macro */
#define BINARY_OP(helper)                                       \
        int dst, src1, src2;                                    \
                                                                \
        GET_TMPVAR(&dst);                                       \
        GET_TMPVAR(&src1);                                      \
        GET_TMPVAR(&src2);                                      \
        if (!helper(env, (int)dst, (int)src1, (int)src2))       \
                return false;                                   \
        return true

static bool rt_visit_op(struct rt_env *env, struct rt_func *func, uint32_t *pc);

/*
 * Visit a bytecode array.
 */
bool
rt_visit_bytecode(
        struct rt_env *env,
        struct rt_func *func)
{
        uint32_t pc;

        pc = 0;
        while (pc < func->bytecode_size) {
                if (!rt_visit_op(env, func, &pc))
                        return false;
        }

        return true;
}

#define GET_U8(v) if (!rt_get_u8(env, func, pc, v)) return false
static INLINE bool rt_get_u8(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        int *val)
{
        if (*pc + 1 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *val = (int)(uint32_t)func->bytecode[*pc];     

        *pc = *pc + 1;

        return true;
}

#define GET_TMPVAR(v) if (!rt_get_tmpvar(env, func, pc, v)) return false
static INLINE bool rt_get_tmpvar(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        int *val)
{
        if (*pc + 2 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *val = (int)(
                (uint32_t)((uint32_t)func->bytecode[*pc] << 8) |
                (uint32_t)func->bytecode[*pc + 1]
               );
        if ((uint32_t)*val >= func->tmpvar_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *pc = *pc + 2;

        return true;
}

#define GET_U32(v) if (!rt_get_u32(env, func, pc, v)) return false
static INLINE bool rt_get_u32(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        uint32_t *val)
{
        if (*pc + 4 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc + 0] << 24) |
               ((uint32_t)func->bytecode[*pc + 1] << 16) |
               ((uint32_t)func->bytecode[*pc + 2] << 8) |
                (uint32_t)func->bytecode[*pc + 3];

        *pc = *pc + 4;

        return true;
}

#define GET_U64(v) if (!rt_get_u64(env, func, pc, v)) return false
static INLINE bool rt_get_u64(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        uint64_t *val)
{
        if (*pc + 8 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint64_t)func->bytecode[*pc + 0] << 56) |
               ((uint64_t)func->bytecode[*pc + 1] << 48) |
               ((uint64_t)func->bytecode[*pc + 2] << 40) |
               ((uint64_t)func->bytecode[*pc + 3] << 32) |
               ((uint64_t)func->bytecode[*pc + 4] << 24) |
               ((uint64_t)func->bytecode[*pc + 5] << 16) |
               ((uint64_t)func->bytecode[*pc + 6] << 8) |
                (uint64_t)func->bytecode[*pc + 7];

        *pc = *pc + 8;

        return true;
}

#define GET_ADDR(v) if (!rt_get_addr(env, func, pc, v)) return false
static INLINE bool rt_get_addr(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        uint32_t *val)
{
        if (*pc + 4 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc + 0] << 24) |
               ((uint32_t)func->bytecode[*pc + 1] << 16) |
               ((uint32_t)func->bytecode[*pc + 2] << 8) |
                (uint32_t)func->bytecode[*pc + 3];

        if (*val > (uint32_t)func->bytecode_size + 1) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *pc = *pc + 4;

        return true;
}

#define GET_STRING(s, l, h) if (!rt_get_string(env, func, pc, s, l, h)) return false
static INLINE bool rt_get_string(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        const char **s,
        uint32_t *len,
        uint32_t *hash)
{
        if (*pc + 8 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
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
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *s = (const char *)&func->bytecode[*pc + 8];
        
        *pc = *pc + 8 + *len;

        return true;
}

/* Visit a OP_LINEINFO instruction. */
static INLINE bool
rt_visit_lineinfo_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        uint32_t line;

        DEBUG_TRACE(*pc, "LINEINFO");

        GET_U32(&line);

        env->line = (int)line;

        return true;
}

/* Visit a OP_ASSIGN instruction. */
static INLINE bool
rt_visit_assign_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst, src;

        DEBUG_TRACE(*pc, "ASSIGN");

        GET_TMPVAR(&dst);
        GET_TMPVAR(&src);

        env->frame->tmpvar[dst] = env->frame->tmpvar[src];

        return true;
}

/* Visit a OP_ICONST instruction. */
static INLINE bool
rt_visit_iconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        uint32_t val;

        DEBUG_TRACE(*pc, "ICONST");

        GET_TMPVAR(&dst);
        GET_U32(&val);

        env->frame->tmpvar[dst].type = NOCT_VALUE_INT;
        env->frame->tmpvar[dst].val.i = (int)val;

        return true;
}

/* Visit a OP_LICONST instruction. */
static INLINE bool
rt_visit_liconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        uint64_t val;

        DEBUG_TRACE(*pc, "ICONST");

        GET_TMPVAR(&dst);
        GET_U64(&val);

        env->frame->tmpvar[dst].type = NOCT_VALUE_LONG;
        env->frame->tmpvar[dst].val.l = (int64_t)val;

        return true;
}

/* Visit a OP_FCONST instruction. */
static INLINE bool
rt_visit_fconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        uint32_t raw;
        float val;

        DEBUG_TRACE(*pc, "FCONST");

        GET_TMPVAR(&dst);
        GET_U32(&raw);

        val = *(float *)&raw;

        env->frame->tmpvar[dst].type = NOCT_VALUE_FLOAT;
        env->frame->tmpvar[dst].val.f = val;

        return true;
}

/* Visit a OP_LFCONST instruction. */
static INLINE bool
rt_visit_lfconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        uint64_t raw;
        double val;

        DEBUG_TRACE(*pc, "FCONST");

        GET_TMPVAR(&dst);
        GET_U64(&raw);

        val = *(double *)&raw;

        env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE;
        env->frame->tmpvar[dst].val.lf = val;

        return true;
}

/* Visit a OP_SCONST instruction. */
static INLINE bool
rt_visit_sconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        const char *s;
        uint32_t len, hash;

        DEBUG_TRACE(*pc, "SCONST");

        GET_TMPVAR(&dst);
        GET_STRING(&s, &len, &hash);

        if (!rt_make_string_with_hash(env, &env->frame->tmpvar[dst], s, len, hash))
                return false;

        return true;
}

/* Visit a OP_ACONST instruction. */
static INLINE bool
rt_visit_aconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;

        DEBUG_TRACE(*pc, "ACONST");

        GET_TMPVAR(&dst);

        if (!rt_make_empty_array(env, &env->frame->tmpvar[dst]))
                return false;

        return true;
}

/* Visit a OP_DCONST instruction. */
static INLINE bool
rt_visit_dconst_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;

        DEBUG_TRACE(*pc, "DCONST");

        GET_TMPVAR(&dst);

        if (!rt_make_empty_dict(env, &env->frame->tmpvar[dst]))
                return false;

        return true;
}

/* Visit a OP_INC instruction. */
static INLINE bool
rt_visit_inc_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
	struct rt_value *val;
	int dst;
	int step;

        DEBUG_TRACE(*pc, "INC");

	GET_TMPVAR(&dst);
	GET_U8(&step);

        val = &env->frame->tmpvar[dst];
        if (val->type != NOCT_VALUE_INT) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }
	val->val.i += step;

        return true;
}

/* Visit a OP_ADD instruction. */
static INLINE bool
rt_visit_add_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "ADD");

        BINARY_OP(noct_ex_add_helper);
}

/* Visit a OP_SUB instruction. */
static INLINE bool
rt_visit_sub_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "SUB");

        BINARY_OP(noct_ex_sub_helper);
}

/* Visit a OP_MUL instruction. */
static INLINE bool
rt_visit_mul_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "MUL");

        BINARY_OP(noct_ex_mul_helper);
}

/* Visit a OP_DIV instruction. */
static INLINE bool
rt_visit_div_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "DIV");

        BINARY_OP(noct_ex_div_helper);
}

/* Visit a OP_MOD instruction. */
static INLINE bool
rt_visit_mod_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "MOD");

        BINARY_OP(noct_ex_mod_helper);
}

/* Visit a OP_AND instruction. */
static INLINE bool
rt_visit_and_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "AND");

        BINARY_OP(noct_ex_and_helper);
}

/* Visit a OP_OR instruction. */
static INLINE bool
rt_visit_or_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "OR");

        BINARY_OP(noct_ex_or_helper);
}

/* Visit a OP_XOR instruction. */
static INLINE bool
rt_visit_xor_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "XOR");

        BINARY_OP(noct_ex_xor_helper);
}

/* Visit a OP_SHL instruction. */
static INLINE bool
rt_visit_shl_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "SHL");

        BINARY_OP(noct_ex_shl_helper);
}

/* Visit a OP_SHR instruction. */
static INLINE bool
rt_visit_shr_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "SHR");

        BINARY_OP(noct_ex_shr_helper);
}

/* Visit a OP_NEG instruction. */
static INLINE bool
rt_visit_neg_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "NEG");

        UNARY_OP(noct_ex_neg_helper);
}

/* Visit a OP_NOT instruction. */
static INLINE bool
rt_visit_not_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "NOT");

        UNARY_OP(noct_ex_not_helper);
}

/* Visit a OP_LT instruction. */
static INLINE bool
rt_visit_lt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LT");

        BINARY_OP(noct_ex_lt_helper);
}

/* Visit a OP_LTE instruction. */
static INLINE bool
rt_visit_lte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LTE");

        BINARY_OP(noct_ex_lte_helper);
}

/* Visit a OP_GT instruction. */
static INLINE bool
rt_visit_gt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GT");

        BINARY_OP(noct_ex_gt_helper);
}

/* Visit a OP_GTE instruction. */
static INLINE bool
rt_visit_gte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GTE");

        BINARY_OP(noct_ex_gte_helper);
}

/* Visit a OP_EQ instruction. */
static INLINE bool
rt_visit_eq_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "EQ");

        BINARY_OP(noct_ex_eq_helper);
}

/* Visit a OP_NEQ instruction. */
static INLINE bool
rt_visit_neq_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "NEQ");

        BINARY_OP(noct_ex_neq_helper);
}

/* Visit a OP_STOREARRAY instruction. */
static INLINE bool
rt_visit_storearray_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "STOREARRAY");

        BINARY_OP(noct_ex_storearray_helper);
}

/* Visit a OP_LOADARRAY instruction. */
static INLINE bool
rt_visit_loadarray_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LOADARRAY");

        BINARY_OP(noct_ex_loadarray_helper);
}

/* Visit a OP_LEN instruction. */
static INLINE bool
rt_visit_len_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LEN");

        UNARY_OP(noct_ex_len_helper);
}

/* Visit a OP_GETDICTKEYBYINDEX instruction. */
static INLINE bool
rt_visit_getdictkeybyindex_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GETDICTKEYBYINDEX");

        BINARY_OP(noct_ex_getdictkeybyindex_helper);
}

/* Visit a OP_GETDICTVALBYINDEX instruction. */
static INLINE bool
rt_visit_getdictvalbyindex_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GETDICTVALBYINDEX");

        BINARY_OP(noct_ex_getdictvalbyindex_helper);
}

/* Visit a OP_LOADYMBOL instruction. */
static INLINE bool
rt_visit_loadsymbol_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        const char *s;
        uint32_t len, hash;

        DEBUG_TRACE(*pc, "LOADSYMBOL");

        GET_TMPVAR(&dst);
        GET_STRING(&s, &len, &hash);

        if (!noct_ex_loadsymbol_helper(env, dst, s, len, hash))
                return false;

        return true;
}

/* Visit a OP_STORESYMBOL instruction. */
static INLINE bool
rt_visit_storesymbol_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        const char *s;
        uint32_t len, hash;
        int src;

        DEBUG_TRACE(*pc, "STORESYMBOL");

        GET_STRING(&s, &len, &hash);
        GET_TMPVAR(&src);

        if (!noct_ex_storesymbol_helper(env, s, len, hash, src))
                return false;

        return true;
}

/* Visit a OP_LOADDOT instruction. */
static INLINE bool
rt_visit_loaddot_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst, dict;
        const char *field;
        uint32_t len, hash;

        DEBUG_TRACE(*pc, "LOADDOT");

        GET_TMPVAR(&dst);
        GET_TMPVAR(&dict);
        GET_STRING(&field, &len, &hash);

        if (!noct_ex_loaddot_helper(env, dst, dict, field, len, hash))
                return false;

        return true;
}

/* Visit a OP_STOREDOT instruction. */
static INLINE bool
rt_visit_storedot_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dict, src;
        const char *field;
        uint32_t len, hash;

        DEBUG_TRACE(*pc, "STOREDOT");

        GET_TMPVAR(&dict);
        GET_STRING(&field, &len, &hash);
        GET_TMPVAR(&src);

        if (!noct_ex_storedot_helper(env, dict, field, len, hash, src))
                return false;

        return true;
}

/* Visit a OP_CALL instruction. */
static INLINE bool
rt_visit_call_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst_tmpvar;
        int func_tmpvar;
        int arg_count;
        int arg_tmpvar;
        int arg[NOCT_ARG_MAX];
        int i;

        DEBUG_TRACE(*pc, "CALL");

        GET_TMPVAR(&dst_tmpvar);
        GET_TMPVAR(&func_tmpvar);
        GET_U8(&arg_count);

        for (i = 0; i < arg_count; i++) {
                GET_TMPVAR(&arg_tmpvar);
                arg[i] = arg_tmpvar;
        }

        if (!noct_ex_call_helper(env, dst_tmpvar, func_tmpvar, arg_count, arg))
                return false;

        return true;
}

/* Visit a OP_THISCALL instruction. */
static INLINE bool
rt_visit_thiscall_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
	int dst_tmpvar;
	int obj_tmpvar;
	int func_tmpvar;
        int arg_count;
        int arg_tmpvar;
        int arg[NOCT_ARG_MAX];
        int i;

        DEBUG_TRACE(*pc, "THISCALL");

	GET_TMPVAR(&dst_tmpvar);
	GET_TMPVAR(&obj_tmpvar);
	GET_TMPVAR(&func_tmpvar);
        GET_U8(&arg_count);

        for (i = 0; i < arg_count; i++) {
                GET_TMPVAR(&arg_tmpvar);
                arg[i] = arg_tmpvar;
        }

	if (!noct_ex_thiscall_helper(env,
                                dst_tmpvar,
                                obj_tmpvar,
                                NULL,
                                0,
				(uint32_t)func_tmpvar,
                                arg_count,
                                arg))
                return false;

        return true;
}

/* Visit a OP_JMP instruction. */
static INLINE bool
rt_visit_jmp_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        uint32_t target;

        DEBUG_TRACE(*pc, "JMP");

        GET_ADDR(&target);

        /* Jump. */
        *pc = target;

        return true;
}

/* Visit a OP_JMPIFTRUE instruction. */
static INLINE bool
rt_visit_jmpiftrue_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        struct rt_value *v;
        int src;
        uint32_t target;
        bool truthy;

        DEBUG_TRACE(*pc, "JMPIFTRUE");

        GET_TMPVAR(&src);
        GET_ADDR(&target);

        v = &env->frame->tmpvar[src];

        /* Evaluate the condition: a zero of any numeric type is false. */
        switch (v->type) {
        case NOCT_VALUE_INT:
                truthy = v->val.i != 0;
                break;
        case NOCT_VALUE_LONG:
                truthy = v->val.l != 0;
                break;
        case NOCT_VALUE_FLOAT:
                truthy = v->val.f != 0.0f;
                break;
        case NOCT_VALUE_DOUBLE:
                truthy = v->val.lf != 0.0;
                break;
        default:
                rt_error(env, N_TR("Condition is not a number."));
                return false;
        }

        /* Jump. */
        if (truthy)
                *pc = target;

        return true;
}

/* Visit a OP_JMPIFFALSE instruction. */
static INLINE bool
rt_visit_jmpiffalse_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        struct rt_value *v;
        int src;
        uint32_t target;
        bool truthy;

        DEBUG_TRACE(*pc, "JMPIFFALSE");

        GET_TMPVAR(&src);
        GET_ADDR(&target);

        v = &env->frame->tmpvar[src];

        /* Evaluate the condition: a zero of any numeric type is false. */
        switch (v->type) {
        case NOCT_VALUE_INT:
                truthy = v->val.i != 0;
                break;
        case NOCT_VALUE_LONG:
                truthy = v->val.l != 0;
                break;
        case NOCT_VALUE_FLOAT:
                truthy = v->val.f != 0.0f;
                break;
        case NOCT_VALUE_DOUBLE:
                truthy = v->val.lf != 0.0;
                break;
        default:
                rt_error(env, N_TR("Condition is not a number."));
                return false;
        }

        /* Jump. */
        if (!truthy)
                *pc = target;

        return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static INLINE bool
rt_visit_safepoint_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        UNUSED_PARAMETER(func);
        UNUSED_PARAMETER(pc);

        DEBUG_TRACE(*pc, "SAFEPOINT");

        om_safepoint(env);

        return true;
}

/* Visit a OP_PBASE instruction. */
static INLINE bool
rt_visit_pbase_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
	int dst;
	int src;
	int base_id;

	UNUSED_PARAMETER(base_id);

	DEBUG_TRACE(*pc, "PBASE");

	GET_TMPVAR(&dst);
	GET_TMPVAR(&src);
	GET_U8(&base_id);

	if (!noct_ex_pbase_helper(env, dst, src))
                return false;

        return true;
}

/* Visit a OP_PLEN instruction. */
static INLINE bool
rt_visit_plen_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLEN");

        UNARY_OP(noct_ex_plen_helper);
}

/* Visit a OP_PCHECK instruction. */
static INLINE bool
rt_visit_pcheck_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst, src, type;

        DEBUG_TRACE(*pc, "PCHECK");

        GET_TMPVAR(&dst);
        GET_TMPVAR(&src);
        GET_U8(&type);

        if (!noct_ex_pcheck_helper(env, (int)dst, (int)src, (int)type))
                return false;

        return true;
}

/* Visit a OP_TYPEIS instruction. */
static INLINE bool
rt_visit_typeis_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst, src, type;

        DEBUG_TRACE(*pc, "TYPEIS");

        GET_TMPVAR(&dst);
        GET_TMPVAR(&src);
        GET_U8(&type);

        if (!noct_ex_typeis_helper(env, (int)dst, (int)src, (int)type))
                return false;

        return true;
}

/* Visit a OP_PLOAD8U instruction. */
static INLINE bool
rt_visit_pload8u_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOAD8U");

        BINARY_OP(noct_ex_pload8u_helper);
}

/* Visit a OP_PSTORE8 instruction. (Operand order: base, ofs, src.) */
static INLINE bool
rt_visit_pstore8_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PSTORE8");

        BINARY_OP(noct_ex_pstore8_helper);
}

/* Visit a OP_PLOAD8S instruction. */
static INLINE bool
rt_visit_pload8s_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOAD8S");

        BINARY_OP(noct_ex_pload8s_helper);
}

/* Visit a OP_PLOAD16U instruction. */
static INLINE bool
rt_visit_pload16u_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOAD16U");

        BINARY_OP(noct_ex_pload16u_helper);
}

/* Visit a OP_PLOAD16S instruction. */
static INLINE bool
rt_visit_pload16s_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOAD16S");

        BINARY_OP(noct_ex_pload16s_helper);
}

/* Visit a OP_PLOAD32 instruction. */
static INLINE bool
rt_visit_pload32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOAD32");

        BINARY_OP(noct_ex_pload32_helper);
}

/* Visit a OP_PLOAD64 instruction. */
static INLINE bool
rt_visit_pload64_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOAD64");

        BINARY_OP(noct_ex_pload64_helper);
}

/* Visit a OP_PSTORE16 instruction. (Operand order: base, ofs, src.) */
static INLINE bool
rt_visit_pstore16_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PSTORE16");

        BINARY_OP(noct_ex_pstore16_helper);
}

/* Visit a OP_PSTORE32 instruction. (Operand order: base, ofs, src.) */
static INLINE bool
rt_visit_pstore32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PSTORE32");

        BINARY_OP(noct_ex_pstore32_helper);
}

/* Visit a OP_PSTORE64 instruction. (Operand order: base, ofs, src.) */
static INLINE bool
rt_visit_pstore64_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PSTORE64");

        BINARY_OP(noct_ex_pstore64_helper);
}

/* Visit raw float32 packed load/store operations. */
static INLINE bool
rt_visit_ploadf32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PLOADF32");
        BINARY_OP(noct_ex_ploadf32_helper);
}

static INLINE bool
rt_visit_pstoref32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "PSTOREF32");

        BINARY_OP(noct_ex_pstoref32_helper);
}

/* Visit a OP_CHECKTYPE instruction. */
static INLINE bool
rt_visit_checktype_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int slot, type;

        DEBUG_TRACE(*pc, "CHECKTYPE");

        GET_TMPVAR(&slot);
        GET_U8(&type);

        if (!noct_ex_checktype_helper(env, (int)slot, (int)type))
                return false;

        return true;
}

/* Visit an OP_IADD instruction. */
static INLINE bool
rt_visit_iadd_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IADD");
        BINARY_OP(noct_ex_iadd_helper);
}

/* Visit an OP_ISUB instruction. */
static INLINE bool
rt_visit_isub_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "ISUB");
        BINARY_OP(noct_ex_isub_helper);
}

/* Visit an OP_IMUL instruction. */
static INLINE bool
rt_visit_imul_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IMUL");
        BINARY_OP(noct_ex_imul_helper);
}

/* Visit an OP_IDIV instruction. */
static INLINE bool
rt_visit_idiv_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IDIV");
        BINARY_OP(noct_ex_idiv_helper);
}

/* Visit an OP_IMOD instruction. */
static INLINE bool
rt_visit_imod_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IMOD");
        BINARY_OP(noct_ex_imod_helper);
}

/* Visit an OP_IAND instruction. */
static INLINE bool
rt_visit_iand_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IAND");
        BINARY_OP(noct_ex_iand_helper);
}

/* Visit an OP_IOR instruction. */
static INLINE bool
rt_visit_ior_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IOR");
        BINARY_OP(noct_ex_ior_helper);
}

/* Visit an OP_IXOR instruction. */
static INLINE bool
rt_visit_ixor_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IXOR");
        BINARY_OP(noct_ex_ixor_helper);
}

/* Visit an OP_ISHL instruction. */
static INLINE bool
rt_visit_ishl_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        int src;
        int imm;

        DEBUG_TRACE(*pc, "ISHL");

        GET_TMPVAR(&dst);
        GET_TMPVAR(&src);
        GET_U8(&imm);

        if (!noct_ex_ishl_helper(env, dst, src, imm))
                return false;

        return true;
}

/* Visit an OP_ISHR instruction. */
static INLINE bool
rt_visit_ishr_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        int src;
        int imm;

        DEBUG_TRACE(*pc, "ISHR");

        GET_TMPVAR(&dst);
        GET_TMPVAR(&src);
        GET_U8(&imm);

        if (!noct_ex_ishr_helper(env, dst, src, imm))
                return false;

        return true;
}

/* Visit an OP_ILT instruction. */
static INLINE bool
rt_visit_ilt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "ILT");
        BINARY_OP(noct_ex_ilt_helper);
}

/* Visit an OP_ILTE instruction. */
static INLINE bool
rt_visit_ilte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "ILTE");
        BINARY_OP(noct_ex_ilte_helper);
}

/* Visit an OP_IGT instruction. */
static INLINE bool
rt_visit_igt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IGT");
        BINARY_OP(noct_ex_igt_helper);
}

/* Visit an OP_IGTE instruction. */
static INLINE bool
rt_visit_igte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IGTE");
        BINARY_OP(noct_ex_igte_helper);
}

/* Visit an OP_FADD instruction. */
static INLINE bool
rt_visit_fadd_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FADD");
        BINARY_OP(noct_ex_fadd_helper);
}

/* Visit an OP_FSUB instruction. */
static INLINE bool
rt_visit_fsub_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FSUB");
        BINARY_OP(noct_ex_fsub_helper);
}

/* Visit an OP_FMUL instruction. */
static INLINE bool
rt_visit_fmul_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FMUL");
        BINARY_OP(noct_ex_fmul_helper);
}

/* Visit an OP_FDIV instruction. */
static INLINE bool
rt_visit_fdiv_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FDIV");
        BINARY_OP(noct_ex_fdiv_helper);
}

/* Visit an OP_FLT instruction. */
static INLINE bool
rt_visit_flt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FLT");
        BINARY_OP(noct_ex_flt_helper);
}

/* Visit an OP_FLTE instruction. */
static INLINE bool
rt_visit_flte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FLTE");
        BINARY_OP(noct_ex_flte_helper);
}

/* Visit an OP_FGT instruction. */
static INLINE bool
rt_visit_fgt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FGT");
        BINARY_OP(noct_ex_fgt_helper);
}

/* Visit an OP_FGTE instruction. */
static INLINE bool
rt_visit_fgte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "FGTE");
        BINARY_OP(noct_ex_fgte_helper);
}

/* Visit an OP_IDIV_CHECKED instruction. */
static INLINE bool
rt_visit_idiv_checked_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IDIV_CHECKED");
        BINARY_OP(noct_ex_idiv_helper);
}

/* Visit an OP_IMOD_CHECKED instruction. */
static INLINE bool
rt_visit_imod_checked_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "IMOD_CHECKED");
        BINARY_OP(noct_ex_imod_helper);
}

/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
rt_visit_vloadi32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int base;
        int ofs;

        DEBUG_TRACE(*pc, "VLOADI32X4");

        GET_U8(&vd);
        GET_TMPVAR(&base);
        GET_TMPVAR(&ofs);

        if (!noct_ex_vloadi32x4_helper(env, vd, base, ofs))
                return false;

        return true;
}

/* Visit an OP_VSTOREI32X4 instruction. */
static INLINE bool
rt_visit_vstorei32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int base;
        int ofs;
        int vs;

        DEBUG_TRACE(*pc, "VSTOREI32X4");

        GET_TMPVAR(&base);
        GET_TMPVAR(&ofs);
        GET_U8(&vs);

        if (!noct_ex_vstorei32x4_helper(env, base, ofs, vs))
                return false;

        return true;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
rt_visit_vsplati32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int src;

        DEBUG_TRACE(*pc, "VSPLATI32");

        GET_U8(&vd);
        GET_TMPVAR(&src);

        if (!noct_ex_vsplati32_helper(env, vd, src, 0))
                return false;

        return true;
}

/* Visit an OP_VGETLANEI32 instruction. */
static INLINE bool
rt_visit_vgetlanei32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        int vs;
        int lane;

        DEBUG_TRACE(*pc, "VGETLANEI32");

        GET_TMPVAR(&dst);
        GET_U8(&vs);
        GET_U8(&lane);

        if (!noct_ex_vgetlanei32_helper(env, dst, vs, lane))
                return false;

        return true;
}

/* Visit an OP_VMOV128 instruction. */
static INLINE bool
rt_visit_vmov128_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int vs;

        DEBUG_TRACE(*pc, "VMOV128");

        GET_U8(&vd);
        GET_U8(&vs);

        if (!noct_ex_vmov128_helper(env, vd, vs, 0))
                return false;

        return true;
}

/* Visit an OP_VADDI32X4 instruction. */
static INLINE bool
rt_visit_vaddi32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VADDI32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vaddi32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VSUBI32X4 instruction. */
static INLINE bool
rt_visit_vsubi32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VSUBI32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vsubi32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VMULI32X4 instruction. */
static INLINE bool
rt_visit_vmuli32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VMULI32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vmuli32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VAND128 instruction. */
static INLINE bool
rt_visit_vand128_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VAND128");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vand128_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VOR128 instruction. */
static INLINE bool
rt_visit_vor128_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VOR128");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vor128_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VXOR128 instruction. */
static INLINE bool
rt_visit_vxor128_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VXOR128");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vxor128_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VSHLI32X4 instruction. */
static INLINE bool
rt_visit_vshli32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int count;

        DEBUG_TRACE(*pc, "VSHLI32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&count);

        if (!noct_ex_vshli32x4_helper(env, vd, va, count))
                return false;

        return true;
}

/* Visit an OP_VSHRI32X4 instruction. */
static INLINE bool
rt_visit_vshri32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int count;

        DEBUG_TRACE(*pc, "VSHRI32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&count);

        if (!noct_ex_vshri32x4_helper(env, vd, va, count))
                return false;

        return true;
}

/* Visit an OP_VLOADF32X4 instruction. */
static INLINE bool
rt_visit_vloadf32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int base;
        int ofs;

        DEBUG_TRACE(*pc, "VLOADF32X4");

        GET_U8(&vd);
        GET_TMPVAR(&base);
        GET_TMPVAR(&ofs);

        if (!noct_ex_vloadf32x4_helper(env, vd, base, ofs))
                return false;

        return true;
}

/* Visit an OP_VSTOREF32X4 instruction. */
static INLINE bool
rt_visit_vstoref32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int base;
        int ofs;
        int vs;

        DEBUG_TRACE(*pc, "VSTOREF32X4");

        GET_TMPVAR(&base);
        GET_TMPVAR(&ofs);
        GET_U8(&vs);

        if (!noct_ex_vstoref32x4_helper(env, base, ofs, vs))
                return false;

        return true;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
rt_visit_vsplatf32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int src;

        DEBUG_TRACE(*pc, "VSPLATF32");

        GET_U8(&vd);
        GET_TMPVAR(&src);

        if (!noct_ex_vsplatf32_helper(env, vd, src, 0))
                return false;

        return true;
}

/* Visit an OP_VGETLANEF32 instruction. */
static INLINE bool
rt_visit_vgetlanef32_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int dst;
        int vs;
        int lane;

        DEBUG_TRACE(*pc, "VGETLANEF32");

        GET_TMPVAR(&dst);
        GET_U8(&vs);
        GET_U8(&lane);

        if (!noct_ex_vgetlanef32_helper(env, dst, vs, lane))
                return false;

        return true;
}

/* Visit an OP_VADDF32X4 instruction. */
static INLINE bool
rt_visit_vaddf32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VADDF32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vaddf32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VSUBF32X4 instruction. */
static INLINE bool
rt_visit_vsubf32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VSUBF32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vsubf32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VMULF32X4 instruction. */
static INLINE bool
rt_visit_vmulf32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VMULF32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vmulf32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VDIVF32X4 instruction. */
static INLINE bool
rt_visit_vdivf32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int va;
        int vb;

        DEBUG_TRACE(*pc, "VDIVF32X4");

        GET_U8(&vd);
        GET_U8(&va);
        GET_U8(&vb);

        if (!noct_ex_vdivf32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
rt_visit_vcvti32f32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int vs;

        DEBUG_TRACE(*pc, "VCVTI32F32X4");

        GET_U8(&vd);
        GET_U8(&vs);

        if (!noct_ex_vcvti32f32x4_helper(env, vd, vs, 0))
                return false;

        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
rt_visit_vcvtf32i32x4_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int vd;
        int vs;

        DEBUG_TRACE(*pc, "VCVTF32I32X4");

        GET_U8(&vd);
        GET_U8(&vs);

        if (!noct_ex_vcvtf32i32x4_helper(env, vd, vs, 0))
                return false;

        return true;
}

static INLINE bool
rt_visit_vindex_hint_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int index_tmp, stop_tmp, remaining_tmp;
	int index_id, lanes, flags;

	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(index_tmp);
	UNUSED_PARAMETER(stop_tmp);
	UNUSED_PARAMETER(remaining_tmp);
	UNUSED_PARAMETER(index_id);
	UNUSED_PARAMETER(lanes);
	UNUSED_PARAMETER(flags);

	GET_TMPVAR(&index_tmp);
	GET_TMPVAR(&stop_tmp);
	GET_TMPVAR(&remaining_tmp);
	GET_U8(&index_id);
	GET_U8(&lanes);
	GET_U8(&flags);

	return true;
}

static INLINE bool
rt_visit_ploop_hint_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int index_tmp, stop_tmp, remaining_tmp;
	int lanes, flags;

	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(index_tmp);
	UNUSED_PARAMETER(stop_tmp);
	UNUSED_PARAMETER(remaining_tmp);
	UNUSED_PARAMETER(lanes);
	UNUSED_PARAMETER(flags);

	GET_TMPVAR(&index_tmp);
	GET_TMPVAR(&stop_tmp);
	GET_TMPVAR(&remaining_tmp);
	GET_U8(&lanes);
	GET_U8(&flags);

	return true;
}

static INLINE bool
rt_visit_tmpvar_type_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int tmp;
	int type;

	UNUSED_PARAMETER(tmp);

	GET_TMPVAR(&tmp);
	GET_U8(&type);

	type &= ~TMPVAR_TYPE_COMPILER_TEMP;

	if (type != TMPVAR_TYPE_DYNAMIC &&
	    type != NOCT_VALUE_INT && type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT && type != NOCT_VALUE_DOUBLE) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	return true;
}

static INLINE bool
rt_visit_materialize_type_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int tmp;
	int type;

	UNUSED_PARAMETER(tmp);

	GET_TMPVAR(&tmp);
	GET_U8(&type);

	if (type != NOCT_VALUE_INT &&
            type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT &&
            type != NOCT_VALUE_DOUBLE) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	return true;
}

static INLINE bool
rt_visit_subjnz_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int value, decrement;
	uint32_t target;
	struct rt_value *v;

	GET_TMPVAR(&value);
	GET_U8(&decrement);
	GET_U32(&target);

	if (target >= func->bytecode_size) {
		rt_error(env, N_TR("Broken bytecode."));
		return false;
	}

	v = &env->frame->tmpvar[value];

	if (v->type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("SUBJNZ operand is not an integer."));
		return false;
	}

	v->val.i -= decrement;

	if (v->val.i != 0)
		*pc = target;

	return true;
}

static INLINE bool
rt_visit_vori32x4i_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int vd, vs, imm, shift;

	UNUSED_PARAMETER(func);

	GET_U8(&vd);
	GET_U8(&vs);
	GET_U8(&imm);
	GET_U8(&shift);

	if (!noct_ex_vori32x4i_helper(env, vd, vs, (imm << 8) | shift))
                return false;

        return true;
}

static INLINE bool
rt_visit_vfmaf32x4_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int vd, va, vb, vc;

	UNUSED_PARAMETER(func);

	GET_U8(&vd);
	GET_U8(&va);
	GET_U8(&vb);
	GET_U8(&vc);

	if (!noct_ex_vfmaf32x4_helper(env, vd, va, (vb << 8) | vc))
                return false;

        return true;
}

static INLINE bool
rt_visit_vcmpi32x4_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int vd;
	int va;
	int vb;
	int pred;

	GET_U8(&vd);
	GET_U8(&va);
	GET_U8(&vb);
	GET_U8(&pred);

	if (vd >= 16 || va >= 16 || vb >= 16 || pred >= VCMP_PREDICATE_COUNT) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vcmpi32x4_helper(env, vd, va, (vb << 8) | pred))
                return false;

        return true;
}

static INLINE bool
rt_visit_vcmpf32x4_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int vd;
	int va;
	int vb;
	int pred;

	GET_U8(&vd);
	GET_U8(&va);
	GET_U8(&vb);
	GET_U8(&pred);

	if (vd >= 16 || va >= 16 || vb >= 16 || pred >= VCMP_PREDICATE_COUNT) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vcmpf32x4_helper(env, vd, va, (vb << 8) | pred))
                return false;

        return true;
}

static INLINE bool
rt_visit_vselect128_op(struct rt_env *env, struct rt_func *func, uint32_t *pc)
{
	int vd, vm, vt, vf;

	GET_U8(&vd); GET_U8(&vm); GET_U8(&vt); GET_U8(&vf);

	if (vd >= 16 || vm >= 16 || vt >= 16 || vf >= 16) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vselect128_helper(env, vd, vm, (vt << 8) | vf))
                return false;

        return true;
}

static INLINE bool
rt_visit_vmins32x4_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int vd;
	int va;
	int vb;

	GET_U8(&vd);
	GET_U8(&va);
	GET_U8(&vb);

	if (vd >= 16 || va >= 16 || vb >= 16) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vmins32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

static INLINE bool
rt_visit_vmaxs32x4_op(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t *pc)
{
	int vd;
	int va;
	int vb;

	GET_U8(&vd);
	GET_U8(&va);
	GET_U8(&vb);

	if (vd >= 16 || va >= 16 || vb >= 16) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vmaxs32x4_helper(env, vd, va, vb))
                return false;

        return true;
}

static INLINE bool
rt_visit_vmaskstorei32x4_op(struct rt_env *env, struct rt_func *func,
			    uint32_t *pc)
{
	int base, ofs, vs, vm;

	GET_TMPVAR(&base);
        GET_TMPVAR(&ofs);
        GET_U8(&vs);
        GET_U8(&vm);

	if (vs >= 16 || vm >= 16) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vmaskstorei32x4_helper(env, base, ofs, (vs << 8) | vm))
                return false;

        return true;
}

static INLINE bool
rt_visit_vinductf32x4_op(struct rt_env *env, struct rt_func *func,
			 uint32_t *pc)
{
	int vd, state, step;

	GET_U8(&vd); GET_TMPVAR(&state); GET_TMPVAR(&step);

	if (vd >= 16) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vinductf32x4_helper(env, vd, state, step))
                return false;
}

static INLINE bool
rt_visit_vgatheri32x4_checked_op(struct rt_env *env, struct rt_func *func,
				 uint32_t *pc)
{
	int vd, base, plen, vi;

	GET_U8(&vd);
        GET_TMPVAR(&base);
        GET_TMPVAR(&plen);
        GET_U8(&vi);

	if (vd >= 16 || vi >= 16) {
		rt_error(env, BROKEN_BYTECODE);
		return false;
	}

	if (!noct_ex_vgatheri32x4_checked_helper(env, vd, base, (plen << 8) | vi))
                return false;

        return true;
}

/* Visit an instruction. */
static bool
rt_visit_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int op;

        GET_U8(&op);

        switch (op) {
        case OP_NOP:
                /* NOP */
                break;
        case OP_LINEINFO:
                if (!rt_visit_lineinfo_op(env, func, pc))
                        return false;
                break;
        case OP_ASSIGN:
                if (!rt_visit_assign_op(env, func, pc))
                        return false;
                break;
        case OP_ICONST:
                if (!rt_visit_iconst_op(env, func, pc))
                        return false;
                break;
        case OP_LICONST:
                if (!rt_visit_liconst_op(env, func, pc))
                        return false;
                break;
        case OP_FCONST:
                if (!rt_visit_fconst_op(env, func, pc))
                        return false;
                break;
        case OP_LFCONST:
                if (!rt_visit_lfconst_op(env, func, pc))
                        return false;
                break;
        case OP_SCONST:
                if (!rt_visit_sconst_op(env, func, pc))
                        return false;
                break;
        case OP_ACONST:
                if (!rt_visit_aconst_op(env, func, pc))
                        return false;
                break;
        case OP_DCONST:
                if (!rt_visit_dconst_op(env, func, pc))
                        return false;
                break;
        case OP_INC:
                if (!rt_visit_inc_op(env, func, pc))
                        return false;
                break;
        case OP_ADD:
                if (!rt_visit_add_op(env, func, pc))
                        return false;
                break;
        case OP_SUB:
                if (!rt_visit_sub_op(env, func, pc))
                        return false;
                break;
        case OP_MUL:
                if (!rt_visit_mul_op(env, func, pc))
                        return false;
                break;
        case OP_DIV:
                if (!rt_visit_div_op(env, func, pc))
                        return false;
                break;
        case OP_MOD:
                if (!rt_visit_mod_op(env, func, pc))
                        return false;
                break;
        case OP_AND:
                if (!rt_visit_and_op(env, func, pc))
                        return false;
                break;
        case OP_OR:
                if (!rt_visit_or_op(env, func, pc))
                        return false;
                break;
        case OP_XOR:
                if (!rt_visit_xor_op(env, func, pc))
                        return false;
                break;
        case OP_SHL:
                if (!rt_visit_shl_op(env, func, pc))
                        return false;
                break;
        case OP_SHR:
                if (!rt_visit_shr_op(env, func, pc))
                        return false;
                break;
        case OP_NEG:
                if (!rt_visit_neg_op(env, func, pc))
                        return false;
                break;
        case OP_NOT:
                if (!rt_visit_not_op(env, func, pc))
                        return false;
                break;
        case OP_LT:
                if (!rt_visit_lt_op(env, func, pc))
                        return false;
                break;
        case OP_LTE:
                if (!rt_visit_lte_op(env, func, pc))
                        return false;
                break;
        case OP_GT:
                if (!rt_visit_gt_op(env, func, pc))
                        return false;
                break;
        case OP_GTE:
                if (!rt_visit_gte_op(env, func, pc))
                        return false;
                break;
        case OP_EQ:
                if (!rt_visit_eq_op(env, func, pc))
                        return false;
                break;
        case OP_EQI:
                /* Same as EQ. EQI is an optimization hint for JIT-compiler. */
                if (!rt_visit_eq_op(env, func, pc))
                        return false;
                break;
        case OP_NEQ:
                if (!rt_visit_neq_op(env, func, pc))
                        return false;
                break;
        case OP_STOREARRAY:
                if (!rt_visit_storearray_op(env, func, pc))
                        return false;
                break;
        case OP_LOADARRAY:
                if (!rt_visit_loadarray_op(env, func, pc))
                        return false;
                break;
        case OP_LEN:
                if (!rt_visit_len_op(env, func, pc))
                        return false;
                break;
        case OP_GETDICTKEYBYINDEX:
                if (!rt_visit_getdictkeybyindex_op(env, func, pc))
                        return false;
                break;
        case OP_GETDICTVALBYINDEX:
                if (!rt_visit_getdictvalbyindex_op(env, func, pc))
                        return false;
                break;
        case OP_LOADSYMBOL:
                if (!rt_visit_loadsymbol_op(env, func, pc))
                        return false;
                break;
        case OP_STORESYMBOL:
                if (!rt_visit_storesymbol_op(env, func, pc))
                        return false;
                break;
        case OP_LOADDOT:
                if (!rt_visit_loaddot_op(env, func, pc))
                        return false;
                break;
        case OP_STOREDOT:
                if (!rt_visit_storedot_op(env, func, pc))
                        return false;
                break;
        case OP_CALL:
                if (!rt_visit_call_op(env, func, pc))
                        return false;
                break;
        case OP_THISCALL:
                if (!rt_visit_thiscall_op(env, func, pc))
                        return false;
                break;
        case OP_JMP:
                if (!rt_visit_jmp_op(env, func, pc))
                        return false;
                break;
        case OP_JMPIFTRUE:
                if (!rt_visit_jmpiftrue_op(env, func, pc))
                        return false;
                break;
        case OP_JMPIFFALSE:
                if (!rt_visit_jmpiffalse_op(env, func, pc))
                        return false;
                break;
        case OP_JMPIFEQ:
                /* Same as JMPIFTRUE. (JMPIFEQ is an optimization hint for JIT-compiler.) */
                if (!rt_visit_jmpiftrue_op(env, func, pc))
                        return false;
                break;
        case OP_SAFEPOINT:
#if defined(NOCT_USE_MULTITHREAD)
                if (!rt_visit_safepoint_op(env, func, pc))
                        return false;
#endif
                break;
        case OP_PBASE:
                if (!rt_visit_pbase_op(env, func, pc))
                        return false;
                break;
        case OP_PLEN:
                if (!rt_visit_plen_op(env, func, pc))
                        return false;
                break;
        case OP_PCHECK:
                if (!rt_visit_pcheck_op(env, func, pc))
                        return false;
                break;
        case OP_TYPEIS:
                if (!rt_visit_typeis_op(env, func, pc))
                        return false;
                break;
        case OP_PLOAD8U:
                if (!rt_visit_pload8u_op(env, func, pc))
                        return false;
                break;
        case OP_PSTORE8:
                if (!rt_visit_pstore8_op(env, func, pc))
                        return false;
                break;
        case OP_CHECKTYPE:
                if (!rt_visit_checktype_op(env, func, pc))
                        return false;
                break;
        case OP_PLOAD8S:
                if (!rt_visit_pload8s_op(env, func, pc))
                        return false;
                break;
        case OP_PLOAD16U:
                if (!rt_visit_pload16u_op(env, func, pc))
                        return false;
                break;
        case OP_PLOAD16S:
                if (!rt_visit_pload16s_op(env, func, pc))
                        return false;
                break;
        case OP_PLOAD32:
                if (!rt_visit_pload32_op(env, func, pc))
                        return false;
                break;
        case OP_PLOAD64:
                if (!rt_visit_pload64_op(env, func, pc))
                        return false;
                break;
        case OP_PSTORE16:
                if (!rt_visit_pstore16_op(env, func, pc))
                        return false;
                break;
        case OP_PSTORE32:
                if (!rt_visit_pstore32_op(env, func, pc))
                        return false;
                break;
        case OP_PSTORE64:
                if (!rt_visit_pstore64_op(env, func, pc))
                        return false;
                break;
        case OP_PLOADF32:
                if (!rt_visit_ploadf32_op(env, func, pc))
                        return false;
                break;
        case OP_PSTOREF32:
                if (!rt_visit_pstoref32_op(env, func, pc))
                        return false;
                break;
        case OP_IADD:
                if (!rt_visit_iadd_op(env, func, pc))
                        return false;
                break;
        case OP_ISUB:
                if (!rt_visit_isub_op(env, func, pc))
                        return false;
                break;
        case OP_IMUL:
                if (!rt_visit_imul_op(env, func, pc))
                        return false;
                break;
        case OP_IDIV:
                if (!rt_visit_idiv_op(env, func, pc))
                        return false;
                break;
        case OP_IMOD:
                if (!rt_visit_imod_op(env, func, pc))
                        return false;
                break;
        case OP_IAND:
                if (!rt_visit_iand_op(env, func, pc))
                        return false;
                break;
        case OP_IOR:
                if (!rt_visit_ior_op(env, func, pc))
                        return false;
                break;
        case OP_IXOR:
                if (!rt_visit_ixor_op(env, func, pc))
                        return false;
                break;
        case OP_ISHL:
                if (!rt_visit_ishl_op(env, func, pc))
                        return false;
                break;
        case OP_ISHR:
                if (!rt_visit_ishr_op(env, func, pc))
                        return false;
                break;
        case OP_ILT:
                if (!rt_visit_ilt_op(env, func, pc))
                        return false;
                break;
        case OP_ILTE:
                if (!rt_visit_ilte_op(env, func, pc))
                        return false;
                break;
        case OP_IGT:
                if (!rt_visit_igt_op(env, func, pc))
                        return false;
                break;
        case OP_IGTE:
                if (!rt_visit_igte_op(env, func, pc))
                        return false;
                break;
        case OP_FADD:
                if (!rt_visit_fadd_op(env, func, pc))
                        return false;
                break;
        case OP_FSUB:
                if (!rt_visit_fsub_op(env, func, pc))
                        return false;
                break;
        case OP_FMUL:
                if (!rt_visit_fmul_op(env, func, pc))
                        return false;
                break;
        case OP_FDIV:
                if (!rt_visit_fdiv_op(env, func, pc))
                        return false;
                break;
        case OP_FLT:
                if (!rt_visit_flt_op(env, func, pc))
                        return false;
                break;
        case OP_FLTE:
                if (!rt_visit_flte_op(env, func, pc))
                        return false;
                break;
        case OP_FGT:
                if (!rt_visit_fgt_op(env, func, pc))
                        return false;
                break;
        case OP_FGTE:
                if (!rt_visit_fgte_op(env, func, pc))
                        return false;
                break;
        case OP_IDIV_CHECKED:
                if (!rt_visit_idiv_checked_op(env, func, pc))
                        return false;
                break;
        case OP_IMOD_CHECKED:
                if (!rt_visit_imod_checked_op(env, func, pc))
                        return false;
                break;
        case OP_VLOADI32X4:
                if (!rt_visit_vloadi32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSTOREI32X4:
                if (!rt_visit_vstorei32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSPLATI32:
                if (!rt_visit_vsplati32_op(env, func, pc))
                        return false;
                break;
        case OP_VGETLANEI32:
                if (!rt_visit_vgetlanei32_op(env, func, pc))
                        return false;
                break;
        case OP_VMOV128:
                if (!rt_visit_vmov128_op(env, func, pc))
                        return false;
                break;
        case OP_VADDI32X4:
                if (!rt_visit_vaddi32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSUBI32X4:
                if (!rt_visit_vsubi32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VMULI32X4:
                if (!rt_visit_vmuli32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VAND128:
                if (!rt_visit_vand128_op(env, func, pc))
                        return false;
                break;
        case OP_VOR128:
                if (!rt_visit_vor128_op(env, func, pc))
                        return false;
                break;
        case OP_VXOR128:
                if (!rt_visit_vxor128_op(env, func, pc))
                        return false;
                break;
        case OP_VSHLI32X4:
                if (!rt_visit_vshli32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSHRI32X4:
                if (!rt_visit_vshri32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VLOADF32X4:
                if (!rt_visit_vloadf32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSTOREF32X4:
                if (!rt_visit_vstoref32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSPLATF32:
                if (!rt_visit_vsplatf32_op(env, func, pc))
                        return false;
                break;
        case OP_VGETLANEF32:
                if (!rt_visit_vgetlanef32_op(env, func, pc))
                        return false;
                break;
        case OP_VADDF32X4:
                if (!rt_visit_vaddf32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VSUBF32X4:
                if (!rt_visit_vsubf32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VMULF32X4:
                if (!rt_visit_vmulf32x4_op(env, func, pc))
                        return false;
                break;
        case OP_VDIVF32X4:
                if (!rt_visit_vdivf32x4_op(env, func, pc))
                        return false;
                break;
	case OP_VCVTI32F32X4:
                if (!rt_visit_vcvti32f32x4_op(env, func, pc))
                        return false;
                break;
	case OP_VCVTF32I32X4:
                if (!rt_visit_vcvtf32i32x4_op(env, func, pc))
                        return false;
                break;
	case OP_VINDEX_HINT:
		if (!rt_visit_vindex_hint_op(env, func, pc))
			return false;
		break;
	case OP_PLOOP_HINT:
		if (!rt_visit_ploop_hint_op(env, func, pc))
			return false;
		break;
	case OP_TMPVAR_TYPE:
		if (!rt_visit_tmpvar_type_op(env, func, pc))
			return false;
		break;
	case OP_MATERIALIZE_TYPE:
		if (!rt_visit_materialize_type_op(env, func, pc))
			return false;
		break;
	case OP_SUBJNZ:
		if (!rt_visit_subjnz_op(env, func, pc))
			return false;
		break;
	case OP_VORI32X4I:
		if (!rt_visit_vori32x4i_op(env, func, pc))
			return false;
		break;
	case OP_VFMAF32X4:
		if (!rt_visit_vfmaf32x4_op(env, func, pc))
			return false;
		break;
	case OP_VCMPI32X4:
		if (!rt_visit_vcmpi32x4_op(env, func, pc))
			return false;
		break;
	case OP_VCMPF32X4:
		if (!rt_visit_vcmpf32x4_op(env, func, pc))
			return false;
		break;
	case OP_VSELECT128:
		if (!rt_visit_vselect128_op(env, func, pc))
			return false;
		break;
	case OP_VMASKSTOREI32X4:
		if (!rt_visit_vmaskstorei32x4_op(env, func, pc))
			return false;
		break;
	case OP_VINDUCTF32X4:
		if (!rt_visit_vinductf32x4_op(env, func, pc))
			return false;
		break;
	case OP_VGATHERI32X4_CHECKED:
		if (!rt_visit_vgatheri32x4_checked_op(env, func, pc))
			return false;
		break;
	case OP_VMINS32X4:
		if (!rt_visit_vmins32x4_op(env, func, pc))
			return false;
		break;
	case OP_VMAXS32X4:
		if (!rt_visit_vmaxs32x4_op(env, func, pc))
			return false;
		break;
        default:
                rt_error(env, N_TR("Unknown opcode %d at pc=%d."), func->bytecode[*pc], *pc);
                return false;
        }

        return true;
}
