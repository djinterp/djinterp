/*******************************************************************************
* djinterp [re_std]                                           is_scoped_enum.hpp
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
* path:      /inc/re_std/type_traits/is_scoped_enum.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_SCOPED_ENUM_HPP
#define RE_STD_TYPE_TRAITS_IS_SCOPED_ENUM_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_enum.hpp"
#include "./is_convertible.hpp"
#include "./underlying_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_SCOPED_ENUM  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_SCOPED_ENUM
    #if defined(__has_builtin)
        #if __has_builtin(__is_scoped_enum)
            #define RE_STD_HAS_IS_SCOPED_ENUM  1
        #else
            #define RE_STD_HAS_IS_SCOPED_ENUM  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_SCOPED_ENUM      1
    #else
        #define RE_STD_HAS_IS_SCOPED_ENUM      0
    #endif
#endif  // RE_STD_HAS_IS_SCOPED_ENUM


namespace re_std
{


// =============================================================================
// I.   IS_SCOPED_ENUM
// =============================================================================

#if RE_STD_HAS_IS_SCOPED_ENUM

// is_scoped_enum
//   trait: intrinsic-backed -- an enumeration declared with enum class or enum struct.
template<typename Type>
struct is_scoped_enum : integral_constant<bool, __is_scoped_enum(Type)>
{};

#else

namespace internal
{

// is_scoped_enum_impl
//   trait: library-level implementation, selected when the intrinsic is
// absent. Primary template -- Type is not an enumeration.
template<typename Type,
         bool = is_enum<Type>::value>
struct is_scoped_enum_impl : false_type
{};

// is_scoped_enum_impl<Type, true>
//   trait: Type IS an enumeration, so the question reduces to whether it
// converts implicitly to its own underlying type. An unscoped enum does;
// a scoped one does not. That difference is the definition of scoped, and
// it is observable through is_convertible -- which is why this trait,
// alone among the eight, needs no intrinsic.
//
//   underlying_type is only well-formed for an enumeration, which is
// exactly what selecting this specialisation has already established.
template<typename Type>
struct is_scoped_enum_impl<Type, true>
    : integral_constant<bool,
          !is_convertible<Type,
                          typename underlying_type<Type>::type>::value>
{};

}  // internal

// is_scoped_enum
//   trait: library-level implementation. NOT a degradation -- it is exact,
// and it is the reason this header does not need the intrinsic to be
// correct on any compiler.
template<typename Type>
struct is_scoped_enum : internal::is_scoped_enum_impl<Type>
{};

#endif  // RE_STD_HAS_IS_SCOPED_ENUM


// =============================================================================
// II.  IS_SCOPED_ENUM_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_scoped_enum_v = is_scoped_enum<Type>::value;

#endif


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_IS_SCOPED_ENUM_HPP
