/******************************************************************************
* djinterp [re_std]                                   is_standard_layout.hpp
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
* path:      /inc/djinterp/re_std/type_traits/is_standard_layout.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_STANDARD_LAYOUT_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_STANDARD_LAYOUT_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_STANDARD_LAYOUT  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_STANDARD_LAYOUT
    #if defined(__has_builtin)
        #if __has_builtin(__is_standard_layout)
            #define D_RE_STD_HAS_IS_STANDARD_LAYOUT  1
        #else
            #define D_RE_STD_HAS_IS_STANDARD_LAYOUT  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_STANDARD_LAYOUT      1
    #else
        #define D_RE_STD_HAS_IS_STANDARD_LAYOUT      0
    #endif
#endif  // D_RE_STD_HAS_IS_STANDARD_LAYOUT


NS_RESTD


// =============================================================================
// I.   IS_STANDARD_LAYOUT
// =============================================================================

#if D_RE_STD_HAS_IS_STANDARD_LAYOUT

// is_standard_layout
//   trait: intrinsic-backed -- layout is compatible with the equivalent C struct.
template<typename _Type>
struct is_standard_layout : integral_constant<bool, __is_standard_layout(_Type)>
{};

#else

// is_standard_layout
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is conservative: callers fall back to member-by-member handling
// rather than treating the type as C-compatible.
template<typename _Type>
struct is_standard_layout : false_type
{};

#endif  // D_RE_STD_HAS_IS_STANDARD_LAYOUT


// =============================================================================
// II.  IS_STANDARD_LAYOUT_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_standard_layout_v = is_standard_layout<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_STANDARD_LAYOUT_
