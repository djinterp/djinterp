/*******************************************************************************
* djinterp [re_std]                                                      gcd.hpp
*
* gcd(_a, _b) returns the greatest common divisor of |_a| and |_b|.
*
* return type:
*   common_type<M, N>::type — the wider of the two integral types.
*   The result is always non-negative.
*
* preconditions (per the standard):
*   - both M and N are integer types other than bool.
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
* path:      /inc/re_std/numeric/gcd.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_GCD_HPP
#define RE_STD_NUMERIC_GCD_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <type_traits>


namespace re_std
{
namespace internal
{

    // abs-as-unsigned: for signed T, return the unsigned magnitude
    // even if _v is the type's minimum (where -_v would overflow as
    // a signed expression). For unsigned T it's identity.
    template<typename T>
    constexpr typename std::make_unsigned<T>::type
    gcd_abs_signed(T _v) RE_STD_NOEXCEPT
    {
        return _v < 0
            ? static_cast<typename std::make_unsigned<T>::type>(0)
              - static_cast<typename std::make_unsigned<T>::type>(_v)
            : static_cast<typename std::make_unsigned<T>::type>(_v);
    }

    // The Euclidean kernel. Operates entirely in unsigned space so
    // we can't accidentally produce a negative intermediate.
    template<typename U>
    RE_STD_CONSTEXPR_CPP14 U gcd_kernel(U _a, U _b) RE_STD_NOEXCEPT
    {
        while (_b != 0)
        {
            U _t = _b;
            _b = _a % _b;
            _a = _t;
        }
        return _a;
    }

}  // internal
template<typename M, typename N>
RE_STD_CONSTEXPR_CPP14 typename std::common_type<M, N>::type
gcd(M _a, N _b) RE_STD_NOEXCEPT
{
    static_assert(std::is_integral<M>::value && std::is_integral<N>::value,
                  "re_std::gcd requires integer arguments");
    static_assert(!std::is_same<typename std::remove_cv<M>::type, bool>::value,
                  "re_std::gcd does not accept bool");
    static_assert(!std::is_same<typename std::remove_cv<N>::type, bool>::value,
                  "re_std::gcd does not accept bool");

    typedef typename std::common_type<M, N>::type           _R;
    typedef typename std::make_unsigned<_R>::type             _UR;

    const _UR _au = static_cast<_UR>(internal::gcd_abs_signed(_a));
    const _UR _bu = static_cast<_UR>(internal::gcd_abs_signed(_b));
    return static_cast<_R>(internal::gcd_kernel(_au, _bu));
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_GCD_HPP
