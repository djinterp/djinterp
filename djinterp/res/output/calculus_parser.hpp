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

#ifndef YY_YY_CALCULUS_PARSER_HPP_INCLUDED
# define YY_YY_CALCULUS_PARSER_HPP_INCLUDED
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
     DIFFERENTIAL = 261,
     EQ = 262,
     NEQ = 263,
     LEQ = 264,
     GEQ = 265,
     ANDAND = 266,
     OROR = 267,
     IFF = 268,
     IMPLIES = 269,
     ARROW = 270,
     CARET_INV = 271,
     CARET_T = 272,
     CARET_C = 273,
     PRIME = 274,
     BACKSLASH = 275,
     HASH = 276,
     ABS_OPEN = 277,
     ABS_CLOSE = 278,
     NORM_OPEN = 279,
     NORM_CLOSE = 280,
     PI = 281,
     EULER = 282,
     INF = 283,
     PHI = 284,
     TAU = 285,
     SIN = 286,
     COS = 287,
     TAN = 288,
     SEC = 289,
     CSC = 290,
     COT = 291,
     ASIN = 292,
     ACOS = 293,
     ATAN = 294,
     ATAN2 = 295,
     ASEC = 296,
     ACSC = 297,
     ACOT = 298,
     SINH = 299,
     COSH = 300,
     TANH = 301,
     SECH = 302,
     CSCH = 303,
     COTH = 304,
     ASINH = 305,
     ACOSH = 306,
     ATANH = 307,
     ASECH = 308,
     ACSCH = 309,
     ACOTH = 310,
     LOG = 311,
     LN = 312,
     LOG10 = 313,
     LOG2 = 314,
     EXP = 315,
     SQRT = 316,
     CBRT = 317,
     ABS = 318,
     FLOOR = 319,
     CEIL = 320,
     ROUND = 321,
     MIN = 322,
     MAX = 323,
     GCD = 324,
     LCM = 325,
     FACTORIAL = 326,
     GAMMA = 327,
     ERF = 328,
     ERFC = 329,
     SIGN = 330,
     POLAR = 331,
     RECT = 332,
     THETA = 333,
     CYL = 334,
     RHO = 335,
     SPHER = 336,
     RE_FN = 337,
     IM_FN = 338,
     ARG_FN = 339,
     CONJ_FN = 340,
     MAG_FN = 341,
     INTEGRAL = 342,
     INTEGRAL2 = 343,
     INTEGRAL3 = 344,
     PARTIAL = 345,
     SUMMATION = 346,
     PRODUCT = 347,
     NABLA = 348,
     DELTA_CAP = 349,
     DELTA_LO = 350,
     DIFF_FN = 351,
     PARTIAL_KW = 352,
     GRAD_KW = 353,
     INTEGRATE_FN = 354,
     LIM = 355,
     LIMIT_FN = 356,
     SUM_FN = 357,
     PRODUCT_FN = 358,
     DIV_KW = 359,
     CURL_KW = 360,
     D_KW = 361,
     D_CAP = 362,
     TRANSPOSE_FN = 363,
     INV_FN = 364,
     DET_FN = 365,
     TRACE_FN = 366,
     TR_FN = 367,
     RANK_FN = 368,
     EIG_FN = 369,
     EIGVEC_FN = 370,
     DOT_FN = 371,
     CROSS_FN = 372,
     NORM_FN = 373,
     NORMALIZE_FN = 374,
     ZEROS_FN = 375,
     ONES_FN = 376,
     EYE_FN = 377,
     DIAG_FN = 378,
     RESHAPE_FN = 379,
     SOLVE_FN = 380,
     LU_FN = 381,
     QR_FN = 382,
     SVD_FN = 383,
     CHOLESKY_FN = 384,
     FORALL = 385,
     EXISTS = 386,
     OR_SYM = 387,
     AND_SYM = 388,
     NOT_SYM = 389,
     IFF_SYM = 390,
     TOP_SYM = 391,
     BOT_SYM = 392,
     FORALL_KW = 393,
     EXISTS_KW = 394,
     AND_KW = 395,
     OR_KW = 396,
     NOT_KW = 397,
     IMPLIES_KW = 398,
     IFF_KW = 399,
     TRUE_KW = 400,
     FALSE_KW = 401,
     IN_SYM = 402,
     NOTIN_SYM = 403,
     UNION_SYM = 404,
     INTER_SYM = 405,
     SUBSETEQ = 406,
     SUPSETEQ = 407,
     SUBSET = 408,
     SUPSET = 409,
     TIMES_SYM = 410,
     CDOT_SYM = 411,
     EMPTY_SYM = 412,
     POWERSET = 413,
     EQUIV_SYM = 414,
     NAT_SET = 415,
     INT_SET = 416,
     RAT_SET = 417,
     REAL_SET = 418,
     CPX_SET = 419,
     UNION_KW = 420,
     INTER_KW = 421,
     DIFF_KW = 422,
     COMP_KW = 423,
     CART_KW = 424,
     PWRSET_KW = 425,
     SUBSET_KW = 426,
     SUPSET_KW = 427,
     PSUBSET_KW = 428,
     EMPTY_KW = 429,
     IN_KW = 430,
     CARD_KW = 431,
     SUP_KW = 432,
     LFLOOR = 433,
     RFLOOR = 434,
     LCEIL = 435,
     RCEIL = 436,
     CHOOSE_KW = 437,
     BINOM_KW = 438,
     PERM_KW = 439,
     COMB_KW = 440,
     MOD_KW = 441,
     TOTIENT_KW = 442,
     MOBIUS_KW = 443,
     ISPRIME_KW = 444,
     NEXTPRIME_KW = 445,
     PRIMES_KW = 446,
     FACTORS_KW = 447,
     SEQUENCE_KW = 448,
     DEGREE_KW = 449,
     NEIGHBORS_KW = 450,
     ADJ_KW = 451,
     CONN_KW = 452,
     CYCLE_KW = 453,
     PATH_KW = 454,
     GRAPH_KW = 455,
     UNOT = 456,
     UPLUS = 457,
     UMINUS = 458,
     POSTFIX_BANG = 459
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 34 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\calculus.y"

    long    ival;
    double  fval;
    char   *sval;


/* Line 2058 of yacc.c  */
#line 268 "calculus_parser.hpp"
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

#endif /* !YY_YY_CALCULUS_PARSER_HPP_INCLUDED  */
