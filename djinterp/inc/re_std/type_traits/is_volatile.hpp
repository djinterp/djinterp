/*******************************************************************************
* djinterp [re_std]                                              is_volatile.hpp
*
* is_volatile trait header:
*   Yields true_type if Type is volatile-qualified at the top level,
* false_type otherwise. The check is for top-level volatile only;
* `volatile int*` (a pointer to volatile int) is not itself volatile.
*
*     is_volatile<volatile int>::value     -> true
*     is_volatile<int>::value              -> false
*     is_volatile<const volatile int>::value -> true
*     is_volatile<volatile int*>::value    -> false  (pointer not volatile)
*     is_volatile<int* volatile>::value    -> true   (pointer is volatile)
*     is_volatile<volatile int&>::value    -> false  (refs not cv-qual'able)
*
*
* path:      /inc/re_std/type_traits/is_volatile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_VOLATILE_HPP
#define RE_STD_TYPE_TRAITS_IS_VOLATILE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_VOLATILE
// =============================================================================

// is_volatile
//   trait: false (primary template).
template<typename Type>
struct is_volatile : false_type
{};

// is_volatile<volatile Type>
//   trait: top-level volatile specialization.
template<typename Type>
struct is_volatile<volatile Type> : true_type
{};


// =============================================================================
// II.  IS_VOLATILE_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_volatile_v
    //   variable: convenience for is_volatile<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_volatile_v = is_volatile<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_VOLATILE_HPP
