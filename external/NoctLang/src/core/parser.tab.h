/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_AST_YY_NOCTLANG_PARSER_TAB_H_INCLUDED
# define YY_AST_YY_NOCTLANG_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ast_yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TOKEN_SYMBOL = 258,            /* TOKEN_SYMBOL  */
    TOKEN_STR = 259,               /* TOKEN_STR  */
    TOKEN_INT = 260,               /* TOKEN_INT  */
    TOKEN_FLOAT = 261,             /* TOKEN_FLOAT  */
    TOKEN_FUNC = 262,              /* TOKEN_FUNC  */
    TOKEN_CLASS = 263,             /* TOKEN_CLASS  */
    TOKEN_NEW = 264,               /* TOKEN_NEW  */
    TOKEN_LAMBDA = 265,            /* TOKEN_LAMBDA  */
    TOKEN_LARR = 266,              /* TOKEN_LARR  */
    TOKEN_RARR = 267,              /* TOKEN_RARR  */
    TOKEN_PLUS = 268,              /* TOKEN_PLUS  */
    TOKEN_MINUS = 269,             /* TOKEN_MINUS  */
    TOKEN_MUL = 270,               /* TOKEN_MUL  */
    TOKEN_DIV = 271,               /* TOKEN_DIV  */
    TOKEN_MOD = 272,               /* TOKEN_MOD  */
    TOKEN_SHL = 273,               /* TOKEN_SHL  */
    TOKEN_SHR = 274,               /* TOKEN_SHR  */
    TOKEN_ASSIGN = 275,            /* TOKEN_ASSIGN  */
    TOKEN_PLUSASSIGN = 276,        /* TOKEN_PLUSASSIGN  */
    TOKEN_MINUSASSIGN = 277,       /* TOKEN_MINUSASSIGN  */
    TOKEN_MULASSIGN = 278,         /* TOKEN_MULASSIGN  */
    TOKEN_DIVASSIGN = 279,         /* TOKEN_DIVASSIGN  */
    TOKEN_MODASSIGN = 280,         /* TOKEN_MODASSIGN  */
    TOKEN_ANDASSIGN = 281,         /* TOKEN_ANDASSIGN  */
    TOKEN_ORASSIGN = 282,          /* TOKEN_ORASSIGN  */
    TOKEN_SHLASSIGN = 283,         /* TOKEN_SHLASSIGN  */
    TOKEN_SHRASSIGN = 284,         /* TOKEN_SHRASSIGN  */
    TOKEN_PLUSPLUS = 285,          /* TOKEN_PLUSPLUS  */
    TOKEN_MINUSMINUS = 286,        /* TOKEN_MINUSMINUS  */
    TOKEN_ANDAND = 287,            /* TOKEN_ANDAND  */
    TOKEN_OROR = 288,              /* TOKEN_OROR  */
    TOKEN_LPAR = 289,              /* TOKEN_LPAR  */
    TOKEN_RPAR = 290,              /* TOKEN_RPAR  */
    TOKEN_RPAR_LBLK = 291,         /* TOKEN_RPAR_LBLK  */
    TOKEN_LBLK = 292,              /* TOKEN_LBLK  */
    TOKEN_LBLK_BLK = 293,          /* TOKEN_LBLK_BLK  */
    TOKEN_RBLK = 294,              /* TOKEN_RBLK  */
    TOKEN_SEMICOLON = 295,         /* TOKEN_SEMICOLON  */
    TOKEN_COLON = 296,             /* TOKEN_COLON  */
    TOKEN_DOT = 297,               /* TOKEN_DOT  */
    TOKEN_COMMA = 298,             /* TOKEN_COMMA  */
    TOKEN_IF = 299,                /* TOKEN_IF  */
    TOKEN_ELSE = 300,              /* TOKEN_ELSE  */
    TOKEN_ELSE_LBLK = 301,         /* TOKEN_ELSE_LBLK  */
    TOKEN_ELSEIF = 302,            /* TOKEN_ELSEIF  */
    TOKEN_WHILE = 303,             /* TOKEN_WHILE  */
    TOKEN_FOR = 304,               /* TOKEN_FOR  */
    TOKEN_IN = 305,                /* TOKEN_IN  */
    TOKEN_DOTDOT = 306,            /* TOKEN_DOTDOT  */
    TOKEN_GT = 307,                /* TOKEN_GT  */
    TOKEN_GTE = 308,               /* TOKEN_GTE  */
    TOKEN_LT = 309,                /* TOKEN_LT  */
    TOKEN_LTE = 310,               /* TOKEN_LTE  */
    TOKEN_EQ = 311,                /* TOKEN_EQ  */
    TOKEN_NEQ = 312,               /* TOKEN_NEQ  */
    TOKEN_RETURN = 313,            /* TOKEN_RETURN  */
    TOKEN_BREAK = 314,             /* TOKEN_BREAK  */
    TOKEN_CONTINUE = 315,          /* TOKEN_CONTINUE  */
    TOKEN_ARROW = 316,             /* TOKEN_ARROW  */
    TOKEN_RPAR_DARROW_LBLK = 317,  /* TOKEN_RPAR_DARROW_LBLK  */
    TOKEN_AND = 318,               /* TOKEN_AND  */
    TOKEN_OR = 319,                /* TOKEN_OR  */
    TOKEN_XOR = 320,               /* TOKEN_XOR  */
    TOKEN_VAR = 321,               /* TOKEN_VAR  */
    UNARYMINUS = 322,              /* UNARYMINUS  */
    TOKEN_NOT = 323,               /* TOKEN_NOT  */
    CALL = 324                     /* CALL  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 117 "src/core/parser.y"

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

#line 150 "src/core/parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE ast_yylval;
extern YYLTYPE ast_yylloc;

int ast_yyparse (void *scanner);

/* "%code provides" blocks.  */
#line 113 "src/core/parser.y"

#define YY_DECL int ast_yylex(void *yyscanner)

#line 183 "src/core/parser.tab.h"

#endif /* !YY_AST_YY_NOCTLANG_PARSER_TAB_H_INCLUDED  */
