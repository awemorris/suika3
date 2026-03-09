/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * AST: Abstract syntax tree
 */

#ifndef NOCT_AST_H
#define NOCT_AST_H

#include <noct/c89compat.h>

/*
 * AST
 */

/* Statement Type */
enum ast_stmt_type {
	AST_STMT_EMPTY,
	AST_STMT_EXPR,
	AST_STMT_ASSIGN,
	AST_STMT_IF,
	AST_STMT_ELIF,
	AST_STMT_ELSE,
	AST_STMT_WHILE,
	AST_STMT_FOR,
	AST_STMT_RETURN,
	AST_STMT_BREAK,
	AST_STMT_CONTINUE,
};

/* Expression Type */
enum ast_expr_type {
	AST_EXPR_TERM,
	AST_EXPR_LT,
	AST_EXPR_LTE,
	AST_EXPR_GT,
	AST_EXPR_GTE,
	AST_EXPR_EQ,
	AST_EXPR_NEQ,
	AST_EXPR_PLUS,
	AST_EXPR_MINUS,
	AST_EXPR_MUL,
	AST_EXPR_DIV,
	AST_EXPR_MOD,
	AST_EXPR_AND,
	AST_EXPR_OR,
	AST_EXPR_XOR,
	AST_EXPR_SHL,
	AST_EXPR_SHR,
	AST_EXPR_NEG,
	AST_EXPR_NOT,
	AST_EXPR_PAR,
	AST_EXPR_SUBSCR,
	AST_EXPR_DOT,
	AST_EXPR_CALL,
	AST_EXPR_THISCALL,
	AST_EXPR_ARRAY,
	AST_EXPR_DICT,
	AST_EXPR_FUNC,
	AST_EXPR_NEW,
};

/* Term Type */
enum ast_term_type {
	AST_TERM_INT,
	AST_TERM_FLOAT,
	AST_TERM_STRING,
	AST_TERM_SYMBOL,
	AST_TERM_EMPTY_ARRAY,
	AST_TERM_EMPTY_DICT,
};

/* Forward Declaration */
struct ast_func_list;
struct ast_func;
struct ast_param_list;
struct ast_param;
struct ast_stmt_list;
struct ast_stmt;
struct ast_expr;
struct ast_term;
struct ast_arg_list;

/* Function List */
struct ast_func_list {
	struct ast_func *list;
};

/* Function */
struct ast_func {
	/* Function name. */
	char *name;

	/* Parameter list. */
	struct ast_param_list *param_list;

	/* Statement list */
	struct ast_stmt_list *stmt_list;

	/* List next node. */
	struct ast_func *next;
};

/* AST Parameter List */
struct ast_param_list {
	struct ast_param *list;
};

/* AST Parameter */
struct ast_param {
	char *name;
	struct ast_param *next;
};

/* AST Statement List */
struct ast_stmt_list {
	struct ast_stmt *list;
};

/* AST Statement */
struct ast_stmt {
	/* Statement type. */
	int type;

	union {
		/* Expression Statement */
		struct {
			/* Expression */
			struct ast_expr *expr;
		} expr;

		/* Assignment Statement */
		struct {
			/* LHS and RHS */
			struct ast_expr *lhs;
			struct ast_expr *rhs;
			bool is_var;
		} assign;

		/* If Block */
		struct {
			/* Condition expression. */
			struct ast_expr *cond;

			/* Statement list. */
			struct ast_stmt_list *stmt_list;
		} if_;

		/* Else-If Block */
		struct {
			/* Condition expression. */
			struct ast_expr *cond;

			/* Statement list. */
			struct ast_stmt_list *stmt_list;
		} elif;

		/* Else Block */
		struct {
			/* Statement list. */
			struct ast_stmt_list *stmt_list;
		} else_;

		/* While Block */
		struct {
			/* Condition expression. */
			struct ast_expr *cond;

			/* Statement list. */
			struct ast_stmt_list *stmt_list;
		} while_;

		/* For Block */
		struct {
			/* Is ranged-for block? */
			bool is_range;

			/* Counter symbol. */
			char *counter_symbol;

			/* Counter start and stop for ranged-for. */
			struct ast_expr *start;
			struct ast_expr *stop;

			/* Key and value symbol. */
			char *key_symbol;
			char *value_symbol;

			/* Array expression. */
			struct ast_expr *collection;

			/* Statement list. */
			struct ast_stmt_list *stmt_list;
		} for_;

		/* Return Statement */
		struct {
			/* Return value expression. */
			struct ast_expr *expr;
		} return_;
	} val;

	/* Source code position. */
	int line;
	int column;

	/* Next list node. */
	struct ast_stmt *next;
};

/* AST Expression */
struct ast_expr {
	/* Expression type. */
	int type;

	union {
		/* Term Expression */
		struct {
			/* Term. */
			struct ast_term *term;
		} term;

		/* Binary Operator Expression */
		struct {
			/* Expressions. */
			struct ast_expr *expr[2];
		} binary;

		/* Unary Operator Expression */
		struct {
			/* Expression. */
			struct ast_expr *expr;
		} unary;

		/* Parensis Expression */
		struct {
			/* Expression. */
			struct ast_expr *expr;
		} par;

		/* Dot Expression */
		struct {
			/* Object expression. */
			struct ast_expr *obj;

			/* Member symbol. */
			char *symbol;
		} dot;

		/* Call Expression */
		struct {
			/* Function expression. */
			struct ast_expr *func;

			/* Argument list. */
			struct ast_arg_list *arg_list;
		} call;

		/* This-Call Expression */
		struct {
			/* Object expression. */
			struct ast_expr *obj;

			/* Function name. */
			char *func;

			/* Argument list. */
			struct ast_arg_list *arg_list;
		} thiscall;

		/* Array Literal Expression */
		struct {
			/* Element list. */
			struct ast_arg_list *elem_list;
		} array;

		/* Dictionary Literal Expression */
		struct {
			/* Element list. */
			struct ast_kv_list *kv_list;
		} dict;

		/* Anonymous Function Literal Expression */
		struct {
			/* Parameter list. */
			struct ast_param_list *param_list;

			/* Statement list. */
			struct ast_stmt_list *stmt_list;
		} func;

		/* New Expression */
		struct {
			/* Class name. */
			char *cls;

			/* Initializer. */
			struct ast_expr *init;
		} new_;
	} val;

	/* Next expression node. */
	struct ast_expr *next;
};

/* Key-Value Pair */
struct ast_kv {
	char *key;
	struct ast_expr *value;
	struct ast_kv *next;
};

/* Key-Value List */
struct ast_kv_list {
	struct ast_kv *list;
};

/* AST Term */
struct ast_term {
	/* Term type. */
	int type;

	union {
		/* Value. */
		int i;
		float f;
		char *s;
		char *symbol;
	} val;
};

/* AST Argument List */
struct ast_arg_list {
	struct ast_expr *list;
};

/*
 * Public
 */
bool ast_build(const char *file_name, const char *text);
void ast_cleanup(void);
struct ast_func_list *ast_get_func_list(void);
const char *ast_get_file_name(void);
const char *ast_get_error_message(void);
int ast_get_error_line(void);

#endif
