/***********************************************************************
* restd                                          intcmp.hpp -- cmp_* family
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
*   uses std::is_signed and std::make_unsigned. restd does NOT yet
* provide make_unsigned (it's not in the type_traits.hpp foundation
* that shipped). Documented as a localised, justified exception to
* the no-std-traits rule, same treatment as iterator_traits's
* tag-translation layer. To remove the std dependency, restd would
* need to ship make_signed/make_unsigned (intrinsic-free, just a
* sequence of partial specs).
*
*
* path:      /inc/restd/utility/intcmp.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_INTCMP_
#define RESTD_UTILITY_INTCMP_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <type_traits>  // std::is_signed, std::make_unsigned -- documented localised exception
    #include <limits>       // std::numeric_limits (for in_range)


namespace restd
{
namespace internal
{

    // ------------------------------------------------------------
    // primary template: same signedness -- direct comparison.
    // ------------------------------------------------------------
    template<typename _T, typename _U,
             bool _TSigned = std::is_signed<_T>::value,
             bool _USigned = std::is_signed<_U>::value>
    struct intcmp_eq
    {
        static D_CONSTEXPR bool apply(_T _t, _U _u) D_NOEXCEPT
        {
            return _t == _u;
        }
    };

    // signed-vs-unsigned: never equal if signed is negative.
    template<typename _T, typename _U>
    struct intcmp_eq<_T, _U, true, false>
    {
        static D_CONSTEXPR bool apply(_T _t, _U _u) D_NOEXCEPT
        {
            return _t >= 0
                && static_cast<typename std::make_unsigned<_T>::type>(_t) == _u;
        }
    };

    // unsigned-vs-signed: mirror.
    template<typename _T, typename _U>
    struct intcmp_eq<_T, _U, false, true>
    {
        static D_CONSTEXPR bool apply(_T _t, _U _u) D_NOEXCEPT
        {
            return _u >= 0
                && _t == static_cast<typename std::make_unsigned<_U>::type>(_u);
        }
    };

    // ------------------------------------------------------------
    // intcmp_lt: same shape, less-than.
    // ------------------------------------------------------------
    template<typename _T, typename _U,
             bool _TSigned = std::is_signed<_T>::value,
             bool _USigned = std::is_signed<_U>::value>
    struct intcmp_lt
    {
        static D_CONSTEXPR bool apply(_T _t, _U _u) D_NOEXCEPT
        {
            return _t < _u;
        }
    };

    // signed-vs-unsigned: signed is < unsigned iff signed is negative
    //   (any negative int < any unsigned), or its unsigned-cast is <.
    template<typename _T, typename _U>
    struct intcmp_lt<_T, _U, true, false>
    {
        static D_CONSTEXPR bool apply(_T _t, _U _u) D_NOEXCEPT
        {
            return _t < 0
                || static_cast<typename std::make_unsigned<_T>::type>(_t) < _u;
        }
    };

    // unsigned-vs-signed: unsigned < signed only if signed > 0 AND
    //   unsigned < unsigned_cast_of_signed.
    template<typename _T, typename _U>
    struct intcmp_lt<_T, _U, false, true>
    {
        static D_CONSTEXPR bool apply(_T _t, _U _u) D_NOEXCEPT
        {
            return _u >= 0
                && _t < static_cast<typename std::make_unsigned<_U>::type>(_u);
        }
    };

}  // namespace internal


// =====================================================================
// Public cmp_* functions.
// =====================================================================

template<typename _T, typename _U>
D_CONSTEXPR bool cmp_equal(_T _t, _U _u) D_NOEXCEPT
{
    return internal::intcmp_eq<_T, _U>::apply(_t, _u);
}

template<typename _T, typename _U>
D_CONSTEXPR bool cmp_not_equal(_T _t, _U _u) D_NOEXCEPT
{
    return !restd::cmp_equal(_t, _u);
}

template<typename _T, typename _U>
D_CONSTEXPR bool cmp_less(_T _t, _U _u) D_NOEXCEPT
{
    return internal::intcmp_lt<_T, _U>::apply(_t, _u);
}

template<typename _T, typename _U>
D_CONSTEXPR bool cmp_greater(_T _t, _U _u) D_NOEXCEPT
{
    return restd::cmp_less(_u, _t);
}

template<typename _T, typename _U>
D_CONSTEXPR bool cmp_less_equal(_T _t, _U _u) D_NOEXCEPT
{
    return !restd::cmp_less(_u, _t);
}

template<typename _T, typename _U>
D_CONSTEXPR bool cmp_greater_equal(_T _t, _U _u) D_NOEXCEPT
{
    return !restd::cmp_less(_t, _u);
}


// =====================================================================
// in_range<R>(_v): whether _v fits in R's representable range.
// =====================================================================

template<typename _R, typename _T>
D_CONSTEXPR bool in_range(_T _t) D_NOEXCEPT
{
    return restd::cmp_greater_equal(_t, std::numeric_limits<_R>::min())
        && restd::cmp_less_equal(_t, std::numeric_limits<_R>::max());
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_INTCMP_
