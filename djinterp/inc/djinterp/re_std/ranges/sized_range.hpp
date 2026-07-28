/******************************************************************************
* djinterp [restd]                                             sized_range.hpp
*
* sized_range concept-trait header:
*   Provides the C++20 sized_range concept as a SFINAE-detection
* trait. sized_range<T>::value is true iff range<T> AND restd::size
* is well-formed when applied to an lvalue of type T.
*
*   PORTABILITY:
*   C++11+ (via range + decltype detection on restd::size).
* C++14+ variable spelling sized_range_v<T>.
*
*
* path:      /inc/djinterp/restd/ranges/sized_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_SIZED_RANGE_
#define DJINTERP_RESTD_RANGES_SIZED_RANGE_ 1

#include "../../core/djinterp.hpp"

#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES && \
      D_ENV_LANG_IS_CPP11_OR_HIGHER )

#include "../type_traits/type_traits.hpp"
#include "../utility/declval.hpp"
#include "../iterator/size.hpp"
#include "./range.hpp"


NS_RESTD


NS_INTERNAL

// has_size
//   trait: SFINAE on restd::size(declval<T&>()).
template<typename _Type,
         typename = void>
struct has_size
    : false_type
{};

template<typename _Type>
struct has_size<_Type,
                void_t<decltype(restd::size(declval<_Type&>()))> >
    : true_type
{};

NS_END  // internal


// ===========================================================================
// I.   SIZED_RANGE
// ===========================================================================

// sized_range
//   trait: range whose ranges::size is well-formed. Matches the
// C++20 ranges::sized_range concept.
template<typename _Type>
struct sized_range
    : integral_constant<bool,
                        range<_Type>::value &&
                        internal::has_size<_Type>::value>
{};


// ===========================================================================
// II.  SIZED_RANGE_V
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool sized_range_v = sized_range<_Type>::value;

#endif


NS_END  // restd


#endif  // alias templates + C++11


#endif  // DJINTERP_RESTD_RANGES_SIZED_RANGE_
