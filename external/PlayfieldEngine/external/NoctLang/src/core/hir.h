/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * HIR: High-level Intermediate Representation
 */

#ifndef NOCT_HIR_H
#define NOCT_HIR_H

#include <noct/noct.h>

/* Maximum Parameters and Arguments Size */
#define HIR_PARAM_SIZE		32

/* Compiler-only return annotation, not a rt_value tag. */
#define HIR_TYPE_VOID (-2)

/*
 * HIR Block Type
 */
enum hir_block_type {
	HIR_BLOCK_FUNC,
	HIR_BLOCK_BASIC,
	HIR_BLOCK_IF,
	HIR_BLOCK_FOR,
	HIR_BLOCK_WHILE,
	HIR_BLOCK_END,
};

/*
 * HIR Expression Type
 */
enum hir_expr_type {
	/* Base */
	HIR_EXPR_TERM,
	HIR_EXPR_LT,
	HIR_EXPR_LTE,
	HIR_EXPR_GT,
	HIR_EXPR_GTE,
	HIR_EXPR_EQ,
	HIR_EXPR_NEQ,
	HIR_EXPR_PLUS,
	HIR_EXPR_MINUS,
	HIR_EXPR_MUL,
	HIR_EXPR_DIV,
	HIR_EXPR_MOD,
	HIR_EXPR_AND,
	HIR_EXPR_OR,
	HIR_EXPR_LAND,
	HIR_EXPR_LOR,
	HIR_EXPR_XOR,
	HIR_EXPR_SHL,
	HIR_EXPR_SHR,
	HIR_EXPR_NEG,
	HIR_EXPR_NOT,
	HIR_EXPR_PAR,
	HIR_EXPR_SUBSCR,
	HIR_EXPR_DOT,
	HIR_EXPR_CALL,
	HIR_EXPR_THISCALL,
	HIR_EXPR_ARRAY,
	HIR_EXPR_DICT,
	HIR_EXPR_NEW,

	/* ABCE */
	HIR_EXPR_PBASE,		/* unary:  packed local -> payload address (long) */
	HIR_EXPR_PLEN,		/* unary:  packed local -> element count (int)    */
	HIR_EXPR_PCHECK,	/* binary: value, elemtype const -> 0/1           */
	HIR_EXPR_TYPEIS,	/* binary: value, type const     -> 0/1           */
	HIR_EXPR_PLOAD8U,	/* binary: base(long), offset    -> int (0..255)  */
	HIR_EXPR_PSTORE8,	/* as LHS only: base(long), offset; RHS = value   */
	HIR_EXPR_PLOAD8S,	/* binary: sign-extended int8 load                */
	HIR_EXPR_PLOAD16U,	/* binary: zero-extended uint16 load              */
	HIR_EXPR_PLOAD16S,	/* binary: sign-extended int16 load               */
	HIR_EXPR_PLOAD32,	/* binary: int32 load (uint32 wraps)              */
	HIR_EXPR_PLOAD64,	/* binary: int64/uint64 load -> long              */
	HIR_EXPR_PLOADF32,	/* binary: float32 load -> float                  */
	HIR_EXPR_PSTORE16,	/* as LHS only                                    */
	HIR_EXPR_PSTORE32,	/* as LHS only                                    */
	HIR_EXPR_PSTORE64,	/* as LHS only                                    */
	HIR_EXPR_PSTOREF32,	/* as LHS only; float32 source                    */
	HIR_EXPR_PMASKSTORE32,	/* optimizer-only: base, offset, lane mask        */
	HIR_EXPR_PGATHER32,	/* optimizer-only: checked base[index lanes]      */
	HIR_EXPR_VINDUCTF32,	/* optimizer-only: exact four-step f32 induction  */

	/* CSE */
	HIR_EXPR_CAPTURE,	/* unary + home local symbol                      */

	/* SIMD */
	HIR_EXPR_SELECT,	/* if conversion                                  */
};

/*
 * HIR Term Type
 */
enum hir_term_type {
	HIR_TERM_SYMBOL,
	HIR_TERM_INT,
	HIR_TERM_LONG,
	HIR_TERM_FLOAT,
	HIR_TERM_DOUBLE,
	HIR_TERM_STRING,
	HIR_TERM_EMPTY_ARRAY,
	HIR_TERM_EMPTY_DICT,
};

/*
 * Intrinsics ID
 */
enum hir_intrinsic_id {
	HIR_INTRINSIC_NONE,
	HIR_INTRINSIC_INT_FROM,
	HIR_INTRINSIC_FLOAT_FROM,
};


/* Source-level storage retained for target-neutral parallel analysis. */
enum hir_local_storage {
	HIR_LOCAL_STORAGE_UNKNOWN = -1,
	HIR_LOCAL_STORAGE_SCALAR,
	HIR_LOCAL_STORAGE_LOGICAL_BUFFER,
	HIR_LOCAL_STORAGE_REDUCTION
};

/*
 * Local Decl Type
 */
enum hir_local_declaration_kind {
	HIR_LOCAL_DECL_UNKNOWN = -1,
	HIR_LOCAL_DECL_PARAMETER,
	HIR_LOCAL_DECL_LET,
	HIR_LOCAL_DECL_VAR,
	HIR_LOCAL_DECL_LOOP_COUNTER,
	HIR_LOCAL_DECL_SYNTHETIC
};

/*
 * Scalar Kind
 */
enum hir_declared_scalar_kind {
	HIR_DECL_SCALAR_UNKNOWN = -1,
	HIR_DECL_SCALAR_INT32,
	HIR_DECL_SCALAR_UINT32,
	HIR_DECL_SCALAR_FLOAT32,
	HIR_DECL_SCALAR_OTHER
};

/*
 * Forward Declaration
 */
struct ast_func;
struct hir_stmt;
struct hir_expr;
struct hir_term;
struct hir_local;

/*
 * HIR Block
 */
struct hir_block {
	/* Block Type */
	int type;

	/* Line number. */
	int line;

	/* Parent Block (NULL if HIR_BLOCK_FUNC) */
	struct hir_block *parent;

	/* Successor Block (NULL if HIR_BLOCK_END) */
	struct hir_block *succ;

	/* Is a tail of siblings? */
	bool stop;

	/* The terminating edge is an explicit source-level return. */
	bool is_return_edge;

	/* The terminating edge is an explicit source-level break. */
	bool is_break_edge;

	/* The terminating edge is an explicit source-level continue. */
	bool is_continue_edge;

	/* Bytecode address. */
	uint32_t addr;

	/*
	 * Bytecode address a "continue" jumps to. (loop blocks only)
	 *
	 * For a ranged for loop, this is the loop-variable incrementer.
	 * For a for-each or a while loop, it is the loop head, because
	 * those advance their cursor before running the body.
	 */
	uint32_t cont_addr;

	/* Block Values */
	union {
		/* Function Header */
		struct {
			/* Function name. */
			char *name;

			/* Parameter names. */
			uint32_t param_count;
			char *param_name[HIR_PARAM_SIZE];

			/*
			 * Parameter type annotation:
			 *  - NOCT_VALUE_* tag per param, or -1 = unannotated.
			 */
			int param_type[HIR_PARAM_SIZE];

			/*
			 * Packed parameter type annotation:
			 *  - NOCT_PACKED_* element kind, or -1 = not typed packed.
			 */
			int param_packed_type[HIR_PARAM_SIZE];

			/* Original parameter annotation spellings. */
			char *param_type_name[HIR_PARAM_SIZE];

			/*
			 * Restrict packed parameter type annotation:
			 *  - rpacked* source annotation.
			 */
			bool param_restricted[HIR_PARAM_SIZE];

			/* Return type. */
			int return_type;
			int return_packed_type;
			char *return_type_name;

			/* Is static? */
			bool is_static;

			/* Is inline? */
			bool is_inline;

			/* Source-level __fast optimization hint. */
			bool is_fast;

			/* Accelerator optimization hint. */
			bool is_accel;

			/* Did the optimizer commit the __fast contract? */
			bool fast_optimized;

			/* Do all syntactic paths return a value? */
			bool returns_on_all_paths;

			/* Optimizer-owned __fast metadata. */
			void *fast_info;

			/* File name. */
			char *file_name;

			/* First inner block. */
			struct hir_block *inner;

			/* Local variable list. */
			struct hir_local *local;

			/* succ must be HIR_BLOCK_ENDFUNC. */
		} func;

		/* Basic Block */
		struct {
			/* Statements in a basic block. */
			struct hir_stmt *stmt_list;
		} basic;

		/* If Block */
		struct {
			/* Condition. */
			struct hir_expr *cond;

			/* First inner block. */
			struct hir_block *inner;

			/* Chained else-if or else block if exists. */
			struct hir_block *chain_next;

			/* Chaining previous. */
			struct hir_block *chain_prev;
		} if_;

		/* For Block */
		struct {
			/* First inner block. */
			struct hir_block *inner;

			/* Is a ranged for? */
			bool is_ranged;

			/* Ranged. */
			char *counter_symbol;
			struct hir_expr *start;
			struct hir_expr *stop;

			/* Key-Value or Value. */
			char *key_symbol;
			char *value_symbol;
			struct hir_expr *collection;

			/* For code generation. */
			uint32_t inc_addr;

			/* Optimizer: typed int region. */
			bool typed_int_region;

			/* Optimizer: ABCE-ed fast loop. */
			bool abce_fast;

			/* Optimizer: vectorized loop. */
			bool is_vector;

			/*
			 * Optimizer: vectorization result unit
			 *  - 0: ordinary loop
			 *  - 1: scalar Packed loop
			 *  - 4: SIMD strip.
			 */
			uint8_t packed_lanes;

			/*
			 * Optimizer: unrolling result unit
			 *  - 0/1: ordinary scalar body
			 *  - 4: four sequential scalar lanes.
			 */
			uint8_t scalar_unroll;
		} for_;

		/* While Block */
		struct {
			/* Condition. */
			struct hir_expr *cond;

			/* First inner block. */
			struct hir_block *inner;
		} while_;

		/* EndFunc Block */
		struct {
			/* No epilogue code. */
			int dummy;
		} end;
	} val;

	/* For debug. */
	int id;
};

/* HIR Statement */
struct hir_stmt {
	/* Line number. */
	int line;

	/* LHS (NULL if no assign) */
	struct hir_expr *lhs;

	/* RHS */
	struct hir_expr *rhs;

	/*
	 * True only for a source-level `return;` assignment to $return.
	 * The return-contract checker uses this flag to distinguish a bare
	 * return from a value-producing return, and the inliner rejects it
	 * when an inlineable return expression is required.
	 */
	bool is_bare_return;

	/* Next item. */
	struct hir_stmt *next;
};

/* HIR Expression */
struct hir_expr {
	/* Expression type. */
	int type;

	union {
		/* Term Expression */
		struct {
			/* Term. */
			struct hir_term *term;
		} term;

		/* Binary Operator Expression */
		struct {
			/* Expressions */
			struct hir_expr *expr[2];
		} binary;

		/* Unary Operator Expression */
		struct {
			struct hir_expr *expr;
		} unary;

		/* Dot Expression */
		struct {
			/* Object expression, */
			struct hir_expr *obj;

			/* Member symbol. */
			char *symbol;
		} dot;

		/* Function Call Expression */
		struct {
			/* Function expression. */
			struct hir_expr *func;

			/* Argument expressions. */
			uint32_t arg_count;
			struct hir_expr *arg[HIR_PARAM_SIZE];
		} call;

		/* This-Call Expression */
		struct {
			/* Object expression. */
			struct hir_expr *obj;

			/* Function name. */
			char *func;

			/* Argument expressions. */
			uint32_t arg_count;
			struct hir_expr *arg[HIR_PARAM_SIZE];
		} thiscall;

		/* Array Literal Expression */
		struct {
			/* Element count. */
			size_t elem_count;

			/* Element expressions. */
			struct hir_expr **elem;

			/* Compiler-owned list used by a multi-dimensional subscript. */
			bool is_multi_index;
		} array;

		/* Dictionary Literal Expression */
		struct {
			/* Key-value pair count. */
			size_t kv_count;

			/* Key strings. */
			char **key;

			/* Value expressions. */
			struct hir_expr **value;
		} dict;

		/* New Expression */
		struct {
			/* Class name. */
			char *cls;

			/* Initializer. */
			struct hir_expr *init;
		} new_;

		/* Capture Expression (optimizer-only: CSE) */
		struct {
			/* Captured expression. */
			struct hir_expr *expr;

			/* Home local variable symbol. */
			char *symbol;
		} capture;

		/* Select Expression (optimizer-only: SIMD predication) */
		struct {
			struct hir_expr *cond;
			struct hir_expr *if_true;
			struct hir_expr *if_false;
		} select;

		/* Masked packed store LHS (optimizer-only: SIMD). */
		struct {
			struct hir_expr *base;
			struct hir_expr *offset;
			struct hir_expr *mask;
		} mask_store;

		/* Checked packed gather (optimizer-only: SIMD). */
		struct {
			struct hir_expr *base;
			struct hir_expr *length;
			struct hir_expr *index;
			struct hir_expr *packed; /* scalar fallback owner */
		} gather;
	} val;
};

/* HIR Term */
struct hir_term {
	/* Term type. */
	int type;

	/* Value by type. */
	union {
		/* Symbol name. */
		char *symbol;

		/* Integer value. */
		int i;

		/* Long value. */
		int64_t l;

		/* Float value. */
		float f;

		/* Double value. */
		double lf;

		/* String value. */
		char *s;
	} val;
};

/* HIR Local Variable Entry */
struct hir_local {
	/* Symbol name. */
	char *symbol;

	/* Variable index. */
	int index;

	/* Is parametre? */
	bool is_parameter;

	/* let or var */
	bool is_let;

	/* Optimizer: type proven */
	int proven_type;

	/* Optimizer hints */
	int declaration_kind;
	int declared_type;
	char *declared_type_name;
	int declared_scalar_kind;
	int declared_packed_type;
	int storage_class;
	int declaration_line;
	const struct hir_stmt *declaration_stmt;
	const struct hir_expr *initializer;

	/* Next. */
	struct hir_local *next;
};

/*
 * Allocates memory from the HIR arena.
 */
void *
hir_malloc(
	size_t size);

/*
 * Duplicates a string in the HIR arena.
 */
char *
hir_strdup(
	const char *s);

/*
 * Reports an out-of-memory error while constructing HIR.
 */
void
hir_out_of_memory(void);

/*
 * Adds a local variable to a function block.
 */
bool
hir_add_local(
	struct hir_block *cur_block,
	const char *symbol);

/*
 * Allocates a fresh HIR block identifier.
 */
int
hir_next_block_id(void);

/*
 * Resolves a source type annotation into target-neutral HIR tags.
 */
bool
hir_resolve_type_annotation(
	int line,
	const char *type_name,
	bool allow_shape,
	int *tag,
	int *packed_type,
	bool *restricted);

/*
 * Build HIR functions from an AST.
 */
bool
hir_build(void);

/*
 * Free constructed HIR functions.
 */
void
hir_cleanup(void);

/*
 * Get a number of constructed HIR functions.
 */
uint32_t
hir_get_function_count(void);

/*
 * Get a constructed HIR function.
 */
struct hir_block *
hir_get_function(
	uint32_t index);

/*
 * Runs the configured HIR optimization passes on one function.
 */
bool
hir_optimize_func(
	struct hir_block *func_block,
	int optimize_level,
	bool print_simd_info,
	bool (*accel_optimize_func)(struct hir_block *func_block,
				    void *userdata),
	void *accel_optimize_userdata);

/*
 * Replace a function's link name while the HIR arena is alive.
 */
bool
hir_set_function_name(
	struct hir_block *func,
	const char *name);

/*
 * Get a file name.
 */
const char *
hir_get_file_name(void);

/*
 * Get an error line number.
 */
int
hir_get_error_line(void);

/*
 * Get an error message.
 */
const char *
hir_get_error_message(void);

/*
 * Set a deterministic error from a mandatory post-build compiler pass.
 */
void
hir_error(
	int line,
	const char *message);

/*
 * Return an immutable intrinsic ID for a call expression.
 */
int
hir_get_intrinsic_call(
	const struct hir_expr *expr);

/*
 * Debug dump.
 */
void
hir_dump_block(
	struct hir_block *block);

#endif
