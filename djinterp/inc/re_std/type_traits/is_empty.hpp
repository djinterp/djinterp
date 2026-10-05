/*******************************************************************************
* djinterp [re_std]                                                 is_empty.hpp
*
* is_empty trait header:
*   is_empty<T>::value is true iff T is a class type with no non-static data
* members, no virtual functions, no virtual bases, and no non-empty bases.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_empty.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_EMPTY_HPP
#define RE_STD_TYPE_TRAITS_IS_EMPTY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_EMPTY  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_EMPTY
    #if defined(__has_builtin)
        #if __has_builtin(__is_empty)
            #define RE_STD_HAS_IS_EMPTY  1
        #else
            #define RE_STD_HAS_IS_EMPTY  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_EMPTY      1
    #else
        #define RE_STD_HAS_IS_EMPTY      0
    #endif
#endif  // RE_STD_HAS_IS_EMPTY


namespace re_std
{


// =============================================================================
// I.   IS_EMPTY
// =============================================================================

#if RE_STD_HAS_IS_EMPTY

// is_empty
//   trait: intrinsic-backed -- a class type occupying no storage of its own.
template<typename Type>
struct is_empty : integral_constant<bool, __is_empty(Type)>
{};

#else

// is_empty
//   trait: degraded fallback (always false) when the intrinsic is absent;
// this property is not observable at the library level.
template<typename Type>
struct is_empty : false_type
{};

#endif  // RE_STD_HAS_IS_EMPTY


// =============================================================================
// II.  IS_EMPTY_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_empty_v = is_empty<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_EMPTY_HPP
