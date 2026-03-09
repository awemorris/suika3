/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Bytecode
 */

#ifndef NOCT_BYTECODE_H
#define NOCT_BYTECODE_H

enum bytecode {
	OP_NOP,			/* 0x00: nop */

	/* tmpvar assignment */
	OP_ASSIGN,		/* 0x01: dst = src */
	OP_ICONST,		/* 0x02: dst = integer constant */
	OP_FCONST,		/* 0x03: dst = floating-point constant */
	OP_SCONST,		/* 0x04: dst = string constant */
	OP_ACONST,		/* 0x05: dst = empty array */
	OP_DCONST,		/* 0x06: dst = empty dictionary */

	/* tmpvar calc (dst = op src1) */
	OP_INC,			/* 0x07: dst = src + 1, assume operands are integers */
	OP_NEG,			/* 0x08: dst = -src */
	OP_NOT,			/* 0x09: dst = !src */

	/* tmpvar calc (dst = src1 op src2) */
	OP_ADD,			/* 0x0a: dst = src1 + src2 */
	OP_SUB,			/* 0x0b: dst = src1 - src2 */
	OP_MUL,			/* 0x0c: dst = src1 * src2 */
	OP_DIV,			/* 0x0d: dst = src1 / src2 */
	OP_MOD,			/* 0x0e: dst = src1 % src2 */
	OP_AND,			/* 0x0f: dst = src1 & src2 */
	OP_OR,			/* 0x10: dst = src1 | src2 */
	OP_XOR,			/* 0x11: dst = src1 ^ src2 */
	OP_SHL,			/* 0x11: dst = src1 << src2 */
	OP_SHR,			/* 0x11: dst = src1 >> src2 */
	OP_LT,			/* 0x12: dst = src1 <  src2 [0 or 1] */
	OP_LTE,			/* 0x13: dst = src1 <= src2 [0 or 1] */
	OP_GT,			/* 0x14: dst = src1 >  src2 [0 or 1] */
	OP_GTE,			/* 0x15: dst = src1 >= src2 [0 or 1] */
	OP_EQ,			/* 0x16: dst = src1 == src2 [0 or 1] */
	OP_NEQ,			/* 0x17: dst = src1 != src2 [0 or 1] */
	OP_EQI,			/* 0x18: dst = src1 == src2 [0 or 1], assume operands are integers */

	/* array/dictionary */
	OP_LOADARRAY,		/* 0x19: dst = src1[src2] */
	OP_STOREARRAY,		/* 0x1a: opr1[opr2] = opr3 */
	OP_LEN,			/* 0x1b: dst = len(src) */

	/* dictionary */
	OP_GETDICTKEYBYINDEX,	/* 0x1c: dst = src1.keyAt(src2) */
	OP_GETDICTVALBYINDEX,	/* 0x1d: dst = src1.valAt(src2) */
	OP_STOREDOT,		/* 0x1e: obj.access = src */
	OP_LOADDOT,		/* 0x1f: dst = obj.access */

	/* symbol */
	OP_STORESYMBOL,		/* 0x20: setSymbol(dst, src) */
	OP_LOADSYMBOL,		/* 0x21: dst = getSymbol(src) */

	/* call */
	OP_CALL,		/* 0x22: func(arg1, ...) */
	OP_THISCALL,		/* 0x23: obj->func(arg1, ...) */

	/* branch */
	OP_JMP,			/* 0x24: PC = src */
	OP_JMPIFTRUE,		/* 0x25: PC = src1 if src2 != 0 */
	OP_JMPIFFALSE,		/* 0x26: PC = src1 if src2 == 0 */
	OP_JMPIFEQ,		/* 0x27: PC = src1 if src2 indicates eq */

	/* line number */
	OP_LINEINFO,		/* 0x28: setDebugLine(src) */
};

#endif
