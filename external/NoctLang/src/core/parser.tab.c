/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         ast_yyparse
#define yylex           ast_yylex
#define yyerror         ast_yyerror
#define yydebug         ast_yydebug
#define yynerrs         ast_yynerrs
#define yylval          ast_yylval
#define yychar          ast_yychar
#define yylloc          ast_yylloc

/* First part of user prologue.  */
#line 1 "src/core/parser.y"

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

#line 105 "src/core/parser.y"

#include "stdio.h"
extern void ast_yyerror(void *scanner, char *s);

#line 187 "NoctLang/build/parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TOKEN_SYMBOL = 3,               /* TOKEN_SYMBOL  */
  YYSYMBOL_TOKEN_STR = 4,                  /* TOKEN_STR  */
  YYSYMBOL_TOKEN_INT = 5,                  /* TOKEN_INT  */
  YYSYMBOL_TOKEN_FLOAT = 6,                /* TOKEN_FLOAT  */
  YYSYMBOL_TOKEN_FUNC = 7,                 /* TOKEN_FUNC  */
  YYSYMBOL_TOKEN_CLASS = 8,                /* TOKEN_CLASS  */
  YYSYMBOL_TOKEN_NEW = 9,                  /* TOKEN_NEW  */
  YYSYMBOL_TOKEN_LAMBDA = 10,              /* TOKEN_LAMBDA  */
  YYSYMBOL_TOKEN_LARR = 11,                /* TOKEN_LARR  */
  YYSYMBOL_TOKEN_RARR = 12,                /* TOKEN_RARR  */
  YYSYMBOL_TOKEN_PLUS = 13,                /* TOKEN_PLUS  */
  YYSYMBOL_TOKEN_MINUS = 14,               /* TOKEN_MINUS  */
  YYSYMBOL_TOKEN_MUL = 15,                 /* TOKEN_MUL  */
  YYSYMBOL_TOKEN_DIV = 16,                 /* TOKEN_DIV  */
  YYSYMBOL_TOKEN_MOD = 17,                 /* TOKEN_MOD  */
  YYSYMBOL_TOKEN_SHL = 18,                 /* TOKEN_SHL  */
  YYSYMBOL_TOKEN_SHR = 19,                 /* TOKEN_SHR  */
  YYSYMBOL_TOKEN_ASSIGN = 20,              /* TOKEN_ASSIGN  */
  YYSYMBOL_TOKEN_PLUSASSIGN = 21,          /* TOKEN_PLUSASSIGN  */
  YYSYMBOL_TOKEN_MINUSASSIGN = 22,         /* TOKEN_MINUSASSIGN  */
  YYSYMBOL_TOKEN_MULASSIGN = 23,           /* TOKEN_MULASSIGN  */
  YYSYMBOL_TOKEN_DIVASSIGN = 24,           /* TOKEN_DIVASSIGN  */
  YYSYMBOL_TOKEN_MODASSIGN = 25,           /* TOKEN_MODASSIGN  */
  YYSYMBOL_TOKEN_ANDASSIGN = 26,           /* TOKEN_ANDASSIGN  */
  YYSYMBOL_TOKEN_ORASSIGN = 27,            /* TOKEN_ORASSIGN  */
  YYSYMBOL_TOKEN_SHLASSIGN = 28,           /* TOKEN_SHLASSIGN  */
  YYSYMBOL_TOKEN_SHRASSIGN = 29,           /* TOKEN_SHRASSIGN  */
  YYSYMBOL_TOKEN_PLUSPLUS = 30,            /* TOKEN_PLUSPLUS  */
  YYSYMBOL_TOKEN_MINUSMINUS = 31,          /* TOKEN_MINUSMINUS  */
  YYSYMBOL_TOKEN_ANDAND = 32,              /* TOKEN_ANDAND  */
  YYSYMBOL_TOKEN_OROR = 33,                /* TOKEN_OROR  */
  YYSYMBOL_TOKEN_LPAR = 34,                /* TOKEN_LPAR  */
  YYSYMBOL_TOKEN_RPAR = 35,                /* TOKEN_RPAR  */
  YYSYMBOL_TOKEN_RPAR_LBLK = 36,           /* TOKEN_RPAR_LBLK  */
  YYSYMBOL_TOKEN_LBLK = 37,                /* TOKEN_LBLK  */
  YYSYMBOL_TOKEN_LBLK_BLK = 38,            /* TOKEN_LBLK_BLK  */
  YYSYMBOL_TOKEN_RBLK = 39,                /* TOKEN_RBLK  */
  YYSYMBOL_TOKEN_SEMICOLON = 40,           /* TOKEN_SEMICOLON  */
  YYSYMBOL_TOKEN_COLON = 41,               /* TOKEN_COLON  */
  YYSYMBOL_TOKEN_DOT = 42,                 /* TOKEN_DOT  */
  YYSYMBOL_TOKEN_COMMA = 43,               /* TOKEN_COMMA  */
  YYSYMBOL_TOKEN_IF = 44,                  /* TOKEN_IF  */
  YYSYMBOL_TOKEN_ELSE = 45,                /* TOKEN_ELSE  */
  YYSYMBOL_TOKEN_ELSE_LBLK = 46,           /* TOKEN_ELSE_LBLK  */
  YYSYMBOL_TOKEN_ELSEIF = 47,              /* TOKEN_ELSEIF  */
  YYSYMBOL_TOKEN_WHILE = 48,               /* TOKEN_WHILE  */
  YYSYMBOL_TOKEN_FOR = 49,                 /* TOKEN_FOR  */
  YYSYMBOL_TOKEN_IN = 50,                  /* TOKEN_IN  */
  YYSYMBOL_TOKEN_DOTDOT = 51,              /* TOKEN_DOTDOT  */
  YYSYMBOL_TOKEN_GT = 52,                  /* TOKEN_GT  */
  YYSYMBOL_TOKEN_GTE = 53,                 /* TOKEN_GTE  */
  YYSYMBOL_TOKEN_LT = 54,                  /* TOKEN_LT  */
  YYSYMBOL_TOKEN_LTE = 55,                 /* TOKEN_LTE  */
  YYSYMBOL_TOKEN_EQ = 56,                  /* TOKEN_EQ  */
  YYSYMBOL_TOKEN_NEQ = 57,                 /* TOKEN_NEQ  */
  YYSYMBOL_TOKEN_RETURN = 58,              /* TOKEN_RETURN  */
  YYSYMBOL_TOKEN_BREAK = 59,               /* TOKEN_BREAK  */
  YYSYMBOL_TOKEN_CONTINUE = 60,            /* TOKEN_CONTINUE  */
  YYSYMBOL_TOKEN_ARROW = 61,               /* TOKEN_ARROW  */
  YYSYMBOL_TOKEN_RPAR_DARROW_LBLK = 62,    /* TOKEN_RPAR_DARROW_LBLK  */
  YYSYMBOL_TOKEN_AND = 63,                 /* TOKEN_AND  */
  YYSYMBOL_TOKEN_OR = 64,                  /* TOKEN_OR  */
  YYSYMBOL_TOKEN_XOR = 65,                 /* TOKEN_XOR  */
  YYSYMBOL_TOKEN_VAR = 66,                 /* TOKEN_VAR  */
  YYSYMBOL_UNARYMINUS = 67,                /* UNARYMINUS  */
  YYSYMBOL_TOKEN_NOT = 68,                 /* TOKEN_NOT  */
  YYSYMBOL_CALL = 69,                      /* CALL  */
  YYSYMBOL_YYACCEPT = 70,                  /* $accept  */
  YYSYMBOL_func_list = 71,                 /* func_list  */
  YYSYMBOL_func = 72,                      /* func  */
  YYSYMBOL_param_list = 73,                /* param_list  */
  YYSYMBOL_stmt_list = 74,                 /* stmt_list  */
  YYSYMBOL_stmt = 75,                      /* stmt  */
  YYSYMBOL_expr_stmt = 76,                 /* expr_stmt  */
  YYSYMBOL_assign_stmt = 77,               /* assign_stmt  */
  YYSYMBOL_plusassign_stmt = 78,           /* plusassign_stmt  */
  YYSYMBOL_minusassign_stmt = 79,          /* minusassign_stmt  */
  YYSYMBOL_mulassign_stmt = 80,            /* mulassign_stmt  */
  YYSYMBOL_divassign_stmt = 81,            /* divassign_stmt  */
  YYSYMBOL_modassign_stmt = 82,            /* modassign_stmt  */
  YYSYMBOL_andassign_stmt = 83,            /* andassign_stmt  */
  YYSYMBOL_orassign_stmt = 84,             /* orassign_stmt  */
  YYSYMBOL_shlassign_stmt = 85,            /* shlassign_stmt  */
  YYSYMBOL_shrassign_stmt = 86,            /* shrassign_stmt  */
  YYSYMBOL_plusplus_stmt = 87,             /* plusplus_stmt  */
  YYSYMBOL_minusminus_stmt = 88,           /* minusminus_stmt  */
  YYSYMBOL_if_stmt = 89,                   /* if_stmt  */
  YYSYMBOL_elif_stmt = 90,                 /* elif_stmt  */
  YYSYMBOL_else_stmt = 91,                 /* else_stmt  */
  YYSYMBOL_while_stmt = 92,                /* while_stmt  */
  YYSYMBOL_for_stmt = 93,                  /* for_stmt  */
  YYSYMBOL_return_stmt = 94,               /* return_stmt  */
  YYSYMBOL_break_stmt = 95,                /* break_stmt  */
  YYSYMBOL_continue_stmt = 96,             /* continue_stmt  */
  YYSYMBOL_expr = 97,                      /* expr  */
  YYSYMBOL_call_expr = 98,                 /* call_expr  */
  YYSYMBOL_thiscall_expr = 99,             /* thiscall_expr  */
  YYSYMBOL_lambda_expr = 100,              /* lambda_expr  */
  YYSYMBOL_arg_list = 101,                 /* arg_list  */
  YYSYMBOL_kv_list = 102,                  /* kv_list  */
  YYSYMBOL_kv = 103,                       /* kv  */
  YYSYMBOL_term = 104                      /* term  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2442

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  113
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  249

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   324


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   218,   218,   223,   229,   234,   240,   245,   252,   256,
     262,   266,   270,   274,   278,   282,   286,   290,   294,   298,
     302,   306,   310,   314,   318,   322,   326,   330,   334,   338,
     342,   347,   353,   358,   364,   370,   376,   382,   388,   394,
     400,   406,   412,   418,   424,   430,   435,   441,   446,   452,
     457,   463,   468,   474,   479,   484,   489,   494,   499,   505,
     510,   515,   521,   527,   532,   537,   542,   547,   552,   557,
     562,   567,   572,   577,   582,   587,   592,   597,   602,   607,
     612,   617,   622,   627,   632,   637,   642,   647,   651,   655,
     660,   665,   671,   677,   681,   686,   692,   697,   703,   708,
     714,   719,   725,   730,   736,   741,   747,   752,   758,   763,
     768,   773,   778,   783
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "TOKEN_SYMBOL",
  "TOKEN_STR", "TOKEN_INT", "TOKEN_FLOAT", "TOKEN_FUNC", "TOKEN_CLASS",
  "TOKEN_NEW", "TOKEN_LAMBDA", "TOKEN_LARR", "TOKEN_RARR", "TOKEN_PLUS",
  "TOKEN_MINUS", "TOKEN_MUL", "TOKEN_DIV", "TOKEN_MOD", "TOKEN_SHL",
  "TOKEN_SHR", "TOKEN_ASSIGN", "TOKEN_PLUSASSIGN", "TOKEN_MINUSASSIGN",
  "TOKEN_MULASSIGN", "TOKEN_DIVASSIGN", "TOKEN_MODASSIGN",
  "TOKEN_ANDASSIGN", "TOKEN_ORASSIGN", "TOKEN_SHLASSIGN",
  "TOKEN_SHRASSIGN", "TOKEN_PLUSPLUS", "TOKEN_MINUSMINUS", "TOKEN_ANDAND",
  "TOKEN_OROR", "TOKEN_LPAR", "TOKEN_RPAR", "TOKEN_RPAR_LBLK",
  "TOKEN_LBLK", "TOKEN_LBLK_BLK", "TOKEN_RBLK", "TOKEN_SEMICOLON",
  "TOKEN_COLON", "TOKEN_DOT", "TOKEN_COMMA", "TOKEN_IF", "TOKEN_ELSE",
  "TOKEN_ELSE_LBLK", "TOKEN_ELSEIF", "TOKEN_WHILE", "TOKEN_FOR",
  "TOKEN_IN", "TOKEN_DOTDOT", "TOKEN_GT", "TOKEN_GTE", "TOKEN_LT",
  "TOKEN_LTE", "TOKEN_EQ", "TOKEN_NEQ", "TOKEN_RETURN", "TOKEN_BREAK",
  "TOKEN_CONTINUE", "TOKEN_ARROW", "TOKEN_RPAR_DARROW_LBLK", "TOKEN_AND",
  "TOKEN_OR", "TOKEN_XOR", "TOKEN_VAR", "UNARYMINUS", "TOKEN_NOT", "CALL",
  "$accept", "func_list", "func", "param_list", "stmt_list", "stmt",
  "expr_stmt", "assign_stmt", "plusassign_stmt", "minusassign_stmt",
  "mulassign_stmt", "divassign_stmt", "modassign_stmt", "andassign_stmt",
  "orassign_stmt", "shlassign_stmt", "shrassign_stmt", "plusplus_stmt",
  "minusminus_stmt", "if_stmt", "elif_stmt", "else_stmt", "while_stmt",
  "for_stmt", "return_stmt", "break_stmt", "continue_stmt", "expr",
  "call_expr", "thiscall_expr", "lambda_expr", "arg_list", "kv_list", "kv",
  "term", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-107)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-7)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       4,    38,    49,  -107,    13,  -107,  -107,    -1,  -107,  -107,
      75,   128,  -107,    41,  -107,  -107,  -107,  -107,    15,    52,
      25,   541,   277,     6,  -107,    34,   816,  -107,    39,    40,
      65,   343,    70,    72,   541,   541,  -107,  -107,  -107,  -107,
    -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,
    -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,   872,  -107,
    -107,  -107,  -107,   195,  -107,   110,    87,  -107,  2082,    -5,
    2082,   -30,  -107,    10,  1257,    85,    89,  -107,   -27,  -107,
     541,  -107,   261,   541,   541,   122,  -107,  1312,  -107,  -107,
    1367,  2169,   541,   541,   541,   541,   541,   541,   541,   541,
     541,   541,   541,   541,   541,   541,   541,   541,   541,   541,
      95,    98,   541,   541,   409,  -107,   138,   541,   541,   541,
     541,   541,   541,   147,   541,   541,   541,  -107,  -107,    55,
     113,  -107,   541,   327,  -107,  -107,   541,   541,  -107,    54,
     982,  -107,  1037,  1092,    78,  -107,   541,  1422,   129,   658,
     191,   282,     9,   724,   262,  1477,  1532,  1587,  1642,  1697,
    1752,  1807,  1862,  1917,  1972,  -107,  -107,  2169,  2137,  -107,
       5,  -107,     8,  2351,  2291,  2321,   797,  2381,   117,  2231,
    2201,  2261,  -107,  -107,    84,  2082,  -107,   393,  2082,  2082,
    -107,   816,  -107,   816,  -107,   816,  -107,   150,   541,  2027,
    -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,
    -107,  -107,   475,  -107,  -107,  -107,   459,  -107,   525,  -107,
     591,   104,   927,  -107,  -107,    11,  -107,  -107,  -107,   541,
     816,  -107,   541,  -107,  1147,  -107,   657,  1202,   816,  -107,
    -107,   816,  -107,  -107,   723,  -107,   789,  -107,  -107
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     2,     0,     1,     3,     0,     6,     8,
       0,     0,     8,     0,   111,   110,   108,   109,     0,     0,
       0,     0,     0,     0,     5,     0,     0,     8,     0,     0,
       0,     0,     0,     0,     0,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,     0,    87,
      88,    93,    63,     0,     7,     0,     0,   112,   102,     0,
      84,   111,     8,     0,     0,     0,     0,   113,     0,   104,
       0,    50,     0,     0,     0,     0,    60,     0,    61,    62,
       0,    85,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    31,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     4,    92,     0,
       0,    89,     0,     0,     8,    64,     0,     0,    90,     0,
       0,    49,     0,     0,     0,    59,     0,     0,    77,    78,
      79,    80,    81,    82,    83,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,    70,    69,    97,
       0,    86,    73,    74,    71,    72,    75,    76,     0,    67,
      66,    68,    91,    95,     0,   103,   101,     0,   107,   106,
     105,     0,     8,     0,     8,     0,     8,     0,     0,     0,
      65,    32,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    96,     0,    94,   100,    46,     0,    48,     0,    52,
       0,     0,     0,    33,    99,     0,    45,    47,    51,     0,
       0,     8,     0,    98,     0,    56,     0,     0,     0,     8,
      55,     0,     8,    54,     0,    58,     0,    53,    57
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -107,  -107,   153,   134,   -12,   -25,  -107,  -107,  -107,  -107,
    -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,
    -107,  -107,  -107,  -107,  -107,  -107,  -107,   -17,  -107,  -107,
    -107,  -106,   -59,    18,  -107
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     2,     3,    10,    11,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    69,    78,    79,    62
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      63,    81,     8,    68,    70,    74,   129,   131,   170,    75,
      76,     1,   138,    -6,    87,    82,   139,    90,    91,    92,
      92,    93,    94,    95,    96,    97,    98,    99,    14,    15,
      16,    17,    -6,    18,    19,     9,    20,    67,   132,    21,
     211,     4,   114,   114,    64,    77,   233,     7,   132,     5,
     116,   116,    65,    13,   132,    66,     1,    75,    76,    22,
     133,   118,    23,   140,   121,   122,   142,   143,    80,   123,
     123,   184,   134,    83,    84,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,    35,   182,   167,   168,    68,   139,    85,
     172,   173,   174,   175,   176,   177,   225,   179,   180,   181,
      88,    12,    89,    75,    76,   185,    75,    76,    13,   188,
     189,   197,   187,   213,   130,   144,   136,   139,   198,   199,
     137,    14,    15,    16,    17,   165,    18,    19,   166,    20,
      92,   171,    21,    94,    95,    96,    97,    98,    99,   128,
     178,   212,   183,   221,   229,     6,    73,   190,     0,     0,
       0,     0,    22,   114,     0,    23,   215,    24,   217,     0,
     219,   116,    25,    26,    27,    28,    29,    30,     0,     0,
     216,   222,   218,     0,   220,     0,    31,    32,    33,     0,
     123,     0,     0,     0,    34,    68,    35,     0,    14,    15,
      16,    17,    92,    18,    19,   235,    20,    96,    97,    21,
       0,     0,   234,   243,     0,   237,   245,     0,     0,   236,
       0,     0,     0,     0,     0,   114,     0,   244,     0,    22,
     246,     0,    23,   116,   127,     0,     0,     0,     0,    25,
      26,    27,    28,    29,    30,     0,     0,     0,     0,     0,
       0,     0,   123,    31,    32,    33,     0,     0,     0,     0,
       0,    34,     0,    35,    14,    15,    16,    17,     0,    18,
      19,     0,    20,    92,     0,    21,     0,    95,    96,    97,
      71,    15,    16,    17,     0,    18,    19,     0,    20,     0,
       0,    21,     0,    92,     0,    22,   114,     0,    23,    97,
     141,     0,     0,     0,   116,    25,    26,    27,    28,    29,
      30,    22,     0,     0,    23,     0,   114,     0,     0,    31,
      32,    33,     0,   123,   116,     0,     0,    34,     0,    35,
      14,    15,    16,    17,     0,    18,    19,     0,    20,    72,
       0,    21,     0,   123,     0,    35,    14,    15,    16,    17,
       0,    18,    19,     0,    20,     0,     0,    21,     0,     0,
       0,    22,     0,     0,    23,     0,   186,     0,     0,     0,
       0,    25,    26,    27,    28,    29,    30,    22,     0,     0,
      23,     0,     0,    86,     0,    31,    32,    33,     0,     0,
       0,     0,     0,    34,     0,    35,    14,    15,    16,    17,
       0,    18,    19,     0,    20,     0,     0,    21,     0,     0,
       0,    35,    14,    15,    16,    17,     0,    18,    19,     0,
      20,     0,     0,    21,     0,     0,     0,    22,     0,     0,
      23,     0,   214,     0,     0,     0,     0,    25,    26,    27,
      28,    29,    30,    22,   169,     0,    23,     0,     0,     0,
       0,    31,    32,    33,     0,     0,     0,     0,     0,    34,
       0,    35,    14,    15,    16,    17,     0,    18,    19,     0,
      20,     0,     0,    21,     0,     0,     0,    35,    14,    15,
      16,    17,     0,    18,    19,     0,    20,     0,     0,    21,
       0,     0,     0,    22,     0,     0,    23,     0,   226,     0,
       0,     0,     0,    25,    26,    27,    28,    29,    30,    22,
     224,     0,    23,     0,     0,     0,     0,    31,    32,    33,
       0,     0,     0,     0,     0,    34,     0,    35,    14,    15,
      16,    17,     0,    18,    19,     0,    20,     0,     0,    21,
       0,     0,     0,    35,    14,    15,    16,    17,     0,    18,
      19,     0,    20,     0,     0,    21,     0,     0,     0,    22,
       0,     0,    23,     0,   227,     0,     0,     0,     0,    25,
      26,    27,    28,    29,    30,    22,     0,     0,    23,     0,
       0,     0,     0,    31,    32,    33,     0,     0,     0,     0,
       0,    34,     0,    35,    14,    15,    16,    17,     0,    18,
      19,     0,    20,     0,     0,    21,     0,     0,     0,    35,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    22,     0,     0,    23,     0,
     228,     0,     0,     0,     0,    25,    26,    27,    28,    29,
      30,     0,     0,     0,     0,     0,     0,     0,     0,    31,
      32,    33,     0,     0,     0,     0,     0,    34,     0,    35,
      14,    15,    16,    17,     0,    18,    19,     0,    20,    92,
       0,    21,     0,    95,    96,    97,    98,    99,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    22,   114,     0,    23,     0,   240,     0,     0,     0,
     116,    25,    26,    27,    28,    29,    30,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    32,    33,     0,   123,
       0,     0,     0,    34,     0,    35,    14,    15,    16,    17,
       0,    18,    19,     0,    20,    92,     0,    21,     0,    95,
      96,    97,     0,    99,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    22,   114,     0,
      23,     0,   247,     0,     0,     0,   116,    25,    26,    27,
      28,    29,    30,     0,     0,     0,     0,     0,     0,     0,
       0,    31,    32,    33,     0,   123,     0,     0,     0,    34,
       0,    35,    14,    15,    16,    17,     0,    18,    19,     0,
      20,     0,     0,    21,     0,     0,     0,     0,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,    14,
      15,    16,    17,    22,    18,    19,    23,    20,   248,     0,
      21,   114,     0,    25,    26,    27,    28,    29,    30,   116,
       0,     0,     0,     0,     0,     0,     0,    31,    32,    33,
      22,     0,     0,    23,   122,    34,     0,    35,   123,     0,
      25,    26,    27,    28,    29,    30,     0,     0,     0,     0,
       0,     0,     0,     0,    31,    32,    33,     0,     0,     0,
       0,     0,    34,    92,    35,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,     0,     0,     0,
       0,     0,   115,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,   230,   231,     0,     0,     0,     0,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,   232,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,   191,   192,     0,
       0,     0,     0,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,   193,   194,     0,     0,     0,     0,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,   195,   196,     0,
       0,     0,     0,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,   238,   239,     0,     0,     0,     0,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,   241,   242,     0,
       0,     0,     0,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,   135,     0,     0,     0,     0,     0,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,   145,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,   146,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,     0,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,   200,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,     0,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,   201,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,   202,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,   203,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,   204,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,   205,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,   206,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,   207,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,   208,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,   209,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,   210,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,   114,     0,     0,     0,     0,     0,   223,     0,   116,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,    92,     0,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,   113,   114,     0,     0,     0,
       0,     0,     0,     0,   116,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,   118,   119,   120,   121,   122,
       0,     0,     0,   123,     0,   124,   125,   126,    92,     0,
      93,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
       0,   114,     0,     0,     0,     0,     0,     0,     0,   116,
      92,     0,    93,    94,    95,    96,    97,    98,    99,   117,
     118,   119,   120,   121,   122,     0,     0,     0,   123,     0,
     124,   125,   126,   114,     0,     0,     0,     0,     0,     0,
       0,   116,    92,     0,    93,    94,    95,    96,    97,    98,
      99,   117,   118,   119,   120,   121,   122,     0,     0,     0,
     123,     0,   124,   125,   126,   114,     0,     0,     0,     0,
       0,     0,    92,   116,    93,    94,    95,    96,    97,    98,
      99,     0,     0,   117,   118,   119,   120,   121,   122,     0,
       0,     0,   123,     0,   124,   114,   126,     0,     0,     0,
       0,     0,    92,   116,    93,    94,    95,    96,    97,    98,
      99,     0,     0,   117,   118,   119,   120,   121,   122,     0,
       0,     0,   123,     0,     0,   114,   126,     0,     0,     0,
       0,     0,    92,   116,    93,    94,    95,    96,    97,    98,
      99,     0,     0,   117,   118,   119,   120,   121,   122,     0,
       0,     0,   123,     0,     0,   114,     0,     0,     0,     0,
       0,     0,    92,   116,    93,    94,    95,    96,    97,    98,
      99,     0,     0,   117,   118,     0,   120,   121,   122,     0,
       0,     0,   123,     0,     0,   114,     0,     0,     0,     0,
       0,     0,    92,   116,    93,    94,    95,    96,    97,    98,
      99,     0,     0,   117,   118,     0,     0,   121,   122,     0,
       0,     0,   123,     0,     0,   114,     0,     0,     0,     0,
       0,     0,    92,   116,    93,    94,    95,    96,    97,    98,
      99,     0,     0,     0,     0,     0,     0,   121,   122,     0,
       0,     0,   123,     0,     0,   114,     0,     0,     0,     0,
       0,     0,     0,   116,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   123
};

static const yytype_int16 yycheck[] =
{
      12,    26,     3,    20,    21,    22,    65,    12,   114,     3,
       4,     7,    39,    43,    31,    27,    43,    34,    35,    11,
      11,    13,    14,    15,    16,    17,    18,    19,     3,     4,
       5,     6,    62,     8,     9,    36,    11,    12,    43,    14,
      35,     3,    34,    34,     3,    39,    35,    34,    43,     0,
      42,    42,    37,    43,    43,     3,     7,     3,     4,    34,
      72,    53,    37,    80,    56,    57,    83,    84,    34,    61,
      61,   130,    62,    34,    34,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,    68,    39,   112,   113,   114,    43,    34,
     117,   118,   119,   120,   121,   122,   212,   124,   125,   126,
      40,    36,    40,     3,     4,   132,     3,     4,    43,   136,
     137,    43,   134,    39,    37,     3,    41,    43,    50,   146,
      41,     3,     4,     5,     6,    40,     8,     9,    40,    11,
      11,     3,    14,    14,    15,    16,    17,    18,    19,    39,
       3,    34,    39,     3,    50,     2,    22,   139,    -1,    -1,
      -1,    -1,    34,    34,    -1,    37,   191,    39,   193,    -1,
     195,    42,    44,    45,    46,    47,    48,    49,    -1,    -1,
     192,   198,   194,    -1,   196,    -1,    58,    59,    60,    -1,
      61,    -1,    -1,    -1,    66,   212,    68,    -1,     3,     4,
       5,     6,    11,     8,     9,   230,    11,    16,    17,    14,
      -1,    -1,   229,   238,    -1,   232,   241,    -1,    -1,   231,
      -1,    -1,    -1,    -1,    -1,    34,    -1,   239,    -1,    34,
     242,    -1,    37,    42,    39,    -1,    -1,    -1,    -1,    44,
      45,    46,    47,    48,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    61,    58,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    -1,    68,     3,     4,     5,     6,    -1,     8,
       9,    -1,    11,    11,    -1,    14,    -1,    15,    16,    17,
       3,     4,     5,     6,    -1,     8,     9,    -1,    11,    -1,
      -1,    14,    -1,    11,    -1,    34,    34,    -1,    37,    17,
      39,    -1,    -1,    -1,    42,    44,    45,    46,    47,    48,
      49,    34,    -1,    -1,    37,    -1,    34,    -1,    -1,    58,
      59,    60,    -1,    61,    42,    -1,    -1,    66,    -1,    68,
       3,     4,     5,     6,    -1,     8,     9,    -1,    11,    62,
      -1,    14,    -1,    61,    -1,    68,     3,     4,     5,     6,
      -1,     8,     9,    -1,    11,    -1,    -1,    14,    -1,    -1,
      -1,    34,    -1,    -1,    37,    -1,    39,    -1,    -1,    -1,
      -1,    44,    45,    46,    47,    48,    49,    34,    -1,    -1,
      37,    -1,    -1,    40,    -1,    58,    59,    60,    -1,    -1,
      -1,    -1,    -1,    66,    -1,    68,     3,     4,     5,     6,
      -1,     8,     9,    -1,    11,    -1,    -1,    14,    -1,    -1,
      -1,    68,     3,     4,     5,     6,    -1,     8,     9,    -1,
      11,    -1,    -1,    14,    -1,    -1,    -1,    34,    -1,    -1,
      37,    -1,    39,    -1,    -1,    -1,    -1,    44,    45,    46,
      47,    48,    49,    34,    35,    -1,    37,    -1,    -1,    -1,
      -1,    58,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,
      -1,    68,     3,     4,     5,     6,    -1,     8,     9,    -1,
      11,    -1,    -1,    14,    -1,    -1,    -1,    68,     3,     4,
       5,     6,    -1,     8,     9,    -1,    11,    -1,    -1,    14,
      -1,    -1,    -1,    34,    -1,    -1,    37,    -1,    39,    -1,
      -1,    -1,    -1,    44,    45,    46,    47,    48,    49,    34,
      35,    -1,    37,    -1,    -1,    -1,    -1,    58,    59,    60,
      -1,    -1,    -1,    -1,    -1,    66,    -1,    68,     3,     4,
       5,     6,    -1,     8,     9,    -1,    11,    -1,    -1,    14,
      -1,    -1,    -1,    68,     3,     4,     5,     6,    -1,     8,
       9,    -1,    11,    -1,    -1,    14,    -1,    -1,    -1,    34,
      -1,    -1,    37,    -1,    39,    -1,    -1,    -1,    -1,    44,
      45,    46,    47,    48,    49,    34,    -1,    -1,    37,    -1,
      -1,    -1,    -1,    58,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    -1,    68,     3,     4,     5,     6,    -1,     8,
       9,    -1,    11,    -1,    -1,    14,    -1,    -1,    -1,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,    37,    -1,
      39,    -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      59,    60,    -1,    -1,    -1,    -1,    -1,    66,    -1,    68,
       3,     4,     5,     6,    -1,     8,     9,    -1,    11,    11,
      -1,    14,    -1,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    34,    -1,    37,    -1,    39,    -1,    -1,    -1,
      42,    44,    45,    46,    47,    48,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    60,    -1,    61,
      -1,    -1,    -1,    66,    -1,    68,     3,     4,     5,     6,
      -1,     8,     9,    -1,    11,    11,    -1,    14,    -1,    15,
      16,    17,    -1,    19,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    34,    -1,
      37,    -1,    39,    -1,    -1,    -1,    42,    44,    45,    46,
      47,    48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    58,    59,    60,    -1,    61,    -1,    -1,    -1,    66,
      -1,    68,     3,     4,     5,     6,    -1,     8,     9,    -1,
      11,    -1,    -1,    14,    -1,    -1,    -1,    -1,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,     3,
       4,     5,     6,    34,     8,     9,    37,    11,    39,    -1,
      14,    34,    -1,    44,    45,    46,    47,    48,    49,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    60,
      34,    -1,    -1,    37,    57,    66,    -1,    68,    61,    -1,
      44,    45,    46,    47,    48,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    58,    59,    60,    -1,    -1,    -1,
      -1,    -1,    66,    11,    68,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    35,    36,    -1,    -1,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    35,    36,    -1,
      -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    35,    36,    -1,    -1,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    35,    36,    -1,
      -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    35,    36,    -1,    -1,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    35,    36,    -1,
      -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    20,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    40,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    -1,    63,    64,    65,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      11,    -1,    13,    14,    15,    16,    17,    18,    19,    52,
      53,    54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,
      63,    64,    65,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    42,    11,    -1,    13,    14,    15,    16,    17,    18,
      19,    52,    53,    54,    55,    56,    57,    -1,    -1,    -1,
      61,    -1,    63,    64,    65,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    11,    42,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    52,    53,    54,    55,    56,    57,    -1,
      -1,    -1,    61,    -1,    63,    34,    65,    -1,    -1,    -1,
      -1,    -1,    11,    42,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    52,    53,    54,    55,    56,    57,    -1,
      -1,    -1,    61,    -1,    -1,    34,    65,    -1,    -1,    -1,
      -1,    -1,    11,    42,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    52,    53,    54,    55,    56,    57,    -1,
      -1,    -1,    61,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    11,    42,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    52,    53,    -1,    55,    56,    57,    -1,
      -1,    -1,    61,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    11,    42,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    52,    53,    -1,    -1,    56,    57,    -1,
      -1,    -1,    61,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    11,    42,    13,    14,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    -1,    56,    57,    -1,
      -1,    -1,    61,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    61
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     7,    71,    72,     3,     0,    72,    34,     3,    36,
      73,    74,    36,    43,     3,     4,     5,     6,     8,     9,
      11,    14,    34,    37,    39,    44,    45,    46,    47,    48,
      49,    58,    59,    60,    66,    68,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   104,    74,     3,    37,     3,    12,    97,   101,
      97,     3,    62,    73,    97,     3,     4,    39,   102,   103,
      34,    75,    74,    34,    34,    34,    40,    97,    40,    40,
      97,    97,    11,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    40,    42,    52,    53,    54,
      55,    56,    57,    61,    63,    64,    65,    39,    39,   102,
      37,    12,    43,    74,    62,    35,    41,    41,    39,    43,
      97,    39,    97,    97,     3,    40,    20,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    40,    40,    97,    97,    35,
     101,     3,    97,    97,    97,    97,    97,    97,     3,    97,
      97,    97,    39,    39,   102,    97,    39,    74,    97,    97,
     103,    35,    36,    35,    36,    35,    36,    43,    50,    97,
      12,    40,    40,    40,    40,    40,    40,    40,    40,    40,
      40,    35,    34,    39,    39,    75,    74,    75,    74,    75,
      74,     3,    97,    40,    35,   101,    39,    39,    39,    50,
      35,    36,    51,    35,    97,    75,    74,    97,    35,    36,
      39,    35,    36,    75,    74,    75,    74,    39,    39
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    70,    71,    71,    72,    72,    73,    73,    74,    74,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    76,    77,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    89,    90,    90,    91,
      91,    92,    92,    93,    93,    93,    93,    93,    93,    94,
      94,    95,    96,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    98,    98,    99,    99,
     100,   100,   101,   101,   102,   102,   103,   103,   104,   104,
     104,   104,   104,   104
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     7,     6,     1,     3,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     4,     5,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     3,     3,     6,     5,     6,     5,     3,
       2,     6,     5,    10,     9,     8,     7,    10,     9,     3,
       2,     2,     2,     1,     3,     4,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     3,     1,     1,     3,
       3,     4,     3,     1,     5,     4,     4,     3,     6,     5,
       5,     4,     1,     3,     1,     3,     3,     3,     1,     1,
       1,     1,     2,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, void *scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (scanner);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, void *scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, void *scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, void *scanner)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void *scanner)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */


/* User initialization code.  */
#line 212 "src/core/parser.y"
{
	ast_yylloc.last_line = yylloc.first_line = 0;
	ast_yylloc.last_column = yylloc.first_column = 0;
}

#line 1807 "NoctLang/build/parser.tab.c"

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* func_list: func  */
#line 219 "src/core/parser.y"
                {
			(yyval.func_list) = ast_accept_func_list(NULL, (yyvsp[0].func));
			debug("func_list: class");
		}
#line 2023 "NoctLang/build/parser.tab.c"
    break;

  case 3: /* func_list: func_list func  */
#line 224 "src/core/parser.y"
                {
			(yyval.func_list) = ast_accept_func_list((yyvsp[-1].func_list), (yyvsp[0].func));
			debug("func_list: func_list func");
		}
#line 2032 "NoctLang/build/parser.tab.c"
    break;

  case 4: /* func: TOKEN_FUNC TOKEN_SYMBOL TOKEN_LPAR param_list TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 230 "src/core/parser.y"
                {
			(yyval.func) = ast_accept_func((yyvsp[-5].sval), (yyvsp[-3].param_list), (yyvsp[-1].stmt_list));
			debug("func: func name(param_list) { stmt_list }");
		}
#line 2041 "NoctLang/build/parser.tab.c"
    break;

  case 5: /* func: TOKEN_FUNC TOKEN_SYMBOL TOKEN_LPAR TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 235 "src/core/parser.y"
                {
			(yyval.func) = ast_accept_func((yyvsp[-4].sval), NULL, (yyvsp[-1].stmt_list));
			debug("func: func name() { stmt_list }");
		}
#line 2050 "NoctLang/build/parser.tab.c"
    break;

  case 6: /* param_list: TOKEN_SYMBOL  */
#line 241 "src/core/parser.y"
                {
			(yyval.param_list) = ast_accept_param_list(NULL, (yyvsp[0].sval));
			debug("param_list: symbol");
		}
#line 2059 "NoctLang/build/parser.tab.c"
    break;

  case 7: /* param_list: param_list TOKEN_COMMA TOKEN_SYMBOL  */
#line 246 "src/core/parser.y"
                {
			(yyval.param_list) = ast_accept_param_list((yyvsp[-2].param_list), (yyvsp[0].sval));
			debug("param_list: param_list symbol");
		}
#line 2068 "NoctLang/build/parser.tab.c"
    break;

  case 8: /* stmt_list: %empty  */
#line 252 "src/core/parser.y"
                {
			(yyval.stmt_list) = ast_accept_stmt_list(NULL, NULL);
			debug("stmt_list: empty");
		}
#line 2077 "NoctLang/build/parser.tab.c"
    break;

  case 9: /* stmt_list: stmt_list stmt  */
#line 257 "src/core/parser.y"
                {
			(yyval.stmt_list) = ast_accept_stmt_list((yyvsp[-1].stmt_list), (yyvsp[0].stmt));
			debug("stmt_list: stmt_list stmt");
		}
#line 2086 "NoctLang/build/parser.tab.c"
    break;

  case 10: /* stmt: expr_stmt  */
#line 263 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2094 "NoctLang/build/parser.tab.c"
    break;

  case 11: /* stmt: assign_stmt  */
#line 267 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2102 "NoctLang/build/parser.tab.c"
    break;

  case 12: /* stmt: plusassign_stmt  */
#line 271 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2110 "NoctLang/build/parser.tab.c"
    break;

  case 13: /* stmt: minusassign_stmt  */
#line 275 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2118 "NoctLang/build/parser.tab.c"
    break;

  case 14: /* stmt: mulassign_stmt  */
#line 279 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2126 "NoctLang/build/parser.tab.c"
    break;

  case 15: /* stmt: divassign_stmt  */
#line 283 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2134 "NoctLang/build/parser.tab.c"
    break;

  case 16: /* stmt: modassign_stmt  */
#line 287 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2142 "NoctLang/build/parser.tab.c"
    break;

  case 17: /* stmt: andassign_stmt  */
#line 291 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2150 "NoctLang/build/parser.tab.c"
    break;

  case 18: /* stmt: orassign_stmt  */
#line 295 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2158 "NoctLang/build/parser.tab.c"
    break;

  case 19: /* stmt: shlassign_stmt  */
#line 299 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2166 "NoctLang/build/parser.tab.c"
    break;

  case 20: /* stmt: shrassign_stmt  */
#line 303 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2174 "NoctLang/build/parser.tab.c"
    break;

  case 21: /* stmt: plusplus_stmt  */
#line 307 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2182 "NoctLang/build/parser.tab.c"
    break;

  case 22: /* stmt: minusminus_stmt  */
#line 311 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2190 "NoctLang/build/parser.tab.c"
    break;

  case 23: /* stmt: if_stmt  */
#line 315 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2198 "NoctLang/build/parser.tab.c"
    break;

  case 24: /* stmt: elif_stmt  */
#line 319 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2206 "NoctLang/build/parser.tab.c"
    break;

  case 25: /* stmt: else_stmt  */
#line 323 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2214 "NoctLang/build/parser.tab.c"
    break;

  case 26: /* stmt: while_stmt  */
#line 327 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2222 "NoctLang/build/parser.tab.c"
    break;

  case 27: /* stmt: for_stmt  */
#line 331 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2230 "NoctLang/build/parser.tab.c"
    break;

  case 28: /* stmt: return_stmt  */
#line 335 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2238 "NoctLang/build/parser.tab.c"
    break;

  case 29: /* stmt: break_stmt  */
#line 339 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2246 "NoctLang/build/parser.tab.c"
    break;

  case 30: /* stmt: continue_stmt  */
#line 343 "src/core/parser.y"
                {
			(yyval.stmt) = (yyvsp[0].stmt);
		}
#line 2254 "NoctLang/build/parser.tab.c"
    break;

  case 31: /* expr_stmt: expr TOKEN_SEMICOLON  */
#line 348 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_expr_stmt((yylsp[-1]).first_line + 1, (yyvsp[-1].expr));
			debug("expr_stmt");
		}
#line 2263 "NoctLang/build/parser.tab.c"
    break;

  case 32: /* assign_stmt: expr TOKEN_ASSIGN expr TOKEN_SEMICOLON  */
#line 354 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_assign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr), false);
			debug("assign_stmt");
		}
#line 2272 "NoctLang/build/parser.tab.c"
    break;

  case 33: /* assign_stmt: TOKEN_VAR expr TOKEN_ASSIGN expr TOKEN_SEMICOLON  */
#line 359 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_assign_stmt((yylsp[-4]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr), true);
			debug("var assign_stmt");
		}
#line 2281 "NoctLang/build/parser.tab.c"
    break;

  case 34: /* plusassign_stmt: expr TOKEN_PLUSASSIGN expr TOKEN_SEMICOLON  */
#line 365 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_plusassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("plusassign_stmt");
		}
#line 2290 "NoctLang/build/parser.tab.c"
    break;

  case 35: /* minusassign_stmt: expr TOKEN_MINUSASSIGN expr TOKEN_SEMICOLON  */
#line 371 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_minusassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("minusassign_stmt");
		}
#line 2299 "NoctLang/build/parser.tab.c"
    break;

  case 36: /* mulassign_stmt: expr TOKEN_MULASSIGN expr TOKEN_SEMICOLON  */
#line 377 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_mulassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("mulassign_stmt");
		}
#line 2308 "NoctLang/build/parser.tab.c"
    break;

  case 37: /* divassign_stmt: expr TOKEN_DIVASSIGN expr TOKEN_SEMICOLON  */
#line 383 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_divassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("divassign_stmt");
		}
#line 2317 "NoctLang/build/parser.tab.c"
    break;

  case 38: /* modassign_stmt: expr TOKEN_MODASSIGN expr TOKEN_SEMICOLON  */
#line 389 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_modassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("modassign_stmt");
		}
#line 2326 "NoctLang/build/parser.tab.c"
    break;

  case 39: /* andassign_stmt: expr TOKEN_ANDASSIGN expr TOKEN_SEMICOLON  */
#line 395 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_andassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("andassign_stmt");
		}
#line 2335 "NoctLang/build/parser.tab.c"
    break;

  case 40: /* orassign_stmt: expr TOKEN_ORASSIGN expr TOKEN_SEMICOLON  */
#line 401 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_orassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("orassign_stmt");
		}
#line 2344 "NoctLang/build/parser.tab.c"
    break;

  case 41: /* shlassign_stmt: expr TOKEN_SHLASSIGN expr TOKEN_SEMICOLON  */
#line 407 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_shlassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("shlassign_stmt");
		}
#line 2353 "NoctLang/build/parser.tab.c"
    break;

  case 42: /* shrassign_stmt: expr TOKEN_SHRASSIGN expr TOKEN_SEMICOLON  */
#line 413 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_shrassign_stmt((yylsp[-3]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("shrassign_stmt");
		}
#line 2362 "NoctLang/build/parser.tab.c"
    break;

  case 43: /* plusplus_stmt: expr TOKEN_PLUSPLUS TOKEN_SEMICOLON  */
#line 419 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_plusplus_stmt((yylsp[-2]).first_line + 1, (yyvsp[-2].expr));
			debug("plusplus_stmt");
		}
#line 2371 "NoctLang/build/parser.tab.c"
    break;

  case 44: /* minusminus_stmt: expr TOKEN_MINUSMINUS TOKEN_SEMICOLON  */
#line 425 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_minusminus_stmt((yylsp[-2]).first_line + 1, (yyvsp[-2].expr));
			debug("plusplus_stmt");
		}
#line 2380 "NoctLang/build/parser.tab.c"
    break;

  case 45: /* if_stmt: TOKEN_IF TOKEN_LPAR expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 431 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_if_stmt((yylsp[-5]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].stmt_list));
			debug("if_stmt: stmt_list");
		}
#line 2389 "NoctLang/build/parser.tab.c"
    break;

  case 46: /* if_stmt: TOKEN_IF TOKEN_LPAR expr TOKEN_RPAR stmt  */
#line 436 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_if_stmt_single((yylsp[-4]).first_line + 1, (yyvsp[-2].expr), (yyvsp[0].stmt));
			debug("if_stmt: stmt_list");
		}
#line 2398 "NoctLang/build/parser.tab.c"
    break;

  case 47: /* elif_stmt: TOKEN_ELSEIF TOKEN_LPAR expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 442 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_elif_stmt((yylsp[-5]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].stmt_list));
			debug("elif_stmt: stmt_list");
		}
#line 2407 "NoctLang/build/parser.tab.c"
    break;

  case 48: /* elif_stmt: TOKEN_ELSEIF TOKEN_LPAR expr TOKEN_RPAR stmt  */
#line 447 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_elif_stmt_single((yylsp[-4]).first_line + 1, (yyvsp[-2].expr), (yyvsp[0].stmt));
			debug("elif_stmt: stmt_list");
		}
#line 2416 "NoctLang/build/parser.tab.c"
    break;

  case 49: /* else_stmt: TOKEN_ELSE_LBLK stmt_list TOKEN_RBLK  */
#line 453 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_else_stmt((yylsp[-2]).first_line + 1, (yyvsp[-1].stmt_list));
			debug("else_stmt: stmt_list");
		}
#line 2425 "NoctLang/build/parser.tab.c"
    break;

  case 50: /* else_stmt: TOKEN_ELSE stmt  */
#line 458 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_else_stmt_single((yylsp[-1]).first_line + 1, (yyvsp[0].stmt));
			debug("else_stmt: stmt_list");
		}
#line 2434 "NoctLang/build/parser.tab.c"
    break;

  case 51: /* while_stmt: TOKEN_WHILE TOKEN_LPAR expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 464 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_while_stmt((yylsp[-5]).first_line + 1, (yyvsp[-3].expr), (yyvsp[-1].stmt_list));
			debug("while_stmt: stmt_list");
		}
#line 2443 "NoctLang/build/parser.tab.c"
    break;

  case 52: /* while_stmt: TOKEN_WHILE TOKEN_LPAR expr TOKEN_RPAR stmt  */
#line 469 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_while_stmt_single((yylsp[-4]).first_line + 1, (yyvsp[-2].expr), (yyvsp[0].stmt));
			debug("while_stmt: stmt_list");
		}
#line 2452 "NoctLang/build/parser.tab.c"
    break;

  case 53: /* for_stmt: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_COMMA TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 475 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_for_kv_stmt((yylsp[-9]).first_line + 1, (yyvsp[-7].sval), (yyvsp[-5].sval), (yyvsp[-3].expr), (yyvsp[-1].stmt_list));
			debug("for_stmt: for(k, v in array) { stmt_list }");
		}
#line 2461 "NoctLang/build/parser.tab.c"
    break;

  case 54: /* for_stmt: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_COMMA TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR stmt  */
#line 480 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_for_kv_stmt_single((yylsp[-8]).first_line + 1, (yyvsp[-6].sval), (yyvsp[-4].sval), (yyvsp[-2].expr), (yyvsp[0].stmt));
			debug("for_stmt: for(k, v in array) stmt");
		}
#line 2470 "NoctLang/build/parser.tab.c"
    break;

  case 55: /* for_stmt: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 485 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_for_v_stmt((yylsp[-7]).first_line + 1, (yyvsp[-5].sval), (yyvsp[-3].expr), (yyvsp[-1].stmt_list));
			debug("for_stmt: for(v in array) { stmt_list }");
		}
#line 2479 "NoctLang/build/parser.tab.c"
    break;

  case 56: /* for_stmt: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_RPAR stmt  */
#line 490 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_for_v_stmt_single((yylsp[-6]).first_line + 1, (yyvsp[-4].sval), (yyvsp[-2].expr), (yyvsp[0].stmt));
			debug("for_stmt: for(v in array) stmt_list");
		}
#line 2488 "NoctLang/build/parser.tab.c"
    break;

  case 57: /* for_stmt: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_DOTDOT expr TOKEN_RPAR_LBLK stmt_list TOKEN_RBLK  */
#line 495 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_for_range_stmt((yylsp[-9]).first_line + 1, (yyvsp[-7].sval), (yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].stmt_list));
			debug("for_stmt: for(i in x..y) { stmt_list }");
		}
#line 2497 "NoctLang/build/parser.tab.c"
    break;

  case 58: /* for_stmt: TOKEN_FOR TOKEN_LPAR TOKEN_SYMBOL TOKEN_IN expr TOKEN_DOTDOT expr TOKEN_RPAR stmt  */
#line 500 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_for_range_stmt_single((yylsp[-8]).first_line + 1, (yyvsp[-6].sval), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].stmt));
			debug("for_stmt: for(i in x..y) stmt");
		}
#line 2506 "NoctLang/build/parser.tab.c"
    break;

  case 59: /* return_stmt: TOKEN_RETURN expr TOKEN_SEMICOLON  */
#line 506 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_return_stmt((yylsp[-2]).first_line + 1, (yyvsp[-1].expr));
			debug("rerurn_stmt:");
		}
#line 2515 "NoctLang/build/parser.tab.c"
    break;

  case 60: /* return_stmt: TOKEN_RETURN TOKEN_SEMICOLON  */
#line 511 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_return_stmt((yylsp[-1]).first_line + 1, NULL);
			debug("rerurn_stmt NULL:");
		}
#line 2524 "NoctLang/build/parser.tab.c"
    break;

  case 61: /* break_stmt: TOKEN_BREAK TOKEN_SEMICOLON  */
#line 516 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_break_stmt((yylsp[-1]).first_line + 1);
			debug("break_stmt:");
		}
#line 2533 "NoctLang/build/parser.tab.c"
    break;

  case 62: /* continue_stmt: TOKEN_CONTINUE TOKEN_SEMICOLON  */
#line 522 "src/core/parser.y"
                {
			(yyval.stmt) = ast_accept_continue_stmt((yylsp[-1]).first_line + 1);
			debug("continue_stmt");
		}
#line 2542 "NoctLang/build/parser.tab.c"
    break;

  case 63: /* expr: term  */
#line 528 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_term_expr((yyvsp[0].term));
			debug("expr: term");
		}
#line 2551 "NoctLang/build/parser.tab.c"
    break;

  case 64: /* expr: TOKEN_LPAR expr TOKEN_RPAR  */
#line 533 "src/core/parser.y"
                {
			(yyval.expr) = (yyvsp[-1].expr);
			debug("expr: (expr)");
		}
#line 2560 "NoctLang/build/parser.tab.c"
    break;

  case 65: /* expr: expr TOKEN_LARR expr TOKEN_RARR  */
#line 538 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_subscr_expr((yyvsp[-3].expr), (yyvsp[-1].expr));
			debug("expr: array[subscript]");
		}
#line 2569 "NoctLang/build/parser.tab.c"
    break;

  case 66: /* expr: expr TOKEN_OR expr  */
#line 543 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_or_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr or expr");
		}
#line 2578 "NoctLang/build/parser.tab.c"
    break;

  case 67: /* expr: expr TOKEN_AND expr  */
#line 548 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_and_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr and expr");
		}
#line 2587 "NoctLang/build/parser.tab.c"
    break;

  case 68: /* expr: expr TOKEN_XOR expr  */
#line 553 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_xor_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr xor expr");
		}
#line 2596 "NoctLang/build/parser.tab.c"
    break;

  case 69: /* expr: expr TOKEN_OROR expr  */
#line 558 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_or_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr or expr");
		}
#line 2605 "NoctLang/build/parser.tab.c"
    break;

  case 70: /* expr: expr TOKEN_ANDAND expr  */
#line 563 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_and_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr and expr");
		}
#line 2614 "NoctLang/build/parser.tab.c"
    break;

  case 71: /* expr: expr TOKEN_LT expr  */
#line 568 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_lt_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr lt expr");
		}
#line 2623 "NoctLang/build/parser.tab.c"
    break;

  case 72: /* expr: expr TOKEN_LTE expr  */
#line 573 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_lte_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr lte expr");
		}
#line 2632 "NoctLang/build/parser.tab.c"
    break;

  case 73: /* expr: expr TOKEN_GT expr  */
#line 578 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_gt_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr gt expr");
		}
#line 2641 "NoctLang/build/parser.tab.c"
    break;

  case 74: /* expr: expr TOKEN_GTE expr  */
#line 583 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_gte_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr gte expr");
		}
#line 2650 "NoctLang/build/parser.tab.c"
    break;

  case 75: /* expr: expr TOKEN_EQ expr  */
#line 588 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_eq_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr eq expr");
		}
#line 2659 "NoctLang/build/parser.tab.c"
    break;

  case 76: /* expr: expr TOKEN_NEQ expr  */
#line 593 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_neq_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr neq expr");
		}
#line 2668 "NoctLang/build/parser.tab.c"
    break;

  case 77: /* expr: expr TOKEN_PLUS expr  */
#line 598 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_plus_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr plus expr");
		}
#line 2677 "NoctLang/build/parser.tab.c"
    break;

  case 78: /* expr: expr TOKEN_MINUS expr  */
#line 603 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_minus_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr sub expr");
		}
#line 2686 "NoctLang/build/parser.tab.c"
    break;

  case 79: /* expr: expr TOKEN_MUL expr  */
#line 608 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_mul_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr mul expr");
		}
#line 2695 "NoctLang/build/parser.tab.c"
    break;

  case 80: /* expr: expr TOKEN_DIV expr  */
#line 613 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_div_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr div expr");
		}
#line 2704 "NoctLang/build/parser.tab.c"
    break;

  case 81: /* expr: expr TOKEN_MOD expr  */
#line 618 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_mod_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr div expr");
		}
#line 2713 "NoctLang/build/parser.tab.c"
    break;

  case 82: /* expr: expr TOKEN_SHL expr  */
#line 623 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_shl_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr shl expr");
		}
#line 2722 "NoctLang/build/parser.tab.c"
    break;

  case 83: /* expr: expr TOKEN_SHR expr  */
#line 628 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_shr_expr((yyvsp[-2].expr), (yyvsp[0].expr));
			debug("expr: expr shr expr");
		}
#line 2731 "NoctLang/build/parser.tab.c"
    break;

  case 84: /* expr: TOKEN_MINUS expr  */
#line 633 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_neg_expr((yyvsp[0].expr));
			debug("expr: neg expr");
		}
#line 2740 "NoctLang/build/parser.tab.c"
    break;

  case 85: /* expr: TOKEN_NOT expr  */
#line 638 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_not_expr((yyvsp[0].expr));
			debug("expr: not expr");
		}
#line 2749 "NoctLang/build/parser.tab.c"
    break;

  case 86: /* expr: expr TOKEN_DOT TOKEN_SYMBOL  */
#line 643 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_dot_expr((yyvsp[-2].expr), (yyvsp[0].sval));
			debug("expr: expr.symbol");
		}
#line 2758 "NoctLang/build/parser.tab.c"
    break;

  case 87: /* expr: call_expr  */
#line 648 "src/core/parser.y"
                {
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 2766 "NoctLang/build/parser.tab.c"
    break;

  case 88: /* expr: thiscall_expr  */
#line 652 "src/core/parser.y"
                {
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 2774 "NoctLang/build/parser.tab.c"
    break;

  case 89: /* expr: TOKEN_LARR arg_list TOKEN_RARR  */
#line 656 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_array_expr((yyvsp[-1].arg_list));
			debug("expr: array");
		}
#line 2783 "NoctLang/build/parser.tab.c"
    break;

  case 90: /* expr: TOKEN_LBLK kv_list TOKEN_RBLK  */
#line 661 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_dict_expr((yyvsp[-1].kv_list));
			debug("expr: dict");
		}
#line 2792 "NoctLang/build/parser.tab.c"
    break;

  case 91: /* expr: TOKEN_CLASS TOKEN_LBLK kv_list TOKEN_RBLK  */
#line 666 "src/core/parser.y"
                {
			/* class is equal to dict. */
			(yyval.expr) = ast_accept_dict_expr((yyvsp[-1].kv_list));
			debug("expr: dict");
		}
#line 2802 "NoctLang/build/parser.tab.c"
    break;

  case 92: /* expr: TOKEN_CLASS TOKEN_LBLK TOKEN_RBLK  */
#line 672 "src/core/parser.y"
                {
			/* class is equal to dict. */
			(yyval.expr) = ast_accept_dict_expr(NULL);
			debug("expr: dict");
		}
#line 2812 "NoctLang/build/parser.tab.c"
    break;

  case 93: /* expr: lambda_expr  */
#line 678 "src/core/parser.y"
                {
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 2820 "NoctLang/build/parser.tab.c"
    break;

  case 94: /* expr: TOKEN_NEW TOKEN_SYMBOL TOKEN_LBLK kv_list TOKEN_RBLK  */
#line 682 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_new_expr((yyvsp[-3].sval), (yyvsp[-1].kv_list));
			debug("expr: new");
		}
#line 2829 "NoctLang/build/parser.tab.c"
    break;

  case 95: /* expr: TOKEN_NEW TOKEN_SYMBOL TOKEN_LBLK TOKEN_RBLK  */
#line 687 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_new_expr((yyvsp[-2].sval), NULL);
			debug("expr: new");
		}
#line 2838 "NoctLang/build/parser.tab.c"
    break;

  case 96: /* call_expr: expr TOKEN_LPAR arg_list TOKEN_RPAR  */
#line 693 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_call_expr((yyvsp[-3].expr), (yyvsp[-1].arg_list));
			debug("expr: call(param_list)");
		}
#line 2847 "NoctLang/build/parser.tab.c"
    break;

  case 97: /* call_expr: expr TOKEN_LPAR TOKEN_RPAR  */
#line 698 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_call_expr((yyvsp[-2].expr), NULL);
			debug("expr: call()");
		}
#line 2856 "NoctLang/build/parser.tab.c"
    break;

  case 98: /* thiscall_expr: expr TOKEN_ARROW TOKEN_SYMBOL TOKEN_LPAR arg_list TOKEN_RPAR  */
#line 704 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_thiscall_expr((yyvsp[-5].expr), (yyvsp[-3].sval), (yyvsp[-1].arg_list));
			debug("expr: thiscall(param_list)");
		}
#line 2865 "NoctLang/build/parser.tab.c"
    break;

  case 99: /* thiscall_expr: expr TOKEN_ARROW TOKEN_SYMBOL TOKEN_LPAR TOKEN_RPAR  */
#line 709 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_thiscall_expr((yyvsp[-4].expr), (yyvsp[-2].sval), NULL);
			debug("expr: thiscall(param_list)");
		}
#line 2874 "NoctLang/build/parser.tab.c"
    break;

  case 100: /* lambda_expr: TOKEN_LPAR param_list TOKEN_RPAR_DARROW_LBLK stmt_list TOKEN_RBLK  */
#line 715 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_func_expr((yyvsp[-3].param_list), (yyvsp[-1].stmt_list));
			debug("expr: func param_list stmt_list");
		}
#line 2883 "NoctLang/build/parser.tab.c"
    break;

  case 101: /* lambda_expr: TOKEN_LPAR TOKEN_RPAR_DARROW_LBLK stmt_list TOKEN_RBLK  */
#line 720 "src/core/parser.y"
                {
			(yyval.expr) = ast_accept_func_expr(NULL, (yyvsp[-1].stmt_list));
			debug("expr: func stmt_list");
		}
#line 2892 "NoctLang/build/parser.tab.c"
    break;

  case 102: /* arg_list: expr  */
#line 726 "src/core/parser.y"
                {
			(yyval.arg_list) = ast_accept_arg_list(NULL, (yyvsp[0].expr));
			debug("arg_list: expr");
		}
#line 2901 "NoctLang/build/parser.tab.c"
    break;

  case 103: /* arg_list: arg_list TOKEN_COMMA expr  */
#line 731 "src/core/parser.y"
                {
			(yyval.arg_list) = ast_accept_arg_list((yyvsp[-2].arg_list), (yyvsp[0].expr));
			debug("arg_list: arg_list arg");
		}
#line 2910 "NoctLang/build/parser.tab.c"
    break;

  case 104: /* kv_list: kv  */
#line 737 "src/core/parser.y"
                {
			(yyval.kv_list) = ast_accept_kv_list(NULL, (yyvsp[0].kv));
			debug("kv_list: kv");
		}
#line 2919 "NoctLang/build/parser.tab.c"
    break;

  case 105: /* kv_list: kv_list TOKEN_COMMA kv  */
#line 742 "src/core/parser.y"
                {
			(yyval.kv_list) = ast_accept_kv_list((yyvsp[-2].kv_list), (yyvsp[0].kv));
			debug("kv_list: kv_list kv");
		}
#line 2928 "NoctLang/build/parser.tab.c"
    break;

  case 106: /* kv: TOKEN_STR TOKEN_COLON expr  */
#line 748 "src/core/parser.y"
                {
			(yyval.kv) = ast_accept_kv((yyvsp[-2].sval), (yyvsp[0].expr));
			debug("kv");
		}
#line 2937 "NoctLang/build/parser.tab.c"
    break;

  case 107: /* kv: TOKEN_SYMBOL TOKEN_COLON expr  */
#line 753 "src/core/parser.y"
                {
			(yyval.kv) = ast_accept_kv((yyvsp[-2].sval), (yyvsp[0].expr));
			debug("kv");
		}
#line 2946 "NoctLang/build/parser.tab.c"
    break;

  case 108: /* term: TOKEN_INT  */
#line 759 "src/core/parser.y"
                {
			(yyval.term) = ast_accept_int_term((yyvsp[0].ival));
			debug("term: int");
		}
#line 2955 "NoctLang/build/parser.tab.c"
    break;

  case 109: /* term: TOKEN_FLOAT  */
#line 764 "src/core/parser.y"
                {
			(yyval.term) = ast_accept_float_term((float)(yyvsp[0].fval));
			debug("term: float");
		}
#line 2964 "NoctLang/build/parser.tab.c"
    break;

  case 110: /* term: TOKEN_STR  */
#line 769 "src/core/parser.y"
                {
			(yyval.term) = ast_accept_str_term((yyvsp[0].sval));
			debug("term: string");
		}
#line 2973 "NoctLang/build/parser.tab.c"
    break;

  case 111: /* term: TOKEN_SYMBOL  */
#line 774 "src/core/parser.y"
                {
			(yyval.term) = ast_accept_symbol_term((yyvsp[0].sval));
			debug("term: symbol");
		}
#line 2982 "NoctLang/build/parser.tab.c"
    break;

  case 112: /* term: TOKEN_LARR TOKEN_RARR  */
#line 779 "src/core/parser.y"
                {
			(yyval.term) = ast_accept_empty_array_term();
			debug("term: empty array symbol");
		}
#line 2991 "NoctLang/build/parser.tab.c"
    break;

  case 113: /* term: TOKEN_LBLK TOKEN_RBLK  */
#line 784 "src/core/parser.y"
                {
			(yyval.term) = ast_accept_empty_dict_term();
			debug("term: empty dict symbol");
		}
#line 3000 "NoctLang/build/parser.tab.c"
    break;


#line 3004 "NoctLang/build/parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (scanner, YY_("syntax error"));
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, scanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 789 "src/core/parser.y"


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
