/******************************************************************************
* djinterp [restd]                                             input_range.hpp
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
* equality_comparable, ...). Restd approximates with iterator_category
* derivation alone — a conservative check that accepts all well-formed
* input iterators but may also accept malformed types that nominally
* expose input_iterator_tag without satisfying the operational
* requirements. Sufficient for SFINAE constraint use; not a strict
* concept binding.
*
*
* path:      /inc/djinterp/restd/ranges/input_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_INPUT_RANGE_
#define DJINTERP_RESTD_RANGES_INPUT_RANGE_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./range.hpp"
#include "./iterator_t.hpp"


NS_RESTD


NS_INTERNAL

// input_range_impl
//   trait: only instantiates the iterator-category check when
// _Type already passes the range trait, avoiding a hard error on
// non-range inputs.
template<typename _Type,
         bool _IsRange = range<_Type>::value>
struct input_range_impl
    : false_type
{};

template<typename _Type>
struct input_range_impl<_Type, true>
    : is_base_of<input_iterator_tag,
                 typename iterator_traits<iterator_t<_Type> >::iterator_category>
{};

NS_END  // internal


// ===========================================================================
// I.   INPUT_RANGE
// ===========================================================================

template<typename _Type>
struct input_range
    : internal::input_range_impl<_Type>
{};


// ===========================================================================
// II.  INPUT_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool input_range_v = input_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_INPUT_RANGE_
