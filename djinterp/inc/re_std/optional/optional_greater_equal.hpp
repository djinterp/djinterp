/*******************************************************************************
* djinterp [re_std]                                   optional_greater_equal.hpp
*
* optional_greater_equal support header:
*   operator>= for optional<T>, in all three overload families.
*
*   Three overload families, as std specifies: optional vs optional, optional vs
* nullopt_t, and optional vs a bare value.  All three are needed - comparing an
* optional against a plain value is the common case, and without that family it
* would go through an implicit conversion to optional and allocate a temporary.
*
*   THE DISENGAGED ORDERING IS NOT ARBITRARY.
* A disengaged optional compares LESS than any engaged one, and two disengaged
* optionals compare equal.  That makes the ordering a total order consistent
* with nullopt being a value below all others, which is what lets optional<T> be
* used as a map key or sorted without surprises.
*
*
* path:      /inc/re_std/optional/optional_greater_equal.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_OPTIONAL_OPTIONAL_GREATER_EQUAL_HPP
#define RE_STD_OPTIONAL_OPTIONAL_GREATER_EQUAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./optional.hpp"
#include "./nullopt.hpp"

namespace re_std
{

// operator>=
//   function: optional vs optional.
template<typename Type>
RE_STD_CONSTEXPR bool operator>=(const optional<Type>& a, const optional<Type>& b)
{
    return (a.has_value() != b.has_value())
               ? (a.has_value())
               : (a.has_value() ? (*a >= *b) : (true));
}

// operator>=
//   function: optional vs nullopt_t.  A disengaged optional is equal to
// nullopt and less than everything else.
template<typename Type>
RE_STD_CONSTEXPR bool operator>=(const optional<Type>& , nullopt_t) RE_STD_NOEXCEPT
{
    return true;
}

template<typename Type>
RE_STD_CONSTEXPR bool operator>=(nullopt_t, const optional<Type>& b) RE_STD_NOEXCEPT
{
    return !b.has_value();
}

// operator>=
//   function: optional vs value.  A disengaged optional compares as if it
// were below every value.
template<typename Type, typename Other>
RE_STD_CONSTEXPR bool operator>=(const optional<Type>& a, const Other& b)
{
    return a.has_value() ? (*a >= b) : (false);
}

template<typename Type, typename Other>
RE_STD_CONSTEXPR bool operator>=(const Other& a, const optional<Type>& b)
{
    return b.has_value() ? (a >= *b) : (true);
}

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_OPTIONAL_OPTIONAL_GREATER_EQUAL_HPP
