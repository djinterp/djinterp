/*******************************************************************************
* djinterp [re_std]                                               ratio_less.hpp
*
* ratio_less header:
*   ratio_less<R1, R2> is true_type iff R1 < R2 as rationals.
*
*   THIS IS THE HARD ONE. Comparing n1/d1 against n2/d2 by
* cross-multiplying to n1*d2 < n2*d1 is correct arithmetic and a bad
* implementation: both products overflow for operands whose comparison
* is perfectly well-defined. ratio_less<ratio<INTMAX_MAX, 2>,
* ratio<INTMAX_MAX, 1>> is obviously true, and obviously overflows.
*
*   Implementations usually answer this with a 128-bit multiply,
* emulated in 32-bit halves when the compiler has no wide type. This
* one instead walks the CONTINUED-FRACTION expansion of both operands:
*
*     n1/d1 = q1 + r1/d1        n2/d2 = q2 + r2/d2
*
*   If the integer parts differ, they settle it. If they match, the
* question reduces to r1/d1 < r2/d2, which -- inverting both sides,
* which reverses the comparison -- is d2/r2 < d1/r1. That is the same
* problem on strictly smaller numbers, so the recursion is Euclid's
* algorithm and terminates in O(log n) steps.
*
*   Nothing is ever multiplied. No intermediate can exceed the largest
* input. No wide type is needed on any platform.
*
*   The recursion is driven through a bool-dispatched helper rather
* than a ternary: in a template, both arms of a ternary are
* instantiated, so a self-referential ternary would recurse forever at
* compile time regardless of which branch the value selects.
*
*   SIGNS ARE HANDLED BEFORE THE WALK:
*   Opposite signs settle immediately. Two negatives are compared by
* negating and swapping, since -a < -b is b < a. Only the
* both-non-negative case reaches the continued-fraction walk, which is
* what lets it assume positive denominators and remainders.
*
*   PORTABILITY:
*   C++11 in std; the _v spelling is C++17 in std and C++14 here.
*
*
* path:      /inc/re_std/ratio/ratio_less.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_LESS_HPP
#define RE_STD_RATIO_RATIO_LESS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"
#include "../type_traits/integral_constant.hpp"


namespace re_std
{


// ===========================================================================
// I.   INTERNAL: CONTINUED-FRACTION COMPARISON
// ===========================================================================

namespace internal
{

    // ratio_less_walk
    //   trait: n1/d1 < n2/d2 for NON-NEGATIVE numerators and POSITIVE
    // denominators. Forward-declared so the step helper can name it.
    template<intmax_t N1, intmax_t D1,
             intmax_t N2, intmax_t D2>
    struct ratio_less_walk;

    // ratio_less_step
    //   trait: one step of the walk. Recurse is computed by the caller
    // so that exactly one of these two specialisations is instantiated --
    // a ternary would instantiate both arms and never terminate.
    template<intmax_t N1, intmax_t D1,
             intmax_t N2, intmax_t D2,
             bool          Recurse>
    struct ratio_less_step
    {
        // terminal: the integer parts differ, or one side divides evenly.
        static const intmax_t _s_q1 = N1 / D1;
        static const intmax_t _s_r1 = N1 % D1;
        static const intmax_t _s_q2 = N2 / D2;
        static const intmax_t _s_r2 = N2 % D2;

        static const bool value =
            ( _s_q1 != _s_q2 ) ? ( _s_q1 < _s_q2 )
                               : ( _s_r1 == 0 ? ( _s_r2 != 0 ) : false );
    };

    // recursive step: integer parts agree and both remainders are
    // non-zero, so compare the inverted fractional parts -- which swaps
    // the operand order, because inverting reverses the comparison.
    template<intmax_t N1, intmax_t D1,
             intmax_t N2, intmax_t D2>
    struct ratio_less_step<N1, D1, N2, D2, true>
    {
        static const bool value =
            ratio_less_walk<D2, N2 % D2, D1, N1 % D1>::value;
    };

    template<intmax_t N1, intmax_t D1,
             intmax_t N2, intmax_t D2>
    struct ratio_less_walk
    {
        static const bool value = ratio_less_step<
            N1, D1, N2, D2,
            ( ( N1 / D1 == N2 / D2 ) &&
              ( N1 % D1 != 0 )         &&
              ( N2 % D2 != 0 ) )>::value;
    };


    // ratio_less_signed
    //   trait: sign dispatch. S1 / _S2 are "numerator is negative".
    // Only the both-non-negative case reaches the walk.
    template<typename R1, typename R2,
             bool S1 = (R1::num < 0),
             bool _S2 = (R2::num < 0)>
    struct ratio_less_signed;

    // negative < non-negative
    template<typename R1, typename R2>
    struct ratio_less_signed<R1, R2, true, false>
    {
        static const bool value = true;
    };

    // non-negative < negative is never true
    template<typename R1, typename R2>
    struct ratio_less_signed<R1, R2, false, true>
    {
        static const bool value = false;
    };

    // both non-negative: walk directly
    template<typename R1, typename R2>
    struct ratio_less_signed<R1, R2, false, false>
    {
        static const bool value =
            ratio_less_walk<R1::num, R1::den,
                            R2::num, R2::den>::value;
    };

    // both negative: -a < -b is b < a, so negate and swap
    template<typename R1, typename R2>
    struct ratio_less_signed<R1, R2, true, true>
    {
        static const bool value =
            ratio_less_walk<-R2::num, R2::den,
                            -R1::num, R1::den>::value;
    };

}  // internal


// ===========================================================================
// II.  RATIO_LESS
// ===========================================================================

// ratio_less
//   trait: whether R1 is strictly less than R2. Never multiplies, so it
// cannot overflow for any representable pair of operands.
template<typename R1,
         typename R2>
struct ratio_less
    : integral_constant<bool, internal::ratio_less_signed<R1, R2>::value>
{};


// ===========================================================================
// III. RATIO_LESS_V (C++14+ variable)
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename R1,
         typename R2>
RE_STD_CONSTEXPR bool ratio_less_v = ratio_less<R1, R2>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_LESS_HPP
