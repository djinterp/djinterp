/*******************************************************************************
* djinterp [re_std]                                             common_range.hpp
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
* path:      /inc/re_std/ranges/common_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_COMMON_RANGE_HPP
#define RE_STD_RANGES_COMMON_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES && \
      RE_STD_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"


namespace re_std
{


namespace internal
{

// common_range_impl
//   trait: when range<Type>, compares iterator_t and sentinel_t.
// Otherwise false. Guarded so that iterator_t / sentinel_t are
// only instantiated for ranges (avoids hard error on non-range
// inputs).
template<typename Type,
         bool IsRange = range<Type>::value>
struct common_range_impl
    : false_type
{};

template<typename Type>
struct common_range_impl<Type, true>
    : is_same<iterator_t<Type>, sentinel_t<Type> >
{};

}  // internal


// ===========================================================================
// I.   COMMON_RANGE
// ===========================================================================

// common_range
//   trait: range whose iterator and sentinel types coincide.
// Matches the C++20 ranges::common_range concept.
// note: legacy iterator-pair algorithms (those taking [first, last)
// of the same iterator type) accept common_range arguments
// directly; non-common ranges must be adapted via common_view.
template<typename Type>
struct common_range
    : internal::common_range_impl<Type>
{};


// ===========================================================================
// II.  COMMON_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool common_range_v = common_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_COMMON_RANGE_HPP
