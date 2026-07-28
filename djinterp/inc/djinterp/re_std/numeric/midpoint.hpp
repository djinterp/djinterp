/***********************************************************************
* restd                                                          midpoint.hpp
*
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
* path:      /inc/restd/numeric/midpoint.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_MIDPOINT_
#define RESTD_NUMERIC_MIDPOINT_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <type_traits>


namespace restd
{

// Integral overload. The trick:
//   - compute the unsigned magnitude of (b - a), divide by 2
//   - add that back to a (with sign correction if b < a)
//
// Because we work in the unsigned-of-common-type and only re-cast at
// the end, no intermediate overflow is possible.
template<typename _T>
constexpr typename std::enable_if
<
    std::is_integral<_T>::value
    && !std::is_same<typename std::remove_cv<_T>::type, bool>::value,
    _T
>::type
midpoint(_T _a, _T _b) D_NOEXCEPT
{
    typedef typename std::make_unsigned<_T>::type _U;
    return _a > _b
        ? static_cast<_T>(_a - static_cast<_T>(static_cast<_U>(_a - _b) / 2))
        : static_cast<_T>(_a + static_cast<_T>(static_cast<_U>(_b - _a) / 2));
}

// Pointer overload — pointers into the same array.
template<typename _T>
constexpr _T* midpoint(_T* _a, _T* _b) D_NOEXCEPT
{
    return _a + (_b - _a) / 2;
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_NUMERIC_MIDPOINT_
