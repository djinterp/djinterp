/***********************************************************************
* restd                                                               lcm.hpp
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
* path:      /inc/restd/numeric/lcm.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_LCM_
#define RESTD_NUMERIC_LCM_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <type_traits>

    #include "restd/numeric/gcd.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{

template<typename _M, typename _N>
D_CONSTEXPR_CPP14 typename std::common_type<_M, _N>::type
lcm(_M _a, _N _b) D_NOEXCEPT
{
    static_assert(std::is_integral<_M>::value && std::is_integral<_N>::value,
                  "restd::lcm requires integer arguments");
    static_assert(!std::is_same<typename std::remove_cv<_M>::type, bool>::value,
                  "restd::lcm does not accept bool");
    static_assert(!std::is_same<typename std::remove_cv<_N>::type, bool>::value,
                  "restd::lcm does not accept bool");

    typedef typename std::common_type<_M, _N>::type _R;
    typedef typename std::make_unsigned<_R>::type   _UR;

    if (_a == 0 || _b == 0) return 0;

    const _UR _au = static_cast<_UR>(internal::gcd_abs_signed(_a));
    const _UR _bu = static_cast<_UR>(internal::gcd_abs_signed(_b));
    // Divide first to limit overflow of the intermediate product.
    return static_cast<_R>((_au / internal::gcd_kernel(_au, _bu)) * _bu);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_NUMERIC_LCM_
