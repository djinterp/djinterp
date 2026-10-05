/*******************************************************************************
* djinterp [re_std]                                          is_null_pointer.hpp
*
* is_null_pointer trait header:
*   Yields true_type if Type (after cv-stripping) is std::nullptr_t,
* false_type otherwise. The C++14 trait spelling.
*
*     is_null_pointer<std::nullptr_t>::value         -> true
*     is_null_pointer<const std::nullptr_t>::value   -> true
*     is_null_pointer<int*>::value                   -> false
*     is_null_pointer<void*>::value                  -> false
*     is_null_pointer<int>::value                    -> false
*
*   PORTABILITY:
*   nullptr_t is a C++11 type. On C++98/03 the type does not exist; this
* trait is omitted entirely. Consumer code must gate on
* RE_STD_LANG_IS_CPP11_OR_HIGHER.
*
*
* path:      /inc/re_std/type_traits/is_null_pointer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NULL_POINTER_HPP
#define RE_STD_TYPE_TRAITS_IS_NULL_POINTER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: nullptr_t is C++11
#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// std
#include <cstddef>
// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./remove_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_NULL_POINTER
// =============================================================================

namespace internal
{

    // is_null_pointer_base
    //   trait: matches std::nullptr_t exactly (post cv-stripping).
    template<typename Type>
    struct is_null_pointer_base : false_type
    {};

    template<>
    struct is_null_pointer_base<std::nullptr_t> : true_type
    {};

}  // internal

// is_null_pointer
//   trait: true if Type (cv-stripped) is std::nullptr_t.
template<typename Type>
struct is_null_pointer
    : internal::is_null_pointer_base<typename remove_cv<Type>::type>
{};


// =============================================================================
// II.  IS_NULL_POINTER_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_null_pointer_v
    //   variable: convenience for is_null_pointer<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_null_pointer_v = is_null_pointer<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_NULL_POINTER_HPP
