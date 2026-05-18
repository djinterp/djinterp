/******************************************************************************
* djinterp [restd]                                           strong_ordering.hpp
*
* strong_ordering class header:
*   The strongest C++20 comparison category. Represents a total
* ordering where equivalent values are also substitutable in every
* observable way (i.e. there is no observable distinction between
* equivalent and equal). Built-in integer comparison returns this
* category; std::string operator<=> returns this category.
*
*     strong_ordering::less
*     strong_ordering::equal        (same value as equivalent below)
*     strong_ordering::equivalent
*     strong_ordering::greater
*
*   The `equal` and `equivalent` instances compare bitwise-equal —
* strong ordering treats them as the same outcome (when an ordering
* is strong, equivalence implies equality).
*
*   IMPLICIT CONVERSIONS:
*   strong_ordering converts to weak_ordering and to partial_ordering
* (both weaker categories), mapping less / equal / greater to the
* corresponding states in the target category.
*
*   IMPLEMENTATION:
*   Stores a single signed-char value:
*     -1 : less
*      0 : equal / equivalent
*     +1 : greater
*
*   PORTABILITY:
*   C++11+ (constexpr ctors and static members). Conversions to
* weak_ordering and partial_ordering require their complete types,
* which this header includes.
*
*
* path:      /inc/djinterp/restd/compare/strong_ordering.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_STRONG_ORDERING_
#define DJINTERP_RESTD_COMPARE_STRONG_ORDERING_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./literal_zero_helper.hpp"
#include "./partial_ordering.hpp"
#include "./weak_ordering.hpp"


NS_RESTD


// =============================================================================
// I.   STRONG_ORDERING
// =============================================================================

class strong_ordering
{
private:
    typedef signed char _value_type;

    // m_value encoding:
    //   -1: less
    //    0: equal / equivalent (same outcome for strong ordering)
    //   +1: greater
    _value_type m_value;


    D_CONSTEXPR
    explicit
    strong_ordering(
        _value_type _v
    ) D_NOEXCEPT
        : m_value(_v)
    {}


public:
    D_STATIC const strong_ordering less;
    D_STATIC const strong_ordering equal;
    D_STATIC const strong_ordering equivalent;
    D_STATIC const strong_ordering greater;


    // ---------------------------------------------------------------
    // Conversions to weaker categories
    // ---------------------------------------------------------------
    //   Per [cmp.strongord.conv]: strong_ordering converts to both
    // weak_ordering and partial_ordering, preserving the ordering
    // state. The conversion is implicit.

    D_CONSTEXPR
    operator weak_ordering() const
    D_NOEXCEPT
    {
        return (m_value < 0)  ? weak_ordering::less
             : (m_value == 0) ? weak_ordering::equivalent
                              : weak_ordering::greater;
    }

    D_CONSTEXPR
    operator partial_ordering() const
    D_NOEXCEPT
    {
        return (m_value < 0)  ? partial_ordering::less
             : (m_value == 0) ? partial_ordering::equivalent
                              : partial_ordering::greater;
    }


    // ---------------------------------------------------------------
    // Comparison vs literal 0
    // ---------------------------------------------------------------

    friend D_CONSTEXPR bool
    operator==(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value == 0);
    }

    friend D_CONSTEXPR bool
    operator!=(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value != 0);
    }

    friend D_CONSTEXPR bool
    operator<(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value < 0);
    }

    friend D_CONSTEXPR bool
    operator<=(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value <= 0);
    }

    friend D_CONSTEXPR bool
    operator>(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value > 0);
    }

    friend D_CONSTEXPR bool
    operator>=(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value >= 0);
    }

    // Reversed-direction overloads.

    friend D_CONSTEXPR bool
    operator==(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) D_NOEXCEPT
    {
        return (0 == _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator!=(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) D_NOEXCEPT
    {
        return (0 != _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator<(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) D_NOEXCEPT
    {
        return (0 < _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator<=(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) D_NOEXCEPT
    {
        return (0 <= _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator>(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) D_NOEXCEPT
    {
        return (0 > _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator>=(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) D_NOEXCEPT
    {
        return (0 >= _v.m_value);
    }
};


// Out-of-class static member definitions. equal and equivalent share
// the same value-encoding (0) per [cmp.strongord]/3.
D_CONSTEXPR_INLINE const strong_ordering strong_ordering::less       = strong_ordering(-1);
D_CONSTEXPR_INLINE const strong_ordering strong_ordering::equal      = strong_ordering( 0);
D_CONSTEXPR_INLINE const strong_ordering strong_ordering::equivalent = strong_ordering( 0);
D_CONSTEXPR_INLINE const strong_ordering strong_ordering::greater    = strong_ordering( 1);


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_STRONG_ORDERING_
