/*******************************************************************************
* djinterp [re_std]                                                   intcmp.hpp
*
* sign-safe integer comparison functions:
*   cmp_equal, cmp_not_equal, cmp_less, cmp_greater, cmp_less_equal,
*   cmp_greater_equal, in_range.
*
* WHY THIS EXISTS:
*   Direct comparison of integers of different signedness applies
* implicit conversions that produce mathematically wrong answers:
*
*     int(-1) < unsigned(0)    // false: -1 converts to UINT_MAX
*     int(-1) == unsigned(-1)  // true:  both convert to UINT_MAX
*
*   The cmp_* family treats both arguments as their actual integer
* values, regardless of declared type. The implementation strategy
* is dispatch on the signedness pair:
*
*     same signedness         -> direct comparison (already correct)
*     signed cmp unsigned     -> if signed < 0, signed is "smaller";
*                                otherwise compare unsigned-vs-unsigned
*     unsigned cmp signed     -> mirror image
*
*   in_range<T>(_v) returns whether _v fits in T's range, computed
* via the cmp_* primitives so the same sign-safety holds.
*
* added in std C++20.
*
*
* DEPENDENCY NOTE:
*   uses std::is_signed and std::make_unsigned. re_std does NOT yet
* provide make_unsigned (it's not in the type_traits.hpp foundation
* that shipped). Documented as a localised, justified exception to
* the no-std-traits rule, same treatment as iterator_traits's
* tag-translation layer. To remove the std dependency, re_std would
* need to ship make_signed/make_unsigned (intrinsic-free, just a
* sequence of partial specs).
*
*
* path:      /inc/re_std/utility/intcmp.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.09
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_INTCMP_HPP
#define RE_STD_UTILITY_INTCMP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <limits>       // std::numeric_limits (for in_range)
    #include <type_traits>  // std::is_signed, std::make_unsigned -- documented localised exception


namespace re_std
{
namespace internal
{

    // ------------------------------------------------------------
    // primary template: same signedness -- direct comparison.
    // ------------------------------------------------------------
    template<typename T, typename U,
             bool TSigned = std::is_signed<T>::value,
             bool USigned = std::is_signed<U>::value>
    struct intcmp_eq
    {
        static RE_STD_CONSTEXPR bool apply(T _t, U _u) RE_STD_NOEXCEPT
        {
            return _t == _u;
        }
    };

    // signed-vs-unsigned: never equal if signed is negative.
    template<typename T, typename U>
    struct intcmp_eq<T, U, true, false>
    {
        static RE_STD_CONSTEXPR bool apply(T _t, U _u) RE_STD_NOEXCEPT
        {
            return _t >= 0
                && static_cast<typename std::make_unsigned<T>::type>(_t) == _u;
        }
    };

    // unsigned-vs-signed: mirror.
    template<typename T, typename U>
    struct intcmp_eq<T, U, false, true>
    {
        static RE_STD_CONSTEXPR bool apply(T _t, U _u) RE_STD_NOEXCEPT
        {
            return _u >= 0
                && _t == static_cast<typename std::make_unsigned<U>::type>(_u);
        }
    };

    // ------------------------------------------------------------
    // intcmp_lt: same shape, less-than.
    // ------------------------------------------------------------
    template<typename T, typename U,
             bool TSigned = std::is_signed<T>::value,
             bool USigned = std::is_signed<U>::value>
    struct intcmp_lt
    {
        static RE_STD_CONSTEXPR bool apply(T _t, U _u) RE_STD_NOEXCEPT
        {
            return _t < _u;
        }
    };

    // signed-vs-unsigned: signed is < unsigned iff signed is negative
    //   (any negative int < any unsigned), or its unsigned-cast is <.
    template<typename T, typename U>
    struct intcmp_lt<T, U, true, false>
    {
        static RE_STD_CONSTEXPR bool apply(T _t, U _u) RE_STD_NOEXCEPT
        {
            return _t < 0
                || static_cast<typename std::make_unsigned<T>::type>(_t) < _u;
        }
    };

    // unsigned-vs-signed: unsigned < signed only if signed > 0 AND
    //   unsigned < unsigned_cast_of_signed.
    template<typename T, typename U>
    struct intcmp_lt<T, U, false, true>
    {
        static RE_STD_CONSTEXPR bool apply(T _t, U _u) RE_STD_NOEXCEPT
        {
            return _u >= 0
                && _t < static_cast<typename std::make_unsigned<U>::type>(_u);
        }
    };

}  // internal
// =====================================================================
// Public cmp_* functions.
// =====================================================================

template<typename T, typename U>
RE_STD_CONSTEXPR bool cmp_equal(T _t, U _u) RE_STD_NOEXCEPT
{
    return internal::intcmp_eq<T, U>::apply(_t, _u);
}

template<typename T, typename U>
RE_STD_CONSTEXPR bool cmp_not_equal(T _t, U _u) RE_STD_NOEXCEPT
{
    return !re_std::cmp_equal(_t, _u);
}

template<typename T, typename U>
RE_STD_CONSTEXPR bool cmp_less(T _t, U _u) RE_STD_NOEXCEPT
{
    return internal::intcmp_lt<T, U>::apply(_t, _u);
}

template<typename T, typename U>
RE_STD_CONSTEXPR bool cmp_greater(T _t, U _u) RE_STD_NOEXCEPT
{
    return re_std::cmp_less(_u, _t);
}

template<typename T, typename U>
RE_STD_CONSTEXPR bool cmp_less_equal(T _t, U _u) RE_STD_NOEXCEPT
{
    return !re_std::cmp_less(_u, _t);
}

template<typename T, typename U>
RE_STD_CONSTEXPR bool cmp_greater_equal(T _t, U _u) RE_STD_NOEXCEPT
{
    return !re_std::cmp_less(_t, _u);
}


// =====================================================================
// in_range<R>(_v): whether _v fits in R's representable range.
// =====================================================================

template<typename R, typename T>
RE_STD_CONSTEXPR bool in_range(T _t) RE_STD_NOEXCEPT
{
    return re_std::cmp_greater_equal(_t, std::numeric_limits<R>::min())
        && re_std::cmp_less_equal(_t, std::numeric_limits<R>::max());
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_UTILITY_INTCMP_HPP
