/*******************************************************************************
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
* path:      /inc/re_std/ratio/ratio_multiply.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_MULTIPLY_HPP
#define RE_STD_RATIO_RATIO_MULTIPLY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"


namespace re_std
{


// ===========================================================================
// I.   RATIO_MULTIPLY
// ===========================================================================

namespace internal
{

    // ratio_multiply_impl
    //   trait: cross-reduces before multiplying. Split out so the public
    // alias below stays a one-liner.
    template<typename R1,
             typename R2>
    struct ratio_multiply_impl
    {
    private:
        static const intmax_t _s_g1 =
            ratio_gcd< ratio_abs<R1::num>::value,
                       ratio_abs<R2::den>::value >::value;
        static const intmax_t _s_g2 =
            ratio_gcd< ratio_abs<R2::num>::value,
                       ratio_abs<R1::den>::value >::value;

    public:
        typedef ratio< (R1::num / _s_g1) * (R2::num / _s_g2),
                       (R1::den / _s_g2) * (R2::den / _s_g1) > type;
    };

}  // internal

// ratio_multiply
//   alias: the reduced product of two ratios.
template<typename R1,
         typename R2>
struct ratio_multiply
    : internal::ratio_multiply_impl<R1, R2>::type
{};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_MULTIPLY_HPP
