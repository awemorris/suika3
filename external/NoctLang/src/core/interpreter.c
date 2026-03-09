/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Bytecode Interpreter
 */

#include "runtime.h"
#include "interpreter.h"
#include "execution.h"
#include "bytecode.h"

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

        *val = func->bytecode[*pc];     

        *pc = *pc + 1;

        return true;
}

#define GET_U16(v) if (!rt_get_u16(env, func, pc, v)) return false
static INLINE bool rt_get_u16(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc,
        int *val)
{
        if (*pc + 2 > func->bytecode_size) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        *val = ((uint32_t)func->bytecode[*pc] << 8) |
                (uint32_t)func->bytecode[*pc + 1];

        *pc = *pc + 2;

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

        *val = ((uint32_t)func->bytecode[*pc] << 8) |
                (uint32_t)func->bytecode[*pc + 1];
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

        DEBUG_TRACE(*pc, "INC");

        GET_TMPVAR(&dst);

        val = &env->frame->tmpvar[dst];
        if (val->type != NOCT_VALUE_INT) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }
        val->val.i++;

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

        BINARY_OP(rt_add_helper);
}

/* Visit a OP_SUB instruction. */
static INLINE bool
rt_visit_sub_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "SUB");

        BINARY_OP(rt_sub_helper);
}

/* Visit a OP_MUL instruction. */
static INLINE bool
rt_visit_mul_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "MUL");

        BINARY_OP(rt_mul_helper);
}

/* Visit a OP_DIV instruction. */
static INLINE bool
rt_visit_div_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "DIV");

        BINARY_OP(rt_div_helper);
}

/* Visit a OP_MOD instruction. */
static INLINE bool
rt_visit_mod_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "MOD");

        BINARY_OP(rt_mod_helper);
}

/* Visit a OP_AND instruction. */
static INLINE bool
rt_visit_and_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "AND");

        BINARY_OP(rt_and_helper);
}

/* Visit a OP_OR instruction. */
static INLINE bool
rt_visit_or_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "OR");

        BINARY_OP(rt_or_helper);
}

/* Visit a OP_XOR instruction. */
static INLINE bool
rt_visit_xor_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "XOR");

        BINARY_OP(rt_xor_helper);
}

/* Visit a OP_SHL instruction. */
static INLINE bool
rt_visit_shl_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "SHL");

        BINARY_OP(rt_shl_helper);
}

/* Visit a OP_SHR instruction. */
static INLINE bool
rt_visit_shr_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "SHR");

        BINARY_OP(rt_shr_helper);
}

/* Visit a OP_NEG instruction. */
static INLINE bool
rt_visit_neg_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "NEG");

        UNARY_OP(rt_neg_helper);
}

/* Visit a OP_NOT instruction. */
static INLINE bool
rt_visit_not_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "NOT");

        UNARY_OP(rt_not_helper);
}

/* Visit a OP_LT instruction. */
static INLINE bool
rt_visit_lt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LT");

        BINARY_OP(rt_lt_helper);
}

/* Visit a OP_LTE instruction. */
static INLINE bool
rt_visit_lte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LTE");

        BINARY_OP(rt_lte_helper);
}

/* Visit a OP_GT instruction. */
static INLINE bool
rt_visit_gt_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GT");

        BINARY_OP(rt_gt_helper);
}

/* Visit a OP_GTE instruction. */
static INLINE bool
rt_visit_gte_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GTE");

        BINARY_OP(rt_gte_helper);
}

/* Visit a OP_EQ instruction. */
static INLINE bool
rt_visit_eq_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "EQ");

        BINARY_OP(rt_eq_helper);
}

/* Visit a OP_NEQ instruction. */
static INLINE bool
rt_visit_neq_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "NEQ");

        BINARY_OP(rt_neq_helper);
}

/* Visit a OP_STOREARRAY instruction. */
static INLINE bool
rt_visit_storearray_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "STOREARRAY");

        BINARY_OP(rt_storearray_helper);
}

/* Visit a OP_LOADARRAY instruction. */
static INLINE bool
rt_visit_loadarray_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LOADARRAY");

        BINARY_OP(rt_loadarray_helper);
}

/* Visit a OP_LEN instruction. */
static INLINE bool
rt_visit_len_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "LEN");

        UNARY_OP(rt_len_helper);
}

/* Visit a OP_GETDICTKEYBYINDEX instruction. */
static INLINE bool
rt_visit_getdictkeybyindex_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GETDICTKEYBYINDEX");

        BINARY_OP(rt_getdictkeybyindex_helper);
}

/* Visit a OP_GETDICTVALBYINDEX instruction. */
static INLINE bool
rt_visit_getdictvalbyindex_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        DEBUG_TRACE(*pc, "GETDICTVALBYINDEX");

        BINARY_OP(rt_getdictvalbyindex_helper);
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

        if (!rt_loadsymbol_helper(env, dst, s, len, hash))
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

        if (!rt_storesymbol_helper(env, s, len, hash, src))
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

        if (!rt_loaddot_helper(env, dst, dict, field, len, hash))
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

        if (!rt_storedot_helper(env, dict, field, len, hash, src))
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

        if (!rt_call_helper(env, dst_tmpvar, func_tmpvar, arg_count, arg))
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
        const char *name;
        uint32_t len, hash;
        int arg_count;
        int arg_tmpvar;
        int arg[NOCT_ARG_MAX];
        int i;

        DEBUG_TRACE(*pc, "THISCALL");

        GET_TMPVAR(&dst_tmpvar);
        GET_TMPVAR(&obj_tmpvar);
        GET_STRING(&name, &len, &hash);
        GET_U8(&arg_count);
        for (i = 0; i < arg_count; i++) {
                GET_TMPVAR(&arg_tmpvar);
                arg[i] = arg_tmpvar;
        }

        if (!rt_thiscall_helper(env, dst_tmpvar, obj_tmpvar, name, len, hash, arg_count, arg))
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
        *pc = (int)target;

        return true;
}

/* Visit a OP_JMPIFTRUE instruction. */
static INLINE bool
rt_visit_jmpiftrue_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int src;
        uint32_t target;

        DEBUG_TRACE(*pc, "JMPIFTRUE");

        GET_TMPVAR(&src);
        GET_ADDR(&target);

        /* Assume src is an int. */
        if (env->frame->tmpvar[src].type != NOCT_VALUE_INT) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        /* Jump. */
        if (env->frame->tmpvar[src].val.i == 1)
                *pc = (int)target;

        return true;
}

/* Visit a OP_JMPIFFALSE instruction. */
static INLINE bool
rt_visit_jmpiffalse_op(
        struct rt_env *env,
        struct rt_func *func,
        uint32_t *pc)
{
        int src;
        uint32_t target;

        DEBUG_TRACE(*pc, "JMPIFFALSE");

        GET_TMPVAR(&src);
        GET_ADDR(&target);

        /* Assume src is an int. */
        if (env->frame->tmpvar[src].type != NOCT_VALUE_INT) {
                rt_error(env, BROKEN_BYTECODE);
                return false;
        }

        /* Jump. */
        if (env->frame->tmpvar[src].val.i == 0)
                *pc = (int)target;

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
        case OP_FCONST:
                if (!rt_visit_fconst_op(env, func, pc))
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
        default:
                rt_error(env, "Unknown opcode %d at pc=%d.", func->bytecode[*pc], *pc);
                return false;
        }

        return true;
}
