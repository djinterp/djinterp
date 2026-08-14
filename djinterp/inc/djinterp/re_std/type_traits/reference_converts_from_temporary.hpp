/******************************************************************************
* re_std [type_traits]                    reference_converts_from_temporary.hpp
*
*   dangling-reference detection (copy-initialization form):
*   `reference_converts_from_temporary<_Ref, _Source>` reports whether, in
* `_Ref r = e;` where e is an expression of type _Source, r would be bound to a
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
* D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY before naming it.  See
* reference_constructs_from_temporary.hpp for the full argument, including why
* Clang's older __reference_binds_to_temporary is deliberately NOT used as a
* fallback (P2255R2 records it as a partial implementation that misses the
* prvalue case, which would make it a false-negative source).
*
*   THE PRVALUE GOTCHA APPLIES HERE TOO:
*     reference_converts_from_temporary<const int&, int>::value == true
* because a non-reference _Source denotes a prvalue.  Pass `int&` to ask about
* binding to an lvalue.
*
*   PRECONDITION:
*   As reference_constructs_from_temporary - completeness of
* remove_reference<_Ref>::type and remove_reference<_Source>::type in the
* rvalue-reference and const-lvalue-reference cases.  Mirrors std.
*
*
* path:      /inc/djinterp/re_std/type_traits/
*                                      reference_converts_from_temporary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_TYPE_TRAITS_REFERENCE_CONVERTS_FROM_TEMPORARY_
#define RESTD_TYPE_TRAITS_REFERENCE_CONVERTS_FROM_TEMPORARY_ 1

// re_std
#include "./type_traits.hpp"    // integral_constant


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY
//   constant: 1 if the __reference_converts_from_temporary builtin is
// available.  When 0, re_std::reference_converts_from_temporary DOES NOT EXIST.
//
//   Detected independently of the _constructs_ sibling even though GCC shipped
// both in 13 and both come from one paper - the corpus rule is one macro per
// symbol, and Clang has already demonstrated that P2255R2's two builtins can
// diverge in behaviour (LLVM issue #114344).  Only a GCC floor is asserted
// behind __has_builtin, for the same reason as the sibling.
#ifndef D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY
    #if defined(__has_builtin)
        #if __has_builtin(__reference_converts_from_temporary)
            #define D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY  1
        #endif
    #endif

    #ifndef D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY
        #if ( defined(D_ENV_COMPILER_GCC) &&                                  \
              D_ENV_COMPILER_VERSION_AT_LEAST(13, 0, 0) )
            #define D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY  1
        #else
            #define D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY  0
        #endif
    #endif  // D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY (fallback)
#endif  // D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY (outer guard)


#if D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY

NS_DJINTERP
NS_RESTD

// reference_converts_from_temporary
//   trait: true if `_Ref r = e;` binds r to a temporary, where e has type
// _Source (a prvalue when _Source is not a reference type).
template<typename _Ref,
         typename _Source>
struct reference_converts_from_temporary
    : integral_constant<bool,
          __reference_converts_from_temporary(_Ref, _Source)>
{};

// reference_converts_from_temporary_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Ref,
             typename _Source>
    D_CONSTEXPR bool reference_converts_from_temporary_v
        = reference_converts_from_temporary<_Ref, _Source>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // re_std
NS_END  // djinterp

#endif  // D_RESTD_HAS_REFERENCE_CONVERTS_FROM_TEMPORARY

#endif  // RESTD_TYPE_TRAITS_REFERENCE_CONVERTS_FROM_TEMPORARY_
