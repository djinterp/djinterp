/*******************************************************************************
* djinterp [re_std]                                       is_standard_layout.hpp
*
* is_standard_layout trait header:
*   is_standard_layout<T>::value is true iff T has a layout a C compiler
* would produce for the equivalent struct -- no virtual functions, no
* virtual bases, all non-static data members in one class and with the
* same access control.
*
*   It is the precondition for offsetof, for reinterpreting a pointer to
* the first member as a pointer to the object, and for passing a type
* across a C ABI boundary at all.
*
*   NO LIBRARY-LEVEL IMPLEMENTATION EXISTS -- intrinsic or nothing.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_standard_layout.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_STANDARD_LAYOUT_HPP
#define RE_STD_TYPE_TRAITS_IS_STANDARD_LAYOUT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_STANDARD_LAYOUT  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_STANDARD_LAYOUT
    #if defined(__has_builtin)
        #if __has_builtin(__is_standard_layout)
            #define RE_STD_HAS_IS_STANDARD_LAYOUT  1
        #else
            #define RE_STD_HAS_IS_STANDARD_LAYOUT  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_STANDARD_LAYOUT      1
    #else
        #define RE_STD_HAS_IS_STANDARD_LAYOUT      0
    #endif
#endif  // RE_STD_HAS_IS_STANDARD_LAYOUT


namespace re_std
{


// =============================================================================
// I.   IS_STANDARD_LAYOUT
// =============================================================================

#if RE_STD_HAS_IS_STANDARD_LAYOUT

// is_standard_layout
//   trait: intrinsic-backed -- layout is compatible with the equivalent C struct.
template<typename Type>
struct is_standard_layout : integral_constant<bool, __is_standard_layout(Type)>
{};

#else

// is_standard_layout
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is conservative: callers fall back to member-by-member handling
// rather than treating the type as C-compatible.
template<typename Type>
struct is_standard_layout : false_type
{};

#endif  // RE_STD_HAS_IS_STANDARD_LAYOUT


// =============================================================================
// II.  IS_STANDARD_LAYOUT_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_standard_layout_v = is_standard_layout<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_STANDARD_LAYOUT_HPP
