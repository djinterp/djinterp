/*******************************************************************************
* djinterp [re_std]                                   has_virtual_destructor.hpp
*
* has_virtual_destructor trait header:
*   has_virtual_destructor<T>::value is true iff T has a virtual
* destructor -- the precondition for deleting a derived object through a
* base pointer without undefined behaviour.
*
*   NO LIBRARY-LEVEL IMPLEMENTATION EXISTS -- virtualness of a member is
* not observable through the type system.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/has_virtual_destructor.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_HAS_VIRTUAL_DESTRUCTOR_HPP
#define RE_STD_TYPE_TRAITS_HAS_VIRTUAL_DESTRUCTOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR
    #if defined(__has_builtin)
        #if __has_builtin(__has_virtual_destructor)
            #define RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR  1
        #else
            #define RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR      1
    #else
        #define RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR      0
    #endif
#endif  // RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR


namespace re_std
{


// =============================================================================
// I.   HAS_VIRTUAL_DESTRUCTOR
// =============================================================================

#if RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR

// has_virtual_destructor
//   trait: intrinsic-backed -- the destructor is declared virtual.
template<typename Type>
struct has_virtual_destructor : integral_constant<bool, __has_virtual_destructor(Type)>
{};

#else

// has_virtual_destructor
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is conservative: a caller told there is no virtual destructor
// avoids polymorphic deletion, which is the safe choice.
template<typename Type>
struct has_virtual_destructor : false_type
{};

#endif  // RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR


// =============================================================================
// II.  HAS_VIRTUAL_DESTRUCTOR_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool has_virtual_destructor_v = has_virtual_destructor<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_HAS_VIRTUAL_DESTRUCTOR_HPP
