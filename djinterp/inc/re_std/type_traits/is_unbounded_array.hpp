/******************************************************************************
* djinterp [re_std]                                       is_unbounded_array.hpp
*
* is_unbounded_array trait header:
*   is_unbounded_array<T>::value is true iff T is an array type of UNKNOWN
* bound -- `U[]`.  This is the incomplete array type that make_unique and the
* shared-pointer factories dispatch on.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_unbounded_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_UNBOUNDED_ARRAY_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_UNBOUNDED_ARRAY_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"



NS_RESTD


// =============================================================================
// I.   IS_UNBOUNDED_ARRAY
// =============================================================================

// is_unbounded_array
//   trait: false (primary template).
template<typename _Type>
struct is_unbounded_array : false_type
{};

// is_unbounded_array<_Type[]>
//   trait: true for an array of unknown bound.
template<typename _Type>
struct is_unbounded_array<_Type[]> : true_type
{};


// =============================================================================
// II.  IS_UNBOUNDED_ARRAY_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_unbounded_array_v = is_unbounded_array<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_UNBOUNDED_ARRAY_
