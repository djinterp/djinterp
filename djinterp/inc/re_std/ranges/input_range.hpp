/*******************************************************************************
* djinterp [re_std]                                              input_range.hpp
*
* input_range concept-trait header:
*   Provides the C++20 input_range concept as a SFINAE-detection
* trait. input_range<T>::value is true iff range<T> AND the
* iterator type's iterator_category derives from input_iterator_tag.
*
*   PORTABILITY:
*   C++11+. Variable spelling C++14+.
*
*   SIMPLIFICATION:
*   The C++20 input_range concept binds to the input_iterator concept,
* which checks more than iterator_category derivation (dereferenceable,
* equality_comparable, ...). Re_std approximates with iterator_category
* derivation alone — a conservative check that accepts all well-formed
* input iterators but may also accept malformed types that nominally
* expose input_iterator_tag without satisfying the operational
* requirements. Sufficient for SFINAE constraint use; not a strict
* concept binding.
*
*
* path:      /inc/re_std/ranges/input_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_INPUT_RANGE_HPP
#define RE_STD_RANGES_INPUT_RANGE_HPP 1

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

// input_range_impl
//   trait: only instantiates the iterator-category check when
// Type already passes the range trait, avoiding a hard error on
// non-range inputs.
template<typename Type,
         bool IsRange = range<Type>::value>
struct input_range_impl
    : false_type
{};

template<typename Type>
struct input_range_impl<Type, true>
    : is_base_of<input_iterator_tag,
                 typename iterator_traits<iterator_t<Type> >::iterator_category>
{};

}  // internal


// ===========================================================================
// I.   INPUT_RANGE
// ===========================================================================

template<typename Type>
struct input_range
    : internal::input_range_impl<Type>
{};


// ===========================================================================
// II.  INPUT_RANGE_V
// ===========================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool input_range_v = input_range<Type>::value;

#endif


}  // re_std


#endif  // alias templates + C++11


#endif  // RE_STD_RANGES_INPUT_RANGE_HPP
