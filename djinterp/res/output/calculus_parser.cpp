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
#line 23 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\calculus.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int   yylex(void);
extern int   yylineno;
extern char *yytext;
void yyerror(const char *s);

/* Line 371 of yacc.c  */
#line 79 "calculus_parser.cpp"

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
   by #include "calculus_parser.hpp".  */
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
/* Line 387 of yacc.c  */
#line 34 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\calculus.y"

    long    ival;
    double  fval;
    char   *sval;


/* Line 387 of yacc.c  */
#line 333 "calculus_parser.cpp"
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

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 361 "calculus_parser.cpp"

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
#define YYFINAL  271
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2724

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  227
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  59
/* YYNRULES -- Number of rules.  */
#define YYNRULES  305
/* YYNRULES -- Number of states.  */
#define YYNSTATES  500

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   459

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   215,     2,     2,     2,   208,     2,     2,
     219,   220,   206,   204,   224,   205,   214,   207,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   225,   223,
     202,   201,   203,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   217,     2,   218,   209,   216,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   221,   226,   222,     2,     2,     2,     2,
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
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   210,   211,   212,   213
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      21,    23,    25,    29,    33,    37,    39,    43,    47,    51,
      53,    57,    61,    65,    67,    71,    75,    79,    81,    84,
      87,    89,    93,    95,    97,    99,   101,   103,   105,   107,
     109,   111,   113,   115,   117,   119,   121,   125,   129,   131,
     135,   139,   143,   145,   149,   151,   155,   159,   163,   167,
     171,   173,   177,   179,   182,   184,   186,   189,   192,   195,
     197,   200,   203,   206,   209,   212,   214,   216,   218,   220,
     224,   228,   233,   238,   242,   246,   250,   253,   257,   261,
     265,   269,   275,   279,   283,   286,   291,   293,   295,   297,
     299,   301,   303,   305,   307,   311,   315,   321,   323,   325,
     327,   329,   333,   337,   341,   345,   347,   351,   357,   363,
     369,   375,   377,   381,   383,   385,   387,   389,   391,   393,
     395,   397,   399,   401,   403,   405,   407,   409,   411,   413,
     415,   417,   422,   424,   426,   428,   430,   432,   434,   436,
     438,   440,   442,   444,   446,   448,   450,   452,   454,   456,
     458,   460,   462,   464,   466,   468,   470,   472,   474,   476,
     478,   480,   482,   484,   486,   488,   490,   492,   494,   496,
     498,   500,   502,   504,   506,   508,   510,   512,   514,   516,
     518,   520,   522,   524,   526,   528,   530,   532,   534,   536,
     538,   540,   542,   544,   546,   548,   550,   552,   554,   556,
     558,   560,   562,   564,   566,   568,   570,   572,   574,   576,
     578,   580,   582,   584,   586,   588,   590,   592,   594,   596,
     598,   600,   602,   604,   606,   608,   610,   612,   614,   616,
     618,   620,   622,   624,   626,   628,   630,   632,   634,   636,
     638,   640,   642,   644,   646,   648,   650,   652,   654,   656,
     658,   660,   662,   664,   666,   668,   670,   672,   674,   676,
     678,   682,   690,   701,   712,   721,   725,   730,   738,   747,
     754,   759,   765,   770,   779,   783,   786,   790,   794,   797,
     800,   805,   812,   817,   824,   829,   836,   841,   848,   850,
     852,   859,   868,   872,   876,   880
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     228,     0,    -1,   229,    -1,   230,    -1,   281,    -1,   283,
      -1,   284,    -1,   231,    -1,   232,    -1,     5,   201,   230,
      -1,   233,    -1,   234,    -1,   233,    13,   234,    -1,   233,
     135,   234,    -1,   233,   144,   234,    -1,   235,    -1,   234,
      14,   235,    -1,   234,    15,   235,    -1,   234,   143,   235,
      -1,   236,    -1,   235,    12,   236,    -1,   235,   132,   236,
      -1,   235,   141,   236,    -1,   237,    -1,   236,    11,   237,
      -1,   236,   133,   237,    -1,   236,   140,   237,    -1,   238,
      -1,   134,   237,    -1,   142,   237,    -1,   240,    -1,   238,
     239,   240,    -1,     7,    -1,     8,    -1,     9,    -1,    10,
      -1,   202,    -1,   203,    -1,   147,    -1,   148,    -1,   151,
      -1,   152,    -1,   153,    -1,   154,    -1,   159,    -1,   241,
      -1,   240,   149,   241,    -1,   240,   150,   241,    -1,   242,
      -1,   241,   204,   242,    -1,   241,   205,   242,    -1,   241,
      20,   242,    -1,   243,    -1,   242,   186,   243,    -1,   244,
      -1,   243,   206,   244,    -1,   243,   207,   244,    -1,   243,
     208,   244,    -1,   243,   156,   244,    -1,   243,   155,   244,
      -1,   245,    -1,   245,   209,   244,    -1,   247,    -1,   245,
     246,    -1,   248,    -1,   248,    -1,   204,   248,    -1,   205,
     248,    -1,   215,   248,    -1,   249,    -1,   248,   215,    -1,
     248,    19,    -1,   248,    17,    -1,   248,    16,    -1,   248,
      18,    -1,   257,    -1,   258,    -1,   259,    -1,     5,    -1,
       5,   216,     3,    -1,     5,   216,     5,    -1,     5,   217,
     230,   218,    -1,     5,   219,   256,   220,    -1,   219,   230,
     220,    -1,   219,   253,   220,    -1,   217,   251,   218,    -1,
     221,   222,    -1,   221,   254,   222,    -1,   221,   255,   222,
      -1,    22,   230,    23,    -1,    24,   230,    25,    -1,    24,
     230,    25,   216,     3,    -1,   178,   230,   179,    -1,   180,
     230,   181,    -1,    21,   249,    -1,   158,   219,   230,   220,
      -1,   273,    -1,   274,    -1,   275,    -1,   276,    -1,   277,
      -1,   278,    -1,   279,    -1,   280,    -1,     3,   184,     3,
      -1,     3,   185,     3,    -1,   219,   250,   182,   250,   220,
      -1,     3,    -1,     5,    -1,   256,    -1,   252,    -1,   256,
     223,   256,    -1,   252,   223,   256,    -1,   230,   224,   230,
      -1,   253,   224,   230,    -1,   230,    -1,   254,   224,   230,
      -1,     5,   147,   230,   225,   230,    -1,     5,   147,   230,
     226,   230,    -1,     5,   175,   230,   225,   230,    -1,     5,
     175,   230,   226,   230,    -1,   230,    -1,   256,   224,   230,
      -1,     3,    -1,     4,    -1,    26,    -1,    30,    -1,    29,
      -1,    27,    -1,    28,    -1,   145,    -1,   146,    -1,   136,
      -1,   137,    -1,   160,    -1,   161,    -1,   162,    -1,   163,
      -1,   164,    -1,   157,    -1,   174,    -1,   260,   219,   256,
     220,    -1,   261,    -1,   262,    -1,   263,    -1,   264,    -1,
     265,    -1,   266,    -1,   267,    -1,   268,    -1,   269,    -1,
     270,    -1,   271,    -1,   272,    -1,    31,    -1,    32,    -1,
      33,    -1,    34,    -1,    35,    -1,    36,    -1,    37,    -1,
      38,    -1,    39,    -1,    40,    -1,    41,    -1,    42,    -1,
      43,    -1,    44,    -1,    45,    -1,    46,    -1,    47,    -1,
      48,    -1,    49,    -1,    50,    -1,    51,    -1,    52,    -1,
      53,    -1,    54,    -1,    55,    -1,    56,    -1,    57,    -1,
      58,    -1,    59,    -1,    60,    -1,    61,    -1,    62,    -1,
      63,    -1,    64,    -1,    65,    -1,    66,    -1,    67,    -1,
      68,    -1,    69,    -1,    70,    -1,    71,    -1,    72,    -1,
      73,    -1,    74,    -1,    75,    -1,    76,    -1,    77,    -1,
      78,    -1,    79,    -1,    80,    -1,    81,    -1,    82,    -1,
      83,    -1,    84,    -1,    85,    -1,    86,    -1,   108,    -1,
     109,    -1,   110,    -1,   111,    -1,   112,    -1,   113,    -1,
     114,    -1,   115,    -1,   116,    -1,   117,    -1,   118,    -1,
     119,    -1,   120,    -1,   121,    -1,   122,    -1,   123,    -1,
     124,    -1,   125,    -1,   126,    -1,   127,    -1,   128,    -1,
     129,    -1,    96,    -1,    99,    -1,   101,    -1,   102,    -1,
     103,    -1,    97,    -1,    98,    -1,   104,    -1,   105,    -1,
     165,    -1,   166,    -1,   167,    -1,   168,    -1,   169,    -1,
     170,    -1,   171,    -1,   172,    -1,   173,    -1,   175,    -1,
     176,    -1,   177,    -1,   183,    -1,   184,    -1,   185,    -1,
     187,    -1,   188,    -1,   189,    -1,   190,    -1,   191,    -1,
     192,    -1,   193,    -1,   194,    -1,   195,    -1,   196,    -1,
     197,    -1,   198,    -1,   199,    -1,   200,    -1,    87,   230,
       6,    -1,    87,   216,   249,   209,   249,   230,     6,    -1,
      91,   216,   219,     5,   201,   230,   220,   209,   249,   230,
      -1,    92,   216,   219,     5,   201,   230,   220,   209,   249,
     230,    -1,   100,   216,   219,     5,    15,   230,   220,   230,
      -1,   106,   207,     6,    -1,   106,   207,   106,     5,    -1,
     106,   209,     3,   207,     6,   209,     3,    -1,   106,   209,
       3,   207,   106,     5,   209,     3,    -1,   107,   216,     5,
     219,   230,   220,    -1,   107,   219,   230,   220,    -1,    90,
       5,   207,    90,     5,    -1,    90,   207,    90,     5,    -1,
      90,   209,     3,   207,    90,     5,   209,     3,    -1,    90,
     216,     5,    -1,    93,   249,    -1,    93,   156,   249,    -1,
      93,   155,   249,    -1,    94,     5,    -1,    95,     5,    -1,
     130,     5,   224,   282,    -1,   130,     5,   147,   230,   224,
     282,    -1,   138,     5,   224,   282,    -1,   138,     5,   175,
     230,   224,   282,    -1,   131,     5,   224,   282,    -1,   131,
       5,   147,   230,   224,   282,    -1,   139,     5,   224,   282,
      -1,   139,     5,   175,   230,   224,   282,    -1,   230,    -1,
     281,    -1,    88,   230,   106,     5,   106,     5,    -1,    89,
     230,   106,     5,   106,     5,   106,     5,    -1,   285,   201,
     230,    -1,     5,   216,     3,    -1,     5,   216,     5,    -1,
       5,   217,   230,   218,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   146,   146,   150,   151,   152,   153,   163,   164,   168,
     172,   176,   177,   178,   179,   183,   184,   185,   186,   190,
     191,   192,   193,   197,   198,   199,   200,   204,   205,   206,
     210,   211,   215,   215,   215,   215,   215,   215,   216,   216,
     217,   217,   217,   217,   218,   222,   223,   224,   228,   229,
     230,   231,   235,   236,   240,   241,   242,   243,   244,   245,
     249,   250,   254,   255,   259,   263,   264,   265,   266,   270,
     271,   272,   273,   274,   275,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   314,   315,   319,
     320,   324,   325,   329,   330,   334,   335,   339,   340,   341,
     342,   346,   347,   350,   350,   353,   353,   353,   353,   353,
     354,   354,   354,   354,   355,   355,   355,   355,   355,   356,
     356,   360,   364,   364,   365,   365,   366,   366,   367,   367,
     368,   368,   369,   369,   372,   372,   372,   372,   372,   372,
     373,   373,   373,   373,   373,   373,   373,   374,   374,   374,
     374,   374,   374,   375,   375,   375,   375,   375,   375,   376,
     376,   376,   376,   376,   378,   378,   378,   378,   378,   378,
     379,   379,   379,   379,   379,   380,   380,   380,   380,   382,
     382,   382,   382,   382,   382,   383,   383,   383,   383,   383,
     386,   386,   386,   386,   386,   386,   387,   387,   387,   387,
     387,   387,   388,   388,   388,   388,   388,   389,   389,   389,
     389,   389,   392,   392,   392,   392,   392,   393,   393,   393,
     393,   396,   396,   396,   396,   396,   396,   397,   397,   397,
     397,   397,   397,   400,   400,   400,   401,   401,   401,   401,
     402,   402,   402,   403,   403,   403,   403,   404,   404,   404,
     417,   421,   425,   429,   433,   437,   438,   439,   440,   441,
     442,   443,   444,   445,   446,   450,   451,   452,   456,   457,
     465,   466,   467,   468,   469,   470,   471,   472,   476,   477,
     481,   482,   486,   490,   491,   492
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "NUMBER", "IDENT",
  "DIFFERENTIAL", "EQ", "NEQ", "LEQ", "GEQ", "ANDAND", "OROR", "IFF",
  "IMPLIES", "ARROW", "CARET_INV", "CARET_T", "CARET_C", "PRIME",
  "BACKSLASH", "HASH", "ABS_OPEN", "ABS_CLOSE", "NORM_OPEN", "NORM_CLOSE",
  "PI", "EULER", "INF", "PHI", "TAU", "SIN", "COS", "TAN", "SEC", "CSC",
  "COT", "ASIN", "ACOS", "ATAN", "ATAN2", "ASEC", "ACSC", "ACOT", "SINH",
  "COSH", "TANH", "SECH", "CSCH", "COTH", "ASINH", "ACOSH", "ATANH",
  "ASECH", "ACSCH", "ACOTH", "LOG", "LN", "LOG10", "LOG2", "EXP", "SQRT",
  "CBRT", "ABS", "FLOOR", "CEIL", "ROUND", "MIN", "MAX", "GCD", "LCM",
  "FACTORIAL", "GAMMA", "ERF", "ERFC", "SIGN", "POLAR", "RECT", "THETA",
  "CYL", "RHO", "SPHER", "RE_FN", "IM_FN", "ARG_FN", "CONJ_FN", "MAG_FN",
  "INTEGRAL", "INTEGRAL2", "INTEGRAL3", "PARTIAL", "SUMMATION", "PRODUCT",
  "NABLA", "DELTA_CAP", "DELTA_LO", "DIFF_FN", "PARTIAL_KW", "GRAD_KW",
  "INTEGRATE_FN", "LIM", "LIMIT_FN", "SUM_FN", "PRODUCT_FN", "DIV_KW",
  "CURL_KW", "D_KW", "D_CAP", "TRANSPOSE_FN", "INV_FN", "DET_FN",
  "TRACE_FN", "TR_FN", "RANK_FN", "EIG_FN", "EIGVEC_FN", "DOT_FN",
  "CROSS_FN", "NORM_FN", "NORMALIZE_FN", "ZEROS_FN", "ONES_FN", "EYE_FN",
  "DIAG_FN", "RESHAPE_FN", "SOLVE_FN", "LU_FN", "QR_FN", "SVD_FN",
  "CHOLESKY_FN", "FORALL", "EXISTS", "OR_SYM", "AND_SYM", "NOT_SYM",
  "IFF_SYM", "TOP_SYM", "BOT_SYM", "FORALL_KW", "EXISTS_KW", "AND_KW",
  "OR_KW", "NOT_KW", "IMPLIES_KW", "IFF_KW", "TRUE_KW", "FALSE_KW",
  "IN_SYM", "NOTIN_SYM", "UNION_SYM", "INTER_SYM", "SUBSETEQ", "SUPSETEQ",
  "SUBSET", "SUPSET", "TIMES_SYM", "CDOT_SYM", "EMPTY_SYM", "POWERSET",
  "EQUIV_SYM", "NAT_SET", "INT_SET", "RAT_SET", "REAL_SET", "CPX_SET",
  "UNION_KW", "INTER_KW", "DIFF_KW", "COMP_KW", "CART_KW", "PWRSET_KW",
  "SUBSET_KW", "SUPSET_KW", "PSUBSET_KW", "EMPTY_KW", "IN_KW", "CARD_KW",
  "SUP_KW", "LFLOOR", "RFLOOR", "LCEIL", "RCEIL", "CHOOSE_KW", "BINOM_KW",
  "PERM_KW", "COMB_KW", "MOD_KW", "TOTIENT_KW", "MOBIUS_KW", "ISPRIME_KW",
  "NEXTPRIME_KW", "PRIMES_KW", "FACTORS_KW", "SEQUENCE_KW", "DEGREE_KW",
  "NEIGHBORS_KW", "ADJ_KW", "CONN_KW", "CYCLE_KW", "PATH_KW", "GRAPH_KW",
  "'='", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'", "UNOT",
  "UPLUS", "UMINUS", "POSTFIX_BANG", "'.'", "'!'", "'_'", "'['", "']'",
  "'('", "')'", "'{'", "'}'", "';'", "','", "':'", "'|'", "$accept",
  "math_expression", "top_form", "expression", "assignment", "conditional",
  "iff_form", "implication", "logical_or", "logical_and", "logical_not",
  "comparison", "comp_op", "set_union", "additive", "modular",
  "multiplicative", "exponential", "implicit_mult", "juxt_factor", "unary",
  "postfix", "primary", "combinatoric_atom", "bracket_body",
  "matrix_rows_2plus", "argument_list_2plus", "set_elements",
  "set_builder_body", "argument_list", "number", "constant",
  "known_function_call", "known_function_name", "trig_function",
  "inverse_trig_function", "hyperbolic_function",
  "inverse_hyperbolic_function", "logarithmic_function",
  "special_function", "coordinate_function", "complex_function",
  "linalg_function_name", "calculus_function_name", "set_function_name",
  "discrete_function_name", "indefinite_integral", "definite_integral",
  "summation_form", "product_form", "limit_form", "derivative_form",
  "nabla_form", "differential_form", "quantified_expression",
  "quantifier_body", "calculus_form", "recurrence_relation",
  "sequence_term", YY_NULL
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
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,    61,    60,    62,    43,    45,    42,    47,    37,    94,
     456,   457,   458,   459,    46,    33,    95,    91,    93,    40,
      41,   123,   125,    59,    44,    58,   124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   227,   228,   229,   229,   229,   229,   230,   230,   231,
     232,   233,   233,   233,   233,   234,   234,   234,   234,   235,
     235,   235,   235,   236,   236,   236,   236,   237,   237,   237,
     238,   238,   239,   239,   239,   239,   239,   239,   239,   239,
     239,   239,   239,   239,   239,   240,   240,   240,   241,   241,
     241,   241,   242,   242,   243,   243,   243,   243,   243,   243,
     244,   244,   245,   245,   246,   247,   247,   247,   247,   248,
     248,   248,   248,   248,   248,   249,   249,   249,   249,   249,
     249,   249,   249,   249,   249,   249,   249,   249,   249,   249,
     249,   249,   249,   249,   249,   249,   249,   249,   249,   249,
     249,   249,   249,   249,   249,   249,   249,   250,   250,   251,
     251,   252,   252,   253,   253,   254,   254,   255,   255,   255,
     255,   256,   256,   257,   257,   258,   258,   258,   258,   258,
     258,   258,   258,   258,   258,   258,   258,   258,   258,   258,
     258,   259,   260,   260,   260,   260,   260,   260,   260,   260,
     260,   260,   260,   260,   261,   261,   261,   261,   261,   261,
     262,   262,   262,   262,   262,   262,   262,   263,   263,   263,
     263,   263,   263,   264,   264,   264,   264,   264,   264,   265,
     265,   265,   265,   265,   266,   266,   266,   266,   266,   266,
     266,   266,   266,   266,   266,   266,   266,   266,   266,   267,
     267,   267,   267,   267,   267,   268,   268,   268,   268,   268,
     269,   269,   269,   269,   269,   269,   269,   269,   269,   269,
     269,   269,   269,   269,   269,   269,   269,   269,   269,   269,
     269,   269,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   272,   272,   272,   272,   272,   272,   272,
     272,   272,   272,   272,   272,   272,   272,   272,   272,   272,
     273,   274,   275,   276,   277,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   279,   279,   279,   280,   280,
     281,   281,   281,   281,   281,   281,   281,   281,   282,   282,
     283,   283,   284,   285,   285,   285
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     3,     3,     3,     1,     3,     3,     3,     1,
       3,     3,     3,     1,     3,     3,     3,     1,     2,     2,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     1,     3,
       3,     3,     1,     3,     1,     3,     3,     3,     3,     3,
       1,     3,     1,     2,     1,     1,     2,     2,     2,     1,
       2,     2,     2,     2,     2,     1,     1,     1,     1,     3,
       3,     4,     4,     3,     3,     3,     2,     3,     3,     3,
       3,     5,     3,     3,     2,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     5,     1,     1,     1,
       1,     3,     3,     3,     3,     1,     3,     5,     5,     5,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     7,    10,    10,     8,     3,     4,     7,     8,     6,
       4,     5,     4,     8,     3,     2,     3,     3,     2,     2,
       4,     6,     4,     6,     4,     6,     4,     6,     1,     1,
       6,     8,     3,     3,     3,     4
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,   123,   124,    78,     0,     0,     0,   125,   128,   129,
     127,   126,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   232,   237,   238,
     233,     0,   234,   235,   236,   239,   240,     0,     0,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,     0,     0,     0,   132,   133,     0,     0,     0,   130,
     131,   139,     0,   134,   135,   136,   137,   138,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   140,   250,   251,
     252,     0,     0,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
       0,     0,     0,     0,     0,     0,     0,     2,     3,     7,
       8,    10,    11,    15,    19,    23,    27,    30,    45,    48,
      52,    54,    60,    62,    65,    69,    75,    76,    77,     0,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,    96,    97,    98,    99,   100,   101,   102,   103,
       4,     5,     6,     0,     0,     0,     0,     0,     0,     0,
      78,    94,    78,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   285,   288,   289,
       0,     0,     0,     0,     0,     0,     0,    28,     0,     0,
      29,     0,     0,     0,    66,    67,    68,   121,     0,   110,
     109,   123,    78,     0,     0,     0,    78,    86,   115,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,    35,    38,    39,
      40,    41,    42,    43,    44,    36,    37,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      63,    64,    73,    72,    74,    71,    70,     0,     0,   104,
     105,     9,    79,    80,     0,     0,     0,     0,    89,    90,
       0,   270,     0,     0,     0,     0,     0,   284,     0,     0,
     287,   286,     0,   275,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    92,    93,    85,
       0,     0,     0,    83,     0,     0,    84,     0,     0,     0,
      87,     0,    88,    12,    13,    14,    16,    17,    18,    20,
      21,    22,    24,    25,    26,    31,    46,    47,    51,    49,
      50,    53,    59,    58,    55,    56,    57,    61,     0,   302,
      81,    82,    79,    80,     0,     0,     0,     0,     0,     0,
     282,     0,     0,     0,     0,   276,     0,     0,   280,     0,
     298,   299,   290,     0,   294,     0,   292,     0,   296,    95,
     112,   111,   122,   113,   107,   108,     0,   114,     0,     0,
     116,   141,    81,    91,     0,     0,     0,   281,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   106,
       0,     0,     0,     0,     0,   300,     0,     0,     0,     0,
       0,     0,     0,   279,   291,   295,   293,   297,   117,   118,
     119,   120,   271,     0,     0,     0,     0,     0,   277,     0,
     301,   283,     0,     0,   274,   278,     0,     0,   272,   273
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   166,   167,   420,   169,   170,   171,   172,   173,   174,
     175,   176,   297,   177,   178,   179,   180,   181,   182,   310,
     183,   184,   185,   264,   258,   259,   265,   269,   270,   260,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   421,   422,   211,   212,   213
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -331
static const yytype_int16 yypact[] =
{
     680,   -91,  -331,  -158,  2503,  1289,  1289,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  1086,  1289,
    1289,    -2,  -177,  -129,  2101,    40,    92,  -331,  -331,  -331,
    -331,  -110,  -331,  -331,  -331,  -331,  -331,  -133,  -130,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,   128,   132,  1492,  -331,  -331,   133,   134,  1492,  -331,
    -331,  -331,   -79,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  1289,  1289,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    2503,  2503,  2503,  1289,  1695,   477,   147,  -331,  -331,  -331,
    -331,    -1,     5,    12,    16,  -331,     7,   -49,   -18,   -36,
    -124,  -331,  2302,  -331,    -9,  -331,  -331,  -331,  -331,   -68,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,   -44,   149,   159,  1289,   110,  1289,  1289,
    -138,  -331,  -154,   144,   143,  2503,   163,    64,    65,   -35,
      83,   171,   170,   -43,   -42,  2503,  2503,  -331,  -331,  -331,
     -41,    23,   176,   175,  1289,  -136,  -125,  -331,  -147,  -139,
    -331,  1289,     6,     3,    -9,    -9,    -9,  -331,   -30,   -34,
    -102,  -118,  -144,  -183,     8,  -180,  -121,  -331,  -331,  -108,
     -31,  -331,  1492,  1492,  1492,  1492,  1492,  1492,  1492,  1492,
    1492,  1492,  1492,  1492,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  1898,  1898,  1898,
    1898,  1898,  1898,  1898,  1898,  1898,  1898,  1898,  1898,  1898,
    -331,    -9,  -331,  -331,  -331,  -331,  -331,  1289,  1289,  -331,
    -331,  -331,    -8,    -7,   -26,  -178,   114,  1289,  -331,   -21,
     -13,  -331,   192,   193,   109,   195,    -6,  -331,   197,   198,
    -331,  -331,   199,  -331,   203,     4,    -4,     1,  1289,   883,
    1289,   883,  1289,   883,  1289,   883,     9,  -331,  -331,  -331,
    1289,  1289,  1289,  -331,  1289,   115,  -331,  1289,  1289,  1289,
    -331,  1289,  -331,     5,     5,     5,    12,    12,    12,    16,
      16,    16,  -331,  -331,  -331,   -49,   -18,   -18,   -36,   -36,
     -36,  -124,  -331,  -331,  -331,  -331,  -331,  -331,  -164,  -331,
      11,  -331,  -331,  -331,    -5,   214,  2503,   117,   118,   215,
    -331,   135,    25,    26,   213,  -331,    24,  1289,  -331,    10,
    -331,  -331,  -331,    13,  -331,    14,  -331,    15,  -331,  -331,
      17,    17,  -331,  -331,  -331,  -331,    20,  -331,   -94,   -90,
    -331,  -331,  -331,  -331,  1289,   225,   226,  -331,   230,  1289,
    1289,  1289,    27,   237,    28,   883,   883,   883,   883,  -331,
    1289,  1289,  1289,  1289,   239,  -331,   137,    37,    29,    30,
      32,   244,    44,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,   249,   252,    47,    48,  1289,  -331,   255,
    -331,  -331,  2503,  2503,  -331,  -331,  1289,  1289,  -331,  -331
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -331,  -331,  -331,     0,  -331,  -331,  -331,  -182,  -172,  -171,
    -100,  -331,  -331,   -38,  -175,  -190,   -40,  -256,  -331,  -331,
    -331,  -127,    -3,  -105,  -331,  -331,  -331,  -331,  -331,  -215,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,  -331,
    -331,  -331,  -331,  -331,   261,  -330,  -331,  -331,  -331
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -306
static const yytype_int16 yytable[] =
{
     168,   221,   300,   229,   325,   223,   224,   312,   313,   314,
     315,   348,   272,   247,   284,   285,   286,   287,   250,   275,
     276,   424,   350,   426,   278,   428,   368,   281,   352,   343,
     452,   304,   305,   254,   255,   256,   354,   363,  -108,   233,
     366,   364,   401,   216,   367,   238,   362,   216,   392,   393,
     394,   395,   396,   397,   369,   311,   441,   216,   217,   218,
     362,   219,   326,   327,  -107,   219,   214,   215,   226,   227,
     228,   237,   326,   327,   241,   219,   242,   353,   326,   327,
     216,   219,   306,   307,   308,   355,   243,   234,   349,   244,
     373,   374,   375,   214,   215,   326,   327,   239,   219,   351,
     298,   299,   398,   376,   377,   378,   240,   379,   380,   381,
     388,   389,   390,   322,   370,   323,   371,   402,   434,   403,
     435,   361,   362,   386,   387,   474,   475,   476,   477,   344,
     453,   460,   461,   245,   273,   462,   463,   246,   248,   249,
     251,   252,   253,   274,   279,   430,   431,   271,   277,   282,
     303,   317,   319,   280,   288,   289,   283,   318,   290,   291,
     292,   293,   320,   257,   263,   268,   294,   328,   329,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   342,   345,
     346,   382,   383,   384,   358,   357,   301,   302,   359,   360,
     365,   372,   400,  -303,  -304,   405,   406,   407,   408,   409,
     410,   411,   412,   413,   414,   230,   316,   231,   415,   295,
     296,   416,  -305,   442,   232,   417,   321,   443,   324,   257,
     447,   418,   330,   445,   446,   448,   449,   450,   451,   429,
     465,   466,   340,   341,   455,   467,   471,   456,   457,   458,
     459,   362,   472,   483,   347,   482,   484,   488,   473,   485,
     486,   356,   487,   489,   490,   491,   492,   493,   495,   385,
     436,   210,     0,   391,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   257,   399,     0,
       0,     0,     0,     0,     0,     0,     0,   404,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   419,     0,
     423,     0,   425,     0,   427,     0,     0,     0,     0,     0,
     257,   257,   432,     0,   433,     0,     0,   437,   438,   439,
       0,   440,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   444,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   454,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   464,     0,     0,     0,     0,   468,
     469,   470,     0,     0,     0,     0,     0,     0,     0,     0,
     478,   479,   480,   481,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       1,     2,   266,     0,     0,     0,     0,   494,     0,   496,
     497,     0,     0,     0,     0,     0,   498,   499,     4,     5,
       0,     6,     0,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,     0,     0,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,     0,     0,     0,
       0,   113,     0,   114,   115,     0,     0,     0,     0,   118,
       0,     0,   119,   120,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   121,   122,     0,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,     0,   142,     0,     0,
     143,   144,   145,     0,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,     0,     0,
       0,   160,   161,     1,     2,     3,     0,     0,     0,     0,
       0,     0,   162,     0,   163,     0,   164,     0,   165,   267,
       0,     4,     5,     0,     6,     0,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,     0,     0,   113,     0,   114,   115,   116,   117,
       0,     0,   118,     0,     0,   119,   120,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   121,   122,     0,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,     0,
     142,     0,     0,   143,   144,   145,     0,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,     0,     0,     0,   160,   161,     1,     2,   222,     0,
       0,     0,     0,     0,     0,   162,     0,   163,     0,   164,
       0,   165,     0,     0,     4,     5,     0,     6,     0,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,     0,     0,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,     0,     0,   113,     0,   114,
     115,   116,   117,     0,     0,   118,     0,     0,   119,   120,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     121,   122,     0,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,     0,   142,     0,     0,   143,   144,   145,     0,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,     0,     0,     0,   160,   161,     1,
       2,   222,     0,     0,     0,     0,     0,     0,   162,     0,
     163,     0,   164,     0,   165,     0,     0,     4,     5,     0,
       6,     0,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,     0,     0,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     0,     0,     0,     0,
     113,     0,   114,   115,     0,     0,     0,     0,   118,     0,
       0,   119,   120,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   121,   122,     0,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,     0,   142,     0,     0,   143,
     144,   145,     0,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,     0,     0,     0,
     160,   161,     1,     2,   222,     0,     0,     0,     0,     0,
       0,   162,   225,   163,     0,   164,     0,   165,     0,     0,
       4,     5,     0,     6,     0,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,     0,     0,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,     0,
       0,     0,     0,   113,     0,   114,   115,     0,     0,     0,
       0,   118,     0,     0,   119,   120,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   121,   122,     0,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,     0,   142,
       0,     0,   143,   144,   145,     0,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
       0,     0,     0,   160,   161,     1,     2,   220,     0,     0,
       0,     0,     0,     0,   162,     0,   163,     0,   164,     0,
     165,     0,     0,     4,     5,     0,     6,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,     0,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     0,     0,     0,     0,   113,     0,   114,   115,
       0,     0,     0,     0,   118,     0,     0,   119,   120,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   121,
     122,     0,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,     0,   142,     0,     0,   143,   144,   145,     0,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,     0,     0,     0,   160,   161,   261,     2,
     262,     0,     0,     0,     0,     0,     0,   162,     0,   163,
       0,   164,     0,   165,     0,     0,     4,     5,     0,     6,
       0,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,     0,     0,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,     0,     0,     0,     0,   113,
       0,   114,   115,     0,     0,     0,     0,   118,     0,     0,
     119,   120,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   121,   122,     0,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,     0,   142,     0,     0,   143,   144,
     145,     0,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,     0,     0,     0,   160,
     161,     1,     2,   220,     0,     0,     0,     0,     0,     0,
     162,     0,   163,     0,   164,     0,   165,     0,     0,     4,
       5,     0,     6,     0,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,     0,     0,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,     0,     0,
       0,     0,     0,     0,   114,   115,     0,     0,     0,     0,
       0,     0,     0,   119,   120,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   121,   122,     0,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,     0,   142,     0,
       0,   143,   144,   145,     0,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,     0,
       0,     0,   160,   161,     1,     2,   220,     0,     0,     0,
       0,     0,     0,   162,     0,   163,     0,   164,     0,   165,
       0,     0,     4,     5,     0,     6,     0,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,     0,
       0,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,     0,     0,     0,     0,     0,     0,   114,   115,     0,
       0,     0,     0,     0,     0,     0,   119,   120,     0,     0,
       0,     0,     0,     0,     0,     0,   235,   236,   121,   122,
       0,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
       0,   142,     0,     0,   143,   144,   145,     0,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,     0,     0,     0,     1,     2,   220,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   163,     0,
     164,     0,   165,     4,     5,     0,     6,     0,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
       0,     0,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     0,     0,     0,     0,     0,     0,   114,   115,
       0,     0,     0,     0,     0,     0,     0,   119,   120,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   121,
     122,     0,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,     0,   142,     0,     0,   143,   144,   145,     0,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,     0,     0,     0,     1,     2,   220,     0,
       0,   309,     0,     0,     0,     0,     0,     0,     0,   163,
       0,   164,     0,   165,     4,     5,     0,     6,     0,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,     0,     0,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,     0,     0,     0,     0,     0,     0,   114,
     115,     0,     0,     0,     0,     0,     0,     0,   119,   120,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     121,   122,     0,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,     0,   142,     0,     0,   143,   144,   145,     0,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     163,     0,   164,     0,   165
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-331)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       0,     4,    20,     5,   219,     5,     6,    16,    17,    18,
      19,   147,    13,   113,     7,     8,     9,    10,   118,    14,
      15,   351,   147,   353,    12,   355,   147,    11,   175,     6,
       6,   155,   156,   160,   161,   162,   175,   220,   182,   216,
     220,   224,   220,   201,   224,     5,   224,   201,   304,   305,
     306,   307,   308,   309,   175,   182,   220,   201,   216,   217,
     224,   219,   216,   217,   182,   219,   184,   185,    68,    69,
      70,    74,   216,   217,   207,   219,   209,   224,   216,   217,
     201,   219,   206,   207,   208,   224,   216,   216,   224,   219,
     272,   273,   274,   184,   185,   216,   217,     5,   219,   224,
     149,   150,   317,   275,   276,   277,   216,   278,   279,   280,
     300,   301,   302,     3,   222,     5,   224,     3,     3,     5,
       5,   223,   224,   298,   299,   455,   456,   457,   458,   106,
     106,   225,   226,     5,   135,   225,   226,     5,     5,     5,
     219,   141,   142,   144,   132,   360,   361,     0,   143,   133,
     186,   219,     3,   141,   147,   148,   140,   201,   151,   152,
     153,   154,     3,   163,   164,   165,   159,    23,    25,     6,
     106,   106,   207,    90,     3,     5,   219,   219,   219,     3,
       5,   281,   282,   283,   181,   179,   204,   205,   218,   223,
     182,   222,   218,   201,   201,   216,   209,     5,     5,    90,
       5,   207,     5,     5,     5,   207,   215,   209,     5,   202,
     203,   207,   201,   218,   216,   219,   216,     3,   218,   219,
       5,   220,   225,   106,   106,    90,   201,   201,    15,   220,
       5,     5,   235,   236,   224,     5,   209,   224,   224,   224,
     220,   224,     5,   106,   244,     6,   209,     3,   220,   220,
     220,   251,   220,   209,     5,     3,   209,   209,     3,   297,
     365,     0,    -1,   303,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   317,   318,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   327,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   348,    -1,
     350,    -1,   352,    -1,   354,    -1,    -1,    -1,    -1,    -1,
     360,   361,   362,    -1,   364,    -1,    -1,   367,   368,   369,
      -1,   371,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   406,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   417,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   444,    -1,    -1,    -1,    -1,   449,
     450,   451,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     460,   461,   462,   463,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,    -1,    -1,    -1,    -1,   487,    -1,   492,
     493,    -1,    -1,    -1,    -1,    -1,   496,   497,    21,    22,
      -1,    24,    -1,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    -1,    -1,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,    -1,    -1,    -1,
      -1,   134,    -1,   136,   137,    -1,    -1,    -1,    -1,   142,
      -1,    -1,   145,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   157,   158,    -1,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,    -1,   180,    -1,    -1,
     183,   184,   185,    -1,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,    -1,    -1,
      -1,   204,   205,     3,     4,     5,    -1,    -1,    -1,    -1,
      -1,    -1,   215,    -1,   217,    -1,   219,    -1,   221,   222,
      -1,    21,    22,    -1,    24,    -1,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,    -1,    -1,   134,    -1,   136,   137,   138,   139,
      -1,    -1,   142,    -1,    -1,   145,   146,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   157,   158,    -1,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,    -1,
     180,    -1,    -1,   183,   184,   185,    -1,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,   204,   205,     3,     4,     5,    -1,
      -1,    -1,    -1,    -1,    -1,   215,    -1,   217,    -1,   219,
      -1,   221,    -1,    -1,    21,    22,    -1,    24,    -1,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    -1,    -1,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,    -1,    -1,   134,    -1,   136,
     137,   138,   139,    -1,    -1,   142,    -1,    -1,   145,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     157,   158,    -1,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,    -1,   180,    -1,    -1,   183,   184,   185,    -1,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,    -1,    -1,   204,   205,     3,
       4,     5,    -1,    -1,    -1,    -1,    -1,    -1,   215,    -1,
     217,    -1,   219,    -1,   221,    -1,    -1,    21,    22,    -1,
      24,    -1,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    -1,    -1,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,    -1,    -1,    -1,    -1,
     134,    -1,   136,   137,    -1,    -1,    -1,    -1,   142,    -1,
      -1,   145,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   157,   158,    -1,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,    -1,   180,    -1,    -1,   183,
     184,   185,    -1,   187,   188,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
     204,   205,     3,     4,     5,    -1,    -1,    -1,    -1,    -1,
      -1,   215,   216,   217,    -1,   219,    -1,   221,    -1,    -1,
      21,    22,    -1,    24,    -1,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    -1,    -1,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,    -1,
      -1,    -1,    -1,   134,    -1,   136,   137,    -1,    -1,    -1,
      -1,   142,    -1,    -1,   145,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   157,   158,    -1,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,    -1,   180,
      -1,    -1,   183,   184,   185,    -1,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,   204,   205,     3,     4,     5,    -1,    -1,
      -1,    -1,    -1,    -1,   215,    -1,   217,    -1,   219,    -1,
     221,    -1,    -1,    21,    22,    -1,    24,    -1,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      -1,    -1,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,    -1,    -1,    -1,    -1,   134,    -1,   136,   137,
      -1,    -1,    -1,    -1,   142,    -1,    -1,   145,   146,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   157,
     158,    -1,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,    -1,   180,    -1,    -1,   183,   184,   185,    -1,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,   204,   205,     3,     4,
       5,    -1,    -1,    -1,    -1,    -1,    -1,   215,    -1,   217,
      -1,   219,    -1,   221,    -1,    -1,    21,    22,    -1,    24,
      -1,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    -1,    -1,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,    -1,    -1,    -1,    -1,   134,
      -1,   136,   137,    -1,    -1,    -1,    -1,   142,    -1,    -1,
     145,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   157,   158,    -1,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,    -1,   180,    -1,    -1,   183,   184,
     185,    -1,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,   204,
     205,     3,     4,     5,    -1,    -1,    -1,    -1,    -1,    -1,
     215,    -1,   217,    -1,   219,    -1,   221,    -1,    -1,    21,
      22,    -1,    24,    -1,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    -1,    -1,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,    -1,    -1,
      -1,    -1,    -1,    -1,   136,   137,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   145,   146,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   157,   158,    -1,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,    -1,   180,    -1,
      -1,   183,   184,   185,    -1,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,   204,   205,     3,     4,     5,    -1,    -1,    -1,
      -1,    -1,    -1,   215,    -1,   217,    -1,   219,    -1,   221,
      -1,    -1,    21,    22,    -1,    24,    -1,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    -1,
      -1,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   145,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   155,   156,   157,   158,
      -1,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
      -1,   180,    -1,    -1,   183,   184,   185,    -1,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,    -1,    -1,    -1,     3,     4,     5,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,
     219,    -1,   221,    21,    22,    -1,    24,    -1,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      -1,    -1,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,   146,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   157,
     158,    -1,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,    -1,   180,    -1,    -1,   183,   184,   185,    -1,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,     3,     4,     5,    -1,
      -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,   219,    -1,   221,    21,    22,    -1,    24,    -1,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    -1,    -1,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,    -1,    -1,    -1,    -1,    -1,    -1,   136,
     137,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     157,   158,    -1,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,    -1,   180,    -1,    -1,   183,   184,   185,    -1,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,    -1,   219,    -1,   221
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     3,     4,     5,    21,    22,    24,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   134,   136,   137,   138,   139,   142,   145,
     146,   157,   158,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   180,   183,   184,   185,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     204,   205,   215,   217,   219,   221,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   240,   241,   242,
     243,   244,   245,   247,   248,   249,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   283,   284,   285,   184,   185,   201,   216,   217,   219,
       5,   249,     5,   230,   230,   216,   230,   230,   230,     5,
     207,   209,   216,   216,   216,   155,   156,   249,     5,     5,
     216,   207,   209,   216,   219,     5,     5,   237,     5,     5,
     237,   219,   230,   230,   248,   248,   248,   230,   251,   252,
     256,     3,     5,   230,   250,   253,     5,   222,   230,   254,
     255,     0,    13,   135,   144,    14,    15,   143,    12,   132,
     141,    11,   133,   140,     7,     8,     9,    10,   147,   148,
     151,   152,   153,   154,   159,   202,   203,   239,   149,   150,
      20,   204,   205,   186,   155,   156,   206,   207,   208,   209,
     246,   248,    16,    17,    18,    19,   215,   219,   201,     3,
       3,   230,     3,     5,   230,   256,   216,   217,    23,    25,
     249,     6,   106,   106,   207,    90,     3,     5,   219,   219,
     249,   249,   219,     6,   106,     3,     5,   230,   147,   224,
     147,   224,   175,   224,   175,   224,   230,   179,   181,   218,
     223,   223,   224,   220,   224,   182,   220,   224,   147,   175,
     222,   224,   222,   234,   234,   234,   235,   235,   235,   236,
     236,   236,   237,   237,   237,   240,   241,   241,   242,   242,
     242,   243,   244,   244,   244,   244,   244,   244,   256,   230,
     218,   220,     3,     5,   230,   216,   209,     5,     5,    90,
       5,   207,     5,     5,     5,     5,   207,   219,   220,   230,
     230,   281,   282,   230,   282,   230,   282,   230,   282,   220,
     256,   256,   230,   230,     3,     5,   250,   230,   230,   230,
     230,   220,   218,     3,   249,   106,   106,     5,    90,   201,
     201,    15,     6,   106,   230,   224,   224,   224,   224,   220,
     225,   226,   225,   226,   230,     5,     5,     5,   230,   230,
     230,   209,     5,   220,   282,   282,   282,   282,   230,   230,
     230,   230,     6,   106,   209,   220,   220,   220,     3,   209,
       5,     3,   209,   209,   230,     3,   249,   249,   230,   230
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
        case 2:
/* Line 1792 of yacc.c  */
#line 146 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\calculus.y"
    { printf("ok\n"); }
    break;


/* Line 1792 of yacc.c  */
#line 2541 "calculus_parser.cpp"
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
#line 495 "D:\\sam\\code\\projects\\frameworks\\djinterp\\release\\res\\bison\\calculus.y"


void yyerror(const char *s) {
    fprintf(stderr, "calculus.y: %s near '%s' on line %d\n",
            s, yytext, yylineno);
}

int main(void) {
    return yyparse();
}
