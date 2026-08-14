/******************************************************************************
* djinterp [restd]                                          partial_ordering.hpp
*
* partial_ordering class header:
*   The weakest of the three C++20 comparison categories. Represents
* the result of a three-way comparison where some pairs of values
* may be incomparable (the `unordered` state). Floating-point
* operator<=> returns this category because NaN is unordered with
* respect to every value including itself.
*
*     partial_ordering::less
*     partial_ordering::equivalent
*     partial_ordering::greater
*     partial_ordering::unordered
*
*   COMPARISON-VS-LITERAL-0:
*   The C++20 idiom for inspecting an ordering value is comparison
* against the literal 0:
*     (cmp == 0)  // ordering is "equivalent"
*     (cmp <  0)  // ordering is "less"
*     (cmp >  0)  // ordering is "greater"
*     (cmp != 0)  // ordering is "less", "greater", or "unordered"
* For partial_ordering specifically, ALL four relational operators
* against 0 return false when the value is `unordered`.
*
*   IMPLEMENTATION:
*   Stores a single signed-char value:
*     -1 : less
*      0 : equivalent
*     +1 : greater
*     +2 : unordered  (encoded specially; both < 0 and > 0 yield false)
*
*   PORTABILITY:
*   C++11+ (constexpr ctors and static members).
*
*
* path:      /inc/djinterp/re_std/compare/partial_ordering.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_PARTIAL_ORDERING_
#define DJINTERP_RESTD_COMPARE_PARTIAL_ORDERING_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./literal_zero_helper.hpp"


NS_RESTD


// =============================================================================
// I.   PARTIAL_ORDERING
// =============================================================================

class partial_ordering
{
private:
    typedef signed char _value_type;

    // m_value encoding:
    //   -1: less
    //    0: equivalent
    //   +1: greater
    //   +2: unordered (sentinel)
    _value_type m_value;


    // private ctor — instances are produced only via the four
    // static const members.
    D_CONSTEXPR
    explicit
    partial_ordering(
        _value_type _v
    ) D_NOEXCEPT
        : m_value(_v)
    {}


    // _is_ordered: true when m_value encodes less / equivalent /
    // greater (i.e. one of {-1, 0, +1}), false for unordered.
    D_CONSTEXPR
    bool
    _is_ordered() const
    D_NOEXCEPT
    {
        return (m_value != 2);
    }


public:
    // Static instances. Definitions live below the class (out-of-
    // class for C++11/14 compatibility; an inline-constexpr in-class
    // initialiser would require C++17).
    D_STATIC const partial_ordering less;
    D_STATIC const partial_ordering equivalent;
    D_STATIC const partial_ordering greater;
    D_STATIC const partial_ordering unordered;


    // ---------------------------------------------------------------
    // Comparison vs literal 0
    // ---------------------------------------------------------------
    // Each operator is provided in both directions (cmp OP 0 and
    // 0 OP cmp) per [cmp.partialord] and [cmp.alg] requirements.

    friend D_CONSTEXPR bool
    operator==(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value == 0);
    }

    friend D_CONSTEXPR bool
    operator!=(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        // !(v == 0): true if v is less / greater / unordered.
        return !(_v._is_ordered() && (_v.m_value == 0));
    }

    friend D_CONSTEXPR bool
    operator<(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value < 0);
    }

    friend D_CONSTEXPR bool
    operator<=(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value <= 0);
    }

    friend D_CONSTEXPR bool
    operator>(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value > 0);
    }

    friend D_CONSTEXPR bool
    operator>=(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value >= 0);
    }

    // Reversed-direction overloads.

    friend D_CONSTEXPR bool
    operator==(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (0 == _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator!=(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) D_NOEXCEPT
    {
        return !(_v._is_ordered() && (0 == _v.m_value));
    }

    friend D_CONSTEXPR bool
    operator<(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) D_NOEXCEPT
    {
        // 0 < v  iff  v > 0  iff  v.m_value > 0
        return _v._is_ordered() && (0 < _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator<=(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (0 <= _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator>(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (0 > _v.m_value);
    }

    friend D_CONSTEXPR bool
    operator>=(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) D_NOEXCEPT
    {
        return _v._is_ordered() && (0 >= _v.m_value);
    }
};


// Out-of-class static member definitions (required for C++11/14;
// could be in-class inline on C++17+).
D_CONSTEXPR_INLINE const partial_ordering partial_ordering::less       = partial_ordering(-1);
D_CONSTEXPR_INLINE const partial_ordering partial_ordering::equivalent = partial_ordering( 0);
D_CONSTEXPR_INLINE const partial_ordering partial_ordering::greater    = partial_ordering( 1);
D_CONSTEXPR_INLINE const partial_ordering partial_ordering::unordered  = partial_ordering( 2);


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_PARTIAL_ORDERING_
