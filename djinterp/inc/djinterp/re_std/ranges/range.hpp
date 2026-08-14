/******************************************************************************
* djinterp [restd]                                                   range.hpp
*
* range concept-trait header:
*   Provides the C++20 range concept as a SFINAE-detection trait.
* range<T>::value is true iff restd::begin and restd::end are both
* well-formed when applied to an lvalue of type T.
*
*   PORTABILITY:
*   - C++11+: real trait via void_t partial-spec SFINAE on the
*     already-shipped iterator_t / sentinel_t aliases.
*   - C++14+: variable-template alias range_v<T> is also defined.
*   - C++98/03: the underlying iterator_t/sentinel_t require alias
*     templates + decltype, so range itself is unavailable; the
*     header is empty on those tiers.
*
*   NAMING:
*   Matches the C++20 concept name std::ranges::range. On a future
* C++20-enabled restd build the trait struct will be replaced by an
* actual `concept range = ...` declaration in the same name slot;
* until then user code should prefer the portable variable spelling
* range_v<T> (C++14+) over the trait form range<T>::value.
*
*
* path:      /inc/djinterp/re_std/ranges/range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_RANGE_
#define DJINTERP_RESTD_RANGES_RANGE_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL DETECTION
// ===========================================================================

NS_INTERNAL

// range_impl
//   trait: SFINAE-friendly inner. Specialised when the iterator_t /
// sentinel_t aliases are both well-formed for _Type.
template<typename _Type,
         typename = void>
struct range_impl
    : false_type
{};

template<typename _Type>
struct range_impl<_Type,
                  void_t<iterator_t<_Type>,
                         sentinel_t<_Type> > >
    : true_type
{};

NS_END  // internal


// ===========================================================================
// I.   RANGE (concept-trait)
// ===========================================================================

// range
//   trait: true when restd::begin and restd::end are well-formed
// on lvalues of _Type. Matches the C++20 ranges::range concept.
template<typename _Type>
struct range
    : internal::range_impl<_Type>
{};


// ===========================================================================
// II.  RANGE_V (variable template, C++14+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// range_v
//   variable: portable spelling of range<_Type>::value.
template<typename _Type>
D_CONSTEXPR bool range_v = range<_Type>::value;

#endif  // variable templates


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_RANGE_
