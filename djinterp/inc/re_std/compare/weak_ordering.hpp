/*******************************************************************************
* djinterp [re_std]                                            weak_ordering.hpp
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
* path:      /inc/re_std/compare/weak_ordering.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_COMPARE_WEAK_ORDERING_HPP
#define RE_STD_COMPARE_WEAK_ORDERING_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./literal_zero_helper.hpp"
#include "./partial_ordering.hpp"


namespace re_std
{


// =============================================================================
// I.   WEAK_ORDERING
// =============================================================================

#if !RE_STD_LANG_IS_CPP17_OR_HIGHER

class weak_ordering;

namespace internal
{

    // weak_ordering_values
    //   struct: the values of weak_ordering -- less, equivalent and greater --
    // for C++11 and C++14, as static data members of a class template that
    // weak_ordering inherits from. weak_ordering's own static members need
    // out-of-class definitions, which a header can only give as inline
    // variables, a C++17 feature; a class template's static members may be
    // defined in a header at any tier. The cost is that below C++17 the values
    // are not usable in constant expressions.
    template<typename Unused = void>
    struct weak_ordering_values
    {
        static const weak_ordering less;
        static const weak_ordering equivalent;
        static const weak_ordering greater;
    };

}  // internal

#endif  // !RE_STD_LANG_IS_CPP17_OR_HIGHER

class weak_ordering
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
    : public internal::weak_ordering_values<>
#endif
{
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
    // the values are built with the private constructor
    template<typename>
    friend struct internal::weak_ordering_values;
#endif

private:
    typedef signed char _value_type;

    // m_value encoding:
    //   -1: less
    //    0: equivalent
    //   +1: greater
    _value_type m_value;


    // private ctor — instances are produced only via the three
    // static const members.
    RE_STD_CONSTEXPR
    explicit
    weak_ordering(
        _value_type _v
    ) RE_STD_NOEXCEPT
        : m_value(_v)
    {}


public:
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    static const weak_ordering less;
    static const weak_ordering equivalent;
    static const weak_ordering greater;
#endif  // below C++17 they are inherited; see weak_ordering_values


    // ---------------------------------------------------------------
    // Conversion to partial_ordering (the weaker category)
    // ---------------------------------------------------------------
    //   Per [cmp.weakord.conv]: weak_ordering converts to
    // partial_ordering, mapping less / equivalent / greater to
    // their same-named counterparts. Never yields unordered.

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
        weak_ordering              _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value == 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator!=(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value != 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator<(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value < 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator<=(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value <= 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator>(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value > 0);
    }

    friend RE_STD_CONSTEXPR bool
    operator>=(
        weak_ordering              _v,
        internal::literal_zero_helper
    ) RE_STD_NOEXCEPT
    {
        return (_v.m_value >= 0);
    }

    // Reversed-direction overloads.

    friend RE_STD_CONSTEXPR bool
    operator==(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) RE_STD_NOEXCEPT
    {
        return (0 == _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator!=(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) RE_STD_NOEXCEPT
    {
        return (0 != _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator<(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) RE_STD_NOEXCEPT
    {
        return (0 < _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator<=(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) RE_STD_NOEXCEPT
    {
        return (0 <= _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator>(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) RE_STD_NOEXCEPT
    {
        return (0 > _v.m_value);
    }

    friend RE_STD_CONSTEXPR bool
    operator>=(
        internal::literal_zero_helper,
        weak_ordering              _v
    ) RE_STD_NOEXCEPT
    {
        return (0 >= _v.m_value);
    }
};


// the values: inline constexpr from C++17, so usable in constant
// expressions; below it, static members of weak_ordering_values, which a
// header may define at any tier
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const weak_ordering
weak_ordering::less = weak_ordering(-1);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const weak_ordering
weak_ordering::equivalent = weak_ordering(0);
RE_STD_INLINE_VAR RE_STD_CONSTEXPR const weak_ordering
weak_ordering::greater = weak_ordering(1);
#else
template<typename Unused>
const weak_ordering
internal::weak_ordering_values<Unused>::less = weak_ordering(-1);
template<typename Unused>
const weak_ordering
internal::weak_ordering_values<Unused>::equivalent = weak_ordering(0);
template<typename Unused>
const weak_ordering
internal::weak_ordering_values<Unused>::greater = weak_ordering(1);
#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_COMPARE_WEAK_ORDERING_HPP
