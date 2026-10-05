/*******************************************************************************
* djinterp [re_std]                                            forward_range.hpp
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
* path:      /inc/re_std/ranges/forward_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_FORWARD_RANGE_HPP
#define RE_STD_RANGES_FORWARD_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES &&                               \
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
         bool     IsRange = range<Type>::value>
struct forward_range_helper
    : false_type
{};

template<typename Type>
struct forward_range_helper<Type, true>
    : is_base_of<forward_iterator_tag,
                 typename iterator_traits<iterator_t<Type> >::iterator_category>
{};

}  // internal


// ===========================================================================
// I.   FORWARD_RANGE
// ===========================================================================

template<typename Type>
struct forward_range
    : internal::forward_range_helper<Type>
{};


// ===========================================================================
// II.  FORWARD_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool forward_range_v = forward_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_FORWARD_RANGE_HPP
