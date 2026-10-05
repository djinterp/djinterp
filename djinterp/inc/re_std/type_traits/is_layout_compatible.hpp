/*******************************************************************************
* djinterp [re_std]                                     is_layout_compatible.hpp
*
* is_layout_compatible trait header:
*   layout-compatibility detection:
*   `is_layout_compatible<TypeA, TypeB>` reports whether two types are
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
* but it may be a false NEGATIVE.  Test RE_STD_HAS_IS_LAYOUT_COMPATIBLE to
* find out which answer you are getting.
*
*   PRECONDITION:
*   TypeA and TypeB shall each be a complete type, cv void, or an array of
* unknown bound.  This mirrors std and cannot be enforced portably.
*
*
* path:      /inc/re_std/type_traits/is_layout_compatible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.12
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_LAYOUT_COMPATIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_LAYOUT_COMPATIBLE_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./type_traits.hpp"    // integral_constant, is_same, remove_cv


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// RE_STD_HAS_IS_LAYOUT_COMPATIBLE
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
#ifndef RE_STD_HAS_IS_LAYOUT_COMPATIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_layout_compatible)
            #define RE_STD_HAS_IS_LAYOUT_COMPATIBLE  1
        #endif
    #endif

    #ifndef RE_STD_HAS_IS_LAYOUT_COMPATIBLE
        #if ( defined(RE_STD_COMPILER_GCC) &&                                  \
              RE_STD_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define RE_STD_HAS_IS_LAYOUT_COMPATIBLE  1
        #elif ( defined(RE_STD_COMPILER_MSVC) &&                               \
                RE_STD_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define RE_STD_HAS_IS_LAYOUT_COMPATIBLE  1
        #else
            #define RE_STD_HAS_IS_LAYOUT_COMPATIBLE  0
        #endif
    #endif  // RE_STD_HAS_IS_LAYOUT_COMPATIBLE (fallback)
#endif  // RE_STD_HAS_IS_LAYOUT_COMPATIBLE (outer guard)


namespace re_std
{

namespace internal
{

    // is_layout_compatible_base
    //   trait: classification core for is_layout_compatible.  The builtin
    // already disregards cv-qualification on both operands, so the intrinsic
    // arm forwards its arguments untouched.
#if RE_STD_HAS_IS_LAYOUT_COMPATIBLE

    template<typename TypeA,
             typename TypeB>
    struct is_layout_compatible_base
        : integral_constant<bool, __is_layout_compatible(TypeA, TypeB)>
    {};

#else

    // is_layout_compatible_base (degraded)
    //   trait: sound-subset classification used when the builtin is absent.
    // Identical types (after cv-stripping) are layout-compatible by
    // definition; every other pair is reported false rather than guessed at.
    template<typename TypeA,
             typename TypeB>
    struct is_layout_compatible_base
        : is_same<typename remove_cv<TypeA>::type,
                  typename remove_cv<TypeB>::type>
    {};

#endif  // RE_STD_HAS_IS_LAYOUT_COMPATIBLE

}  // internal


// is_layout_compatible
//   trait: true if TypeA and TypeB are layout-compatible types.
template<typename TypeA,
         typename TypeB>
struct is_layout_compatible
    : internal::is_layout_compatible_base<TypeA, TypeB>
{};

// is_layout_compatible_v (C++14+)
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename TypeA,
             typename TypeB>
    RE_STD_CONSTEXPR bool is_layout_compatible_v
        = is_layout_compatible<TypeA, TypeB>::value;
#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_IS_LAYOUT_COMPATIBLE_HPP
