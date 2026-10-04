/******************************************************************************
* djinterp [re_std]                                       unexpected_compare.hpp
*
* unexpected comparison header:
*   Provides operator== for two unexpected<E1> / unexpected<E2> wrappers.
* C++20 standard synthesises operator!= from this; re_std ships it
* explicitly on every tier so C++11–C++17 code can rely on it.
*
*   ELEMENT-TYPE REQUIREMENT:
*   _E1 and _E2 must be equality-comparable; the comparison defers
* directly to their operator==. Heterogeneous error types compare via
* whatever cross-type op== they expose.
*
*
* path:      /inc/djinterp/re_std/expected/unexpected_compare.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RE_STD_UNEXPECTED_COMPARE_
#define DJINTERP_RE_STD_UNEXPECTED_COMPARE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "./unexpected.hpp"


#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   OPERATOR==
// ===========================================================================

// operator==
//   function: two unexpected<E> values compare equal iff their stored
// errors compare equal.
template<typename _E1,
         typename _E2>
D_CONSTEXPR_CPP20 bool
operator==(
    unexpected<_E1> const& _lhs,
    unexpected<_E2> const& _rhs
)
{
    return _lhs.error() == _rhs.error();
}


// ===========================================================================
// II.  OPERATOR!=  (C++11-C++17 only; synthesised in std from C++20)
// ===========================================================================

#if !D_ENV_LANG_IS_CPP20_OR_HIGHER

// operator!=
//   function: defined as !(lhs == rhs).
template<typename _E1,
         typename _E2>
D_CONSTEXPR_CPP20 bool
operator!=(
    unexpected<_E1> const& _lhs,
    unexpected<_E2> const& _rhs
)
{
    return !(_lhs == _rhs);
}

#endif  // !C++20


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_UNEXPECTED_COMPARE_
