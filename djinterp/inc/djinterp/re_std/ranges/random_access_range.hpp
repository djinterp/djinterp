/******************************************************************************
* djinterp [restd]                                     random_access_range.hpp
*
* random_access_range concept-trait header:
*   Provides the C++20 random_access_range concept as a SFINAE-
* detection trait. random_access_range<T>::value is true iff range<T>
* AND the iterator type's iterator_category derives from
* random_access_iterator_tag.
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*
* path:      /inc/djinterp/restd/ranges/random_access_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANDOM_ACCESS_RANGE_
#define DJINTERP_RESTD_RANGES_RANDOM_ACCESS_RANGE_ 1

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
struct random_access_range_impl
    : false_type
{};

template<typename _Type>
struct random_access_range_impl<_Type, true>
    : is_base_of<random_access_iterator_tag,
                 typename iterator_traits<iterator_t<_Type> >::iterator_category>
{};

NS_END  // internal


// ===========================================================================
// I.   RANDOM_ACCESS_RANGE
// ===========================================================================

template<typename _Type>
struct random_access_range
    : internal::random_access_range_impl<_Type>
{};


// ===========================================================================
// II.  RANDOM_ACCESS_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool random_access_range_v = random_access_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_RANDOM_ACCESS_RANGE_
