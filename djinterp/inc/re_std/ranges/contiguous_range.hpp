/*******************************************************************************
* djinterp [re_std]                                         contiguous_range.hpp
*
* contiguous_range concept-trait header:
*   Provides the C++20 contiguous_range concept as a SFINAE-detection
* trait. contiguous_range<T>::value is true iff range<T> AND the
* iterator type's iterator_category derives from contiguous_iterator_tag.
*
*   PORTABILITY:
*   - Requires contiguous_iterator_tag which is shipped C++20+ in
*     re_std's <iterator>. The trait itself is therefore gated on
*     C++20+ language tier; below C++20 the header is empty.
*   - Variable spelling C++14+ (which combined with the C++20 gate
*     means: only ever present on C++20+).
*
*   DETECTION LIMITATION:
*   In re_std, raw pointers carry iterator_concept = contiguous_iterator_tag
* but iterator_category = random_access_iterator_tag. Raw pointers are
* therefore detected by checking iterator_concept first when it is
* present (C++20+); user-defined contiguous iterators must explicitly
* expose iterator_category = contiguous_iterator_tag because re_std's
* iterator_traits primary does not yet pull iterator_concept through
* (see SYMBOLS_ITERATOR notes on iter_concept). This limitation is
* tracked in the iterator roadmap.
*
*
* path:      /inc/re_std/ranges/contiguous_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_CONTIGUOUS_RANGE_HPP
#define RE_STD_RANGES_CONTIGUOUS_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

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
struct contiguous_range_impl
    : false_type
{};

template<typename Type>
struct contiguous_range_impl<Type, true>
    : is_base_of<contiguous_iterator_tag,
                 typename iterator_traits<iterator_t<Type> >::iterator_category>
{};

}  // internal


// ===========================================================================
// I.   CONTIGUOUS_RANGE
// ===========================================================================

template<typename Type>
struct contiguous_range
    : internal::contiguous_range_impl<Type>
{};


// ===========================================================================
// II.  CONTIGUOUS_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool contiguous_range_v = contiguous_range<Type>::value;

#endif


}  // re_std


#endif  // C++20+


#endif  // RE_STD_RANGES_CONTIGUOUS_RANGE_HPP
