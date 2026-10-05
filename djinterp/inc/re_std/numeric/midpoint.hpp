/*******************************************************************************
* djinterp [re_std]                                                 midpoint.hpp
*
* midpoint algorithm header:
* midpoint(_a, _b) returns the average of _a and _b without overflow.
*
* For INTEGRAL types, the obvious (a + b) / 2 formula overflows when
* a + b exceeds the type's range. The standard mandates an overflow-
* free formulation that rounds toward _a:
*
*   midpoint(7, 10)        ==  8   (rounds toward a == 7)
*   midpoint(10, 7)        ==  9   (rounds toward a == 10)
*   midpoint(INT_MAX, 1)   ==  representable correctly
*
* For POINTER types into the same array, returns a pointer halfway
* between (rounds toward _a). Per the standard, _a and _b must point
* into the same array (or be one-past-the-end of it); other pointer
* relationships are UB.
*
* deferred (separate follow-up phase):
*   - floating-point midpoint(double, double) — has subtle rounding
*     and infinity/NaN requirements that benefit from focused testing.
*
* added in std C++20.
*
*
* path:      /inc/re_std/numeric/midpoint.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.09
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_NUMERIC_MIDPOINT_HPP
#define RE_STD_NUMERIC_MIDPOINT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>
    #include <type_traits>


namespace re_std
{

// Integral overload. The trick:
//   - compute the unsigned magnitude of (b - a), divide by 2
//   - add that back to a (with sign correction if b < a)
//
// Because we work in the unsigned-of-common-type and only re-cast at
// the end, no intermediate overflow is possible.
template<typename T>
constexpr typename std::enable_if
<
    std::is_integral<T>::value
    && !std::is_same<typename std::remove_cv<T>::type, bool>::value,
    T
>::type
midpoint(T _a, T _b) RE_STD_NOEXCEPT
{
    typedef typename std::make_unsigned<T>::type _U;
    return _a > _b
        ? static_cast<T>(_a - static_cast<T>(static_cast<_U>(_a - _b) / 2))
        : static_cast<T>(_a + static_cast<T>(static_cast<_U>(_b - _a) / 2));
}

// Pointer overload — pointers into the same array.
template<typename T>
constexpr T* midpoint(T* _a, T* _b) RE_STD_NOEXCEPT
{
    return _a + (_b - _a) / 2;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_NUMERIC_MIDPOINT_HPP
