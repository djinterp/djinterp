/******************************************************************************
* djinterp [re_std]                                       is_scoped_enum.hpp
*
* is_scoped_enum trait header:
*   is_scoped_enum<T>::value is true iff T is a scoped enumeration --
* declared enum class or enum struct.
*
*   THE ONE TRAIT IN THIS GROUP THAT DOES NOT NEED AN INTRINSIC:
*   Scopedness is observable through the type system. A scoped enum does
* not convert implicitly to its underlying type and an unscoped one does,
* so is_enum combined with is_convertible answers the question exactly.
* The intrinsic is used where available because it is cheaper for the
* compiler, but the fallback below is EXACT rather than degraded -- unlike
* the other seven, where a missing intrinsic means a missing answer.
*
*   BACK-PORT: std added this in C++23; re_std provides it from C++11, on
* every compiler, with or without the intrinsic -- a twelve-year lead.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_scoped_enum.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_SCOPED_ENUM_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_SCOPED_ENUM_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_enum.hpp"
#include "./is_convertible.hpp"
#include "./underlying_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_SCOPED_ENUM  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_SCOPED_ENUM
    #if defined(__has_builtin)
        #if __has_builtin(__is_scoped_enum)
            #define D_RE_STD_HAS_IS_SCOPED_ENUM  1
        #else
            #define D_RE_STD_HAS_IS_SCOPED_ENUM  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_SCOPED_ENUM      1
    #else
        #define D_RE_STD_HAS_IS_SCOPED_ENUM      0
    #endif
#endif  // D_RE_STD_HAS_IS_SCOPED_ENUM


NS_RESTD


// =============================================================================
// I.   IS_SCOPED_ENUM
// =============================================================================

#if D_RE_STD_HAS_IS_SCOPED_ENUM

// is_scoped_enum
//   trait: intrinsic-backed -- an enumeration declared with enum class or enum struct.
template<typename _Type>
struct is_scoped_enum : integral_constant<bool, __is_scoped_enum(_Type)>
{};

#else

NS_INTERNAL

// is_scoped_enum_impl
//   trait: library-level implementation, selected when the intrinsic is
// absent. Primary template -- _Type is not an enumeration.
template<typename _Type,
         bool = is_enum<_Type>::value>
struct is_scoped_enum_impl : false_type
{};

// is_scoped_enum_impl<_Type, true>
//   trait: _Type IS an enumeration, so the question reduces to whether it
// converts implicitly to its own underlying type. An unscoped enum does;
// a scoped one does not. That difference is the definition of scoped, and
// it is observable through is_convertible -- which is why this trait,
// alone among the eight, needs no intrinsic.
//
//   underlying_type is only well-formed for an enumeration, which is
// exactly what selecting this specialisation has already established.
template<typename _Type>
struct is_scoped_enum_impl<_Type, true>
    : integral_constant<bool,
          !is_convertible<_Type,
                          typename underlying_type<_Type>::type>::value>
{};

NS_END  // internal

// is_scoped_enum
//   trait: library-level implementation. NOT a degradation -- it is exact,
// and it is the reason this header does not need the intrinsic to be
// correct on any compiler.
template<typename _Type>
struct is_scoped_enum : internal::is_scoped_enum_impl<_Type>
{};

#endif  // D_RE_STD_HAS_IS_SCOPED_ENUM


// =============================================================================
// II.  IS_SCOPED_ENUM_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_scoped_enum_v = is_scoped_enum<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_SCOPED_ENUM_
