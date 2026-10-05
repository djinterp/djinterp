/*******************************************************************************
* djinterp [re_std]                                                     view.hpp
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
* path:      /inc/re_std/ranges/view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_VIEW_HPP
#define RE_STD_RANGES_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./enable_view.hpp"


namespace re_std
{


// ===========================================================================
// I.   VIEW
// ===========================================================================

// view
//   trait: range that owns its iteration state cheaply — by C++20
// definition: movable + default_initializable + enable_view.
// Matches the C++20 ranges::view concept (with the movable / default-
// init simplifications noted in this header's banner).
template<typename Type>
struct view
    : integral_constant<bool,
                        range<Type>::value
                          && is_move_constructible<Type>::value
                          && is_default_constructible<Type>::value
                          && enable_view<Type>::value>
{};


// ===========================================================================
// II.  VIEW_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool view_v = view<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_VIEW_HPP
