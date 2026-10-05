/*******************************************************************************
* djinterp [re_std]                                          strong_ordering.hpp
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
* path:      /inc/re_std/compare/strong_ordering.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_COMPARE_STRONG_ORDERING_HPP
#define RE_STD_COMPARE_STRONG_ORDERING_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./literal_zero_helper.hpp"
#include "./partial_ordering.hpp"
#include "./weak_ordering.hpp"


namespace re_std
{


// =============================================================================
// I.   STRONG_ORDERING
// =============================================================================

#if !RE_STD_LANG_IS_CPP17_OR_HIGHER

class strong_ordering;

namespace internal
{

    // strong_ordering_values
    //   struct: the values of strong_ordering -- less, equal, equivalent and
    // greater -- for C++11 and C++14, as static data members of a class
    // template that strong_ordering inherits from. strong_ordering's own static
    // members need out-of-class definitions, which a header can only give as
    // inline variables, a C++17 feature; a class template's static members may
    // be defined in a header at any tier. The cost is that below C++17 the
    // values are not usable in constant expressions.
    template<typename Unused = void>
    struct strong_ordering_values
    {
        static const strong_ordering less;
        static const strong_ordering equal;
        static const strong_ordering equivalent;
        static const strong_ordering greater;
    };

}  // internal

#endif  // !RE_STD_LANG_IS_CPP17_OR_HIGHER

class strong_ordering
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
    : public internal::strong_ordering_values<>
#endif
{
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
    // the values are built with the private constructor
    template<typename>
    friend struct internal::strong_ordering_values;
#endif

private:
    typedef signed char _value_type;

    // m_value encoding:
    //   -1: less
    //    0: equal / equivalent (same outcome for strong ordering)
    //   +1: greater
    _value_type m_value;


    RE_STD_CONSTEXPR
    explicit
    strong_ordering(
        _value_type _v
    ) RE_STD_NOEXCEPT
        : m_value(_v)
    {}


public:
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    static const strong_ordering less;
    static const strong_ordering equal;
    static const strong_ordering equivalent;
    static const strong_ordering greater;
#endif  // below C++17 they are inherited; see strong_ordering_values


    // ---------------------------------------------------------------
    // Conversions to weaker categories
    // ---------------------------------------------------------------
    //   Per [cmp.strongord.conv]: strong_ordering converts to both
    // weak_ordering and partial_ordering, preserving the ordering
    // state. The conversion is implicit.

    RE_STD_CONSTEXPR
    operator weak_ordering() const
    RE_STD_NOEXCEPT
    {
        return (m_value < 0)  ? weak_ordering::less
             : (m_value == 0) ? weak_ordering::equivalent
                              : weak_ordering::greater;
    }

    RE_STD_CONSTEXPR
    operator partial_ordering() const
    RE_STD_NOEXCEPT
    {
        return (m_value < 0)  ? partial_ordering::less
             : (m_value == 0) ? partial_ordering::equivalent
                              : partial_ordering::greater;
    }


    // ---------------------------------------------------------------
    // Comparison vs literal 0
    // ---------------------------------------------------------------

    friend RE_STD_CONSTEXPR bool
    operator==(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value == 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator!=(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value != 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator<(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value < 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator<=(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value <= 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator>(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value > 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator>=(
        strong_ordering            _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value >= 0);
    }

    // Reversed-direction overloads.

    friend RE_STD_CONSTEXPR bool
    operator==(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return (0 == _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator!=(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return (0 != _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator<(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return (0 < _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator<=(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return (0 <= _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator>(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return (0 > _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator>=(
        internal::literal_zero_helper,
        strong_ordering            _v
    ) RE_STD_NOEXCEPT
    {
        return (0 >= _v.m_value);
    }
};


// the values: inline constexpr from C++17, so usable in constant
// expressions; below it, static members of strong_ordering_values, which a
// header may define at any tier
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const strong_ordering
strong_ordering::less = strong_ordering(-1);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const strong_ordering
strong_ordering::equal = strong_ordering(0);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const strong_ordering
strong_ordering::equivalent = strong_ordering(0);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const strong_ordering
strong_ordering::greater = strong_ordering(1);
#else
template<typename Unused>
const strong_ordering
internal::strong_ordering_values<Unused>::less = strong_ordering(-1);
template<typename Unused>
const strong_ordering
internal::strong_ordering_values<Unused>::equal = strong_ordering(0);
template<typename Unused>
const strong_ordering
internal::strong_ordering_values<Unused>::equivalent = strong_ordering(0);
template<typename Unused>
const strong_ordering
internal::strong_ordering_values<Unused>::greater = strong_ordering(1);
#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_COMPARE_STRONG_ORDERING_HPP
