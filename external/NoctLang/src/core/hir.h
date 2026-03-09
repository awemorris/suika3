/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * HIR: High-level intermediate representation
 */

#ifndef NOCT_HIR_H
#define NOCT_HIR_H

#include <noct/c89compat.h>

/* HIR Block Type */
enum hir_block_type {
	HIR_BLOCK_FUNC,
	HIR_BLOCK_BASIC,
	HIR_BLOCK_IF,
	HIR_BLOCK_FOR,
	HIR_BLOCK_WHILE,
	HIR_BLOCK_END,
};

/* HIR Expression Type */
enum hir_expr_type {
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
};

/* HIR Term Type */
enum hir_term_type {
	HIR_TERM_SYMBOL,
	HIR_TERM_INT,
	HIR_TERM_FLOAT,
	HIR_TERM_STRING,
	HIR_TERM_EMPTY_ARRAY,
	HIR_TERM_EMPTY_DICT,
};

/* Maximum Parameters and Arguments Size */
#define HIR_PARAM_SIZE		32

/* Forward Declaration */
struct hir_cfg_node;
struct hir_stmt;
struct hir_expr;
struct hir_term;
struct hir_local;

/* HIR Block */
struct hir_block {
	/* Block Type */
	int type;

	/* Line number. */
	int line;

	/* Parent Block (NULL on FIR_BLOCK_FUNC) */
	struct hir_block *parent;

	/* Successor Block (NULL on HIR_BLOCK_END) */
	struct hir_block *succ;

	/* Is a tail of siblings? */
	bool stop;

	/* Bytecode address. */
	uint32_t addr;

	/* Block Values */
	union {
		/* Function Header */
		struct {
			/* Function name. */
			char *name;

			/* Parameter names. */
			int param_count;
			char *param_name[HIR_PARAM_SIZE];

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
			int arg_count;
			struct hir_expr *arg[HIR_PARAM_SIZE];
		} call;

		/* This-Call Expression */
		struct {
			/* Object expression. */
			struct hir_expr *obj;

			/* Function name. */
			char *func;

			/* Argument expressions. */
			int arg_count;
			struct hir_expr *arg[HIR_PARAM_SIZE];
		} thiscall;

		/* Array Literal Expression */
		struct {
			/* Element count. */
			int elem_count;

			/* Element expressions. */
			struct hir_expr **elem;
		} array;

		/* Dictionary Literal Expression */
		struct {
			/* Key-value pair count. */
			int kv_count;

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

		/* Float value. */
		float f;

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

	/* Next. */
	struct hir_local *next;
};

/* Build HIR functions from an AST. */
bool hir_build(void);

/* Free constructed HIR functions. */
void hir_cleanup(void);

/* Get a number of constructed HIR functions. */
int hir_get_function_count(void);

/* Get a constructed HIR function. */
struct hir_block *hir_get_function(int index);

/* Get a file name. */
const char *hir_get_file_name(void);

/* Get an error line number. */
int hir_get_error_line(void);

/* Get an error message. */
const char *hir_get_error_message(void);

/* Debug dump. */
void hir_dump_block(struct hir_block *block);

#endif
