/*******************************************************************************
* djinterp [re_std]                                       optional_three_way.hpp
*
* optional_three_way support header:
*   operator<=> for optional<T>, in all three overload families.
*
*   THE DISENGAGED ORDERING IS THE SAME RULE THE SIX RELATIONAL OPERATORS USE,
* stated once in one place: a disengaged optional is EQUAL to nullopt and LESS
* than every engaged optional and every bare value. Expressing it as
* `x.has_value() <=> y.has_value()` when engagement differs is not a
* shortcut - false < true is exactly the required ordering, and routing it
* through bool's own <=> keeps the two definitions from drifting apart.
*
*   THE nullopt OVERLOAD RETURNS strong_ordering UNCONDITIONALLY, even when T
* has no ordering at all. That is correct and deliberate: comparing against
* nullopt only ever inspects engagement, never a value, so T's comparison
* category is irrelevant. optional<T>{} <=> nullopt is well-formed for a T that
* is not three-way comparable.
*
*   STD IS C++20; re_std IS C++20 - hard ceiling, operator<=> is a core
* language feature.
*
*
* path:      /inc/re_std/optional/optional_three_way.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_OPTIONAL_OPTIONAL_THREE_WAY_HPP
#define RE_STD_OPTIONAL_OPTIONAL_THREE_WAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../compare/compare"
#include "./optional.hpp"
#include "./nullopt.hpp"

namespace re_std
{

// operator<=>
//   function: optional vs optional.
template<typename Type, typename Other>
RE_STD_CONSTEXPR typename compare_three_way_result<Type, Other>::type
operator<=>(const optional<Type>& a, const optional<Other>& b)
{
    return (a.has_value() && b.has_value())
               ? (*a <=> *b)
               : static_cast<
                     typename compare_three_way_result<Type, Other>::type>(
                         a.has_value() <=> b.has_value());
}

// operator<=>
//   function: optional vs nullopt.  Always strong_ordering - only engagement
// is inspected, so Type need not be comparable at all.
template<typename Type>
RE_STD_CONSTEXPR strong_ordering operator<=>(const optional<Type>& a,
                                        nullopt_t) RE_STD_NOEXCEPT
{
    return a.has_value() <=> false;
}

// operator<=>
//   function: optional vs value.  A disengaged optional is below every value.
template<typename Type, typename Other>
RE_STD_CONSTEXPR typename compare_three_way_result<Type, Other>::type
operator<=>(const optional<Type>& a, const Other& b)
{
    return a.has_value()
               ? (*a <=> b)
               : static_cast<
                     typename compare_three_way_result<Type, Other>::type>(
                         strong_ordering::less);
}

}

#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_OPTIONAL_OPTIONAL_THREE_WAY_HPP
