/******************************************************************************
* djinterp [re_std]                                                    ratio.hpp
*
* ratio class header:
*   Exact rational arithmetic performed entirely in the type system.
* ratio<N, D> is a TYPE, not a value: it carries a numerator and a
* denominator as static members, and every operation on it produces
* another type. No ratio is ever instantiated at run time.
*
*     ratio<2, 4>::num  ->  1        (reduced on construction)
*     ratio<2, 4>::den  ->  2
*     ratio<1, -3>::num ->  -1       (sign normalised onto num)
*     ratio<1, -3>::den ->  3
*
*   NORMALISATION HAPPENS AT DEFINITION, NOT AT USE:
*   num and den are always the reduced form with a POSITIVE
* denominator, and `type` names the already-reduced ratio. That is what
* makes ratio_equal a plain member comparison rather than a
* cross-multiplication -- two ratios are equal exactly when their
* reduced forms match.
*
*   WHY intmax_t AND NOT A TEMPLATE PARAMETER TYPE:
*   The standard fixes the parameters as intmax_t so that ratio<1,3>
* names one type across the whole program regardless of how the
* literals were spelled. Keeping that matters more than the
* flexibility.
*
*   OVERFLOW IS THE ENTIRE DESIGN PROBLEM:
*   Every operation in this module is written to avoid intermediate
* overflow rather than to be short. The naive n1*d2 + n2*d1 overflows
* for operands that have a perfectly representable result, and the
* standard requires the result be correct whenever it is representable.
* See ratio_add.hpp and ratio_multiply.hpp for the two reductions, and
* ratio_less.hpp for the comparison, which uses a continued-fraction
* walk so it needs no type wider than intmax_t at all.
*
*   PORTABILITY:
*   std added <ratio> in C++11 and re_std matches it exactly -- no
* back-port, because intmax_t and the template machinery both arrive
* with C++11 and there is nothing below it to reach.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_
#define DJINTERP_RE_STD_RATIO_RATIO_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdint>
#include <climits>


NS_RESTD


// ===========================================================================
// I.   INTERNAL: SIGN, ABS, GCD
// ===========================================================================

NS_INTERNAL

    // ratio_sign
    //   trait: -1, 0 or +1. Used to move a negative denominator's sign
    // onto the numerator during normalisation.
    template<std::intmax_t _V>
    struct ratio_sign
    {
        static const std::intmax_t value = (_V < 0) ? -1 : ((_V > 0) ? 1 : 0);
    };

    // ratio_abs
    //   trait: magnitude. Safe here only because ratio.hpp static_asserts
    // that neither parameter is the most-negative intmax_t, whose
    // negation is not representable.
    template<std::intmax_t _V>
    struct ratio_abs
    {
        static const std::intmax_t value = (_V < 0) ? -_V : _V;
    };

    // ratio_gcd
    //   trait: Euclid on the type system. Operands must be non-negative.
    // gcd(x, 0) is x, which gives gcd(0, d) == d and makes ratio<0, D>
    // normalise to 0/1 rather than dividing by zero.
    template<std::intmax_t _A,
             std::intmax_t _B>
    struct ratio_gcd
    {
        static const std::intmax_t value = ratio_gcd<_B, _A % _B>::value;
    };

    template<std::intmax_t _A>
    struct ratio_gcd<_A, 0>
    {
        static const std::intmax_t value = _A;
    };

NS_END  // internal


// ===========================================================================
// II.  RATIO
// ===========================================================================

// ratio
//   class: the reduced rational _Num/_Den. num and den are the reduced
// form with den > 0; `type` names that reduced ratio, so ratio<2,4>::type
// is ratio<1,2>.
template<std::intmax_t _Num,
         std::intmax_t _Den = 1>
class ratio
{
private:
    static const std::intmax_t _s_gcd =
        internal::ratio_gcd< internal::ratio_abs<_Num>::value,
                             internal::ratio_abs<_Den>::value >::value;

public:
    // A zero denominator is not a run-time error to be diagnosed later;
    // it is a malformed type, so it is rejected at definition.
    static_assert(_Den != 0,
        "re_std::ratio: denominator may not be zero");

    // The most-negative intmax_t has no representable negation, so it
    // cannot be normalised. Rejecting it here is what lets ratio_abs and
    // the sign flip in ratio_subtract stay honest everywhere else.
    static_assert(_Num != INTMAX_MIN && _Den != INTMAX_MIN,
        "re_std::ratio: numerator and denominator must be negatable");

    static const std::intmax_t num =
        _Num * internal::ratio_sign<_Den>::value / _s_gcd;

    static const std::intmax_t den =
        internal::ratio_abs<_Den>::value / _s_gcd;

    typedef ratio<num, den> type;
};


// Out-of-class definitions. Before C++17 a static const data member that
// is odr-used -- bound to a reference, or address-taken -- still needs
// one, and duration/time_point in <chrono> will do exactly that. From
// C++17 the in-class initialiser is itself the definition and repeating
// it is deprecated, so the definitions are gated.
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER

    template<std::intmax_t _Num, std::intmax_t _Den>
    const std::intmax_t ratio<_Num, _Den>::num;

    template<std::intmax_t _Num, std::intmax_t _Den>
    const std::intmax_t ratio<_Num, _Den>::den;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_
