/*******************************************************************************
* djinterp [re_std]                                                 is_final.hpp
*
* is_final trait header:
*   is_final<T>::value is true iff T is a class type marked `final`.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_final.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_FINAL_HPP
#define RE_STD_TYPE_TRAITS_IS_FINAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_FINAL  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_FINAL
    #if defined(__has_builtin)
        #if __has_builtin(__is_final)
            #define RE_STD_HAS_IS_FINAL  1
        #else
            #define RE_STD_HAS_IS_FINAL  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_FINAL      1
    #else
        #define RE_STD_HAS_IS_FINAL      0
    #endif
#endif  // RE_STD_HAS_IS_FINAL


namespace re_std
{


// =============================================================================
// I.   IS_FINAL
// =============================================================================

#if RE_STD_HAS_IS_FINAL

// is_final
//   trait: intrinsic-backed -- a class marked final.
template<typename Type>
struct is_final : integral_constant<bool, __is_final(Type)>
{};

#else

// is_final
//   trait: degraded fallback (always false) when the intrinsic is absent;
// this property is not observable at the library level.
template<typename Type>
struct is_final : false_type
{};

#endif  // RE_STD_HAS_IS_FINAL


// =============================================================================
// II.  IS_FINAL_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_final_v = is_final<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_FINAL_HPP
