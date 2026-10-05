/*******************************************************************************
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
* path:      /inc/re_std/ratio/ratio.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_HPP
#define RE_STD_RATIO_RATIO_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <climits>

// re_std
#include "../cstdint/cstdint.hpp"  // intmax_t, INTMAX_MIN, INTMAX_MAX


namespace re_std
{


// ===========================================================================
// I.   INTERNAL: SIGN, ABS, GCD
// ===========================================================================

namespace internal
{

    // ratio_sign
    //   trait: -1, 0 or +1. Used to move a negative denominator's sign
    // onto the numerator during normalisation.
    template<intmax_t V>
    struct ratio_sign
    {
        static const intmax_t value = (V < 0) ? -1 : ((V > 0) ? 1 : 0);
    };

    // ratio_abs
    //   trait: magnitude. Safe here only because ratio.hpp static_asserts
    // that neither parameter is the most-negative intmax_t, whose
    // negation is not representable.
    template<intmax_t V>
    struct ratio_abs
    {
        static const intmax_t value = (V < 0) ? -V : V;
    };

    // ratio_gcd
    //   trait: Euclid on the type system. Operands must be non-negative.
    // gcd(x, 0) is x, which gives gcd(0, d) == d and makes ratio<0, D>
    // normalise to 0/1 rather than dividing by zero.
    template<intmax_t A,
             intmax_t B>
    struct ratio_gcd
    {
        static const intmax_t value = ratio_gcd<B, A % B>::value;
    };

    template<intmax_t A>
    struct ratio_gcd<A, 0>
    {
        static const intmax_t value = A;
    };

}  // internal


// ===========================================================================
// II.  RATIO
// ===========================================================================

// ratio
//   class: the reduced rational Num/Den. num and den are the reduced
// form with den > 0; `type` names that reduced ratio, so ratio<2,4>::type
// is ratio<1,2>.
template<intmax_t Num,
         intmax_t Den = 1>
class ratio
{
private:
    static const intmax_t _s_gcd =
        internal::ratio_gcd< internal::ratio_abs<Num>::value,
                             internal::ratio_abs<Den>::value >::value;

public:
    // A zero denominator is not a run-time error to be diagnosed later;
    // it is a malformed type, so it is rejected at definition.
    static_assert(Den != 0,
        "re_std::ratio: denominator may not be zero");

    // The most-negative intmax_t has no representable negation, so it
    // cannot be normalised. Rejecting it here is what lets ratio_abs and
    // the sign flip in ratio_subtract stay honest everywhere else.
    static_assert(Num != INTMAX_MIN && Den != INTMAX_MIN,
        "re_std::ratio: numerator and denominator must be negatable");

    static const intmax_t num =
        Num * internal::ratio_sign<Den>::value / _s_gcd;

    static const intmax_t den =
        internal::ratio_abs<Den>::value / _s_gcd;

    typedef ratio<num, den> type;
};


// Out-of-class definitions. Before C++17 a static const data member that
// is odr-used -- bound to a reference, or address-taken -- still needs
// one, and duration/time_point in <chrono> will do exactly that. From
// C++17 the in-class initialiser is itself the definition and repeating
// it is deprecated, so the definitions are gated.
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER

    template<intmax_t Num, intmax_t Den>
    const intmax_t ratio<Num, Den>::num;

    template<intmax_t Num, intmax_t Den>
    const intmax_t ratio<Num, Den>::den;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_HPP
