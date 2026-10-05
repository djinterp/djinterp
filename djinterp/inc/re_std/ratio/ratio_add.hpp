/*******************************************************************************
* djinterp [re_std]                                                ratio_add.hpp
*
* ratio_add header:
*   ratio_add<R1, R2> is the reduced sum R1 + R2.
*
*   THE DENOMINATOR GCD IS TAKEN FIRST, AND IT MATTERS:
*   The schoolbook form n1*d2 + n2*d1 over d1*d2 overflows far earlier
* than it needs to. Dividing out the denominators' common factor first
* keeps every intermediate as small as possible:
*
*     g  = gcd(den1, den2)
*     n  = num1 * (den2 / g) + num2 * (den1 / g)
*     d  = den1 * (den2 / g)                     [ = lcm(den1, den2) ]
*
*   For ratios that already share a denominator -- overwhelmingly the
* common case in <chrono>, where everything is some power of ten apart
* -- g is that denominator and the multipliers collapse to 1.
*
*   The result is handed to ratio, which reduces it; ratio_add itself
* does not attempt to reduce n and d.
*
*   PORTABILITY:
*   C++11, matching std.
*
*
* path:      /inc/re_std/ratio/ratio_add.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_RATIO_RATIO_ADD_HPP
#define RE_STD_RATIO_RATIO_ADD_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./ratio.hpp"


namespace re_std
{


// ===========================================================================
// I.   RATIO_ADD
// ===========================================================================

namespace internal
{

    // ratio_add_impl
    //   trait: lcm-based addition. See the header note for why the
    // denominator gcd is taken before anything is multiplied.
    template<typename R1,
             typename R2>
    struct ratio_add_impl
    {
    private:
        static const intmax_t _s_g =
            ratio_gcd<R1::den, R2::den>::value;

    public:
        typedef ratio<
            R1::num * (R2::den / _s_g) + R2::num * (R1::den / _s_g),
            R1::den * (R2::den / _s_g) > type;
    };

}  // internal

// ratio_add
//   alias: the reduced sum of two ratios.
template<typename R1,
         typename R2>
struct ratio_add
    : internal::ratio_add_impl<R1, R2>::type
{};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RATIO_RATIO_ADD_HPP
