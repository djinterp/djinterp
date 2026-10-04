/******************************************************************************
* djinterp [re_std]                                         is_bounded_array.hpp
*
* is_bounded_array trait header:
*   is_bounded_array<T>::value is true iff T is an array type of KNOWN
* bound -- `U[N]`.  Cv-qualification is preserved by the partial
* specialization, so `const U[N]` matches too.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_bounded_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_BOUNDED_ARRAY_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_BOUNDED_ARRAY_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include <cstddef>


NS_RESTD


// =============================================================================
// I.   IS_BOUNDED_ARRAY
// =============================================================================

// is_bounded_array
//   trait: false (primary template).
template<typename _Type>
struct is_bounded_array : false_type
{};

// is_bounded_array<_Type[_N]>
//   trait: true for an array of known bound.
template<typename _Type,
         std::size_t _N>
struct is_bounded_array<_Type[_N]> : true_type
{};


// =============================================================================
// II.  IS_BOUNDED_ARRAY_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_bounded_array_v = is_bounded_array<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_BOUNDED_ARRAY_
