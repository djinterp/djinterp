/*******************************************************************************
* djinterp [re_std]                                                      lcm.hpp
*
* lcm algorithm header:
* lcm(_a, _b) returns the least common multiple of |_a| and |_b|.
*
* return type: common_type<M, N>::type. Always non-negative.
*
* identity:  lcm(0, k) == 0  for any k (including k = 0).
*
* implementation:  |a / gcd(a, b)| * |b|, with the division done first
* to reduce intermediate overflow. UB if the result is not
* representable in the common type — std requires the same.
*
*
* path:      /inc/re_std/numeric/lcm.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_LCM_HPP
#define RE_STD_NUMERIC_LCM_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <type_traits>

    #include "re_std/numeric/gcd.hpp"


namespace re_std
{

template<typename M, typename N>
RE_STD_CONSTEXPR_CPP14 typename std::common_type<M, N>::type
lcm(M _a, N _b) RE_STD_NOEXCEPT
{
    static_assert(std::is_integral<M>::value && std::is_integral<N>::value,
                  "re_std::lcm requires integer arguments");
    static_assert(!std::is_same<typename std::remove_cv<M>::type, bool>::value,
                  "re_std::lcm does not accept bool");
    static_assert(!std::is_same<typename std::remove_cv<N>::type, bool>::value,
                  "re_std::lcm does not accept bool");

    typedef typename std::common_type<M, N>::type _R;
    typedef typename std::make_unsigned<_R>::type   _UR;

    if (_a == 0 || _b == 0) return 0;

    const _UR _au = static_cast<_UR>(internal::gcd_abs_signed(_a));
    const _UR _bu = static_cast<_UR>(internal::gcd_abs_signed(_b));
    // Divide first to limit overflow of the intermediate product.
    return static_cast<_R>((_au / internal::gcd_kernel(_au, _bu)) * _bu);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_LCM_HPP
