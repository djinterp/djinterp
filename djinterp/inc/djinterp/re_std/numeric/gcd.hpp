/***********************************************************************
* restd                                                               gcd.hpp
*
* gcd(_a, _b) returns the greatest common divisor of |_a| and |_b|.
*
* return type:
*   common_type<_M, _N>::type — the wider of the two integral types.
*   The result is always non-negative.
*
* preconditions (per the standard):
*   - both _M and _N are integer types other than bool.
*   - the absolute values of _a and _b must be representable in the
*     common type.
*
* implementation strategy:
*   - convert each input to its corresponding unsigned type via the
*     "unsigned-cast of negation when negative" trick, which avoids
*     UB on INT_MIN.
*   - apply the Euclidean algorithm in unsigned space.
*   - cast back to common_type.
*
* added in std C++17; constexpr from inception.
*
*
* path:      /inc/djinterp/re_std/numeric/gcd.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_GCD_
#define RESTD_NUMERIC_GCD_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <type_traits>


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{
namespace internal
{

    // abs-as-unsigned: for signed _T, return the unsigned magnitude
    // even if _v is the type's minimum (where -_v would overflow as
    // a signed expression). For unsigned _T it's identity.
    template<typename _T>
    constexpr typename std::make_unsigned<_T>::type
    gcd_abs_signed(_T _v) D_NOEXCEPT
    {
        return _v < 0
            ? static_cast<typename std::make_unsigned<_T>::type>(0)
              - static_cast<typename std::make_unsigned<_T>::type>(_v)
            : static_cast<typename std::make_unsigned<_T>::type>(_v);
    }

    // The Euclidean kernel. Operates entirely in unsigned space so
    // we can't accidentally produce a negative intermediate.
    template<typename _U>
    D_CONSTEXPR_CPP14 _U gcd_kernel(_U _a, _U _b) D_NOEXCEPT
    {
        while (_b != 0)
        {
            _U _t = _b;
            _b = _a % _b;
            _a = _t;
        }
        return _a;
    }

}  // namespace internal


template<typename _M, typename _N>
D_CONSTEXPR_CPP14 typename std::common_type<_M, _N>::type
gcd(_M _a, _N _b) D_NOEXCEPT
{
    static_assert(std::is_integral<_M>::value && std::is_integral<_N>::value,
                  "restd::gcd requires integer arguments");
    static_assert(!std::is_same<typename std::remove_cv<_M>::type, bool>::value,
                  "restd::gcd does not accept bool");
    static_assert(!std::is_same<typename std::remove_cv<_N>::type, bool>::value,
                  "restd::gcd does not accept bool");

    typedef typename std::common_type<_M, _N>::type           _R;
    typedef typename std::make_unsigned<_R>::type             _UR;

    const _UR _au = static_cast<_UR>(internal::gcd_abs_signed(_a));
    const _UR _bu = static_cast<_UR>(internal::gcd_abs_signed(_b));
    return static_cast<_R>(internal::gcd_kernel(_au, _bu));
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_NUMERIC_GCD_
