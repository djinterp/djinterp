/******************************************************************************
* djinterp [restd]                                            common_range.hpp
*
* common_range concept-trait header:
*   Provides the C++20 common_range concept as a SFINAE-detection
* trait. common_range<T>::value is true iff range<T> AND iterator_t<T>
* is the same type as sentinel_t<T> (i.e. the range exposes a legacy
* iterator-pair interface where end() returns an iterator, not a
* distinct sentinel type).
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*
* path:      /inc/djinterp/re_std/ranges/common_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_COMMON_RANGE_
#define DJINTERP_RESTD_RANGES_COMMON_RANGE_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"


NS_RESTD


NS_INTERNAL

// common_range_impl
//   trait: when range<_Type>, compares iterator_t and sentinel_t.
// Otherwise false. Guarded so that iterator_t / sentinel_t are
// only instantiated for ranges (avoids hard error on non-range
// inputs).
template<typename _Type,
         bool _IsRange = range<_Type>::value>
struct common_range_impl
    : false_type
{};

template<typename _Type>
struct common_range_impl<_Type, true>
    : is_same<iterator_t<_Type>, sentinel_t<_Type> >
{};

NS_END  // internal


// ===========================================================================
// I.   COMMON_RANGE
// ===========================================================================

// common_range
//   trait: range whose iterator and sentinel types coincide.
// Matches the C++20 ranges::common_range concept.
// note: legacy iterator-pair algorithms (those taking [first, last)
// of the same iterator type) accept common_range arguments
// directly; non-common ranges must be adapted via common_view.
template<typename _Type>
struct common_range
    : internal::common_range_impl<_Type>
{};


// ===========================================================================
// II.  COMMON_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool common_range_v = common_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_COMMON_RANGE_
