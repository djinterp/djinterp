/***********************************************************************
* re_std                                                              lcm.hpp
*
* lcm(_a, _b) returns the least common multiple of |_a| and |_b|.
*
* return type: common_type<_M, _N>::type. Always non-negative.
*
* identity:  lcm(0, k) == 0  for any k (including k = 0).
*
* implementation:  |a / gcd(a, b)| * |b|, with the division done first
* to reduce intermediate overflow. UB if the result is not
* representable in the common type — std requires the same.
*
*
* path:      /inc/djinterp/re_std/numeric/lcm.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_LCM_
#define DJINTERP_RE_STD_NUMERIC_LCM_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <type_traits>

    #include "re_std/numeric/gcd.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{

template<typename _M, typename _N>
D_CONSTEXPR_CPP14 typename std::common_type<_M, _N>::type
lcm(_M _a, _N _b) D_NOEXCEPT
{
    static_assert(std::is_integral<_M>::value && std::is_integral<_N>::value,
                  "re_std::lcm requires integer arguments");
    static_assert(!std::is_same<typename std::remove_cv<_M>::type, bool>::value,
                  "re_std::lcm does not accept bool");
    static_assert(!std::is_same<typename std::remove_cv<_N>::type, bool>::value,
                  "re_std::lcm does not accept bool");

    typedef typename std::common_type<_M, _N>::type _R;
    typedef typename std::make_unsigned<_R>::type   _UR;

    if (_a == 0 || _b == 0) return 0;

    const _UR _au = static_cast<_UR>(internal::gcd_abs_signed(_a));
    const _UR _bu = static_cast<_UR>(internal::gcd_abs_signed(_b));
    // Divide first to limit overflow of the intermediate product.
    return static_cast<_R>((_au / internal::gcd_kernel(_au, _bu)) * _bu);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_NUMERIC_LCM_
