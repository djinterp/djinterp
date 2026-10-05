/*******************************************************************************
* djinterp [re_std]                      reference_constructs_from_temporary.hpp
*
* reference_constructs_from_temporary trait header:
*   dangling-reference detection (direct-initialization form):
*   `reference_constructs_from_temporary<Ref, Source>` reports whether, in
* `Ref r(e);` where e is an expression of type Source, r would be bound to a
* TEMPORARY that dies at the end of the full-expression - i.e. whether r
* dangles.  This is the P2255R2 trait that makes "does this reference outlive
* what it points at?" a compile-time question.
*
*   STD IS C++23; re_std IS C++98.
*   The builtin behind the trait is accepted in every language mode and yields
* a core constant expression at every tier, so the trait carries no language
* gate - only an intrinsic gate.  C++23 -> C++98 is a 25-year back-port.
*
*   READ THIS BEFORE USING: A NON-REFERENCE Source IS A PRVALUE.
*   Per [meta.unary.prop], the source expression has type Source exactly, so a
* non-reference Source denotes a PRVALUE - and binding any reference to a
* prvalue materialises a temporary.  Therefore:
*
*     reference_constructs_from_temporary<const int&, int>::value == true
*
* even though the types match.  That surprises nearly everyone the first time.
* Pass `int&` as Source to ask about binding to an lvalue.
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
* RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY lets callers guard:
*
*     #if RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
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
*   When Ref is an rvalue reference, or an lvalue reference to a const- but
* not volatile-qualified object type, both remove_reference<Ref>::type and
* remove_reference<Source>::type shall be complete types, cv void, or arrays
* of unknown bound.  Mirrors std; not portably enforceable.
*
*
* path:      /inc/re_std/type_traits/reference_constructs_from_temporary.hpp
*                                    reference_constructs_from_temporary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_HPP
#define RE_STD_TYPE_TRAITS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./type_traits.hpp"    // integral_constant


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
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
#ifndef RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
    #if defined(__has_builtin)
        #if __has_builtin(__reference_constructs_from_temporary)
            #define RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY  1
        #endif
    #endif

    #ifndef RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY
        #if ( defined(RE_STD_COMPILER_GCC) &&                                  \
              RE_STD_COMPILER_VERSION_AT_LEAST(13, 0, 0) )
            #define RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY  1
        #else
            #define RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY  0
        #endif
    #endif  // RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY (fallback)
#endif  // RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY (outer guard)


#if RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY

namespace re_std
{

// reference_constructs_from_temporary
//   trait: true if `Ref r(e);` binds r to a temporary, where e has type
// Source (a prvalue when Source is not a reference type).
template<typename Ref,
         typename Source>
struct reference_constructs_from_temporary
    : integral_constant<bool,
          __reference_constructs_from_temporary(Ref, Source)>
{};

// reference_constructs_from_temporary_v (C++14+)
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename Ref,
             typename Source>
    RE_STD_CONSTEXPR bool reference_constructs_from_temporary_v
        = reference_constructs_from_temporary<Ref, Source>::value;
#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std
#endif  // RE_STD_HAS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_REFERENCE_CONSTRUCTS_FROM_TEMPORARY_HPP
