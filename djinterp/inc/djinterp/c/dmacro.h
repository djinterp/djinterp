/******************************************************************************
* djinterp [core]                                                     dmacro.h
*
*   Comprehensive macro utilities for the djinterp framework, providing token
* manipulation, variadic argument processing, iteration, X-macro facilities,
* and compile-time code generation.
*   The module also selects generated macro-family variants according to the
* detected preprocessor environment and user configuration.
*
*
* path:      /inc/djinterp/c/dmacro.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.03.23
*                                                          revised: 2026.09.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION SYSTEM
    --------------------
    1.  Configuration constants
         1.  Variadic argument limits
             a.  D_CFG_DMACRO_VARG_DEFAULT
             b.  D_CFG_DMACRO_VARG_MIN
             c.  D_CFG_DMACRO_VARG_LIMIT
         2.  Variant selection constants
    2.  User configuration
         1.  D_CFG_DMACRO_OVERRIDE
         2.  D_CFG_DMACRO_VARG_MAX
         3.  D_CFG_DMACRO_USE_MSVC_COMPAT
    3.  Effective configuration
         1.  D_INTERNAL_DMACRO_RAW_MAX
         2.  D_INTERNAL_DMACRO_CLAMPED_MAX
         3.  D_DMACRO_VARIANT / D_DMACRO_*_MAX
    4.  Public aliases
         1.  D_CFG_VARG_COUNT_MAX / D_VARG_COUNT_MAX
         2.  D_CFG_FOR_EACH_MAX / D_FOR_EACH_MAX
         3.  D_CFG_FOR_EACH_PAIR_MAX / D_FOR_EACH_PAIR_MAX
         4.  D_CFG_FOR_EACH_TRIPLE_MAX / D_FOR_EACH_TRIPLE_MAX
         5.  D_CFG_FOR_EACH_4TUPLE_MAX / D_FOR_EACH_4TUPLE_MAX
         6.  D_CFG_MACRO_VARIANT / D_MACRO_VARIANT
         7.  D_VARG_PAIR_MAX
    5.  Query and limit macros
         1.  D_DMACRO_IS_OVERRIDE_ENABLED
         2.  D_DMACRO_IS_MSVC_COMPAT
         3.  D_DMACRO_USES_ENV_DETECTION
         4.  D_DMACRO_CHECK_VARG_LIMIT
         5.  D_DMACRO_CHECK_PAIR_LIMIT
         6.  D_DMACRO_CHECK_TRIPLE_LIMIT
         7.  D_DMACRO_CHECK_4TUPLE_LIMIT
    6.  Feature include configuration
         1.  D_CFG_DMACRO_INCLUDE_FOR_EACH
         2.  D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR
         3.  D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR
         4.  D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR
         5.  D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE
         6.  D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR
         7.  D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR
         8.  D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA
         9.  D_CFG_DMACRO_INCLUDE_TUPLE_1 through D_CFG_DMACRO_INCLUDE_TUPLE_16
    7.  Variant file includes
         1.  Variant 64
         2.  Variant 127 (MSVC traditional preprocessor)
         3.  Variant 128
         4.  Variant 256
         5.  Variant 512
         6.  Variant 1024

2.  BASIC TOKEN MANIPULATION
    ------------------------
    1.  Token pasting
         1.  D_INTERNAL_CONCAT_HELPER
         2.  D_CONCAT
    2.  Stringification
         1.  D_STRINGIFY
         2.  D_TOSTR
    3.  Expansion control
         1.  D_EXPAND
         2.  D_EMPTY
         3.  D_DEFER
         4.  D_OBSTRUCT
         5.  D_UNPACK
    4.  Separator tokens
         1.  D_SEPARATOR_COMMA
         2.  D_SEPARATOR_SEMICOLON
         3.  D_SEPARATOR_SPACE

3.  ARRAY UTILITIES
    ---------------
    1.  Compile-time array sizing
         1.  D_ARRAY_COUNT
         2.  D_ARRAY_COUNT_SAFE
         3.  D_ARRAY_COUNT_T
    2.  Array generation
         1.  D_MAKE_ARRAY
         2.  D_MAKE_STRING_ARRAY

4.  ARGUMENT SELECTION
    ------------------
    1.  Dynamic position access
         1.  D_INTERNAL_VARG_GET_N
         2.  D_VARG_LAST
    2.  Positional accessors
         1.  D_VARG_GET_FIRST through D_VARG_GET_TENTH
    3.  List operations
         1.  D_HEAD
         2.  D_REST
         3.  D_TAIL
    4.  Parentheses handling
         1.  D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER
         2.  D_VARGS_REMOVE_PARENTHESES

5.  MACRO EXPANSION AND EVALUATION
    ------------------------------
    1.  Cascading expansion helpers
         1.  D_INTERNAL_EVAL_0001 through D_INTERNAL_EVAL_1024
    2.  Public evaluation macros
         1.  D_EVAL

6.  BOOLEAN AND CONDITIONAL LOGIC
    -----------------------------
    1.  Probe mechanism
         1.  D_PROBE / D_CHECK_IMPL / D_CHECK
    2.  Parentheses detection
         1.  D_IS_PAREN_PROBE / D_IS_PAREN_WRAPPER / D_IS_PAREN
    3.  Meta wrapper
         1.  D_META
    4.  Conditional expansion
         1.  D_IF family
    5.  Immediate if
         1.  D_IIF family
    6.  Boolean negation
         1.  D_NOT family
         2.  D_COMPL family
    7.  Boolean normalization
         1.  D_BOOL
    8.  Logical operations
         1.  D_AND family
         2.  D_OR family

7.  CORE ITERATION INFRASTRUCTURE
    -----------------------------
    1.  Increment
         1.  D_INC
    2.  Map termination
         1.  D_INTERNAL_MAP_END / D_INTERNAL_MAP_OUT
         2.  D_INTERNAL_MAP_GET_END
         3.  D_INTERNAL_MAP_NEXT0 / D_INTERNAL_MAP_NEXT1 / D_INTERNAL_MAP_NEXT
    3.  Data-passing map helpers
         1.  D_INTERNAL_MAP_DATA0 / D_INTERNAL_MAP_DATA1
    4.  Indexed map helpers
         1.  D_INTERNAL_MAP_IDX0 / D_INTERNAL_MAP_IDX1

8.  FOR_EACH UNIFIED INTERFACE
    --------------------------
    1.  Single-element iteration
         1.  D_FOR_EACH / D_FOR_EACH_SPACE
         2.  D_FOR_EACH_SEP
         3.  D_FOR_EACH_COMMA
         4.  D_FOR_EACH_SEMICOLON
    2.  Pair iteration
         1.  D_INTERNAL_PAIR_COUNT
         2.  D_FOR_EACH_PAIR / D_FOR_EACH_PAIR_SPACE
         3.  D_FOR_EACH_PAIR_COMMA
         4.  D_FOR_EACH_PAIR_SEMICOLON
         5.  D_FOR_EACH_PAIR_SEP
    3.  Triple iteration
         1.  D_INTERNAL_TRIPLE_COUNT
         2.  D_FOR_EACH_TRIPLE
         3.  D_FOR_EACH_TRIPLE_COMMA
         4.  D_FOR_EACH_TRIPLE_SEP
    4.  4-tuple iteration
         1.  D_INTERNAL_4TUPLE_COUNT
         2.  D_FOR_EACH_4TUPLE
         3.  D_FOR_EACH_4TUPLE_COMMA
         4.  D_FOR_EACH_4TUPLE_SEP
    5.  With data parameter
         1.  D_FOR_EACH_DATA
    6.  Indexed iteration
         1.  D_FOR_EACH_INDEXED
    7.  Generic N-tuple dispatch
         1.  D_FOR_EACH_NTUPLE family
              a.  D_FOR_EACH_NTUPLE_COMMA
              b.  D_FOR_EACH_NTUPLE_SEP
         2.  D_FOR_EACH_1TUPLE through D_FOR_EACH_3TUPLE aliases

9.  MEMBER ACCESS ITERATION
    -----------------------
    1.  Pointer member access
         1.  D_INTERNAL_MEMBER_PTR_OP / D_INTERNAL_MEMBER_PTR_OP_EXPAND
         2.  D_FOR_EACH_MEMBER_PTR
    2.  Direct member access
         1.  D_INTERNAL_MEMBER_DOT_OP / D_INTERNAL_MEMBER_DOT_OP_EXPAND
         2.  D_FOR_EACH_MEMBER_DOT

10. ADVANCED ITERATION PATTERNS
    ---------------------------
    1.  Adjacent pair iteration
         1.  D_INTERNAL_MAP_ADJ0 / D_INTERNAL_MAP_ADJ1
         2.  D_FOR_EACH_ADJACENT_PAIR

11. POINTER ARRAY INITIALIZATION
    ----------------------------
    1.  Data-comma iteration
         1.  D_INTERNAL_MAP_DATA_COMMA0 / D_INTERNAL_MAP_DATA_COMMA1
         2.  D_FOR_EACH_DATA_COMMA
    2.  Struct array helpers
         1.  D_INTERNAL_PTR_ELEM
         2.  D_INTERNAL_TUPLE_TO_BRACES
         3.  D_STRUCT_ARRAY_INIT

12. UTILITY OPERATORS
    -----------------
    1.  Debug/test operators
         1.  D_PRINT_OP / D_PRINT_VAL_OP
         2.  D_DECLARE_VAR_OP / D_DECLARE_TYPED_OP
         3.  D_ASSIGN_OP / D_INIT_ZERO_OP

13. COMPILE-TIME ASSERTIONS
    -----------------------
    1.  Size/type checks
         1.  D_ASSERT_SAME_SIZE
*/

#ifndef DJINTERP_С_MACRO_
#define DJINTERP_С_MACRO_ 1

// std
#include <stddef.h>       // size_t
// djinterp
#include "../env/env.h"   // environment detection


//==============================================================================
// 1.  CONFIGURATION SYSTEM
//==============================================================================

// Controls variadic argument limits, macro variant selection, and provides
// user-overridable settings for maximum flexibility.
//
// CONFIGURATION HIERARCHY (highest to lowest priority):
//   1. D_CFG_DMACRO_OVERRIDE - if 1, use D_CFG_DMACRO_* values directly
//   2. D_CFG_DMACRO_VARG_MAX - user-specified max (if override enabled)
//   3. D_ENV_PP_MAX_MACRO_ARGS - environment-detected limit
//   4. D_CFG_DMACRO_VARG_DEFAULT (128) - fallback default
//

// 1.1    Configuration constants
//------------------------------------------------------------------------------

// 1.1.1  Variadic argument limits
//     a.
// D_CFG_DMACRO_VARG_DEFAULT
//   constant: default maximum variadic argument count (128).
#define D_CFG_DMACRO_VARG_DEFAULT 128

//     b.
// D_CFG_DMACRO_VARG_MIN
//   constant: minimum supported variadic argument count (64).
#define D_CFG_DMACRO_VARG_MIN     64

//     c.
// D_CFG_DMACRO_VARG_LIMIT
//   constant: absolute maximum supported by the framework (1024).
#define D_CFG_DMACRO_VARG_LIMIT   1024

// 1.1.2  Variant selection constants
//   constants: supported generated-header variant levels.
#define D_CFG_DMACRO_VARIANT_64   64
#define D_CFG_DMACRO_VARIANT_128  128
#define D_CFG_DMACRO_VARIANT_256  256
#define D_CFG_DMACRO_VARIANT_512  512
#define D_CFG_DMACRO_VARIANT_1024 1024


// 1.2    User configuration
//------------------------------------------------------------------------------

// 1.2.1
// D_CFG_DMACRO_OVERRIDE
//   macro: master override flag for dmacro configuration.
#ifndef D_CFG_DMACRO_OVERRIDE
    #define D_CFG_DMACRO_OVERRIDE 0
#endif  // D_CFG_DMACRO_OVERRIDE

// 1.2.2
// D_CFG_DMACRO_VARG_MAX
//   macro: user-specified maximum variadic argument count.
#ifndef D_CFG_DMACRO_VARG_MAX
    #define D_CFG_DMACRO_VARG_MAX D_CFG_DMACRO_VARG_DEFAULT
#endif  // D_CFG_DMACRO_VARG_MAX

// 1.2.3
// D_CFG_DMACRO_USE_MSVC_COMPAT
//   macro: enables MSVC-compatible preprocessor limits.
#ifndef D_CFG_DMACRO_USE_MSVC_COMPAT
    #if ( defined(_MSC_VER) &&                                               \
          !defined(__clang__) )
        #if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
            #define D_CFG_DMACRO_USE_MSVC_COMPAT 1
        #else
            #define D_CFG_DMACRO_USE_MSVC_COMPAT 0
        #endif
    #else
        #define D_CFG_DMACRO_USE_MSVC_COMPAT 0
    #endif
#endif  // D_CFG_DMACRO_USE_MSVC_COMPAT


// 1.3    Effective configuration
//------------------------------------------------------------------------------

// 1.3.1
// D_INTERNAL_DMACRO_RAW_MAX
//   macro (internal): raw configured variadic-argument maximum.
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

// 1.3.2
// D_INTERNAL_DMACRO_CLAMPED_MAX
//   macro (internal): raw maximum clamped to the supported range.
#if (D_INTERNAL_DMACRO_RAW_MAX < D_CFG_DMACRO_VARG_MIN)
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_CFG_DMACRO_VARG_MIN
#elif (D_INTERNAL_DMACRO_RAW_MAX > D_CFG_DMACRO_VARG_LIMIT)
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_CFG_DMACRO_VARG_LIMIT
#else
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_INTERNAL_DMACRO_RAW_MAX
#endif

// 1.3.3
// D_DMACRO_VARIANT / D_DMACRO_*_MAX
//   macro family: effective variant and arity limits.
// MSVC traditional preprocessor compatibility.
#if (D_CFG_DMACRO_USE_MSVC_COMPAT == 1)
    #define D_DMACRO_VARIANT      127
    #define D_DMACRO_VARG_MAX     127
    #define D_DMACRO_PAIR_MAX     63
    #define D_DMACRO_TRIPLE_MAX   42
    #define D_DMACRO_4TUPLE_MAX   31

// round to the nearest supported generated-header variant.
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


// 1.4    Public aliases
//------------------------------------------------------------------------------

// 1.4.1
// D_CFG_VARG_COUNT_MAX / D_VARG_COUNT_MAX
//   macro aliases: maximum number of variadic arguments supported.
#define D_CFG_VARG_COUNT_MAX D_DMACRO_VARG_MAX
#define D_VARG_COUNT_MAX     D_DMACRO_VARG_MAX

// 1.4.2
// D_CFG_FOR_EACH_MAX / D_FOR_EACH_MAX
//   macro aliases: maximum elements D_FOR_EACH can iterate over.
#define D_CFG_FOR_EACH_MAX D_DMACRO_VARG_MAX
#define D_FOR_EACH_MAX     D_DMACRO_VARG_MAX

// 1.4.3
// D_CFG_FOR_EACH_PAIR_MAX / D_FOR_EACH_PAIR_MAX
//   macro aliases: maximum pairs D_FOR_EACH_PAIR can iterate over.
#define D_CFG_FOR_EACH_PAIR_MAX D_DMACRO_PAIR_MAX
#define D_FOR_EACH_PAIR_MAX     D_DMACRO_PAIR_MAX

// 1.4.4
// D_CFG_FOR_EACH_TRIPLE_MAX / D_FOR_EACH_TRIPLE_MAX
//   macro aliases: maximum triples D_FOR_EACH_TRIPLE can iterate over.
#define D_CFG_FOR_EACH_TRIPLE_MAX D_DMACRO_TRIPLE_MAX
#define D_FOR_EACH_TRIPLE_MAX     D_DMACRO_TRIPLE_MAX

// 1.4.5
// D_CFG_FOR_EACH_4TUPLE_MAX / D_FOR_EACH_4TUPLE_MAX
//   macro aliases: maximum 4-tuples D_FOR_EACH_4TUPLE can iterate over.
#define D_CFG_FOR_EACH_4TUPLE_MAX D_DMACRO_4TUPLE_MAX
#define D_FOR_EACH_4TUPLE_MAX     D_DMACRO_4TUPLE_MAX

// 1.4.6
// D_CFG_MACRO_VARIANT / D_MACRO_VARIANT
//   macro aliases: identifies the selected generated-header variant.
#define D_CFG_MACRO_VARIANT D_DMACRO_VARIANT
#define D_MACRO_VARIANT     D_DMACRO_VARIANT

// 1.4.7
// D_VARG_PAIR_MAX
//   macro alias: legacy spelling for D_DMACRO_PAIR_MAX.
#define D_VARG_PAIR_MAX D_DMACRO_PAIR_MAX


// 1.5    Query and limit macros
//------------------------------------------------------------------------------

// 1.5.1
// D_DMACRO_IS_OVERRIDE_ENABLED
//   macro: evaluates to 1 when override mode is active; otherwise 0.
#define D_DMACRO_IS_OVERRIDE_ENABLED() (D_CFG_DMACRO_OVERRIDE == 1)

// 1.5.2
// D_DMACRO_IS_MSVC_COMPAT
//   macro: evaluates to 1 in MSVC compatibility mode; otherwise 0.
#define D_DMACRO_IS_MSVC_COMPAT() (D_CFG_DMACRO_USE_MSVC_COMPAT == 1)

// 1.5.3
// D_DMACRO_USES_ENV_DETECTION
//   macro: evaluates to 1 when environment detection supplies the limit.
#if (D_CFG_DMACRO_OVERRIDE == 1)
    #define D_DMACRO_USES_ENV_DETECTION() 0
#elif defined(D_ENV_PP_MAX_MACRO_ARGS)
    #define D_DMACRO_USES_ENV_DETECTION() 1
#else
    #define D_DMACRO_USES_ENV_DETECTION() 0
#endif

// 1.5.4
// D_DMACRO_CHECK_VARG_LIMIT
//   macro: tests whether an arity is within the variadic limit.
#define D_DMACRO_CHECK_VARG_LIMIT(n)                                          \
    ((n) <= D_DMACRO_VARG_MAX)

// 1.5.5
// D_DMACRO_CHECK_PAIR_LIMIT
//   macro: tests whether an arity is within the pair limit.
#define D_DMACRO_CHECK_PAIR_LIMIT(n)                                          \
    ((n) <= D_DMACRO_PAIR_MAX)

// 1.5.6
// D_DMACRO_CHECK_TRIPLE_LIMIT
//   macro: tests whether an arity is within the triple limit.
#define D_DMACRO_CHECK_TRIPLE_LIMIT(n)                                        \
    ((n) <= D_DMACRO_TRIPLE_MAX)

// 1.5.7
// D_DMACRO_CHECK_4TUPLE_LIMIT
//   macro: tests whether an arity is within the 4-tuple limit.
#define D_DMACRO_CHECK_4TUPLE_LIMIT(n)                                        \
    ((n) <= D_DMACRO_4TUPLE_MAX)


// 1.6    Feature include configuration
//------------------------------------------------------------------------------
//
// Each generated macro family and tuple arity can be independently
// enabled (1) or disabled (0).  All default to enabled.
// Define any of these BEFORE including dmacro.h to override.
//
// Example - keep only what is needed:
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

// 1.6.1
// D_CFG_DMACRO_INCLUDE_FOR_EACH
//   macro: enables the generated single-element iteration family.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH

// 1.6.2
// D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR
//   macro: enables the generated separator iteration family.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR

// 1.6.3
// D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR
//   macro: enables the generated pair iteration family.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR

// 1.6.4
// D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR
//   macro: enables generated pair iteration with separators.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR

// 1.6.5
// D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE
//   macro: enables the generated triple iteration family.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE

// 1.6.6
// D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR
//   macro: enables generated triple iteration with separators.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR

// 1.6.7
// D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR
//   macro: enables generated data-plus-separator iteration.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR

// 1.6.8
// D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA
//   macro: enables the generated comma-separated iteration family.
#ifndef D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA
    #define D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA 1
#endif  // D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA

// 1.6.9
// D_CFG_DMACRO_INCLUDE_TUPLE_1 through D_CFG_DMACRO_INCLUDE_TUPLE_16
//   macro family: enables generated tuple-iteration families by arity.
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_1
    #define D_CFG_DMACRO_INCLUDE_TUPLE_1  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_1
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_2
    #define D_CFG_DMACRO_INCLUDE_TUPLE_2  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_2
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_3
    #define D_CFG_DMACRO_INCLUDE_TUPLE_3  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_3
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_4
    #define D_CFG_DMACRO_INCLUDE_TUPLE_4  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_4
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_5
    #define D_CFG_DMACRO_INCLUDE_TUPLE_5  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_5
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_6
    #define D_CFG_DMACRO_INCLUDE_TUPLE_6  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_6
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_7
    #define D_CFG_DMACRO_INCLUDE_TUPLE_7  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_7
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_8
    #define D_CFG_DMACRO_INCLUDE_TUPLE_8  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_8
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_9
    #define D_CFG_DMACRO_INCLUDE_TUPLE_9  1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_9
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_10
    #define D_CFG_DMACRO_INCLUDE_TUPLE_10 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_10
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_11
    #define D_CFG_DMACRO_INCLUDE_TUPLE_11 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_11
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_12
    #define D_CFG_DMACRO_INCLUDE_TUPLE_12 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_12
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_13
    #define D_CFG_DMACRO_INCLUDE_TUPLE_13 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_13
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_14
    #define D_CFG_DMACRO_INCLUDE_TUPLE_14 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_14
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_15
    #define D_CFG_DMACRO_INCLUDE_TUPLE_15 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_15
#ifndef D_CFG_DMACRO_INCLUDE_TUPLE_16
    #define D_CFG_DMACRO_INCLUDE_TUPLE_16 1
#endif  // D_CFG_DMACRO_INCLUDE_TUPLE_16


// 1.7    Variant file includes
//------------------------------------------------------------------------------
//
// Each .h contains exactly one macro family (FOO_0 .. FOO_N).
// Core utilities are always pulled in; everything else honours
// the section 1.6 toggles.
//
// generated file naming (C-Macro-Generator.ps1 --generate-all-modules):
//   for_each{N}.h               for_each_mvc.h
//   for_each_separator{N}.h      for_each_separator_mvc.h
//   for_each_pair{N}.h           for_each_pair_mvc.h
//   for_each_comma{N}.h          for_each_comma_mvc.h
//   for_each_{K}_tuple_sep{N}.h  for_each_{K}_tuple_sep_mvc.h
//   for_each_{K}_tuple_comma{N}.h for_each_{K}_tuple_comma_mvc.h

// 1.7.1  Variant 64
#if (D_DMACRO_VARIANT == 64)
    #include "./util/macro/varg_has_args64.h"                       // variadic emptiness detection
    #include "./util/macro/varg_get_arg64.h"                        // variadic argument access
    #include "./util/macro/inc64.h"                                 // token increment table
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include "./util/macro/for_each64.h"                        // generated iteration family
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include "./util/macro/for_each_separator64.h"              // generated separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include "./util/macro/for_each_pair64.h"                   // generated pair iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include "./util/macro/for_each_pair_separator64.h"         // generated pair-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include "./util/macro/for_each_triple64.h"                 // generated triple iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include "./util/macro/for_each_triple_separator64.h"       // generated triple-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include "./util/macro/for_each_data_separator64.h"         // generated data-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include "./util/macro/for_each_comma64.h"                  // generated comma iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include "./util/macro/for_each_1_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_1_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include "./util/macro/for_each_2_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_2_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include "./util/macro/for_each_3_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_3_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include "./util/macro/for_each_4_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_4_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include "./util/macro/for_each_5_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_5_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include "./util/macro/for_each_6_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_6_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include "./util/macro/for_each_7_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_7_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include "./util/macro/for_each_8_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_8_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include "./util/macro/for_each_9_tuple_sep64.h"            // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_9_tuple_comma64.h"      // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include "./util/macro/for_each_10_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_10_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include "./util/macro/for_each_11_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_11_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include "./util/macro/for_each_12_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_12_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include "./util/macro/for_each_13_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_13_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include "./util/macro/for_each_14_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_14_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include "./util/macro/for_each_15_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_15_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include "./util/macro/for_each_16_tuple_sep64.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_16_tuple_comma64.h"     // generated tuple-comma iteration
        #endif
    #endif

// 1.7.2  Variant 127 (MSVC traditional preprocessor)
#elif (D_DMACRO_VARIANT == 127)
    #include "./util/macro/varg_has_args_mvc.h"                     // variadic emptiness detection
    #include "./util/macro/varg_get_arg_mvc.h"                      // variadic argument access
    #include "./util/macro/inc_mvc.h"                               // token increment table
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include "./util/macro/for_each_mvc.h"                      // generated iteration family
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include "./util/macro/for_each_separator_mvc.h"            // generated separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include "./util/macro/for_each_pair_mvc.h"                 // generated pair iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include "./util/macro/for_each_pair_separator_mvc.h"       // generated pair-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include "./util/macro/for_each_triple_mvc.h"               // generated triple iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include "./util/macro/for_each_triple_separator_mvc.h"     // generated triple-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include "./util/macro/for_each_data_separator_mvc.h"       // generated data-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include "./util/macro/for_each_comma_mvc.h"                // generated comma iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include "./util/macro/for_each_1_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_1_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include "./util/macro/for_each_2_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_2_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include "./util/macro/for_each_3_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_3_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include "./util/macro/for_each_4_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_4_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include "./util/macro/for_each_5_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_5_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include "./util/macro/for_each_6_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_6_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include "./util/macro/for_each_7_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_7_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include "./util/macro/for_each_8_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_8_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include "./util/macro/for_each_9_tuple_sep_mvc.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_9_tuple_comma_mvc.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include "./util/macro/for_each_10_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_10_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include "./util/macro/for_each_11_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_11_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include "./util/macro/for_each_12_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_12_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include "./util/macro/for_each_13_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_13_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include "./util/macro/for_each_14_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_14_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include "./util/macro/for_each_15_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_15_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include "./util/macro/for_each_16_tuple_sep_mvc.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_16_tuple_comma_mvc.h"   // generated tuple-comma iteration
        #endif
    #endif

// 1.7.3  Variant 128
#elif (D_DMACRO_VARIANT == 128)
    #include "./util/macro/varg_count128.h"                         // variadic argument count
    #include "./util/macro/varg_has_args128.h"                      // variadic emptiness detection
    #include "./util/macro/varg_get_arg128.h"                       // variadic argument access
    #include "./util/macro/inc128.h"                                // token increment table
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include "./util/macro/for_each128.h"                       // generated iteration family
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include "./util/macro/for_each_separator128.h"             // generated separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include "./util/macro/for_each_pair128.h"                  // generated pair iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include "./util/macro/for_each_pair_separator128.h"        // generated pair-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include "./util/macro/for_each_triple128.h"                // generated triple iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include "./util/macro/for_each_triple_separator128.h"      // generated triple-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include "./util/macro/for_each_data_separator128.h"        // generated data-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include "./util/macro/for_each_comma128.h"                 // generated comma iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include "./util/macro/for_each_1_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_1_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include "./util/macro/for_each_2_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_2_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include "./util/macro/for_each_3_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_3_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include "./util/macro/for_each_4_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_4_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include "./util/macro/for_each_5_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_5_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include "./util/macro/for_each_6_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_6_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include "./util/macro/for_each_7_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_7_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include "./util/macro/for_each_8_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_8_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include "./util/macro/for_each_9_tuple_sep128.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_9_tuple_comma128.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include "./util/macro/for_each_10_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_10_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include "./util/macro/for_each_11_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_11_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include "./util/macro/for_each_12_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_12_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include "./util/macro/for_each_13_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_13_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include "./util/macro/for_each_14_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_14_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include "./util/macro/for_each_15_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_15_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include "./util/macro/for_each_16_tuple_sep128.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_16_tuple_comma128.h"    // generated tuple-comma iteration
        #endif
    #endif

// 1.7.4  Variant 256
#elif (D_DMACRO_VARIANT == 256)
    #include "./util/macro/varg_has_args256.h"                      // variadic emptiness detection
    #include "./util/macro/varg_get_arg256.h"                       // variadic argument access
    #include "./util/macro/inc256.h"                                // token increment table
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include "./util/macro/for_each256.h"                       // generated iteration family
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include "./util/macro/for_each_separator256.h"             // generated separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include "./util/macro/for_each_pair256.h"                  // generated pair iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include "./util/macro/for_each_pair_separator256.h"        // generated pair-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include "./util/macro/for_each_triple256.h"                // generated triple iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include "./util/macro/for_each_triple_separator256.h"      // generated triple-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include "./util/macro/for_each_data_separator256.h"        // generated data-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include "./util/macro/for_each_comma256.h"                 // generated comma iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include "./util/macro/for_each_1_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_1_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include "./util/macro/for_each_2_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_2_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include "./util/macro/for_each_3_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_3_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include "./util/macro/for_each_4_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_4_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include "./util/macro/for_each_5_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_5_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include "./util/macro/for_each_6_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_6_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include "./util/macro/for_each_7_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_7_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include "./util/macro/for_each_8_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_8_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include "./util/macro/for_each_9_tuple_sep256.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_9_tuple_comma256.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include "./util/macro/for_each_10_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_10_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include "./util/macro/for_each_11_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_11_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include "./util/macro/for_each_12_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_12_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include "./util/macro/for_each_13_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_13_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include "./util/macro/for_each_14_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_14_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include "./util/macro/for_each_15_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_15_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include "./util/macro/for_each_16_tuple_sep256.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_16_tuple_comma256.h"    // generated tuple-comma iteration
        #endif
    #endif

// 1.7.5  Variant 512
#elif (D_DMACRO_VARIANT == 512)
    #include "./util/macro/varg_has_args512.h"                      // variadic emptiness detection
    #include "./util/macro/varg_get_arg512.h"                       // variadic argument access
    #include "./util/macro/inc512.h"                                // token increment table
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include "./util/macro/for_each512.h"                       // generated iteration family
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include "./util/macro/for_each_separator512.h"             // generated separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include "./util/macro/for_each_pair512.h"                  // generated pair iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include "./util/macro/for_each_pair_separator512.h"        // generated pair-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include "./util/macro/for_each_triple512.h"                // generated triple iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include "./util/macro/for_each_triple_separator512.h"      // generated triple-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include "./util/macro/for_each_data_separator512.h"        // generated data-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include "./util/macro/for_each_comma512.h"                 // generated comma iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include "./util/macro/for_each_1_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_1_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include "./util/macro/for_each_2_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_2_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include "./util/macro/for_each_3_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_3_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include "./util/macro/for_each_4_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_4_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include "./util/macro/for_each_5_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_5_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include "./util/macro/for_each_6_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_6_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include "./util/macro/for_each_7_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_7_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include "./util/macro/for_each_8_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_8_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include "./util/macro/for_each_9_tuple_sep512.h"           // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_9_tuple_comma512.h"     // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include "./util/macro/for_each_10_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_10_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include "./util/macro/for_each_11_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_11_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include "./util/macro/for_each_12_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_12_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include "./util/macro/for_each_13_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_13_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include "./util/macro/for_each_14_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_14_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include "./util/macro/for_each_15_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_15_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include "./util/macro/for_each_16_tuple_sep512.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_16_tuple_comma512.h"    // generated tuple-comma iteration
        #endif
    #endif

// 1.7.6  Variant 1024
#else  // 1024
    #include "./util/macro/varg_has_args1024.h"                     // variadic emptiness detection
    #include "./util/macro/varg_get_arg1024.h"                      // variadic argument access
    #include "./util/macro/inc1024.h"                               // token increment table
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH == 1)
        #include "./util/macro/for_each1024.h"                      // generated iteration family
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR == 1)
        #include "./util/macro/for_each_separator1024.h"            // generated separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR == 1)
        #include "./util/macro/for_each_pair1024.h"                 // generated pair iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR == 1)
        #include "./util/macro/for_each_pair_separator1024.h"       // generated pair-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE == 1)
        #include "./util/macro/for_each_triple1024.h"               // generated triple iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR == 1)
        #include "./util/macro/for_each_triple_separator1024.h"     // generated triple-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR == 1)
        #include "./util/macro/for_each_data_separator1024.h"       // generated data-separator iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
        #include "./util/macro/for_each_comma1024.h"                // generated comma iteration
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_1 == 1)
        #include "./util/macro/for_each_1_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_1_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_2 == 1)
        #include "./util/macro/for_each_2_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_2_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_3 == 1)
        #include "./util/macro/for_each_3_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_3_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_4 == 1)
        #include "./util/macro/for_each_4_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_4_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_5 == 1)
        #include "./util/macro/for_each_5_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_5_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_6 == 1)
        #include "./util/macro/for_each_6_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_6_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_7 == 1)
        #include "./util/macro/for_each_7_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_7_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_8 == 1)
        #include "./util/macro/for_each_8_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_8_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_9 == 1)
        #include "./util/macro/for_each_9_tuple_sep1024.h"          // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_9_tuple_comma1024.h"    // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_10 == 1)
        #include "./util/macro/for_each_10_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_10_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_11 == 1)
        #include "./util/macro/for_each_11_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_11_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_12 == 1)
        #include "./util/macro/for_each_12_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_12_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_13 == 1)
        #include "./util/macro/for_each_13_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_13_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_14 == 1)
        #include "./util/macro/for_each_14_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_14_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_15 == 1)
        #include "./util/macro/for_each_15_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_15_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
    #if (D_CFG_DMACRO_INCLUDE_TUPLE_16 == 1)
        #include "./util/macro/for_each_16_tuple_sep1024.h"         // generated tuple-separator iteration
        #if (D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA == 1)
            #include "./util/macro/for_each_16_tuple_comma1024.h"   // generated tuple-comma iteration
        #endif
    #endif
#endif


//==============================================================================
// 2.  BASIC TOKEN MANIPULATION
//==============================================================================


// 2.1    Token pasting
//------------------------------------------------------------------------------

// 2.1.1
// D_INTERNAL_CONCAT_HELPER
//   macro (internal): performs actual token concatenation after expansion
#define D_INTERNAL_CONCAT_HELPER(a, b) a##b

// 2.1.2
// D_CONCAT
//   macro: token-paste two identifiers with macro expansion.
#define D_CONCAT(a, b)                                                        \
    D_INTERNAL_CONCAT_HELPER(a, b)

// 2.2    Stringification
//------------------------------------------------------------------------------

// 2.2.1
// D_STRINGIFY
//   macro: convert argument to string literal without expanding it first.
#define D_STRINGIFY(x)                                                        \
    #x

// 2.2.2
// D_TOSTR
//   macro: stringify with one macro expansion pass (expand then stringify).
#define D_TOSTR(x)                                                            \
    D_STRINGIFY(x)

// 2.3    Expansion control
//------------------------------------------------------------------------------

// 2.3.1
// D_EXPAND
//   macro: force one additional macro expansion pass.
#define D_EXPAND(x)                                                           \
    x

// 2.3.2
// D_EMPTY
//   macro: expands to nothing, useful for conditional expansion.
#define D_EMPTY()

// 2.3.3
// D_DEFER
//   macro: defers macro expansion by one pass.
#define D_DEFER(id)                                                           \
    id D_EMPTY()

// 2.3.4
// D_OBSTRUCT
//   macro: defers macro expansion by two passes.
#define D_OBSTRUCT(...)                                                       \
    __VA_ARGS__ D_DEFER(D_EMPTY)()

// 2.3.5
// D_UNPACK
//   macro: remove parentheses from arguments.
#define D_UNPACK(...)                                                         \
    __VA_ARGS__

// 2.4    Separator tokens
//------------------------------------------------------------------------------

// 2.4.1
// D_SEPARATOR_COMMA
//   macro: expands to a comma separator.
#define D_SEPARATOR_COMMA ,

// 2.4.2
// D_SEPARATOR_SEMICOLON
//   macro: expands to a semicolon separator.
#define D_SEPARATOR_SEMICOLON ;

// 2.4.3
// D_SEPARATOR_SPACE
//   macro: expands to an empty separator token sequence.
#define D_SEPARATOR_SPACE


//==============================================================================
// 3.  ARRAY UTILITIES
//==============================================================================


// 3.1    Compile-time array sizing
//------------------------------------------------------------------------------

// 3.1.1
// D_ARRAY_COUNT
//   macro: returns the number of elements in a statically allocated array.
#define D_ARRAY_COUNT(arr)                                                    \
    ( (sizeof(arr)) / (sizeof((arr)[0])) )

// 3.1.2
// D_ARRAY_COUNT_SAFE
//   macro: count variadic elements of specified type; returns 0 if empty.
#define D_ARRAY_COUNT_SAFE(type, ...)                                         \
    ( D_VARG_COUNT(__VA_ARGS__) == 0 ?                                        \
        0 :                                                                   \
        (sizeof((type[]){ __VA_ARGS__ }) / sizeof(type)) )

// 3.1.3
// D_ARRAY_COUNT_T
//   macro: count variadic elements of specified type using array size.
#define D_ARRAY_COUNT_T(type, ...)                                           \
    ( (sizeof((type[]){ __VA_ARGS__ }) / sizeof(type)) )

// 3.2    Array generation
//------------------------------------------------------------------------------

// 3.2.1
// D_MAKE_ARRAY
//   macro: generate array initialization.
// Usage: D_MAKE_ARRAY(int, nums, 1, 2, 3) -> int nums[] = { 1, 2, 3 }
// Note: Direct expansion for MSVC traditional preprocessor compatibility
#define D_MAKE_ARRAY(type, name, ...)                                         \
    type name[] = { __VA_ARGS__ }

// 3.2.2
// D_MAKE_STRING_ARRAY
//   macro: generate string array from identifiers.
// Usage: D_MAKE_STRING_ARRAY(names, foo, bar) ->
//          const char* names[] = { "foo", "bar" }
// Note: Uses D_FOR_EACH_COMMA with D_STRINGIFY for MSVC compatibility
#define D_MAKE_STRING_ARRAY(name, ...)                                        \
    const char* name[] = { D_FOR_EACH_COMMA(D_STRINGIFY, __VA_ARGS__) }


//==============================================================================
// 4.  ARGUMENT SELECTION
//==============================================================================


// 4.1    Dynamic position access
//------------------------------------------------------------------------------

// 4.1.1
// D_INTERNAL_VARG_GET_N
//   macro (internal): select arg N (where N is a number token)
#define D_INTERNAL_VARG_GET_N(_n, ...)                                        \
    D_CONCAT(D_VARG_GET_ARG_, _n)(__VA_ARGS__)

// 4.1.2
// D_VARG_LAST
//   macro: expands to the last argument in __VA_ARGS__.
#define D_VARG_LAST(...)                                                      \
    D_INTERNAL_VARG_GET_N(D_VARG_COUNT(__VA_ARGS__), __VA_ARGS__)

// 4.2    Positional accessors
//------------------------------------------------------------------------------

// 4.2.1
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

// 4.3    List operations
//------------------------------------------------------------------------------

// 4.3.1
// D_HEAD
//   macro alias: first variadic argument.
#define D_HEAD(...) D_VARG_GET_FIRST(__VA_ARGS__)

// 4.3.2
// D_REST
//   macro: expands to all arguments except the first.
#define D_REST(first, ...) __VA_ARGS__

// 4.3.3
// D_TAIL
//   macro alias: D_REST.
#define D_TAIL(first, ...) __VA_ARGS__


// 4.4    Parentheses handling
//------------------------------------------------------------------------------

// 4.4.1
// D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER
//   macro: removes one layer of parentheses from a variadic token sequence.
#define D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER(...) __VA_ARGS__

// 4.4.2
// D_VARGS_REMOVE_PARENTHESES
//   macro: removes one layer of parentheses from an argument.
#define D_VARGS_REMOVE_PARENTHESES(arg)                                       \
    D_INTERNAL_VARGS_REMOVE_PARENTHESES_HELPER arg


//==============================================================================
// 5.  MACRO EXPANSION AND EVALUATION
//==============================================================================


// 5.1    Cascading expansion helpers
//------------------------------------------------------------------------------

// 5.1.1
// D_INTERNAL_EVAL_0001 through D_INTERNAL_EVAL_1024
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

// 5.2    Public evaluation macros
//------------------------------------------------------------------------------

// 5.2.1
// D_EVAL
//   macro: forces multiple macro expansion passes (1024 iterations).
#define D_EVAL(...)  D_INTERNAL_EVAL_1024(__VA_ARGS__)


//==============================================================================
// 6.  BOOLEAN AND CONDITIONAL LOGIC
//==============================================================================


// 6.1    Probe mechanism
//------------------------------------------------------------------------------

// 6.1.1
// D_PROBE / D_CHECK_IMPL / D_CHECK
//   macro family: implements probe-based preprocessor detection.
#define D_PROBE(x) x, 1
#define D_CHECK_IMPL(x, n, ...) n
#define D_CHECK(...) D_CHECK_IMPL(__VA_ARGS__, 0,)

// 6.2    Parentheses detection
//------------------------------------------------------------------------------

// 6.2.1
// D_IS_PAREN_PROBE / D_IS_PAREN_WRAPPER / D_IS_PAREN
//   macro family: detects whether a token sequence begins with parentheses.
#define D_IS_PAREN_PROBE(...) D_PROBE(~)
#define D_IS_PAREN_WRAPPER(x) D_IS_PAREN_PROBE x
#define D_IS_PAREN(x) D_CHECK(D_IS_PAREN_WRAPPER(x))

// 6.3    Meta wrapper
//------------------------------------------------------------------------------

// 6.3.1
// D_META
//   macro: wrap arguments in double parentheses.
#define D_META(...) ((__VA_ARGS__))

// 6.4    Conditional expansion
//------------------------------------------------------------------------------

// 6.4.1
// D_IF family
//   macro family: conditionally expands a true branch and optional else branch.
#define D_IF(cond) D_CONCAT(D_IF_, cond)
#define D_IF_1(true_case) true_case D_IF_1_ELSE
#define D_IF_0(true_case) D_IF_0_ELSE
#define D_IF_1_ELSE(...)
#define D_IF_0_ELSE(...) __VA_ARGS__

// 6.5    Immediate if
//------------------------------------------------------------------------------

// 6.5.1
// D_IIF family
//   macro family: selects one of two branches from a normalized condition.
#define D_IIF(cond) D_CONCAT(D_IIF_, cond)
#define D_IIF_0(t, f) f
#define D_IIF_1(t, f) t

// 6.6    Boolean negation
//------------------------------------------------------------------------------

// 6.6.1
// D_NOT family
//   macro family: maps 0 to 1 and other tokens to 0.
#define D_NOT(x) D_CHECK(D_CONCAT(D_NOT_, x))
#define D_NOT_0 D_PROBE(~)

// 6.6.2
// D_COMPL family
//   macro family: complements a normalized 0/1 token.
#define D_COMPL(b) D_CONCAT(D_COMPL_, b)
#define D_COMPL_0 1
#define D_COMPL_1 0

// 6.7    Boolean normalization
//------------------------------------------------------------------------------

// 6.7.1
// D_BOOL
//   macro: normalizes a token to 0 or 1.
#define D_BOOL(x) D_COMPL(D_NOT(x))

// 6.8    Logical operations
//------------------------------------------------------------------------------

// 6.8.1
// D_AND family
//   macro family: logical conjunction on normalized boolean values.
#define D_AND(x, y) D_CONCAT(D_AND_, D_CONCAT(D_BOOL(x), D_BOOL(y)))
#define D_AND_00 0
#define D_AND_01 0
#define D_AND_10 0
#define D_AND_11 1

// 6.8.2
// D_OR family
//   macro family: logical disjunction on normalized boolean values.
#define D_OR(x, y) D_CONCAT(D_OR_, D_CONCAT(D_BOOL(x), D_BOOL(y)))
#define D_OR_00 0
#define D_OR_01 1
#define D_OR_10 1
#define D_OR_11 1


//==============================================================================
// 7.  CORE ITERATION INFRASTRUCTURE
//==============================================================================


// 7.1    Increment
//------------------------------------------------------------------------------

// 7.1.1
// D_INC
//   macro: increments a number token (0-63 -> 1-64).
#define D_INC(x)                                                            \
    D_CONCAT(D_INTERNAL_INC_, x)

// 7.2    Map termination
//------------------------------------------------------------------------------

// 7.2.1
// D_INTERNAL_MAP_END / D_INTERNAL_MAP_OUT
//   macro family: sentinel tokens used to terminate recursive map expansion.

// D_INTERNAL_MAP_END
//   macro: sentinel macro that terminates recursive map expansion.
#define D_INTERNAL_MAP_END(...)

// D_INTERNAL_MAP_OUT
//   macro: helper expansion token for map recursion (expands to nothing).
#define D_INTERNAL_MAP_OUT

// 7.2.2
// D_INTERNAL_MAP_GET_END
//   macro: expands to `0, D_INTERNAL_MAP_END` for end-marker detection.
#define D_INTERNAL_MAP_GET_END()                                            \
    0, D_INTERNAL_MAP_END

// 7.2.3
// D_INTERNAL_MAP_NEXT0 / D_INTERNAL_MAP_NEXT1 / D_INTERNAL_MAP_NEXT
//   macro family: selects the next recursive map worker.

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

// 7.3    Data-passing map helpers
//------------------------------------------------------------------------------

// 7.3.1
// D_INTERNAL_MAP_DATA0 / D_INTERNAL_MAP_DATA1
//   macro family: recursive workers that pass auxiliary data.

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

// 7.4    Indexed map helpers
//------------------------------------------------------------------------------

// 7.4.1
// D_INTERNAL_MAP_IDX0 / D_INTERNAL_MAP_IDX1
//   macro family: recursive workers that propagate a token index.

// D_INTERNAL_MAP_IDX0
//   macro: indexed mapping worker (even step); applies f(i, x), then
// increments i.
#define D_INTERNAL_MAP_IDX0(f, i, x, peek, ...)                             \
    f(i, x)                                                                 \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_IDX1)(f,                       \
                                                   D_INC(i),                \
                                                   peek,                    \
                                                   __VA_ARGS__)

// D_INTERNAL_MAP_IDX1
//   macro: indexed mapping worker (odd step); applies f(i, x), then
// increments i.
#define D_INTERNAL_MAP_IDX1(f, i, x, peek, ...)                             \
    f(i, x)                                                                 \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_IDX0)(f,                       \
                                                   D_INC(i),                \
                                                   peek,                    \
                                                   __VA_ARGS__)


//==============================================================================
// 8.  FOR_EACH UNIFIED INTERFACE
//==============================================================================

// 8.1    Single-element iteration
//------------------------------------------------------------------------------

// 8.1.1
// D_FOR_EACH / D_FOR_EACH_SPACE
//   macro family: applies a function-like macro to each element.
// Usage: D_FOR_EACH(fn, a, b, c) -> fn(a) fn(b) fn(c)
#define D_FOR_EACH(_fn, ...)                                                \
    D_CONCAT(D_INTERNAL_FOR_EACH_SEPARATOR_,                                \
             D_VARG_COUNT(__VA_ARGS__))(_fn, D_EMPTY, __VA_ARGS__)

#define D_FOR_EACH_SPACE(_fn, ...)                                          \
    D_FOR_EACH(_fn, __VA_ARGS__)

// 8.1.2
// D_FOR_EACH_SEP
//   macro: applies a function-like macro with a custom separator.
// Usage: D_FOR_EACH_SEP(;, fn, a, b, c) -> fn(a); fn(b); fn(c)
#define D_FOR_EACH_SEP(_sep, _fn, ...)                                      \
    D_CONCAT(D_INTERNAL_FOR_EACH_SEPARATOR_,                                \
             D_VARG_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)

// 8.1.3
// D_FOR_EACH_COMMA
//   macro: applies a function-like macro with comma separation.
#if defined(D_ENV_COMPILER_MSVC)
    #define D_FOR_EACH_COMMA(_fn, ...)                                      \
        D_CONCAT(D_INTERNAL_FOR_EACH_COMMA_,                                \
                 D_VARG_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)
#else
    #define D_FOR_EACH_COMMA(_fn, ...)                                      \
        D_FOR_EACH_SEP(D_SEPARATOR_COMMA, _fn, __VA_ARGS__)
#endif

// 8.1.4
// D_FOR_EACH_SEMICOLON
//   macro: applies a function-like macro with semicolon separation.
#define D_FOR_EACH_SEMICOLON(_fn, ...)                                      \
    D_FOR_EACH_SEP(D_SEPARATOR_SEMICOLON, _fn, __VA_ARGS__)


// 8.2    Pair iteration
//------------------------------------------------------------------------------

// 8.2.1
// D_INTERNAL_PAIR_COUNT
//   macro (internal): computes the number of pairs from an argument count.
#define D_INTERNAL_PAIR_COUNT(...)                                          \
    (D_VARG_COUNT(__VA_ARGS__) / 2)

// 8.2.2
// D_FOR_EACH_PAIR / D_FOR_EACH_PAIR_SPACE
//   macro family: applies a function-like macro to each pair.
// Usage: D_FOR_EACH_PAIR(fn, a, 1, b, 2) -> fn(a, 1) fn(b, 2)
#define D_FOR_EACH_PAIR(_fn, ...)                                           \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn,                       \
                                                 D_EMPTY,                   \
                                                 __VA_ARGS__)

#define D_FOR_EACH_PAIR_SPACE(_fn, ...)                                     \
    D_FOR_EACH_PAIR(_fn, __VA_ARGS__)

// 8.2.3
// D_FOR_EACH_PAIR_COMMA
//   macro: applies a function-like macro to each pair with comma separation.
#define D_FOR_EACH_PAIR_COMMA(_fn, ...)                                     \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn,                       \
                                                 D_SEPARATOR_COMMA,         \
                                                 __VA_ARGS__)
// 8.2.4
// D_FOR_EACH_PAIR_SEMICOLON
//   macro: applies a function-like macro to each pair with semicolon
// separation.
#define D_FOR_EACH_PAIR_SEMICOLON(_fn, ...)                                 \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn,                       \
                                                 D_SEPARATOR_SEMICOLON,     \
                                                 __VA_ARGS__)

// 8.2.5
// D_FOR_EACH_PAIR_SEP
//   macro: applies a function-like macro to each pair with a custom separator.
#define D_FOR_EACH_PAIR_SEP(_fn, _sep, ...)                                 \
    D_CONCAT(D_INTERNAL_FOR_EACH_2_TUPLE_SEPARATOR_,                        \
             D_INTERNAL_PAIR_COUNT(__VA_ARGS__))(_fn, _sep, __VA_ARGS__)


// 8.3    Triple iteration
//------------------------------------------------------------------------------

// 8.3.1
// D_INTERNAL_TRIPLE_COUNT
//   macro (internal): computes the number of triples from an argument count.
#define D_INTERNAL_TRIPLE_COUNT(...)                                        \
    (D_VARG_COUNT(__VA_ARGS__) / 3)

// 8.3.2
// D_FOR_EACH_TRIPLE
//   macro: applies a function-like macro to each triple.
// Usage: D_FOR_EACH_TRIPLE(fn, a, b, c, d, e, f) -> fn(a,b,c) fn(d,e,f)
#define D_FOR_EACH_TRIPLE(_fn, ...)                                         \
    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_,                                   \
             D_INTERNAL_TRIPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// 8.3.3
// D_FOR_EACH_TRIPLE_COMMA
//   macro: applies a function-like macro to each triple with comma separation.
#define D_FOR_EACH_TRIPLE_COMMA(_fn, ...)                                   \
    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_COMMA_,                             \
             D_INTERNAL_TRIPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// 8.3.4
// D_FOR_EACH_TRIPLE_SEP
//   macro: applies a function-like macro to each triple with a custom
// separator.
#define D_FOR_EACH_TRIPLE_SEP(_sep, _fn, ...)                               \
    D_CONCAT(D_INTERNAL_FOR_EACH_TRIPLE_SEP_,                               \
             D_INTERNAL_TRIPLE_COUNT(__VA_ARGS__))(_sep, _fn, __VA_ARGS__)


// 8.4    4-tuple iteration
//------------------------------------------------------------------------------

// 8.4.1
// D_INTERNAL_4TUPLE_COUNT
//   macro (internal): computes the number of 4-tuples from an argument count.
#define D_INTERNAL_4TUPLE_COUNT(...)                                        \
    (D_VARG_COUNT(__VA_ARGS__) / 4)

// 8.4.2
// D_FOR_EACH_4TUPLE
//   macro: applies a function-like macro to each 4-tuple.
// Usage: D_FOR_EACH_4TUPLE(fn, a,b,c,d, e,f,g,h) -> fn(a,b,c,d) fn(e,f,g,h)
#define D_FOR_EACH_4TUPLE(_fn, ...)                                         \
    D_CONCAT(D_INTERNAL_FOR_EACH_4TUPLE_,                                   \
             D_INTERNAL_4TUPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// 8.4.3
// D_FOR_EACH_4TUPLE_COMMA
//   macro: applies a function-like macro to each 4-tuple with comma separation.
#define D_FOR_EACH_4TUPLE_COMMA(_fn, ...)                                   \
    D_CONCAT(D_INTERNAL_FOR_EACH_4TUPLE_COMMA_,                             \
             D_INTERNAL_4TUPLE_COUNT(__VA_ARGS__))(_fn, __VA_ARGS__)

// 8.4.4
// D_FOR_EACH_4TUPLE_SEP
//   macro: applies a function-like macro to each 4-tuple with a custom
// separator.
#define D_FOR_EACH_4TUPLE_SEP(_sep, _fn, ...)                               \
    D_CONCAT(D_INTERNAL_FOR_EACH_4TUPLE_SEP_,                               \
             D_INTERNAL_4TUPLE_COUNT(__VA_ARGS__))(_sep, _fn, __VA_ARGS__)


// 8.5    With data parameter
//------------------------------------------------------------------------------

// 8.5.1
// D_FOR_EACH_DATA
//   macro: applies a function-like macro with auxiliary data.
// Usage: D_FOR_EACH_DATA(fn, ctx, a, b, c) -> fn(ctx,a) fn(ctx,b) fn(ctx,c)
#if ( defined(D_ENV_PP_HAS_VA_OPT_ENABLED) &&                                 \
      (D_ENV_PP_HAS_VA_OPT_ENABLED) )
    #define D_FOR_EACH_DATA(_fn, _data, ...)                                  \
        __VA_OPT__(D_EVAL(D_INTERNAL_MAP_DATA0(_fn, _data, __VA_ARGS__, (), 0)))
#else
    #define D_FOR_EACH_DATA(_fn, _data, ...)                                  \
        D_EVAL(D_INTERNAL_MAP_DATA0(_fn, _data, __VA_ARGS__, (), 0))
#endif


// 8.6    Indexed iteration
//------------------------------------------------------------------------------

// 8.6.1
// D_FOR_EACH_INDEXED
//   macro: applies a function-like macro with a zero-based token index.
// Usage: D_FOR_EACH_INDEXED(fn, a, b, c) -> fn(0, a) fn(1, b) fn(2, c)
#if ( defined(D_ENV_PP_HAS_VA_OPT_ENABLED) &&                                 \
      (D_ENV_PP_HAS_VA_OPT_ENABLED) )
    #define D_FOR_EACH_INDEXED(_fn, ...)                                      \
        __VA_OPT__(D_EVAL(D_INTERNAL_MAP_IDX0(_fn, 0, __VA_ARGS__, (), 0)))
#else
    #define D_FOR_EACH_INDEXED(_fn, ...)                                      \
        D_EVAL(D_INTERNAL_MAP_IDX0(_fn, 0, __VA_ARGS__, (), 0))
#endif


// 8.7    Generic N-tuple dispatch
//------------------------------------------------------------------------------

// 8.7.1
// D_FOR_EACH_NTUPLE
//   macro family: dispatches generic N-tuple iteration.
// Usage: D_FOR_EACH_NTUPLE(2, fn, a, b, c, d) ->
//        D_FOR_EACH_PAIR(fn, a, b, c, d)
#define D_FOR_EACH_NTUPLE(_n, _fn, ...)                                       \
    D_CONCAT(D_FOR_EACH_, D_CONCAT(_n, TUPLE))(_fn, __VA_ARGS__)

//     a.
// D_FOR_EACH_NTUPLE_COMMA
//   macro: generic N-tuple iteration with comma separators.
#define D_FOR_EACH_NTUPLE_COMMA(_n, _fn, ...)                                 \
    D_CONCAT(D_FOR_EACH_, D_CONCAT(_n, TUPLE_COMMA))(_fn, __VA_ARGS__)

//     b.
// D_FOR_EACH_NTUPLE_SEP
//   macro: generic N-tuple iteration with a supplied separator.
#define D_FOR_EACH_NTUPLE_SEP(_n, _sep, _fn, ...)                             \
    D_CONCAT(D_FOR_EACH_, D_CONCAT(_n, TUPLE_SEP))(_sep, _fn, __VA_ARGS__)

// 8.7.2
// D_FOR_EACH_1TUPLE through D_FOR_EACH_3TUPLE aliases
//   macro family: convenience aliases for supported tuple arities.
#define D_FOR_EACH_1TUPLE        D_FOR_EACH
#define D_FOR_EACH_1TUPLE_COMMA  D_FOR_EACH_COMMA
#define D_FOR_EACH_1TUPLE_SEP    D_FOR_EACH_SEP
#define D_FOR_EACH_2TUPLE        D_FOR_EACH_PAIR
#define D_FOR_EACH_2TUPLE_COMMA  D_FOR_EACH_PAIR_COMMA
#define D_FOR_EACH_2TUPLE_SEP    D_FOR_EACH_PAIR_SEP
#define D_FOR_EACH_3TUPLE        D_FOR_EACH_TRIPLE
#define D_FOR_EACH_3TUPLE_COMMA  D_FOR_EACH_TRIPLE_COMMA
#define D_FOR_EACH_3TUPLE_SEP    D_FOR_EACH_TRIPLE_SEP

//==============================================================================
// 9.  MEMBER ACCESS ITERATION
//==============================================================================


// 9.1    Pointer member access
//------------------------------------------------------------------------------

// 9.1.1
// D_INTERNAL_MEMBER_PTR_OP / D_INTERNAL_MEMBER_PTR_OP_EXPAND
//   macro family: expands pointer-member names across an iteration.

#define D_INTERNAL_MEMBER_PTR_OP(obj, member, x) obj->D_CONCAT(member, x)
#define D_INTERNAL_MEMBER_PTR_OP_EXPAND(_objmem, _x) \
    D_VARG_GET_FIRST _objmem->D_CONCAT(D_VARG_GET_SECOND _objmem, _x)


// 9.1.2
// D_FOR_EACH_MEMBER_PTR
//   macro: member access with -> operator.
// Usage: D_FOR_EACH_MEMBER_PTR(obj, field_, a, b) -> obj->field_a obj->field_b
#define D_FOR_EACH_MEMBER_PTR(_obj, _member, ...) \
    D_FOR_EACH_DATA(D_INTERNAL_MEMBER_PTR_OP_EXPAND,                         \
                    (_obj, _member),                                         \
                    __VA_ARGS__)

// 9.2    Direct member access
//------------------------------------------------------------------------------

// 9.2.1
// D_INTERNAL_MEMBER_DOT_OP / D_INTERNAL_MEMBER_DOT_OP_EXPAND
//   macro family: expands direct-member names across an iteration.

#define D_INTERNAL_MEMBER_DOT_OP(obj, member, x) obj.D_CONCAT(member, x)
#define D_INTERNAL_MEMBER_DOT_OP_EXPAND(_objmem, _x) \
    D_VARG_GET_FIRST _objmem.D_CONCAT(D_VARG_GET_SECOND _objmem, _x)


// 9.2.2
// D_FOR_EACH_MEMBER_DOT
//   macro: member access with . operator.
#define D_FOR_EACH_MEMBER_DOT(_obj, _member, ...) \
    D_FOR_EACH_DATA(D_INTERNAL_MEMBER_DOT_OP_EXPAND,                         \
                    (_obj, _member),                                         \
                    __VA_ARGS__)


//==============================================================================
// 10.  ADVANCED ITERATION PATTERNS
//==============================================================================


// 10.1    Adjacent pair iteration
//------------------------------------------------------------------------------

// 10.1.1
// D_INTERNAL_MAP_ADJ0 / D_INTERNAL_MAP_ADJ1
//   macro family: recursive workers for overlapping adjacent pairs.
#define D_INTERNAL_MAP_ADJ0(f, x, y, peek, ...) f(x, y)                       \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_ADJ1)(f, y, peek, __VA_ARGS__)
#define D_INTERNAL_MAP_ADJ1(f, x, y, peek, ...) f(x, y)                       \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_ADJ0)(f, y, peek, __VA_ARGS__)

// 10.1.2
// D_FOR_EACH_ADJACENT_PAIR
//   macro: overlapping pair iteration.
// Usage: D_FOR_EACH_ADJACENT_PAIR(fn, a, b, c) -> fn(a,b) fn(b,c)
#define D_FOR_EACH_ADJACENT_PAIR(fn, first, second, ...)                      \
    D_EVAL(D_INTERNAL_MAP_ADJ0(fn, first, second, __VA_ARGS__, (), 0))


//==============================================================================
// 11.  POINTER ARRAY INITIALIZATION
//==============================================================================


// 11.1    Data-comma iteration
//------------------------------------------------------------------------------

// 11.1.1
// D_INTERNAL_MAP_DATA_COMMA0 / D_INTERNAL_MAP_DATA_COMMA1
//   macro family: recursive data workers that emit comma separators.
#define D_INTERNAL_MAP_DATA_COMMA0(f, data, x, peek, ...) , f(data, x)        \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_DATA_COMMA1)(f, data, peek, __VA_ARGS__)
#define D_INTERNAL_MAP_DATA_COMMA1(f, data, x, peek, ...) , f(data, x)        \
    D_INTERNAL_MAP_NEXT(peek, D_INTERNAL_MAP_DATA_COMMA0)(f, data, peek, __VA_ARGS__)

// 11.1.2
// D_FOR_EACH_DATA_COMMA
//   macro: iteration with data, comma-separated.
#define D_FOR_EACH_DATA_COMMA(fn, data, first, ...)                           \
    fn(data, first)                                                           \
    D_EVAL(D_INTERNAL_MAP_DATA_COMMA0(fn, data, __VA_ARGS__, (), 0))

// 11.2    Struct array helpers
//------------------------------------------------------------------------------

// 11.2.1
// D_INTERNAL_PTR_ELEM
//   macro (internal): creates a void pointer to a compound literal.
#define D_INTERNAL_PTR_ELEM(element_type, tuple)                              \
    (void*)&(element_type){ D_UNPACK tuple }

// 11.2.2
// D_INTERNAL_TUPLE_TO_BRACES
//   macro (internal): converts a tuple to a brace-enclosed initializer.
#define D_INTERNAL_TUPLE_TO_BRACES(_unused, tuple)                            \
    { D_UNPACK tuple }

// 11.2.3
// D_STRUCT_ARRAY_INIT
//   macro: initializes an array of structs from tuples.
#define D_STRUCT_ARRAY_INIT(...)                                              \
    { D_FOR_EACH_DATA_COMMA(D_INTERNAL_TUPLE_TO_BRACES, _, __VA_ARGS__) }


//==============================================================================
// 12.  UTILITY OPERATORS
//==============================================================================


// 12.1    Debug/test operators
//------------------------------------------------------------------------------

// 12.1.1
// D_PRINT_OP / D_PRINT_VAL_OP
//   macro family: generated debugging print statements.
#define D_PRINT_OP(x)           printf("%s\n", D_STRINGIFY(x));
#define D_PRINT_VAL_OP(x)       printf("%s = %d\n", D_STRINGIFY(x), (int)(x));

// 12.1.2
// D_DECLARE_VAR_OP / D_DECLARE_TYPED_OP
//   macro family: generated variable declarations.
#define D_DECLARE_VAR_OP(x)     int x;
#define D_DECLARE_TYPED_OP(t,n) t n;

// 12.1.3
// D_ASSIGN_OP / D_INIT_ZERO_OP
//   macro family: generated assignment statements.
#define D_ASSIGN_OP(var, value) var = value;
#define D_INIT_ZERO_OP(x)       x = 0;


//==============================================================================
// 13.  COMPILE-TIME ASSERTIONS
//==============================================================================


// 13.1    Size/type checks
//------------------------------------------------------------------------------

// 13.1.1
// D_ASSERT_SAME_SIZE
//   macro: assert two types have the same size at compile time.
#define D_ASSERT_SAME_SIZE(type1, type2)                                      \
    D_STATIC_ASSERT(sizeof(type1) == sizeof(type2),                           \
                    "Size mismatch: " #type1 " vs " #type2)


#endif  // DJINTERP_С_MACRO_