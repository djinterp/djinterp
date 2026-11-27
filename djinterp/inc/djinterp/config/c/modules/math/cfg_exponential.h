/*******************************************************************************
* djinterp [c][math]                                           cfg_exponential.h
*
*   Build-time configuration of the exponential's C core
* (c/math/exponential.h): which form the double family's fast functions,
* d_math_d_exp_fast and d_math_d_log_fast, take. Each is by default
* djinterp's own formula, e^x within 0.511 of an ulp and log x within 0.53
* of one, proven, the same bits on every platform where double is evaluated
* in double (S490, S500); either can be the C library's exp or log instead,
* the platform's bits at the platform's time (S505). These knobs choose for
* the plain fast names, once for the build; d_math_d_exp_fast_with and
* d_math_d_log_fast_with choose call by call at run time, by an argument
* that takes the same two values (S506).
*   The knobs are enumerated: each names one of the values below, and is
* refused otherwise. Each is #ifndef-guarded, so a definition made before
* exponential.h is first included - with -D, in cfg_custom.h, or in the
* including file - wins over the default here. D_CFG_MATH_FAST sets both
* fast functions; D_CFG_MATH_EXP_FAST and D_CFG_MATH_LOG_FAST, each
* D_CFG_MATH_FAST by default, set one.
*
*   targets:  c/math/exponential.h -> D_CFG_MATH_EXP_FAST, D_CFG_MATH_LOG_FAST
*   requires: cfg_common.h (user overrides, the testing preset,
*             D_CFG_IS_INT_LITERAL)
*
* HOW TO USE
* ----------
*   Not included by a caller: exponential.h's parts include it. Set a knob
* before the first of them, with -D, in cfg_custom.h or in the including
* file: -DD_CFG_MATH_LOG_FAST=D_CFG_MATH_FAST_LIBRARY makes d_math_d_log_fast
* the C library's log, d_math_d_exp_fast staying djinterp's own.
*
* OPTIONS
* -------
*   The knobs' values, D_CFG_MATH_FAST_OWN and D_CFG_MATH_FAST_LIBRARY, are
* also what d_math_d_exp_fast_with and d_math_d_log_fast_with take at run
* time, whatever the knobs say.
*
* CONFIGURATION
* -------------
*   D_CFG_MATH_FAST: D_CFG_MATH_FAST_OWN, the default, or
* D_CFG_MATH_FAST_LIBRARY, for both fast functions. D_CFG_MATH_EXP_FAST and
* D_CFG_MATH_LOG_FAST: the same for d_math_d_exp_fast or d_math_d_log_fast
* alone, D_CFG_MATH_FAST by default. Where double is evaluated wider than
* double, a 32-bit x87, both are the C library's whatever the knobs say
* (S491, S501).
*
*
* path:      /inc/djinterp/config/c/modules/math/cfg_exponential.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.25
*                                                            revised: 2026.11.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  D_CFG_MATH_FAST_OWN
    2.  D_CFG_MATH_FAST_LIBRARY
2.  KNOBS
    -----
    1.  D_CFG_MATH_FAST
    2.  D_CFG_MATH_EXP_FAST
    3.  D_CFG_MATH_LOG_FAST
*/

#ifndef DJINTERP_CONFIG_C_MATH_CFG_EXPONENTIAL_H
#define DJINTERP_CONFIG_C_MATH_CFG_EXPONENTIAL_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides


//==============================================================================
// 1.  VALUES
//==============================================================================

// Neither is 0, which a misspelled or empty value reads as.

// 1.1
// D_CFG_MATH_FAST_OWN
//   constant: djinterp's own formula, with its proven bound and the same
// bits on every platform where double is evaluated in double.
#define D_CFG_MATH_FAST_OWN     1

// 1.2
// D_CFG_MATH_FAST_LIBRARY
//   constant: the C library's function (std:: in C++), the platform's bits.
#define D_CFG_MATH_FAST_LIBRARY 2


//==============================================================================
// 2.  KNOBS
//==============================================================================

// 2.1
// D_CFG_MATH_FAST
//   knob: the form of both fast functions, d_math_d_exp_fast and
// d_math_d_log_fast; D_CFG_MATH_FAST_OWN by default.
#ifndef D_CFG_MATH_FAST
    #define D_CFG_MATH_FAST D_CFG_MATH_FAST_OWN
#endif  // D_CFG_MATH_FAST

#if ( (!D_CFG_IS_INT_LITERAL(D_CFG_MATH_FAST)) ||                             \
      ( (D_CFG_MATH_FAST != D_CFG_MATH_FAST_OWN) &&                           \
        (D_CFG_MATH_FAST != D_CFG_MATH_FAST_LIBRARY) ) )
    #error "cfg_exponential.h: D_CFG_MATH_FAST must name one of its values"
#endif

// 2.2
// D_CFG_MATH_EXP_FAST
//   knob: the form of d_math_d_exp_fast alone; D_CFG_MATH_FAST by default.
#ifndef D_CFG_MATH_EXP_FAST
    #define D_CFG_MATH_EXP_FAST D_CFG_MATH_FAST
#endif  // D_CFG_MATH_EXP_FAST

#if ( (!D_CFG_IS_INT_LITERAL(D_CFG_MATH_EXP_FAST)) ||                         \
      ( (D_CFG_MATH_EXP_FAST != D_CFG_MATH_FAST_OWN) &&                       \
        (D_CFG_MATH_EXP_FAST != D_CFG_MATH_FAST_LIBRARY) ) )
    #error "cfg_exponential.h: D_CFG_MATH_EXP_FAST must name one of its values"
#endif

// 2.3
// D_CFG_MATH_LOG_FAST
//   knob: the form of d_math_d_log_fast alone; D_CFG_MATH_FAST by default.
#ifndef D_CFG_MATH_LOG_FAST
    #define D_CFG_MATH_LOG_FAST D_CFG_MATH_FAST
#endif  // D_CFG_MATH_LOG_FAST

#if ( (!D_CFG_IS_INT_LITERAL(D_CFG_MATH_LOG_FAST)) ||                         \
      ( (D_CFG_MATH_LOG_FAST != D_CFG_MATH_FAST_OWN) &&                       \
        (D_CFG_MATH_LOG_FAST != D_CFG_MATH_FAST_LIBRARY) ) )
    #error "cfg_exponential.h: D_CFG_MATH_LOG_FAST must name one of its values"
#endif


#endif  // DJINTERP_CONFIG_C_MATH_CFG_EXPONENTIAL_H
