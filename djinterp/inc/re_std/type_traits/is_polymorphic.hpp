/*******************************************************************************
* djinterp [re_std]                                           is_polymorphic.hpp
*
* is_polymorphic trait header:
*   is_polymorphic<T>::value is true iff T is a class type declaring or
* inheriting at least one virtual function.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_polymorphic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_POLYMORPHIC_HPP
#define RE_STD_TYPE_TRAITS_IS_POLYMORPHIC_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_POLYMORPHIC  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_POLYMORPHIC
    #if defined(__has_builtin)
        #if __has_builtin(__is_polymorphic)
            #define RE_STD_HAS_IS_POLYMORPHIC  1
        #else
            #define RE_STD_HAS_IS_POLYMORPHIC  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_POLYMORPHIC      1
    #else
        #define RE_STD_HAS_IS_POLYMORPHIC      0
    #endif
#endif  // RE_STD_HAS_IS_POLYMORPHIC


namespace re_std
{


// =============================================================================
// I.   IS_POLYMORPHIC
// =============================================================================

#if RE_STD_HAS_IS_POLYMORPHIC

// is_polymorphic
//   trait: intrinsic-backed -- a class with a virtual function.
template<typename Type>
struct is_polymorphic : integral_constant<bool, __is_polymorphic(Type)>
{};

#else

// is_polymorphic
//   trait: degraded fallback (always false) when the intrinsic is absent;
// this property is not observable at the library level.
template<typename Type>
struct is_polymorphic : false_type
{};

#endif  // RE_STD_HAS_IS_POLYMORPHIC


// =============================================================================
// II.  IS_POLYMORPHIC_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_polymorphic_v = is_polymorphic<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_POLYMORPHIC_HPP
