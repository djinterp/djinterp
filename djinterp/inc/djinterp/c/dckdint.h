/*******************************************************************************
* djinterp [c]                                                         dckdint.h
*
* Checked arithmetic on sizes.
*   d_ckd_add, d_ckd_sub and d_ckd_mul compute the sum, difference or product
* of two size_t values and say, in the same call, whether it fits: each stores
* the result through its first argument, wrapped as unsigned arithmetic
* wraps, and returns true when the exact value does not fit, as C23's
* ckd_add, ckd_sub and ckd_mul do. A size is then computed and checked once,
* before the call that uses it:
*         if (d_ckd_mul(&bytes, count, element_size)) { return false; }
*   The tier is chosen once, here, from env: C23's <stdckdint.h> where env
* finds it (D_ENV_C_HAS_STDCKDINT_H); else the compiler's checked builtins
* (D_ENV_COMPILER_HAS_OVERFLOW_BUILTINS: GCC 5 and later, Clang 3.8 and
* later, at every level of C and C++); else the portable test, a comparison
* for the sum and the difference and a division for the product, as memory's
* d_mem_*_would_overflow predicates test. Every tier gives the same answer
* and stores the same value; D_CKD_TIER names the one a build uses.
* Pre-defining D_ENV_C_HAS_STDCKDINT_H or D_ENV_COMPILER_HAS_OVERFLOW_BUILTINS
* to 0 builds a lower tier, which is how the tests reach all three.
*   targets:  every module that computes a size from a count: an allocation,
*             a copy's length, a buffer's growth
*   requires: djinterp.h (env, bool, size_t); <stdckdint.h> at its tier
*
*
* path:      /inc/djinterp/c/dckdint.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TIERS
    -----
    1.  The tier a build uses
         1.  D_CKD_TIER_STDCKDINT, D_CKD_TIER_BUILTINS, D_CKD_TIER_PORTABLE
         2.  D_CKD_TIER
2.  CHECKED ARITHMETIC
    ------------------
    1.  The operations
         1.  d_ckd_add
         2.  d_ckd_sub
         3.  d_ckd_mul
*/

#ifndef DJINTERP_C_DCKDINT_H
#define DJINTERP_C_DCKDINT_H 1

// djinterp
#include "./djinterp.h"    // framework root: env, bool, size_t, D_INLINE


//==============================================================================
// 1.  TIERS
//==============================================================================


// 1.1    The tier a build uses
//------------------------------------------------------------------------------
// 1.1.1
// D_CKD_TIER_STDCKDINT, D_CKD_TIER_BUILTINS, D_CKD_TIER_PORTABLE
//   constant: the three tiers, best first: C23's <stdckdint.h>, the
// compiler's checked builtins, and the portable test.
#define D_CKD_TIER_STDCKDINT    3
#define D_CKD_TIER_BUILTINS     2
#define D_CKD_TIER_PORTABLE     1

// 1.1.2
// D_CKD_TIER
//   constant: the tier this build uses, the best env reports; a test reads
// it to say which one ran.
#if D_ENV_C_HAS_STDCKDINT_H
    #define D_CKD_TIER          D_CKD_TIER_STDCKDINT
    #include <stdckdint.h>     // ckd_add, ckd_sub, ckd_mul
#elif D_ENV_COMPILER_HAS_OVERFLOW_BUILTINS
    #define D_CKD_TIER          D_CKD_TIER_BUILTINS
#else
    #define D_CKD_TIER          D_CKD_TIER_PORTABLE
#endif  // D_ENV_C_HAS_STDCKDINT_H

// D_INTERNAL_CKD_SIZE_MAX
//   constant (internal): the largest size_t, spelled without <stdint.h>,
// which ISO C++98 does not have.
#define D_INTERNAL_CKD_SIZE_MAX ((size_t)-1)


//   C linkage for everything below, so that C and C++ see one set of names.
// Both spellings expand to nothing under a C compiler.
D_EXTERN_C_BEGIN


//==============================================================================
// 2.  CHECKED ARITHMETIC
//==============================================================================


// 2.1    The operations
//------------------------------------------------------------------------------
/**
 * @brief Adds two sizes, and says whether the sum fits in a size_t.
 *
 * @param[out] _result  receives _a + _b; where the sum does not fit, it
 *                      receives the sum wrapped modulo SIZE_MAX + 1, as
 *                      unsigned addition gives it.
 * @param[in]  _a       the first addend.
 * @param[in]  _b       the second addend.
 * @return true when _a + _b exceeds SIZE_MAX, false when it fits.
 * @warning _result must point to a size_t; it may not be null.
 */
D_NODISCARD D_INLINE
bool d_ckd_add(
    size_t* _result,
    size_t  _a,
    size_t  _b
)
{
#if (D_CKD_TIER == D_CKD_TIER_STDCKDINT)
    return ckd_add(_result, _a, _b);
#elif (D_CKD_TIER == D_CKD_TIER_BUILTINS)
    return D_ENV_COMPILER_ADD_OVERFLOW(_a, _b, _result);
#else
    *_result = _a + _b;

    return (*_result < _a);
#endif  // D_CKD_TIER
}

/**
 * @brief Subtracts one size from another, and says whether the difference
 *        fits in a size_t.
 *
 * @param[out] _result  receives _a - _b; where _b exceeds _a, it receives the
 *                      difference wrapped modulo SIZE_MAX + 1, as unsigned
 *                      subtraction gives it.
 * @param[in]  _a       the minuend.
 * @param[in]  _b       the subtrahend.
 * @return true when _b exceeds _a, false when the difference fits.
 * @warning _result must point to a size_t; it may not be null.
 */
D_NODISCARD D_INLINE
bool d_ckd_sub(
    size_t* _result,
    size_t  _a,
    size_t  _b
)
{
#if (D_CKD_TIER == D_CKD_TIER_STDCKDINT)
    return ckd_sub(_result, _a, _b);
#elif (D_CKD_TIER == D_CKD_TIER_BUILTINS)
    return D_ENV_COMPILER_SUB_OVERFLOW(_a, _b, _result);
#else
    *_result = _a - _b;

    return (_b > _a);
#endif  // D_CKD_TIER
}

/**
 * @brief Multiplies two sizes, and says whether the product fits in a
 *        size_t.
 *
 * @param[out] _result  receives _a * _b; where the product does not fit, it
 *                      receives the product wrapped modulo SIZE_MAX + 1, as
 *                      unsigned multiplication gives it.
 * @param[in]  _a       the first factor: a count, typically.
 * @param[in]  _b       the second factor: an element's size, typically.
 * @return true when _a * _b exceeds SIZE_MAX, false when it fits.
 * @warning _result must point to a size_t; it may not be null.
 */
D_NODISCARD D_INLINE
bool d_ckd_mul(
    size_t* _result,
    size_t  _a,
    size_t  _b
)
{
#if (D_CKD_TIER == D_CKD_TIER_STDCKDINT)
    return ckd_mul(_result, _a, _b);
#elif (D_CKD_TIER == D_CKD_TIER_BUILTINS)
    return D_ENV_COMPILER_MUL_OVERFLOW(_a, _b, _result);
#else
    *_result = _a * _b;

    // the division never produces the wrapped value it tests for
    return ( (_a != 0) &&
             (_b > (D_INTERNAL_CKD_SIZE_MAX / _a)) );
#endif  // D_CKD_TIER
}


D_EXTERN_C_END


#endif  // DJINTERP_C_DCKDINT_H
