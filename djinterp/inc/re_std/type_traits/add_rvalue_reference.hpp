/*******************************************************************************
* djinterp [re_std]                                     add_rvalue_reference.hpp
*
* add_rvalue_reference trait header:
*   Yields the rvalue-reference form of Type. If Type is `void` (any
* cv-qualification), the trait is a no-op. If Type is an lvalue
* reference, reference collapsing yields an lvalue reference (`U& &&`
* collapses to `U&`). If Type is already an rvalue reference, it is
* yielded unchanged.
*
*     add_rvalue_reference<int>::type             -> int&&
*     add_rvalue_reference<int&>::type            -> int&     (collapse)
*     add_rvalue_reference<int&&>::type           -> int&&    (idempotent)
*     add_rvalue_reference<void>::type            -> void     (no-op)
*
*   PORTABILITY:
*   Rvalue references are a C++11 feature. On C++98/03 this trait is a
* pure passthrough -- it yields Type unchanged for any input. This
* preserves compilation but is semantically degraded; calling code that
* requires an rvalue reference must itself be gated on
* RE_STD_LANG_HAS_RVALUE_REFERENCES.
*
*
* path:      /inc/re_std/type_traits/add_rvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ADD_RVALUE_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_ADD_RVALUE_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   ADD_RVALUE_REFERENCE
// =============================================================================

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    // add_rvalue_reference
    //   trait: yields Type&& for referenceable Type; reference collapsing
    // turns `U& &&` into `U&` and `U&& &&` into `U&&`.
    template<typename Type>
    struct add_rvalue_reference
    {
        typedef Type&& type;
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
    template<typename Type>
    struct add_rvalue_reference
    {
        typedef Type type;
    };

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


// =============================================================================
// II.  ADD_RVALUE_REFERENCE_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // add_rvalue_reference_t
    //   alias: convenience alias for add_rvalue_reference<Type>::type.
    template<typename Type>
    using add_rvalue_reference_t =
        typename add_rvalue_reference<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ADD_RVALUE_REFERENCE_HPP
