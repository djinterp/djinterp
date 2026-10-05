/*******************************************************************************
* djinterp [re_std]                                                  is_void.hpp
*
* is_void trait header:
*   Detects whether a type, ignoring cv-qualifiers, is `void`.
*
*     is_void<void>::value                -> true
*     is_void<const void>::value          -> true
*     is_void<volatile void>::value       -> true
*     is_void<const volatile void>::value -> true
*     is_void<int>::value                 -> false
*     is_void<void*>::value               -> false  (pointer to void is not
*                                                   void)
*
*
* path:      /inc/re_std/type_traits/is_void.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_VOID_HPP
#define RE_STD_TYPE_TRAITS_IS_VOID_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_VOID
// =============================================================================

namespace internal
{

    // is_void_base
    //   trait: false (primary template).
    template<typename Type>
    struct is_void_base : false_type
    {};

    // is_void_base<void>
    //   trait: true for void.
    template<>
    struct is_void_base<void> : true_type
    {};

}  // internal

// is_void
//   trait: true if Type is `void`, ignoring cv-qualifiers.
template<typename Type>
struct is_void
    : internal::is_void_base<typename remove_cv<Type>::type>
{};


// =============================================================================
// II.  IS_VOID_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_void_v
    //   variable: convenience for is_void<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_void_v = is_void<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_VOID_HPP
