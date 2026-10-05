/*******************************************************************************
* djinterp [re_std]                        reference_converts_from_temporary.hpp
*
* reference_converts_from_temporary trait header:
*   dangling-reference detection (copy-initialization form):
*   `reference_converts_from_temporary<Ref, Source>` reports whether, in
* `Ref r = e;` where e is an expression of type Source, r would be bound to a
* TEMPORARY that dies at the end of the full-expression.  Sibling of
* reference_constructs_from_temporary; the only difference is that the
* reference is COPY-initialized here rather than direct-initialized.
*
*   STD IS C++23; re_std IS C++98 - a 25-year back-port, on the same reasoning
* as its sibling: the builtin is accepted in every language mode and is a core
* constant expression at every tier, so there is no language gate.
*
*   WHEN DOES THIS ACTUALLY DIFFER FROM THE _constructs_ FORM?
*   Less often than the two names suggest, and it is worth knowing why.  Under
* [dcl.init.ref] the temporary bound to a reference is ALWAYS copy-initialized,
* even when the reference itself is direct-initialized - so an `explicit`
* constructor on the target type is non-viable for BOTH traits.  Verified on
* GCC 13.3: `struct C { explicit C(int); };` gives false from both, and an
* explicit conversion FUNCTION on the source gives false from both.  Every case
* in re_std's test matrix agrees between the two.
*
*   Ship both anyway.  They are separately specified, separately named in std,
* and separately spelled by the compiler; a caller reaching for the
* copy-initialization form should find it rather than be told to use the other
* one and hope the distinction never bites.
*
*   OMITTED, NOT DEGRADED, WHEN THE BUILTIN IS ABSENT.
*   Same inverted polarity as its sibling: this is a hazard detector, so a
* degraded `false` would silently report "no dangling reference" on a compiler
* that cannot tell, disarming the check.  When the builtin is missing the trait
* is NOT DECLARED - test
* RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY before naming it.  See
* reference_constructs_from_temporary.hpp for the full argument, including why
* Clang's older __reference_binds_to_temporary is deliberately NOT used as a
* fallback (P2255R2 records it as a partial implementation that misses the
* prvalue case, which would make it a false-negative source).
*
*   THE PRVALUE GOTCHA APPLIES HERE TOO:
*     reference_converts_from_temporary<const int&, int>::value == true
* because a non-reference Source denotes a prvalue.  Pass `int&` to ask about
* binding to an lvalue.
*
*   PRECONDITION:
*   As reference_constructs_from_temporary - completeness of
* remove_reference<Ref>::type and remove_reference<Source>::type in the
* rvalue-reference and const-lvalue-reference cases.  Mirrors std.
*
*
* path:      /inc/re_std/type_traits/reference_converts_from_temporary.hpp
*                                      reference_converts_from_temporary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_REFERENCE_CONVERTS_FROM_TEMPORARY_HPP
#define RE_STD_TYPE_TRAITS_REFERENCE_CONVERTS_FROM_TEMPORARY_HPP 1

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

// RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY
//   constant: 1 if the __reference_converts_from_temporary builtin is
// available.  When 0, re_std::reference_converts_from_temporary DOES NOT EXIST.
//
//   Detected independently of the _constructs_ sibling even though GCC shipped
// both in 13 and both come from one paper - the corpus rule is one macro per
// symbol, and Clang has already demonstrated that P2255R2's two builtins can
// diverge in behaviour (LLVM issue #114344).  Only a GCC floor is asserted
// behind __has_builtin, for the same reason as the sibling.
#ifndef RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY
    #if defined(__has_builtin)
        #if __has_builtin(__reference_converts_from_temporary)
            #define RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY  1
        #endif
    #endif

    #ifndef RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY
        #if ( defined(RE_STD_COMPILER_GCC) &&                                  \
              RE_STD_COMPILER_VERSION_AT_LEAST(13, 0, 0) )
            #define RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY  1
        #else
            #define RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY  0
        #endif
    #endif  // RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY (fallback)
#endif  // RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY (outer guard)


#if RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY

namespace re_std
{

// reference_converts_from_temporary
//   trait: true if `Ref r = e;` binds r to a temporary, where e has type
// Source (a prvalue when Source is not a reference type).
template<typename Ref,
         typename Source>
struct reference_converts_from_temporary
    : integral_constant<bool,
          __reference_converts_from_temporary(Ref, Source)>
{};

// reference_converts_from_temporary_v (C++14+)
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename Ref,
             typename Source>
    RE_STD_CONSTEXPR bool reference_converts_from_temporary_v
        = reference_converts_from_temporary<Ref, Source>::value;
#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std
#endif  // RE_STD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_REFERENCE_CONVERTS_FROM_TEMPORARY_HPP
