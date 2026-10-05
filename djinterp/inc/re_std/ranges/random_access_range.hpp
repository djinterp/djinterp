/*******************************************************************************
* djinterp [re_std]                                      random_access_range.hpp
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
* path:      /inc/re_std/ranges/random_access_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_RANDOM_ACCESS_RANGE_HPP
#define RE_STD_RANGES_RANDOM_ACCESS_RANGE_HPP 1

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
struct random_access_range_impl
    : false_type
{};

template<typename Type>
struct random_access_range_impl<Type, true>
    : is_base_of<random_access_iterator_tag,
                 typename iterator_traits<iterator_t<Type> >::iterator_category>
{};

}  // internal


// ===========================================================================
// I.   RANDOM_ACCESS_RANGE
// ===========================================================================

template<typename Type>
struct random_access_range
    : internal::random_access_range_impl<Type>
{};


// ===========================================================================
// II.  RANDOM_ACCESS_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool random_access_range_v = random_access_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_RANDOM_ACCESS_RANGE_HPP
