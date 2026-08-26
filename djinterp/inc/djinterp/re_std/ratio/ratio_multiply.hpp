/******************************************************************************
* djinterp [re_std]                                           ratio_multiply.hpp
*
* ratio_multiply header:
*   ratio_multiply<R1, R2> is the reduced product R1 * R2.
*
*   THE CROSS-REDUCTION, AND WHY IT IS NOT AN OPTIMISATION:
*   Computing num1*num2 / den1*den2 and reducing afterwards is wrong,
* not merely slow: the intermediate products overflow for operands
* whose reduced product is perfectly representable. ratio_multiply<
* ratio<INTMAX_MAX, 2>, ratio<2, INTMAX_MAX> > is exactly 1, but the
* naive numerator is 2*INTMAX_MAX.
*
*   So the common factors are cancelled ACROSS the two ratios first:
*
*     g1 = gcd(num1, den2)      g2 = gcd(num2, den1)
*     result = (num1/g1 * num2/g2) / (den1/g2 * den2/g1)
*
*   Each surviving factor is no larger than it was, so if the reduced
* result fits, every intermediate fits too.
*
*   PORTABILITY:
*   C++11, matching std.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_multiply.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_MULTIPLY_
#define DJINTERP_RE_STD_RATIO_RATIO_MULTIPLY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_MULTIPLY
// ===========================================================================

NS_INTERNAL

    // ratio_multiply_impl
    //   helper: cross-reduces before multiplying. Split out so the public
    // alias below stays a one-liner.
    template<typename _R1,
             typename _R2>
    struct ratio_multiply_impl
    {
    private:
        static const std::intmax_t _s_g1 =
            ratio_gcd< ratio_abs<_R1::num>::value,
                       ratio_abs<_R2::den>::value >::value;
        static const std::intmax_t _s_g2 =
            ratio_gcd< ratio_abs<_R2::num>::value,
                       ratio_abs<_R1::den>::value >::value;

    public:
        typedef ratio< (_R1::num / _s_g1) * (_R2::num / _s_g2),
                       (_R1::den / _s_g2) * (_R2::den / _s_g1) > type;
    };

NS_END  // internal

// ratio_multiply
//   alias: the reduced product of two ratios.
template<typename _R1,
         typename _R2>
struct ratio_multiply
    : internal::ratio_multiply_impl<_R1, _R2>::type
{};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_MULTIPLY_
