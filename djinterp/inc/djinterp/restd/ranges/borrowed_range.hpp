/******************************************************************************
* djinterp [restd]                                          borrowed_range.hpp
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
* path:      /inc/djinterp/restd/ranges/borrowed_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_BORROWED_RANGE_
#define DJINTERP_RESTD_RANGES_BORROWED_RANGE_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./enable_borrowed_range.hpp"


NS_RESTD


// ===========================================================================
// I.   BORROWED_RANGE
// ===========================================================================

// borrowed_range
//   trait: range whose iterators remain valid after the range
// itself is destroyed. True when T is an lvalue reference (the
// underlying object outlives the range expression) OR when
// enable_borrowed_range was specialised true for the unqualified
// value type. Matches the C++20 ranges::borrowed_range concept.
template<typename _Type>
struct borrowed_range
    : integral_constant<bool,
                        range<_Type>::value
                          && (is_lvalue_reference<_Type>::value
                              || enable_borrowed_range<
                                     typename remove_cv<
                                         typename remove_reference<_Type>::type
                                     >::type
                                 >::value)>
{};


// ===========================================================================
// II.  BORROWED_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool borrowed_range_v = borrowed_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_BORROWED_RANGE_
