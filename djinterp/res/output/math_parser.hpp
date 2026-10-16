/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_YY_FLOWCHART_PARSER_HPP_INCLUDED
# define YY_YY_FLOWCHART_PARSER_HPP_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INTEGER = 258,
     NUMBER = 259,
     IDENT = 260,
     EQ = 261,
     NEQ = 262,
     LEQ = 263,
     GEQ = 264,
     ANDAND = 265,
     OROR = 266,
     ABS_OPEN = 267,
     ABS_CLOSE = 268,
     PI = 269,
     EULER = 270,
     INF = 271,
     PHI = 272,
     TAU = 273,
     SIN = 274,
     COS = 275,
     TAN = 276,
     SEC = 277,
     CSC = 278,
     COT = 279,
     ASIN = 280,
     ACOS = 281,
     ATAN = 282,
     ATAN2 = 283,
     ASEC = 284,
     ACSC = 285,
     ACOT = 286,
     SINH = 287,
     COSH = 288,
     TANH = 289,
     SECH = 290,
     CSCH = 291,
     COTH = 292,
     ASINH = 293,
     ACOSH = 294,
     ATANH = 295,
     ASECH = 296,
     ACSCH = 297,
     ACOTH = 298,
     LOG = 299,
     LN = 300,
     LOG10 = 301,
     LOG2 = 302,
     EXP = 303,
     SQRT = 304,
     CBRT = 305,
     ABS = 306,
     FLOOR = 307,
     CEIL = 308,
     ROUND = 309,
     MIN = 310,
     MAX = 311,
     GCD = 312,
     LCM = 313,
     FACTORIAL = 314,
     GAMMA = 315,
     ERF = 316,
     ERFC = 317,
     SIGN = 318,
     POLAR = 319,
     RECT = 320,
     THETA = 321,
     CYL = 322,
     RHO = 323,
     SPHER = 324,
     RE_FN = 325,
     IM_FN = 326,
     ARG_FN = 327,
     CONJ_FN = 328,
     MAG_FN = 329,
     UNOT = 330,
     UPLUS = 331,
     UMINUS = 332,
     POSTFIX = 333
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 17 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\math.y"

    long    ival;
    double  fval;
    char   *sval;


/* Line 2058 of yacc.c  */
#line 142 "flowchart_parser.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_FLOWCHART_PARSER_HPP_INCLUDED  */
