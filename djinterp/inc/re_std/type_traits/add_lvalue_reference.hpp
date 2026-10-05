/*******************************************************************************
* djinterp [re_std]                                     add_lvalue_reference.hpp
*
* add_lvalue_reference trait header:
*   Yields the lvalue-reference form of Type. If Type is `void` (any
* cv-qualification), the trait is a no-op and yields Type unchanged.
* If Type is already an lvalue or rvalue reference, reference collapsing
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
* other types use the primary template, where `Type&` is well-formed
* by the language rules.
*
*
* path:      /inc/re_std/type_traits/add_lvalue_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ADD_LVALUE_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_ADD_LVALUE_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   ADD_LVALUE_REFERENCE
// =============================================================================

// add_lvalue_reference
//   trait: yields Type& for any referenceable Type. Reference collapsing
// (C++11+) handles the case where Type is already a reference; pre-C++11
// the only reference form is lvalue, and `Type&` with Type = U& folds
// to U& by the deduction rules.
template<typename Type>
struct add_lvalue_reference
{
    typedef Type& type;
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

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // add_lvalue_reference_t
    //   alias: convenience alias for add_lvalue_reference<Type>::type.
    template<typename Type>
    using add_lvalue_reference_t =
        typename add_lvalue_reference<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_ADD_LVALUE_REFERENCE_HPP
