/*******************************************************************************
* djinterp [re_std]                                         partial_ordering.hpp
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
* path:      /inc/re_std/compare/partial_ordering.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_COMPARE_PARTIAL_ORDERING_HPP
#define RE_STD_COMPARE_PARTIAL_ORDERING_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./literal_zero_helper.hpp"


namespace re_std
{


// =============================================================================
// I.   PARTIAL_ORDERING
// =============================================================================

#if !RE_STD_LANG_IS_CPP17_OR_HIGHER

class partial_ordering;

namespace internal
{

    // partial_ordering_values
    //   struct: the values of partial_ordering -- less, equivalent, greater and
    // unordered -- for C++11 and C++14, as static data members of a class
    // template that partial_ordering inherits from. partial_ordering's own
    // static members need out-of-class definitions, which a header can only
    // give as inline variables, a C++17 feature; a class template's static
    // members may be defined in a header at any tier. The cost is that below
    // C++17 the values are not usable in constant expressions.
    template<typename Unused = void>
    struct partial_ordering_values
    {
        static const partial_ordering less;
        static const partial_ordering equivalent;
        static const partial_ordering greater;
        static const partial_ordering unordered;
    };

}  // internal

#endif  // !RE_STD_LANG_IS_CPP17_OR_HIGHER

class partial_ordering
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
    : public internal::partial_ordering_values<>
#endif
{
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
    // the values are built with the private constructor
    template<typename>
    friend struct internal::partial_ordering_values;
#endif

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
    RE_STD_CONSTEXPR
    explicit
    partial_ordering(
        _value_type _v
    ) RE_STD_NOEXCEPT
        : m_value(_v)
    {}


    // _is_ordered: true when m_value encodes less / equivalent /
    // greater (i.e. one of {-1, 0, +1}), false for unordered.
    RE_STD_CONSTEXPR
    bool
    _is_ordered() const
    RE_STD_NOEXCEPT
    {
        return (m_value != 2);
    }


public:
    // the values: static members, defined inline constexpr below the class
    // from C++17; inherited from internal::partial_ordering_values below it
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    static const partial_ordering less;
    static const partial_ordering equivalent;
    static const partial_ordering greater;
    static const partial_ordering unordered;
#endif  // below C++17 they are inherited; see partial_ordering_values


    // ---------------------------------------------------------------
    // Comparison vs literal 0
    // ---------------------------------------------------------------
    // Each operator is provided in both directions (cmp OP 0 and
    // 0 OP cmp) per [cmp.partialord] and [cmp.alg] requirements.

    friend RE_STD_CONSTEXPR bool
    operator==(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value == 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator!=(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        // !(v == 0): true if v is less / greater / unordered.
        return !(_v._is_ordered() && (_v.m_value == 0));
    }

    friend RE_STD_CONSTEXPR bool
    operator<(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value < 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator<=(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value <= 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator>(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value > 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator>=(
        partial_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (_v.m_value >= 0);
    }

    // Reversed-direction overloads.

    friend RE_STD_CONSTEXPR bool
    operator==(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (0 == _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator!=(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return !(_v._is_ordered() && (0 == _v.m_value));
    }

    friend RE_STD_CONSTEXPR bool
    operator<(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        // 0 < v  iff  v > 0  iff  v.m_value > 0
        return _v._is_ordered() && (0 < _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator<=(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (0 <= _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator>(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (0 > _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator>=(
        internal::literal_zero_helper,
        partial_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return _v._is_ordered() && (0 >= _v.m_value);
    }
};


// the values: inline constexpr from C++17, so usable in constant
// expressions; below it, static members of partial_ordering_values, which a
// header may define at any tier
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const partial_ordering
partial_ordering::less = partial_ordering(-1);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const partial_ordering
partial_ordering::equivalent = partial_ordering(0);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const partial_ordering
partial_ordering::greater = partial_ordering(1);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const partial_ordering
partial_ordering::unordered = partial_ordering(2);
#else
template<typename Unused>
const partial_ordering
internal::partial_ordering_values<Unused>::less = partial_ordering(-1);
template<typename Unused>
const partial_ordering
internal::partial_ordering_values<Unused>::equivalent = partial_ordering(0);
template<typename Unused>
const partial_ordering
internal::partial_ordering_values<Unused>::greater = partial_ordering(1);
template<typename Unused>
const partial_ordering
internal::partial_ordering_values<Unused>::unordered = partial_ordering(2);
#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_COMPARE_PARTIAL_ORDERING_HPP
