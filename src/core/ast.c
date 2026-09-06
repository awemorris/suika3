/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * AST: Abstract Syntax Tree
 */

#include <noct/noct.h>
#include "ast.h"
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/* False assertions. */
#define NEVER_COME_HERE		(0)
#define UNIMPLEMENTED		(0)

/* Arena allocator size. */
#if !defined(NOCT_MEMORY_SMALL)
#define ARENA_SIZE		(64 * 1024 * 1024)
#else
#define ARENA_SIZE		(512 * 1024)
#endif

/* Function flags passed by the parser. */
#define AST_FUNC_STATIC 1
#define AST_FUNC_INLINE 2
#define AST_FUNC_FAST   4
#define AST_FUNC_ACCEL  8

/* List operation. */
#define AST_ADD_TO_LAST(type, list, p)			\
	do {						\
		if (list == NULL) {			\
			list = p;			\
		} else {				\
			type *elem = list;		\
			while (elem->next)		\
				elem = elem->next;	\
			elem->next = p;			\
		}					\
	} while (0);

/* Constructed AST. */
static struct ast_func_list *ast_func_list;

/* Early declarations (also declared in the parser prologue). */
struct ast_func *ast_accept_func(int flags, char *name, struct ast_param_list *param_list, char *return_type_name, struct ast_stmt_list *stmt_list);
struct ast_func_list *ast_accept_func_list(struct ast_func_list *func_list, struct ast_func *func);
struct ast_expr *ast_accept_term_expr(struct ast_term *term);
struct ast_term *ast_accept_symbol_term(char *symbol);
struct ast_term *ast_accept_str_term(char *str);
struct ast_expr *ast_accept_call_expr(struct ast_expr *expr1, struct ast_arg_list *arg_list);
char *ast_accept_type_extent_int(int64_t value);
char *ast_accept_type_extent_list(char *list, char *extent);
char *ast_accept_shaped_type(char *name, char *extents);
struct ast_stmt *ast_accept_assign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs, bool is_var, bool is_let);
struct ast_expr *ast_accept_class_expr(struct ast_kv_list *kv_list);
struct ast_stmt *ast_accept_expr_stmt(int line, struct ast_expr *expr);
struct ast_stmt *ast_accept_return_stmt(int line, struct ast_expr *expr);
struct ast_stmt_list *ast_accept_stmt_list(struct ast_stmt_list *stmt_list, struct ast_stmt *stmt);

/* Top-level declarations, synthesized into $init.<file>. */
static struct ast_stmt_list *ast_init_stmt_list;

/* Top-level require declarations. */
static struct ast_require *ast_require_list;
static struct ast_require *ast_require_tail;
static uint32_t ast_require_count;

/* Implicitly file-local package initializer selected by @package paths. */
static char *ast_package_init_name;

/* File name. */
static char *ast_file_name;

struct ast_static_symbol {
	char *source_name;
	char *link_name;
	struct ast_static_symbol *next;
};
static struct ast_static_symbol *ast_static_symbols;
static struct ast_static_symbol *ast_declared_symbols;

/*
 * Error position and message. (set by the parser)
 */
int ast_error_line;
int ast_error_column;
char ast_error_message[1024];

/*
 * Arena allocator.
 */
static struct arena_info ast_arena;

/*
 * Lexer and Parser
 */
typedef void *yyscan_t;
int ast_yylex_init(yyscan_t *scanner);
int ast_yy_scan_string(const char *yystr, yyscan_t scanner);
int ast_yylex_destroy(yyscan_t scanner);
int ast_yyparse(yyscan_t scanner);

/* Forward Declarations */
static struct ast_stmt *ast_accept_xassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs, int type);
static struct ast_stmt *ast_accept_plusplus_or_minusminus_stmt(int line, struct ast_expr *expr,int type);
static struct ast_expr *ast_accept_binary_expr(struct ast_expr *expr1, struct ast_expr *expr2, int type);
static struct ast_expr *ast_accept_unary_expr(struct ast_expr *e, int type);
static void ast_out_of_memory(void);
void *ast_malloc(size_t size);
char *ast_strdup(const char *s);
void ast_free(void *p);
#if 0
static void ast_free_func_list(struct ast_func_list *func_list);
static void ast_free_func(struct ast_func *func);
static void ast_free_arg_list(struct ast_arg_list *arg_list);
static void ast_free_param(struct ast_param *param);
static void ast_free_stmt_list(struct ast_stmt_list *stmt_list);
static void ast_free_stmt(struct ast_stmt *stmt);
static void ast_free_expr(struct ast_expr *expr);
static void ast_free_kv_list(struct ast_kv_list *kv_list);
static void ast_free_kv(struct ast_kv *kv);
static void ast_free_term(struct ast_term *term);
#endif

static struct ast_expr *ast_copy_expr(struct ast_expr *expr);
static struct ast_term *ast_copy_term(struct ast_term *term);
static struct ast_arg_list *ast_copy_arg_list(struct ast_arg_list *arg_list);
static struct ast_kv_list *ast_copy_kv_list(struct ast_kv_list *kv_list);
static void ast_printf(const char *format, ...);
static void ast_out_of_memory(void);
void *ast_malloc(size_t size);
void *ast_realloc(void *p, size_t size);
char *ast_strdup(const char *s);
void ast_free(void *p);

/*
 * Builds one canonical integer extent spelling.
 */
char *
ast_accept_type_extent_int(
	int64_t value)
{
	char buf[32];
	char *result;

	snprintf(buf, sizeof(buf), "%" PRId64, value);
	result = ast_strdup(buf);
	if (result == NULL)
		ast_out_of_memory();

	return result;
}

/*
 * Appends one extent to a canonical extent list.
 */
char *
ast_accept_type_extent_list(
	char *list,
	char *extent)
{
	char *result;
	size_t size;

	size = strlen(list) + strlen(extent) + 2;
	result = ast_malloc(size);
	if (result == NULL) {
		ast_free(list);
		ast_free(extent);
		ast_out_of_memory();
		return NULL;
	}

	snprintf(result, size, "%s,%s", list, extent);
	ast_free(list);
	ast_free(extent);

	return result;
}

/*
 * Builds one canonical shaped type annotation.
 */
char *
ast_accept_shaped_type(
	char *name,
	char *extents)
{
	char *result;
	size_t size;

	size = strlen(name) + strlen(extents) + 3;
	result = ast_malloc(size);
	if (result == NULL) {
		ast_free(name);
		ast_free(extents);
		ast_out_of_memory();
		return NULL;
	}

	snprintf(result, size, "%s(%s)", name, extents);
	ast_free(name);
	ast_free(extents);

	return result;
}

/*
 * Build an AST from a script string.
 */
bool
ast_build(
	const char *file_name,
	const char *text)
{
	yyscan_t scanner;

	assert(file_name != NULL);
	assert(text != NULL);

	/* Initialize the arena allocator. */
	if (!arena_init(&ast_arena, ARENA_SIZE)) {
		ast_out_of_memory();
		return false;
	}

	/* Reset the per-compilation lists. */
	ast_func_list = NULL;
	ast_init_stmt_list = NULL;
	ast_static_symbols = NULL;
	ast_declared_symbols = NULL;
	ast_require_list = NULL;
	ast_require_tail = NULL;
	ast_require_count = 0;
	ast_package_init_name = NULL;
	ast_error_message[0] = '\0';

	/* Copy the file name. */

	ast_file_name = ast_strdup(file_name);
	if (ast_file_name == NULL)
		return false;

	/* Parse the text by using the parser. */
	ast_yylex_init(&scanner);
	ast_yy_scan_string(text, scanner);
	if (ast_yyparse(scanner) != 0) {
		ast_yylex_destroy(scanner);
		return false;
	}
	ast_yylex_destroy(scanner);

	/* A require-only module still needs a valid, empty compilation unit. */
	if (ast_require_count != 0 && ast_func_list == NULL &&
	    ast_init_stmt_list == NULL) {
		ast_init_stmt_list = ast_accept_stmt_list(
			NULL, ast_accept_return_stmt(0, NULL));
		if (ast_init_stmt_list == NULL)
			return false;
	}

	/* Synthesize the $init.<file> function from top-level decls. */
	if (ast_init_stmt_list != NULL) {
		struct ast_func *init_func;
		char *init_name;
		size_t len;

		len = strlen(ast_file_name) + 16;
		init_name = ast_malloc(len);
		if (init_name == NULL) {
			ast_out_of_memory();
			return false;
		}
		snprintf(init_name, len, "$init.%s", ast_file_name);
		init_func = ast_accept_func(0, init_name, NULL, NULL, ast_init_stmt_list);
		if (init_func == NULL)
			return false;
		if (ast_accept_func_list(ast_func_list, init_func) == NULL)
			return false;
		ast_init_stmt_list = NULL;
	}

	return true;
}

bool
ast_build_app_initializer(
	const char *file_name,
	const char *func_name,
	const char *const *init_name,
	uint32_t init_count)
{
	struct ast_stmt_list *stmt_list;
	struct ast_func *func;
	uint32_t i;

	if (!arena_init(&ast_arena, ARENA_SIZE)) {
		ast_out_of_memory();
		return false;
	}
	ast_func_list = NULL;
	ast_init_stmt_list = NULL;
	ast_static_symbols = NULL;
	ast_declared_symbols = NULL;
	ast_require_list = NULL;
	ast_require_tail = NULL;
	ast_require_count = 0;
	ast_package_init_name = NULL;
	ast_error_message[0] = '\0';
	ast_file_name = ast_strdup(file_name);
	if (ast_file_name == NULL)
		return false;
	stmt_list = NULL;
	for (i = 0; i < init_count; i++) {
		struct ast_expr *callee;
		struct ast_expr *call;
		struct ast_stmt *stmt;

		callee = ast_accept_term_expr(
			ast_accept_symbol_term(ast_strdup(init_name[i])));
		if (callee == NULL)
			return false;
		call = ast_accept_call_expr(callee, NULL);
		stmt = ast_accept_expr_stmt(0, call);
		stmt_list = ast_accept_stmt_list(stmt_list, stmt);
		if (stmt_list == NULL)
			return false;
	}
	stmt_list = ast_accept_stmt_list(stmt_list,
					 ast_accept_return_stmt(0, NULL));
	if (stmt_list == NULL)
		return false;
	func = ast_accept_func(0, ast_strdup(func_name), NULL,
			       ast_strdup("void"), stmt_list);
	if (func == NULL || ast_accept_func_list(NULL, func) == NULL)
		return false;
	return true;
}

static bool
ast_file_name_is_relative(void)
{
	return ast_file_name[0] != '/' && ast_file_name[0] != '\\' &&
	       !(ast_file_name[0] != '\0' && ast_file_name[1] == ':');
}

static char *
ast_make_static_name(const char *name)
{
	static const char hex[] = "0123456789abcdef";
	size_t file_len;
	size_t name_len;
	size_t i;
	char *mangled;
	char *p;

	if (!ast_file_name_is_relative()) {
		ast_printf(N_TR("static declarations require a relative source path."));
		return NULL;
	}
	file_len = strlen(ast_file_name);
	name_len = strlen(name);
	mangled = ast_malloc(9 + file_len * 2 + 1 + name_len * 2 + 1);
	if (mangled == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memcpy(mangled, "$static.", 8);
	p = mangled + 8;
	for (i = 0; i < file_len; i++) {
		unsigned char c = (unsigned char)ast_file_name[i];
		*p++ = hex[c >> 4];
		*p++ = hex[c & 15];
	}
	*p++ = '.';
	for (i = 0; i < name_len; i++) {
		unsigned char c = (unsigned char)name[i];
		*p++ = hex[c >> 4];
		*p++ = hex[c & 15];
	}
	*p = '\0';
	return mangled;
}

static char *
ast_register_static_symbol(char *name)
{
	struct ast_static_symbol *entry;

	for (entry = ast_static_symbols; entry != NULL; entry = entry->next) {
		if (strcmp(entry->source_name, name) == 0) {
			ast_printf(N_TR("Duplicate static declaration '%s'."), name);
			return NULL;
		}
	}
	entry = ast_malloc(sizeof(*entry));
	if (entry == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(entry, 0, sizeof(*entry));
	entry->source_name = name;
	entry->link_name = ast_make_static_name(name);
	if (entry->link_name == NULL)
		return NULL;
	entry->next = ast_static_symbols;
	ast_static_symbols = entry;
	return entry->link_name;
}

static bool
ast_is_package_entry(void)
{
	const char *name;
	const char *file;
	const char *slash;
	const char *dot;
	size_t name_len;

	if (ast_file_name == NULL || strncmp(ast_file_name, "@package/", 9) != 0)
		return false;
	name = ast_file_name + 9;
	slash = strchr(name, '/');
	if (slash == NULL || slash == name)
		return false;
	file = slash + 1;
	dot = strrchr(file, '.');
	if (dot == NULL ||
	    (strcmp(dot, ".noct") != 0 && strcmp(dot, ".nct") != 0))
		return false;
	name_len = (size_t)(slash - name);
	return (size_t)(dot - file) == name_len &&
	       strncmp(name, file, name_len) == 0;
}

static bool
ast_register_declared_symbol(char *name)
{
	struct ast_static_symbol *entry;

	if (strncmp(name, "__noct_nap_", 11) == 0) {
		ast_printf(N_TR("The '__noct_nap_' symbol prefix is reserved."));
		return false;
	}
	for (entry = ast_declared_symbols; entry != NULL; entry = entry->next) {
		if (strcmp(entry->source_name, name) == 0) {
			ast_printf(N_TR("Duplicate top-level declaration '%s'."), name);
			return false;
		}
	}
	entry = ast_malloc(sizeof(*entry));
	if (entry == NULL) {
		ast_out_of_memory();
		return false;
	}
	memset(entry, 0, sizeof(*entry));
	entry->source_name = name;
	entry->next = ast_declared_symbols;
	ast_declared_symbols = entry;
	return true;
}

const char *
ast_resolve_static_symbol(const char *name)
{
	struct ast_static_symbol *entry;

	for (entry = ast_static_symbols; entry != NULL; entry = entry->next) {
		if (strcmp(entry->source_name, name) == 0)
			return entry->link_name;
	}
	return name;
}

/* Record a top-level source dependency. */
bool
ast_accept_require(char *name)
{
	struct ast_require *req;
	struct ast_require *p;

	for (p = ast_require_list; p != NULL; p = p->next) {
		if (strcmp(p->name, name) == 0)
			return true;
	}
	req = ast_malloc(sizeof(*req));
	if (req == NULL) {
		ast_out_of_memory();
		return false;
	}
	memset(req, 0, sizeof(*req));
	req->name = name;
	if (ast_require_tail != NULL)
		ast_require_tail->next = req;
	else
		ast_require_list = req;
	ast_require_tail = req;
	ast_require_count++;
	return true;
}

uint32_t
ast_get_require_count(void)
{
	return ast_require_count;
}

const char *
ast_get_require_name(uint32_t index)
{
	struct ast_require *req;

	for (req = ast_require_list; req != NULL && index != 0;
	     req = req->next)
		index--;
	return req != NULL ? req->name : NULL;
}

/*
 * Free an AST.
 */
void
ast_cleanup(void)
{
#if 0
	if (ast_func_list != NULL) {
		ast_free_func_list(ast_func_list);
		ast_func_list = NULL;
	}
	if (ast_file_name != NULL) {
		ast_free(ast_file_name);
		ast_file_name = NULL;
	}
#endif
	arena_cleanup(&ast_arena);
}

/*
 * Get an AST.
 */
struct ast_func_list *
ast_get_func_list(void)
{
	assert(ast_func_list != NULL);

	return ast_func_list;
}

/*
 * Get the file name.
 */
const char *
ast_get_file_name(void)
{
	assert(ast_file_name != NULL);

	return ast_file_name;
}

/*
 * Get the error message.
 */
const char *
ast_get_error_message(void)
{
	return &ast_error_message[0];
}

/*
 * Get the error line.
 */
int
ast_get_error_line(void)
{
	return ast_error_line;
}


/* Append a statement to the synthesized $init statement list. */
static bool
ast_append_init_stmt(struct ast_stmt *stmt)
{
	if (stmt == NULL)
		return false;
	if (ast_init_stmt_list == NULL) {
		ast_init_stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
		if (ast_init_stmt_list == NULL) {
			ast_out_of_memory();
			return false;
		}
		memset(ast_init_stmt_list, 0, sizeof(struct ast_stmt_list));
		ast_init_stmt_list->list = stmt;
	} else {
		AST_ADD_TO_LAST(struct ast_stmt, ast_init_stmt_list->list, stmt);
	}
	return true;
}

/* Build a "Global.markConst(\"name\")" expression statement. */
static struct ast_stmt *
ast_make_mark_const_stmt(int line, const char *name)
{
	struct ast_expr *fn;
	struct ast_expr *arg;
	struct ast_expr *call;
	struct ast_arg_list *args;
	struct ast_stmt *stmt;
	char *name_dup;

	fn = ast_accept_term_expr(ast_accept_symbol_term(ast_strdup("Global.markConst")));
	name_dup = ast_strdup(name);
	if (fn == NULL || name_dup == NULL)
		return NULL;
	arg = ast_accept_term_expr(ast_accept_str_term(name_dup));
	if (arg == NULL)
		return NULL;
	args = ast_malloc(sizeof(struct ast_arg_list));
	if (args == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(args, 0, sizeof(struct ast_arg_list));
	args->list = arg;
	call = ast_accept_call_expr(fn, args);
	if (call == NULL)
		return NULL;
	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_EXPR;
	stmt->val.expr.expr = call;
	stmt->line = line;
	return stmt;
}

/*
 * Called from the parser for a top-level "var"/"let" declaration.
 * Lowered as a PLAIN assignment (creates a global at load time) plus,
 * for let, a Global.markConst call.  See docs/design/03-class.md.
 */
bool
ast_accept_toplevel_var(int line, char *name, struct ast_expr *rhs, bool is_let,
			bool is_static)
{
	struct ast_expr *lhs;
	struct ast_stmt *stmt;

	if (!ast_register_declared_symbol(name))
		return false;
	if (is_static) {
		name = ast_register_static_symbol(name);
		if (name == NULL)
			return false;
	}
	lhs = ast_accept_term_expr(ast_accept_symbol_term(name));
	if (lhs == NULL)
		return false;
	stmt = ast_accept_assign_stmt(line, lhs, rhs, false, false);
	if (stmt == NULL)
		return false;
	if (!ast_append_init_stmt(stmt))
		return false;
	if (is_let) {
		if (!ast_append_init_stmt(ast_make_mark_const_stmt(line, name)))
			return false;
	}
	return true;
}

/*
 * Called from the parser for a top-level "class Name { ... }".
 * Lowered as: Name = class { ... }; Global.markConst("Name");
 */
bool
ast_accept_toplevel_class(int line, char *name, struct ast_kv_list *kv_list)
{
	struct ast_expr *cls;

	cls = ast_accept_class_expr(kv_list);
	if (cls == NULL)
		return false;
	if (!ast_accept_toplevel_var(line, name, cls, true, false))
		return false;
	return true;
}

/* Called from the parser when it accepted a func_list. */
struct ast_func_list *
ast_accept_func_list(
	struct ast_func_list *func_list,
	struct ast_func *func)
{
	assert(func != NULL);

	if (func_list == NULL) {
		/* If this is the first element, allocate a list. */
		func_list = ast_malloc(sizeof(struct ast_func_list));
		if (func_list == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(func_list, 0, sizeof(struct ast_func_list));

		/* Set a func to the list top. */
		func_list->list = func;

		/* Set the func_list to the AST root. */
		ast_func_list = func_list;
	} else {
		/* Add a func to the list tail. */
		AST_ADD_TO_LAST(struct ast_func, func_list->list, func);
	}

	return func_list;
}

/* Called from the parser when it accepted a func. */
struct ast_func *
ast_accept_func(
	int flags,
	char *name,
	struct ast_param_list *param_list,
	char *return_type_name,
	struct ast_stmt_list *stmt_list)
{
	struct ast_func *f;
	bool is_package_init;

	assert(name != NULL);
	is_package_init = strcmp(name, "package_init") == 0 &&
			  ast_is_package_entry();
	if (is_package_init) {
		if (param_list != NULL || return_type_name == NULL ||
		    strcmp(return_type_name, "void") != 0) {
			ast_printf(N_TR("package_init() must have no parameters and declare ': void'."));
			return NULL;
		}
		flags |= AST_FUNC_STATIC;
	}
	if (!ast_register_declared_symbol(name))
		return NULL;

	if ((flags & AST_FUNC_INLINE) != 0 && (flags & AST_FUNC_STATIC) == 0) {
		ast_printf(N_TR("__inline functions must also be static."));
		return NULL;
	}
	if ((flags & AST_FUNC_STATIC) != 0) {
		name = ast_register_static_symbol(name);
		if (name == NULL)
			return NULL;
		if (is_package_init)
			ast_package_init_name = name;
	}

	/* Allocate a func. */
	f = ast_malloc(sizeof(struct ast_func));
	if (f == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(f, 0, sizeof(struct ast_func));
	f->name = name;
	f->param_list = param_list;
	f->return_type_name = return_type_name;
	f->is_static = (flags & AST_FUNC_STATIC) != 0;
	f->is_inline = (flags & AST_FUNC_INLINE) != 0;
	f->is_fast = (flags & AST_FUNC_FAST) != 0;
	f->is_accel = (flags & AST_FUNC_ACCEL) != 0;
	f->stmt_list = stmt_list;

	return f;
}

const char *
ast_get_package_init_name(void)
{
	return ast_package_init_name;
}

/* Called from the parser when it accepted a param_list. */
struct ast_param_list *
ast_accept_param_list(
	struct ast_param_list *param_list,
	char *name)
{
	struct ast_param *param;

	assert(name != NULL);

	param = ast_malloc(sizeof(struct ast_param));
	if (param == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(param, 0, sizeof(struct ast_param));
	param->name = name;

	if (param_list == NULL) {
		/* If this is a top param, allocate a list. */
		param_list = ast_malloc(sizeof(struct ast_param_list));
		if (param_list == NULL) {
			ast_free(param);
			ast_out_of_memory();
			return NULL;
		}
		memset(param_list, 0, sizeof(struct ast_param_list));
		param_list->list = param;
	} else {
		/* Add a param to the tail. */
		AST_ADD_TO_LAST(struct ast_param, param_list->list, param);
	}

	return param_list;
}

/* Called from the parser when it accepted an annotated parameter. */
struct ast_param_list *
ast_accept_param_list_typed(
	struct ast_param_list *param_list,
	char *name,
	char *type_name)
{
	struct ast_param_list *l;
	struct ast_param *p;

	l = ast_accept_param_list(param_list, name);
	if (l == NULL)
		return NULL;
	p = l->list;
	while (p->next != NULL)
		p = p->next;
	p->type_name = type_name;
	return l;
}

/* Called from the parser when it accepted a stmt_list. */
struct ast_stmt_list *
ast_accept_stmt_list(
	struct ast_stmt_list *stmt_list,
	struct ast_stmt *stmt)
{
	if (stmt_list == NULL) {
		/* If this is a top element, allocate a list. */
		stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
		if (stmt_list == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(stmt_list, 0, sizeof(struct ast_stmt_list));

		/* Add a stmt to the top. */
		stmt_list->list = stmt;
	} else {
		/* Add a stmt to the tail. */
		AST_ADD_TO_LAST(struct ast_stmt, stmt_list->list, stmt);
	}

	return stmt_list;
}

/* Called from the parser when it accepted a expr_stmt. */
struct ast_stmt *
ast_accept_expr_stmt(
	int line,
	struct ast_expr *expr)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_EXPR;
	stmt->val.expr.expr = expr;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a assign_stmt. */
struct ast_stmt *
ast_accept_assign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs,
	bool is_var,
	bool is_let)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_ASSIGN;
	stmt->val.assign.lhs = lhs;
	stmt->val.assign.rhs = rhs;
	stmt->val.assign.is_var = is_var;
	stmt->val.assign.is_let = is_let;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a typed assign_stmt. */
struct ast_stmt *
ast_accept_assign_stmt_typed(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs,
	bool is_var,
	bool is_let,
	char *type_name)
{
	struct ast_stmt *stmt;

	stmt = ast_accept_assign_stmt(line, lhs, rhs, is_var, is_let);
	if (stmt == NULL)
		return NULL;
	stmt->val.assign.type_name = type_name;
	return stmt;
}

/* Called from the parser when it accepted a plusassign_stmt. */
struct ast_stmt *
ast_accept_plusassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_PLUS);
}

/* Called from the parser when it accepted a minusassign_stmt. */
struct ast_stmt *
ast_accept_minusassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_MINUS);
}

/* Called from the parser when it accepted a mulassign_stmt. */
struct ast_stmt *
ast_accept_mulassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_MUL);
}

/* Called from the parser when it accepted a mulassign_stmt. */
struct ast_stmt *
ast_accept_divassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_DIV);
}

/* Called from the parser when it accepted a modassign_stmt. */
struct ast_stmt *
ast_accept_modassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_MOD);
}

/* Called from the parser when it accepted a andassign_stmt. */
struct ast_stmt *
ast_accept_andassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_AND);
}

/* Called from the parser when it accepted a orassign_stmt. */
struct ast_stmt *
ast_accept_orassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_OR);
}

/* Called from the parser when it accepted a shlassign_stmt. */
struct ast_stmt *
ast_accept_shlassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_SHL);
}

/* Called from the parser when it accepted a orassign_stmt. */
struct ast_stmt *
ast_accept_shrassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs)
{
	return ast_accept_xassign_stmt(line, lhs, rhs, AST_EXPR_SHR);
}

static struct ast_stmt *
ast_accept_xassign_stmt(
	int line,
	struct ast_expr *lhs,
	struct ast_expr *rhs,
	int type)
{
	struct ast_stmt *stmt;
	struct ast_expr *expr, *rhs0;

	rhs0 = ast_copy_expr(lhs);
	if (rhs0 == NULL)
		return NULL;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = type;
	expr->val.binary.expr[0] = rhs0;
	expr->val.binary.expr[1] = rhs;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_ASSIGN;
	stmt->val.assign.lhs = lhs;
	stmt->val.assign.rhs = expr;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a plusplus_stmt. */
struct ast_stmt *
ast_accept_plusplus_stmt(
	int line,
	struct ast_expr *expr)
{
	return ast_accept_plusplus_or_minusminus_stmt(line, expr, AST_EXPR_PLUS);
}

/* Called from the parser when it accepted a minusminus_stmt. */
struct ast_stmt *
ast_accept_minusminus_stmt(
	int line,
	struct ast_expr *expr)
{
	return ast_accept_plusplus_or_minusminus_stmt(line, expr, AST_EXPR_MINUS);
}

static struct ast_stmt *
ast_accept_plusplus_or_minusminus_stmt(
	int line,
	struct ast_expr *expr,
	int type)
{
	struct ast_stmt *stmt;
	struct ast_expr *minus_expr, *lhs, *rhs0, *rhs1;
	struct ast_term *term;

	lhs = ast_copy_expr(expr);
	if (lhs == NULL)
		return NULL;

	rhs0 = ast_copy_expr(expr);
	if (rhs0 == NULL)
		return NULL;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_INT;
	term->val.i = 1;

	rhs1 = ast_malloc(sizeof(struct ast_expr));
	if (rhs1 == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(rhs1, 0, sizeof(struct ast_expr));
	rhs1->type = AST_EXPR_TERM;
	rhs1->val.term.term = term;

	minus_expr = ast_malloc(sizeof(struct ast_expr));
	if (minus_expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(minus_expr, 0, sizeof(struct ast_expr));
	minus_expr->type = type;
	minus_expr->val.binary.expr[0] = rhs0;
	minus_expr->val.binary.expr[1] = rhs1;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_ASSIGN;
	stmt->val.assign.lhs = lhs;
	stmt->val.assign.rhs = minus_expr;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a if_stmt. */
struct ast_stmt *
ast_accept_if_stmt(
	int line,
	struct ast_expr *cond,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_IF;
	stmt->val.if_.cond = cond;
	stmt->val.if_.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a if_stmt. */
struct ast_stmt *
ast_accept_if_stmt_single(
	int line,
	struct ast_expr *cond,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *if_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	if_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (if_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(if_stmt, 0, sizeof(struct ast_stmt));
	if_stmt->type = AST_STMT_IF;
	if_stmt->val.if_.cond = cond;
	if_stmt->val.if_.stmt_list = stmt_list;
	if_stmt->line = line;

	return if_stmt;
}

/* Called from the parser when it accepted a elif_stmt. */
struct ast_stmt *
ast_accept_elif_stmt(
	int line,
	struct ast_expr *cond,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_ELIF;
	stmt->val.elif.cond = cond;
	stmt->val.elif.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a elif_stmt. */
struct ast_stmt *
ast_accept_elif_stmt_single(
	int line,
	struct ast_expr *cond,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *elif_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	elif_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (elif_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(elif_stmt, 0, sizeof(struct ast_stmt));
	elif_stmt->type = AST_STMT_ELIF;
	elif_stmt->val.elif.cond = cond;
	elif_stmt->val.elif.stmt_list = stmt_list;
	elif_stmt->line = line;

	return elif_stmt;
}

/* Called from the parser when it accepted a else_stmt. */
struct ast_stmt *
ast_accept_else_stmt(
	int line,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_ELSE;
	stmt->val.else_.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a else_stmt. */
struct ast_stmt *
ast_accept_else_stmt_single(
	int line,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *else_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	else_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (else_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(else_stmt, 0, sizeof(struct ast_stmt));
	else_stmt->type = AST_STMT_ELSE;
	else_stmt->val.else_.stmt_list = stmt_list;
	else_stmt->line = line;

	return else_stmt;
}

/* Called from the parser when it accepted a while_stmt. */
struct ast_stmt *
ast_accept_while_stmt(
	int line,
	struct ast_expr *cond,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_WHILE;
	stmt->val.while_.cond = cond;
	stmt->val.while_.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a while_stmt. */
struct ast_stmt *
ast_accept_while_stmt_single(
	int line,
	struct ast_expr *cond,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *while_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	while_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (while_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(while_stmt, 0, sizeof(struct ast_stmt));
	while_stmt->type = AST_STMT_WHILE;
	while_stmt->val.while_.cond = cond;
	while_stmt->val.while_.stmt_list = stmt_list;
	while_stmt->line = line;

	return while_stmt;
}

/* Called from the parser when it accepted a for_stmt with for(v in) syntax. */
struct ast_stmt *
ast_accept_for_v_stmt(
	int line,
	char *iter_sym,
	struct ast_expr *array,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_FOR;
	stmt->val.for_.is_range = false;
	stmt->val.for_.value_symbol = iter_sym;
	stmt->val.for_.collection = array;
	stmt->val.for_.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a for_stmt with for(v in) syntax. */
struct ast_stmt *
ast_accept_for_v_stmt_single(
	int line,
	char *iter_sym,
	struct ast_expr *array,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *for_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	for_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (for_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(for_stmt, 0, sizeof(struct ast_stmt));
	for_stmt->type = AST_STMT_FOR;
	for_stmt->val.for_.is_range = false;
	for_stmt->val.for_.value_symbol = iter_sym;
	for_stmt->val.for_.collection = array;
	for_stmt->val.for_.stmt_list = stmt_list;
	for_stmt->line = line;

	return for_stmt;
}

/* Called from the parser when it accepted a for_stmt with for(k, v in) syntax. */
struct ast_stmt *
ast_accept_for_kv_stmt(
	int line,
	char *key_sym,
	char *val_sym,
	struct ast_expr *array,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_FOR;
	stmt->val.for_.is_range = false;
	stmt->val.for_.key_symbol = key_sym;
	stmt->val.for_.value_symbol = val_sym;
	stmt->val.for_.collection = array;
	stmt->val.for_.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a for_stmt with for(k, v in) syntax. */
struct ast_stmt *
ast_accept_for_kv_stmt_single(
	int line,
	char *key_sym,
	char *val_sym,
	struct ast_expr *array,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *for_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	for_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (for_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(for_stmt, 0, sizeof(struct ast_stmt));
	for_stmt->type = AST_STMT_FOR;
	for_stmt->val.for_.is_range = false;
	for_stmt->val.for_.key_symbol = key_sym;
	for_stmt->val.for_.value_symbol = val_sym;
	for_stmt->val.for_.collection = array;
	for_stmt->val.for_.stmt_list = stmt_list;
	for_stmt->line = line;

	return for_stmt;
}

/* Called from the parser when it accepted a for_stmt with for(i in ..) syntax. */
struct ast_stmt *
ast_accept_for_range_stmt(
	int line,
	char *counter_sym,
	struct ast_expr *start,
	struct ast_expr *stop,
	struct ast_stmt_list *stmt_list)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_FOR;
	stmt->val.for_.is_range = true;
	stmt->val.for_.counter_symbol = counter_sym;
	stmt->val.for_.start = start;
	stmt->val.for_.stop = stop;
	stmt->val.for_.stmt_list = stmt_list;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a for_stmt with for(i in ..) syntax. */
struct ast_stmt *
ast_accept_for_range_stmt_single(
	int line,
	char *counter_sym,
	struct ast_expr *start,
	struct ast_expr *stop,
	struct ast_stmt *stmt)
{
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *for_stmt;

	stmt_list = ast_malloc(sizeof(struct ast_stmt_list));
	if (stmt_list == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt_list, 0, sizeof(struct ast_stmt_list));
	stmt_list->list = stmt;

	for_stmt = ast_malloc(sizeof(struct ast_stmt));
	if (for_stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(for_stmt, 0, sizeof(struct ast_stmt));
	for_stmt->type = AST_STMT_FOR;
	for_stmt->val.for_.is_range = true;
	for_stmt->val.for_.counter_symbol = counter_sym;
	for_stmt->val.for_.start = start;
	for_stmt->val.for_.stop = stop;
	for_stmt->val.for_.stmt_list = stmt_list;
	for_stmt->line = line;

	return for_stmt;
}

/* Called from the parser when it accepted a return_stmt. */
struct ast_stmt *
ast_accept_return_stmt(
	int line,
	struct ast_expr *expr)
{
	struct ast_stmt *stmt;
	struct ast_term *term;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_RETURN;
	stmt->val.return_.has_value = expr != NULL;
	stmt->line = line;

	if (expr == NULL) {
		term = ast_malloc(sizeof(struct ast_term));
		if (term == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(term, 0, sizeof(struct ast_term));
		term->type = AST_TERM_INT;
		term->val.i = 1;

		expr = ast_malloc(sizeof(struct ast_expr));
		if (expr == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(expr, 0, sizeof(struct ast_expr));
		expr->type = AST_EXPR_TERM;
		expr->val.term.term = term;
	}

	stmt->val.return_.expr = expr;

	return stmt;
}
	
/* Called from the parser when it accepted a break_stmt. */
struct ast_stmt *
ast_accept_break_stmt(
	int line)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_BREAK;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a continue_stmt. */
struct ast_stmt *
ast_accept_continue_stmt(
	int line)
{
	struct ast_stmt *stmt;

	stmt = ast_malloc(sizeof(struct ast_stmt));
	if (stmt == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(stmt, 0, sizeof(struct ast_stmt));
	stmt->type = AST_STMT_CONTINUE;
	stmt->line = line;

	return stmt;
}

/* Called from the parser when it accepted a expr with a term. */
struct ast_expr *
ast_accept_term_expr(
	struct ast_term *term)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_TERM;
	expr->val.term.term = term;

	return expr;
}

/* Called from the parser when it accepted a expr with a < operator. */
struct ast_expr *
ast_accept_lt_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_LT);
}

/* Called from the parser when it accepted a expr with a <= operator. */
struct ast_expr *
ast_accept_lte_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_LTE);
}

/* Called from the parser when it accepted a expr with a == operator. */
struct ast_expr *
ast_accept_eq_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_EQ);
}

/* Called from the parser when it accepted a expr with a != operator. */
struct ast_expr *
ast_accept_neq_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_NEQ);
}

/* Called from the parser when it accepted a expr with a >= operator. */
struct ast_expr *
ast_accept_gte_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_GTE);
}

/* Called from the parser when it accepted a expr with a > operator. */
struct ast_expr *
ast_accept_gt_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_GT);
}

/* Called from the parser when it accepted a expr with a + operator. */
struct ast_expr *
ast_accept_plus_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_PLUS);
}

/* Called from the parser when it accepted a expr with a - operator. */
struct ast_expr *
ast_accept_minus_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_MINUS);
}

/* Called from the parser when it accepted a expr with a * operator. */
struct ast_expr *
ast_accept_mul_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_MUL);
}

/* Called from the parser when it accepted a expr with a / operator. */
struct ast_expr *
ast_accept_div_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_DIV);
}

/* Called from the parser when it accepted a expr with a % operator. */
struct ast_expr *
ast_accept_mod_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_MOD);
}

/* Called from the parser when it accepted a expr with a & or && operator. */
struct ast_expr *
ast_accept_and_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_AND);
}

struct ast_expr *
ast_accept_land_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_LAND);
}

struct ast_expr *
ast_accept_lor_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_LOR);
}

/* Called from the parser when it accepted a expr with a | or || operator. */
struct ast_expr *
ast_accept_or_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_OR);
}

/* Called from the parser when it accepted a expr with a ^ operator. */
struct ast_expr *
ast_accept_xor_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_XOR);
}

/* Called from the parser when it accepted a expr with a << operator. */
struct ast_expr *
ast_accept_shl_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_SHL);
}

/* Called from the parser when it accepted a expr with a >> operator. */
struct ast_expr *
ast_accept_shr_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_SHR);
}

static struct ast_expr *
ast_accept_binary_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2,
	int type)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = type;
	expr->val.binary.expr[0] = expr1;
	expr->val.binary.expr[1] = expr2;

	return expr;
}

/* Called from the parser when it accepted a expr with a ! operator. */
struct ast_expr *
ast_accept_neg_expr(
	struct ast_expr *e)
{
	return ast_accept_unary_expr(e, AST_EXPR_NEG);
}

/* Called from the parser when it accepted a expr with a ! operator. */
struct ast_expr *
ast_accept_not_expr(
	struct ast_expr *e)
{
	return ast_accept_unary_expr(e, AST_EXPR_NOT);
}

static struct ast_expr *
ast_accept_unary_expr(
	struct ast_expr *e,
	int type)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = type;
	expr->val.unary.expr = e;

	return expr;
}

/* Called from the parser when it accepted a expr with a () syntax. */
struct ast_expr *
ast_accept_par_expr(
	struct ast_expr *e)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_PAR;
	expr->val.par.expr = e;

	return expr;
}

/* Called from the parser when it accepted a expr with a array[subscript] syntax. */
struct ast_expr *
ast_accept_subscr_expr(
	struct ast_expr *expr1,
	struct ast_expr *expr2)
{
	return ast_accept_binary_expr(expr1, expr2, AST_EXPR_SUBSCR);
}

/* Called from the parser when it accepted a expr with a object.receiver syntax. */
struct ast_expr *
ast_accept_dot_expr(
	struct ast_expr *obj,
	char *symbol)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_DOT;
	expr->val.dot.obj = obj;
	expr->val.dot.symbol = symbol;

	return expr;
}

/* Called from the parser when it accepted a expr with a call() syntax. */
struct ast_expr *
ast_accept_call_expr(
	struct ast_expr *expr1,
	struct ast_arg_list *arg_list)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_CALL;
	expr->val.call.func = expr1;
	expr->val.call.arg_list = arg_list;

	return expr;
}

/* Called from the parser when it accepted a expr with a array literal syntax. */
struct ast_expr *
ast_accept_array_expr(
	struct ast_arg_list *elem_list)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_ARRAY;
	expr->val.array.elem_list = elem_list;

	return expr;
}

/* Called from the parser when it accepted a expr with a dictionary literal syntax. */
struct ast_expr *
ast_accept_dict_expr(
	struct ast_kv_list *kv_list)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_DICT;
	expr->val.dict.kv_list = kv_list;

	return expr;
}

/* Called from the parser when it accepted a class literal. */
struct ast_expr *
ast_accept_class_expr(
	struct ast_kv_list *kv_list)
{
	struct ast_expr *expr;

	expr = ast_accept_dict_expr(kv_list);
	if (expr == NULL)
		return NULL;
	expr->val.dict.is_class = true;

	return expr;
}

/* Called from the parser when it accepted a expr with an anonymous function syntax. */
struct ast_expr *
ast_accept_func_expr(
	struct ast_param_list *param_list,
	struct ast_stmt_list *stmt_list)
{
	struct ast_expr *expr;

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_FUNC;
	expr->val.func.param_list = param_list;
	expr->val.func.stmt_list = stmt_list;

	return expr;
}

/* Called from the parser when it accepted a expr with a new syntax. */
struct ast_expr *
ast_accept_new_expr(
	char *cls,
	struct ast_kv_list *kv_list)
{
	struct ast_expr *expr, *kvexpr;

	if (kv_list != NULL) {
		kvexpr = ast_malloc(sizeof(struct ast_expr));
		if (kvexpr == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(kvexpr, 0, sizeof(struct ast_expr));
		kvexpr->type = AST_EXPR_DICT;
		kvexpr->val.dict.kv_list = kv_list;
	} else {
		kvexpr = NULL;
	}

	expr = ast_malloc(sizeof(struct ast_expr));
	if (expr == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(expr, 0, sizeof(struct ast_expr));
	expr->type = AST_EXPR_NEW;
	expr->val.new_.cls = cls;
	expr->val.new_.init = kvexpr;

	return expr;
}

/* Called from the parser when it accepted an extend expression. */
struct ast_expr *
ast_accept_extend_expr(
	char *cls,
	struct ast_kv_list *kv_list)
{
	struct ast_expr *expr;

	expr = ast_accept_new_expr(cls, kv_list);
	if (expr == NULL)
		return NULL;
	expr->val.new_.is_extend = true;

	return expr;
}

/* Called from the parser when it accepted a key-value list. */
struct ast_kv_list *
ast_accept_kv_list(
	struct ast_kv_list *kv_list,
	struct ast_kv *kv)
{
	if (kv_list == NULL) {
		kv_list = ast_malloc(sizeof(struct ast_kv_list));
		if (kv_list == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(kv_list, 0, sizeof(struct ast_kv_list));
	}

	kv->next = kv_list->list;
	kv_list->list = kv;

	return kv_list;
}

/* Called from the parser when it accepted a key-value pair. */
struct ast_kv *
ast_accept_kv(
	char *key,
	struct ast_expr *value)
{
	struct ast_kv *kv;

	kv = ast_malloc(sizeof(struct ast_kv));
	if (kv == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(kv, 0, sizeof(struct ast_kv));
	kv->key = key;
	kv->value = value;

	return kv;
}

/* Called from the parser when it accepted a term with an integer. */
struct ast_term *
ast_accept_int_term(
	int i)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_INT;
	term->val.i = i;

	return term;
}

/* Called from the parser when it accepted a term with a long. */
struct ast_term *
ast_accept_long_term(
	int64_t l)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_LONG;
	term->val.l = l;

	return term;
}

/* Called from the parser when it accepted a term with a float. */
struct ast_term *
ast_accept_float_term(
	float f)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_FLOAT;
	term->val.f = f;

	return term;
}

/* Called from the parser when it accepted a term with a double. */
struct ast_term *
ast_accept_double_term(
	double lf)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_DOUBLE;
	term->val.lf = lf;

	return term;
}

/* Called from the parser when it accepted a term with a string. */
struct ast_term *
ast_accept_str_term(
	char *s)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_STRING;
	term->val.s = s;

	return term;
}

/* Called from the parser when it accepted a term with a symbol. */
struct ast_term *
ast_accept_symbol_term(
	char *s)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_SYMBOL;
	term->val.symbol = s;

	return term;
}

/* Called from the parser when it accepted a term with an empty array. */
struct ast_term *
ast_accept_empty_array_term(void)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_EMPTY_ARRAY;

	return term;
}

/* Called from the parser when it accepted a term with an empty dictionary. */
struct ast_term *
ast_accept_empty_dict_term(void)
{
	struct ast_term *term;

	term = ast_malloc(sizeof(struct ast_term));
	if (term == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(term, 0, sizeof(struct ast_term));
	term->type = AST_TERM_EMPTY_DICT;

	return term;
}

/* Called from the parser when it accepted an arg_list. */
struct ast_arg_list *
ast_accept_arg_list(
	struct ast_arg_list *arg_list,
	struct ast_expr *expr)
{
	assert(expr != NULL);
	assert(expr->next == NULL);

	if (arg_list == NULL) {
		/* Alloc an arg_list. */
		arg_list = ast_malloc(sizeof(struct ast_arg_list));
		if (arg_list == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(arg_list, 0, sizeof(struct ast_arg_list));

		/* Set expr to the list first element. */
		arg_list->list = expr;
	} else {
		AST_ADD_TO_LAST(struct ast_expr, arg_list->list, expr);
	}

	return arg_list;
}

#if 0
/* Free an AST func_list. */
static void
ast_free_func_list(
	struct ast_func_list *func_list)
{
	assert(func_list != NULL);
	assert(func_list->list != NULL);

	ast_free_func(func_list->list);
	ast_free(func_list);
	func_list = NULL;
}

/* Free an AST func. */
static void
ast_free_func(
	struct ast_func *func)
{
	assert(func != NULL);
	assert(func->name != NULL);

	if (func->next != NULL)
		ast_free_func(func->next);

	ast_free(func->name);
	if (func->param_list != NULL) {
		assert(func->param_list->list != NULL);

		ast_free_param(func->param_list->list);

		ast_free(func->param_list);
	}
	if (func->stmt_list != NULL) {
		ast_free_stmt_list(func->stmt_list);
		func->stmt_list = NULL;
	}
	ast_free(func);
}

/* Free an AST arg_list. */
static void
ast_free_arg_list(
	struct ast_arg_list *arg_list)
{
	assert(arg_list != NULL);

	ast_free_expr(arg_list->list);
	arg_list->list = NULL;
}

/* Free an AST param. */
static void
ast_free_param(
	struct ast_param *param)
{
	assert(param != NULL);
	assert(param->name != NULL);

	if (param->next != NULL)
		ast_free_param(param->next);

	ast_free(param->name);
	ast_free(param);
}

/* Free an AST stmt. */
static void
ast_free_stmt_list(
	struct ast_stmt_list *stmt_list)
{
	assert(stmt_list != NULL);

	ast_free_stmt(stmt_list->list);
	ast_free(stmt_list);
}

/* Free an AST stmt. */
static void
ast_free_stmt(
	struct ast_stmt *stmt)
{
	if (stmt->next != NULL) {
		ast_free_stmt(stmt->next);
		stmt->next = NULL;
	}

	switch (stmt->type) {
	case AST_STMT_EXPR:
		if (stmt->val.expr.expr != NULL) {
			ast_free_expr(stmt->val.expr.expr);
			stmt->val.expr.expr = NULL;
		}
		break;
	case AST_STMT_ASSIGN:
		if (stmt->val.assign.lhs != NULL) {
			ast_free_expr(stmt->val.assign.lhs);
			stmt->val.assign.lhs = NULL;
		}
		if (stmt->val.assign.rhs != NULL) {
			ast_free_expr(stmt->val.assign.rhs);
			stmt->val.assign.rhs = NULL;
		}
		break;
	case AST_STMT_IF:
		if (stmt->val.if_.cond != NULL) {
			ast_free_expr(stmt->val.if_.cond);
			stmt->val.if_.cond = NULL;
		}
		if (stmt->val.if_.stmt_list != NULL) {
			ast_free_stmt_list(stmt->val.if_.stmt_list);
			stmt->val.if_.stmt_list = NULL;
		}
		break;
	case AST_STMT_ELIF:
		if (stmt->val.elif.cond != NULL) {
			ast_free_expr(stmt->val.elif.cond);
			stmt->val.elif.cond = NULL;
		}
		if (stmt->val.elif.stmt_list != NULL) {
			ast_free_stmt_list(stmt->val.elif.stmt_list);
			stmt->val.elif.stmt_list = NULL;
		}
		break;
	case AST_STMT_ELSE:
		if (stmt->val.else_.stmt_list != NULL) {
			ast_free_stmt_list(stmt->val.else_.stmt_list);
			stmt->val.else_.stmt_list = NULL;
		}
		break;
	case AST_STMT_WHILE:
		if (stmt->val.while_.cond != NULL) {
			ast_free_expr(stmt->val.while_.cond);
			stmt->val.while_.cond = NULL;
		}
		if (stmt->val.while_.stmt_list != NULL) {
			ast_free_stmt_list(stmt->val.while_.stmt_list);
			stmt->val.while_.stmt_list = NULL;
		}
		break;
	case AST_STMT_FOR:
		if (stmt->val.for_.counter_symbol != NULL) {
			ast_free(stmt->val.for_.counter_symbol);
			stmt->val.for_.counter_symbol = NULL;
		}
		if (stmt->val.for_.key_symbol != NULL) {
			ast_free(stmt->val.for_.key_symbol);
			stmt->val.for_.key_symbol = NULL;
		}
		if (stmt->val.for_.value_symbol != NULL) {
			ast_free(stmt->val.for_.value_symbol);
			stmt->val.for_.value_symbol = NULL;
		}
		if (stmt->val.for_.collection != NULL) {
			ast_free_expr(stmt->val.for_.collection);
			stmt->val.for_.collection = NULL;
		}
		if (stmt->val.for_.stmt_list != NULL) {
			ast_free_stmt_list(stmt->val.for_.stmt_list);
			stmt->val.for_.stmt_list = NULL;
		}
		break;
	case AST_STMT_RETURN:
		if (stmt->val.return_.expr != NULL) {
			ast_free_expr(stmt->val.return_.expr);
			stmt->val.return_.expr = NULL;
		}
		break;
	case AST_STMT_BREAK:
		break;
	case AST_STMT_CONTINUE:
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	ast_free(stmt);
	stmt = NULL;
}

/* Free an AST expr. */
static void
ast_free_expr(
	struct ast_expr *expr)
{
	assert(expr != NULL);

	if (expr->next != NULL) {
		ast_free_expr(expr->next);
		expr->next = NULL;
	}

	switch (expr->type) {
	case AST_EXPR_TERM:
		if (expr->val.term.term != NULL) {
			ast_free_term(expr->val.term.term);
			expr->val.term.term = NULL;
		}
		break;
	case AST_EXPR_LT:
	case AST_EXPR_LTE:
	case AST_EXPR_EQ:
	case AST_EXPR_NEQ:
	case AST_EXPR_GTE:
	case AST_EXPR_GT:
	case AST_EXPR_PLUS:
	case AST_EXPR_MINUS:
	case AST_EXPR_MUL:
	case AST_EXPR_DIV:
	case AST_EXPR_MOD:
	case AST_EXPR_AND:
	case AST_EXPR_OR:
	case AST_EXPR_SUBSCR:
		if (expr->val.binary.expr[0] != NULL) {
			ast_free_expr(expr->val.binary.expr[0]);
			expr->val.binary.expr[0] = NULL;
		}
		if (expr->val.binary.expr[1] != NULL) {
			ast_free_expr(expr->val.binary.expr[1]);
			expr->val.binary.expr[1] = NULL;
		}
		break;
	case AST_EXPR_NEG:
		if (expr->val.unary.expr != NULL) {
			ast_free_expr(expr->val.unary.expr);
			expr->val.unary.expr = NULL;
		}
		break;
	case AST_EXPR_DOT:
		if (expr->val.dot.obj != NULL) {
			ast_free_expr(expr->val.dot.obj);
			expr->val.dot.obj = NULL;
		}
		if (expr->val.dot.symbol != NULL) {
			ast_free(expr->val.dot.symbol);
			expr->val.dot.symbol = NULL;
		}
		break;
	case AST_EXPR_CALL:
		if (expr->val.call.func != NULL) {
			ast_free_expr(expr->val.call.func);
			expr->val.call.func = NULL;
		}
		if (expr->val.call.arg_list != NULL) {
			ast_free_arg_list(expr->val.call.arg_list);
			expr->val.call.arg_list = NULL;
		}
		break;
	case AST_EXPR_ARRAY:
		if (expr->val.array.elem_list != NULL) {
			ast_free_arg_list(expr->val.array.elem_list);
			expr->val.call.func = NULL;
		}
		break;
	case AST_EXPR_DICT:
		if (expr->val.dict.kv_list != NULL) {
			ast_free_kv_list(expr->val.dict.kv_list);
			expr->val.dict.kv_list = NULL;
		}
		break;
	case AST_EXPR_FUNC:
		if (expr->val.func.param_list != NULL) {
			ast_free_param(expr->val.func.param_list->list);
			expr->val.func.param_list = NULL;
		}
		if (expr->val.func.stmt_list != NULL) {
			ast_free_stmt_list(expr->val.func.stmt_list);
			expr->val.func.stmt_list = NULL;
		}
		break;
	}

	ast_free(expr);
	expr = NULL;
}

/* Free a key-value list. */
static void
ast_free_kv_list(
	struct ast_kv_list *kv_list)
{
	ast_free_kv(kv_list->list);
	kv_list->list = NULL;

	ast_free(kv_list);
}

/* Free a key-value pair. */
static void
ast_free_kv(
	struct ast_kv *kv)
{
	if (kv->next != NULL) {
		ast_free_kv(kv->next);
		kv->next = NULL;
	}

	ast_free(kv->key);
	kv->key = NULL;

	ast_free_expr(kv->value);
	kv->value = NULL;

	ast_free(kv);
	kv = NULL;
}

/* Free an AST term. */
static void
ast_free_term(
	struct ast_term *term)
{
	switch (term->type) {
	case AST_TERM_STRING:
		if (term->val.s != NULL) {
			ast_free(term->val.s);
			term->val.s = NULL;
		}
		break;
	case AST_TERM_SYMBOL:
		if (term->val.symbol != NULL) {
			ast_free(term->val.symbol);
			term->val.symbol = NULL;
		}
		break;
	default:
		/* Other types don't need be freed. */
		break;
	}

	ast_free(term);
	term = NULL;
}
#endif

/* Copy an AST expr. */
static struct ast_expr *
ast_copy_expr(
	struct ast_expr *expr)
{
	struct ast_expr *dst;

	assert(expr != NULL);

	dst = ast_malloc(sizeof(struct ast_expr));
	if (dst == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(dst, 0, sizeof(struct ast_expr));
	dst->type = expr->type;

	switch (expr->type) {
	case AST_EXPR_TERM:
		if (expr->val.term.term != NULL) {
			dst->val.term.term = ast_copy_term(expr->val.term.term);
			if (dst->val.term.term == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_LT:
	case AST_EXPR_LTE:
	case AST_EXPR_EQ:
	case AST_EXPR_NEQ:
	case AST_EXPR_GTE:
	case AST_EXPR_GT:
	case AST_EXPR_PLUS:
	case AST_EXPR_MINUS:
	case AST_EXPR_MUL:
	case AST_EXPR_DIV:
	case AST_EXPR_MOD:
	case AST_EXPR_AND:
	case AST_EXPR_OR:
	case AST_EXPR_SUBSCR:
		if (expr->val.binary.expr[0] != NULL) {
			dst->val.binary.expr[0] = ast_copy_expr(expr->val.binary.expr[0]);
			if (dst->val.binary.expr[0] == NULL)
				return NULL;
		}
		if (expr->val.binary.expr[1] != NULL) {
			dst->val.binary.expr[1] = ast_copy_expr(expr->val.binary.expr[1]);
			if (dst->val.binary.expr[1] == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_NEG:
		if (expr->val.unary.expr != NULL) {
			dst->val.unary.expr = ast_copy_expr(expr->val.unary.expr);
			if (dst->val.unary.expr == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_DOT:
		if (expr->val.dot.obj != NULL) {
			dst->val.dot.obj = ast_copy_expr(expr->val.dot.obj);
			if (dst->val.dot.obj == NULL)
				return NULL;
		}
		if (expr->val.dot.symbol != NULL) {
			dst->val.dot.symbol = ast_strdup(expr->val.dot.symbol);
			if (dst->val.dot.symbol == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_CALL:
		if (expr->val.call.func != NULL) {
			dst->val.call.func = ast_copy_expr(expr->val.call.func);
			if (dst->val.call.func == NULL)
				return NULL;
		}
		if (expr->val.call.arg_list != NULL) {
			dst->val.call.arg_list = ast_copy_arg_list(expr->val.call.arg_list);
			if (dst->val.call.arg_list == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_ARRAY:
		dst->val.array.is_multi_index =
			expr->val.array.is_multi_index;
		if (expr->val.array.elem_list != NULL) {
			dst->val.array.elem_list = ast_copy_arg_list(expr->val.array.elem_list);
			if (dst->val.array.elem_list == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_DICT:
		if (expr->val.dict.kv_list != NULL) {
			dst->val.dict.kv_list = ast_copy_kv_list(expr->val.dict.kv_list);
			if (dst->val.dict.kv_list == NULL)
				return NULL;
		}
		break;
	case AST_EXPR_FUNC:
		ast_printf("Anonymous function inside += or -= is not allowed.");
		return NULL;
	}

	return dst;
}

/* Copy a term. */
static struct ast_term *
ast_copy_term(
	struct ast_term *term)
{
	struct ast_term *dst;

	dst = ast_malloc(sizeof(struct ast_term));
	if (dst == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memcpy(dst, term, sizeof(struct ast_term));

	switch (term->type) {
	case AST_TERM_STRING:
		if (term->val.s != NULL) {
			dst->val.s = ast_strdup(term->val.s);
			if (dst->val.s == NULL) {
				ast_out_of_memory();
				return NULL;
			}
		}
		break;
	case AST_TERM_SYMBOL:
		if (term->val.symbol != NULL) {
			dst->val.symbol = ast_strdup(term->val.symbol);
			if (dst->val.symbol == NULL) {
				ast_out_of_memory();
				return NULL;
			}
		}
		break;
	default:
		/* Other types don't need be duplicated. */
		break;
	}

	return dst;
}

/* Copy an arg list. */
static struct ast_arg_list *
ast_copy_arg_list(
	struct ast_arg_list *arg_list)
{
	struct ast_arg_list *dst;
	struct ast_expr *p, *expr;

	dst = ast_malloc(sizeof(struct ast_arg_list));
	if (dst == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(dst, 0, sizeof(struct ast_arg_list));

	p = arg_list->list;
	while (p != NULL) {
		expr = ast_copy_expr(p);
		if (expr == NULL)
			return NULL;

		AST_ADD_TO_LAST(struct ast_expr, dst->list, expr);
		p = p->next;
	}

	return dst;
}

/* Copy a kv list. */
static struct ast_kv_list *
ast_copy_kv_list(
	struct ast_kv_list *kv_list)
{
	struct ast_kv_list *dst;
	struct ast_kv *p, *kv;

	dst = ast_malloc(sizeof(struct ast_kv_list));
	if (dst == NULL) {
		ast_out_of_memory();
		return NULL;
	}
	memset(dst, 0, sizeof(struct ast_kv_list));

	p = kv_list->list;
	while (p != NULL) {
		kv = ast_malloc(sizeof(struct ast_kv));
		if (kv == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		memset(kv, 0, sizeof(struct ast_kv));
		kv->key = ast_strdup(p->key);
		if (kv->key == NULL) {
			ast_out_of_memory();
			return NULL;
		}
		kv->value = ast_copy_expr(p->value);
		if (kv->value == NULL) {
			ast_out_of_memory();
			return NULL;
		}

		AST_ADD_TO_LAST(struct ast_kv, dst->list, kv);
		p = p->next;
	}

	return dst;	
}

/*
 * Error Handling
 */

/* Log an error message. */
static void
ast_printf(
	const char *format,
	...)
{
	va_list ap;

	va_start(ap, format);
	vsnprintf(ast_error_message, sizeof(ast_error_message), format, ap);
	va_end(ap);
}

/* Log an out-of-memory error. */
static void
ast_out_of_memory(void)
{
	ast_printf(N_TR("%s: Out of memory while parsing."), ast_file_name);
}

/*
 * Memory Allocation
 */

/* malloc() alternative. */
void *
ast_malloc(
	size_t size)
{
	void *ret;

	ret = arena_alloc(&ast_arena, size);
	assert((uintptr_t)ret % 8 == 0);
	if (ret == NULL) {
		ast_out_of_memory();
		return NULL;
	}

	return ret;
}

/* realloc() alternative. */
void *
ast_realloc(void *p, size_t size)
{
	size_t orig_size;
	void *ret;

	orig_size = arena_get_block_size(p);
	ret = arena_alloc(&ast_arena, size);
	if (ret == NULL) {
		ast_out_of_memory();
		return NULL;
	}

	memcpy(ret, p, orig_size);

	return ret;	
}

/* strdup() alternative. */
char *
ast_strdup(
	const char *s)
{
	char *ret;
	size_t len;

	len = strlen(s) + 1;
	ret = arena_alloc(&ast_arena, len);
	if (ret == NULL)
		return NULL;

	memcpy(ret, s, len);
	return ret;
}

/* free() alternative. */
void
ast_free(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * In the current implementation, we don't free individual
	 * objects because we use an arena allocator.
	 */
}
