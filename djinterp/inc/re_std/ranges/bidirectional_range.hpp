/*******************************************************************************
* djinterp [re_std]                                      bidirectional_range.hpp
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
* path:      /inc/re_std/ranges/bidirectional_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_BIDIRECTIONAL_RANGE_HPP
#define RE_STD_RANGES_BIDIRECTIONAL_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


namespace re_std
{


namespace internal
{

template<typename Type,
         bool IsRange = range<Type>::value>
struct bidirectional_range_impl
    : false_type
{};

template<typename Type>
struct bidirectional_range_impl<Type, true>
    : is_base_of<bidirectional_iterator_tag,
                 typename iterator_traits<iterator_t<Type> >::iterator_category>
{};

}  // internal


// ===========================================================================
// I.   BIDIRECTIONAL_RANGE
// ===========================================================================

template<typename Type>
struct bidirectional_range
    : internal::bidirectional_range_impl<Type>
{};


// ===========================================================================
// II.  BIDIRECTIONAL_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool bidirectional_range_v = bidirectional_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_BIDIRECTIONAL_RANGE_HPP
