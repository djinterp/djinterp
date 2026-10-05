/*******************************************************************************
* djinterp [re_std]                                         common_reference.hpp
*
* common_reference trait:
*   The C++20 generalization of common_type that preserves reference
* qualifiers when they are compatible. Where common_type<int&, int&> gives
* `int` (decayed), common_reference<int&, int&> gives `int&`. Used by
* ranges-style code that needs to express "a reference type compatible
* with both inputs."
*
*   Defined recursively just like common_type:
*     common_reference<>           -- no `type` member
*     common_reference<T>          -- type = T (no decay)
*     common_reference<T1, T2>     -- the 4-bullet chain (see below)
*     common_reference<T1, T2, R...> -- recursive on common_reference<...,R>
*
*   THE 4-BULLET CHAIN (binary case):
*     1. If both T1 and T2 are reference types and COMMON-REF(T1, T2) is
*        well-formed, the result is COMMON-REF(T1, T2).
*     2. Otherwise, if basic_common_reference<remove_cvref<T1>,
*        remove_cvref<T2>, XREF<T1>, XREF<T2>>::type is well-formed, that
*        is the result. (XREF<T> is a qualifier-reapplying alias template.)
*     3. Otherwise, if common_type<T1, T2>::type is well-formed, that is
*        the result.
*     4. Otherwise, if COND-RES(T1, T2) is well-formed, that is the result.
*     5. Otherwise, no `type` member is defined.
*
*   COMMON-REF(A, B) is itself a 4-case dispatcher on the value categories
* of A and B (LL, RR, LR, RL), implemented as four partial specializations
* of internal::common_ref. The primary common_ref template has no `type`
* member, so non-matching cases SFINAE-fall through to bullet 2.
*
*   COND-RES(X, Y) is the type of `false ? <X-returning-call>() :
* <Y-returning-call>()`, where the call expressions use function-reference
* indirection to preserve the precise value category of X and Y. This is
* not the same as a naive `decltype(false ? declval<X>() : declval<Y>())`
* -- declval always returns rvalue references, which would distort the
* conditional-expression result.
*
*   PORTABILITY:
*   Available on C++11 and later, gated on
* RE_STD_LANG_HAS_ALIAS_TEMPLATES (the trait's signature uses
* template-template parameters that take a single type and yield a type --
* that requires alias templates, since the qualifier-applying templates
* are typically alias templates). Standardized in C++20; re_std backports.
*
*   DEPENDENCIES:
*   common_type, basic_common_reference, decay, remove_reference,
* remove_cv, is_reference, is_convertible, void_t, re_std::declval.
*
*
* path:      /inc/re_std/type_traits/common_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_COMMON_REFERENCE_HPP
#define RE_STD_TYPE_TRAITS_COMMON_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if    RE_STD_LANG_IS_CPP11_OR_HIGHER \
    && RE_STD_LANG_HAS_ALIAS_TEMPLATES

// re_std
#include "./common_type.hpp"
#include "./basic_common_reference.hpp"
#include "./decay.hpp"
#include "./remove_reference.hpp"
#include "./remove_cv.hpp"
#include "./is_reference.hpp"
#include "./is_convertible.hpp"
#include "./void_t.hpp"
#include "./enable_if.hpp"
#include "../utility/declval.hpp"


namespace re_std
{


    // common_reference
    //   trait: primary template -- has no `type` member. Defined with
    //          empty body for reliable SFINAE (see common_type.hpp for
    //          the same reasoning).
    template<typename... Ts>
    struct common_reference
    {};


    namespace internal
    {

        // copy_cv
        //   trait: yields To with From's cv-qualifiers applied. From
        //          is expected to be a non-reference type. Used inside
        //          the COMMON-REF LL computation.
        template<typename From, typename To>
        struct copy_cv
        { typedef To type; };

        template<typename From, typename To>
        struct copy_cv<const From, To>
        { typedef const To type; };

        template<typename From, typename To>
        struct copy_cv<volatile From, To>
        { typedef volatile To type; };

        template<typename From, typename To>
        struct copy_cv<const volatile From, To>
        { typedef const volatile To type; };

        // cond_res
        //   trait: COND-RES(X, Y). Yields the type of the conditional
        //          expression `false ? <X-call>() : <Y-call>()` where
        //          each call expression has the precise value category
        //          and qualification of X / Y. The function-reference
        //          dance (`X(&)()`) is the standard's prescribed way to
        //          obtain such an expression.
        template<typename X, typename Y, typename = void>
        struct cond_res
        {};

        template<typename X, typename Y>
        struct cond_res<
            X,
            Y,
            re_std::void_t<decltype(
                false
                ? re_std::declval<X(&)()>()()
                : re_std::declval<Y(&)()>()() )> >
        {
            typedef decltype(
                false
                ? re_std::declval<X(&)()>()()
                : re_std::declval<Y(&)()>()() ) type;
        };

        // remove_cvref_local
        //   trait: internal equivalent of C++20's remove_cvref. Used by
        //          the basic_common_reference query at bullet 2. Inlined
        //          here (rather than depending on a public remove_cvref)
        //          because remove_cvref may not yet be ported.
        template<typename T>
        struct remove_cvref_local
        {
            typedef typename remove_cv<
                typename remove_reference<T>::type >::type type;
        };

        // xref
        //   trait: qualifier-reapplying template. xref<T>::apply<U>
        //          yields U with T's cv- and reference-qualifiers. For
        //          a non-reference cv-unqualified U.
        template<typename T>
        struct xref
        { template<typename U> using apply = U; };

        template<typename T>
        struct xref<const T>
        { template<typename U> using apply = const U; };

        template<typename T>
        struct xref<volatile T>
        { template<typename U> using apply = volatile U; };

        template<typename T>
        struct xref<const volatile T>
        { template<typename U> using apply = const volatile U; };

        template<typename T>
        struct xref<T&>
        { template<typename U>
          using apply = typename xref<T>::template apply<U>&; };

        template<typename T>
        struct xref<T&&>
        { template<typename U>
          using apply = typename xref<T>::template apply<U>&&; };

        // ----- COMMON-REF(A, B) implementation -----------------------

        // common_ref_LL_inner
        //   trait: shared LL computation. Yields cond_res of the
        //          cv-merged lvalue-reference forms. Has `type` only
        //          when cond_res is well-formed (regardless of whether
        //          that type is a reference). The is_reference check
        //          is applied at the outer common_ref<X&, Y&> spec.
        template<typename X, typename Y, typename = void>
        struct common_ref_LL_inner
        {};

        template<typename X, typename Y>
        struct common_ref_LL_inner<
            X,
            Y,
            re_std::void_t<typename cond_res<
                typename copy_cv<Y, X>::type&,
                typename copy_cv<X, Y>::type& >::type> >
        {
            typedef typename cond_res<
                typename copy_cv<Y, X>::type&,
                typename copy_cv<X, Y>::type& >::type type;
        };

        // common_ref
        //   trait: primary; no `type` member. Specializations for the
        //          four reference-pattern cases (LL / RR / LR / RL)
        //          provide `type` when their respective COMMON-REF
        //          rule is well-formed.
        template<typename A, typename B, typename = void>
        struct common_ref
        {};

        // LL: both lvalue refs.
        //   COMMON-REF(X&, Y&) = cond_res<COPYCV(X,Y)&, COPYCV(Y,X)&>
        //   only if that result is itself a reference type.
        template<typename X, typename Y>
        struct common_ref<
            X&,
            Y&,
            typename enable_if<
                is_reference<
                    typename common_ref_LL_inner<X, Y>::type
                    >::value
                >::type>
        {
            typedef typename common_ref_LL_inner<X, Y>::type type;
        };

        // RR: both rvalue refs.
        //   C = remove_reference<COMMON-REF(X&, Y&)>::type&&
        //   only if X&& and Y&& are both convertible to C.
        template<typename X, typename Y>
        struct common_ref<
            X&&,
            Y&&,
            typename enable_if<
                (    is_reference<
                         typename common_ref_LL_inner<X, Y>::type
                         >::value
                  && is_convertible<
                         X&&,
                         typename remove_reference<
                             typename common_ref_LL_inner<X, Y>::type
                             >::type&& >::value
                  && is_convertible<
                         Y&&,
                         typename remove_reference<
                             typename common_ref_LL_inner<X, Y>::type
                             >::type&& >::value )
                >::type>
        {
            typedef typename remove_reference<
                typename common_ref_LL_inner<X, Y>::type
                >::type&& type;
        };

        // LR: A is rvalue ref, B is lvalue ref.
        //   D = COMMON-REF(const X&, Y&) = LL_inner<const X, Y>::type
        //   only if D is a reference and X&& is convertible to D.
        template<typename X, typename Y>
        struct common_ref<
            X&&,
            Y&,
            typename enable_if<
                (    is_reference<
                         typename common_ref_LL_inner<const X, Y>::type
                         >::value
                  && is_convertible<
                         X&&,
                         typename common_ref_LL_inner<const X, Y>::type
                         >::value )
                >::type>
        {
            typedef typename common_ref_LL_inner<const X, Y>::type type;
        };

        // RL: A is lvalue ref, B is rvalue ref. Symmetric to LR --
        //   COMMON-REF(A, B) = COMMON-REF(B, A).
        template<typename X, typename Y>
        struct common_ref<
            X&,
            Y&&,
            typename enable_if<
                (    is_reference<
                         typename common_ref_LL_inner<const Y, X>::type
                         >::value
                  && is_convertible<
                         Y&&,
                         typename common_ref_LL_inner<const Y, X>::type
                         >::value )
                >::type>
            : common_ref<Y&&, X&>
        {};

        // ----- 4-bullet fallback chain -------------------------------

        // common_reference_sub1
        //   trait: bullet 1 -- COMMON-REF if well-formed.
        template<typename T1, typename T2, typename = void>
        struct common_reference_sub1
        {};

        template<typename T1, typename T2>
        struct common_reference_sub1<
            T1, T2,
            re_std::void_t<typename common_ref<T1, T2>::type> >
        {
            typedef typename common_ref<T1, T2>::type type;
        };

        // common_reference_sub2
        //   trait: bullet 2 -- basic_common_reference query.
        template<typename T1, typename T2, typename = void>
        struct common_reference_sub2
        {};

        template<typename T1, typename T2>
        struct common_reference_sub2<
            T1, T2,
            re_std::void_t<typename basic_common_reference<
                typename remove_cvref_local<T1>::type,
                typename remove_cvref_local<T2>::type,
                xref<T1>::template apply,
                xref<T2>::template apply >::type> >
        {
            typedef typename basic_common_reference<
                typename remove_cvref_local<T1>::type,
                typename remove_cvref_local<T2>::type,
                xref<T1>::template apply,
                xref<T2>::template apply >::type type;
        };

        // common_reference_sub3
        //   trait: bullet 3 -- common_type fallback.
        template<typename T1, typename T2, typename = void>
        struct common_reference_sub3
        {};

        template<typename T1, typename T2>
        struct common_reference_sub3<
            T1, T2,
            re_std::void_t<typename common_type<T1, T2>::type> >
        {
            typedef typename common_type<T1, T2>::type type;
        };

        // common_reference_sub4
        //   trait: bullet 4 -- COND-RES.
        template<typename T1, typename T2, typename = void>
        struct common_reference_sub4
        {};

        template<typename T1, typename T2>
        struct common_reference_sub4<
            T1, T2,
            re_std::void_t<typename cond_res<T1, T2>::type> >
        {
            typedef typename cond_res<T1, T2>::type type;
        };

        // common_reference_2_4: chain link 4 (sub3 fallback to sub4)
        template<typename T1, typename T2, typename = void>
        struct common_reference_2_4
            : common_reference_sub4<T1, T2>
        {};

        template<typename T1, typename T2>
        struct common_reference_2_4<
            T1, T2,
            re_std::void_t<typename common_reference_sub3<T1, T2>::type> >
            : common_reference_sub3<T1, T2>
        {};

        // common_reference_2_3: chain link 3 (sub2 fallback to 2_4)
        template<typename T1, typename T2, typename = void>
        struct common_reference_2_3
            : common_reference_2_4<T1, T2>
        {};

        template<typename T1, typename T2>
        struct common_reference_2_3<
            T1, T2,
            re_std::void_t<typename common_reference_sub2<T1, T2>::type> >
            : common_reference_sub2<T1, T2>
        {};

        // common_reference_2_2: chain link 2 (sub1 fallback to 2_3)
        template<typename T1, typename T2, typename = void>
        struct common_reference_2_2
            : common_reference_2_3<T1, T2>
        {};

        template<typename T1, typename T2>
        struct common_reference_2_2<
            T1, T2,
            re_std::void_t<typename common_reference_sub1<T1, T2>::type> >
            : common_reference_sub1<T1, T2>
        {};

        // common_reference_n_impl
        //   trait: SFINAE-friendly recursive case for n >= 3 args.
        //          Mirrors common_type_n_impl in shape.
        template<typename Void, typename CR, typename... Rest>
        struct common_reference_n_impl
        {};

        template<typename CR, typename... Rest>
        struct common_reference_n_impl<
            re_std::void_t<typename CR::type>,
            CR,
            Rest...>
            : common_reference<typename CR::type, Rest...>
        {};

    }  // internal


    // common_reference<T>
    //   trait: 1-arg case; type = T (no decay).
    template<typename T>
    struct common_reference<T>
    {
        typedef T type;
    };

    // common_reference<T1, T2>
    //   trait: binary case; runs the 4-bullet fallback chain.
    template<typename T1, typename T2>
    struct common_reference<T1, T2>
        : internal::common_reference_2_2<T1, T2>
    {};

    // common_reference<T1, T2, R...>
    //   trait: n-arg case (n >= 3 by partial ordering).
    template<typename T1,
             typename T2,
             typename... R>
    struct common_reference<T1, T2, R...>
        : internal::common_reference_n_impl<
              void,
              common_reference<T1, T2>,
              R... >
    {};


    // common_reference_t
    //   alias: type alias for the trait. Always available because the
    //          enclosing file is gated on alias-templates support.
    template<typename... Ts>
    using common_reference_t = typename common_reference<Ts...>::type;


}  // re_std


#endif  // CPP11+ && ALIAS_TEMPLATES

#endif  // RE_STD_TYPE_TRAITS_COMMON_REFERENCE_HPP
