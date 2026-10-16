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
     STRING = 261,
     HEX_COLOR = 262,
     MEASUREMENT = 263,
     PERCENTAGE = 264,
     ANGLE_DEG = 265,
     DIM_2D = 266,
     DIM_3D = 267,
     DEGREE = 268,
     IN_SYM = 269,
     DOTDOT = 270,
     DOLLAR_DOLLAR = 271,
     EQ = 272,
     NEQ = 273,
     LEQ = 274,
     GEQ = 275,
     ANDAND = 276,
     OROR = 277,
     NODE_KW = 278,
     EDGE_KW = 279,
     CONNECTOR_KW = 280,
     GROUP_KW = 281,
     LAYER_KW = 282,
     ANNOTATION_KW = 283,
     DIMENSION_KW = 284,
     DIM_KW = 285,
     TIME_KW = 286,
     DEFAULTS_KW = 287,
     AT_KW = 288,
     FROM_KW = 289,
     TO_KW = 290,
     VIA_KW = 291,
     STYLE_KW = 292,
     CONTENT_KW = 293,
     TEXT_KW = 294,
     LABEL_KW = 295,
     RICH_KW = 296,
     EXPR_KW = 297,
     ANIMATE_KW = 298,
     IN_KW = 299,
     BR_KW = 300,
     TRUE_KW = 301,
     FALSE_KW = 302,
     YES_KW = 303,
     NO_KW = 304,
     LNOT = 305,
     UMINUS = 306,
     POSTFIX_BANG = 307
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 68 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\flowchart.y"

    long    ival;
    double  fval;
    char   *sval;


/* Line 2058 of yacc.c  */
#line 116 "flowchart_parser.hpp"
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
