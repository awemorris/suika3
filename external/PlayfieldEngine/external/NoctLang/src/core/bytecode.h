/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Bytecode
 */

#ifndef NOCT_BYTECODE_H
#define NOCT_BYTECODE_H

enum bytecode {
	/* no operation */
	OP_NOP,			/* 0x00:   0: nop */

	/* tmpvar assignment */
	OP_ASSIGN,		/* 0x01:   1: dst = src */
	OP_ICONST,		/* 0x02:   2: dst = integer constant */
	OP_LICONST,		/* 0x03:   3: dst = integer constant */
	OP_FCONST,		/* 0x04:   4: dst = floating-point constant */
	OP_LFCONST,		/* 0x05:   5: dst = floating-point constant */
	OP_SCONST,		/* 0x06:   6: dst = string constant */
	OP_ACONST,		/* 0x07:   7: dst = empty array */
	OP_DCONST,		/* 0x08:   8: dst = empty dictionary */

	/* tmpvar calc unary op (dst = op src1) */
	OP_INC,			/* 0x09:   9: dst = src + 1, assume operands are integers */
	OP_NEG,			/* 0x0a:  10: dst = -src */
	OP_NOT,			/* 0x0b:  11: dst = !src */

	/* tmpvar calc binary op (dst = src1 op src2) */
	OP_ADD,			/* 0x0c:  12: dst = src1 + src2 */
	OP_SUB,			/* 0x0d:  13: dst = src1 - src2 */
	OP_MUL,			/* 0x0e:  14: dst = src1 * src2 */
	OP_DIV,			/* 0x0f:  15: dst = src1 / src2 */
	OP_MOD,			/* 0x10:  16: dst = src1 % src2 */
	OP_AND,			/* 0x11:  17: dst = src1 & src2 */
	OP_OR,			/* 0x12:  18: dst = src1 | src2 */
	OP_XOR,			/* 0x13:  19: dst = src1 ^ src2 */
	OP_SHL,			/* 0x14:  20: dst = src1 << src2 */
	OP_SHR,			/* 0x15:  21: dst = src1 >> src2 */
	OP_LT,			/* 0x16:  22: dst = src1 <  src2 [0 or 1] */
	OP_LTE,			/* 0x17:  23: dst = src1 <= src2 [0 or 1] */
	OP_GT,			/* 0x18:  24: dst = src1 >  src2 [0 or 1] */
	OP_GTE,			/* 0x19:  25: dst = src1 >= src2 [0 or 1] */
	OP_EQ,			/* 0x1a:  26: dst = src1 == src2 [0 or 1] */
	OP_NEQ,			/* 0x1b:  27: dst = src1 != src2 [0 or 1] */
	OP_EQI,			/* 0x1c:  28: dst = src1 == src2 [0 or 1], assume operands are integers */

	/* array and dictionary (subscript) */
	OP_LOADARRAY,		/* 0x1d:  29: dst = src1[src2] */
	OP_STOREARRAY,		/* 0x1e:  30: opr1[opr2] = opr3 */
	OP_LEN,			/* 0x1f:  31: dst = len(src) */

	/* dictionary (property) */
	OP_STOREDOT,		/* 0x20:  32: obj.access = src */
	OP_LOADDOT,		/* 0x21:  33: dst = obj.access */

	/* dictionary (traverse) */
	OP_GETDICTKEYBYINDEX,	/* 0x22:  34: dst = src1.keyAt(src2) */
	OP_GETDICTVALBYINDEX,	/* 0x23:  35: dst = src1.valAt(src2) */

	/* symbol */
	OP_STORESYMBOL,		/* 0x24:  36: setSymbol(dst, src) */
	OP_LOADSYMBOL,		/* 0x25:  37: dst = getSymbol(src) */

	/* call */
	OP_CALL,		/* 0x26:  38: func(arg1, ...) */
	OP_THISCALL,		/* 0x27:  39: obj->func(arg1, ...) */

	/* branch */
	OP_JMP,			/* 0x28:  40: PC = src */
	OP_JMPIFTRUE,		/* 0x29:  41: PC = src1 if src2 != 0 */
	OP_JMPIFFALSE,		/* 0x2a:  42: PC = src1 if src2 == 0 */
	OP_JMPIFEQ,		/* 0x2b:  43: PC = src1 if src2 indicates eq (EQI->JMPIFEQ) */

	/* safepoint */
	OP_SAFEPOINT,		/* 0x2c:  44: safepoint() */

	/* line number */
	OP_LINEINFO,		/* 0x2d:  45: setDebugLine(src) */
};

#endif
