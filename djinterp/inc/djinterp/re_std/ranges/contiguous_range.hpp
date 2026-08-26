/******************************************************************************
* djinterp [re_std]                                       contiguous_range.hpp
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
* path:      /inc/djinterp/re_std/ranges/contiguous_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_CONTIGUOUS_RANGE_
#define DJINTERP_RE_STD_RANGES_CONTIGUOUS_RANGE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


NS_RESTD


NS_INTERNAL

template<typename _Type,
         bool _IsRange = range<_Type>::value>
struct contiguous_range_impl
    : false_type
{};

template<typename _Type>
struct contiguous_range_impl<_Type, true>
    : is_base_of<contiguous_iterator_tag,
                 typename iterator_traits<iterator_t<_Type> >::iterator_category>
{};

NS_END  // internal


// ===========================================================================
// I.   CONTIGUOUS_RANGE
// ===========================================================================

template<typename _Type>
struct contiguous_range
    : internal::contiguous_range_impl<_Type>
{};


// ===========================================================================
// II.  CONTIGUOUS_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool contiguous_range_v = contiguous_range<_Type>::value;

#endif


NS_END  // re_std


#endif  // C++20+


#endif  // DJINTERP_RE_STD_RANGES_CONTIGUOUS_RANGE_
