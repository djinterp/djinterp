/******************************************************************************
* djinterp [re_std]                                                   cfloat.hpp
*
* the floating-point limit macros:
*   FLT_ / DBL_ / LDBL_ radix, precision, range and rounding constants.
* All macros, so none of them can live in re_std:: -- macros have no
* namespace. As with <climits>, this header's contribution is to fill the
* gaps a platform's own <cfloat> may leave, never to override it.
*
*   THE GAP HERE IS THE C11 SET THAT C++17 ADOPTED:
*   C++17 pulled in C11's *_DECIMAL_DIG, *_HAS_SUBNORM and *_TRUE_MIN.
* Libraries older than that provide the C++98 set and stop, so code that
* uses DBL_TRUE_MIN fails to compile on them even though the compiler
* knows the value perfectly well. Each is recovered from the matching
* GCC/Clang __DBL_DENORM_MIN__-style builtin, which is the compiler
* stating a fact about its own float format -- a re-derivation, not an
* estimate.
*
*   WHERE NO BUILTIN EXISTS, NOTHING IS DEFINED:
*   MSVC exposes no such builtins. On that compiler a missing macro stays
* missing, which the caller can test with #ifdef, rather than being
* filled with a value invented for it. A wrong DBL_TRUE_MIN would be far
* worse than an absent one: it would silently misjudge every subnormal
* comparison written against it.
*
*   *_HAS_SUBNORM IS TRISTATE, NOT BOOLEAN:
*   The C11 spelling is -1 (indeterminable), 0 (absent) or 1 (present).
* GCC's __FLT_HAS_DENORM__ is 0/1 only, so the derived macro can never
* produce -1. That is a narrowing of the answer, not a wrong one: the
* compiler does know, so "indeterminable" was never the right value.
*
*   PREFER numeric_limits:
*   re_std::numeric_limits<double>::denorm_min() is DBL_TRUE_MIN with a
* type and a namespace, is constexpr from C++11, and exists uniformly for
* every floating-point type including ones that have no macro family
* here. These macros are for preprocessor conditionals.
*
*
* path:      /inc/djinterp/re_std/cfloat/cfloat.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CFLOAT_CFLOAT_
#define DJINTERP_RE_STD_CFLOAT_CFLOAT_ 1

// djinterp
#include "../../core/djinterp.hpp"

// std
//   the macros themselves -- the whole payload of the header.
#include <cfloat>


// ===========================================================================
// I.   DECIMAL_DIG FAMILY  (C11 / C++17)
// ===========================================================================
//
//   Digits needed to round-trip the type through decimal text. Distinct
// from the older *_DIG, which counts digits that survive the round trip
// in the other direction (decimal -> binary -> decimal).

#if !defined(DECIMAL_DIG) && defined(__DECIMAL_DIG__)
    // DECIMAL_DIG
    //   constant: round-trip digits for the widest supported type.
    #define DECIMAL_DIG         __DECIMAL_DIG__
#endif

#if !defined(FLT_DECIMAL_DIG) && defined(__FLT_DECIMAL_DIG__)
    // FLT_DECIMAL_DIG
    //   constant: round-trip decimal digits for float (typically 9).
    #define FLT_DECIMAL_DIG     __FLT_DECIMAL_DIG__
#endif

#if !defined(DBL_DECIMAL_DIG) && defined(__DBL_DECIMAL_DIG__)
    // DBL_DECIMAL_DIG
    //   constant: round-trip decimal digits for double (typically 17).
    #define DBL_DECIMAL_DIG     __DBL_DECIMAL_DIG__
#endif

#if !defined(LDBL_DECIMAL_DIG) && defined(__LDBL_DECIMAL_DIG__)
    // LDBL_DECIMAL_DIG
    //   constant: round-trip decimal digits for long double.
    #define LDBL_DECIMAL_DIG    __LDBL_DECIMAL_DIG__
#endif


// ===========================================================================
// II.  TRUE_MIN FAMILY  (C11 / C++17)
// ===========================================================================
//
//   Smallest positive value INCLUDING subnormals -- distinct from *_MIN,
// which is the smallest positive NORMAL value. The two differ by many
// orders of magnitude (for double, 4.9e-324 against 2.2e-308), so
// substituting one for the other is not a rounding difference.

#if !defined(FLT_TRUE_MIN) && defined(__FLT_DENORM_MIN__)
    // FLT_TRUE_MIN
    //   constant: smallest positive float, subnormals included.
    #define FLT_TRUE_MIN        __FLT_DENORM_MIN__
#endif

#if !defined(DBL_TRUE_MIN) && defined(__DBL_DENORM_MIN__)
    // DBL_TRUE_MIN
    //   constant: smallest positive double, subnormals included.
    #define DBL_TRUE_MIN        __DBL_DENORM_MIN__
#endif

#if !defined(LDBL_TRUE_MIN) && defined(__LDBL_DENORM_MIN__)
    // LDBL_TRUE_MIN
    //   constant: smallest positive long double, subnormals included.
    #define LDBL_TRUE_MIN       __LDBL_DENORM_MIN__
#endif


// ===========================================================================
// III. HAS_SUBNORM FAMILY  (C11 / C++17)
// ===========================================================================

#if !defined(FLT_HAS_SUBNORM) && defined(__FLT_HAS_DENORM__)
    // FLT_HAS_SUBNORM
    //   constant: 1 if float supports subnormals, 0 if not. Never -1 --
    // see the header comment.
    #define FLT_HAS_SUBNORM     __FLT_HAS_DENORM__
#endif

#if !defined(DBL_HAS_SUBNORM) && defined(__DBL_HAS_DENORM__)
    // DBL_HAS_SUBNORM
    //   constant: 1 if double supports subnormals, 0 if not.
    #define DBL_HAS_SUBNORM     __DBL_HAS_DENORM__
#endif

#if !defined(LDBL_HAS_SUBNORM) && defined(__LDBL_HAS_DENORM__)
    // LDBL_HAS_SUBNORM
    //   constant: 1 if long double supports subnormals, 0 if not.
    #define LDBL_HAS_SUBNORM    __LDBL_HAS_DENORM__
#endif


// ===========================================================================
// IV.  EVALUATION METHOD
// ===========================================================================

#if !defined(FLT_EVAL_METHOD) && defined(__FLT_EVAL_METHOD__)
    // FLT_EVAL_METHOD
    //   constant: the width intermediate results are computed at. -1
    // indeterminable, 0 evaluate at the operands' own width, 1 at double,
    // 2 at long double. Worth checking on x87 targets, where 2 is common
    // and float arithmetic quietly carries extra precision.
    #define FLT_EVAL_METHOD     __FLT_EVAL_METHOD__
#endif


#endif  // DJINTERP_RE_STD_CFLOAT_CFLOAT_
