/*******************************************************************************
* djinterp [c/math]                                                math_common.h
*
* The math module's C kit: what every math kernel is spelled with.
*   The math module is a C core with C++ faces, as memory, color and
* functional are. Every algorithm is written once, here in the C layer, and
* compiled by both languages: a C program calls it, and the C++ faces under
* inc/djinterp/math/ forward to it rather than restating it.
*
*   A kernel is a header-defined function. Five qualifiers say what kind:
*
*     D_MATH_FN     pure arithmetic: constexpr from C++14 (the bodies are
*                   statements, which C++11's constexpr cannot hold), a
*                   header inline in C and below C++14. The same text is a C
*                   function and a C++ constant-expression function. One
*                   that answers twice, as C's remquo does, stores its
*                   second answer through a pointer it is given: a constant
*                   expression where the object pointed to is the
*                   evaluation's own, as with C++23's std::remquo.
*     D_MATH_FN_RT  never constexpr: it calls the C library or writes into a
*                   buffer it is given. A header inline in both languages.
*     D_MATH_FN_ENTRY  a function a table points to, as a boxed kind's
*                   entries are, which a call through the table does not
*                   inline: a header inline the compiler is never told it
*                   must inline, as it is told the other two's outside
*                   testing builds. Told so, gcc stops at -Og at a call
*                   through a constant table that it finds the function of.
*     D_MATH_FN_KERNEL  a large kernel of pure arithmetic that many
*                   functions share, as cplx.h's long double kernels serve
*                   its three families: constexpr from C++14, as D_MATH_FN,
*                   but never forced inline, so that a translation unit
*                   compiles it once however many callers it has, where
*                   forced it would be compiled into each.
*     D_MATH_FN_KERNEL_RT  such a large kernel that calls the C library, as
*                   the plain forms of decomp.h's and cmat.h's eigensolvers
*                   do: never constexpr, and never forced inline.
*
*   In C a header inline is the root's D_INLINE, which is a `static inline`:
* every translation unit that uses a kernel gets its own copy, and no .c file
* is needed for one. In C++ it is an `inline` function with C++ linkage, so
* the two languages never share a symbol and can be linked together freely.
*
*   C has no templates, so a kernel that serves every integer type is written
* once over the widest integer the build can spell -- dj_math_imax and
* dj_math_umax, below -- and every narrower type reaches it by a conversion
* that loses nothing. That is intmax_t and uintmax_t wherever dstdint.h
* declares them, and long and unsigned long where it cannot (ISO strict C++98
* wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS,
* none of which has a type wider than long for a kernel to miss).
*
*   A target may evaluate float and double wider than their types -- a
* 32-bit x87 holds both in its 64-bit registers -- and FLT_EVAL_METHOD says
* so. There a value the C library returns can be wider than its type too,
* and a compiler takes a call's result to be of its type; a kernel that
* hands such a result on stores it first, so that it is (section 4).
*
*
* path:      /inc/djinterp/c/modules/math/math_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KERNEL QUALIFIERS
    -----------------
    1.  Kernel qualifiers
         1.  D_MATH_FN
         2.  D_MATH_FN_RT
         3.  D_MATH_FN_ENTRY
         4.  D_MATH_FN_KERNEL
         5.  D_MATH_FN_KERNEL_RT
2.  WIDEST INTEGERS
    ---------------
    1.  Types
         1.  dj_math_imax
         2.  dj_math_umax
    2.  Limits
         1.  D_MATH_UMAX_MAX
         2.  D_MATH_IMAX_MAX
         3.  D_MATH_IMAX_MIN
3.  SHARED VALUE TYPES
    ------------------
    1.  Ratio
         1.  dj_math_ratio
4.  EVALUATION
    ----------
    1.  Wider than the type
         1.  D_MATH_EVAL_METHOD
         2.  D_MATH_FLT_EVAL_WIDE
         3.  D_MATH_DBL_EVAL_WIDE
         4.  D_MATH_LDBL_EVAL_WIDE
*/

#ifndef DJINTERP_C_MATH_MATH_COMMON_H
#define DJINTERP_C_MATH_MATH_COMMON_H 1

// djinterp
#ifdef __cplusplus
    #include "../../djinterp_cpp.h"  // framework root
#else
    #include "../../djinterp.h"       // framework root
#endif  // __cplusplus
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // intmax_t, uintmax_t, INTMAX_MAX
// FLT_EVAL_METHOD: C99's <float.h>; for C++, re_std's cfloat.hpp, which
// recovers it before C++11 from the compiler's own __FLT_EVAL_METHOD__
#ifdef __cplusplus
    // C++ callers include cfloat.hpp themselves (no C header includes C++)
#else
    #include <float.h>                            // FLT_EVAL_METHOD
#endif  // __cplusplus


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
//   qualifier: a math kernel of pure arithmetic, which may store a second
// answer through a pointer, as remquo does. constexpr from C++14 when the
// header is compiled as C++; a header inline in C and below C++14.
#ifdef __cplusplus
    #define D_MATH_FN               D_CONSTEXPR_CPP14 D_INLINE
#else
    #define D_MATH_FN               D_INLINE
#endif  // __cplusplus

// 1.1.2
// D_MATH_FN_RT
//   qualifier: a math kernel that is never constexpr, because it calls the C
// library or writes into a buffer; a header inline in both languages.
#define D_MATH_FN_RT                D_INLINE

// 1.1.3
// D_MATH_FN_ENTRY
//   qualifier: a function a table points to, called through it, and never
// forced inline: in C a static inline, so each translation unit has its own
// copy, and in C++ an inline function, so the copies are one.
#ifdef __cplusplus
    #define D_MATH_FN_ENTRY         inline
#else
    #define D_MATH_FN_ENTRY         static D_ENV_COMPILER_INLINE
#endif  // __cplusplus

// 1.1.4
// D_MATH_FN_KERNEL
//   qualifier: a large kernel of pure arithmetic that many functions share,
// never forced inline, so that a translation unit compiles it once: constexpr
// from C++14 when the header is compiled as C++, as D_MATH_FN is; in C a
// static inline, and below C++14 an inline function.
#ifdef __cplusplus
    #define D_MATH_FN_KERNEL        D_CONSTEXPR_CPP14 inline
#else
    #define D_MATH_FN_KERNEL        static D_ENV_COMPILER_INLINE
#endif  // __cplusplus

// 1.1.5
// D_MATH_FN_KERNEL_RT
//   qualifier: a large kernel that calls the C library, so is never
// constexpr, and is never forced inline, so that a translation unit compiles
// it once: in C a static inline, in C++ an inline function.
#ifdef __cplusplus
    #define D_MATH_FN_KERNEL_RT     inline
#else
    #define D_MATH_FN_KERNEL_RT     static D_ENV_COMPILER_INLINE
#endif  // __cplusplus


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
// dj_math_imax
//   type: the widest signed integer the build can spell.
// 2.1.2
// dj_math_umax
//   type: the widest unsigned integer the build can spell.
#if defined(INTMAX_MAX)
    typedef intmax_t                dj_math_imax;
    typedef uintmax_t               dj_math_umax;
#else
    typedef long                    dj_math_imax;
    typedef unsigned long           dj_math_umax;
#endif

// 2.2    Limits
//------------------------------------------------------------------------------
// Spelled as casts rather than as INTMAX_MAX and its kin, which on a 32-bit
// target are `long long` literals that C++ before C++11 rejects; being casts,
// they are not for #if. The minimum assumes two's complement, which C23 and
// C++20 require and every supported target has.
// 2.2.1
// D_MATH_UMAX_MAX
//   constant: the greatest value of dj_math_umax.
#define D_MATH_UMAX_MAX             ((dj_math_umax)-1)

// 2.2.2
// D_MATH_IMAX_MAX
//   constant: the greatest value of dj_math_imax.
#define D_MATH_IMAX_MAX             ((dj_math_imax)(D_MATH_UMAX_MAX >> 1))

// 2.2.3
// D_MATH_IMAX_MIN
//   constant: the least value of dj_math_imax.
#define D_MATH_IMAX_MIN             (-D_MATH_IMAX_MAX - 1)


//==============================================================================
// 3.  SHARED VALUE TYPES
//==============================================================================


// 3.1    Ratio
//------------------------------------------------------------------------------
// 3.1.1
// dj_math_ratio
//   struct: a quotient kept as its two terms, each exact or rounded once, so a
// caller divides in the precision it wants and the result is rounded only once
// more: a C++ face asked for a float divides two floats, not a long double.
struct dj_math_ratio
{
    long double numerator;    // the dividend
    long double denominator;  // the divisor; never 0
};


//==============================================================================
// 4.  EVALUATION
//==============================================================================
// How wide the target evaluates each floating type, from FLT_EVAL_METHOD. A
// 32-bit x87 evaluates float and double in long double (2): there a value held
// in a register carries more than its type, and so can one the C library
// returns -- glibc's i386 sqrtf leaves the x87's 64 bits -- which a compiler
// takes to be of its type. A kernel that hands on the C library's result
// stores it through a volatile where its type is evaluated wider, so that the
// value is of its type; elsewhere it returns it as it comes.


// 4.1    Wider than the type
//------------------------------------------------------------------------------
// 4.1.1
// D_MATH_EVAL_METHOD
//   constant: FLT_EVAL_METHOD where the build states it, and 0 where it does
// not. -1 is indeterminable; 0 evaluates each type in itself, 1 float and
// double in double, 2 both in long double; C23's 16, 32, 64 and 128 evaluate
// whatever is narrower than _Float16 to _Float128 in that type.
#if defined(FLT_EVAL_METHOD)
    #define D_MATH_EVAL_METHOD      FLT_EVAL_METHOD
#else
    #define D_MATH_EVAL_METHOD      0
#endif

// 4.1.2
// D_MATH_FLT_EVAL_WIDE
//   constant: 1 where the target evaluates float wider than float, or cannot
// say; 0 where it evaluates float in float.
#if ( (D_MATH_EVAL_METHOD == 0)  ||                                            \
      (D_MATH_EVAL_METHOD == 16) ||                                            \
      (D_MATH_EVAL_METHOD == 32) )
    #define D_MATH_FLT_EVAL_WIDE    0
#else
    #define D_MATH_FLT_EVAL_WIDE    1
#endif

// 4.1.3
// D_MATH_DBL_EVAL_WIDE
//   constant: 1 where the target evaluates double wider than double, or
// cannot say; 0 where it evaluates double in double.
#if ( (D_MATH_EVAL_METHOD == 0)  ||                                            \
      (D_MATH_EVAL_METHOD == 1)  ||                                            \
      (D_MATH_EVAL_METHOD == 16) ||                                            \
      (D_MATH_EVAL_METHOD == 32) ||                                            \
      (D_MATH_EVAL_METHOD == 64) )
    #define D_MATH_DBL_EVAL_WIDE    0
#else
    #define D_MATH_DBL_EVAL_WIDE    1
#endif

// 4.1.4
// D_MATH_LDBL_EVAL_WIDE
//   constant: 1 where the target evaluates long double wider than long
// double -- C23's 128 with a narrower long double -- or cannot say; 0 where
// it evaluates long double in long double, as every method before C23's
// does.
#if ( (D_MATH_EVAL_METHOD < 0) ||                                              \
      ( (D_MATH_EVAL_METHOD == 128) &&                                         \
        (LDBL_MANT_DIG < 113) ) )
    #define D_MATH_LDBL_EVAL_WIDE   1
#else
    #define D_MATH_LDBL_EVAL_WIDE   0
#endif


#endif  // DJINTERP_C_MATH_MATH_COMMON_H
