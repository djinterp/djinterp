/*******************************************************************************
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
* path:      /inc/re_std/type_traits/is_bounded_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_BOUNDED_ARRAY_HPP
#define RE_STD_TYPE_TRAITS_IS_BOUNDED_ARRAY_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_BOUNDED_ARRAY
// =============================================================================

// is_bounded_array
//   trait: false (primary template).
template<typename Type>
struct is_bounded_array : false_type
{};

// is_bounded_array<Type[N]>
//   trait: true for an array of known bound.
template<typename Type,
         std::size_t N>
struct is_bounded_array<Type[N]> : true_type
{};


// =============================================================================
// II.  IS_BOUNDED_ARRAY_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_bounded_array_v = is_bounded_array<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_BOUNDED_ARRAY_HPP
