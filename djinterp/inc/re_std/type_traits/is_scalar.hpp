/*******************************************************************************
* djinterp [re_std]                                                is_scalar.hpp
*
* is_scalar trait header:
*   Yields true_type if Type is a scalar type (per [basic.types]):
*     - arithmetic (integral or floating-point)
*     - enumeration
*     - pointer
*     - pointer-to-member
*     - std::nullptr_t (C++11+)
*
*     is_scalar<int>::value             -> true   (arithmetic)
*     is_scalar<float>::value           -> true   (arithmetic)
*     is_scalar<int*>::value            -> true   (pointer)
*     is_scalar<int (S::*)>::value      -> true   (member pointer)
*     is_scalar<std::nullptr_t>::value  -> true   (nullptr_t, C++11+)
*     is_scalar<int[5]>::value          -> false  (array)
*     is_scalar<void()>::value          -> false  (function)
*     is_scalar<S>::value               -> false  (class)
*     is_scalar<int&>::value            -> false  (reference)
*     is_scalar<void>::value            -> false
*
*
* path:      /inc/re_std/type_traits/is_scalar.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_SCALAR_HPP
#define RE_STD_TYPE_TRAITS_IS_SCALAR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./is_arithmetic.hpp"
#include "./is_enum.hpp"
#include "./is_pointer.hpp"
#include "./is_member_pointer.hpp"

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "./is_null_pointer.hpp"
#endif


namespace re_std
{


// =============================================================================
// I.   IS_SCALAR
// =============================================================================

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // is_scalar (C++11+)
    //   trait: arithmetic OR enum OR pointer OR member-pointer OR nullptr_t.
    template<typename Type>
    struct is_scalar
        : integral_constant<bool,
              ( is_arithmetic<Type>::value      ||
                is_enum<Type>::value            ||
                is_pointer<Type>::value         ||
                is_member_pointer<Type>::value  ||
                is_null_pointer<Type>::value )>
    {};

#else

    // is_scalar (C++98/03)
    //   trait: arithmetic OR enum OR pointer OR member-pointer.
    template<typename Type>
    struct is_scalar
        : integral_constant<bool,
              ( is_arithmetic<Type>::value      ||
                is_enum<Type>::value            ||
                is_pointer<Type>::value         ||
                is_member_pointer<Type>::value )>
    {};

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


// =============================================================================
// II.  IS_SCALAR_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_scalar_v
    //   variable: convenience for is_scalar<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_scalar_v = is_scalar<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_SCALAR_HPP
