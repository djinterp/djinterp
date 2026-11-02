/*******************************************************************************
* djinterp [config]                                                 cfg_dmacro.h
*
*   Build-time configuration for dmacro.h: the variadic limits, the generated
* macro-family variant, and which families are included.
*
*   targets:  c/dmacro.h -> D_CFG_DMACRO_*, D_DMACRO_VARG_MAX, D_DMACRO_VARIANT,
*                           D_DMACRO_PAIR_MAX, D_DMACRO_TRIPLE_MAX,
*                           D_DMACRO_4TUPLE_MAX, D_INTERNAL_DMACRO_*
*   requires: cfg_common.h (helpers, user overrides); env/env.h
*             (D_ENV_PP_MAX_MACRO_ARGS)
*
*   THE ONE HOME.  Until 2026.10.31 this file and dmacro.h each held the same
* sixty-two definitions, and dmacro.h never included this one, so this copy
* resolved nothing.  dmacro.h now includes it in place of its own section 0,
* and keeps section 0.7, which includes the generated variant files: choosing
* which file to include is the module's business, deciding it is this file's.
*
*
* path:      /inc/djinterp/config/c/util/cfg_dmacro.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.09
*                                                            revised: 2026.11.01
*******************************************************************************/

#ifndef DJINTERP_CONFIG_C_UTIL_CFG_DMACRO_H
#define DJINTERP_CONFIG_C_UTIL_CFG_DMACRO_H 1

// djinterp
#include "../../cfg_common.h"         // D_CFG_* helpers, user overrides
#include "../../../env/env.h"         // D_ENV_PP_MAX_MACRO_ARGS


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
//   4. D_INTERNAL_DMACRO_VARG_DEFAULT (128) - fallback default
//
// -----------------------------------------------------------------------------

// --- 0.1  Configuration Constants ---
//   Constants, not knobs: nothing overrides them, so they are D_INTERNAL_*
// (until 2026.11.01 they were D_CFG_DMACRO_VARG_DEFAULT, _MIN and _LIMIT).

// D_INTERNAL_DMACRO_VARG_DEFAULT
//   internal: default maximum variadic argument count (128)
#define D_INTERNAL_DMACRO_VARG_DEFAULT 128

// D_INTERNAL_DMACRO_VARG_MIN
//   internal: minimum supported variadic argument count (64)
#define D_INTERNAL_DMACRO_VARG_MIN     64

// D_INTERNAL_DMACRO_VARG_LIMIT
//   internal: absolute maximum supported by the framework (1024)
#define D_INTERNAL_DMACRO_VARG_LIMIT   1024

// D_CFG_DMACRO_VARIANT_*
//   brief: the variant levels there are generated macro files for (the
// for_each*N.h set under c/util/macro/); 0.3 rounds the effective maximum up
// to one of them, and D_DMACRO_VARIANT names it.  Values, not knobs: a level
// added here needs its generated files.
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
    #define D_CFG_DMACRO_VARG_MAX D_INTERNAL_DMACRO_VARG_DEFAULT
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

// validation: #if reads these switches as numbers, so each must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_OVERRIDE)
    #error "D_CFG_DMACRO_OVERRIDE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_USE_MSVC_COMPAT)
    #error "D_CFG_DMACRO_USE_MSVC_COMPAT must be 0 or 1"
#endif


// --- 0.3  Effective Value Calculation ---

// D_INTERNAL_DMACRO_RAW_MAX
//   internal: the maximum before clamping: D_CFG_DMACRO_VARG_MAX under
// D_CFG_DMACRO_OVERRIDE, else the smaller of it and D_ENV_PP_MAX_MACRO_ARGS
// where env.h detected one, else D_INTERNAL_DMACRO_VARG_DEFAULT.
// step 1: determine raw max value
#if D_CFG_IS_ON(D_CFG_DMACRO_OVERRIDE)
    #define D_INTERNAL_DMACRO_RAW_MAX D_CFG_DMACRO_VARG_MAX

#elif defined(D_ENV_PP_MAX_MACRO_ARGS)
    #if (D_ENV_PP_MAX_MACRO_ARGS > D_CFG_DMACRO_VARG_MAX)
        #define D_INTERNAL_DMACRO_RAW_MAX D_CFG_DMACRO_VARG_MAX
    #else
        #define D_INTERNAL_DMACRO_RAW_MAX D_ENV_PP_MAX_MACRO_ARGS
    #endif

#else
    #define D_INTERNAL_DMACRO_RAW_MAX D_INTERNAL_DMACRO_VARG_DEFAULT
#endif

// D_INTERNAL_DMACRO_CLAMPED_MAX
//   internal: D_INTERNAL_DMACRO_RAW_MAX clamped to D_INTERNAL_DMACRO_VARG_MIN
// .. D_INTERNAL_DMACRO_VARG_LIMIT; step 4 rounds it up to a variant.
// step 2: clamp to supported range
#if (D_INTERNAL_DMACRO_RAW_MAX < D_INTERNAL_DMACRO_VARG_MIN)
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_INTERNAL_DMACRO_VARG_MIN
#elif (D_INTERNAL_DMACRO_RAW_MAX > D_INTERNAL_DMACRO_VARG_LIMIT)
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_INTERNAL_DMACRO_VARG_LIMIT
#else
    #define D_INTERNAL_DMACRO_CLAMPED_MAX D_INTERNAL_DMACRO_RAW_MAX
#endif

// step 3: MSVC traditional preprocessor compatibility
#if D_CFG_IS_ON(D_CFG_DMACRO_USE_MSVC_COMPAT)
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
//   The names a caller reads. Until 2026.11.01 each also had a D_CFG_* twin
// (D_CFG_VARG_COUNT_MAX, D_CFG_FOR_EACH_MAX and the rest), which no knob set
// and nothing read; they are gone.

// D_VARG_COUNT_MAX
//   brief: maximum number of variadic arguments supported
#define D_VARG_COUNT_MAX     D_DMACRO_VARG_MAX

// D_FOR_EACH_MAX
//   brief: maximum elements D_FOR_EACH can iterate over
#define D_FOR_EACH_MAX       D_DMACRO_VARG_MAX

// D_FOR_EACH_PAIR_MAX
//   brief: maximum pairs D_FOR_EACH_PAIR can iterate over
#define D_FOR_EACH_PAIR_MAX  D_DMACRO_PAIR_MAX

// D_FOR_EACH_TRIPLE_MAX
//   brief: maximum triples D_FOR_EACH_TRIPLE can iterate over
#define D_FOR_EACH_TRIPLE_MAX D_DMACRO_TRIPLE_MAX

// D_FOR_EACH_4TUPLE_MAX
//   brief: maximum 4-tuples D_FOR_EACH_4TUPLE can iterate over
#define D_FOR_EACH_4TUPLE_MAX D_DMACRO_4TUPLE_MAX

// D_MACRO_VARIANT
//   brief: identifies which variant of auto-generated headers is in use
#define D_MACRO_VARIANT      D_DMACRO_VARIANT

// D_VARG_PAIR_MAX
//   brief: alias for D_DMACRO_PAIR_MAX (legacy compatibility)
#define D_VARG_PAIR_MAX D_DMACRO_PAIR_MAX


// --- 0.5  Configuration Query Macros ---

// D_DMACRO_IS_OVERRIDE_ENABLED
//   brief: returns 1 if override mode is active, 0 otherwise
#define D_DMACRO_IS_OVERRIDE_ENABLED() D_CFG_IS_ON(D_CFG_DMACRO_OVERRIDE)

// D_DMACRO_IS_MSVC_COMPAT
//   brief: returns 1 if MSVC compatibility mode is active, 0 otherwise
#define D_DMACRO_IS_MSVC_COMPAT() D_CFG_IS_ON(D_CFG_DMACRO_USE_MSVC_COMPAT)

// D_DMACRO_USES_ENV_DETECTION
//   brief: returns 1 if environment detection is being used, 0 otherwise
#if D_CFG_IS_ON(D_CFG_DMACRO_OVERRIDE)
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
// EXAMPLE - keep only what you need:
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

// validation: #if reads these switches as numbers, so each must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_SEPARATOR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_PAIR_SEPARATOR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_TRIPLE_SEPARATOR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_DATA_SEPARATOR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA)
    #error "D_CFG_DMACRO_INCLUDE_FOR_EACH_COMMA must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_1)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_1 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_2)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_2 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_3)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_3 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_4)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_4 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_5)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_5 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_6)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_6 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_7)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_7 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_8)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_8 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_9)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_9 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_10)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_10 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_11)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_11 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_12)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_12 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_13)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_13 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_14)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_14 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_15)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_15 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DMACRO_INCLUDE_TUPLE_16)
    #error "D_CFG_DMACRO_INCLUDE_TUPLE_16 must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_C_UTIL_CFG_DMACRO_H
