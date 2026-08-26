/******************************************************************************
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
* path:      /inc/djinterp/re_std/ratio/ratio_add.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_ADD_
#define DJINTERP_RE_STD_RATIO_RATIO_ADD_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./ratio.hpp"


NS_RESTD


// ===========================================================================
// I.   RATIO_ADD
// ===========================================================================

NS_INTERNAL

    // ratio_add_impl
    //   helper: lcm-based addition. See the header note for why the
    // denominator gcd is taken before anything is multiplied.
    template<typename _R1,
             typename _R2>
    struct ratio_add_impl
    {
    private:
        static const std::intmax_t _s_g =
            ratio_gcd<_R1::den, _R2::den>::value;

    public:
        typedef ratio<
            _R1::num * (_R2::den / _s_g) + _R2::num * (_R1::den / _s_g),
            _R1::den * (_R2::den / _s_g) > type;
    };

NS_END  // internal

// ratio_add
//   alias: the reduced sum of two ratios.
template<typename _R1,
         typename _R2>
struct ratio_add
    : internal::ratio_add_impl<_R1, _R2>::type
{};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_ADD_
