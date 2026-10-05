/*******************************************************************************
* djinterp [re_std]                                       unexpected_compare.hpp
*
* unexpected comparison header:
*   Provides operator== for two unexpected<E1> / unexpected<E2> wrappers.
* C++20 standard synthesises operator!= from this; re_std ships it
* explicitly on every tier so C++11–C++17 code can rely on it.
*
*   ELEMENT-TYPE REQUIREMENT:
*   E1 and E2 must be equality-comparable; the comparison defers
* directly to their operator==. Heterogeneous error types compare via
* whatever cross-type op== they expose.
*
*
* path:      /inc/re_std/expected/unexpected_compare.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXPECTED_UNEXPECTED_COMPARE_HPP
#define RE_STD_EXPECTED_UNEXPECTED_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "./unexpected.hpp"


namespace re_std
{


// ===========================================================================
// I.   OPERATOR==
// ===========================================================================

// operator==
//   function: two unexpected<E> values compare equal iff their stored
// errors compare equal.
template<typename E1,
         typename E2>
RE_STD_CONSTEXPR_CPP20 bool
operator==(
    unexpected<E1> const& _lhs,
    unexpected<E2> const& _rhs
)
{
    return _lhs.error() == _rhs.error();
}


// ===========================================================================
// II.  OPERATOR!=  (C++11-C++17 only; synthesised in std from C++20)
// ===========================================================================

#if !RE_STD_LANG_IS_CPP20_OR_HIGHER

// operator!=
//   function: defined as !(lhs == rhs).
template<typename E1,
         typename E2>
RE_STD_CONSTEXPR_CPP20 bool
operator!=(
    unexpected<E1> const& _lhs,
    unexpected<E2> const& _rhs
)
{
    return !(_lhs == _rhs);
}

#endif  // !C++20


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_EXPECTED_UNEXPECTED_COMPARE_HPP
