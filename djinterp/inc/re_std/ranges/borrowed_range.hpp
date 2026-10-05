/*******************************************************************************
* djinterp [re_std]                                           borrowed_range.hpp
*
* borrowed_range concept-trait header:
*   Provides the C++20 borrowed_range concept as a SFINAE-detection
* trait. borrowed_range<T>::value is true iff range<T> AND either
* T is an lvalue reference type OR enable_borrowed_range is true
* for the cv-stripped, ref-stripped form of T.
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*
* path:      /inc/re_std/ranges/borrowed_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_BORROWED_RANGE_HPP
#define RE_STD_RANGES_BORROWED_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./enable_borrowed_range.hpp"


namespace re_std
{


// ===========================================================================
// I.   BORROWED_RANGE
// ===========================================================================

// borrowed_range
//   trait: range whose iterators remain valid after the range
// itself is destroyed. True when T is an lvalue reference (the
// underlying object outlives the range expression) OR when
// enable_borrowed_range was specialised true for the unqualified
// value type. Matches the C++20 ranges::borrowed_range concept.
template<typename Type>
struct borrowed_range
    : integral_constant<bool,
                        range<Type>::value
                          && (is_lvalue_reference<Type>::value
                              || enable_borrowed_range<
                                     typename remove_cv<
                                         typename remove_reference<Type>::type
                                     >::type
                                 >::value)>
{};


// ===========================================================================
// II.  BORROWED_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool borrowed_range_v = borrowed_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_BORROWED_RANGE_HPP
