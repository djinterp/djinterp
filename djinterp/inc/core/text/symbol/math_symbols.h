/******************************************************************************
* djinterp [math]                                               math_symbols.h
*
* Mathematical symbols as UTF-8 string literals.
*   Works in plain C99+ without embedding non-ASCII in your source.
*   All symbols are provided as UTF-8 encoded string literals.
*
* Usage:
*   printf("f: " D_SYM_MATH_REALS " " D_SYM_MATH_ARROW_RIGHT " " D_SYM_MATH_REALS "\n");
*   printf("x " D_SYM_MATH_ELEMENT_OF " " D_SYM_MATH_INTEGERS "\n");
*   printf(D_SYM_MATH_INTEGRAL " f(x) dx\n");
*
* Notes:
*   - Output must be interpreted as UTF-8 (most terminals)
*   - On Windows console, run `chcp 65001` or call SetConsoleOutputCP(CP_UTF8)
*
* SYMBOL GROUPS:
*   I.    Basic Arithmetic & Operations
*   II.   Comparison & Equality
*   III.  Set Theory & Logic
*   IV.   Greek Letters (Uppercase)
*   V.    Greek Letters (Lowercase)
*   VI.   Calculus & Analysis
*   VII.  Algebra & Number Theory
*   VIII. Geometry & Vectors
*   IX.   Number Sets
*   X.    Subscripts & Superscripts
*   XI.   Fractions & Numeric
*   XII.  Arrows
*   XIII. Brackets & Delimiters
*   XIV.  Miscellaneous Mathematical
*
* path:      \inc\math\math_symbols.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2024.04.24
******************************************************************************/

#ifndef DJINTERP_MATH_SYMBOLS_
#define DJINTERP_MATH_SYMBOLS_ 1

#include "../../djinterp.h"


// ============================================================================
// I.    BASIC ARITHMETIC & OPERATIONS
// ============================================================================
// fundamental mathematical operators
#define D_SYM_MATH_PLUS                     "\x2B"              // U+002B
#define D_SYM_MATH_MINUS                    "\xE2\x88\x92"      // U+2212 (true minus)
#define D_SYM_MATH_HYPHEN_MINUS             "\x2D"              // U+002D (ASCII hyphen)
#define D_SYM_MATH_TIMES                    "\xC3\x97"          // U+00D7
#define D_SYM_MATH_MULTIPLY_DOT             "\xE2\x8B\x85"      // U+22C5
#define D_SYM_MATH_MULTIPLY_X               "\xE2\x9C\x95"      // U+2715
#define D_SYM_MATH_DIVIDE                   "\xC3\xB7"          // U+00F7
#define D_SYM_MATH_SLASH                    "\xE2\x88\x95"      // U+2215 (division slash)
#define D_SYM_MATH_PLUS_MINUS               "\xC2\xB1"          // U+00B1
#define D_SYM_MATH_MINUS_PLUS               "\xE2\x88\x93"      // U+2213
#define D_SYM_MATH_ASTERISK                 "\xE2\x88\x97"      // U+2217
#define D_SYM_MATH_STAR                     "\xE2\x8B\x86"      // U+22C6
#define D_SYM_MATH_CIRCLE                   "\xE2\x88\x98"      // U+2218 (composition)
#define D_SYM_MATH_BULLET                   "\xE2\x88\x99"      // U+2219
#define D_SYM_MATH_PERCENT                  "\x25"              // U+0025
#define D_SYM_MATH_PERMILLE                 "\xE2\x80\xB0"      // U+2030
#define D_SYM_MATH_PERMYRIAD                "\xE2\x80\xB1"      // U+2031

// roots and powers
#define D_SYM_MATH_SQRT                     "\xE2\x88\x9A"      // U+221A
#define D_SYM_MATH_CBRT                     "\xE2\x88\x9B"      // U+221B
#define D_SYM_MATH_FOURTHRT                 "\xE2\x88\x9C"      // U+221C

// n-ary operators
#define D_SYM_MATH_SUM                      "\xE2\x88\x91"      // U+2211
#define D_SYM_MATH_PRODUCT                  "\xE2\x88\x8F"      // U+220F
#define D_SYM_MATH_COPRODUCT                "\xE2\x88\x90"      // U+2210


// ============================================================================
// II.   COMPARISON & EQUALITY
// ============================================================================
// equality relations
#define D_SYM_MATH_EQUALS                   "\x3D"              // U+003D
#define D_SYM_MATH_NOT_EQUAL                "\xE2\x89\xA0"      // U+2260
#define D_SYM_MATH_APPROX                   "\xE2\x89\x88"      // U+2248
#define D_SYM_MATH_APPROX_EQUAL             "\xE2\x89\x85"      // U+2245
#define D_SYM_MATH_ALMOST_EQUAL             "\xE2\x89\x83"      // U+2243
#define D_SYM_MATH_IDENTICAL                "\xE2\x89\xA1"      // U+2261
#define D_SYM_MATH_NOT_IDENTICAL            "\xE2\x89\xA2"      // U+2262
#define D_SYM_MATH_EQUIV                    "\xE2\x89\x8D"      // U+224D
#define D_SYM_MATH_CORRESPONDS              "\xE2\x89\x99"      // U+2259
#define D_SYM_MATH_DEFINES                  "\xE2\x89\x9C"      // U+225C (delta equal)
#define D_SYM_MATH_COLON_EQUALS             "\xE2\x89\x94"      // U+2254
#define D_SYM_MATH_EQUALS_COLON             "\xE2\x89\x95"      // U+2255
#define D_SYM_MATH_RING_EQUAL               "\xE2\x89\x97"      // U+2257
#define D_SYM_MATH_TILDE                    "\xE2\x88\xBC"      // U+223C
#define D_SYM_MATH_NOT_TILDE                "\xE2\x89\x81"      // U+2241

// ordering relations
#define D_SYM_MATH_LESS                     "\x3C"              // U+003C
#define D_SYM_MATH_GREATER                  "\x3E"              // U+003E
#define D_SYM_MATH_LESS_EQUAL               "\xE2\x89\xA4"      // U+2264
#define D_SYM_MATH_GREATER_EQUAL            "\xE2\x89\xA5"      // U+2265
#define D_SYM_MATH_LESS_LESS                "\xE2\x89\xAA"      // U+226A
#define D_SYM_MATH_GREATER_GREATER          "\xE2\x89\xAB"      // U+226B
#define D_SYM_MATH_NOT_LESS                 "\xE2\x89\xAE"      // U+226E
#define D_SYM_MATH_NOT_GREATER              "\xE2\x89\xAF"      // U+226F
#define D_SYM_MATH_NOT_LESS_EQUAL           "\xE2\x89\xB0"      // U+2270
#define D_SYM_MATH_NOT_GREATER_EQUAL        "\xE2\x89\xB1"      // U+2271
#define D_SYM_MATH_PRECEDES                 "\xE2\x89\xBA"      // U+227A
#define D_SYM_MATH_SUCCEEDS                 "\xE2\x89\xBB"      // U+227B
#define D_SYM_MATH_PRECEDES_EQUAL           "\xE2\x89\xBC"      // U+227C
#define D_SYM_MATH_SUCCEEDS_EQUAL           "\xE2\x89\xBD"      // U+227D

// proportionality
#define D_SYM_MATH_PROPORTIONAL             "\xE2\x88\x9D"      // U+221D


// ============================================================================
// III.  SET THEORY & LOGIC
// ============================================================================
// set membership
#define D_SYM_MATH_ELEMENT_OF               "\xE2\x88\x88"      // U+2208
#define D_SYM_MATH_NOT_ELEMENT_OF           "\xE2\x88\x89"      // U+2209
#define D_SYM_MATH_CONTAINS                 "\xE2\x88\x8B"      // U+220B
#define D_SYM_MATH_NOT_CONTAINS             "\xE2\x88\x8C"      // U+220C
#define D_SYM_MATH_SMALL_ELEMENT_OF         "\xE2\x88\x8A"      // U+220A
#define D_SYM_MATH_SMALL_CONTAINS           "\xE2\x88\x8D"      // U+220D

// set operations
#define D_SYM_MATH_EMPTY_SET                "\xE2\x88\x85"      // U+2205
#define D_SYM_MATH_INTERSECTION             "\xE2\x88\xA9"      // U+2229
#define D_SYM_MATH_UNION                    "\xE2\x88\xAA"      // U+222A
#define D_SYM_MATH_SUBSET                   "\xE2\x8A\x82"      // U+2282
#define D_SYM_MATH_SUPERSET                 "\xE2\x8A\x83"      // U+2283
#define D_SYM_MATH_NOT_SUBSET               "\xE2\x8A\x84"      // U+2284
#define D_SYM_MATH_NOT_SUPERSET             "\xE2\x8A\x85"      // U+2285
#define D_SYM_MATH_SUBSET_EQUAL             "\xE2\x8A\x86"      // U+2286
#define D_SYM_MATH_SUPERSET_EQUAL           "\xE2\x8A\x87"      // U+2287
#define D_SYM_MATH_NOT_SUBSET_EQUAL         "\xE2\x8A\x88"      // U+2288
#define D_SYM_MATH_NOT_SUPERSET_EQUAL       "\xE2\x8A\x89"      // U+2289
#define D_SYM_MATH_PROPER_SUBSET            "\xE2\x8A\x8A"      // U+228A
#define D_SYM_MATH_PROPER_SUPERSET          "\xE2\x8A\x8B"      // U+228B
#define D_SYM_MATH_SET_MINUS                "\xE2\x88\x96"      // U+2216
#define D_SYM_MATH_SYMMETRIC_DIFF           "\xE2\x8A\x96"      // U+2296
#define D_SYM_MATH_COMPLEMENT               "\xE2\x88\x81"      // U+2201

// n-ary set operations
#define D_SYM_MATH_N_ARY_INTERSECTION       "\xE2\x8B\x82"      // U+22C2
#define D_SYM_MATH_N_ARY_UNION              "\xE2\x8B\x83"      // U+22C3

// logical operators
#define D_SYM_MATH_AND                      "\xE2\x88\xA7"      // U+2227
#define D_SYM_MATH_OR                       "\xE2\x88\xA8"      // U+2228
#define D_SYM_MATH_NOT                      "\xC2\xAC"          // U+00AC
#define D_SYM_MATH_XOR                      "\xE2\x8A\xBB"      // U+22BB
#define D_SYM_MATH_NAND                     "\xE2\x8A\xBC"      // U+22BC
#define D_SYM_MATH_NOR                      "\xE2\x8A\xBD"      // U+22BD

// quantifiers
#define D_SYM_MATH_FOR_ALL                  "\xE2\x88\x80"      // U+2200
#define D_SYM_MATH_EXISTS                   "\xE2\x88\x83"      // U+2203
#define D_SYM_MATH_NOT_EXISTS               "\xE2\x88\x84"      // U+2204
#define D_SYM_MATH_UNIQUE_EXISTS            "\xE2\x88\x83!"     // (exists unique)

// logical relations
#define D_SYM_MATH_THEREFORE                "\xE2\x88\xB4"      // U+2234
#define D_SYM_MATH_BECAUSE                  "\xE2\x88\xB5"      // U+2235
#define D_SYM_MATH_PROVES                   "\xE2\x8A\xA2"      // U+22A2
#define D_SYM_MATH_MODELS                   "\xE2\x8A\xA8"      // U+22A8
#define D_SYM_MATH_TRUE                     "\xE2\x8A\xA4"      // U+22A4
#define D_SYM_MATH_FALSE                    "\xE2\x8A\xA5"      // U+22A5
#define D_SYM_MATH_ASSERTION                "\xE2\x8A\xA6"      // U+22A6


// ============================================================================
// IV.   GREEK LETTERS (UPPERCASE)
// ============================================================================
#define D_SYM_MATH_ALPHA_UPPER              "\xCE\x91"          // U+0391
#define D_SYM_MATH_BETA_UPPER               "\xCE\x92"          // U+0392
#define D_SYM_MATH_GAMMA_UPPER              "\xCE\x93"          // U+0393
#define D_SYM_MATH_DELTA_UPPER              "\xCE\x94"          // U+0394
#define D_SYM_MATH_EPSILON_UPPER            "\xCE\x95"          // U+0395
#define D_SYM_MATH_ZETA_UPPER               "\xCE\x96"          // U+0396
#define D_SYM_MATH_ETA_UPPER                "\xCE\x97"          // U+0397
#define D_SYM_MATH_THETA_UPPER              "\xCE\x98"          // U+0398
#define D_SYM_MATH_IOTA_UPPER               "\xCE\x99"          // U+0399
#define D_SYM_MATH_KAPPA_UPPER              "\xCE\x9A"          // U+039A
#define D_SYM_MATH_LAMBDA_UPPER             "\xCE\x9B"          // U+039B
#define D_SYM_MATH_MU_UPPER                 "\xCE\x9C"          // U+039C
#define D_SYM_MATH_NU_UPPER                 "\xCE\x9D"          // U+039D
#define D_SYM_MATH_XI_UPPER                 "\xCE\x9E"          // U+039E
#define D_SYM_MATH_OMICRON_UPPER            "\xCE\x9F"          // U+039F
#define D_SYM_MATH_PI_UPPER                 "\xCE\xA0"          // U+03A0
#define D_SYM_MATH_RHO_UPPER                "\xCE\xA1"          // U+03A1
#define D_SYM_MATH_SIGMA_UPPER              "\xCE\xA3"          // U+03A3
#define D_SYM_MATH_TAU_UPPER                "\xCE\xA4"          // U+03A4
#define D_SYM_MATH_UPSILON_UPPER            "\xCE\xA5"          // U+03A5
#define D_SYM_MATH_PHI_UPPER                "\xCE\xA6"          // U+03A6
#define D_SYM_MATH_CHI_UPPER                "\xCE\xA7"          // U+03A7
#define D_SYM_MATH_PSI_UPPER                "\xCE\xA8"          // U+03A8
#define D_SYM_MATH_OMEGA_UPPER              "\xCE\xA9"          // U+03A9


// ============================================================================
// V.    GREEK LETTERS (LOWERCASE)
// ============================================================================
#define D_SYM_MATH_ALPHA                    "\xCE\xB1"          // U+03B1
#define D_SYM_MATH_BETA                     "\xCE\xB2"          // U+03B2
#define D_SYM_MATH_GAMMA                    "\xCE\xB3"          // U+03B3
#define D_SYM_MATH_DELTA                    "\xCE\xB4"          // U+03B4
#define D_SYM_MATH_EPSILON                  "\xCE\xB5"          // U+03B5
#define D_SYM_MATH_ZETA                     "\xCE\xB6"          // U+03B6
#define D_SYM_MATH_ETA                      "\xCE\xB7"          // U+03B7
#define D_SYM_MATH_THETA                    "\xCE\xB8"          // U+03B8
#define D_SYM_MATH_IOTA                     "\xCE\xB9"          // U+03B9
#define D_SYM_MATH_KAPPA                    "\xCE\xBA"          // U+03BA
#define D_SYM_MATH_LAMBDA                   "\xCE\xBB"          // U+03BB
#define D_SYM_MATH_MU                       "\xCE\xBC"          // U+03BC
#define D_SYM_MATH_NU                       "\xCE\xBD"          // U+03BD
#define D_SYM_MATH_XI                       "\xCE\xBE"          // U+03BE
#define D_SYM_MATH_OMICRON                  "\xCE\xBF"          // U+03BF
#define D_SYM_MATH_PI                       "\xCF\x80"          // U+03C0
#define D_SYM_MATH_RHO                      "\xCF\x81"          // U+03C1
#define D_SYM_MATH_SIGMA_FINAL              "\xCF\x82"          // U+03C2 (final sigma)
#define D_SYM_MATH_SIGMA                    "\xCF\x83"          // U+03C3
#define D_SYM_MATH_TAU                      "\xCF\x84"          // U+03C4
#define D_SYM_MATH_UPSILON                  "\xCF\x85"          // U+03C5
#define D_SYM_MATH_PHI                      "\xCF\x86"          // U+03C6
#define D_SYM_MATH_CHI                      "\xCF\x87"          // U+03C7
#define D_SYM_MATH_PSI                      "\xCF\x88"          // U+03C8
#define D_SYM_MATH_OMEGA                    "\xCF\x89"          // U+03C9

// variant greek letters
#define D_SYM_MATH_VAR_EPSILON              "\xCF\xB5"          // U+03F5
#define D_SYM_MATH_VAR_THETA                "\xCF\x91"          // U+03D1
#define D_SYM_MATH_VAR_KAPPA                "\xCF\xB0"          // U+03F0
#define D_SYM_MATH_VAR_PHI                  "\xCF\x95"          // U+03D5
#define D_SYM_MATH_VAR_RHO                  "\xCF\xB1"          // U+03F1
#define D_SYM_MATH_VAR_PI                   "\xCF\x96"          // U+03D6


// ============================================================================
// VI.   CALCULUS & ANALYSIS
// ============================================================================
// integrals
#define D_SYM_MATH_INTEGRAL                 "\xE2\x88\xAB"      // U+222B
#define D_SYM_MATH_DOUBLE_INTEGRAL          "\xE2\x88\xAC"      // U+222C
#define D_SYM_MATH_TRIPLE_INTEGRAL          "\xE2\x88\xAD"      // U+222D
#define D_SYM_MATH_CONTOUR_INTEGRAL         "\xE2\x88\xAE"      // U+222E
#define D_SYM_MATH_SURFACE_INTEGRAL         "\xE2\x88\xAF"      // U+222F
#define D_SYM_MATH_VOLUME_INTEGRAL          "\xE2\x88\xB0"      // U+2230
#define D_SYM_MATH_CLOCKWISE_INTEGRAL       "\xE2\x88\xB1"      // U+2231
#define D_SYM_MATH_ANTICLOCKWISE_INTEGRAL   "\xE2\x88\xB2"      // U+2232
#define D_SYM_MATH_ANTICLOCKWISE_CONTOUR    "\xE2\x88\xB3"      // U+2233

// differentials
#define D_SYM_MATH_PARTIAL                  "\xE2\x88\x82"      // U+2202
#define D_SYM_MATH_NABLA                    "\xE2\x88\x87"      //  U+2207 (gradient/del)
#define D_SYM_MATH_INCREMENT                "\xE2\x88\x86"      // U+2206 (laplace/increment)
#define D_SYM_MATH_PRIME                    "\xE2\x80\xB2"      // U+2032
#define D_SYM_MATH_DOUBLE_PRIME             "\xE2\x80\xB3"      // U+2033
#define D_SYM_MATH_TRIPLE_PRIME             "\xE2\x80\xB4"      // U+2034
#define D_SYM_MATH_QUADRUPLE_PRIME          "\xE2\x81\x97"      // U+2057
#define D_SYM_MATH_REVERSED_PRIME           "\xE2\x80\xB5"      // U+2035
#define D_SYM_MATH_DERIVATIVE               D_SYM_MATH_PRIME

// limits
#define D_SYM_MATH_INFINITY                 "\xE2\x88\x9E"      // U+221E
#define D_SYM_MATH_ALEPH                    "\xE2\x84\xB5"      // U+2135 (aleph)
#define D_SYM_MATH_BETH                     "\xE2\x84\xB6"      // U+2136 (beth)
#define D_SYM_MATH_GIMEL                    "\xE2\x84\xB7"      // U+2137 (gimel)
#define D_SYM_MATH_DALET                    "\xE2\x84\xB8"      // U+2138 (dalet)


// ============================================================================
// VII.  ALGEBRA & NUMBER THEORY
// ============================================================================
// divisibility
#define D_SYM_MATH_DIVIDES                  "\xE2\x88\xA3"      // U+2223
#define D_SYM_MATH_NOT_DIVIDES              "\xE2\x88\xA4"      // U+2224
#define D_SYM_MATH_PARALLEL                 "\xE2\x88\xA5"      // U+2225
#define D_SYM_MATH_NOT_PARALLEL             "\xE2\x88\xA6"      // U+2226

// modular arithmetic
#define D_SYM_MATH_MODULO                   "\x6D\x6F\x64"      // mod (text)

// group theory
#define D_SYM_MATH_WREATH                   "\xE2\x89\x80"      // U+2240
#define D_SYM_MATH_NORMAL_SUBGROUP          "\xE2\x8A\xB2"      // U+22B2
#define D_SYM_MATH_CONTAINS_NORMAL          "\xE2\x8A\xB3"      // U+22B3
#define D_SYM_MATH_NORMAL_SUBGROUP_EQ       "\xE2\x8A\xB4"      // U+22B4
#define D_SYM_MATH_CONTAINS_NORMAL_EQ       "\xE2\x8A\xB5"      // U+22B5

// abstract algebra
#define D_SYM_MATH_DIRECT_SUM               "\xE2\x8A\x95"      // U+2295
#define D_SYM_MATH_CIRCLED_MINUS            "\xE2\x8A\x96"      // U+2296
#define D_SYM_MATH_TENSOR_PRODUCT           "\xE2\x8A\x97"      // U+2297
#define D_SYM_MATH_CIRCLED_SLASH            "\xE2\x8A\x98"      // U+2298
#define D_SYM_MATH_CIRCLED_DOT              "\xE2\x8A\x99"      // U+2299
#define D_SYM_MATH_CIRCLED_RING             "\xE2\x8A\x9A"      // U+229A
#define D_SYM_MATH_CIRCLED_ASTERISK         "\xE2\x8A\x9B"      // U+229B
#define D_SYM_MATH_CIRCLED_EQUALS           "\xE2\x8A\x9C"      // U+229C
#define D_SYM_MATH_CIRCLED_DASH             "\xE2\x8A\x9D"      // U+229D

// square operators
#define D_SYM_MATH_SQUARED_PLUS             "\xE2\x8A\x9E"      // U+229E
#define D_SYM_MATH_SQUARED_MINUS            "\xE2\x8A\x9F"      // U+229F
#define D_SYM_MATH_SQUARED_TIMES            "\xE2\x8A\xA0"      // U+22A0
#define D_SYM_MATH_SQUARED_DOT              "\xE2\x8A\xA1"      // U+22A1


// ============================================================================
// VIII. GEOMETRY & VECTORS
// ============================================================================
// angles
#define D_SYM_MATH_ANGLE                    "\xE2\x88\xA0"      // U+2220
#define D_SYM_MATH_MEASURED_ANGLE           "\xE2\x88\xA1"      // U+2221
#define D_SYM_MATH_SPHERICAL_ANGLE          "\xE2\x88\xA2"      // U+2222
#define D_SYM_MATH_RIGHT_ANGLE              "\xE2\x88\x9F"      // U+221F
#define D_SYM_MATH_DEGREE                   "\xC2\xB0"          // U+00B0

// perpendicular & parallel
#define D_SYM_MATH_PERPENDICULAR            "\xE2\x9F\x82"      // U+27C2
#define D_SYM_MATH_PERP                     D_SYM_MATH_PERPENDICULAR

// geometric shapes
#define D_SYM_MATH_TRIANGLE                 "\xE2\x96\xB3"      // U+25B3
#define D_SYM_MATH_SQUARE                   "\xE2\x96\xA1"      // U+25A1
#define D_SYM_MATH_CIRCLE_OP                "\xE2\x97\x8B"      // U+25CB
#define D_SYM_MATH_DIAMOND                  "\xE2\x97\x87"      // U+25C7
#define D_SYM_MATH_LOZENGE                  "\xE2\x97\x8A"      // U+25CA

// vector operations
#define D_SYM_MATH_DOT_PRODUCT              D_SYM_MATH_MULTIPLY_DOT // ⋅
#define D_SYM_MATH_CROSS_PRODUCT            "\xC3\x97"          // U+00D7
#define D_SYM_MATH_VECTOR_OR_CROSS          "\xE2\x8B\x80"      // U+22C0 (wedge)
#define D_SYM_MATH_WEDGE                    "\xE2\x88\xA7"      // U+2227
#define D_SYM_MATH_OUTER_PRODUCT            "\xE2\x8A\x97"      // U+2297


// ============================================================================
// IX.   NUMBER SETS
// ============================================================================
// standard number sets (double-struck/blackboard bold)
#define D_SYM_MATH_NATURALS                 "\xE2\x84\x95"      // U+2115
#define D_SYM_MATH_INTEGERS                 "\xE2\x84\xA4"      // U+2124
#define D_SYM_MATH_RATIONALS                "\xE2\x84\x9A"      // U+211A
#define D_SYM_MATH_REALS                    "\xE2\x84\x9D"      // U+211D
#define D_SYM_MATH_COMPLEX                  "\xE2\x84\x82"      // U+2102
#define D_SYM_MATH_PRIMES                   "\xE2\x84\x99"      // U+2119
#define D_SYM_MATH_QUATERNIONS              "\xE2\x84\x8D"      // U+210D

// additional double-struck letters
#define D_SYM_MATH_DBL_A                    "\xF0\x9D\x94\xB8"  // U+1D538
#define D_SYM_MATH_DBL_B                    "\xF0\x9D\x94\xB9"  // U+1D539
#define D_SYM_MATH_DBL_D                    "\xF0\x9D\x94\xBB"  // U+1D53B
#define D_SYM_MATH_DBL_E                    "\xF0\x9D\x94\xBC"  // U+1D53C
#define D_SYM_MATH_DBL_F                    "\xF0\x9D\x94\xBD"  // U+1D53D
#define D_SYM_MATH_DBL_G                    "\xF0\x9D\x94\xBE"  // U+1D53E
#define D_SYM_MATH_DBL_I                    "\xF0\x9D\x95\x80"  // U+1D540
#define D_SYM_MATH_DBL_J                    "\xF0\x9D\x95\x81"  // U+1D541
#define D_SYM_MATH_DBL_K                    "\xF0\x9D\x95\x82"  // U+1D542
#define D_SYM_MATH_DBL_L                    "\xF0\x9D\x95\x83"  // U+1D543
#define D_SYM_MATH_DBL_M                    "\xF0\x9D\x95\x84"  // U+1D544
#define D_SYM_MATH_DBL_O                    "\xF0\x9D\x95\x86"  // U+1D546
#define D_SYM_MATH_DBL_S                    "\xF0\x9D\x95\x8A"  // U+1D54A
#define D_SYM_MATH_DBL_T                    "\xF0\x9D\x95\x8B"  // U+1D54B
#define D_SYM_MATH_DBL_U                    "\xF0\x9D\x95\x8C"  // U+1D54C
#define D_SYM_MATH_DBL_V                    "\xF0\x9D\x95\x8D"  // U+1D54D
#define D_SYM_MATH_DBL_W                    "\xF0\x9D\x95\x8E"  // U+1D54E
#define D_SYM_MATH_DBL_X                    "\xF0\x9D\x95\x8F"  // U+1D54F
#define D_SYM_MATH_DBL_Y                    "\xF0\x9D\x95\x90"  // U+1D550


// ============================================================================
// X.    SUBSCRIPTS & SUPERSCRIPTS
// ============================================================================
// superscript digits
#define D_SYM_MATH_SUP_0                    "\xE2\x81\xB0"      // U+2070
#define D_SYM_MATH_SUP_1                    "\xC2\xB9"          // U+00B9
#define D_SYM_MATH_SUP_2                    "\xC2\xB2"          // U+00B2
#define D_SYM_MATH_SUP_3                    "\xC2\xB3"          // U+00B3
#define D_SYM_MATH_SUP_4                    "\xE2\x81\xB4"      // U+2074
#define D_SYM_MATH_SUP_5                    "\xE2\x81\xB5"      // U+2075
#define D_SYM_MATH_SUP_6                    "\xE2\x81\xB6"      // U+2076
#define D_SYM_MATH_SUP_7                    "\xE2\x81\xB7"      // U+2077
#define D_SYM_MATH_SUP_8                    "\xE2\x81\xB8"      // U+2078
#define D_SYM_MATH_SUP_9                    "\xE2\x81\xB9"      // U+2079

// superscript operators
#define D_SYM_MATH_SUP_PLUS                 "\xE2\x81\xBA"      // U+207A
#define D_SYM_MATH_SUP_MINUS                "\xE2\x81\xBB"      // U+207B
#define D_SYM_MATH_SUP_EQUALS               "\xE2\x81\xBC"      // U+207C
#define D_SYM_MATH_SUP_LPAREN               "\xE2\x81\xBD"      // U+207D
#define D_SYM_MATH_SUP_RPAREN               "\xE2\x81\xBE"      // U+207E
#define D_SYM_MATH_SUP_N                    "\xE2\x81\xBF"      // U+207F
#define D_SYM_MATH_SUP_I                    "\xE2\x81\xB1"      // U+2071

// subscript digits
#define D_SYM_MATH_SUB_0                    "\xE2\x82\x80"      // U+2080
#define D_SYM_MATH_SUB_1                    "\xE2\x82\x81"      // U+2081
#define D_SYM_MATH_SUB_2                    "\xE2\x82\x82"      // U+2082
#define D_SYM_MATH_SUB_3                    "\xE2\x82\x83"      // U+2083
#define D_SYM_MATH_SUB_4                    "\xE2\x82\x84"      // U+2084
#define D_SYM_MATH_SUB_5                    "\xE2\x82\x85"      // U+2085
#define D_SYM_MATH_SUB_6                    "\xE2\x82\x86"      // U+2086
#define D_SYM_MATH_SUB_7                    "\xE2\x82\x87"      // U+2087
#define D_SYM_MATH_SUB_8                    "\xE2\x82\x88"      // U+2088
#define D_SYM_MATH_SUB_9                    "\xE2\x82\x89"      // U+2089

// subscript operators
#define D_SYM_MATH_SUB_PLUS                 "\xE2\x82\x8A"      // U+208A
#define D_SYM_MATH_SUB_MINUS                "\xE2\x82\x8B"      // U+208B
#define D_SYM_MATH_SUB_EQUALS               "\xE2\x82\x8C"      // U+208C
#define D_SYM_MATH_SUB_LPAREN               "\xE2\x82\x8D"      // U+208D
#define D_SYM_MATH_SUB_RPAREN               "\xE2\x82\x8E"      // U+208E

// subscript letters
#define D_SYM_MATH_SUB_A                    "\xE2\x82\x90"      // U+2090
#define D_SYM_MATH_SUB_E                    "\xE2\x82\x91"      // U+2091
#define D_SYM_MATH_SUB_H                    "\xE2\x82\x95"      // U+2095
#define D_SYM_MATH_SUB_I                    "\xE1\xB5\xA2"      // U+1D62
#define D_SYM_MATH_SUB_J                    "\xE2\xB1\xBC"      // U+2C7C
#define D_SYM_MATH_SUB_K                    "\xE2\x82\x96"      // U+2096
#define D_SYM_MATH_SUB_L                    "\xE2\x82\x97"      // U+2097
#define D_SYM_MATH_SUB_M                    "\xE2\x82\x98"      // U+2098
#define D_SYM_MATH_SUB_N                    "\xE2\x82\x99"      // U+2099
#define D_SYM_MATH_SUB_O                    "\xE2\x82\x92"      // U+2092
#define D_SYM_MATH_SUB_P                    "\xE2\x82\x9A"      // U+209A
#define D_SYM_MATH_SUB_R                    "\xE1\xB5\xA3"      // U+1D63
#define D_SYM_MATH_SUB_S                    "\xE2\x82\x9B"      // U+209B
#define D_SYM_MATH_SUB_T                    "\xE2\x82\x9C"      // U+209C
#define D_SYM_MATH_SUB_U                    "\xE1\xB5\xA4"      // U+1D64
#define D_SYM_MATH_SUB_V                    "\xE1\xB5\xA5"      // U+1D65
#define D_SYM_MATH_SUB_X                    "\xE2\x82\x93"      // U+2093


// ============================================================================
// XI.   FRACTIONS & NUMERIC
// ============================================================================
// vulgar fractions
#define D_SYM_MATH_FRAC_1_4                 "\xC2\xBC"          // U+00BC
#define D_SYM_MATH_FRAC_1_2                 "\xC2\xBD"          // U+00BD
#define D_SYM_MATH_FRAC_3_4                 "\xC2\xBE"          // U+00BE
#define D_SYM_MATH_FRAC_1_7                 "\xE2\x85\x90"      // U+2150
#define D_SYM_MATH_FRAC_1_9                 "\xE2\x85\x91"      // U+2151
#define D_SYM_MATH_FRAC_1_10                "\xE2\x85\x92"      // U+2152
#define D_SYM_MATH_FRAC_1_3                 "\xE2\x85\x93"      // U+2153
#define D_SYM_MATH_FRAC_2_3                 "\xE2\x85\x94"      // U+2154
#define D_SYM_MATH_FRAC_1_5                 "\xE2\x85\x95"      // U+2155
#define D_SYM_MATH_FRAC_2_5                 "\xE2\x85\x96"      // U+2156
#define D_SYM_MATH_FRAC_3_5                 "\xE2\x85\x97"      // U+2157
#define D_SYM_MATH_FRAC_4_5                 "\xE2\x85\x98"      // U+2158
#define D_SYM_MATH_FRAC_1_6                 "\xE2\x85\x99"      // U+2159
#define D_SYM_MATH_FRAC_5_6                 "\xE2\x85\x9A"      // U+215A
#define D_SYM_MATH_FRAC_1_8                 "\xE2\x85\x9B"      // U+215B
#define D_SYM_MATH_FRAC_3_8                 "\xE2\x85\x9C"      // U+215C
#define D_SYM_MATH_FRAC_5_8                 "\xE2\x85\x9D"      // U+215D
#define D_SYM_MATH_FRAC_7_8                 "\xE2\x85\x9E"      // U+215E
#define D_SYM_MATH_FRAC_0_3                 "\xE2\x86\x89"      // U+2189

// fraction slash
#define D_SYM_MATH_FRACTION_SLASH           "\xE2\x81\x84"      // U+2044


// ============================================================================
// XII.  ARROWS
// ============================================================================
// basic arrows
#define D_SYM_MATH_ARROW_LEFT               "\xE2\x86\x90"      // U+2190
#define D_SYM_MATH_ARROW_UP                 "\xE2\x86\x91"      // U+2191
#define D_SYM_MATH_ARROW_RIGHT              "\xE2\x86\x92"      // U+2192
#define D_SYM_MATH_ARROW_DOWN               "\xE2\x86\x93"      // U+2193
#define D_SYM_MATH_ARROW_LEFT_RIGHT         "\xE2\x86\x94"      // U+2194
#define D_SYM_MATH_ARROW_UP_DOWN            "\xE2\x86\x95"      // U+2195

// diagonal arrows
#define D_SYM_MATH_ARROW_NW                 "\xE2\x86\x96"      // U+2196
#define D_SYM_MATH_ARROW_NE                 "\xE2\x86\x97"      // U+2197
#define D_SYM_MATH_ARROW_SE                 "\xE2\x86\x98"      // U+2198
#define D_SYM_MATH_ARROW_SW                 "\xE2\x86\x99"      // U+2199

// double arrows (implications)
#define D_SYM_MATH_DOUBLE_ARROW_LEFT        "\xE2\x87\x90"      // U+21D0
#define D_SYM_MATH_DOUBLE_ARROW_UP          "\xE2\x87\x91"      // U+21D1
#define D_SYM_MATH_DOUBLE_ARROW_RIGHT       "\xE2\x87\x92"      // U+21D2
#define D_SYM_MATH_DOUBLE_ARROW_DOWN        "\xE2\x87\x93"      // U+21D3
#define D_SYM_MATH_DOUBLE_ARROW_LEFT_RIGHT  "\xE2\x87\x94"      // U+21D4
#define D_SYM_MATH_DOUBLE_ARROW_UP_DOWN     "\xE2\x87\x95"      // U+21D5

// logical arrows (aliases)
#define D_SYM_MATH_IMPLIES                  D_SYM_MATH_DOUBLE_ARROW_RIGHT  //
#define D_SYM_MATH_IMPLIED_BY               D_SYM_MATH_DOUBLE_ARROW_LEFT   //
#define D_SYM_MATH_IFF                      D_SYM_MATH_DOUBLE_ARROW_LEFT_RIGHT  //
#define D_SYM_MATH_MAPS_TO                  "\xE2\x86\xA6"      // U+21A6
#define D_SYM_MATH_MAPS_FROM                "\xE2\x86\xA4"      // U+21A4

// long arrows
#define D_SYM_MATH_LONG_ARROW_LEFT          "\xE2\x9F\xB5"      // U+27F5
#define D_SYM_MATH_LONG_ARROW_RIGHT         "\xE2\x9F\xB6"      // U+27F6
#define D_SYM_MATH_LONG_ARROW_LEFT_RIGHT    "\xE2\x9F\xB7"      // U+27F7
#define D_SYM_MATH_LONG_DOUBLE_LEFT         "\xE2\x9F\xB8"      // U+27F8
#define D_SYM_MATH_LONG_DOUBLE_RIGHT        "\xE2\x9F\xB9"      // U+27F9
#define D_SYM_MATH_LONG_DOUBLE_LEFT_RIGHT   "\xE2\x9F\xBA"      // U+27FA

// hooked and barbed arrows
#define D_SYM_MATH_ARROW_HOOK_LEFT          "\xE2\x86\xA9"      // U+21A9
#define D_SYM_MATH_ARROW_HOOK_RIGHT         "\xE2\x86\xAA"      // U+21AA
#define D_SYM_MATH_ARROW_TAIL_LEFT          "\xE2\x86\xA2"      // U+21A2
#define D_SYM_MATH_ARROW_TAIL_RIGHT         "\xE2\x86\xA3"      // U+21A3
#define D_SYM_MATH_ARROW_TWO_HEAD_LEFT      "\xE2\x86\x9E"      // U+219E
#define D_SYM_MATH_ARROW_TWO_HEAD_RIGHT     "\xE2\x86\xA0"      // U+21A0


// ============================================================================
// XIII. BRACKETS & DELIMITERS
// ============================================================================
// angle brackets
#define D_SYM_MATH_LANGLE                   "\xE2\x9F\xA8"      // U+27E8
#define D_SYM_MATH_RANGLE                   "\xE2\x9F\xA9"      // U+27E9
#define D_SYM_MATH_DOUBLE_LANGLE            "\xE2\x9F\xAA"      // U+27EA
#define D_SYM_MATH_DOUBLE_RANGLE            "\xE2\x9F\xAB"      // U+27EB
																   
// floor and ceiling											   
#define D_SYM_MATH_LFLOOR                   "\xE2\x8C\x8A"      // U+230A
#define D_SYM_MATH_RFLOOR                   "\xE2\x8C\x8B"      // U+230B
#define D_SYM_MATH_LCEIL                    "\xE2\x8C\x88"      // U+2308
#define D_SYM_MATH_RCEIL                    "\xE2\x8C\x89"      // U+2309
																   
// double brackets												   
#define D_SYM_MATH_LBRACKET_DBL             "\xE2\x9F\xA6"      // U+27E6
#define D_SYM_MATH_RBRACKET_DBL             "\xE2\x9F\xA7"      // U+27E7
																   
// vertical bars												   
#define D_SYM_MATH_VERT                     "\x7C"              // U+007C
#define D_SYM_MATH_DOUBLE_VERT              "\xE2\x80\x96"      // U+2016
#define D_SYM_MATH_TRIPLE_VERT              "\xE2\x81\x9D"      // U+205D

// norms (aliases)
#define D_SYM_MATH_NORM                     D_SYM_MATH_DOUBLE_VERT  //
#define D_SYM_MATH_ABS                      D_SYM_MATH_VERT         //


// ============================================================================
// XIV.  MISCELLANEOUS MATHEMATICAL
// ============================================================================
// dots
#define D_SYM_MATH_CDOT                     "\xE2\x8B\x85"      // U+22C5
#define D_SYM_MATH_CDOTS                    "\xE2\x8B\xAF"      // U+22EF
#define D_SYM_MATH_VDOTS                    "\xE2\x8B\xAE"      // U+22EE
#define D_SYM_MATH_DDOTS                    "\xE2\x8B\xB1"      // U+22F1
#define D_SYM_MATH_IDDOTS                   "\xE2\x8B\xB0"      // U+22F0
#define D_SYM_MATH_LDOTS                    "\xE2\x80\xA6"      // U+2026
#define D_SYM_MATH_ELLIPSIS                 D_SYM_MATH_LDOTS

// function notation
#define D_SYM_MATH_FUNCTION                 "\xC6\x92"          // U+0192
#define D_SYM_MATH_SCRIPT_L                 "\xE2\x84\x93"      // U+2113 (script l, liter)
#define D_SYM_MATH_SCRIPT_E                 "\xE2\x84\xAF"      // U+212F (Euler's e)
#define D_SYM_MATH_SCRIPT_G                 "\xE2\x84\x8A"      // U+210A
#define D_SYM_MATH_SCRIPT_H                 "\xE2\x84\x8B"      // U+210B (Hamiltonian)
#define D_SYM_MATH_SCRIPT_I                 "\xE2\x84\x90"      // U+2110
#define D_SYM_MATH_SCRIPT_L_UPPER           "\xE2\x84\x92"      // U+2112 (Laplace)
#define D_SYM_MATH_SCRIPT_R                 "\xE2\x84\x9B"      // U+211B
#define D_SYM_MATH_WEIERSTRASS_P            "\xE2\x84\x98"      // U+2118

// miscellaneous operators
#define D_SYM_MATH_THERE4                   "\xE2\x88\xB4"      // U+2234 (therefore)
#define D_SYM_MATH_BECAUSE_SYM              "\xE2\x88\xB5"      // U+2235
#define D_SYM_MATH_RATIO                    "\xE2\x88\xB6"      // U+2236
#define D_SYM_MATH_PROPORTION               "\xE2\x88\xB7"      // U+2237
#define D_SYM_MATH_DOT_MINUS                "\xE2\x88\xB8"      // U+2238
#define D_SYM_MATH_EXCESS                   "\xE2\x88\xB9"      // U+2239
#define D_SYM_MATH_GEOMETRIC_PROPORTION     "\xE2\x88\xBA"      // U+223A
#define D_SYM_MATH_HOMOTHETIC               "\xE2\x88\xBB"      // U+223B

// miscellaneous symbols
#define D_SYM_MATH_QED                      "\xE2\x88\x8E"      // U+220E
#define D_SYM_MATH_END_OF_PROOF             D_SYM_MATH_QED
#define D_SYM_MATH_ASTERISM                 "\xE2\x81\x82"      // U+2042
#define D_SYM_MATH_REFERENCE_MARK           "\xE2\x80\xBB"      // U+203B


#endif  // DJINTERP_MATH_SYMBOLS_
