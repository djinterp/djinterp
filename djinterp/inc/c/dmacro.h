/******************************************************************************
* djinterp [core]                                                     dmacro.h
*
*   Comprehensive macro utilities for the djinterp framework, providing 
* token manipulation, variadic argument processing, iteration, X-macro 
* functionality, and code generation utilities.
*   This module contains fundamental macros used throughout the djinterp 
* framework for metaprogramming, template-like functionality in C, and 
* compile-time code generation.
*
* 
* path:      \inc\dmacro.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.23
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    CONFIGURATION SYSTEM
      --------------------
      1.  User-Overridable Settings
          a.  D_CFG_DMACRO_VARG_DEFAULT   - default variadic limit (128)
          b.  D_CFG_DMACRO_VARG_MIN       - minimum allowed (64)
          c.  D_CFG_DMACRO_VARG_LIMIT     - maximum allowed (1024)
      2.  Variant Selection Constants
          a.  D_CFG_DMACRO_VARIANT_64 through D_CFG_DMACRO_VARIANT_1024
      3.  Effective Configuration Values
          a.  D_DMACRO_VARG_MAX, D_DMACRO_PAIR_MAX, D_DMACRO_TRIPLE_MAX
          b.  D_DMACRO_4TUPLE_MAX, D_DMACRO_VARIANT
      4.  Public Aliases
          a.  D_CFG_VARG_COUNT_MAX, D_VARG_COUNT_MAX
          b.  D_CFG_FOR_EACH_MAX, D_FOR_EACH_MAX
          c.  D_CFG_FOR_EACH_PAIR_MAX, D_FOR_EACH_PAIR_MAX
          d.  D_CFG_FOR_EACH_TRIPLE_MAX, D_FOR_EACH_TRIPLE_MAX
          e.  D_CFG_FOR_EACH_4TUPLE_MAX, D_FOR_EACH_4TUPLE_MAX
          f.  D_CFG_MACRO_VARIANT, D_MACRO_VARIANT
          g.  D_VARG_PAIR_MAX
      5.  Query Macros
          a.  D_DMACRO_IS_OVERRIDE_ENABLED
          b.  D_DMACRO_IS_MSVC_COMPAT
      6.  Limit Checking Macros
          a.  D_DMACRO_CHECK_VARG_LIMIT
          b.  D_DMACRO_CHECK_PAIR_LIMIT
          c.  D_DMACRO_CHECK_TRIPLE_LIMIT
          d.  D_DMACRO_CHECK_4TUPLE_LIMIT

I.    BASIC TOKEN MANIPULATION
      ------------------------
      1.  Token Pasting
          a.  D_INTERNAL_CONCAT_HELPER
          b.  D_CONCAT
      2.  Stringification
          a.  D_STRINGIFY
          b.  D_TOSTR
      3.  Expansion Control
          a.  D_EXPAND
          b.  D_EMPTY
          c.  D_DEFER
          d.  D_OBSTRUCT
          e.  D_UNPACK
      4.  Separator Tokens
          a.  D_SEPARATOR_COMMA
          b.  D_SEPARATOR_SEMICOLON
          c.  D_SEPARATOR_SPACE

II.   ARRAY UTILITIES
      ---------------
      1.  Compile-Time Array Sizing
          a.  D_ARRAY_COUNT
          b.  D_ARRAY_COUNT_SAFE
          c.  D_ARRAY_COUNT_T
      2.  Array Generation
          a.  D_MAKE_ARRAY
          b.  D_MAKE_STRING_ARRAY

III.  ARGUMENT SELECTION
      ------------------
      1.  Positional Accessors
          a.  D_VARG_GET_FIRST through D_VARG_GET_TENTH
      2.  List Operations
          a.  D_HEAD
          b.  D_REST
          c.  D_TAIL
      3.  Dynamic Position Access
          a.  D_INTERNAL_VARG_GET_N
          b.  D_VARG_LAST
      4.  Parentheses Handling
          a.  D_VARGS_REMOVE_PARENTHESES_IMPL
          b.  D_VARGS_REMOVE_PARENTHESES

IV.   MACRO EXPANSION AND EVALUATION
      ------------------------------
      1.  Cascading Expansion Helpers
          a.  D_INTERNAL_EVAL_0001 through D_INTERNAL_EVAL_1024
      2.  Public Evaluation Macros
          a.  D_EVAL

V.    BOOLEAN AND CONDITIONAL LOGIC
      -----------------------------
      1.  Probe Mechanism
          a.  D_PROBE
          b.  D_CHECK_IMPL
          c.  D_CHECK
      2.  Parentheses Detection
          a.  D_IS_PAREN_PROBE
          b.  D_IS_PAREN_WRAPPER
          c.  D_IS_PAREN
      3.  Meta Wrapper
          a.  D_META
      4.  Conditional Expansion
          a.  D_IF, D_IF_0, D_IF_1
          b.  D_IF_0_ELSE, D_IF_1_ELSE
      5.  Immediate If
          a.  D_IIF, D_IIF_0, D_IIF_1
      6.  Boolean Negation
          a.  D_NOT, D_NOT_0
          b.  D_COMPL, D_COMPL_0, D_COMPL_1
      7.  Boolean Normalization
          a.  D_BOOL
      8.  Logical Operations
          a.  D_AND, D_AND_00, D_AND_01, D_AND_10, D_AND_11
          b.  D_OR,  D_OR_00,  D_OR_01,  D_OR_10,  D_OR_11

VI.   INCREMENT MACRO
      ---------------
      1.  D_INC

VII.  CORE ITERATION INFRASTRUCTURE
      -----------------------------
      1.  Map Termination
          a.  D_INTERNAL_MAP_END
          b.  D_INTERNAL_MAP_OUT
          c.  D_INTERNAL_MAP_GET_END
          d.  D_INTERNAL_MAP_NEXT0, D_INTERNAL_MAP_NEXT1, D_INTERNAL_MAP_NEXT
      2.  Data-Passing Map Helpers
          a.  D_INTERNAL_MAP_DATA0, D_INTERNAL_MAP_DATA1
      3.  Indexed Map Helpers
          a.  D_INTERNAL_MAP_IDX0, D_INTERNAL_MAP_IDX1

VIII. FOR_EACH IMPLEMENTATIONS
      ------------------------
      1.  Basic Iteration
          a.  D_FOR_EACH              - fn(a)  fn(b)  fn(c)
          b.  D_FOR_EACH_COMMA        - fn(a), fn(b), fn(c)
          c.  D_FOR_EACH_SEP          - fn(a) sep fn(b) sep fn(c)
          d.  D_FOR_EACH_SEMICOLON    - fn(a); fn(b); fn(c);
          e.  D_FOR_EACH_SPACE        - fn(a)  fn(b)  fn(c)
      2.  Pair Iteration (2-tuples)
          a.  D_FOR_EACH_PAIR         - fn(a,b) fn(c,d)
          b.  D_FOR_EACH_PAIR_COMMA   - fn(a,b), fn(c,d)
          c.  D_FOR_EACH_PAIR_SEP     - fn(a,b) sep fn(c,d)
      3.  Triple Iteration (3-tuples)
          a.  D_FOR_EACH_TRIPLE       - fn(a,b,c)  fn(d,e,f)
          b.  D_FOR_EACH_TRIPLE_COMMA - fn(a,b,c), fn(d,e,f)
          c.  D_FOR_EACH_TRIPLE_SEP   - fn(a,b,c) sep fn(d,e,f)
      4.  4-Tuple Iteration
          a.  D_FOR_EACH_4TUPLE       - fn(a,b,c,d) fn(e,f,g,h)
          b.  D_FOR_EACH_4TUPLE_COMMA - fn(a,b,c,d), fn(e,f,g,h)
          c.  D_FOR_EACH_4TUPLE_SEP   - fn(a,b,c,d) sep fn(e,f,g,h)
      5.  N-Tuple Generic Interface
          a.  D_FOR_EACH_NTUPLE
          b.  D_FOR_EACH_NTUPLE_COMMA
          c.  D_FOR_EACH_NTUPLE_SEP
      6.  Tuple Aliases
          a.  D_FOR_EACH_1TUPLE, D_FOR_EACH_2TUPLE, D_FOR_EACH_3TUPLE

IX.   MEMBER ACCESS ITERATION
      -----------------------
      1.  Pointer Member Access (->)
          a.  D_INTERNAL_MEMBER_PTR_OP
          b.  D_FOR_EACH_MEMBER_PTR
          c.  D_INTERNAL_MEMBER_PTR_OP_EXPAND
      2.  Direct Member Access (.)
          a.  D_INTERNAL_MEMBER_DOT_OP
          b.  D_FOR_EACH_MEMBER_DOT
          c.  D_INTERNAL_MEMBER_DOT_OP_EXPAND

X.    ADVANCED ITERATION PATTERNS
      ---------------------------
      1.  Adjacent Pair Iteration
          a.  D_INTERNAL_MAP_ADJ0, D_INTERNAL_MAP_ADJ1
          b.  D_FOR_EACH_ADJACENT_PAIR

XI.   POINTER ARRAY INITIALIZATION
      ----------------------------
      1.  Data-Comma Iteration
          a.  D_INTERNAL_MAP_DATA_COMMA0, D_INTERNAL_MAP_DATA_COMMA1
          b.  D_FOR_EACH_DATA_COMMA
      2.  Struct Array Helpers
          a.  D_INTERNAL_PTR_ELEM
          b.  D_INTERNAL_TUPLE_TO_BRACES
          c.  D_STRUCT_ARRAY_INIT

XII.  UTILITY OPERATORS
      -----------------
      1.  Debug/Test Operators
          a.  D_PRINT_OP
          b.  D_PRINT_VAL_OP
          c.  D_DECLARE_VAR_OP
          d.  D_DECLARE_TYPED_OP
          e.  D_ASSIGN_OP
          f.  D_INIT_ZERO_OP

XIII. COMPILE-TIME ASSERTIONS
      -----------------------
      1.  Size/Type Checks
          a.  D_ASSERT_SAME_SIZE
*/


#ifndef DJINTERP_MACRO_
#define DJINTERP_MACRO_ 1

#include <stddef.h>
#include ".\env.h"


// =============================================================================
// 0.   CONFIGURATION SYSTEM
// =============================================================================
//
// Controls variadic argument limits, macro variant selection, and provides
// user-overridable settings for maximum flexibility.
//
// CONFIGURATION HIERARCHY (highest to lowest priority):
//   1. D_CFG_DMACRO_OVERRIDE - if 1, use D_CFG_DMACRO_* values directly
//   2. D_CFG_DMACRO_VARG_MAX - user-specified max (if override enabled)
//   3. D_ENV_PP_MAX_MACRO_ARGS - environment-detected limit
//   4. D_CFG_DMACRO_VARG_DEFAULT (128) - fallback default
//
// -----------------------------------------------------------------------------

// --- 0.1  Configuration Constants ---

// D_CFG_DMACRO_VARG_DEFAULT
//   brief: default maximum variadic argument count (128)
#define D_CFG_DMACRO_VARG_DEFAULT 128

// D_CFG_DMACRO_VARG_MIN
//   brief: minimum supported variadic argument count (64)
#define D_CFG_DMACRO_VARG_MIN     64

// D_CFG_DMACRO_VARG_LIMIT
//   brief: absolute maximum supported by the framework (1024)
#define D_CFG_DMACRO_VARG_LIMIT   1024

// supported variant levels (must match available *N.h files)
#define D_CFG_DMACRO_VARIANT_64   64
#define D_CFG_DMACRO_VARIANT_128  128
#define D_CFG_DMACRO_VARIANT_256  256
#define D_CFG_DMACRO_VARIANT_512  512
#define D_CFG_DMACRO_VARIANT_1024 1024


// --- 0.2  User Configuration Options ---

// D_CFG_DMACRO_OVERRIDE
//   brief: master override flag for dmacro configuration
#ifndef D_CFG_DMACRO_OVERRIDE
    #define D_CFG_DMACRO_OVERRIDE 0
#endif

// D_CFG_DMACRO_VARG_MAX
//   brief: user-specified maximum variadic argument count
#ifndef D_CFG_DMACRO_VARG_MAX
    #define D_CFG_DMACRO_VARG_MAX D_CFG_DMACRO_VARG_DEFAULT
#endif

// D_CFG_DMACRO_USE_MSVC_COMPAT
//   brief: enable MSVC-compatible limits (127 instead of 128, etc.)
#ifndef D_CFG_DMACRO_USE_MSVC_COMPAT
    #if ( defined(_MSC_VER) && !defined(__clang__) )
        #if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
            #define D_CFG_DMACRO_USE_MSVC_COMPAT 1
        #else
            #define D_CFG_DMACRO_USE_MSVC_COMPAT 0
        #endif
    #else
        #define D_CFG_DMACRO_USE_MSVC_COMPAT 0
    #endif
#endif


// --- 0.3  Effective Value Calculation ---

// step 1: determine raw max value
#if (D_CFG_DMACRO_OVERRIDE == 1)
    #define D_INTERNAL_DMACRO_RAW_MAX D_CFG_DMACRO_VARG_MAX

#elif defined(D_ENV_PP_MAX_MACRO_ARGS)
    #if (D_ENV_PP_MAX_MACRO_ARGS > D_CFG_DMACRO_VARG_MAX)
        #define D_INTERNAL_DMACRO_RAW_MAX D_CFG_DMACRO_VARG_MAX
    #else
        #define D_INTERNAL_DMACRO_RAW_MAX D_ENV_PP_MAX_MACRO_ARGS
    #endif

#else
    #define D_INTERNAL_DMACRO_RAW_MAX D_CFG_DMACRO_VARG_DEFAULT
#endif

// step 2: clamp to supported range
#if (D_INTERNAL_DMACRO_RAW_MAX < D_CFG_DMACRO_VARG_MIN)
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_CFG_DMACRO_VARG_MIN
#elif (D_INTERNAL_DMACRO_RAW_MAX > D_CFG_DMACRO_VARG_LIMIT)
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_CFG_DMACRO_VARG_LIMIT
#else
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_INTERNAL_DMACRO_RAW_MAX
#endif

// step 3: MSVC traditional preprocessor compatibility
#if (D_CFG_DMACRO_USE_MSVC_COMPAT == 1)
    #define D_DMACRO_VARIANT      127
    #define D_DMACRO_VARG_MAX     127
    #define D_DMACRO_PAIR_MAX     63
    #define D_DMACRO_TRIPLE_MAX   42
    #define D_DMACRO_4TUPLE_MAX   31

// step 4: round up to nearest supported variant (non-MSVC path)
#elif (D_INTERNAL_DMACRO_CLAMPED_MAX <= 64)
    #define D_DMACRO_VARIANT      D_CFG_DMACRO_VARIANT_64
    #define D_DMACRO_VARG_MAX     64
    #define D_DMACRO_PAIR_MAX     32
    #define D_DMACRO_TRIPLE_MAX   21
    #define D_DMACRO_4TUPLE_MAX   16
#elif (D_INTERNAL_DMACRO_CLAMPED_MAX <= 128)
    #define D_DMACRO_VARIANT      D_CFG_DMACRO_VARIANT_128
    #define D_DMACRO_VARG_MAX     128
    #define D_DMACRO_PAIR_MAX     64
    #define D_DMACRO_TRIPLE_MAX   42
    #define D_DMACRO_4TUPLE_MAX   32
#elif (D_INTERNAL_DMACRO_CLAMPED_MAX <= 256)
    #define D_DMACRO_VARIANT      D_CFG_DMACRO_VARIANT_256
    #define D_DMACRO_VARG_MAX     256
    #define D_DMACRO_PAIR_MAX     128
    #define D_DMACRO_TRIPLE_MAX   85
    #define D_DMACRO_4TUPLE_MAX   64
#elif (D_INTERNAL_DMACRO_CLAMPED_MAX <= 512)
    #define D_DMACRO_VARIANT      D_CFG_DMACRO_VARIANT_512
    #define D_DMACRO_VARG_MAX     512
    #define D_DMACRO_PAIR_MAX     256
    #define D_DMACRO_TRIPLE_MAX   170
    #define D_DMACRO_4TUPLE_MAX   128
#else
    #define D_DMACRO_VARIANT      D_CFG_DMACRO_VARIANT_1024
    #define D_DMACRO_VARG_MAX     1024
    #define D_DMACRO_PAIR_MAX     512
    #define D_DMACRO_TRIPLE_MAX   341
    #define D_DMACRO_4TUPLE_MAX   256
#endif


// --- 0.4  Public Configuration Macros ---

// D_CFG_VARG_COUNT_MAX / D_VARG_COUNT_MAX
//   brief: maximum number of variadic arguments supported
#define D_CFG_VARG_COUNT_MAX D_DMACRO_VARG_MAX
#define D_VARG_COUNT_MAX     D_DMACRO_VARG_MAX

// D_CFG_FOR_EACH_MAX / D_FOR_EACH_MAX
//   brief: maximum elements D_FOR_EACH can iterate over
#define D_CFG_FOR_EACH_MAX D_DMACRO_VARG_MAX
#define D_FOR_EACH_MAX     D_DMACRO_VARG_MAX

// D_CFG_FOR_EACH_PAIR_MAX / D_FOR_EACH_PAIR_MAX
//   brief: maximum pairs D_FOR_EACH_PAIR can iterate over
#define D_CFG_FOR_EACH_PAIR_MAX D_DMACRO_PAIR_MAX
#define D_FOR_EACH_PAIR_MAX     D_DMACRO_PAIR_MAX

// D_CFG_FOR_EACH_TRIPLE_MAX / D_FOR_EACH_TRIPLE_MAX
//   brief: maximum triples D_FOR_EACH_TRIPLE can iterate over
#define D_CFG_FOR_EACH_TRIPLE_MAX D_DMACRO_TRIPLE_MAX
#define D_FOR_EACH_TRIPLE_MAX     D_DMACRO_TRIPLE_MAX

// D_CFG_FOR_EACH_4TUPLE_MAX / D_FOR_EACH_4TUPLE_MAX
//   brief: maximum 4-tuples D_FOR_EACH_4TUPLE can iterate over
#define D_CFG_FOR_EACH_4TUPLE_MAX D_DMACRO_4TUPLE_MAX
#define D_FOR_EACH_4TUPLE_MAX     D_DMACRO_4TUPLE_MAX

// D_CFG_MACRO_VARIANT / D_MACRO_VARIANT
//   brief: identifies which variant of auto-generated headers is in use
#define D_CFG_MACRO_VARIANT D_DMACRO_VARIANT
#define D_MACRO_VARIANT     D_DMACRO_VARIANT

// D_VARG_PAIR_MAX
//   brief: alias for D_DMACRO_PAIR_MAX (legacy compatibility)
#define D_VARG_PAIR_MAX D_DMACRO_PAIR_MAX


// --- 0.5  Configuration Query Macros ---

// D_DMACRO_IS_OVERRIDE_ENABLED
//   brief: returns 1 if override mode is active, 0 otherwise
#define D_DMACRO_IS_OVERRIDE_ENABLED() (D_CFG_DMACRO_OVERRIDE == 1)

// D_DMACRO_IS_MSVC_COMPAT
//   brief: returns 1 if MSVC compatibility mode is active, 0 otherwise
#define D_DMACRO_IS_MSVC_COMPAT() (D_CFG_DMACRO_USE_MSVC_COMPAT == 1)

// D_DMACRO_USES_ENV_DETECTION
//   brief: returns 1 if environment detection is being used, 0 otherwise
#if (D_CFG_DMACRO_OVERRIDE == 1)
    #define D_DMACRO_USES_ENV_DETECTION() 0
#elif defined(D_ENV_PP_MAX_MACRO_ARGS)
    #define D_DMACRO_USES_ENV_DETECTION() 1
#else
    #define D_DMACRO_USES_ENV_DETECTION() 0
#endif

// D_DMACRO_CHECK_VARG_LIMIT
//   brief: compile-time check that N does not exceed the configured limit
#define D_DMACRO_CHECK_VARG_LIMIT(n) \
    ((n) <= D_DMACRO_VARG_MAX)

// D_DMACRO_CHECK_PAIR_LIMIT
//   brief: compile-time check that N does not exceed the pair limit
#define D_DMACRO_CHECK_PAIR_LIMIT(n) \
    ((n) <= D_DMACRO_PAIR_MAX)

// D_DMACRO_CHECK_TRIPLE_LIMIT
//   brief: compile-time check that N does not exceed the triple limit
#define D_DMACRO_CHECK_TRIPLE_LIMIT(n) \
    ((n) <= D_DMACRO_TRIPLE_MAX)

// D_DMACRO_CHECK_4TUPLE_LIMIT
//   brief: compile-time check that N does not exceed the 4-tuple limit
#define D_DMACRO_CHECK_4TUPLE_LIMIT(n) \
    ((n) <= D_DMACRO_4TUPLE_MAX)


// --- 0.6  Feature Include Configuration ---
//
// Each generated macro family and tuple arity can be independently
// enabled (1) or disabled (0).  All default to enabled.
// Define any of these BEFORE including dmacro.h to override.
//
// EXAMPLE — keep only what you need:
//   #define D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE           0
//   #define D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR 0
//   #define D_CFG_DMACRO_INCLUDE_TUPLE_5  0
//     ...
//   #define D_CFG_DMACRO_INCLUDE_TUPLE_16 0
//   #include "dmacro.h"
//
// Core utilities (count_args, varg_has_args, varg_get_arg, inc) are always
// included because every other feature depends on them.
//
// Each generated .h file contains exactly ONE macro family:
//   FOO_0, FOO_1, ... FOO_N   and nothing else.

// D_CFG_DMACRO_INCLUDE_FOR_EACH
//   brief: D_INTERNAL_FOR_EACH_N
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR
//   brief: D_INTERNAL_FOR_EACH_SEPARATOR_N
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR
//   brief: D_INTERNAL_FOR_EACH_PAIR_N (even only)
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR
//   brief: D_INTERNAL_FOR_EACH_PAIR_SEPARATOR_N (even only)
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE
//   brief: D_INTERNAL_FOR_EACH_TRIPLE_N (div-by-3 only)
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR
//   brief: D_INTERNAL_FOR_EACH_TRIPLE_SEPARATOR_N (div-by-3 only)
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR
//   brief: D_INTERNAL_FOR_EACH_DATA_SEPARATOR_N
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR 1
#endif

// D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA
//   brief: D_INTERNAL_FOR_EACH_COMMA_N (MSVC-safe hardcoded comma)
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA 1
#endif

// D_CFG_DMACRO_INCLUDE_TUPLE_1 .. D_CFG_DMACRO_INCLUDE_TUPLE_16
//   brief: per-arity tuple iteration families (separator + comma variants)
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_1
    #define D_CFG_DMACRO_INCLUDE_TUPLE_1  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_2
    #define D_CFG_DMACRO_INCLUDE_TUPLE_2  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_3
    #define D_CFG_DMACRO_INCLUDE_TUPLE_3  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_4
    #define D_CFG_DMACRO_INCLUDE_TUPLE_4  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_5
    #define D_CFG_DMACRO_INCLUDE_TUPLE_5  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_6
    #define D_CFG_DMACRO_INCLUDE_TUPLE_6  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_7
    #define D_CFG_DMACRO_INCLUDE_TUPLE_7  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_8
    #define D_CFG_DMACRO_INCLUDE_TUPLE_8  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_9
    #define D_CFG_DMACRO_INCLUDE_TUPLE_9  1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_10
    #define D_CFG_DMACRO_INCLUDE_TUPLE_10 1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_11
    #define D_CFG_DMACRO_INCLUDE_TUPLE_11 1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_12
    #define D_CFG_DMACRO_INCLUDE_TUPLE_12 1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_13
    #define D_CFG_DMACRO_INCLUDE_TUPLE_13 1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_14
    #define D_CFG_DMACRO_INCLUDE_TUPLE_14 1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_15
    #define D_CFG_DMACRO_INCLUDE_TUPLE_15 1
#endif
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_16
    #define D_CFG_DMACRO_INCLUDE_TUPLE_16 1
#endif


// --- 0.7  Variant File Includes ---
//
// Each .h contains exactly one macro family (FOO_0 .. FOO_N).
// Core utilities are always pulled in; everything else honours
// the section 0.6 toggles.
//
// FILE NAMING (generated by C-Macro-Generator.ps1 --generate-all-modules):
//   for_each{N}.h               for_each_mvc.h
//   for_each_separator{N}.h      for_each_separator_mvc.h
//   for_each_pair{N}.h           for_each_pair_mvc.h
//   for_each_comma{N}.h          for_each_comma_mvc.h
//   for_each_{K}_tuple_sep{N}.h  for_each_{K}_tuple_sep_mvc.h
//   for_each_{K}_tuple_comma{N}.h for_each_{K}_tuple_comma_mvc.h

// ---- variant 64 ----
#if (D_DMACRO_VARIANT == 64)
    #include ".\core\macro\count_args64.h"
    #include ".\core\macro\varg_has_args64.h"
    #include ".\core\macro\varg_get_arg64.h"
    #include ".\core\macro\inc64.h"
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include ".\core\macro\for_each64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include ".\core\macro\for_each_separator64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include ".\core\macro\for_each_pair64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include ".\core\macro\for_each_pair_separator64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include ".\core\macro\for_each_triple64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include ".\core\macro\for_each_triple_separator64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include ".\core\macro\for_each_data_separator64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include ".\core\macro\for_each_comma64.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include ".\core\macro\for_each_1_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_1_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include ".\core\macro\for_each_2_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_2_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include ".\core\macro\for_each_3_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_3_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include ".\core\macro\for_each_4_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_4_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include ".\core\macro\for_each_5_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_5_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include ".\core\macro\for_each_6_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_6_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include ".\core\macro\for_each_7_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_7_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include ".\core\macro\for_each_8_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_8_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include ".\core\macro\for_each_9_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_9_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include ".\core\macro\for_each_10_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_10_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include ".\core\macro\for_each_11_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_11_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include ".\core\macro\for_each_12_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_12_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include ".\core\macro\for_each_13_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_13_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include ".\core\macro\for_each_14_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_14_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include ".\core\macro\for_each_15_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_15_tuple_comma64.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include ".\core\macro\for_each_16_tuple_sep64.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_16_tuple_comma64.h"
        #endif
    #endif

// ---- variant 127 (MSVC traditional preprocessor) ----
#elif (D_DMACRO_VARIANT == 127)
    #include ".\core\macro\count_args_mvc.h"
    #include ".\core\macro\varg_has_args_mvc.h"
    #include ".\core\macro\varg_get_arg_mvc.h"
    #include ".\core\macro\inc_mvc.h"
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include ".\core\macro\for_each_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include ".\core\macro\for_each_separator_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include ".\core\macro\for_each_pair_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include ".\core\macro\for_each_pair_separator_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include ".\core\macro\for_each_triple_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include ".\core\macro\for_each_triple_separator_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include ".\core\macro\for_each_data_separator_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include ".\core\macro\for_each_comma_mvc.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include ".\core\macro\for_each_1_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_1_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include ".\core\macro\for_each_2_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_2_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include ".\core\macro\for_each_3_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_3_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include ".\core\macro\for_each_4_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_4_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include ".\core\macro\for_each_5_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_5_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include ".\core\macro\for_each_6_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_6_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include ".\core\macro\for_each_7_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_7_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include ".\core\macro\for_each_8_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_8_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include ".\core\macro\for_each_9_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_9_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include ".\core\macro\for_each_10_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_10_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include ".\core\macro\for_each_11_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_11_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include ".\core\macro\for_each_12_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_12_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include ".\core\macro\for_each_13_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_13_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include ".\core\macro\for_each_14_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_14_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include ".\core\macro\for_each_15_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_15_tuple_comma_mvc.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include ".\core\macro\for_each_16_tuple_sep_mvc.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_16_tuple_comma_mvc.h"
        #endif
    #endif

// ---- variant 128 ----
#elif (D_DMACRO_VARIANT == 128)
    #include ".\core\macro\varg_count128.h"
    #include ".\core\macro\varg_has_args128.h"
    #include ".\core\macro\varg_get_arg128.h"
    #include ".\core\macro\inc128.h"
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include ".\core\macro\for_each128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include ".\core\macro\for_each_separator128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include ".\core\macro\for_each_pair128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include ".\core\macro\for_each_pair_separator128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include ".\core\macro\for_each_triple128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include ".\core\macro\for_each_triple_separator128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include ".\core\macro\for_each_data_separator128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include ".\core\macro\for_each_comma128.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include ".\core\macro\for_each_1_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_1_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include ".\core\macro\for_each_2_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_2_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include ".\core\macro\for_each_3_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_3_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include ".\core\macro\for_each_4_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_4_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include ".\core\macro\for_each_5_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_5_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include ".\core\macro\for_each_6_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_6_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include ".\core\macro\for_each_7_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_7_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include ".\core\macro\for_each_8_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_8_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include ".\core\macro\for_each_9_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_9_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include ".\core\macro\for_each_10_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_10_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include ".\core\macro\for_each_11_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_11_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include ".\core\macro\for_each_12_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_12_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include ".\core\macro\for_each_13_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_13_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include ".\core\macro\for_each_14_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_14_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include ".\core\macro\for_each_15_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_15_tuple_comma128.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include ".\core\macro\for_each_16_tuple_sep128.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_16_tuple_comma128.h"
        #endif
    #endif

// ---- variant 256 ----
#elif (D_DMACRO_VARIANT == 256)
    #include ".\core\macro\count_args256.h"
    #include ".\core\macro\varg_has_args256.h"
    #include ".\core\macro\varg_get_arg256.h"
    #include ".\core\macro\inc256.h"
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include ".\core\macro\for_each256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include ".\core\macro\for_each_separator256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include ".\core\macro\for_each_pair256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include ".\core\macro\for_each_pair_separator256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include ".\core\macro\for_each_triple256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include ".\core\macro\for_each_triple_separator256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include ".\core\macro\for_each_data_separator256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include ".\core\macro\for_each_comma256.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include ".\core\macro\for_each_1_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_1_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include ".\core\macro\for_each_2_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_2_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include ".\core\macro\for_each_3_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_3_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include ".\core\macro\for_each_4_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_4_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include ".\core\macro\for_each_5_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_5_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include ".\core\macro\for_each_6_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_6_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include ".\core\macro\for_each_7_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_7_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include ".\core\macro\for_each_8_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_8_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include ".\core\macro\for_each_9_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_9_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include ".\core\macro\for_each_10_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_10_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include ".\core\macro\for_each_11_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_11_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include ".\core\macro\for_each_12_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_12_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include ".\core\macro\for_each_13_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_13_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include ".\core\macro\for_each_14_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_14_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include ".\core\macro\for_each_15_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_15_tuple_comma256.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include ".\core\macro\for_each_16_tuple_sep256.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_16_tuple_comma256.h"
        #endif
    #endif

// ---- variant 512 ----
#elif (D_DMACRO_VARIANT == 512)
    #include ".\core\macro\count_args512.h"
    #include ".\core\macro\varg_has_args512.h"
    #include ".\core\macro\varg_get_arg512.h"
    #include ".\core\macro\inc512.h"
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include ".\core\macro\for_each512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include ".\core\macro\for_each_separator512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include ".\core\macro\for_each_pair512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include ".\core\macro\for_each_pair_separator512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include ".\core\macro\for_each_triple512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include ".\core\macro\for_each_triple_separator512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include ".\core\macro\for_each_data_separator512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include ".\core\macro\for_each_comma512.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include ".\core\macro\for_each_1_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_1_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include ".\core\macro\for_each_2_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_2_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include ".\core\macro\for_each_3_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_3_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include ".\core\macro\for_each_4_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_4_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include ".\core\macro\for_each_5_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_5_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include ".\core\macro\for_each_6_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_6_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include ".\core\macro\for_each_7_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_7_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include ".\core\macro\for_each_8_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_8_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include ".\core\macro\for_each_9_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_9_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include ".\core\macro\for_each_10_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_10_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include ".\core\macro\for_each_11_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_11_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include ".\core\macro\for_each_12_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_12_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include ".\core\macro\for_each_13_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_13_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include ".\core\macro\for_each_14_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_14_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include ".\core\macro\for_each_15_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_15_tuple_comma512.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include ".\core\macro\for_each_16_tuple_sep512.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_16_tuple_comma512.h"
        #endif
    #endif

// ---- variant 1024 (fallback) ----
#else  // 1024
    #include ".\core\macro\count_args1024.h"
    #include ".\core\macro\varg_has_args1024.h"
    #include ".\core\macro\varg_get_arg1024.h"
    #include ".\core\macro\inc1024.h"
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include ".\core\macro\for_each1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include ".\core\macro\for_each_separator1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include ".\core\macro\for_each_pair1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include ".\core\macro\for_each_pair_separator1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include ".\core\macro\for_each_triple1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include ".\core\macro\for_each_triple_separator1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include ".\core\macro\for_each_data_separator1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include ".\core\macro\for_each_comma1024.h"
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include ".\core\macro\for_each_1_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_1_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include ".\core\macro\for_each_2_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_2_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include ".\core\macro\for_each_3_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_3_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include ".\core\macro\for_each_4_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_4_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include ".\core\macro\for_each_5_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_5_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include ".\core\macro\for_each_6_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_6_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include ".\core\macro\for_each_7_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_7_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include ".\core\macro\for_each_8_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_8_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include ".\core\macro\for_each_9_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_9_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include ".\core\macro\for_each_10_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_10_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include ".\core\macro\for_each_11_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_11_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include ".\core\macro\for_each_12_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_12_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include ".\core\macro\for_each_13_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_13_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include ".\core\macro\for_each_14_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_14_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include ".\core\macro\for_each_15_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_15_tuple_comma1024.h"
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include ".\core\macro\for_each_16_tuple_sep1024.h"
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include ".\core\macro\for_each_16_tuple_comma1024.h"
        #endif
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
///                     I. BASIC TOKEN MANIPULATION                         ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_CONCAT_HELPER
//   macro (internal): performs actual token concatenation after expansion
#define D_INTERNAL_CONCAT_HELPER(a, b) a##b

// D_CONCAT
//   macro: token-paste two identifiers with macro expansion
#define D_CONCAT(a, b)  \
    D_INTERNAL_CONCAT_HELPER(a, b)

// D_STRINGIFY
//   macro: convert argument to string literal without expanding it first
#define D_STRINGIFY(x)  \
    #x

// D_TOSTR
//   macro: stringify with one macro expansion pass (expand then stringify)
#define D_TOSTR(x)  \
    D_STRINGIFY(x)

// D_EXPAND
//   macro: force one additional macro expansion pass
#define D_EXPAND(x)  \
    x

// D_EMPTY
//   macro: expands to nothing, useful for conditional expansion
#define D_EMPTY()

// D_DEFER
//   macro: defers macro expansion by one pass
#define D_DEFER(id)  \
    id D_EMPTY()

// D_OBSTRUCT
//   macro: defers macro expansion by two passes
#define D_OBSTRUCT(...)  \
    __VA_ARGS__ D_DEFER(D_EMPTY)()

// D_UNPACK
//   macro: remove parentheses from arguments
#define D_UNPACK(...)  \
    __VA_ARGS__

// D_SEPARATOR_COMMA
//   macro:
#define D_SEPARATOR_COMMA ,

// D_SEPARATOR_SEMICOLON
//   macro:
#define D_SEPARATOR_SEMICOLON ;

// D_SEPARATOR_SPACE
//   macro:
#define D_SEPARATOR_SPACE 


///////////////////////////////////////////////////////////////////////////////
///                     II. ARRAY UTILITIES                                 ///
///////////////////////////////////////////////////////////////////////////////

// D_ARRAY_COUNT
//   macro: returns the number of elements in a statically allocated array
#define D_ARRAY_COUNT(arr)  \
    ( (sizeof(arr)) / (sizeof((arr)[0])) )

// D_ARRAY_COUNT_SAFE
//   macro: count variadic elements of specified type; returns 0 if empty
#define D_ARRAY_COUNT_SAFE(type, ...)                           \
    ( D_VARG_COUNT(__VA_ARGS__) == 0 ?                          \
        0 :                                                     \
        (sizeof((type[]){ __VA_ARGS__ }) / sizeof(type)) )

// D_ARRAY_COUNT_T
//   macro: count variadic elements of specified type using array size
#define D_ARRAY_COUNT_T(type, ...)  \
    ( (sizeof((type[]){ __VA_ARGS__ }) / sizeof(type)) )

// D_MAKE_ARRAY
//   macro: generate array initialization
//   Usage: D_MAKE_ARRAY(int, nums, 1, 2, 3) -> int nums[] = { 1, 2, 3 }
//   Note: Direct expansion for MSVC traditional preprocessor compatibility
#define D_MAKE_ARRAY(type, name, ...)                                         \
    type name[] = { __VA_ARGS__ }

// D_MAKE_STRING_ARRAY
//   macro: generate string array from identifiers
//   Usage: D_MAKE_STRING_ARRAY(names, foo, bar) -> 
//          const char* names[] = { "foo", "bar" }
//   Note: Uses D_FOR_EACH_COMMA with D_STRINGIFY for MSVC compatibility
#define D_MAKE_STRING_ARRAY(name, ...)                                        \
    const char* name[] = { D_FOR_EACH_COMMA(D_STRINGIFY, __VA_ARGS__) }


///////////////////////////////////////////////////////////////////////////////
///                     III. ARGUMENT COUNTING UTILITIES                    ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_VARG_GET_N
//   macro (internal): select arg N (where N is a number token)
#define D_INTERNAL_VARG_GET_N(_n, ...) \
    D_CONCAT(D_VARG_GET_ARG_, _n)(__VA_ARGS__)

// D_VARG_GET_FIRST through D_VARG_GET_TENTH
//   macro (alias): convenience names for positional argument extraction.
#define D_VARG_GET_FIRST(...)   D_VARG_GET_ARG_1(__VA_ARGS__)
#define D_VARG_GET_SECOND(...)  D_VARG_GET_ARG_2(__VA_ARGS__)
#define D_VARG_GET_THIRD(...)   D_VARG_GET_ARG_3(__VA_ARGS__)
#define D_VARG_GET_FOURTH(...)  D_VARG_GET_ARG_4(__VA_ARGS__)
#define D_VARG_GET_FIFTH(...)   D_VARG_GET_ARG_5(__VA_ARGS__)
#define D_VARG_GET_SIXTH(...)   D_VARG_GET_ARG_6(__VA_ARGS__)
#define D_VARG_GET_SEVENTH(...) D_VARG_GET_ARG_7(__VA_ARGS__)
#define D_VARG_GET_EIGHTH(...)  D_VARG_GET_ARG_8(__VA_ARGS__)
#define D_VARG_GET_NINTH(...)   D_VARG_GET_ARG_9(__VA_ARGS__)
#define D_VARG_GET_TENTH(...)   D_VARG_GET_ARG_10(__VA_ARGS__)

// D_HEAD - alias for D_FIRST
#define D_HEAD(...) D_VARG_GET_FIRST(__VA_ARGS__)

// D_REST - get all arguments except the first
#define D_REST(first, ...) __VA_ARGS__

// D_TAIL - alias for D_REST
#define D_TAIL(first, ...) __VA_ARGS__

// D_VARG_LAST
//   macro: expands to the last argument in __VA_ARGS__
#define D_VARG_LAST(...) \
    D_INTERNAL_VARG_GET_N(D_VARG_COUNT(__VA_ARGS__), __VA_ARGS__)

// D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER
//   macro: 
#define D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER(...) __VA_ARGS__

// D_VARGS_REMOVE_PARENTHESES
//   macro: 
#define D_VARGS_REMOVE_PARENTHESES(arg)  \
    D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER arg


///////////////////////////////////////////////////////////////////////////////
///                    IV. MACRO EXPANSION AND EVALUATION                   ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_EVAL_####
//   macro (internal): cascading expansion helpers, each doubles the prior
#define D_INTERNAL_EVAL_0001(...)  __VA_ARGS__
#define D_INTERNAL_EVAL_0002(...)  D_INTERNAL_EVAL_0001(D_INTERNAL_EVAL_0001(__VA_ARGS__))
#define D_INTERNAL_EVAL_0004(...)  D_INTERNAL_EVAL_0002(D_INTERNAL_EVAL_0002(__VA_ARGS__))
#define D_INTERNAL_EVAL_0008(...)  D_INTERNAL_EVAL_0004(D_INTERNAL_EVAL_0004(__VA_ARGS__))
#define D_INTERNAL_EVAL_0016(...)  D_INTERNAL_EVAL_0008(D_INTERNAL_EVAL_0008(__VA_ARGS__))
#define D_INTERNAL_EVAL_0032(...)  D_INTERNAL_EVAL_0016(D_INTERNAL_EVAL_0016(__VA_ARGS__))
#define D_INTERNAL_EVAL_0064(...)  D_INTERNAL_EVAL_0032(D_INTERNAL_EVAL_0032(__VA_ARGS__))
#define D_INTERNAL_EVAL_0128(...)  D_INTERNAL_EVAL_0064(D_INTERNAL_EVAL_0064(__VA_ARGS__))
#define D_INTERNAL_EVAL_0256(...)  D_INTERNAL_EVAL_0128(D_INTERNAL_EVAL_0128(__VA_ARGS__))
#define D_INTERNAL_EVAL_0512(...)  D_INTERNAL_EVAL_0256(D_INTERNAL_EVAL_0256(__VA_ARGS__))
#define D_INTERNAL_EVAL_1024(...)  D_INTERNAL_EVAL_0512(D_INTERNAL_EVAL_0512(__VA_ARGS__))

// D_EVAL
//   macro: forces multiple macro expansion passes (1024 iterations)
#define D_EVAL(...)  D_INTERNAL_EVAL_1024(__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////
///                     V. BOOLEAN AND CONDITIONAL LOGIC                    ///
///////////////////////////////////////////////////////////////////////////////

// Probe mechanism for boolean detection
#define D_PROBE(x) x, 1
#define D_CHECK_IMPL(x, n, ...) n
#define D_CHECK(...) D_CHECK_IMPL(__VA_ARGS__, 0,)

// Parentheses detection
#define D_IS_PAREN_PROBE(...) D_PROBE(~)
#define D_IS_PAREN_WRAPPER(x) D_IS_PAREN_PROBE x
#define D_IS_PAREN(x) D_CHECK(D_IS_PAREN_WRAPPER(x))

// D_META
//   macro: wrap arguments in double parentheses
#define D_META(...) ((__VA_ARGS__))

// D_IF - conditional expansion based on boolean value
#define D_IF(cond) D_CONCAT(D_IF_, cond)
#define D_IF_1(true_case) true_case D_IF_1_ELSE
#define D_IF_0(true_case) D_IF_0_ELSE
#define D_IF_1_ELSE(...)
#define D_IF_0_ELSE(...) __VA_ARGS__

// D_IIF - immediate if (simpler two-branch conditional)
#define D_IIF(cond) D_CONCAT(D_IIF_, cond)
#define D_IIF_0(t, f) f
#define D_IIF_1(t, f) t

// D_NOT - logical negation (only 0 -> 1, everything else -> 0)
#define D_NOT(x) D_CHECK(D_CONCAT(D_NOT_, x))
#define D_NOT_0 D_PROBE(~)

// D_COMPL - complement (strict: only works on 0 or 1)
#define D_COMPL(b) D_CONCAT(D_COMPL_, b)
#define D_COMPL_0 1
#define D_COMPL_1 0

// D_BOOL - normalize value to 0 or 1
#define D_BOOL(x) D_COMPL(D_NOT(x))

// D_AND - logical AND (requires normalized boolean inputs)
#define D_AND(x, y) D_CONCAT(D_AND_, D_CONCAT(D_BOOL(x), D_BOOL(y)))
#define D_AND_00 0
#define D_AND_01 0
#define D_AND_10 0
#define D_AND_11 1

// D_OR - logical OR (requires normalized boolean inputs)
#define D_OR(x, y) D_CONCAT(D_OR_, D_CONCAT(D_BOOL(x), D_BOOL(y)))
#define D_OR_00 0
#define D_OR_01 1
#define D_OR_10 1
#define D_OR_11 1


///////////////////////////////////////////////////////////////////////////////
///                    VI. CORE ITERATION INFRASTRUCTURE                    ///
///////////////////////////////////////////////////////////////////////////////

// D_INC
//   macro: increments a number token (0-63 -> 1-64).
#define D_INC(x)                                                            \
    D_CONCAT(D_INTERNAL_INC_, x)

// map termination detection (for recursive implementations)

// D_INTERNAL_MAP_END
//   macro: sentinel macro that terminates recursive map expansion.
#define D_INTERNAL_MAP_END(...)

// D_INTERNAL_MAP_OUT
//   macro: helper expansion token for map recursion (expands to nothing).
#define D_INTERNAL_MAP_OUT

// D_INTERNAL_MAP_GET_END
//   macro: expands to `0, D_INTERNAL_MAP_END` for end-marker detection.
#define D_INTERNAL_MAP_GET_END()                                            \
    0, D_INTERNAL_MAP_END

// D_INTERNAL_MAP_NEXT0
//   macro: selects the continuation macro via argument shifting.
#define D_INTERNAL_MAP_NEXT0(item, next, ...)                               \
    next D_INTERNAL_MAP_OUT

// D_INTERNAL_MAP_NEXT1
//   macro: wraps D_INTERNAL_MAP_NEXT0 to enable end-marker detection.
#define D_INTERNAL_MAP_NEXT1(item, next)                                    \
    D_INTERNAL_MAP_NEXT0(item, next, 0)

// D_INTERNAL_MAP_NEXT
//   macro: resolves to the next map worker (or END) based on `peek`.
#define D_INTERNAL_MAP_NEXT(item, next)                                     \
    D_INTERNAL_MAP_NEXT1(D_INTERNAL_MAP_GET_END item, next)

// core mapping with data parameter (for D_FOR_EACH_DATA)

// D_INTERNAL_MAP_DATA0
//   macro: data-mapping worker (even step) for D_FOR_EACH_DATA.
#define D_INTERNAL_MAP_DATA0(f, data, x, peek, ...)                         \
    f(data, x)                                                              \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_DATA1)(f,                      \
                                                    data,                   \
                                                    peek,                   \
                                                    __VA_ARGS__)

// D_INTERNAL_MAP_DATA1
//   macro: data-mapping worker (odd step) for D_FOR_EACH_DATA.
#define D_INTERNAL_MAP_DATA1(f, data, x, peek, ...)                         \
    f(data, x)                                                              \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_DATA0)(f,                      \
                                                    data,                   \
                                                    peek,                   \
                                                    __VA_ARGS__)

// indexed mapping (for D_FOR_EACH_INDEXED)

// D_INTERNAL_MAP_IDX0
//   macro: indexed mapping worker (even step); applies f(i, x) then increments i.
#define D_INTERNAL_MAP_IDX0(f, i, x, peek, ...)                             \
    f(i, x)                                                                 \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_IDX1)(f,                       \
                                                   D_INC(i),                \
                                                   peek,                    \
                                                   __VA_ARGS__)

// D_INTERNAL_MAP_IDX1
//   macro: indexed mapping worker (odd step); applies f(i, x) then increments i.
#define D_INTERNAL_MAP_IDX1(f, i, x, peek, ...)                             \
    f(i, x)                                                                 \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_IDX0)(f,                       \
                                                   D_INC(i),                \
                                                   peek,                    \
                                                   __VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////
///                    VII. FOR_EACH UNIFIED INTERFACE                      ///
///////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// VII.1  Single-Element Iteration
// -----------------------------------------------------------------------------

// D_FOR_EACH
//   macro: apply function to each element with no separator.
// Usage: D_FOR_EACH(fn, a, b, c) -> fn(a) fn(b) fn(c)
#define D_FOR_EACH(_fn, ...)                                                \
    D_CONCAT(D_INTERNAL_FOR_EACH_SEPARATOR_,                                \
             D_VARG_COUNT(__VA_ARGS__))(_fn, D_EMPTY, __VA_ARGS__)

// D_FOR_EACH_SPACE
//   macro: alias for D_FOR_EACH (space/no separator).
#define D_FOR_EACH_SPACE(_fn, ...)                                          \
    D_FOR_EACH(_fn, __VA_ARGS__)

// D_FOR_EACH_SEP
//   macro: apply function to each element with custom separator.
// Usage: D_FOR_EACH_SEP(;, fn, a, b, c) -> fn(a); fn(b); fn(c)
#define D_FOR_EACH_SEP(_sep, _fn, ...)                                      \
    D_CONCAT(D_INTERNAL_FOR_EACH_SEPARATOR_,                                \
             D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)

#if defined(D_ENV_COMPILER_MSVC)
    // D_FOR_EACH_COMMA
    //   macro: apply function to each element, comma-separated.
    #define D_FOR_EACH_COMMA(_fn, ...)                                      \
        D_CONCAT(D_INTERNAL_FOR_EACH_COMMA_,                                \
                 D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)
#else
    // D_FOR_EACH_COMMA
    //   macro: apply function to each element, comma-separated.
    #define D_FOR_EACH_COMMA(_fn, ...)                                      \
        D_FOR_EACH_SEP(D_SEPARATOR_COMMA, _fn, __VA_ARGS__)

#endif

// D_FOR_EACH_SEMICOLON
//   macro: apply function to each element, semicolon-separated.
#define D_FOR_EACH_SEMICOLON(_fn, ...)                                      \
    D_FOR_EACH_SEP(D_SEPARATOR_SEMICOLON, _fn, __VA_ARGS__)


// -----------------------------------------------------------------------------
// VII.2  Pair Iteration (2-tuples)
// -----------------------------------------------------------------------------

// D_INTERNAL_PAIR_COUNT
//   macro (internal): compute number of pairs from arg count.
#define D_INTERNAL_PAIR_COUNT(...)                                          \
    (D_VARG_COUNT(__VA_ARGS__) / 2)

// D_FOR_EACH_PAIR
//   macro: apply function to each pair with no separator.
// Usage: D_FOR_EACH_PAIR(fn, a, 1, b, 2) -> fn(a, 1) fn(b, 2)
#define D_FOR_EACH_PAIR(_fn, ...)                                           \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn,                       \
                                                 D_EMPTY,                   \
                                                 __VA_ARGS__)

// D_FOR_EACH_PAIR_COMMA
//   macro: apply function to each pair, comma-separated.
#define D_FOR_EACH_PAIR_COMMA(_fn, ...)                                     \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn,                       \
                                                 D_SEPARATOR_COMMA,         \
                                                 __VA_ARGS__)
// D_FOR_EACH_PAIR_SEMICOLON
//   macro: apply function to each element, semicolon-separated.
#define D_FOR_EACH_PAIR_SEMICOLON(_fn, ...)                                 \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn,                       \
                                                 D_SEPARATOR_SEMICOLON,     \
                                                 __VA_ARGS__)

// D_FOR_EACH_SPACE
//   macro: alias for D_FOR_EACH (space/no separator).
#define D_FOR_EACH_PAIR_SPACE(_fn, ...)                                     \
    D_FOR_EACH_PAIR(_fn, __VA_ARGS__)

// D_FOR_EACH_PAIR_SEP
//   macro: apply function to each pair with custom separator.
#define D_FOR_EACH_PAIR_SEP(_fn, _sep, ...)                                 \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)


// -----------------------------------------------------------------------------
// VII.3  Triple Iteration (3-tuples)
// -----------------------------------------------------------------------------

// D_INTERNAL_TRIPLE_COUNT
//   macro (internal): compute number of triples from arg count.
#define D_INTERNAL_TRIPLE_COUNT(...)                                        \
    (D_VARG_COUNT(__VA_ARGS__) / 3)

// D_FOR_EACH_TRIPLE
//   macro: apply function to each triple with no separator.
// Usage: D_FOR_EACH_TRIPLE(fn, a, b, c, d, e, f) -> fn(a,b,c) fn(d,e,f)
#define D_FOR_EACH_TRIPLE(_fn, ...)                                         \
    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_,                                   \
             D_INTERNAL_TRIPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// D_FOR_EACH_TRIPLE_COMMA
//   macro: apply function to each triple, comma-separated.
#define D_FOR_EACH_TRIPLE_COMMA(_fn, ...)                                   \
    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_COMMA_,                             \
             D_INTERNAL_TRIPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// D_FOR_EACH_TRIPLE_SEP
//   macro: apply function to each triple with custom separator.
#define D_FOR_EACH_TRIPLE_SEP(_sep, _fn, ...)                               \
    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_SEP_,                               \
             D_INTERNAL_TRIPLE_COUNT(__VA_ARGS__))(_sep, _fn, __VA_ARGS__)


// -----------------------------------------------------------------------------
// VII.4  4-Tuple Iteration
// -----------------------------------------------------------------------------

// D_INTERNAL_4TUPLE_COUNT
//   macro (internal): compute number of 4-tuples from arg count.
#define D_INTERNAL_4TUPLE_COUNT(...)                                        \
    (D_VARG_COUNT(__VA_ARGS__) / 4)

// D_FOR_EACH_4TUPLE
//   macro: apply function to each 4-tuple with no separator.
// Usage: D_FOR_EACH_4TUPLE(fn, a,b,c,d, e,f,g,h) -> fn(a,b,c,d) fn(e,f,g,h)
#define D_FOR_EACH_4TUPLE(_fn, ...)                                         \
    D_CONCAT(D_INTERNAL_FOR_EACH_4TUPLE_,                                   \
             D_INTERNAL_4TUPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// D_FOR_EACH_4TUPLE_COMMA
//   macro: apply function to each 4-tuple, comma-separated.
#define D_FOR_EACH_4TUPLE_COMMA(_fn, ...)                                   \
    D_CONCAT(D_INTERNAL_FOR_EACH_4TUPLE_COMMA_,                             \
             D_INTERNAL_4TUPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// D_FOR_EACH_4TUPLE_SEP
//   macro: apply function to each 4-tuple with custom separator.
#define D_FOR_EACH_4TUPLE_SEP(_sep, _fn, ...)                               \
    D_CONCAT(D_INTERNAL_FOR_EACH_4TUPLE_SEP_,                               \
             D_INTERNAL_4TUPLE_COUNT(__VA_ARGS__))(_sep, _fn, __VA_ARGS__)


// -----------------------------------------------------------------------------
// VII.5  With Data Parameter
// -----------------------------------------------------------------------------

// D_FOR_EACH_DATA
//   macro: apply function with extra data to each element.
// Usage: D_FOR_EACH_DATA(fn, ctx, a, b, c) -> fn(ctx,a) fn(ctx,b) fn(ctx,c)
#if defined(D_ENV_PP_HAS_VA_OPT_ENABLED) && (D_ENV_PP_HAS_VA_OPT_ENABLED)
    #define D_FOR_EACH_DATA(_fn, _data, ...) \
        __VA_OPT__(D_EVAL(D_INTERNAL_MAP_DATA0(_fn, _data, __VA_ARGS__, (), 0)))
#else
    #define D_FOR_EACH_DATA(_fn, _data, ...) \
        D_EVAL(D_INTERNAL_MAP_DATA0(_fn, _data, __VA_ARGS__, (), 0))
#endif


// -----------------------------------------------------------------------------
// VII.6  Indexed Iteration
// -----------------------------------------------------------------------------

// D_FOR_EACH_INDEXED
//   macro: apply function with 0-based index to each element.
// Usage: D_FOR_EACH_INDEXED(fn, a, b, c) -> fn(0, a) fn(1, b) fn(2, c)
#if defined(D_ENV_PP_HAS_VA_OPT_ENABLED) && (D_ENV_PP_HAS_VA_OPT_ENABLED)
    #define D_FOR_EACH_INDEXED(_fn, ...) \
        __VA_OPT__(D_EVAL(D_INTERNAL_MAP_IDX0(_fn, 0, __VA_ARGS__, (), 0)))
#else
    #define D_FOR_EACH_INDEXED(_fn, ...) \
        D_EVAL(D_INTERNAL_MAP_IDX0(_fn, 0, __VA_ARGS__, (), 0))
#endif


// -----------------------------------------------------------------------------
// VII.7  Generic N-Tuple Dispatch
// -----------------------------------------------------------------------------

// D_FOR_EACH_NTUPLE
//   macro: generic N-tuple iteration dispatcher.
// Usage: D_FOR_EACH_NTUPLE(2, fn, a, b, c, d) -> D_FOR_EACH_PAIR(fn, a, b, c, d)
#define D_FOR_EACH_NTUPLE(_n, _fn, ...) \
    D_CONCAT(D_FOR_EACH_, D_CONCAT(_n, TUPLE))(_fn, __VA_ARGS__)

#define D_FOR_EACH_NTUPLE_COMMA(_n, _fn, ...) \
    D_CONCAT(D_FOR_EACH_, D_CONCAT(_n, TUPLE_COMMA))(_fn, __VA_ARGS__)

#define D_FOR_EACH_NTUPLE_SEP(_n, _sep, _fn, ...) \
    D_CONCAT(D_FOR_EACH_, D_CONCAT(_n, TUPLE_SEP))(_sep, _fn, __VA_ARGS__)

// N-tuple naming aliases
#define D_FOR_EACH_1TUPLE        D_FOR_EACH
#define D_FOR_EACH_1TUPLE_COMMA  D_FOR_EACH_COMMA
#define D_FOR_EACH_1TUPLE_SEP    D_FOR_EACH_SEP
#define D_FOR_EACH_2TUPLE        D_FOR_EACH_PAIR
#define D_FOR_EACH_2TUPLE_COMMA  D_FOR_EACH_PAIR_COMMA
#define D_FOR_EACH_2TUPLE_SEP    D_FOR_EACH_PAIR_SEP
#define D_FOR_EACH_3TUPLE        D_FOR_EACH_TRIPLE
#define D_FOR_EACH_3TUPLE_COMMA  D_FOR_EACH_TRIPLE_COMMA
#define D_FOR_EACH_3TUPLE_SEP    D_FOR_EACH_TRIPLE_SEP

///////////////////////////////////////////////////////////////////////////////
///                    VIII. MEMBER ACCESS ITERATION                        ///
///////////////////////////////////////////////////////////////////////////////

// Pointer member access internals
#define D_INTERNAL_MEMBER_PTR_OP(obj, member, x) obj->D_CONCAT(member, x)
#define D_INTERNAL_MEMBER_DOT_OP(obj, member, x) obj.D_CONCAT(member, x)

// D_FOR_EACH_MEMBER_PTR
//   macro: member access with -> operator
// Usage: D_FOR_EACH_MEMBER_PTR(obj, field_, a, b) -> obj->field_a obj->field_b
#define D_FOR_EACH_MEMBER_PTR(_obj, _member, ...) \
    D_FOR_EACH_DATA(D_INTERNAL_MEMBER_PTR_OP_EXPAND, (_obj, _member), __VA_ARGS__)

#define D_INTERNAL_MEMBER_PTR_OP_EXPAND(_objmem, _x) \
    D_VARG_GET_FIRST _objmem->D_CONCAT(D_VARG_GET_SECOND _objmem, _x)

// D_FOR_EACH_MEMBER_DOT
//   macro: member access with . operator
#define D_FOR_EACH_MEMBER_DOT(_obj, _member, ...) \
    D_FOR_EACH_DATA(D_INTERNAL_MEMBER_DOT_OP_EXPAND, (_obj, _member), __VA_ARGS__)

#define D_INTERNAL_MEMBER_DOT_OP_EXPAND(_objmem, _x) \
    D_VARG_GET_FIRST _objmem.D_CONCAT(D_VARG_GET_SECOND _objmem, _x)


///////////////////////////////////////////////////////////////////////////////
///                    IX. ADVANCED ITERATION PATTERNS                      ///
///////////////////////////////////////////////////////////////////////////////

// Adjacent pair internals (overlapping pairs)
#define D_INTERNAL_MAP_ADJ0(f, x, y, peek, ...) f(x, y)                       \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_ADJ1)(f, y, peek, __VA_ARGS__)
#define D_INTERNAL_MAP_ADJ1(f, x, y, peek, ...) f(x, y)                       \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_ADJ0)(f, y, peek, __VA_ARGS__)

// D_FOR_EACH_ADJACENT_PAIR
//   macro: overlapping pair iteration
// Usage: D_FOR_EACH_ADJACENT_PAIR(fn, a, b, c) -> fn(a,b) fn(b,c)
#define D_FOR_EACH_ADJACENT_PAIR(fn, first, second, ...)                      \
    D_EVAL(D_INTERNAL_MAP_ADJ0(fn, first, second, __VA_ARGS__, (), 0))


///////////////////////////////////////////////////////////////////////////////
///                    X.  POINTER ARRAY INITIALIZATION                     ///
///////////////////////////////////////////////////////////////////////////////

// Comma-separated FOR_EACH with data internals
#define D_INTERNAL_MAP_DATA_COMMA0(f, data, x, peek, ...) , f(data, x)        \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_DATA_COMMA1)(f, data, peek, __VA_ARGS__)
#define D_INTERNAL_MAP_DATA_COMMA1(f, data, x, peek, ...) , f(data, x)        \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_DATA_COMMA0)(f, data, peek, __VA_ARGS__)

// D_FOR_EACH_DATA_COMMA
//   macro: iteration with data, comma-separated
#define D_FOR_EACH_DATA_COMMA(fn, data, first, ...)                           \
    fn(data, first)                                                           \
    D_EVAL(D_INTERNAL_MAP_DATA_COMMA0(fn, data, __VA_ARGS__, (), 0))

// D_INTERNAL_PTR_ELEM
//   macro (internal): create void pointer to compound literal
#define D_INTERNAL_PTR_ELEM(element_type, tuple)                              \
    (void*)&(element_type){ D_UNPACK tuple }

// D_INTERNAL_TUPLE_TO_BRACES
//   macro (internal): convert tuple to brace-enclosed initializer
#define D_INTERNAL_TUPLE_TO_BRACES(_unused, tuple)                            \
    { D_UNPACK tuple }

// D_STRUCT_ARRAY_INIT
//   macro: initialize array of structs from tuples
#define D_STRUCT_ARRAY_INIT(...)                                              \
    { D_FOR_EACH_DATA_COMMA(D_INTERNAL_TUPLE_TO_BRACES, _, __VA_ARGS__) }


///////////////////////////////////////////////////////////////////////////////
///                     XI.  UTILITY OPERATORS                              ///
///////////////////////////////////////////////////////////////////////////////

// Debugging/testing operators
#define D_PRINT_OP(x)           printf("%s\n", D_STRINGIFY(x));
#define D_PRINT_VAL_OP(x)       printf("%s = %d\n", D_STRINGIFY(x), (int)(x));
#define D_DECLARE_VAR_OP(x)     int x;
#define D_DECLARE_TYPED_OP(t,n) t n;
#define D_ASSIGN_OP(var, value) var = value;
#define D_INIT_ZERO_OP(x)       x = 0;


///////////////////////////////////////////////////////////////////////////////
///                    XII. COMPILE-TIME ASSERTIONS                         ///
///////////////////////////////////////////////////////////////////////////////

// D_ASSERT_SAME_SIZE
//   macro: assert two types have the same size at compile time
#define D_ASSERT_SAME_SIZE(type1, type2)                                      \
    D_STATIC_ASSERT(sizeof(type1) == sizeof(type2),                           \
                    "Size mismatch: " #type1 " vs " #type2)


#endif  // DJINTERP_MACRO_