/******************************************************************************
* djinterp [re_std]                               has_virtual_destructor.hpp
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
* path:      /inc/djinterp/re_std/type_traits/has_virtual_destructor.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_HAS_VIRTUAL_DESTRUCTOR_
#define DJINTERP_RE_STD_TYPE_TRAITS_HAS_VIRTUAL_DESTRUCTOR_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR
    #if defined(__has_builtin)
        #if __has_builtin(__has_virtual_destructor)
            #define D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR  1
        #else
            #define D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR      1
    #else
        #define D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR      0
    #endif
#endif  // D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR


NS_RESTD


// =============================================================================
// I.   HAS_VIRTUAL_DESTRUCTOR
// =============================================================================

#if D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR

// has_virtual_destructor
//   trait: intrinsic-backed -- the destructor is declared virtual.
template<typename _Type>
struct has_virtual_destructor : integral_constant<bool, __has_virtual_destructor(_Type)>
{};

#else

// has_virtual_destructor
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is conservative: a caller told there is no virtual destructor
// avoids polymorphic deletion, which is the safe choice.
template<typename _Type>
struct has_virtual_destructor : false_type
{};

#endif  // D_RE_STD_HAS_HAS_VIRTUAL_DESTRUCTOR


// =============================================================================
// II.  HAS_VIRTUAL_DESTRUCTOR_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool has_virtual_destructor_v = has_virtual_destructor<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_HAS_VIRTUAL_DESTRUCTOR_
