/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * scmback: GNU Guile Scheme translation backend
 *
 * This backend emits Guile Scheme code and assumes a small runtime that
 * provides noct-array-ref, noct-array-set!, noct-dot-ref, noct-dot-set!,
 * noct-dict, noct-new, noct-for-each, noct-for-each-kv and noct-eq?.
 * Noct arrays are emitted as Scheme lists so that subscript assignment can
 * grow them dynamically.
 */

#include <noct/noct.h>
#include "ast.h"
#include "hir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

/* False assertions. */
#define NEVER_COME_HERE		0
#define NOT_IMPLEMENTED		0

/* Allocation size. */
#define ALLOC_SIZE		(8192)

/*
 * String writers.
 */

#define PUT(s)		if (!scmback_put(s)) return false
#define PUT1(s, x)	if (!scmback_put(s, x)) return false
#define PUT2(s, x, y)	if (!scmback_put(s, x, y)) return false
#define PUT_INDENT()	if (!scmback_put_indent()) return false

/*
 * Translation context.
 */

static FILE *fp;
static int indent;
static bool has_main;
static struct hir_block *cur_func;
static int loop_nest;

#if 0
static char *body;
static size_t body_len;
static size_t body_alloc_size;
#endif

/*
 * Error position and message.
 */

static char *scmback_file_name;
static int scmback_error_line;
static char scmback_error_message[65536];

/*
 * Forward declaration
 */

static bool scmback_translate_func(struct hir_block *func);
static bool is_parameter_name(const char *symbol);
static bool is_local_name(const char *symbol);
static bool scmback_visit_block(struct hir_block *block);
static bool scmback_visit_basic_block(struct hir_block *block);
static bool scmback_visit_if_block(struct hir_block *block);
static bool scmback_visit_for_block(struct hir_block *block);
static bool scmback_visit_for_range_block(struct hir_block *block);
static bool scmback_visit_for_kv_block(struct hir_block *block);
static bool scmback_visit_for_v_block(struct hir_block *block);
static bool scmback_visit_while_block(struct hir_block *block);
static bool scmback_visit_stmt(struct hir_stmt *stmt);
static bool scmback_visit_expr(struct hir_expr *expr);
static bool scmback_visit_unary_expr(struct hir_expr *expr);
static bool scmback_visit_binary_expr(struct hir_expr *expr);
static bool scmback_visit_dot_expr(struct hir_expr *expr);
static bool scmback_visit_call_expr(struct hir_expr *expr);
static bool scmback_visit_thiscall_expr(struct hir_expr *expr);
static bool scmback_visit_array_expr(struct hir_expr *expr);
static bool scmback_visit_dict_expr(struct hir_expr *expr);
static bool scmback_visit_new_expr(struct hir_expr *expr);
static bool scmback_visit_term(struct hir_term *term);
static bool scmback_put(const char *format, ...);
static bool scmback_put_indent(void);
static bool scmback_put_string_literal(const char *s);
static void scmback_fatal(const char *msg, ...);
static void scmback_out_of_memory(void);

/*
 * Start Guile Scheme backend.
 */
NOCT_DLL
bool
noct_scmback_start(
	const char *fname)
{
	indent = 0;
	has_main = false;

	fp = fopen(fname, "wb");
	if (fp == NULL) {
		scmback_fatal(N_TR("Cannot open file %s."));
		return false;
	}
	return true;

	/* For in-memory translation. */
#if 0
	if (body != NULL) {
		free(body);
		body = NULL;
	}

	body = malloc(ALLOC_SIZE);
	if (body == NULL) {
		scmback_out_of_memory();
		return false;
	}
	body_len = 0;
	body_alloc_size = ALLOC_SIZE;

	return true;
#endif
}

/*
 * Translate a file using Guile Scheme backend.
 */
NOCT_DLL
bool
noct_scmback_translate(
	const char *fname,
	const char *data)
{
	uint32_t func_count, i;

	/* Do parse, build AST. */
	if (!ast_build(fname, data)) {
		printf(N_TR("Error: %s:%d: %s\n"),
		       ast_get_file_name(),
		       ast_get_error_line(),
		       ast_get_error_message());
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		printf(N_TR("Error: %s:%d: %s\n"),
		       hir_get_file_name(),
		       hir_get_error_line(),
		       hir_get_error_message());
		return false;
	}

	/* For each HIR function. */
	func_count = hir_get_function_count();
	for (i = 0; i < func_count; i++) {
		struct hir_block *hfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(i);
		cur_func = hfunc;

		/* Check if it is a main function. */
		if (strcmp(hfunc->val.func.name, "main") == 0)
			has_main = true;

		/* Put a C function. */
		if (!scmback_translate_func(hfunc))
			return false;
	}

	/* Free intermediated. */
	hir_cleanup();
	ast_cleanup();

	return true;
}

/*
 * Finalize the Guile Scheme backend.
 */
NOCT_DLL
bool
noct_scmback_finalize(void)
{
	if (fp == NULL)
		return false;

	/*
	 * Runtime helpers.
	 *
	 * Noct arrays are represented as Scheme lists here, because Guile
	 * vectors are fixed-size.  noct-subscript-set! returns the updated
	 * collection, so the code generator must assign it back to the base
	 * variable when the base is a symbol.
	 */
	PUT("(define noct-helpers #t)\n");
	PUT("(define (newArray size)\n");
	PUT("  (make-list size #f))\n");
	PUT("(define (noct-list-set lst idx val)\n");
	PUT("  (let loop ((xs lst) (i idx))\n");
	PUT("    (cond\n");
	PUT("     ((< i 0)\n");
	PUT("      (error \"noct-list-set: negative index\" idx))\n");
	PUT("     ((null? xs)\n");
	PUT("      (if (= i 0)\n");
	PUT("          (list val)\n");
	PUT("          (cons 0 (loop '() (- i 1)))))\n");
	PUT("     ((= i 0)\n");
	PUT("      (cons val (cdr xs)))\n");
	PUT("     (else\n");
	PUT("      (cons (car xs) (loop (cdr xs) (- i 1)))))))\n");
	PUT("(define (noct-list-push lst val)\n");
	PUT("  (append lst (list val)))\n");
	PUT("(define (noct-list-pop lst)\n");
	PUT("  (let loop ((xs lst))\n");
	PUT("    (cond\n");
	PUT("     ((null? xs)\n");
	PUT("      '())\n");
	PUT("     ((null? (cdr xs))\n");
	PUT("      '())\n");
	PUT("     (else\n");
	PUT("      (cons (car xs) (loop (cdr xs)))))))\n");
	PUT("(define (noct-array-ref arr idx)\n");
	PUT("  (noct-subscript-ref arr idx))\n");
	PUT("(define (noct-array-set! arr idx val)\n");
	PUT("  (noct-subscript-set! arr idx val))\n");
	PUT("(define (noct-array-push arr val)\n");
	PUT("  (cond\n");
	PUT("   ((list? arr) (noct-list-push arr val))\n");
	PUT("   (else (error \"noct-array-push: unsupported object\" arr))))\n");
	PUT("(define (noct-array-pop arr)\n");
	PUT("  (cond\n");
	PUT("   ((list? arr) (noct-list-pop arr))\n");
	PUT("   (else (error \"noct-array-pop: unsupported object\" arr))))\n");
	PUT("(define (noct-dot-ref obj key)\n");
	PUT("  (cond\n");
	PUT("   ((hash-table? obj)\n");
	PUT("    (hash-ref obj key #f))\n");
	PUT("   ((and (string? obj) (eq? key 'length))\n");
	PUT("    (string-length obj))\n");
	PUT("   ((and (list? obj) (eq? key 'push))\n");
	PUT("    (lambda (val) (noct-list-push obj val)))\n");
	PUT("   ((and (list? obj) (eq? key 'pop))\n");
	PUT("    (lambda () (noct-list-pop obj)))\n");
	PUT("   ((and (list? obj) (eq? key 'length))\n");
	PUT("    (length obj))\n");
	PUT("   ((and (vector? obj) (eq? key 'length))\n");
	PUT("    (vector-length obj))\n");
	PUT("   (else\n");
	PUT("    (error \"noct-dot-ref: unsupported object/key\" obj key))))\n");
	PUT("(define (noct-dot-set! obj key val)\n");
	PUT("  (cond\n");
	PUT("   ((hash-table? obj)\n");
	PUT("    (hash-set! obj key val)\n");
	PUT("    val)\n");
	PUT("   (else\n");
	PUT("    (error \"noct-dot-set!: unsupported object/key\" obj key val))))\n");
	PUT("(define (noct-dict . pairs)\n");
	PUT("  (let ((h (make-hash-table)))\n");
	PUT("    (for-each\n");
	PUT("     (lambda (p)\n");
	PUT("       (hash-set! h (car p) (cdr p)))\n");
	PUT("     pairs)\n");
	PUT("    h))\n");
	PUT("(define (noct-new cls init)\n");
	PUT("  (let ((h (make-hash-table)))\n");
	PUT("    (hash-for-each\n");
	PUT("     (lambda (k v)\n");
	PUT("       (hash-set! h k v))\n");
	PUT("     cls)\n");
	PUT("    (hash-for-each\n");
	PUT("     (lambda (k v)\n");
	PUT("       (hash-set! h k v))\n");
	PUT("     init)\n");
	PUT("    h))\n");
	PUT("(define (noct-for-each proc collection)\n");
	PUT("  (cond\n");
	PUT("   ((list? collection)\n");
	PUT("    (for-each proc collection))\n");
	PUT("   ((hash-table? collection)\n");
	PUT("    (hash-for-each\n");
	PUT("     (lambda (k v)\n");
	PUT("       (proc v))\n");
	PUT("     collection))\n");
	PUT("   ((vector? collection)\n");
	PUT("    (let loop ((i 0))\n");
	PUT("      (if (< i (vector-length collection))\n");
	PUT("          (begin\n");
	PUT("            (proc (vector-ref collection i))\n");
	PUT("            (loop (+ i 1)))\n");
	PUT("          #t)))\n");
	PUT("   (else\n");
	PUT("    (error \"noct-for-each: unsupported collection\" collection))))\n");
	PUT("(define (noct-for-each-kv proc collection)\n");
	PUT("  (cond\n");
	PUT("   ((hash-table? collection)\n");
	PUT("    (hash-for-each proc collection))\n");
	PUT("   ((list? collection)\n");
	PUT("    (let loop ((i 0) (xs collection))\n");
	PUT("      (if (null? xs)\n");
	PUT("          #t\n");
	PUT("          (begin\n");
	PUT("            (proc i (car xs))\n");
	PUT("            (loop (+ i 1) (cdr xs))))))\n");
	PUT("   ((vector? collection)\n");
	PUT("    (let loop ((i 0))\n");
	PUT("      (if (< i (vector-length collection))\n");
	PUT("          (begin\n");
	PUT("            (proc i (vector-ref collection i))\n");
	PUT("            (loop (+ i 1)))\n");
	PUT("          #t)))\n");
	PUT("   (else\n");
	PUT("    (error \"noct-for-each-kv: unsupported collection\" collection))))\n");
	PUT("(define (noct-eq? a b)\n");
	PUT("  (equal? a b))\n");
	PUT("(define (noct-plus a b)\n");
	PUT("  (cond\n");
	PUT("   ((and (number? a) (number? b))\n");
	PUT("    (+ a b))\n");
	PUT("   ((or (string? a) (string? b))\n");
	PUT("    (string-append\n");
	PUT("     (if (string? a) a (object->string a))\n");
	PUT("     (if (string? b) b (object->string b))))\n");
	PUT("   (else\n");
	PUT("    (error \"unsupported + operands\" a b))))\n");
	PUT("(define (noct-subscript-ref obj key)\n");
	PUT("  (cond\n");
	PUT("   ((list? obj)\n");
	PUT("    (list-ref obj key))\n");
	PUT("   ((hash-table? obj)\n");
	PUT("    (hash-ref obj key #f))\n");
	PUT("   ((vector? obj)\n");
	PUT("    (vector-ref obj key))\n");
	PUT("   (else\n");
	PUT("    (error \"noct-subscript-ref: unsupported object\" obj))))\n");
	PUT("(define (noct-subscript-set! obj key val)\n");
	PUT("  (cond\n");
	PUT("   ((list? obj)\n");
	PUT("    (noct-list-set obj key val))\n");
	PUT("   ((hash-table? obj)\n");
	PUT("    (hash-set! obj key val)\n");
	PUT("    obj)\n");
	PUT("   ((vector? obj)\n");
	PUT("    (vector-set! obj key val)\n");
	PUT("    obj)\n");
	PUT("   (else\n");
	PUT("    (error \"noct-subscript-set!: unsupported object\" obj))))\n");
	PUT("(define (noct-global-set! sym val)\n");
	PUT("  (module-define! (current-module) sym val)\n");
	PUT("  val)\n");
	PUT("(define (print s) (display s) (newline))\n");
	PUT("(define (noct-list-take lst n)\n");
	PUT("  (cond\n");
	PUT("   ((<= n 0) '())\n");
	PUT("   ((null? lst) '())\n");
	PUT("   (else\n");
	PUT("    (cons (car lst)\n");
	PUT("          (noct-list-take (cdr lst) (- n 1))))))\n");
	PUT("(define (noct-list-resize lst new-size)\n");
	PUT("  (let ((len (length lst)))\n");
	PUT("    (cond\n");
	PUT("     ((< new-size len)\n");
	PUT("      (noct-list-take lst new-size))\n");
	PUT("     ((> new-size len)\n");
	PUT("      (append lst\n");
	PUT("              (make-list (- new-size len) 0)))\n");
	PUT("     (else\n");
	PUT("      lst))))\n");
	PUT("(define (noct-array-resize arr size)\n");
	PUT("  (cond\n");
	PUT("   ((list? arr)\n");
	PUT("    (noct-list-resize arr size))\n");
	PUT("   (else\n");
	PUT("    (error \"noct-array-resize: unsupported object\" arr))))\n");
	PUT("(define (noct-length obj)\n");
	PUT("  (cond\n");
	PUT("   ((list? obj)\n");
	PUT("    (length obj))\n");
	PUT("   ((string? obj)\n");
	PUT("    (string-length obj))\n");
	PUT("   ((vector? obj)\n");
	PUT("    (vector-length obj))\n");
	PUT("   ((hash-table? obj)\n");
	PUT("    (hash-count (const #t) obj))\n");
	PUT("   (else\n");
	PUT("    (error \"length: unsupported object\" obj))))\n");
	PUT("(define (noct-div a b)\n");
	PUT("  (cond\n");
	PUT("   ((and (integer? a)\n");
	PUT("         (integer? b))\n");
	PUT("    (quotient a b))\n");
	PUT("   (else\n");
	PUT("    (/ (exact->inexact a)\n");
	PUT("       (exact->inexact b)))))\n");
	PUT("(define (noct-and a b)\n");
	PUT("  (cond\n");
	PUT("   ((and (boolean? a)\n");
	PUT("         (boolean? b))\n");
	PUT("    (and a b))\n");
	PUT("   ((and (integer? a)\n");
	PUT("         (integer? b))\n");
	PUT("    (logand a b))\n");
	PUT("   (else\n");
	PUT("    (if a b a))))\n");
	PUT("(define (noct-or a b)\n");
	PUT("  (cond\n");
	PUT("   ((and (boolean? a)\n");
	PUT("         (boolean? b))\n");
	PUT("    (or a b))\n");
	PUT("   ((and (integer? a)\n");
	PUT("         (integer? b))\n");
	PUT("    (logior a b))\n");
	PUT("   (else\n");
	PUT("    (if a a b))))\n");

	if (has_main)
		PUT("\n(main)\n");

	fclose(fp);
	fp = NULL;

	return true;
}

static
const char *
func_name_sanitizer(
	const char *name)
{
	static char buf[1024];
	const char *s;
	char *d;

	if (name[0] != '$')
		return name;

	s = name;
	d = buf;
	while (*s != '\0') {
		if (isalnum(*s))
			*d = *s;
		else
			*d = '_';
		s++;
		d++;
	}
	return buf;
}

/* Translate a function. */
static
bool
scmback_translate_func(
	struct hir_block *func)
{
	struct hir_block *cur_block;
	struct hir_local *local;
	bool first_local;
	uint32_t i;

	scmback_file_name = strdup(func->val.func.file_name);
	if (scmback_file_name == NULL) {
		scmback_out_of_memory();
		return false;
	}

	scmback_put("(define (%s", func_name_sanitizer(func->val.func.name));
	for (i = 0; i < func->val.func.param_count; i++)
		PUT1(" %s", func->val.func.param_name[i]);
	PUT(")\n");

	indent++;
	PUT_INDENT();
	PUT("(let (");
	local = func->val.func.local;
	first_local = true;
	while (local != NULL) {
		if (!is_parameter_name(local->symbol)) {
			if (!first_local)
				PUT(" ");
			PUT1("(%s 0)", local->symbol);
			first_local = false;
		}
		local = local->next;
	}
	PUT(")\n");

	indent++;
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%return)\n");

	indent++;
	cur_block = func->val.func.inner;
	while (cur_block != NULL) {
		if (!scmback_visit_block(cur_block))
			return false;
		if (cur_block->stop)
			break;
		cur_block = cur_block->succ;
	}

	PUT_INDENT();
	PUT("0\n");

	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n\n");

	return true;
}

static bool is_parameter_name(const char *symbol)
{
	uint32_t i;

	for (i = 0; i < cur_func->val.func.param_count; i++) {
		if (strcmp(cur_func->val.func.param_name[i], symbol) == 0)
			return true;
	}
	return false;
}

static bool is_local_name(const char *symbol)
{
	struct hir_local *local;

	local = cur_func->val.func.local;
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return true;
		local = local->next;
	}

	return false;
}

/* Visit a block. */
static bool
scmback_visit_block(
	struct hir_block *block)
{
	assert(block != NULL);

	scmback_error_line = block->line;

	switch (block->type) {
	case HIR_BLOCK_BASIC:
		if (!scmback_visit_basic_block(block))
			return false;
		break;
	case HIR_BLOCK_IF:
		if (!scmback_visit_if_block(block))
			return false;
		break;
	case HIR_BLOCK_FOR:
		if (!scmback_visit_for_block(block))
			return false;
		break;
	case HIR_BLOCK_WHILE:
		if (!scmback_visit_while_block(block))
			return false;
		break;
	case HIR_BLOCK_END:
		return true;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	return true;
}

/* Visit a basic block. */
static bool
scmback_visit_basic_block(
	struct hir_block *block)
{
	struct hir_stmt *stmt;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_BASIC);

	stmt = block->val.basic.stmt_list;
	while (stmt != NULL) {
		if (!scmback_visit_stmt(stmt))
			return false;
		stmt = stmt->next;
	}

	return true;
}

/* Visit an if block. */
static bool
scmback_visit_if_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_IF);

	PUT_INDENT();
	PUT("(if ");
	if (!scmback_visit_expr(block->val.if_.cond)) return false;
	PUT("\n");
	b = block->val.if_.inner;
	if (b != NULL) {
		indent++;
		PUT_INDENT();
		PUT("(begin\n");
		indent++;
		while (b != NULL) {
			if (!scmback_visit_block(b)) return false;
			if (b->stop) {
				if (b->succ != NULL &&
				    b->succ->parent != NULL &&
				    (b->succ->parent->type == HIR_BLOCK_FOR ||
				     b->succ->parent->type == HIR_BLOCK_WHILE)) {
					PUT_INDENT();
					PUT("(%%continue 0)\n");
					break;
				} else {
					if (loop_nest > 0) {
						PUT_INDENT();
						PUT("(%%break 0)\n");
						break;
					}
				}
			}
			b = b->succ;
		}
		indent--;
		PUT_INDENT();
		PUT(")\n");
	} else {
		indent++;
		PUT_INDENT();
		PUT("0\n");
		indent--;
	}

	if (block->val.if_.chain_next != NULL) {
		if (block->val.if_.chain_next->val.if_.cond == NULL) {
			b = block->val.if_.chain_next->val.if_.inner;
			if (b != NULL) {
				PUT_INDENT();
				PUT("(begin\n");
				indent++;
				while (b != NULL) {
					if (!scmback_visit_block(b)) return false;
					if (b->succ != NULL &&
					    b->succ->parent != NULL &&
					    (b->succ->parent->type == HIR_BLOCK_FOR ||
					     b->succ->parent->type == HIR_BLOCK_WHILE)) {
						PUT_INDENT();
						PUT("(%%continue 0)\n");
						break;
					} else {
						if (loop_nest > 0) {
							PUT_INDENT();
							PUT("(%%break 0)\n");
							break;
						}
					}
					b = b->succ;
				}
				indent--;
				PUT_INDENT();
				PUT(")\n");
			} else {
				PUT_INDENT();
				PUT("0\n");
				indent--;
			}
		} else {
			if (!scmback_visit_if_block(block->val.if_.chain_next)) return false;
		}
	} else {
		PUT_INDENT();
		PUT("#f\n");
	}

	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a "for" block. */
static bool
scmback_visit_for_block(
	struct hir_block *block)
{
	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);

	/* Dispatch by type. */
	if (block->val.for_.is_ranged) {
		/* This is a ranged-for loop. */
		if (!scmback_visit_for_range_block(block))
			return false;
	} else if (block->val.for_.key_symbol != NULL) {
		/* This is a for-each-key-and-value loop. */
		if (!scmback_visit_for_kv_block(block))
			return false;
	} else {
		/* This is a for-each-value loop. */
		if (!scmback_visit_for_v_block(block))
			return false;
	}

	return true;
}

/* Visit a ranged-for block. (e.g., "for (i in 0..10)" */
static bool
scmback_visit_for_range_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(block->val.for_.is_ranged);

	b = block->val.for_.inner;
	if (b == NULL)
		return true;

	/*
	 * Wrap the whole loop with call/cc so that (%break 0) can escape
	 * from anywhere inside the loop body.
	 */
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%break)\n");
	indent++;

	PUT_INDENT();
	PUT1("(let %%noct-loop ((%s ", block->val.for_.counter_symbol);
	if (!scmback_visit_expr(block->val.for_.start)) return false;
	PUT("))\n");
	indent++;
	PUT_INDENT();
	PUT1("(if (< %s ", block->val.for_.counter_symbol);
	if (!scmback_visit_expr(block->val.for_.stop)) return false;
	PUT(")\n");
	indent++;
	PUT_INDENT();
	PUT("(begin\n");
	indent++;
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%continue)\n");
	indent++;
	PUT_INDENT();
	PUT("(begin\n");
	indent++;
	loop_nest++;
	while (b != NULL) {
		if (!scmback_visit_block(b)) return false;
		if (b->stop)
			break;
		b = b->succ;
	}
	loop_nest--;
	PUT_INDENT();
	PUT("0\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	PUT_INDENT();
	PUT1("(%%noct-loop (+ %s 1))\n", block->val.for_.counter_symbol);
	indent--;
	PUT_INDENT();
	PUT(")\n");
	PUT_INDENT();
	PUT("0\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a for-key-value block. */
static bool
scmback_visit_for_kv_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);

	b = block->val.for_.inner;
	if (b == NULL)
		return true;

	/*
	 * noct-for-each-kv itself is not break-aware, so wrap the call in
	 * call/cc and let (%break 0) escape from the callback.
	 */
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%break)\n");
	indent++;

	PUT_INDENT();
	PUT("(noct-for-each-kv\n");
	indent++;
	PUT_INDENT();
	PUT2("(lambda (%s %s)\n", block->val.for_.key_symbol, block->val.for_.value_symbol);
	indent++;
	PUT_INDENT();
	PUT("(begin\n");
	indent++;
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%continue)\n");
	indent++;
	loop_nest++;
	while (b != NULL) {
		if (!scmback_visit_block(b)) return false;
		if (b->stop)
			break;
		b = b->succ;
	}
	loop_nest--;
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	if (!scmback_visit_expr(block->val.for_.collection)) return false;
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a for-value block. */
static bool
scmback_visit_for_v_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);

	b = block->val.for_.inner;
	if (b == NULL)
		return true;

	/*
	 * noct-for-each itself is not break-aware, so wrap the call in
	 * call/cc and let (%break 0) escape from the callback.
	 */
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%break)\n");
	indent++;

	PUT_INDENT();
	PUT("(noct-for-each\n");
	indent++;
	PUT_INDENT();
	PUT1("(lambda (%s)\n", block->val.for_.value_symbol);
	indent++;
	PUT_INDENT();
	PUT("(begin\n");
	indent++;
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%continue)\n");
	indent++;
	loop_nest++;
	while (b != NULL) {
		if (!scmback_visit_block(b)) return false;
		if (b->stop)
			break;
		b = b->succ;
	}
	loop_nest--;
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	PUT_INDENT();
	if (!scmback_visit_expr(block->val.for_.collection)) return false;
	PUT("\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a "while" block. */
static bool
scmback_visit_while_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_WHILE);

	b = block->val.while_.inner;
	if (b == NULL)
		return true;

	/*
	 * Wrap the whole loop with call/cc so that (%break 0) can escape
	 * from anywhere inside the loop body.
	 */
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%break)\n");
	indent++;

	PUT_INDENT();
	PUT("(let %%noct-loop ()\n");
	indent++;
	PUT_INDENT();
	PUT("(if ");
	if (!scmback_visit_expr(block->val.while_.cond)) return false;
	PUT("\n");
	indent++;
	PUT_INDENT();
	PUT("(begin\n");
	indent++;
	PUT_INDENT();
	PUT("(call/cc\n");
	indent++;
	PUT_INDENT();
	PUT("(lambda (%%continue)\n");
	indent++;
	loop_nest++;
	while (b != NULL) {
		if (!scmback_visit_block(b)) return false;
		if (b->stop)
			break;
		b = b->succ;
	}
	loop_nest--;
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	PUT_INDENT();
	PUT("(%%noct-loop)\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a statement. */
static bool
scmback_visit_stmt(
	struct hir_stmt *stmt)
{
	assert(stmt != NULL);
	assert(stmt->rhs != NULL);

	if (stmt->lhs != NULL) {
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			assert(stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL);
			PUT_INDENT();
			if (strcmp(stmt->lhs->val.term.term->val.symbol, "$return") == 0) {
				PUT("(%return ");
				if (!scmback_visit_expr(stmt->rhs))
					return false;
				PUT(")\n");
			} else {
				if (is_local_name(stmt->lhs->val.term.term->val.symbol) ||
				    is_parameter_name(stmt->lhs->val.term.term->val.symbol)) {
					PUT1("(set! %s ", stmt->lhs->val.term.term->val.symbol);
				} else {
					PUT1("(noct-global-set! '%s ", stmt->lhs->val.term.term->val.symbol);
				}
				if (!scmback_visit_expr(stmt->rhs))
					return false;
				PUT(")\n");
			}
		} else if (stmt->lhs->type == HIR_EXPR_SUBSCR) {
			struct hir_expr *base;

			base = stmt->lhs->val.binary.expr[0];
			PUT_INDENT();

			/*
			 * For list-backed arrays, noct-subscript-set! returns the
			 * updated collection.  If the base is a simple symbol, store
			 * the result back into that symbol.
			 */
			if (base->type == HIR_EXPR_TERM &&
			    base->val.term.term->type == HIR_TERM_SYMBOL) {
				const char *symbol = base->val.term.term->val.symbol;

				if (is_local_name(symbol) || is_parameter_name(symbol)) {
					PUT1("(set! %s ", symbol);
				} else {
					PUT1("(noct-global-set! '%s ", symbol);
				}

				PUT("(noct-subscript-set! ");
				if (!scmback_visit_expr(base)) return false;
				PUT(" ");
				if (!scmback_visit_expr(stmt->lhs->val.binary.expr[1])) return false;
				PUT(" ");
				if (!scmback_visit_expr(stmt->rhs)) return false;
				PUT(")");
				PUT(")\n");
			} else {
				PUT("(noct-subscript-set! ");
				if (!scmback_visit_expr(base)) return false;
				PUT(" ");
				if (!scmback_visit_expr(stmt->lhs->val.binary.expr[1])) return false;
				PUT(" ");
				if (!scmback_visit_expr(stmt->rhs)) return false;
				PUT(")\n");
			}
		} else if (stmt->lhs->type == HIR_EXPR_DOT) {
			PUT_INDENT();
			PUT("(noct-dot-set! ");
			if (!scmback_visit_expr(stmt->lhs->val.dot.obj)) return false;
			PUT1(" \"%s\" ", stmt->lhs->val.dot.symbol);
			if (!scmback_visit_expr(stmt->rhs)) return false;
			PUT(")\n");
		} else {
			scmback_fatal(N_TR("LHS is not a symbol or an array element."));
			return false;
		}
	}

	PUT_INDENT();
	if (!scmback_visit_expr(stmt->rhs))
		return false;

	return true;
}

/* Visit an expression. */
static bool
scmback_visit_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);

	switch (expr->type) {
	case HIR_EXPR_TERM:
		/* Visit a term inside the expr. */
		if (!scmback_visit_term(expr->val.term.term))
			return false;
		break;
	case HIR_EXPR_PAR:
		assert(NOT_IMPLEMENTED);
		/* Visit an expr inside the expr. */
		PUT("(");
		if (!scmback_visit_expr(expr->val.unary.expr))
			return false;
		PUT(")");
		break;
	case HIR_EXPR_NOT:
	case HIR_EXPR_NEG:
		/* For unary operators. */
		if (!scmback_visit_unary_expr(expr))
			return false;
		break;
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SUBSCR:
		/* For the binary operators. */
		if (!scmback_visit_binary_expr(expr))
			return false;
		break;
	case HIR_EXPR_SHL:
		PUT("(ash ");
		if (!scmback_visit_expr(expr->val.binary.expr[0]))
			return false;
		PUT(" ");
		if (!scmback_visit_expr(expr->val.binary.expr[1]))
			return false;
		PUT(")");
		break;
	case HIR_EXPR_SHR:
		PUT("(ash ");
		if (!scmback_visit_expr(expr->val.binary.expr[0]))
			return false;
		PUT(" (- ");
		if (!scmback_visit_expr(expr->val.binary.expr[1]))
			return false;
		PUT("))");
		break;
	case HIR_EXPR_DOT:
		/* For the dot operator. */
		if (!scmback_visit_dot_expr(expr))
			return false;
		break;
	case HIR_EXPR_CALL:
		/* For a function call. */
		if (!scmback_visit_call_expr(expr))
			return false;
		break;
	case HIR_EXPR_THISCALL:
		/* For a method call. */
		if (!scmback_visit_thiscall_expr(expr))
			return false;
		break;
	case HIR_EXPR_ARRAY:
		/* For an array expression. */
		if (!scmback_visit_array_expr(expr))
			return false;
		break;
	case HIR_EXPR_DICT:
		/* For a dictionary expression. */
		if (!scmback_visit_dict_expr(expr))
			return false;
		break;
	case HIR_EXPR_NEW:
		/* For a new expression. */
		if (!scmback_visit_new_expr(expr))
			return false;
		break;
	default:
		printf("unimplemented expr: %d\n", expr->type);
		assert(NEVER_COME_HERE);
		abort();
		break;
	}

	return true;
}

/* Visit an unary expression. */
static bool
scmback_visit_unary_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_NEG || expr->type == HIR_EXPR_NOT);

	switch (expr->type) {
	case HIR_EXPR_NEG:
		PUT("(- ");
		if (!scmback_visit_expr(expr->val.unary.expr))
			return false;
		PUT(")");
		break;
	case HIR_EXPR_NOT:
		PUT("(not ");
		if (!scmback_visit_expr(expr->val.unary.expr))
			return false;
		PUT(")");
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	return true;
}

/* Visit a binary expression. */
static bool
scmback_visit_binary_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);

	/* Put the operator. */
	switch (expr->type) {
	case HIR_EXPR_LT:
		PUT("(< ");
		break;
	case HIR_EXPR_LTE:
		PUT("(<= ");
		break;
	case HIR_EXPR_EQ:
		PUT("(noct-eq? ");
		break;
	case HIR_EXPR_NEQ:
		PUT("(not (noct-eq? ");
		break;
	case HIR_EXPR_GTE:
		PUT("(>= ");
		break;
	case HIR_EXPR_GT:
		PUT("(> ");
		break;
	case HIR_EXPR_PLUS:
		PUT("(noct-plus ");
		break;
	case HIR_EXPR_MINUS:
		PUT("(- ");
		break;
	case HIR_EXPR_MUL:
		PUT("(* ");
		break;
	case HIR_EXPR_DIV:
		PUT("(noct-div ");
		break;
	case HIR_EXPR_MOD:
		PUT("(modulo ");
		break;
	case HIR_EXPR_AND:
		PUT("(noct-and ");
		break;
	case HIR_EXPR_OR:
		PUT("(noct-or ");
		break;
	case HIR_EXPR_XOR:
		PUT("(logxor ");
		break;
	case HIR_EXPR_SUBSCR:
		PUT("(noct-subscript-ref ");
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	/* Put the two operands. */
	if (!scmback_visit_expr(expr->val.binary.expr[0]))
		return false;
	PUT(" ");
	if (!scmback_visit_expr(expr->val.binary.expr[1]))
		return false;

	/* Add an extra ) for a not. */
	if (expr->type == HIR_EXPR_NEQ)
		PUT(")");

	/* Close. */
	PUT(")");

	return true;
}

/* Visit a dot expression. */
static bool
scmback_visit_dot_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DOT);
	assert(expr->val.dot.obj != NULL);
	assert(expr->val.dot.symbol != NULL);

	if (strcmp(expr->val.dot.symbol, "length") == 0) {
		PUT("(noct-length ");
		if (!scmback_visit_expr(expr->val.dot.obj))
			return false;
		PUT(")");
	} else {
		PUT("(noct-dot-ref ");
		if (!scmback_visit_expr(expr->val.dot.obj))
			return false;
		PUT1(" \"%s\")", expr->val.dot.symbol);
	}
	return true;
}

/* Visit a call expression. */
static bool
scmback_visit_call_expr(
	struct hir_expr *expr)
{
	uint32_t arg_count, i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_CALL);
	assert(expr->val.call.func != NULL);
	assert(expr->val.call.arg_count < HIR_PARAM_SIZE);

	arg_count = expr->val.call.arg_count;
	
	/* Put (func. */
	PUT("(");
	if (!scmback_visit_expr(expr->val.call.func))
		return false;

	/* Put the arguments. */
	for (i = 0; i < arg_count; i++) {
		PUT(" ");
		if (!scmback_visit_expr(expr->val.call.arg[i]))
			return false;
	}

	/* Close. */
	PUT(")");

	return true;
}

/* Visit a thiscall expression. */
static bool
scmback_visit_thiscall_expr(
	struct hir_expr *expr)
{
	uint32_t arg_count, i;
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_THISCALL);
	assert(expr->val.thiscall.func != NULL);
	assert(expr->val.thiscall.arg_count < HIR_PARAM_SIZE);
	arg_count = expr->val.thiscall.arg_count;

	if (strcmp(expr->val.thiscall.func, "push") == 0) {
		assert(expr->val.thiscall.arg_count == 1);
		PUT("(noct-array-push ");
		if (!scmback_visit_expr(expr->val.thiscall.obj))
			return false;
		PUT(" ");
		if (!scmback_visit_expr(expr->val.thiscall.arg[0]))
			return false;
		PUT(")");
		return true;
	}
	if (strcmp(expr->val.thiscall.func, "pop") == 0) {
		assert(expr->val.thiscall.arg_count == 0);
		PUT("(noct-array-pop ");
		if (!scmback_visit_expr(expr->val.thiscall.obj))
			return false;
		PUT(")");
		return true;
	}
	if (strcmp(expr->val.thiscall.func, "resize") == 0) {
		assert(expr->val.thiscall.arg_count == 1);
		PUT("(noct-array-resize ");
		if (!scmback_visit_expr(expr->val.thiscall.obj))
			return false;
		PUT(" ");
		if (!scmback_visit_expr(expr->val.thiscall.arg[0]))
			return false;
		PUT(")");
		return true;
	}
	if (strcmp(expr->val.thiscall.func, "charAt") == 0) {
		assert(expr->val.thiscall.arg_count == 1);
		PUT("(string (string-ref ");
		if (!scmback_visit_expr(expr->val.thiscall.obj)) return false;
		PUT(" ");
		if (!scmback_visit_expr(expr->val.thiscall.arg[0])) return false;
		PUT("))");
		return true;
	}

	PUT("((noct-dot-ref ");
	if (!scmback_visit_expr(expr->val.thiscall.obj))
		return false;
	PUT1(" \"%s\")", expr->val.thiscall.func);
	for (i = 0; i < arg_count; i++) {
		PUT(" ");
		if (!scmback_visit_expr(expr->val.thiscall.arg[i]))
			return false;
	}
	PUT(" ");
	if (!scmback_visit_expr(expr->val.thiscall.obj))
		return false;
	PUT(")");
	return true;
}

/* Visit an array expression. (e.g., "[1 2 3]") */
static bool
scmback_visit_array_expr(
	struct hir_expr *expr)
{
	uint32_t elem_count, i;
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_ARRAY);
	assert(expr->val.array.elem_count > 0);
	elem_count = expr->val.array.elem_count;
	PUT("(list");
	for (i = 0; i < elem_count; i++) {
		PUT(" ");
		if (!scmback_visit_expr(expr->val.array.elem[i])) return false;
	}
	PUT(")");
	return true;
}

/* Visit a dictionary expression. (e.g., "{key: value}") */
static bool
scmback_visit_dict_expr(
	struct hir_expr *expr)
{
	uint32_t kv_count, i;
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DICT);
	kv_count = expr->val.dict.kv_count;
	PUT("(noct-dict");
	for (i = 0; i < kv_count; i++) {
		PUT1(" (cons \"%s\" ", expr->val.dict.key[i]);
		if (!scmback_visit_expr(expr->val.dict.value[i])) return false;
		PUT(")");
	}
	PUT(")");
	return true;
}

/* Visit a new expression. */
static bool
scmback_visit_new_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_NEW);
	PUT1("(noct-new %s (noct-dict", expr->val.new_.cls);
	if (expr->val.new_.init != NULL) {
		uint32_t i, kv_count = expr->val.new_.init->val.dict.kv_count;
		for (i = 0; i < kv_count; i++) {
			PUT1(" (cons \"%s\" ", expr->val.new_.init->val.dict.key[i]);
			if (!scmback_visit_expr(expr->val.new_.init->val.dict.value[i])) return false;
			PUT(")");
		}
	}
	PUT("))");
	return true;
}

/* Visit a term. */
static bool
scmback_visit_term(
	struct hir_term *term)
{
	assert(term != NULL);
	switch (term->type) {
	case HIR_TERM_SYMBOL:
		if (!scmback_put(func_name_sanitizer(term->val.symbol))) return false;
		break;
	case HIR_TERM_INT:
		if (!scmback_put("%d", term->val.i)) return false;
		break;
	case HIR_TERM_FLOAT:
		if (!scmback_put("%f", term->val.f)) return false;
		break;
	case HIR_TERM_STRING:
		if (!scmback_put_string_literal(term->val.s)) return false;
		break;
	case HIR_TERM_EMPTY_ARRAY:
		if (!scmback_put("(list)")) return false;
		break;
	case HIR_TERM_EMPTY_DICT:
		if (!scmback_put("(noct-dict)")) return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}
	return true;
}

/* Put a Scheme string literal. */
static bool
scmback_put_string_literal(
	const char *s)
{
	const unsigned char *p;
	PUT("\"");
	for (p = (const unsigned char *)s; *p != '\0'; p++) {
		switch (*p) {
		case '\\': PUT("\\\\"); break;
		case '"': PUT("\\\""); break;
		case '\n': PUT("\\n"); break;
		case '\r': PUT("\\r"); break;
		case '\t': PUT("\t"); break;
		default:
			if (*p < 0x20) {
				PUT1("\\x%02x;", *p);
			} else {
				char buf[2];
				buf[0] = (char)*p;
				buf[1] = '\0';
				PUT1("%s", buf);
			}
			break;
		}
	}
	PUT("\"");
	return true;
}

/* Put a translated string. */
static bool
scmback_put(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(fp, format, ap);
	va_end(ap);

	return true;

#if 0
	char buf[8192];
	va_list ap;
	size_t add_len, rem_len;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	add_len = strlen(buf);
	rem_len = body_alloc_size - body_len - 1;
	if (add_len > rem_len) {
		body = realloc(buf, body_alloc_size + ALLOC_SIZE);
		if (body == NULL) {
			scmback_out_of_memory();
			return false;
		}
		body_alloc_size += ALLOC_SIZE;
	}

	strcat(body, buf);
	body_len += add_len;

	return true;
#endif
}

/* Put a translated string. */
static bool
scmback_put_indent(void)
{
	int i;

	for (i = 0; i < indent; i++)
		fprintf(fp, "  ");

	return true;
}


/* Set an error message. */
static void
scmback_fatal(
	const char *msg,
	...)
{
	va_list ap;

	va_start(ap, msg);
	vsnprintf(scmback_error_message,
		  sizeof(scmback_error_message),
		  msg,
		  ap);
	va_end(ap);
}

/* Out of memory. */
static void
scmback_out_of_memory(void)
{
	printf("Out of memory.\n");
}
