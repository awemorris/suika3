%{
/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

#include <noct/c89compat.h>
#include "ast.h"

#include <stdio.h>
#include <string.h>

#undef DEBUG
#ifdef DEBUG
static void print_debug(const char *s);
#define debug(s) print_debug(s)
#else
#define debug(s)
#endif

#define YYMALLOC ast_malloc

extern int ast_error_line;
extern int ast_error_column;

int ast_yylex(void *);
void ast_yyerror(void *, char *s);
void *ast_malloc(size_t size); 

/* Internal: called back from the parser. */
struct ast_func_list *ast_accept_func_list(struct ast_func_list *impl_list, struct ast_func *func);
struct ast_func *ast_accept_func(char *name, struct ast_param_list *param_list, struct ast_stmt_list *stmt_list);
struct ast_param_list *ast_accept_param_list(struct ast_param_list *param_list, char *name);
struct ast_stmt_list *ast_accept_stmt_list(struct ast_stmt_list *stmt_list, struct ast_stmt *stmt);
void ast_accept_stmt(struct ast_stmt *stmt, int line);
struct ast_stmt *ast_accept_expr_stmt(int line, struct ast_expr *expr);
struct ast_stmt *ast_accept_assign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs, bool is_var);
struct ast_stmt *ast_accept_plusassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_minusassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_mulassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_divassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_modassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_andassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_orassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_shlassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_shrassign_stmt(int line, struct ast_expr *lhs, struct ast_expr *rhs);
struct ast_stmt *ast_accept_plusplus_stmt(int line, struct ast_expr *expr);
struct ast_stmt *ast_accept_minusminus_stmt(int line, struct ast_expr *expr);
struct ast_stmt *ast_accept_if_stmt(int line, struct ast_expr *cond, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_if_stmt_single(int line, struct ast_expr *cond, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_elif_stmt(int line, struct ast_expr *cond, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_elif_stmt_single(int line, struct ast_expr *cond, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_else_stmt(int line, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_else_stmt_single(int line, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_while_stmt(int line, struct ast_expr *cond, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_while_stmt_single(int line, struct ast_expr *cond, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_for_kv_stmt(int line, char *key_sym, char *val_sym, struct ast_expr *array, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_for_kv_stmt_single(int line, char *key_sym, char *val_sym, struct ast_expr *array, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_for_v_stmt(int line, char *iter_sym, struct ast_expr *array, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_for_v_stmt_single(int line, char *iter_sym, struct ast_expr *array, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_for_range_stmt(int line, char *counter_sym, struct ast_expr *start, struct ast_expr *stop, struct ast_stmt_list *stmt_list);
struct ast_stmt *ast_accept_for_range_stmt_single(int line, char *counter_sym, struct ast_expr *start, struct ast_expr *stop, struct ast_stmt *stmt);
struct ast_stmt *ast_accept_return_stmt(int line, struct ast_expr *expr);
struct ast_stmt *ast_accept_break_stmt(int line);
struct ast_stmt *ast_accept_continue_stmt(int line);
struct ast_expr *ast_accept_term_expr(struct ast_term *term);
struct ast_expr *ast_accept_lt_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_lte_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_gt_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_gte_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_eq_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_neq_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_plus_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_minus_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_mul_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_div_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_mod_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_and_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_or_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_xor_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_shl_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_shr_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_neg_expr(struct ast_expr *expr);
struct ast_expr *ast_accept_not_expr(struct ast_expr *expr);
struct ast_expr *ast_accept_par_expr(struct ast_expr *expr);
struct ast_expr *ast_accept_subscr_expr(struct ast_expr *expr1, struct ast_expr *expr2);
struct ast_expr *ast_accept_dot_expr(struct ast_expr *obj, char *symbol);
struct ast_expr *ast_accept_call_expr(struct ast_expr *func, struct ast_arg_list *arg_list);
struct ast_expr *ast_accept_thiscall_expr(struct ast_expr *obj, char *func, struct ast_arg_list *arg_list);
struct ast_expr *ast_accept_array_expr(struct ast_arg_list *arg_list);
struct ast_expr *ast_accept_dict_expr(struct ast_kv_list *kv_list);
struct ast_expr *ast_accept_func_expr(struct ast_param_list *param_list, struct ast_stmt_list *stmt_list);
struct ast_expr *ast_accept_new_expr(char *cls, struct ast_kv_list *kv_list);
struct ast_kv_list *ast_accept_kv_list(struct ast_kv_list *kv_list, struct ast_kv *kv);
struct ast_kv *ast_accept_kv(char *key, struct ast_expr *value);
struct ast_term *ast_accept_int_term(int i);
struct ast_term *ast_accept_float_term(float f);
struct ast_term *ast_accept_str_term(char *s);
struct ast_term *ast_accept_symbol_term(char *symbol);
struct ast_term *ast_accept_empty_array_term(void);
struct ast_term *ast_accept_empty_dict_term(void);
struct ast_arg_list *ast_accept_arg_list(struct ast_arg_list *arg_list, struct ast_expr *expr);

%}

%{
#include "stdio.h"
extern void ast_yyerror(void *scanner, char *s);
%}

%parse-param { void *scanner }
%lex-param { scanner }

%code provides {
#define YY_DECL int ast_yylex(void *yyscanner)
}

%union {
	int ival;
	double fval;
	char *sval;

	struct ast_func_list *func_list;
	struct ast_func *func;
	struct ast_param_list *param_list;
	struct ast_stmt_list *stmt_list;
	struct ast_stmt *stmt;
	struct ast_expr *expr;
	struct ast_term *term;
	struct ast_arg_list *arg_list;
	struct ast_kv_list *kv_list;
	struct ast_kv *kv;
}

%token <sval> TOKEN_SYMBOL TOKEN_STR
%token <ival> TOKEN_INT
%token <fval> TOKEN_FLOAT
%token TOKEN_FUNC TOKEN_CLASS TOKEN_NEW TOKEN_LAMBDA TOKEN_LARR TOKEN_RARR TOKEN_PLUS TOKEN_MINUS TOKEN_MUL
%token TOKEN_DIV TOKEN_MOD TOKEN_SHL TOKEN_SHR
%token TOKEN_ASSIGN TOKEN_PLUSASSIGN TOKEN_MINUSASSIGN TOKEN_MULASSIGN TOKEN_DIVASSIGN TOKEN_MODASSIGN TOKEN_ANDASSIGN TOKEN_ORASSIGN TOKEN_SHLASSIGN TOKEN_SHRASSIGN
%token TOKEN_PLUSPLUS TOKEN_MINUSMINUS TOKEN_ANDAND TOKEN_OROR
%token TOKEN_LPAR TOKEN_RPAR TOKEN_RPAR_LBLK TOKEN_LBLK TOKEN_LBLK_BLK TOKEN_RBLK TOKEN_SEMICOLON TOKEN_COLON
%token TOKEN_DOT TOKEN_COMMA TOKEN_IF TOKEN_ELSE TOKEN_ELSE_LBLK TOKEN_ELSEIF TOKEN_WHILE TOKEN_FOR TOKEN_IN TOKEN_DOTDOT TOKEN_GT
%token TOKEN_GTE TOKEN_LT TOKEN_LTE TOKEN_EQ TOKEN_NEQ TOKEN_RETURN TOKEN_BREAK
%token TOKEN_CONTINUE TOKEN_ARROW TOKEN_RPAR_DARROW_LBLK TOKEN_AND TOKEN_OR TOKEN_XOR TOKEN_VAR

%type <func_list> func_list;
%type <func> func;
%type <param_list> param_list;
%type <stmt_list> stmt_list;
%type <stmt> stmt;
%type <stmt> expr_stmt;
%type <stmt> assign_stmt;
%type <stmt> plusassign_stmt;
%type <stmt> minusassign_stmt;
%type <stmt> mulassign_stmt;
%type <stmt> divassign_stmt;
%type <stmt> modassign_stmt;
%type <stmt> andassign_stmt;
%type <stmt> orassign_stmt;
%type <stmt> shlassign_stmt;
%type <stmt> shrassign_stmt;
%type <stmt> plusplus_stmt;
%type <stmt> minusminus_stmt;
%type <stmt> if_stmt;
%type <stmt> elif_stmt;
%type <stmt> else_stmt;
%type <stmt> while_stmt;
%type <stmt> for_stmt;
%type <stmt> return_stmt;
%type <stmt> break_stmt;
%type <stmt> continue_stmt;
%type <expr> expr;
%type <expr> call_expr;
%type <expr> thiscall_expr;
%type <expr> lambda_expr;
%type <kv_list> kv_list;
%type <kv> kv;
%type <term> term;
%type <arg_list> arg_list;

%left UNARYMINUS
%left TOKEN_OROR
%left TOKEN_ANDAND
%left TOKEN_NOT
%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_XOR
%left TOKEN_LT
%left TOKEN_LTE
%left TOKEN_GT
%left TOKEN_GTE
%left TOKEN_EQ
%left TOKEN_NEQ
%left TOKEN_PLUS
%left TOKEN_MINUS
%left TOKEN_SHL
%left TOKEN_SHR
%left TOKEN_MUL
%left TOKEN_DIV
%left TOKEN_MOD
%left TOKEN_DOT
%right TOKEN_RPAR_DARROW_LBLK
%right TOKEN_ARROW
%right TOKEN_LBLK
%right TOKEN_LARR
%right TOKEN_LPAR

%precedence CALL

%locations

%initial-action {
	ast_yylloc.last_line = yylloc.first_line = 0;
	ast_yylloc.last_column = yylloc.first_column = 0;
}

%%
func_list	: func
		{
			$$ = ast_accept_func_list(NULL, $1);
			debug("func_list: class");
		}
		| func_list func
		{
			$$ = ast_accept_func_list($1, $2);
			debug("func_list: func_list func");
		}
		;
func		: TOKEN_FUNC TOKEN_SYMBOL TOKEN_LPAR param_list TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_func($2, $4, $6);
			debug("func: func name(param_list) { stmt_list }");
		}
		| TOKEN_FUNC TOKEN_SYMBOL TOKEN_LPAR TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_func($2, NULL, $5);
			debug("func: func name() { stmt_list }");
		}
		;
param_list	: TOKEN_SYMBOL
		{
			$$ = ast_accept_param_list(NULL, $1);
			debug("param_list: symbol");
		}
		| param_list TOKEN_COMMA TOKEN_SYMBOL
		{
			$$ = ast_accept_param_list($1, $3);
			debug("param_list: param_list symbol");
		}
		;
stmt_list	: /* empty */
		{
			$$ = ast_accept_stmt_list(NULL, NULL);
			debug("stmt_list: empty");
		}
		| stmt_list stmt
		{
			$$ = ast_accept_stmt_list($1, $2);
			debug("stmt_list: stmt_list stmt");
		}
		;
stmt		: expr_stmt
		{
			$$ = $1;
		}
		| assign_stmt
		{
			$$ = $1;
		}
		| plusassign_stmt
		{
			$$ = $1;
		}
		| minusassign_stmt
		{
			$$ = $1;
		}
		| mulassign_stmt
		{
			$$ = $1;
		}
		| divassign_stmt
		{
			$$ = $1;
		}
		| modassign_stmt
		{
			$$ = $1;
		}
		| andassign_stmt
		{
			$$ = $1;
		}
		| orassign_stmt
		{
			$$ = $1;
		}
		| shlassign_stmt
		{
			$$ = $1;
		}
		| shrassign_stmt
		{
			$$ = $1;
		}
		| plusplus_stmt
		{
			$$ = $1;
		}
		| minusminus_stmt
		{
			$$ = $1;
		}
		| if_stmt
		{
			$$ = $1;
		}
		| elif_stmt
		{
			$$ = $1;
		}
		| else_stmt
		{
			$$ = $1;
		}
		| while_stmt
		{
			$$ = $1;
		}
		| for_stmt
		{
			$$ = $1;
		}
		| return_stmt
		{
			$$ = $1;
		}
		| break_stmt
		{
			$$ = $1;
		}
		| continue_stmt
		{
			$$ = $1;
		}
		;
expr_stmt	: expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_expr_stmt(@1.first_line + 1, $1);
			debug("expr_stmt");
		}
		;
assign_stmt	: expr TOKEN_ASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_assign_stmt(@1.first_line + 1, $1, $3, false);
			debug("assign_stmt");
		}
		| TOKEN_VAR expr TOKEN_ASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_assign_stmt(@1.first_line + 1, $2, $4, true);
			debug("var assign_stmt");
		}
		;
plusassign_stmt	: expr TOKEN_PLUSASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_plusassign_stmt(@1.first_line + 1, $1, $3);
			debug("plusassign_stmt");
		}
		;
minusassign_stmt: expr TOKEN_MINUSASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_minusassign_stmt(@1.first_line + 1, $1, $3);
			debug("minusassign_stmt");
		}
		;
mulassign_stmt	: expr TOKEN_MULASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_mulassign_stmt(@1.first_line + 1, $1, $3);
			debug("mulassign_stmt");
		}
		;
divassign_stmt	: expr TOKEN_DIVASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_divassign_stmt(@1.first_line + 1, $1, $3);
			debug("divassign_stmt");
		}
		;
modassign_stmt	: expr TOKEN_MODASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_modassign_stmt(@1.first_line + 1, $1, $3);
			debug("modassign_stmt");
		}
		;
andassign_stmt	: expr TOKEN_ANDASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_andassign_stmt(@1.first_line + 1, $1, $3);
			debug("andassign_stmt");
		}
		;
orassign_stmt	: expr TOKEN_ORASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_orassign_stmt(@1.first_line + 1, $1, $3);
			debug("orassign_stmt");
		}
		;
shlassign_stmt	: expr TOKEN_SHLASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_shlassign_stmt(@1.first_line + 1, $1, $3);
			debug("shlassign_stmt");
		}
		;
shrassign_stmt	: expr TOKEN_SHRASSIGN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_shrassign_stmt(@1.first_line + 1, $1, $3);
			debug("shrassign_stmt");
		}
		;
plusplus_stmt	: expr TOKEN_PLUSPLUS TOKEN_SEMICOLON
		{
			$$ = ast_accept_plusplus_stmt(@1.first_line + 1, $1);
			debug("plusplus_stmt");
		}
		;
minusminus_stmt	: expr TOKEN_MINUSMINUS TOKEN_SEMICOLON
		{
			$$ = ast_accept_minusminus_stmt(@1.first_line + 1, $1);
			debug("plusplus_stmt");
		}
		;
if_stmt		: TOKEN_IF TOKEN_LPAR expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_if_stmt(@1.first_line + 1, $3, $5);
			debug("if_stmt: stmt_list");
		}
		| TOKEN_IF TOKEN_LPAR expr TOKEN_RPAR stmt
		{
			$$ = ast_accept_if_stmt_single(@1.first_line + 1, $3, $5);
			debug("if_stmt: stmt_list");
		}
		;
elif_stmt	: TOKEN_ELSEIF TOKEN_LPAR expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_elif_stmt(@1.first_line + 1, $3, $5);
			debug("elif_stmt: stmt_list");
		}
		| TOKEN_ELSEIF TOKEN_LPAR expr TOKEN_RPAR stmt
		{
			$$ = ast_accept_elif_stmt_single(@1.first_line + 1, $3, $5);
			debug("elif_stmt: stmt_list");
		}
		;
else_stmt	: TOKEN_ELSE_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_else_stmt(@1.first_line + 1, $2);
			debug("else_stmt: stmt_list");
		}
		| TOKEN_ELSE stmt
		{
			$$ = ast_accept_else_stmt_single(@1.first_line + 1, $2);
			debug("else_stmt: stmt_list");
		}
		;
while_stmt	: TOKEN_WHILE TOKEN_LPAR expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_while_stmt(@1.first_line + 1, $3, $5);
			debug("while_stmt: stmt_list");
		}
		| TOKEN_WHILE TOKEN_LPAR expr TOKEN_RPAR stmt
		{
			$$ = ast_accept_while_stmt_single(@1.first_line + 1, $3, $5);
			debug("while_stmt: stmt_list");
		}
		;
for_stmt	: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_COMMA TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_for_kv_stmt(@1.first_line + 1, $3, $5, $7, $9);
			debug("for_stmt: for(k, v in array) { stmt_list }");
		}
		| TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_COMMA TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR stmt
		{
			$$ = ast_accept_for_kv_stmt_single(@1.first_line + 1, $3, $5, $7, $9);
			debug("for_stmt: for(k, v in array) stmt");
		}
		| TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_for_v_stmt(@1.first_line + 1, $3, $5, $7);
			debug("for_stmt: for(v in array) { stmt_list }");
		}
		| TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR stmt
		{
			$$ = ast_accept_for_v_stmt_single(@1.first_line + 1, $3, $5, $7);
			debug("for_stmt: for(v in array) stmt_list");
		}
		| TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_DOTDOT expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_for_range_stmt(@1.first_line + 1, $3, $5, $7, $9);
			debug("for_stmt: for(i in x..y) { stmt_list }");
		}
		| TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_DOTDOT expr TOKEN_RPAR stmt
		{
			$$ = ast_accept_for_range_stmt_single(@1.first_line + 1, $3, $5, $7, $9);
			debug("for_stmt: for(i in x..y) stmt");
		}
		;
return_stmt	: TOKEN_RETURN expr TOKEN_SEMICOLON
		{
			$$ = ast_accept_return_stmt(@1.first_line + 1, $2);
			debug("rerurn_stmt:");
		}
		| TOKEN_RETURN TOKEN_SEMICOLON
		{
			$$ = ast_accept_return_stmt(@1.first_line + 1, NULL);
			debug("rerurn_stmt NULL:");
		}		;
break_stmt	: TOKEN_BREAK TOKEN_SEMICOLON
		{
			$$ = ast_accept_break_stmt(@1.first_line + 1);
			debug("break_stmt:");
		}
		;
continue_stmt	: TOKEN_CONTINUE TOKEN_SEMICOLON
		{
			$$ = ast_accept_continue_stmt(@1.first_line + 1);
			debug("continue_stmt");
		}
		;
expr		: term
		{
			$$ = ast_accept_term_expr($1);
			debug("expr: term");
		}
		| TOKEN_LPAR expr TOKEN_RPAR
		{
			$$ = $2;
			debug("expr: (expr)");
		}
		| expr TOKEN_LARR expr TOKEN_RARR
		{
			$$ = ast_accept_subscr_expr($1, $3);
			debug("expr: array[subscript]");
		}
		| expr TOKEN_OR expr
		{
			$$ = ast_accept_or_expr($1, $3);
			debug("expr: expr or expr");
		}
		| expr TOKEN_AND expr
		{
			$$ = ast_accept_and_expr($1, $3);
			debug("expr: expr and expr");
		}
		| expr TOKEN_XOR expr
		{
			$$ = ast_accept_xor_expr($1, $3);
			debug("expr: expr xor expr");
		}
		| expr TOKEN_OROR expr
		{
			$$ = ast_accept_or_expr($1, $3);
			debug("expr: expr or expr");
		}
		| expr TOKEN_ANDAND expr
		{
			$$ = ast_accept_and_expr($1, $3);
			debug("expr: expr and expr");
		}
		| expr TOKEN_LT expr
		{
			$$ = ast_accept_lt_expr($1, $3);
			debug("expr: expr lt expr");
		}
		| expr TOKEN_LTE expr
		{
			$$ = ast_accept_lte_expr($1, $3);
			debug("expr: expr lte expr");
		}
		| expr TOKEN_GT expr
		{
			$$ = ast_accept_gt_expr($1, $3);
			debug("expr: expr gt expr");
		}
		| expr TOKEN_GTE expr
		{
			$$ = ast_accept_gte_expr($1, $3);
			debug("expr: expr gte expr");
		}
		| expr TOKEN_EQ expr
		{
			$$ = ast_accept_eq_expr($1, $3);
			debug("expr: expr eq expr");
		}
		| expr TOKEN_NEQ expr
		{
			$$ = ast_accept_neq_expr($1, $3);
			debug("expr: expr neq expr");
		}
		| expr TOKEN_PLUS expr
		{
			$$ = ast_accept_plus_expr($1, $3);
			debug("expr: expr plus expr");
		}
		| expr TOKEN_MINUS expr
		{
			$$ = ast_accept_minus_expr($1, $3);
			debug("expr: expr sub expr");
		}
		| expr TOKEN_MUL expr
		{
			$$ = ast_accept_mul_expr($1, $3);
			debug("expr: expr mul expr");
		}
		| expr TOKEN_DIV expr
		{
			$$ = ast_accept_div_expr($1, $3);
			debug("expr: expr div expr");
		}
		| expr TOKEN_MOD expr
		{
			$$ = ast_accept_mod_expr($1, $3);
			debug("expr: expr div expr");
		}
		| expr TOKEN_SHL expr
		{
			$$ = ast_accept_shl_expr($1, $3);
			debug("expr: expr shl expr");
		}
		| expr TOKEN_SHR expr
		{
			$$ = ast_accept_shr_expr($1, $3);
			debug("expr: expr shr expr");
		}
		| TOKEN_MINUS expr %prec UNARYMINUS
		{
			$$ = ast_accept_neg_expr($2);
			debug("expr: neg expr");
		}
		| TOKEN_NOT expr
		{
			$$ = ast_accept_not_expr($2);
			debug("expr: not expr");
		}
		| expr TOKEN_DOT TOKEN_SYMBOL
		{
			$$ = ast_accept_dot_expr($1, $3);
			debug("expr: expr.symbol");
		}
		| call_expr
		{
			$$ = $1;
		}
		| thiscall_expr
		{
			$$ = $1;
		}
		| TOKEN_LARR arg_list TOKEN_RARR
		{
			$$ = ast_accept_array_expr($2);
			debug("expr: array");
		}
		| TOKEN_LBLK kv_list TOKEN_RBLK
		{
			$$ = ast_accept_dict_expr($2);
			debug("expr: dict");
		}
		| TOKEN_CLASS TOKEN_LBLK kv_list TOKEN_RBLK
		{
			/* class is equal to dict. */
			$$ = ast_accept_dict_expr($3);
			debug("expr: dict");
		}
		| TOKEN_CLASS TOKEN_LBLK TOKEN_RBLK
		{
			/* class is equal to dict. */
			$$ = ast_accept_dict_expr(NULL);
			debug("expr: dict");
		}
		| lambda_expr
		{
			$$ = $1;
		}
		| TOKEN_NEW TOKEN_SYMBOL TOKEN_LBLK kv_list TOKEN_RBLK
		{
			$$ = ast_accept_new_expr($2, $4);
			debug("expr: new");
		}
		| TOKEN_NEW TOKEN_SYMBOL TOKEN_LBLK TOKEN_RBLK
		{
			$$ = ast_accept_new_expr($2, NULL);
			debug("expr: new");
		}
		;
call_expr	: expr TOKEN_LPAR arg_list TOKEN_RPAR
		{
			$$ = ast_accept_call_expr($1, $3);
			debug("expr: call(param_list)");
		}
		| expr TOKEN_LPAR TOKEN_RPAR
		{
			$$ = ast_accept_call_expr($1, NULL);
			debug("expr: call()");
		}
		;
thiscall_expr	: expr TOKEN_ARROW TOKEN_SYMBOL TOKEN_LPAR arg_list TOKEN_RPAR
		{
			$$ = ast_accept_thiscall_expr($1, $3, $5);
			debug("expr: thiscall(param_list)");
		}
		| expr TOKEN_ARROW TOKEN_SYMBOL TOKEN_LPAR TOKEN_RPAR
		{
			$$ = ast_accept_thiscall_expr($1, $3, NULL);
			debug("expr: thiscall(param_list)");
		}

lambda_expr	: TOKEN_LPAR param_list TOKEN_RPAR_DARROW_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_func_expr($2, $4);
			debug("expr: func param_list stmt_list");
		}
		| TOKEN_LPAR TOKEN_RPAR_DARROW_LBLK stmt_list TOKEN_RBLK
		{
			$$ = ast_accept_func_expr(NULL, $3);
			debug("expr: func stmt_list");
		}
		;
arg_list	: expr
		{
			$$ = ast_accept_arg_list(NULL, $1);
			debug("arg_list: expr");
		}
		| arg_list TOKEN_COMMA expr
		{
			$$ = ast_accept_arg_list($1, $3);
			debug("arg_list: arg_list arg");
		}
		;
kv_list		: kv
		{
			$$ = ast_accept_kv_list(NULL, $1);
			debug("kv_list: kv");
		}
		| kv_list TOKEN_COMMA kv
		{
			$$ = ast_accept_kv_list($1, $3);
			debug("kv_list: kv_list kv");
		}
		;
kv		: TOKEN_STR TOKEN_COLON expr
		{
			$$ = ast_accept_kv($1, $3);
			debug("kv");
		}
		| TOKEN_SYMBOL TOKEN_COLON expr
		{
			$$ = ast_accept_kv($1, $3);
			debug("kv");
		}
		;
term		: TOKEN_INT
		{
			$$ = ast_accept_int_term($1);
			debug("term: int");
		}
		| TOKEN_FLOAT
		{
			$$ = ast_accept_float_term((float)$1);
			debug("term: float");
		}
		| TOKEN_STR
		{
			$$ = ast_accept_str_term($1);
			debug("term: string");
		}
		| TOKEN_SYMBOL
		{
			$$ = ast_accept_symbol_term($1);
			debug("term: symbol");
		}
		| TOKEN_LARR TOKEN_RARR
		{
			$$ = ast_accept_empty_array_term();
			debug("term: empty array symbol");
		}
		| TOKEN_LBLK TOKEN_RBLK
		{
			$$ = ast_accept_empty_dict_term();
			debug("term: empty dict symbol");
		}
		;
%%

#ifdef DEBUG
static void print_debug(const char *s)
{
	fprintf(stderr, "%s\n", s);
}
#endif

void ast_yyerror(void *scanner, char *s)
{
	extern int ast_error_line;
	extern int ast_error_column;
	extern char ast_error_message[65536];
	extern const char *noct_gettext(const char *msg);

	(void)scanner;
	(void)s;

	ast_error_line = ast_yylloc.first_line + 1;
	ast_error_column = ast_yylloc.first_column + 1;
	
	if (s != NULL)
		strcpy(ast_error_message, N_TR(s));
	else
		strcpy(ast_error_message, "");
}
