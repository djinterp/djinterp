/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 30 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\flowchart.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex(void);
extern int yylineno;
extern char *yytext;
void yyerror(const char *s);

/* Line 371 of yacc.c  */
#line 79 "flowchart_parser.cpp"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "flowchart_parser.hpp".  */
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
/* Line 387 of yacc.c  */
#line 68 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\flowchart.y"

    long    ival;
    double  fval;
    char   *sval;


/* Line 387 of yacc.c  */
#line 181 "flowchart_parser.cpp"
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

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 209 "flowchart_parser.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   489

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  74
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  80
/* YYNRULES -- Number of rules.  */
#define YYNRULES  216
/* YYNRULES -- Number of states.  */
#define YYNSTATES  418

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   307

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    73,     2,     2,    72,    57,     2,     2,
      68,    69,    55,    53,    64,    54,    70,    56,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,    71,
      51,    50,    52,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    65,    58,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    66,     2,    67,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    59,    60,    61
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    11,    14,    18,    22,    23,
      26,    28,    30,    32,    34,    36,    38,    42,    46,    50,
      54,    58,    62,    68,    73,    79,    80,    83,    85,    90,
      95,    97,    99,   101,   111,   125,   129,   133,   141,   145,
     147,   151,   157,   165,   173,   177,   181,   187,   190,   194,
     200,   208,   212,   218,   224,   230,   238,   243,   247,   259,
     261,   263,   265,   267,   270,   273,   276,   279,   280,   283,
     285,   287,   292,   297,   304,   306,   310,   312,   316,   318,
     320,   321,   324,   326,   328,   331,   333,   336,   339,   343,
     345,   349,   352,   355,   358,   361,   366,   371,   377,   379,
     381,   386,   390,   391,   394,   400,   408,   415,   424,   430,
     437,   443,   450,   452,   455,   460,   461,   464,   467,   471,
     475,   479,   480,   483,   489,   495,   501,   507,   509,   513,
     515,   518,   520,   522,   524,   526,   528,   530,   532,   534,
     539,   543,   547,   549,   551,   553,   555,   557,   561,   567,
     575,   576,   578,   579,   581,   583,   587,   589,   594,   598,
     603,   605,   608,   610,   615,   619,   620,   623,   625,   630,
     632,   636,   640,   642,   644,   646,   648,   650,   652,   654,
     658,   660,   662,   666,   668,   672,   674,   678,   680,   682,
     684,   686,   688,   690,   692,   696,   700,   702,   706,   710,
     714,   716,   720,   722,   725,   728,   731,   733,   736,   738,
     740,   742,   744,   746,   748,   750,   755
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      75,     0,    -1,    76,    77,    -1,    79,    -1,    79,    80,
      -1,    79,    82,    -1,    79,    80,    82,    -1,    79,    82,
      80,    -1,    -1,    77,    78,    -1,    83,    -1,   101,    -1,
     112,    -1,   117,    -1,   119,    -1,   118,    -1,    29,    62,
      11,    -1,    29,    62,    12,    -1,    30,    62,     3,    -1,
      31,    62,     5,    -1,    31,    62,    81,    -1,   141,    15,
     141,    -1,    63,   141,    64,   141,    65,    -1,    32,    66,
     122,    67,    -1,    23,     5,    84,    95,    99,    -1,    -1,
      62,    85,    -1,     5,    -1,     5,    68,     3,    69,    -1,
       5,    66,    86,    67,    -1,    87,    -1,    88,    -1,    89,
      -1,     5,    62,   141,    64,     5,    62,   141,    64,    93,
      -1,     5,    62,   141,    64,     5,    62,   141,    64,     5,
      62,   141,    64,    93,    -1,     5,    62,   141,    -1,     5,
      62,    90,    -1,     5,    62,    90,    64,     5,    62,   140,
      -1,    63,    91,    65,    -1,    92,    -1,    91,    64,    92,
      -1,    68,   141,    64,   141,    69,    -1,    68,   141,    64,
     141,    64,   141,    69,    -1,     5,    14,    63,   141,    64,
     141,    65,    -1,     5,    44,    94,    -1,   141,    15,   141,
      -1,    63,   141,    64,   141,    65,    -1,    33,    96,    -1,
      33,    96,    97,    -1,    68,   141,    64,   141,    69,    -1,
      68,   141,    64,   141,    64,   141,    69,    -1,     5,    70,
       5,    -1,     5,    70,     5,    53,   129,    -1,     5,    70,
       5,    54,   129,    -1,    63,     3,    64,     3,    65,    -1,
      63,     3,    64,     3,    64,     3,    65,    -1,     5,    68,
     133,    69,    -1,     5,    62,    98,    -1,     5,    62,    98,
      64,     5,    62,    98,    64,     5,    62,    98,    -1,     8,
      -1,    10,    -1,     4,    -1,     3,    -1,     4,    13,    -1,
       3,    13,    -1,     4,     5,    -1,     3,     5,    -1,    -1,
      99,   100,    -1,   121,    -1,   135,    -1,    24,     5,   102,
     106,    -1,    34,   103,    35,   103,    -1,    34,   103,    35,
     103,    36,   104,    -1,     5,    -1,     5,    70,     5,    -1,
     105,    -1,   104,    64,   105,    -1,    96,    -1,     5,    -1,
      -1,   106,   107,    -1,   121,    -1,   108,    -1,    40,   109,
      -1,   110,    -1,   110,   111,    -1,   110,   121,    -1,   110,
     111,   121,    -1,     6,    -1,    42,    62,   141,    -1,    33,
       5,    -1,    33,     4,    -1,    33,     3,    -1,    33,    96,
      -1,    25,     5,   113,   116,    -1,    34,   114,    35,   114,
      -1,    34,   114,    35,   114,   115,    -1,   103,    -1,    92,
      -1,     5,    68,   133,    69,    -1,     5,    62,   128,    -1,
      -1,   116,   121,    -1,    26,     5,    66,    77,    67,    -1,
      26,     5,    33,    96,    66,    77,    67,    -1,    26,     5,
     121,    66,    77,    67,    -1,    26,     5,    33,    96,   121,
      66,    77,    67,    -1,    27,     5,    66,    77,    67,    -1,
      27,     5,   121,    66,    77,    67,    -1,    28,     5,     5,
     120,   135,    -1,    28,     5,     5,   120,   121,   135,    -1,
      96,    -1,    33,    96,    -1,    37,    66,   122,    67,    -1,
      -1,   122,   123,    -1,   122,    71,    -1,     5,    62,   127,
      -1,    43,    62,   124,    -1,    66,   125,    67,    -1,    -1,
     125,   126,    -1,    33,   128,    66,   122,    67,    -1,     4,
      62,    66,   122,    67,    -1,     3,    62,    66,   122,    67,
      -1,     8,    62,    66,   122,    67,    -1,   128,    -1,   127,
      64,   128,    -1,   129,    -1,   128,   129,    -1,     4,    -1,
       3,    -1,     8,    -1,     9,    -1,    10,    -1,     7,    -1,
       6,    -1,     5,    -1,     5,    68,   132,    69,    -1,    68,
     130,    69,    -1,    63,   131,    65,    -1,    46,    -1,    47,
      -1,    48,    -1,    49,    -1,   141,    -1,   141,    64,   141,
      -1,   141,    64,   141,    64,   141,    -1,   141,    64,   141,
      64,   141,    64,   141,    -1,    -1,   127,    -1,    -1,   133,
      -1,   134,    -1,   133,    64,   134,    -1,   128,    -1,    38,
      66,   136,    67,    -1,    39,    62,     6,    -1,    39,    62,
       6,   121,    -1,   137,    -1,   137,   121,    -1,     6,    -1,
      41,    66,   138,    67,    -1,    42,    62,   141,    -1,    -1,
     138,   139,    -1,     6,    -1,     6,    63,   122,    65,    -1,
      45,    -1,    72,   141,    72,    -1,    16,   141,    16,    -1,
      46,    -1,    47,    -1,    48,    -1,    49,    -1,     3,    -1,
     142,    -1,   143,    -1,     5,    50,   141,    -1,   144,    -1,
     145,    -1,   144,    22,   145,    -1,   146,    -1,   145,    21,
     146,    -1,   148,    -1,   146,   147,   148,    -1,    17,    -1,
      18,    -1,    19,    -1,    20,    -1,    51,    -1,    52,    -1,
     149,    -1,   148,    53,   149,    -1,   148,    54,   149,    -1,
     150,    -1,   149,    55,   150,    -1,   149,    56,   150,    -1,
     149,    57,   150,    -1,   151,    -1,   151,    58,   150,    -1,
     152,    -1,    54,   151,    -1,    53,   151,    -1,    73,   151,
      -1,   153,    -1,   152,    73,    -1,     4,    -1,     3,    -1,
       8,    -1,     9,    -1,    10,    -1,     6,    -1,     5,    -1,
       5,    68,   132,    69,    -1,    68,   141,    69,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   113,   113,   117,   118,   119,   120,   121,   124,   126,
     130,   131,   132,   133,   134,   135,   143,   144,   145,   149,
     150,   154,   155,   159,   167,   170,   172,   176,   177,   178,
     182,   183,   184,   188,   189,   193,   199,   200,   204,   208,
     209,   213,   214,   218,   219,   223,   224,   228,   229,   233,
     234,   235,   236,   237,   238,   239,   240,   244,   245,   249,
     250,   251,   252,   253,   254,   255,   256,   259,   261,   265,
     266,   274,   278,   279,   283,   284,   288,   289,   293,   294,
     297,   299,   303,   304,   308,   312,   313,   314,   315,   319,
     320,   324,   325,   326,   327,   335,   339,   340,   344,   345,
     346,   350,   353,   355,   363,   364,   365,   366,   370,   371,
     379,   380,   384,   385,   396,   399,   401,   402,   406,   407,
     412,   415,   417,   421,   422,   423,   424,   430,   431,   435,
     436,   440,   441,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   451,   451,   451,   455,   456,   457,   458,
     461,   463,   466,   468,   472,   473,   477,   485,   486,   487,
     491,   492,   496,   497,   498,   501,   503,   507,   508,   509,
     510,   511,   519,   519,   519,   519,   519,   530,   531,   535,
     539,   543,   544,   548,   549,   553,   554,   558,   558,   558,
     558,   558,   558,   562,   563,   564,   568,   569,   570,   571,
     575,   576,   580,   581,   582,   583,   587,   588,   592,   593,
     594,   595,   596,   597,   598,   599,   600
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "NUMBER", "IDENT", "STRING",
  "HEX_COLOR", "MEASUREMENT", "PERCENTAGE", "ANGLE_DEG", "DIM_2D",
  "DIM_3D", "DEGREE", "IN_SYM", "DOTDOT", "DOLLAR_DOLLAR", "EQ", "NEQ",
  "LEQ", "GEQ", "ANDAND", "OROR", "NODE_KW", "EDGE_KW", "CONNECTOR_KW",
  "GROUP_KW", "LAYER_KW", "ANNOTATION_KW", "DIMENSION_KW", "DIM_KW",
  "TIME_KW", "DEFAULTS_KW", "AT_KW", "FROM_KW", "TO_KW", "VIA_KW",
  "STYLE_KW", "CONTENT_KW", "TEXT_KW", "LABEL_KW", "RICH_KW", "EXPR_KW",
  "ANIMATE_KW", "IN_KW", "BR_KW", "TRUE_KW", "FALSE_KW", "YES_KW", "NO_KW",
  "'='", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'", "LNOT",
  "UMINUS", "POSTFIX_BANG", "':'", "'['", "','", "']'", "'{'", "'}'",
  "'('", "')'", "'.'", "';'", "'$'", "'!'", "$accept",
  "flowchart_document", "flowchart_header", "element_list",
  "flowchart_element", "dimension_spec", "time_spec", "time_range",
  "defaults_spec", "node_definition", "node_type_opt", "shape_spec",
  "shape_definition", "parametric_shape", "implicit_shape",
  "point_list_shape", "point_list", "point_seq", "point",
  "parameter_range", "range_expr", "position_spec", "position",
  "orientation_clause", "angle", "node_decoration_list", "node_decoration",
  "edge_definition", "edge_spec", "node_reference", "waypoint_list",
  "waypoint", "edge_decoration_list", "edge_decoration", "edge_label",
  "label_spec", "label_text", "label_position", "connector_definition",
  "connector_spec", "connector_end", "routing_clause",
  "connector_decoration_list", "group_definition", "layer_definition",
  "annotation_definition", "annotation_position", "style_block",
  "property_list", "property", "animation_block", "keyframe_list",
  "keyframe", "value_list", "value_seq", "value_item", "value_inner",
  "value_list_opt", "arg_list_opt", "arg_list", "arg", "content_block",
  "content_body", "text_content", "rich_text_list", "rich_text_element",
  "boolean", "expression", "assignment", "conditional", "logical_or",
  "logical_and", "comparison", "comp_op", "additive", "multiplicative",
  "exponential", "unary", "postfix", "primary", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
      61,    60,    62,    43,    45,    42,    47,    37,    94,   305,
     306,   307,    58,    91,    44,    93,   123,   125,    40,    41,
      46,    59,    36,    33
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    74,    75,    76,    76,    76,    76,    76,    77,    77,
      78,    78,    78,    78,    78,    78,    79,    79,    79,    80,
      80,    81,    81,    82,    83,    84,    84,    85,    85,    85,
      86,    86,    86,    87,    87,    88,    89,    89,    90,    91,
      91,    92,    92,    93,    93,    94,    94,    95,    95,    96,
      96,    96,    96,    96,    96,    96,    96,    97,    97,    98,
      98,    98,    98,    98,    98,    98,    98,    99,    99,   100,
     100,   101,   102,   102,   103,   103,   104,   104,   105,   105,
     106,   106,   107,   107,   108,   109,   109,   109,   109,   110,
     110,   111,   111,   111,   111,   112,   113,   113,   114,   114,
     114,   115,   116,   116,   117,   117,   117,   117,   118,   118,
     119,   119,   120,   120,   121,   122,   122,   122,   123,   123,
     124,   125,   125,   126,   126,   126,   126,   127,   127,   128,
     128,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   130,   130,   130,   130,
     131,   131,   132,   132,   133,   133,   134,   135,   135,   135,
     136,   136,   137,   137,   137,   138,   138,   139,   139,   139,
     139,   139,   140,   140,   140,   140,   140,   141,   141,   142,
     143,   144,   144,   145,   145,   146,   146,   147,   147,   147,
     147,   147,   147,   148,   148,   148,   149,   149,   149,   149,
     150,   150,   151,   151,   151,   151,   152,   152,   153,   153,
     153,   153,   153,   153,   153,   153,   153
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     2,     2,     3,     3,     0,     2,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     5,     4,     5,     0,     2,     1,     4,     4,
       1,     1,     1,     9,    13,     3,     3,     7,     3,     1,
       3,     5,     7,     7,     3,     3,     5,     2,     3,     5,
       7,     3,     5,     5,     5,     7,     4,     3,    11,     1,
       1,     1,     1,     2,     2,     2,     2,     0,     2,     1,
       1,     4,     4,     6,     1,     3,     1,     3,     1,     1,
       0,     2,     1,     1,     2,     1,     2,     2,     3,     1,
       3,     2,     2,     2,     2,     4,     4,     5,     1,     1,
       4,     3,     0,     2,     5,     7,     6,     8,     5,     6,
       5,     6,     1,     2,     4,     0,     2,     2,     3,     3,
       3,     0,     2,     5,     5,     5,     5,     1,     3,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       3,     3,     1,     1,     1,     1,     1,     3,     5,     7,
       0,     1,     0,     1,     1,     3,     1,     4,     3,     4,
       1,     2,     1,     4,     3,     0,     2,     1,     4,     1,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     3,     1,     3,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     1,     3,     3,     3,
       1,     3,     1,     2,     2,     2,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     4,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     8,     3,     0,     0,     1,     2,
       0,     0,     4,     5,    16,    17,    18,     0,     0,     0,
       0,     0,     0,     9,    10,    11,    12,    13,    15,    14,
       0,   115,     6,     7,    25,     0,     0,     0,     0,     0,
     209,   208,   214,   213,   210,   211,   212,     0,     0,     0,
       0,     0,    20,     0,   177,   178,   180,   181,   183,   185,
     193,   196,   200,   202,   206,     0,     0,     0,     0,    80,
       0,   102,     0,     0,     8,     0,     8,     0,     0,     0,
     152,   214,   204,   203,   214,     0,     0,   205,     0,     0,
       0,   187,   188,   189,   190,   191,   192,     0,     0,     0,
       0,     0,     0,     0,   207,     0,     0,    23,   117,   116,
      27,    26,     0,    67,    74,     0,    71,    74,     0,    99,
      98,     0,    95,     0,     0,     0,     0,   115,     0,     8,
       0,     8,     0,   112,     0,   179,   132,   131,   138,   137,
     136,   133,   134,   135,   142,   143,   144,   145,   150,     0,
     156,   129,     0,   153,   154,     0,   216,    21,   182,   184,
     186,   194,   195,   197,   198,   199,   201,     0,     0,     0,
       0,    47,    24,     0,     0,     0,    81,    83,    82,     0,
       0,     0,   103,     0,     0,     0,     0,     8,     0,     0,
     104,     0,   108,     0,   113,     0,     0,     0,   110,   152,
     151,   127,     0,     0,   146,   130,   215,     0,     0,   118,
     121,   119,     0,     0,    30,    31,    32,     0,     0,    48,
      68,    69,    70,    75,    72,    89,     0,    84,    85,     0,
       0,    96,     0,    51,     0,     0,     0,     8,   114,   106,
     109,     0,     0,   111,     0,     0,   141,   140,     0,   155,
      22,     0,     0,    29,    28,     0,     0,     0,     0,    86,
      87,   100,     0,     0,    97,    56,     0,     0,     0,     0,
     105,     0,   162,     0,     0,     0,   160,   158,   139,   128,
     147,     0,     0,     0,     0,   120,   122,     0,    36,    35,
      62,    61,    59,    60,    57,    79,    78,    73,    76,    90,
      93,    92,    91,    94,    88,     0,    41,     0,    52,    53,
       0,    54,     0,    49,   107,   165,     0,   157,   161,   159,
       0,     0,     0,     0,     0,     0,    39,     0,     0,    66,
      64,    65,    63,     0,     0,     0,   101,     0,     0,     0,
     164,   148,   115,   115,   115,   115,     0,    38,     0,     0,
       0,    77,    42,    55,    50,   167,     0,   169,   163,     0,
     166,     0,     0,     0,     0,     0,    40,     0,     0,     0,
     115,     0,     0,   149,   125,   124,   126,   123,   176,   172,
     173,   174,   175,    37,     0,     0,     0,   171,   170,     0,
       0,   168,     0,    33,     0,     0,     0,     0,     0,     0,
       0,    44,     0,     0,    58,     0,     0,     0,     0,     0,
       0,    45,     0,    34,     0,     0,    43,    46
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     9,    23,     5,    12,    52,    13,    24,
      67,   111,   213,   214,   215,   216,   288,   325,   119,   393,
     401,   113,   296,   219,   294,   172,   220,    25,    69,   120,
     297,   298,   116,   176,   177,   227,   228,   259,    26,    71,
     121,   264,   122,    27,    28,    29,   134,    75,    65,   109,
     211,   251,   286,   200,   150,   151,   203,   202,   152,   153,
     154,   198,   275,   276,   339,   360,   383,    53,    54,    55,
      56,    57,    58,    97,    59,    60,    61,    62,    63,    64
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -273
static const yytype_int16 yypact[] =
{
     125,   -43,   -31,    40,  -273,   184,   287,    91,  -273,   369,
      41,     5,    81,   134,  -273,  -273,  -273,   178,   217,   235,
     267,   284,   295,  -273,  -273,  -273,  -273,  -273,  -273,  -273,
      44,  -273,  -273,  -273,   169,   272,   277,   131,   -15,   333,
    -273,  -273,    10,  -273,  -273,  -273,  -273,   153,   153,   224,
     224,   153,  -273,   228,  -273,  -273,   318,   320,   218,   249,
     209,  -273,   285,   273,  -273,    15,   340,   316,   347,  -273,
       7,  -273,   111,   288,  -273,   289,  -273,   293,    85,   224,
     323,   300,  -273,  -273,     9,   292,   319,  -273,   224,   153,
     153,  -273,  -273,  -273,  -273,  -273,  -273,   153,   153,   153,
     153,   153,   153,   153,  -273,   291,   325,  -273,  -273,  -273,
     157,  -273,   111,  -273,   315,   354,   148,   225,   224,  -273,
    -273,   355,   361,   226,   396,   224,   -10,  -273,   223,  -273,
     234,  -273,   111,  -273,   237,  -273,  -273,  -273,   332,  -273,
    -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,   323,   224,
     323,  -273,   334,   337,  -273,   224,  -273,  -273,   320,   218,
     249,   209,   209,  -273,  -273,  -273,  -273,   323,   336,   399,
     402,   403,   237,   404,   347,    26,  -273,  -273,  -273,   323,
     343,     7,  -273,   323,   405,   348,   349,  -273,   345,    59,
    -273,   290,  -273,   339,  -273,   352,   353,   266,  -273,   323,
     357,   323,   351,   358,   360,  -273,  -273,   323,   363,   357,
    -273,  -273,   364,   362,  -273,  -273,  -273,   365,   368,  -273,
    -273,  -273,  -273,  -273,   378,  -273,   370,  -273,   165,   120,
     224,   414,   132,   255,   417,   224,   350,  -273,  -273,  -273,
    -273,    99,   416,  -273,   366,   323,  -273,  -273,   224,  -273,
    -273,    13,   119,  -273,  -273,   167,   137,   224,    20,   361,
    -273,  -273,   175,   371,  -273,  -273,   323,   323,   256,   204,
    -273,   356,  -273,   359,   374,   372,   361,   361,  -273,   323,
     367,   375,   376,   379,   323,  -273,  -273,   377,   380,   382,
      80,    96,  -273,  -273,   383,   226,  -273,   384,  -273,  -273,
    -273,  -273,   226,  -273,  -273,   224,  -273,   323,  -273,  -273,
     437,  -273,   224,  -273,  -273,  -273,   224,  -273,  -273,  -273,
     224,   385,   386,   387,   276,   270,  -273,   438,   444,  -273,
    -273,  -273,  -273,   445,   137,   373,   323,   389,   388,    12,
    -273,   391,  -273,  -273,  -273,  -273,   377,  -273,   394,   397,
     398,  -273,  -273,  -273,  -273,   395,   224,  -273,  -273,   224,
    -273,   224,    65,    67,    68,   109,  -273,   164,   224,   167,
    -273,   446,   392,  -273,  -273,  -273,  -273,  -273,  -273,  -273,
    -273,  -273,  -273,  -273,   401,   406,    24,  -273,  -273,   456,
     458,  -273,     0,  -273,   407,   408,   141,   224,   167,   224,
     224,  -273,   451,   409,  -273,   410,   411,   224,   462,   224,
     224,  -273,    -1,  -273,   412,   413,  -273,  -273
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -273,  -273,  -273,   -68,  -273,  -273,   455,  -273,   460,  -273,
    -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -272,    71,
    -273,  -273,   -67,  -273,  -238,  -273,  -273,  -273,  -273,   -59,
    -273,   142,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,
     299,  -273,  -273,  -273,  -273,  -273,  -273,   -35,  -125,  -273,
    -273,  -273,  -273,   314,  -141,  -146,  -273,  -273,   283,   -87,
     278,  -131,  -273,  -273,  -273,  -273,  -273,   -49,  -273,  -273,
    -273,   400,   393,  -273,   390,   238,   152,   240,  -273,  -273
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -20
static const yytype_int16 yytable[] =
{
      85,    86,   189,    77,   205,   126,   128,   201,   130,   115,
     -19,   133,   117,   395,   395,   326,   281,   282,   355,     6,
     105,   283,    73,   300,   301,   302,   201,    73,   356,   105,
     135,     7,   225,   -19,   -19,   -19,   -19,   -19,   -19,   157,
       8,   222,   -19,   396,   396,   171,   284,    40,    41,    42,
      43,    76,    44,    45,    46,   205,   187,   357,   106,    79,
      79,   191,   397,   193,   105,   194,   243,   106,   226,   180,
     105,    31,   105,   105,   366,   118,   186,    80,    80,   358,
     285,   178,   107,   124,   359,   329,   108,   182,   125,   391,
     123,   188,   229,   330,    16,   108,   232,    47,    48,   197,
     204,   331,   106,    30,   279,   272,   208,    49,   106,   332,
     106,   106,    50,    11,   105,   224,   123,    51,   132,   236,
     308,   309,    40,    41,    84,    43,   238,    44,    45,    46,
     108,   385,   374,   205,   375,   376,   108,   221,   108,   108,
     273,   274,   295,   324,    40,    41,    84,    43,   124,    44,
      45,    46,   106,   125,     1,     2,    40,    41,    81,    43,
     404,    44,    45,    46,    72,    10,   336,   378,    73,   271,
     290,   291,    47,    48,   124,   292,   377,   293,   205,   125,
     108,   262,   287,    34,   207,    73,   269,    50,   175,   261,
     205,   303,    51,   260,    47,    48,   207,    74,   258,   280,
     124,   265,    73,   289,   400,   125,    47,    48,   299,    50,
     379,   380,   381,   382,    51,    10,    11,   362,   363,   364,
     365,    50,    35,   169,   304,   170,    51,    40,    41,    84,
      43,    66,    44,    45,    46,    91,    92,    93,    94,   305,
      36,   318,   319,    88,   306,   386,    17,    18,    19,    20,
      21,    22,   163,   164,   165,   166,   335,    17,    18,    19,
      20,    21,    22,   338,   100,   101,   102,   340,   312,    95,
      96,   341,    37,   313,    73,   195,   196,    47,    48,   136,
     137,   138,   139,   140,   141,   142,   143,    82,    83,    38,
     190,    87,    50,   179,   183,   173,   184,    51,    14,    15,
      39,   192,    98,    99,   195,   196,    68,   371,   266,   267,
     372,    70,   373,    17,    18,    19,    20,    21,    22,   384,
     310,   311,   144,   145,   146,   147,   136,   137,   138,   139,
     140,   141,   142,   143,   346,   347,   161,   162,    78,   148,
      89,    90,   345,   103,   149,   110,   104,   402,   403,   112,
     405,   406,   114,   167,   127,   129,   155,   239,   411,   131,
     414,   415,    17,    18,    19,    20,    21,    22,    80,   144,
     145,   146,   147,    17,    18,    19,    20,    21,    22,    17,
      18,    19,    20,    21,    22,   173,   148,   168,   156,   174,
     181,   149,    17,    18,    19,    20,    21,    22,    73,   185,
     199,   207,   210,   206,   212,   217,   240,   230,   218,   223,
     233,   237,   234,   235,   256,   242,   246,   270,   241,   263,
     268,   245,   277,   314,   248,   315,   252,   247,   250,   253,
     255,   320,   257,   307,   254,   278,   316,   321,   322,   317,
     337,   323,   352,   348,   327,   118,   328,   333,   334,   349,
     350,   342,   343,   344,   353,   361,   367,   354,   370,   368,
     369,   392,   387,   394,   388,   389,   407,   412,    33,   398,
     390,   399,    32,   408,   409,   410,   351,   416,   417,   413,
     231,   209,   244,   159,     0,   249,     0,   160,     0,   158
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-273)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      49,    50,   127,    38,   150,    72,    74,   148,    76,    68,
       0,    78,     5,    14,    14,   287,     3,     4,     6,    62,
       5,     8,    37,     3,     4,     5,   167,    37,    16,     5,
      79,    62,     6,    23,    24,    25,    26,    27,    28,    88,
       0,   172,    32,    44,    44,   112,    33,     3,     4,     5,
       6,    66,     8,     9,    10,   201,    66,    45,    43,    50,
      50,   129,    62,   131,     5,   132,   197,    43,    42,   118,
       5,    66,     5,     5,   346,    68,   125,    68,    68,    67,
      67,   116,    67,    63,    72,     5,    71,   122,    68,    65,
       5,   126,   179,    13,     3,    71,   183,    53,    54,   134,
     149,     5,    43,    62,   245,     6,   155,    63,    43,    13,
      43,    43,    68,    32,     5,   174,     5,    73,    33,   187,
     266,   267,     3,     4,     5,     6,    67,     8,     9,    10,
      71,   369,    67,   279,    67,    67,    71,   172,    71,    71,
      41,    42,     5,   284,     3,     4,     5,     6,    63,     8,
       9,    10,    43,    68,    29,    30,     3,     4,     5,     6,
     398,     8,     9,    10,    33,    31,   307,     3,    37,   237,
       3,     4,    53,    54,    63,     8,    67,    10,   324,    68,
      71,   230,    63,     5,    64,    37,   235,    68,    40,    69,
     336,   258,    73,   228,    53,    54,    64,    66,    33,   248,
      63,    69,    37,   252,    63,    68,    53,    54,   257,    68,
      46,    47,    48,    49,    73,    31,    32,   342,   343,   344,
     345,    68,     5,    66,   259,    68,    73,     3,     4,     5,
       6,    62,     8,     9,    10,    17,    18,    19,    20,    64,
       5,   276,   277,    15,    69,   370,    23,    24,    25,    26,
      27,    28,   100,   101,   102,   103,   305,    23,    24,    25,
      26,    27,    28,   312,    55,    56,    57,   316,    64,    51,
      52,   320,     5,    69,    37,    38,    39,    53,    54,     3,
       4,     5,     6,     7,     8,     9,    10,    47,    48,     5,
      67,    51,    68,    68,    68,    70,    70,    73,    11,    12,
       5,    67,    53,    54,    38,    39,    34,   356,    53,    54,
     359,    34,   361,    23,    24,    25,    26,    27,    28,   368,
      64,    65,    46,    47,    48,    49,     3,     4,     5,     6,
       7,     8,     9,    10,    64,    65,    98,    99,     5,    63,
      22,    21,    66,    58,    68,     5,    73,   396,   397,    33,
     399,   400,     5,    62,    66,    66,    64,    67,   407,    66,
     409,   410,    23,    24,    25,    26,    27,    28,    68,    46,
      47,    48,    49,    23,    24,    25,    26,    27,    28,    23,
      24,    25,    26,    27,    28,    70,    63,    62,    69,    35,
      35,    68,    23,    24,    25,    26,    27,    28,    37,     3,
      68,    64,    66,    69,     5,     3,    67,    64,     5,     5,
       5,    66,    64,    64,    36,    62,    65,    67,    66,     5,
       3,    64,     6,    67,    64,    66,    62,    69,    65,    67,
      62,    64,    62,    62,    69,    69,    62,    62,    62,    67,
       3,    62,    69,     5,    64,    68,    64,    64,    64,     5,
       5,    66,    66,    66,    65,    64,    62,    69,    63,    62,
      62,     5,    16,     5,    72,    64,    15,     5,    13,    62,
      64,    63,    12,    64,    64,    64,   334,    65,    65,   408,
     181,   167,   199,    90,    -1,   207,    -1,    97,    -1,    89
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    29,    30,    75,    76,    79,    62,    62,     0,    77,
      31,    32,    80,    82,    11,    12,     3,    23,    24,    25,
      26,    27,    28,    78,    83,   101,   112,   117,   118,   119,
      62,    66,    82,    80,     5,     5,     5,     5,     5,     5,
       3,     4,     5,     6,     8,     9,    10,    53,    54,    63,
      68,    73,    81,   141,   142,   143,   144,   145,   146,   148,
     149,   150,   151,   152,   153,   122,    62,    84,    34,   102,
      34,   113,    33,    37,    66,   121,    66,   121,     5,    50,
      68,     5,   151,   151,     5,   141,   141,   151,    15,    22,
      21,    17,    18,    19,    20,    51,    52,   147,    53,    54,
      55,    56,    57,    58,    73,     5,    43,    67,    71,   123,
       5,    85,    33,    95,     5,   103,   106,     5,    68,    92,
     103,   114,   116,     5,    63,    68,    96,    66,    77,    66,
      77,    66,    33,    96,   120,   141,     3,     4,     5,     6,
       7,     8,     9,    10,    46,    47,    48,    49,    63,    68,
     128,   129,   132,   133,   134,    64,    69,   141,   145,   146,
     148,   149,   149,   150,   150,   150,   150,    62,    62,    66,
      68,    96,    99,    70,    35,    40,   107,   108,   121,    68,
     141,    35,   121,    68,    70,     3,   141,    66,   121,   122,
      67,    77,    67,    77,    96,    38,    39,   121,   135,    68,
     127,   128,   131,   130,   141,   129,    69,    64,   141,   127,
      66,   124,     5,    86,    87,    88,    89,     3,     5,    97,
     100,   121,   135,     5,   103,     6,    42,   109,   110,   133,
      64,   114,   133,     5,    64,    64,    77,    66,    67,    67,
      67,    66,    62,   135,   132,    64,    65,    69,    64,   134,
      65,   125,    62,    67,    69,    62,    36,    62,    33,   111,
     121,    69,   141,     5,   115,    69,    53,    54,     3,   141,
      67,    77,     6,    41,    42,   136,   137,     6,    69,   128,
     141,     3,     4,     8,    33,    67,   126,    63,    90,   141,
       3,     4,     8,    10,    98,     5,    96,   104,   105,   141,
       3,     4,     5,    96,   121,    64,    69,    62,   129,   129,
      64,    65,    64,    69,    67,    66,    62,    67,   121,   121,
      64,    62,    62,    62,   128,    91,    92,    64,    64,     5,
      13,     5,    13,    64,    64,   141,   128,     3,   141,   138,
     141,   141,    66,    66,    66,    66,    64,    65,     5,     5,
       5,   105,    69,    65,    69,     6,    16,    45,    67,    72,
     139,    64,   122,   122,   122,   122,    92,    62,    62,    62,
      63,   141,   141,   141,    67,    67,    67,    67,     3,    46,
      47,    48,    49,   140,   141,    98,   122,    16,    72,    64,
      64,    65,     5,    93,     5,    14,    44,    62,    62,    63,
      63,    94,   141,   141,    98,   141,   141,    15,    64,    64,
      64,   141,     5,    93,   141,   141,    65,    65
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
      
/* Line 1792 of yacc.c  */
#line 1817 "flowchart_parser.cpp"
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



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
		      yytoken, &yylval);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2055 of yacc.c  */
#line 603 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\flowchart.y"


void yyerror(const char *s) {
    fprintf(stderr, "flowchart.y: %s near '%s' on line %d\n",
            s, yytext, yylineno);
}

int main(void) {
    int rc = yyparse();
    if (rc == 0) printf("ok\n");
    return rc;
}
