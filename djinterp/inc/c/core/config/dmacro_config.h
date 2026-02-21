/******************************************************************************
* djinterp [core]                                                 config_env.h
*
* 
*
*
* path:      \inc\core\config\dmacro_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.09
******************************************************************************/

#ifndef DJINTERP_CONFIG_DMACRO_
#define DJINTERP_CONFIG_DMACRO_ 1

#include "..\..\dconfig.h"


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


#endif  // DJINTERP_CONFIG_DMACRO_