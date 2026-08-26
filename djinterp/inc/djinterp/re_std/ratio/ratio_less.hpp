/******************************************************************************
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
* path:      /inc/djinterp/re_std/ratio/ratio_less.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_LESS_
#define DJINTERP_RE_STD_RATIO_RATIO_LESS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"
#include "../type_traits/integral_constant.hpp"


NS_RESTD


// ===========================================================================
// I.   INTERNAL: CONTINUED-FRACTION COMPARISON
// ===========================================================================

NS_INTERNAL

    // ratio_less_walk
    //   trait: n1/d1 < n2/d2 for NON-NEGATIVE numerators and POSITIVE
    // denominators. Forward-declared so the step helper can name it.
    template<std::intmax_t _N1, std::intmax_t _D1,
             std::intmax_t _N2, std::intmax_t _D2>
    struct ratio_less_walk;

    // ratio_less_step
    //   helper: one step of the walk. _Recurse is computed by the caller
    // so that exactly one of these two specialisations is instantiated --
    // a ternary would instantiate both arms and never terminate.
    template<std::intmax_t _N1, std::intmax_t _D1,
             std::intmax_t _N2, std::intmax_t _D2,
             bool          _Recurse>
    struct ratio_less_step
    {
        // terminal: the integer parts differ, or one side divides evenly.
        static const std::intmax_t _s_q1 = _N1 / _D1;
        static const std::intmax_t _s_r1 = _N1 % _D1;
        static const std::intmax_t _s_q2 = _N2 / _D2;
        static const std::intmax_t _s_r2 = _N2 % _D2;

        static const bool value =
            ( _s_q1 != _s_q2 ) ? ( _s_q1 < _s_q2 )
                               : ( _s_r1 == 0 ? ( _s_r2 != 0 ) : false );
    };

    // recursive step: integer parts agree and both remainders are
    // non-zero, so compare the inverted fractional parts -- which swaps
    // the operand order, because inverting reverses the comparison.
    template<std::intmax_t _N1, std::intmax_t _D1,
             std::intmax_t _N2, std::intmax_t _D2>
    struct ratio_less_step<_N1, _D1, _N2, _D2, true>
    {
        static const bool value =
            ratio_less_walk<_D2, _N2 % _D2, _D1, _N1 % _D1>::value;
    };

    template<std::intmax_t _N1, std::intmax_t _D1,
             std::intmax_t _N2, std::intmax_t _D2>
    struct ratio_less_walk
    {
        static const bool value = ratio_less_step<
            _N1, _D1, _N2, _D2,
            ( ( _N1 / _D1 == _N2 / _D2 ) &&
              ( _N1 % _D1 != 0 )         &&
              ( _N2 % _D2 != 0 ) )>::value;
    };


    // ratio_less_signed
    //   helper: sign dispatch. _S1 / _S2 are "numerator is negative".
    // Only the both-non-negative case reaches the walk.
    template<typename _R1, typename _R2,
             bool _S1 = (_R1::num < 0),
             bool _S2 = (_R2::num < 0)>
    struct ratio_less_signed;

    // negative < non-negative
    template<typename _R1, typename _R2>
    struct ratio_less_signed<_R1, _R2, true, false>
    {
        static const bool value = true;
    };

    // non-negative < negative is never true
    template<typename _R1, typename _R2>
    struct ratio_less_signed<_R1, _R2, false, true>
    {
        static const bool value = false;
    };

    // both non-negative: walk directly
    template<typename _R1, typename _R2>
    struct ratio_less_signed<_R1, _R2, false, false>
    {
        static const bool value =
            ratio_less_walk<_R1::num, _R1::den,
                            _R2::num, _R2::den>::value;
    };

    // both negative: -a < -b is b < a, so negate and swap
    template<typename _R1, typename _R2>
    struct ratio_less_signed<_R1, _R2, true, true>
    {
        static const bool value =
            ratio_less_walk<-_R2::num, _R2::den,
                            -_R1::num, _R1::den>::value;
    };

NS_END  // internal


// ===========================================================================
// II.  RATIO_LESS
// ===========================================================================

// ratio_less
//   trait: whether R1 is strictly less than R2. Never multiplies, so it
// cannot overflow for any representable pair of operands.
template<typename _R1,
         typename _R2>
struct ratio_less
    : integral_constant<bool, internal::ratio_less_signed<_R1, _R2>::value>
{};


// ===========================================================================
// III. RATIO_LESS_V (C++14+ variable)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _R1,
         typename _R2>
D_CONSTEXPR bool ratio_less_v = ratio_less<_R1, _R2>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_LESS_
