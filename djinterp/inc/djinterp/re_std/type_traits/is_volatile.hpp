/******************************************************************************
* djinterp [restd]                                              is_volatile.hpp
*
* is_volatile trait header:
*   Yields true_type if _Type is volatile-qualified at the top level,
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
* path:      /inc/djinterp/re_std/type_traits/is_volatile.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_VOLATILE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_VOLATILE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_VOLATILE
// =============================================================================

// is_volatile
//   trait: false (primary template).
template<typename _Type>
struct is_volatile : false_type
{};

// is_volatile<volatile _Type>
//   trait: top-level volatile specialization.
template<typename _Type>
struct is_volatile<volatile _Type> : true_type
{};


// =============================================================================
// II.  IS_VOLATILE_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_volatile_v
    //   variable: convenience for is_volatile<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_volatile_v = is_volatile<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_VOLATILE_
