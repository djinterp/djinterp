/******************************************************************************
* re_std [optional]                                           optional_less.hpp
*
*   operator< for optional<T>, in all three overload families.
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
* path:      /inc/djinterp/re_std/optional/optional_less.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_OPTIONAL_LESS_
#define RESTD_OPTIONAL_LESS_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./optional.hpp"
#include "./nullopt.hpp"

NS_DJINTERP
NS_RESTD

// operator<
//   function: optional vs optional.
template<typename _Type>
D_CONSTEXPR bool operator<(const optional<_Type>& a, const optional<_Type>& b)
{
    return (a.has_value() != b.has_value())
               ? (!a.has_value())
               : (a.has_value() ? (*a < *b) : (false));
}

// operator<
//   function: optional vs nullopt_t.  A disengaged optional is equal to
// nullopt and less than everything else.
template<typename _Type>
D_CONSTEXPR bool operator<(const optional<_Type>& , nullopt_t) D_NOEXCEPT
{
    return false;
}

template<typename _Type>
D_CONSTEXPR bool operator<(nullopt_t, const optional<_Type>& b) D_NOEXCEPT
{
    return b.has_value();
}

// operator<
//   function: optional vs value.  A disengaged optional compares as if it
// were below every value.
template<typename _Type, typename _Other>
D_CONSTEXPR bool operator<(const optional<_Type>& a, const _Other& b)
{
    return a.has_value() ? (*a < b) : (true);
}

template<typename _Type, typename _Other>
D_CONSTEXPR bool operator<(const _Other& a, const optional<_Type>& b)
{
    return b.has_value() ? (a < *b) : (false);
}

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_OPTIONAL_LESS_
