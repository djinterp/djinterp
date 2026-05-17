/******************************************************************************
* djinterp [restd]                                     bidirectional_range.hpp
*
* bidirectional_range concept-trait header:
*   Provides the C++20 bidirectional_range concept as a SFINAE-
* detection trait. bidirectional_range<T>::value is true iff range<T>
* AND the iterator type's iterator_category derives from
* bidirectional_iterator_tag.
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*
* path:      /inc/djinterp/restd/ranges/bidirectional_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_BIDIRECTIONAL_RANGE_
#define DJINTERP_RESTD_RANGES_BIDIRECTIONAL_RANGE_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


NS_RESTD


NS_INTERNAL

template<typename _Type,
         bool _IsRange = range<_Type>::value>
struct bidirectional_range_impl
    : false_type
{};

template<typename _Type>
struct bidirectional_range_impl<_Type, true>
    : is_base_of<bidirectional_iterator_tag,
                 typename iterator_traits<iterator_t<_Type> >::iterator_category>
{};

NS_END  // internal


// ===========================================================================
// I.   BIDIRECTIONAL_RANGE
// ===========================================================================

template<typename _Type>
struct bidirectional_range
    : internal::bidirectional_range_impl<_Type>
{};


// ===========================================================================
// II.  BIDIRECTIONAL_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool bidirectional_range_v = bidirectional_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_BIDIRECTIONAL_RANGE_
