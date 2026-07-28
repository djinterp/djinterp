/******************************************************************************
* djinterp [restd]                                             weak_ordering.hpp
*
* weak_ordering class header:
*   The middle C++20 comparison category. Represents a total ordering
* where distinct elements may compare equivalent without being equal
* (e.g. case-insensitive string compare: "Foo" and "foo" are
* equivalent but not identical).
*
*     weak_ordering::less
*     weak_ordering::equivalent
*     weak_ordering::greater
*
*   No `unordered` state, so all relational operators vs 0 produce
* a real boolean result (in contrast to partial_ordering).
*
*   IMPLICIT CONVERSION:
*   weak_ordering converts implicitly to partial_ordering (the
* weaker category), mapping less → less, equivalent → equivalent,
* greater → greater. Never produces partial_ordering::unordered.
*
*   IMPLEMENTATION:
*   Stores a single signed-char value:
*     -1 : less
*      0 : equivalent
*     +1 : greater
*
*   PORTABILITY:
*   C++11+ (constexpr ctors and static members). The conversion to
* partial_ordering requires partial_ordering's complete type, which
* this header includes.
*
*
* path:      /inc/djinterp/restd/compare/weak_ordering.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_WEAK_ORDERING_
#define DJINTERP_RESTD_COMPARE_WEAK_ORDERING_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./literal_zero_helper.hpp"
#include "./partial_ordering.hpp"


NS_RESTD


// =============================================================================
// I.   WEAK_ORDERING
// =============================================================================

class weak_ordering
{
private:
    typedef signed char _value_type;

    // m_value encoding:
    //   -1: less
    //    0: equivalent
    //   +1: greater
    _value_type m_value;


    // private ctor — instances are produced only via the three
    // static const members.
    D_CONSTEXPR
    explicit
    weak_ordering(
        _value_type _v
    ) D_NOEXCEPT
        : m_value(_v)
    {}


public:
    D_STATIC const weak_ordering less;
    D_STATIC const weak_ordering equivalent;
    D_STATIC const weak_ordering greater;


    // ---------------------------------------------------------------
    // Conversion to partial_ordering (the weaker category)
    // ---------------------------------------------------------------
    //   Per [cmp.weakord.conv]: weak_ordering converts to
    // partial_ordering, mapping less / equivalent / greater to
    // their same-named counterparts. Never yields unordered.

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
        weak_ordering              _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value == 0);
    }

    friend D_CONSTEXPR bool
    operator!=(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value != 0);
    }

    friend D_CONSTEXPR bool
    operator<(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value < 0);
    }

    friend D_CONSTEXPR bool
    operator<=(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value <= 0);
    }

    friend D_CONSTEXPR bool
    operator>(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value > 0);
    }

    friend D_CONSTEXPR bool
    operator>=(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return (_v.m_value >= 0);
    }

    // Reversed-direction overloads.

    friend D_CONSTEXPR bool
    operator==(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) D_NOEXCEPT
    {
        return (0 == _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator!=(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) D_NOEXCEPT
    {
        return (0 != _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator<(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) D_NOEXCEPT
    {
        return (0 < _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator<=(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) D_NOEXCEPT
    {
        return (0 <= _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator>(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) D_NOEXCEPT
    {
        return (0 > _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator>=(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) D_NOEXCEPT
    {
        return (0 >= _v.m_value);
    }
};


// Out-of-class static member definitions.
D_CONSTEXPR_INLINE const weak_ordering weak_ordering::less       = weak_ordering(-1);
D_CONSTEXPR_INLINE const weak_ordering weak_ordering::equivalent = weak_ordering( 0);
D_CONSTEXPR_INLINE const weak_ordering weak_ordering::greater    = weak_ordering( 1);


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_WEAK_ORDERING_
