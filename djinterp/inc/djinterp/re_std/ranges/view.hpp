/******************************************************************************
* djinterp [re_std]                                                  view.hpp
*
* view concept-trait header:
*   Provides the C++20 view concept as a SFINAE-detection trait.
* view<T>::value is true iff range<T> AND T is movable AND
* default-initializable AND enable_view<T>.
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*   SIMPLIFICATIONS RELATIVE TO C++20:
*   - The C++20 'movable' concept requires move_constructible AND
*     swappable AND assignable_from<T&, T>. Re_std approximates
*     'movable' with 'move_constructible' alone — assignable_from is
*     not yet a separate trait in re_std, and swap is universally
*     available. The simplification is conservative (every C++20
*     movable type passes; some edge cases at the boundary may
*     differ on hostile types).
*   - 'default_initializable' is approximated as is_default_constructible.
*
*   Both simplifications are documented in coverage_data.py.
*
*
* path:      /inc/djinterp/re_std/ranges/view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_VIEW_
#define DJINTERP_RE_STD_RANGES_VIEW_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./enable_view.hpp"


NS_RESTD


// ===========================================================================
// I.   VIEW
// ===========================================================================

// view
//   trait: range that owns its iteration state cheaply — by C++20
// definition: movable + default_initializable + enable_view.
// Matches the C++20 ranges::view concept (with the movable / default-
// init simplifications noted in this header's banner).
template<typename _Type>
struct view
    : integral_constant<bool,
                        range<_Type>::value
                          && is_move_constructible<_Type>::value
                          && is_default_constructible<_Type>::value
                          && enable_view<_Type>::value>
{};


// ===========================================================================
// II.  VIEW_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool view_v = view<_Type>::value;

#endif


NS_END  // re_std


#endif  // alias templates + C++11


#endif  // DJINTERP_RE_STD_RANGES_VIEW_
