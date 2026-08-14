/******************************************************************************
* re_std [optional]                                      optional_three_way.hpp
*
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
* path:      /inc/djinterp/re_std/optional/optional_three_way.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_OPTIONAL_THREE_WAY_
#define RESTD_OPTIONAL_THREE_WAY_ 1

#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../compare/compare.hpp"
#include "./optional.hpp"
#include "./nullopt.hpp"

NS_DJINTERP
NS_RESTD

// operator<=>
//   function: optional vs optional.
template<typename _Type, typename _Other>
D_CONSTEXPR typename compare_three_way_result<_Type, _Other>::type
operator<=>(const optional<_Type>& a, const optional<_Other>& b)
{
    return (a.has_value() && b.has_value())
               ? (*a <=> *b)
               : static_cast<
                     typename compare_three_way_result<_Type, _Other>::type>(
                         a.has_value() <=> b.has_value());
}

// operator<=>
//   function: optional vs nullopt.  Always strong_ordering - only engagement
// is inspected, so _Type need not be comparable at all.
template<typename _Type>
D_CONSTEXPR strong_ordering operator<=>(const optional<_Type>& a,
                                        nullopt_t) D_NOEXCEPT
{
    return a.has_value() <=> false;
}

// operator<=>
//   function: optional vs value.  A disengaged optional is below every value.
template<typename _Type, typename _Other>
D_CONSTEXPR typename compare_three_way_result<_Type, _Other>::type
operator<=>(const optional<_Type>& a, const _Other& b)
{
    return a.has_value()
               ? (*a <=> b)
               : static_cast<
                     typename compare_three_way_result<_Type, _Other>::type>(
                         strong_ordering::less);
}

NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_OPTIONAL_THREE_WAY_
