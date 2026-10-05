/*******************************************************************************
* djinterp [re_std]                                              is_abstract.hpp
*
* is_abstract trait header:
*   is_abstract<T>::value is true iff T is a class type with at least one
* pure virtual function that no derived override has supplied -- that is,
* iff T cannot be instantiated.
*
*   NO LIBRARY-LEVEL IMPLEMENTATION EXISTS. The classic
* is-it-constructible probe cannot answer this: an abstract class may
* still have a usable constructor signature, and forming the test
* expression is itself ill-formed for an abstract type.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_abstract.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_ABSTRACT_HPP
#define RE_STD_TYPE_TRAITS_IS_ABSTRACT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_ABSTRACT  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_ABSTRACT
    #if defined(__has_builtin)
        #if __has_builtin(__is_abstract)
            #define RE_STD_HAS_IS_ABSTRACT  1
        #else
            #define RE_STD_HAS_IS_ABSTRACT  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_ABSTRACT      1
    #else
        #define RE_STD_HAS_IS_ABSTRACT      0
    #endif
#endif  // RE_STD_HAS_IS_ABSTRACT


namespace re_std
{


// =============================================================================
// I.   IS_ABSTRACT
// =============================================================================

#if RE_STD_HAS_IS_ABSTRACT

// is_abstract
//   trait: intrinsic-backed -- a class type with at least one unoverridden pure virtual function.
template<typename Type>
struct is_abstract : integral_constant<bool, __is_abstract(Type)>
{};

#else

// is_abstract
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is the less useful degradation of the two available -- a caller
// told a type is not abstract may attempt to instantiate it and fail to
// compile. That is still a compile-time failure, not a silent one, and
// the intrinsic is present on every compiler re_std targets.
template<typename Type>
struct is_abstract : false_type
{};

#endif  // RE_STD_HAS_IS_ABSTRACT


// =============================================================================
// II.  IS_ABSTRACT_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_abstract_v = is_abstract<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_ABSTRACT_HPP
