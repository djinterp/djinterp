/******************************************************************************
* djinterp [restd]                                   add_lvalue_reference.hpp
*
* add_lvalue_reference trait header:
*   Yields the lvalue-reference form of _Type. If _Type is `void` (any
* cv-qualification), the trait is a no-op and yields _Type unchanged.
* If _Type is already an lvalue or rvalue reference, reference collapsing
* (C++11+) or natural template-argument substitution (C++98/03) yields
* the appropriate lvalue reference.
*
*     add_lvalue_reference<int>::type             -> int&
*     add_lvalue_reference<int&>::type            -> int&     (idempotent)
*     add_lvalue_reference<int&&>::type           -> int&     (ref collapse, C++11+)
*     add_lvalue_reference<void>::type            -> void     (no-op)
*     add_lvalue_reference<const void>::type      -> const void
*
*   PORTABILITY:
*   The four cv-qualified forms of `void` are handled by explicit
* specializations to avoid forming the ill-formed type `void&`. All
* other types use the primary template, where `_Type&` is well-formed
* by the language rules.
*
*
* path:      /inc/djinterp/restd/type_traits/add_lvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ADD_LVALUE_REFERENCE_
#define DJINTERP_RESTD_TYPE_TRAITS_ADD_LVALUE_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   ADD_LVALUE_REFERENCE
// =============================================================================

// add_lvalue_reference
//   trait: yields _Type& for any referenceable _Type. Reference collapsing
// (C++11+) handles the case where _Type is already a reference; pre-C++11
// the only reference form is lvalue, and `_Type&` with _Type = U& folds
// to U& by the deduction rules.
template<typename _Type>
struct add_lvalue_reference
{
    typedef _Type& type;
};

// void specializations: forming `void&` is ill-formed, so the four
// cv-qualified flavors of void are explicitly mapped to themselves.

template<>
struct add_lvalue_reference<void>
{
    typedef void type;
};

template<>
struct add_lvalue_reference<const void>
{
    typedef const void type;
};

template<>
struct add_lvalue_reference<volatile void>
{
    typedef volatile void type;
};

template<>
struct add_lvalue_reference<const volatile void>
{
    typedef const volatile void type;
};


// =============================================================================
// II.  ADD_LVALUE_REFERENCE_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // add_lvalue_reference_t
    //   alias: convenience alias for add_lvalue_reference<_Type>::type.
    template<typename _Type>
    using add_lvalue_reference_t =
        typename add_lvalue_reference<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ADD_LVALUE_REFERENCE_
