/*******************************************************************************
* djinterp [re_std]                                                 is_const.hpp
*
* is_const trait header:
*   Detects whether a type has a top-level const qualifier. Note that
* references are never const-qualified at the top level (the referent
* may be).
*
*     is_const<const int>::value       -> true
*     is_const<int>::value             -> false
*     is_const<int* const>::value      -> true   (pointer is const)
*     is_const<const int*>::value      -> false  (pointee is const, not
*                                                pointer)
*     is_const<const int&>::value      -> false  (references are never
*                                                const at top level)
*
*
* path:      /inc/re_std/type_traits/is_const.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_CONST_HPP
#define RE_STD_TYPE_TRAITS_IS_CONST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_CONST
// =============================================================================

// is_const
//   trait: false (primary template).
template<typename Type>
struct is_const : false_type
{};

// is_const<const Type>
//   trait: true for top-level const-qualified types.
template<typename Type>
struct is_const<const Type> : true_type
{};


// =============================================================================
// II.  IS_CONST_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_const_v
    //   variable: convenience for is_const<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_const_v = is_const<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_CONST_HPP
