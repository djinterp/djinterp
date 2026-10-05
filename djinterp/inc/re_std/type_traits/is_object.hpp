/*******************************************************************************
* djinterp [re_std]                                                is_object.hpp
*
* is_object trait header:
*   Yields true_type if Type is an object type, per [basic.types].
* An object type is any type that is NOT a function, NOT a reference,
* and NOT void. Equivalent to:
*   !(is_function || is_reference || is_void)
*
*     is_object<int>::value          -> true   (scalar is object)
*     is_object<int[5]>::value       -> true   (array is object)
*     is_object<S>::value            -> true   (class is object)
*     is_object<int*>::value         -> true   (pointer is object)
*     is_object<int&>::value         -> false  (reference)
*     is_object<void()>::value       -> false  (function)
*     is_object<void>::value         -> false  (void)
*
*
* path:      /inc/re_std/type_traits/is_object.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_OBJECT_HPP
#define RE_STD_TYPE_TRAITS_IS_OBJECT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./is_function.hpp"
#include "./is_reference.hpp"
#include "./is_void.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_OBJECT
// =============================================================================

// is_object
//   trait: NOT function, NOT reference, NOT void.
template<typename Type>
struct is_object
    : integral_constant<bool,
          ( !is_function<Type>::value  &&
            !is_reference<Type>::value &&
            !is_void<Type>::value )>
{};


// =============================================================================
// II.  IS_OBJECT_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_object_v
    //   variable: convenience for is_object<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_object_v = is_object<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_OBJECT_HPP
