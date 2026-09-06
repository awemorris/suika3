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
	OP_INC,			/* 0x09:   9: dst += step(imm8), assume dst is integer */
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
	OP_THISCALL,		/* 0x27:  39: member call: dst,obj,callee,argc,args; inject obj iff param[0] is "this" */

	/* branch */
	OP_JMP,			/* 0x28:  40: PC = src */
	OP_JMPIFTRUE,		/* 0x29:  41: PC = src1 if src2 != 0 */
	OP_JMPIFFALSE,		/* 0x2a:  42: PC = src1 if src2 == 0 */
	OP_JMPIFEQ,		/* 0x2b:  43: PC = src1 if src2 indicates eq (EQI->JMPIFEQ) */

	/* safepoint */
	OP_SAFEPOINT,		/* 0x2c:  44: safepoint() */

	/* line number */
	OP_LINEINFO,		/* 0x2d:  45: setDebugLine(src) */

	/*
	 * The following are optional.
	 */

	/* ABCE optimizer. */
	OP_PLOOP_HINT,		/* 0x2e:  46: no-op: index/stop/remaining(u16), lanes/flags(imm8) */
	OP_PBASE,		/* 0x2f:  47: dst = packed payload; base_id(imm8) is a register hint */
	OP_PCHECK,		/* 0x30:  48: dst = (src is packed && elem type == imm8) */
	OP_PLEN,		/* 0x31:  49: dst = elem count of packed src (0 if not packed) */
	OP_TYPEIS,		/* 0x32:  50: dst = (typeof(src) == imm8) */
	OP_CHECKTYPE,		/* 0x33:  51: error unless typeof(tmpvar) == imm8 */
	OP_PLOAD8U,		/* 0x34:  52: dst = *(uint8 *)(src1 + src2), no checks */
	OP_PSTORE8,		/* 0x35:  53: *(uint8 *)(opr1 + opr2) = opr3, no checks */
	OP_PLOAD8S,		/* 0x36:  54: dst = *(int8   *)base[ofs], sign-extended */
	OP_PLOAD16U,		/* 0x37:  55: dst = *(uint16 *)base[ofs] */
	OP_PLOAD16S,		/* 0x38:  56: dst = *(int16  *)base[ofs], sign-extended */
	OP_PLOAD32,		/* 0x39:  57: dst = *(int32  *)base[ofs] (uint32 wraps) */
	OP_PLOAD64,		/* 0x3a:  58: dst = *(int64  *)base[ofs] as long */
	OP_PSTORE16,		/* 0x3b:  59: *(uint16 *)base[ofs] = src */
	OP_PSTORE32,		/* 0x3c:  60: *(uint32 *)base[ofs] = src */
	OP_PSTORE64,		/* 0x3d:  61: *(uint64 *)base[ofs] = src (int or long) */
	OP_PLOADF32,		/* 0x3e:  62: dst(float) = base[ofs] */
	OP_PSTOREF32,		/* 0x3f:  63: base[ofs] = src(float) */

	/* Typed operation optimizer. */
	OP_TMPVAR_TYPE,		/* 0x40:  64: declaration: tmp(u16) has fixed primitive tag(imm8) */
	OP_MATERIALIZE_TYPE,	/* 0x41:  65: tmp(u16) must expose fixed primitive tag(imm8) */
	OP_IADD,		/* 0x42:  66: dst = src1 + src2 (int32, wraps) */
	OP_ISUB,		/* 0x43:  67: dst = src1 - src2 (int32, wraps) */
	OP_IMUL,		/* 0x44:  68: dst = src1 * src2 (int32, low 32, wraps) */
	OP_IDIV,		/* 0x45:  69: dst = src1 / src2 (int32; emitted only for literal src2 not in {0,-1}) */
	OP_IMOD,		/* 0x46:  70: dst = src1 % src2 (int32; same literal rule) */
	OP_IAND,		/* 0x47:  71: dst = src1 & src2 (int32) */
	OP_IOR,			/* 0x48:  72: dst = src1 | src2 (int32) */
	OP_IXOR,		/* 0x49:  73: dst = src1 ^ src2 (int32) */
	OP_ISHL,		/* 0x4a:  74: dst = (uint32)src1 << imm (imm = operand 3, 0..31) */
	OP_ISHR,		/* 0x4b:  75: dst = (uint32)src1 >> imm (LOGICAL; imm = operand 3, 0..31) */
	OP_ILT,			/* 0x4c:  76: dst = src1 <  src2 (int32) [0 or 1] */
	OP_ILTE,		/* 0x4d:  77: dst = src1 <= src2 (int32) [0 or 1] */
	OP_IGT,			/* 0x4e:  78: dst = src1 >  src2 (int32) [0 or 1] */
	OP_IGTE,		/* 0x4f:  79: dst = src1 >= src2 (int32) [0 or 1] */
	OP_FADD,		/* 0x50:  80: dst = src1 + src2 (float, IEEE binary32) */
	OP_FSUB,		/* 0x51:  81: dst = src1 - src2 (float) */
	OP_FMUL,		/* 0x52:  82: dst = src1 * src2 (float) */
	OP_FDIV,		/* 0x53:  83: dst = src1 / src2 (float; IEEE-total, 07 Part 0) */
	OP_FLT,			/* 0x54:  84: dst = src1 <  src2 (float) [0 or 1; NaN -> 0] */
	OP_FLTE,		/* 0x55:  85: dst = src1 <= src2 (float) [0 or 1; NaN -> 0] */
	OP_FGT,			/* 0x56:  86: dst = src1 >  src2 (float) [0 or 1; NaN -> 0] */
	OP_FGTE,		/* 0x57:  87: dst = src1 >= src2 (float) [0 or 1; NaN -> 0] */
	OP_IDIV_CHECKED,	/* 0x58:  88: dst(u16) = src1(u16) / src2(u16), checked int32 */
	OP_IMOD_CHECKED,	/* 0x59:  89: dst(u16) = src1(u16) % src2(u16), checked int32 */

	/* SIMD optimizer. */
	OP_VLOADI32X4,		/* 0x5a:  90: vreg[vd](imm8) = 16B at base(u16).l + sext(ofs(u16).i)*4 */
	OP_VSTOREI32X4,		/* 0x5b:  91: 16B at base(u16).l + sext(ofs(u16).i)*4 = vreg[vs](imm8) */
	OP_VSPLATI32,		/* 0x5c:  92: vreg[vd](imm8).i[0..3] = src(u16).val.i */
	OP_VGETLANEI32,		/* 0x5d:  93: dst(u16) = int vreg[vs](imm8).i[lane(imm8)] */
	OP_VMOV128,		/* 0x5e:  94: vreg[vd](imm8) = vreg[vs](imm8) */
	OP_VADDI32X4,		/* 0x5f:  95: vd = va + vb, lane-wise int32 wrap (imm8 x3) */
	OP_VSUBI32X4,		/* 0x60:  96: vd = va - vb (imm8 x3) */
	OP_VMULI32X4,		/* 0x61:  97: vd = va * vb, low 32 (imm8 x3) */
	OP_VAND128,		/* 0x62:  98: vd = va & vb (imm8 x3) */
	OP_VOR128,		/* 0x63:  99: vd = va | vb (imm8 x3) */
	OP_VXOR128,		/* 0x64: 100: vd = va ^ vb (imm8 x3) */
	OP_VSHLI32X4,		/* 0x65: 101: vd = va << c, lane-wise (imm8 x3; c in 1..31) */
	OP_VSHRI32X4,		/* 0x66: 102: vd = va >> c, LOGICAL (imm8 x3; c in 1..31) */
	OP_VLOADF32X4,		/* 0x67: 103: vd = four float32 elements at base + ofs*4 */
	OP_VSTOREF32X4,		/* 0x68: 104: store four float32 elements */
	OP_VSPLATF32,		/* 0x69: 105: vd.f[0..3] = src.val.f */
	OP_VGETLANEF32,		/* 0x6a: 106: dst = float vd.f[lane] */
	OP_VADDF32X4,		/* 0x6b: 107: vd = va + vb (IEEE binary32) */
	OP_VSUBF32X4,		/* 0x6c: 108: vd = va - vb */
	OP_VMULF32X4,		/* 0x6d: 109: vd = va * vb */
	OP_VDIVF32X4,		/* 0x6e: 110: vd = va / vb */
	OP_VCVTI32F32X4,	/* 0x6f: 111: vd.f = (float)va.i */
	OP_VCVTF32I32X4,	/* 0x70: 112: vd.i = (int32_t)va.f */
	OP_VINDEX_HINT,		/* 0x71: 113: no-op: index/stop/remaining(u16), required_vregs/lanes/flags(imm8) */
	OP_SUBJNZ,		/* 0x72: 114: value(u16) -= decrement(imm8); jump target(u32) if nonzero */
	OP_VORI32X4I,		/* 0x73: 115: vd = vs | (imm8 << shift), lane-wise; four imm8 operands */
	OP_VFMAF32X4,		/* 0x74: 116: vd = fmaf(va, vb, vc), lane-wise; four imm8 operands */
	OP_VCMPI32X4,		/* 0x75: 117: vd = compare(va, vb, pred); four imm8 operands */
	OP_VCMPF32X4,		/* 0x76: 118: vd = compare(va, vb, pred); four imm8 operands */
	OP_VSELECT128,		/* 0x77: 119: vd = mask ? vt : vf, bitwise; four imm8 operands */
	OP_VMASKSTOREI32X4,	/* 0x78: 120: base(u16), ofs(u16), value(vreg), mask(vreg) */
	OP_VINDUCTF32X4,	/* 0x79: 121: vd(u8), state(u16), step(u16), exact recurrence */
	OP_VGATHERI32X4_CHECKED,/* 0x7a: 122: vd(u8), base(u16), plen(u16), vi(u8) */
	OP_VMINS32X4,		/* 0x7b: 123: vd(u8) = signed-min(va(u8), vb(u8)) */
	OP_VMAXS32X4,		/* 0x7c: 124: vd(u8) = signed-max(va(u8), vb(u8)) */
};

/* OP_CHECKTYPE imm8 encoding for element-specific packed annotations. */
#define TYPECHECK_PACKED_BASE	16
#define TYPECHECK_RPACKED_BASE	32

/* OP_CHECKTYPE performs an exact return check and uses return diagnostics. */
#define TYPECHECK_RETURN_FLAG	128
#define TYPECHECK_LOCAL_FLAG	64

/* OP_TMPVAR_TYPE tag operand metadata. */
#define TMPVAR_TYPE_COMPILER_TEMP	0x80
#define TMPVAR_TYPE_DYNAMIC		0x7f

/* OP_PLOOP_HINT flags. */
#define PLOOP_TYPED_INT			0x01
#define PLOOP_TYPED_FLOAT		0x02
#define PLOOP_ALLOW_REGCACHE		0x04
#define PLOOP_HAS_CONTROL		0x08
#define PLOOP_UNROLL4			0x10	/* body contains four sequential scalar lanes */

/* Predicates for OP_VCMPI32X4 and OP_VCMPF32X4. */
enum vector_compare_predicate {
	VCMP_EQ,
	VCMP_NE,
	VCMP_LT,
	VCMP_LE,
	VCMP_GT,
	VCMP_GE,
	VCMP_PREDICATE_COUNT
};

/* OP_VINDEX_HINT flags. */
#define VINDEX_CURSOR_ONLY		0x01
#define VINDEX_WRITEBACK_STOP		0x02
#define VINDEX_FORCE_SCALAR		0x04	/* keep vector state memory-canonical */
#define VINDEX_REQUIRE_MASKSTORE	0x08	/* region contains VMASKSTOREI32X4 */
#define VINDEX_REQUIRE_INDUCT		0x10	/* region contains exact VINDUCTF32X4 */
#define VINDEX_REQUIRE_GATHER		0x20	/* region contains checked gather */

#endif
