/******************************************************************************
* re_std [type_traits]                  reference_constructs_from_temporary.hpp
*
*   dangling-reference detection (direct-initialization form):
*   `reference_constructs_from_temporary<_Ref, _Source>` reports whether, in
* `_Ref r(e);` where e is an expression of type _Source, r would be bound to a
* TEMPORARY that dies at the end of the full-expression - i.e. whether r
* dangles.  This is the P2255R2 trait that makes "does this reference outlive
* what it points at?" a compile-time question.
*
*   STD IS C++23; re_std IS C++98.
*   The builtin behind the trait is accepted in every language mode and yields
* a core constant expression at every tier, so the trait carries no language
* gate - only an intrinsic gate.  C++23 -> C++98 is a 25-year back-port.
*
*   READ THIS BEFORE USING: A NON-REFERENCE _Source IS A PRVALUE.
*   Per [meta.unary.prop], the source expression has type _Source exactly, so a
* non-reference _Source denotes a PRVALUE - and binding any reference to a
* prvalue materialises a temporary.  Therefore:
*
*     reference_constructs_from_temporary<const int&, int>::value == true
*
* even though the types match.  That surprises nearly everyone the first time.
* Pass `int&` as _Source to ask about binding to an lvalue.
*
*   THIS SYMBOL IS OMITTED WHEN THE BUILTIN IS ABSENT - IT DOES NOT DEGRADE.
*   Every other intrinsic-backed trait in re_std degrades to a conservative
* answer, because for those (is_layout_compatible, is_class, ...) FALSE is the
* safe direction: under-reporting only refuses to authorise something.
*
*   Here the polarity is INVERTED.  This trait is a hazard detector, used as
* `static_assert(!reference_constructs_from_temporary<T, U>::value)`.  A
* degraded `false` would not be conservative - it would silently report "no
* dangling reference here" on a compiler that cannot actually tell, quietly
* disarming the check and shipping the bug.  A degraded `true` is sound but
* useless: it fails every such assertion everywhere.
*
*   So when the builtin is missing the trait is NOT DECLARED AT ALL.  Naming it
* is then a clear, immediate, localised compile error at the point of use
* rather than a silent wrong answer, and
* D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY lets callers guard:
*
*     #if D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
*         static_assert(!re_std::reference_constructs_from_temporary<T, U>::value,
*                       "would dangle");
*     #endif
*
* This is the project's "omit" path, not an #error - re_std still compiles.
*
*   WHY NOT FALL BACK ON __reference_binds_to_temporary?
*   Clang carries that older intrinsic, and it is tempting as a fallback.  It
* must not be used: P2255R2 records that it only PARTIALLY implements the
* direct-initialization variant and specifically does NOT handle binding to a
* prvalue of the same or derived type.  That is precisely the case above - so
* it would answer `false` for `<const int&, int>`, a FALSE NEGATIVE in a
* hazard detector.  A partial hazard detector is worse than an absent one.
*
*   PRECONDITION:
*   When _Ref is an rvalue reference, or an lvalue reference to a const- but
* not volatile-qualified object type, both remove_reference<_Ref>::type and
* remove_reference<_Source>::type shall be complete types, cv void, or arrays
* of unknown bound.  Mirrors std; not portably enforceable.
*
*
* path:      /inc/djinterp/re_std/type_traits/
*                                    reference_constructs_from_temporary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_TYPE_TRAITS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_
#define RESTD_TYPE_TRAITS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_ 1

// re_std
#include "./type_traits.hpp"    // integral_constant


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
//   constant: 1 if the __reference_constructs_from_temporary builtin is
// available.  When 0, re_std::reference_constructs_from_temporary DOES NOT
// EXIST - test this macro before naming the trait.
//
//   __has_builtin is the primary probe.  Only a GCC floor is asserted behind
// it: GCC gained both P2255R2 builtins in 13.  No Clang or MSVC version arm is
// claimed, because Clang's implementation has documented divergences (LLVM
// issue #114344, function types) and asserting a floor for a compiler whose
// behaviour has not been verified here would turn a safe omission into a
// wrong answer.  __has_builtin answers correctly for both.
#ifndef D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
    #if defined(__has_builtin)
        #if __has_builtin(__reference_constructs_from_temporary)
            #define D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY  1
        #endif
    #endif

    #ifndef D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
        #if ( defined(D_ENV_COMPILER_GCC) &&                                  \
              D_ENV_COMPILER_VERSION_AT_LEAST(13, 0, 0) )
            #define D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY  1
        #else
            #define D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY  0
        #endif
    #endif  // D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY (fallback)
#endif  // D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY (outer guard)


#if D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY

NS_DJINTERP
NS_RESTD

// reference_constructs_from_temporary
//   trait: true if `_Ref r(e);` binds r to a temporary, where e has type
// _Source (a prvalue when _Source is not a reference type).
template<typename _Ref,
         typename _Source>
struct reference_constructs_from_temporary
    : integral_constant<bool,
          __reference_constructs_from_temporary(_Ref, _Source)>
{};

// reference_constructs_from_temporary_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Ref,
             typename _Source>
    D_CONSTEXPR bool reference_constructs_from_temporary_v
        = reference_constructs_from_temporary<_Ref, _Source>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // re_std
NS_END  // djinterp

#endif  // D_RESTD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY

#endif  // RESTD_TYPE_TRAITS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_
