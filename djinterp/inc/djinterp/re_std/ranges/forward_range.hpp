/******************************************************************************
* djinterp [restd]                                           forward_range.hpp
*
* forward_range concept-trait header:
*   Provides the C++20 forward_range concept as a SFINAE-detection
* trait. forward_range<T>::value is true iff range<T> AND the
* iterator type's iterator_category derives from forward_iterator_tag.
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*   SIMPLIFICATION:
*   See input_range.hpp — the same iterator_category-based check
* is used here, accepting iterator types that may not formally model
* the C++20 forward_iterator concept beyond category derivation.
*
*
* path:      /inc/djinterp/re_std/ranges/forward_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_FORWARD_RANGE_
#define DJINTERP_RESTD_RANGES_FORWARD_RANGE_ 1

// djinterp
#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES &&                               \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


NS_RESTD


NS_INTERNAL

template<typename _Type,
         bool     _IsRange = range<_Type>::value>
struct forward_range_helper
    : false_type
{};

template<typename _Type>
struct forward_range_helper<_Type, true>
    : is_base_of<forward_iterator_tag,
                 typename iterator_traits<iterator_t<_Type> >::iterator_category>
{};

NS_END  // internal


// ===========================================================================
// I.   FORWARD_RANGE
// ===========================================================================

template<typename _Type>
struct forward_range
    : internal::forward_range_helper<_Type>
{};


// ===========================================================================
// II.  FORWARD_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool forward_range_v = forward_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_FORWARD_RANGE_
