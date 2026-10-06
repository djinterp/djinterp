/*******************************************************************************
* djinterp [c]                                                     math_common.h
*
* The math module's C kit: what every math kernel is spelled with.
*   The math module is a C core with C++ faces, as memory, color and
* functional are. Every algorithm is written once, here in the C layer, and
* compiled by both languages: a C program calls it, and the C++ faces under
* inc/djinterp/math/ forward to it rather than restating it.
*
*   A kernel is a header-defined function. Two qualifiers say what kind:
*
*     D_MATH_FN     pure arithmetic: constexpr from C++14 (the bodies are
*                   statements, which C++11's constexpr cannot hold), a
*                   header inline in C and below C++14. The same text is a C
*                   function and a C++ constant-expression function.
*     D_MATH_FN_RT  never constexpr: it calls the C library or writes through
*                   a pointer it is given. A header inline in both languages.
*
*   In C a header inline is the root's D_INLINE, which is a `static inline`:
* every translation unit that uses a kernel gets its own copy, and no .c file
* is needed for one. In C++ it is an `inline` function with C++ linkage, so
* the two languages never share a symbol and can be linked together freely.
*
*   C has no templates, so a kernel that serves every integer type is written
* once over the widest integer the build can spell -- d_math_imax and
* d_math_umax, below -- and every narrower type reaches it by a conversion
* that loses nothing. That is intmax_t and uintmax_t wherever dstdint.h
* declares them, and long and unsigned long where it cannot (ISO strict C++98
* on a 32-bit target, which has no 64-bit type at all, and so no type wider
* than long for a kernel to miss).
*
*
* path:      /inc/djinterp/c/math/math_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KERNEL QUALIFIERS
    -----------------
    1.  Kernel qualifiers
         1.  D_MATH_FN
         2.  D_MATH_FN_RT
2.  WIDEST INTEGERS
    ---------------
    1.  Types
         1.  d_math_imax
         2.  d_math_umax
    2.  Limits
         1.  D_MATH_UMAX_MAX
         2.  D_MATH_IMAX_MAX
         3.  D_MATH_IMAX_MIN
3.  SHARED VALUE TYPES
    ------------------
    1.  Ratio
         1.  d_math_ratio
*/

#ifndef DJINTERP_C_MATH_MATH_COMMON_H
#define DJINTERP_C_MATH_MATH_COMMON_H 1

// djinterp
#ifdef __cplusplus
    #include "../../djinterp.hpp"  // framework root
#else
    #include "../djinterp.h"       // framework root
#endif  // __cplusplus
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // intmax_t, uintmax_t, INTMAX_MAX


//==============================================================================
// 1.  KERNEL QUALIFIERS
//==============================================================================
// The root's own qualifiers, composed once for the math module. The C++
// spellings come from djinterp.hpp, which the C++ branch above includes; the C
// ones from djinterp.h.


// 1.1    Kernel qualifiers
//------------------------------------------------------------------------------
// 1.1.1
// D_MATH_FN
//   qualifier: a math kernel of pure arithmetic. constexpr from C++14 when the
// header is compiled as C++; a header inline in C and below C++14.
#ifdef __cplusplus
    #define D_MATH_FN               D_CONSTEXPR_CPP14 D_INLINE
#else
    #define D_MATH_FN               D_INLINE
#endif  // __cplusplus

// 1.1.2
// D_MATH_FN_RT
//   qualifier: a math kernel that is never constexpr, because it calls the C
// library or writes through a pointer; a header inline in both languages.
#define D_MATH_FN_RT                D_INLINE


//==============================================================================
// 2.  WIDEST INTEGERS
//==============================================================================
// The widest signed and unsigned integers the build can spell: the types a
// kernel serving every integer type is written over. dstdint.h declares
// intmax_t and its limits only where a 64-bit type exists, so their presence
// is tested by the limit macro.


// 2.1    Types
//------------------------------------------------------------------------------
// 2.1.1
// d_math_imax
//   type: the widest signed integer the build can spell.
// 2.1.2
// d_math_umax
//   type: the widest unsigned integer the build can spell.
#if defined(INTMAX_MAX)
    typedef intmax_t                d_math_imax;
    typedef uintmax_t               d_math_umax;
#else
    typedef long                    d_math_imax;
    typedef unsigned long           d_math_umax;
#endif

// 2.2    Limits
//------------------------------------------------------------------------------
// Spelled as casts rather than as INTMAX_MAX and its kin, which on a 32-bit
// target are `long long` literals that C++ before C++11 rejects; being casts,
// they are not for #if. The minimum assumes two's complement, which C23 and
// C++20 require and every supported target has.
// 2.2.1
// D_MATH_UMAX_MAX
//   constant: the greatest value of d_math_umax.
#define D_MATH_UMAX_MAX             ((d_math_umax)-1)

// 2.2.2
// D_MATH_IMAX_MAX
//   constant: the greatest value of d_math_imax.
#define D_MATH_IMAX_MAX             ((d_math_imax)(D_MATH_UMAX_MAX >> 1))

// 2.2.3
// D_MATH_IMAX_MIN
//   constant: the least value of d_math_imax.
#define D_MATH_IMAX_MIN             (-D_MATH_IMAX_MAX - 1)


//==============================================================================
// 3.  SHARED VALUE TYPES
//==============================================================================


// 3.1    Ratio
//------------------------------------------------------------------------------
// 3.1.1
// d_math_ratio
//   struct: a quotient kept as its two terms, each exact or rounded once, so a
// caller divides in the precision it wants and the result is rounded only once
// more: a C++ face asked for a float divides two floats, not a long double.
struct d_math_ratio
{
    long double numerator;    // the dividend
    long double denominator;  // the divisor; never 0
};


#endif  // DJINTERP_C_MATH_MATH_COMMON_H
