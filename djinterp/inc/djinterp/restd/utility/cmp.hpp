/***********************************************************************
* restd                                                           cmp.hpp
*
* sign-aware integer comparisons:
*   The six cmp_* functions plus in_range, all introduced in C++20.
* Standard arithmetic comparison operators on mixed-signedness integer
* types yield surprising results because the signed operand is
* implicit-converted to unsigned, e.g. -1 < 1u is false. cmp_* honour
* mathematical order regardless of representation.
*
*   Family:
*     cmp_equal        -- mathematical equality
*     cmp_not_equal    -- !cmp_equal
*     cmp_less         -- mathematical less-than
*     cmp_greater      -- cmp_less with operands swapped
*     cmp_less_equal   -- !cmp_greater
*     cmp_greater_equal-- !cmp_less
*
*     in_range<R>(t)   -- true iff t fits in R's representable range,
*                         using the cmp_less rules.
*
*   STANDARD STATUS:
*   C++20. restd back-ports to C++11+. Use of the trait surface
* (make_unsigned, numeric_limits replacements) is restricted to what
* restd already ships -- in particular, integral_constant-based
* overload selection rather than C++17's if-constexpr.
*
*   IMPLEMENTATION NOTE:
*   C++20's spec uses if-constexpr inside a single function. We use
* three SFINAE-selected overloads per cmp_* function (T-and-U same
* signedness, T signed / U unsigned, T unsigned / U signed). Compiles
* to identical code under reasonable optimisation.
*
*
* path:      /inc/restd/utility/cmp.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_CMP_
#define RESTD_UTILITY_CMP_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_signed.hpp"
#include "../type_traits/make_unsigned.hpp"

NS_RESTD

// =============================================================================
// CMP_EQUAL
// =============================================================================

// cmp_equal: same signedness -- direct.
template<typename _T, typename _U>
D_CONSTEXPR
typename enable_if<
    is_signed<_T>::value == is_signed<_U>::value,
    bool
>::type
cmp_equal(_T _t, _U _u) noexcept
{
    return _t == _u;
}

// cmp_equal: T signed, U unsigned.
template<typename _T, typename _U>
D_CONSTEXPR
typename enable_if<
    is_signed<_T>::value && !is_signed<_U>::value,
    bool
>::type
cmp_equal(_T _t, _U _u) noexcept
{
    return _t < 0
        ? false
        : static_cast<typename make_unsigned<_T>::type>(_t) == _u;
}

// cmp_equal: T unsigned, U signed.
template<typename _T, typename _U>
D_CONSTEXPR
typename enable_if<
    !is_signed<_T>::value && is_signed<_U>::value,
    bool
>::type
cmp_equal(_T _t, _U _u) noexcept
{
    return _u < 0
        ? false
        : _t == static_cast<typename make_unsigned<_U>::type>(_u);
}

// =============================================================================
// CMP_NOT_EQUAL
// =============================================================================

// cmp_not_equal: defined in terms of cmp_equal.
template<typename _T, typename _U>
D_CONSTEXPR bool cmp_not_equal(_T _t, _U _u) noexcept
{
    return !cmp_equal(_t, _u);
}

// =============================================================================
// CMP_LESS
// =============================================================================

// cmp_less: same signedness -- direct.
template<typename _T, typename _U>
D_CONSTEXPR
typename enable_if<
    is_signed<_T>::value == is_signed<_U>::value,
    bool
>::type
cmp_less(_T _t, _U _u) noexcept
{
    return _t < _u;
}

// cmp_less: T signed, U unsigned. Negative t is always less than
// any unsigned u; otherwise compare as unsigned.
template<typename _T, typename _U>
D_CONSTEXPR
typename enable_if<
    is_signed<_T>::value && !is_signed<_U>::value,
    bool
>::type
cmp_less(_T _t, _U _u) noexcept
{
    return _t < 0
        ? true
        : static_cast<typename make_unsigned<_T>::type>(_t) < _u;
}

// cmp_less: T unsigned, U signed. Negative u is never greater than
// any unsigned t; otherwise compare as unsigned.
template<typename _T, typename _U>
D_CONSTEXPR
typename enable_if<
    !is_signed<_T>::value && is_signed<_U>::value,
    bool
>::type
cmp_less(_T _t, _U _u) noexcept
{
    return _u < 0
        ? false
        : _t < static_cast<typename make_unsigned<_U>::type>(_u);
}

// =============================================================================
// CMP_GREATER / CMP_LESS_EQUAL / CMP_GREATER_EQUAL
// =============================================================================

// cmp_greater: cmp_less with arguments reversed.
template<typename _T, typename _U>
D_CONSTEXPR bool cmp_greater(_T _t, _U _u) noexcept
{
    return cmp_less(_u, _t);
}

// cmp_less_equal: !(t > u) = !cmp_less(u, t).
template<typename _T, typename _U>
D_CONSTEXPR bool cmp_less_equal(_T _t, _U _u) noexcept
{
    return !cmp_less(_u, _t);
}

// cmp_greater_equal: !(t < u) = !cmp_less(t, u).
template<typename _T, typename _U>
D_CONSTEXPR bool cmp_greater_equal(_T _t, _U _u) noexcept
{
    return !cmp_less(_t, _u);
}

// =============================================================================
// IN_RANGE
// =============================================================================

// in_range<_R>(_t)
//   function: true iff _t is representable as a value of type _R.
//   Uses cmp_less_equal twice: R::min() <= t and t <= R::max().
//
//   Without <limits>, we cannot easily synthesise R::min() / R::max();
//   instead we use the integer-promotion bounds derived from
//   make_unsigned / is_signed at compile time.
//
//   IMPLEMENTATION:
//   The standard's wording uses std::numeric_limits<R>::min() and ::max().
//   This implementation reproduces those bounds without depending on
//   <limits> by computing them from is_signed and the bit-width of R.
template<typename _R, typename _T>
D_CONSTEXPR bool in_range(_T _t) noexcept
{
    // Compute R's min / max via bit-pattern reinterpretation on the
    // unsigned companion type:
    //   unsigned R: min = 0;            max = ~UR(0)
    //     signed R: min = (UR(1) << k); max = (UR(1) << k) - 1   where k = bits-1
    // For signed-min we cast the unsigned 0x80...0 directly to _R rather
    // than using `-pos_max - 1` style. That conversion is implementation-
    // defined on C++17 and earlier, but yields the minimum value on every
    // two's-complement platform we target; C++20 made it well-defined
    // (P0907R4 mandates two's-complement representation). Crucially we
    // avoid `-static_cast<_R>(0x80...0)` -- negating INT_MIN is itself
    // undefined behaviour, which the obvious-looking formulation hits.
    return cmp_greater_equal(
               _t,
               is_signed<_R>::value
                   ? static_cast<_R>(
                       static_cast<typename make_unsigned<_R>::type>(1)
                       << (sizeof(_R) * 8 - 1))
                   : static_cast<_R>(0))
        && cmp_less_equal(
               _t,
               is_signed<_R>::value
                   ? static_cast<_R>(
                       (static_cast<typename make_unsigned<_R>::type>(1)
                        << (sizeof(_R) * 8 - 1)) - 1)
                   : static_cast<_R>(
                       ~static_cast<typename make_unsigned<_R>::type>(0)));
}

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_CMP_
