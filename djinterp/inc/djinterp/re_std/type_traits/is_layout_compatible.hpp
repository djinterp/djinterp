/******************************************************************************
* re_std [type_traits]                                 is_layout_compatible.hpp
*
*   layout-compatibility detection:
*   `is_layout_compatible<_TypeA, _TypeB>` reports whether two types are
* layout-compatible in the sense of [basic.types.general] - i.e. whether they
* are the same type, layout-compatible enumerations, or layout-compatible
* standard-layout class types, in every case disregarding cv-qualification.
*
*   STD IS C++20; re_std IS C++98.
*   std added this trait in C++20, but the compiler builtin that answers the
* question is not itself a language feature: it is accepted in every language
* mode the compiler supports, and its result is a core constant expression at
* every tier.  re_std therefore ships the trait from C++98 - a 22-year
* back-port - with no language gate at all.  Only the builtin is gated.
*
*   DEGRADATION (no #error, ever):
*   When the builtin is absent the trait is still declared, but answers from a
* SOUND SUBSET rather than failing to compile: two types that are the same
* after cv-stripping are always layout-compatible, so that case still reports
* true.  Everything else reports false.  The result is therefore never a false
* POSITIVE - code that guards a reinterpret_cast on this trait stays correct -
* but it may be a false NEGATIVE.  Test D_RESTD_HAS_IS_LAYOUT_COMPATIBLE to
* find out which answer you are getting.
*
*   PRECONDITION:
*   _TypeA and _TypeB shall each be a complete type, cv void, or an array of
* unknown bound.  This mirrors std and cannot be enforced portably.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_layout_compatible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.12
******************************************************************************/

#ifndef RESTD_TYPE_TRAITS_IS_LAYOUT_COMPATIBLE_
#define RESTD_TYPE_TRAITS_IS_LAYOUT_COMPATIBLE_ 1

// re_std
#include "./type_traits.hpp"    // integral_constant, is_same, remove_cv


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_IS_LAYOUT_COMPATIBLE
//   constant: 1 if the __is_layout_compatible builtin is available.
//
//   __has_builtin is the primary probe and is deliberately tried first: it is
// the only mechanism that stays correct as vendors add the builtin in releases
// this table does not know about.  Clang in particular gained the two
// layout-compatibility TRAITS well after it gained most of its other type
// builtins, and gained the two pointer-interconvertibility FUNCTIONS later
// still, so a single family-wide version check would be wrong for it.  The
// version arms below are conservative floors for compilers whose __has_builtin
// either does not exist or does not answer for type traits.
#ifndef D_RESTD_HAS_IS_LAYOUT_COMPATIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_layout_compatible)
            #define D_RESTD_HAS_IS_LAYOUT_COMPATIBLE  1
        #endif
    #endif

    #ifndef D_RESTD_HAS_IS_LAYOUT_COMPATIBLE
        #if ( defined(D_ENV_COMPILER_GCC) &&                                  \
              D_ENV_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define D_RESTD_HAS_IS_LAYOUT_COMPATIBLE  1
        #elif ( defined(D_ENV_COMPILER_MSVC) &&                               \
                D_ENV_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define D_RESTD_HAS_IS_LAYOUT_COMPATIBLE  1
        #else
            #define D_RESTD_HAS_IS_LAYOUT_COMPATIBLE  0
        #endif
    #endif  // D_RESTD_HAS_IS_LAYOUT_COMPATIBLE (fallback)
#endif  // D_RESTD_HAS_IS_LAYOUT_COMPATIBLE (outer guard)


NS_DJINTERP
NS_RESTD

NS_INTERNAL

    // is_layout_compatible_base
    //   trait: classification core for is_layout_compatible.  The builtin
    // already disregards cv-qualification on both operands, so the intrinsic
    // arm forwards its arguments untouched.
#if D_RESTD_HAS_IS_LAYOUT_COMPATIBLE

    template<typename _TypeA,
             typename _TypeB>
    struct is_layout_compatible_base
        : integral_constant<bool, __is_layout_compatible(_TypeA, _TypeB)>
    {};

#else

    // is_layout_compatible_base (degraded)
    //   trait: sound-subset classification used when the builtin is absent.
    // Identical types (after cv-stripping) are layout-compatible by
    // definition; every other pair is reported false rather than guessed at.
    template<typename _TypeA,
             typename _TypeB>
    struct is_layout_compatible_base
        : is_same<typename remove_cv<_TypeA>::type,
                  typename remove_cv<_TypeB>::type>
    {};

#endif  // D_RESTD_HAS_IS_LAYOUT_COMPATIBLE

NS_END  // internal


// is_layout_compatible
//   trait: true if _TypeA and _TypeB are layout-compatible types.
template<typename _TypeA,
         typename _TypeB>
struct is_layout_compatible
    : internal::is_layout_compatible_base<_TypeA, _TypeB>
{};

// is_layout_compatible_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _TypeA,
             typename _TypeB>
    D_CONSTEXPR bool is_layout_compatible_v
        = is_layout_compatible<_TypeA, _TypeB>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // re_std
NS_END  // djinterp

#endif  // RESTD_TYPE_TRAITS_IS_LAYOUT_COMPATIBLE_
