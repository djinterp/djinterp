/******************************************************************************
* djinterp [restd]                                   add_rvalue_reference.hpp
*
* add_rvalue_reference trait header:
*   Yields the rvalue-reference form of _Type. If _Type is `void` (any
* cv-qualification), the trait is a no-op. If _Type is an lvalue
* reference, reference collapsing yields an lvalue reference (`U& &&`
* collapses to `U&`). If _Type is already an rvalue reference, it is
* yielded unchanged.
*
*     add_rvalue_reference<int>::type             -> int&&
*     add_rvalue_reference<int&>::type            -> int&     (collapse)
*     add_rvalue_reference<int&&>::type           -> int&&    (idempotent)
*     add_rvalue_reference<void>::type            -> void     (no-op)
*
*   PORTABILITY:
*   Rvalue references are a C++11 feature. On C++98/03 this trait is a
* pure passthrough -- it yields _Type unchanged for any input. This
* preserves compilation but is semantically degraded; calling code that
* requires an rvalue reference must itself be gated on
* D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*
*
* path:      /inc/djinterp/restd/type_traits/add_rvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ADD_RVALUE_REFERENCE_
#define DJINTERP_RESTD_TYPE_TRAITS_ADD_RVALUE_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   ADD_RVALUE_REFERENCE
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // add_rvalue_reference
    //   trait: yields _Type&& for referenceable _Type; reference collapsing
    // turns `U& &&` into `U&` and `U&& &&` into `U&&`.
    template<typename _Type>
    struct add_rvalue_reference
    {
        typedef _Type&& type;
    };

    // void specializations: `void&&` is ill-formed.

    template<>
    struct add_rvalue_reference<void>
    {
        typedef void type;
    };

    template<>
    struct add_rvalue_reference<const void>
    {
        typedef const void type;
    };

    template<>
    struct add_rvalue_reference<volatile void>
    {
        typedef volatile void type;
    };

    template<>
    struct add_rvalue_reference<const volatile void>
    {
        typedef const volatile void type;
    };

#else  // C++98/03: rvalue references unavailable. Passthrough.

    // add_rvalue_reference
    //   trait: passthrough on C++98/03 (rvalue references unavailable).
    template<typename _Type>
    struct add_rvalue_reference
    {
        typedef _Type type;
    };

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =============================================================================
// II.  ADD_RVALUE_REFERENCE_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // add_rvalue_reference_t
    //   alias: convenience alias for add_rvalue_reference<_Type>::type.
    template<typename _Type>
    using add_rvalue_reference_t =
        typename add_rvalue_reference<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ADD_RVALUE_REFERENCE_
