/*******************************************************************************
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
* path:      /inc/re_std/type_traits/is_unbounded_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_UNBOUNDED_ARRAY_HPP
#define RE_STD_TYPE_TRAITS_IS_UNBOUNDED_ARRAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"



namespace re_std
{


// =============================================================================
// I.   IS_UNBOUNDED_ARRAY
// =============================================================================

// is_unbounded_array
//   trait: false (primary template).
template<typename Type>
struct is_unbounded_array : false_type
{};

// is_unbounded_array<Type[]>
//   trait: true for an array of unknown bound.
template<typename Type>
struct is_unbounded_array<Type[]> : true_type
{};


// =============================================================================
// II.  IS_UNBOUNDED_ARRAY_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_unbounded_array_v = is_unbounded_array<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_UNBOUNDED_ARRAY_HPP
