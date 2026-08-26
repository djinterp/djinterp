/******************************************************************************
* djinterp [re_std]                                        common_reference.hpp
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
* D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES (the trait's signature uses
* template-template parameters that take a single type and yield a type --
* that requires alias templates, since the qualifier-applying templates
* are typically alias templates). Standardized in C++20; re_std backports.
*
*   DEPENDENCIES:
*   common_type, basic_common_reference, decay, remove_reference,
* remove_cv, is_reference, is_convertible, void_t, re_std::declval.
*
*
* path:      /inc/djinterp/re_std/type_traits/common_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_COMMON_REFERENCE_
#define DJINTERP_RE_STD_TYPE_TRAITS_COMMON_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if    D_ENV_LANG_IS_CPP11_OR_HIGHER \
    && D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

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


NS_RESTD


    // common_reference
    //   trait: primary template -- has no `type` member. Defined with
    //          empty body for reliable SFINAE (see common_type.hpp for
    //          the same reasoning).
    template<typename... _Ts>
    struct common_reference
    {};


    NS_INTERNAL

        // copy_cv
        //   trait: yields _To with _From's cv-qualifiers applied. _From
        //          is expected to be a non-reference type. Used inside
        //          the COMMON-REF LL computation.
        template<typename _From, typename _To>
        struct copy_cv
        { typedef _To type; };

        template<typename _From, typename _To>
        struct copy_cv<const _From, _To>
        { typedef const _To type; };

        template<typename _From, typename _To>
        struct copy_cv<volatile _From, _To>
        { typedef volatile _To type; };

        template<typename _From, typename _To>
        struct copy_cv<const volatile _From, _To>
        { typedef const volatile _To type; };

        // cond_res
        //   trait: COND-RES(X, Y). Yields the type of the conditional
        //          expression `false ? <X-call>() : <Y-call>()` where
        //          each call expression has the precise value category
        //          and qualification of X / Y. The function-reference
        //          dance (`X(&)()`) is the standard's prescribed way to
        //          obtain such an expression.
        template<typename _X, typename _Y, typename = void>
        struct cond_res
        {};

        template<typename _X, typename _Y>
        struct cond_res<
            _X,
            _Y,
            re_std::void_t<decltype(
                false
                ? re_std::declval<_X(&)()>()()
                : re_std::declval<_Y(&)()>()() )> >
        {
            typedef decltype(
                false
                ? re_std::declval<_X(&)()>()()
                : re_std::declval<_Y(&)()>()() ) type;
        };

        // remove_cvref_local
        //   trait: internal equivalent of C++20's remove_cvref. Used by
        //          the basic_common_reference query at bullet 2. Inlined
        //          here (rather than depending on a public remove_cvref)
        //          because remove_cvref may not yet be ported.
        template<typename _T>
        struct remove_cvref_local
        {
            typedef typename remove_cv<
                typename remove_reference<_T>::type >::type type;
        };

        // xref
        //   trait: qualifier-reapplying template. xref<T>::apply<U>
        //          yields U with T's cv- and reference-qualifiers. For
        //          a non-reference cv-unqualified U.
        template<typename _T>
        struct xref
        { template<typename _U> using apply = _U; };

        template<typename _T>
        struct xref<const _T>
        { template<typename _U> using apply = const _U; };

        template<typename _T>
        struct xref<volatile _T>
        { template<typename _U> using apply = volatile _U; };

        template<typename _T>
        struct xref<const volatile _T>
        { template<typename _U> using apply = const volatile _U; };

        template<typename _T>
        struct xref<_T&>
        { template<typename _U>
          using apply = typename xref<_T>::template apply<_U>&; };

        template<typename _T>
        struct xref<_T&&>
        { template<typename _U>
          using apply = typename xref<_T>::template apply<_U>&&; };

        // ----- COMMON-REF(A, B) implementation -----------------------

        // common_ref_LL_inner
        //   trait: shared LL computation. Yields cond_res of the
        //          cv-merged lvalue-reference forms. Has `type` only
        //          when cond_res is well-formed (regardless of whether
        //          that type is a reference). The is_reference check
        //          is applied at the outer common_ref<_X&, _Y&> spec.
        template<typename _X, typename _Y, typename = void>
        struct common_ref_LL_inner
        {};

        template<typename _X, typename _Y>
        struct common_ref_LL_inner<
            _X,
            _Y,
            re_std::void_t<typename cond_res<
                typename copy_cv<_Y, _X>::type&,
                typename copy_cv<_X, _Y>::type& >::type> >
        {
            typedef typename cond_res<
                typename copy_cv<_Y, _X>::type&,
                typename copy_cv<_X, _Y>::type& >::type type;
        };

        // common_ref
        //   trait: primary; no `type` member. Specializations for the
        //          four reference-pattern cases (LL / RR / LR / RL)
        //          provide `type` when their respective COMMON-REF
        //          rule is well-formed.
        template<typename _A, typename _B, typename = void>
        struct common_ref
        {};

        // LL: both lvalue refs.
        //   COMMON-REF(X&, Y&) = cond_res<COPYCV(X,Y)&, COPYCV(Y,X)&>
        //   only if that result is itself a reference type.
        template<typename _X, typename _Y>
        struct common_ref<
            _X&,
            _Y&,
            typename enable_if<
                is_reference<
                    typename common_ref_LL_inner<_X, _Y>::type
                    >::value
                >::type>
        {
            typedef typename common_ref_LL_inner<_X, _Y>::type type;
        };

        // RR: both rvalue refs.
        //   C = remove_reference<COMMON-REF(X&, Y&)>::type&&
        //   only if X&& and Y&& are both convertible to C.
        template<typename _X, typename _Y>
        struct common_ref<
            _X&&,
            _Y&&,
            typename enable_if<
                (    is_reference<
                         typename common_ref_LL_inner<_X, _Y>::type
                         >::value
                  && is_convertible<
                         _X&&,
                         typename remove_reference<
                             typename common_ref_LL_inner<_X, _Y>::type
                             >::type&& >::value
                  && is_convertible<
                         _Y&&,
                         typename remove_reference<
                             typename common_ref_LL_inner<_X, _Y>::type
                             >::type&& >::value )
                >::type>
        {
            typedef typename remove_reference<
                typename common_ref_LL_inner<_X, _Y>::type
                >::type&& type;
        };

        // LR: A is rvalue ref, B is lvalue ref.
        //   D = COMMON-REF(const X&, Y&) = LL_inner<const X, Y>::type
        //   only if D is a reference and X&& is convertible to D.
        template<typename _X, typename _Y>
        struct common_ref<
            _X&&,
            _Y&,
            typename enable_if<
                (    is_reference<
                         typename common_ref_LL_inner<const _X, _Y>::type
                         >::value
                  && is_convertible<
                         _X&&,
                         typename common_ref_LL_inner<const _X, _Y>::type
                         >::value )
                >::type>
        {
            typedef typename common_ref_LL_inner<const _X, _Y>::type type;
        };

        // RL: A is lvalue ref, B is rvalue ref. Symmetric to LR --
        //   COMMON-REF(A, B) = COMMON-REF(B, A).
        template<typename _X, typename _Y>
        struct common_ref<
            _X&,
            _Y&&,
            typename enable_if<
                (    is_reference<
                         typename common_ref_LL_inner<const _Y, _X>::type
                         >::value
                  && is_convertible<
                         _Y&&,
                         typename common_ref_LL_inner<const _Y, _X>::type
                         >::value )
                >::type>
            : common_ref<_Y&&, _X&>
        {};

        // ----- 4-bullet fallback chain -------------------------------

        // common_reference_sub1
        //   trait: bullet 1 -- COMMON-REF if well-formed.
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_sub1
        {};

        template<typename _T1, typename _T2>
        struct common_reference_sub1<
            _T1, _T2,
            re_std::void_t<typename common_ref<_T1, _T2>::type> >
        {
            typedef typename common_ref<_T1, _T2>::type type;
        };

        // common_reference_sub2
        //   trait: bullet 2 -- basic_common_reference query.
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_sub2
        {};

        template<typename _T1, typename _T2>
        struct common_reference_sub2<
            _T1, _T2,
            re_std::void_t<typename basic_common_reference<
                typename remove_cvref_local<_T1>::type,
                typename remove_cvref_local<_T2>::type,
                xref<_T1>::template apply,
                xref<_T2>::template apply >::type> >
        {
            typedef typename basic_common_reference<
                typename remove_cvref_local<_T1>::type,
                typename remove_cvref_local<_T2>::type,
                xref<_T1>::template apply,
                xref<_T2>::template apply >::type type;
        };

        // common_reference_sub3
        //   trait: bullet 3 -- common_type fallback.
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_sub3
        {};

        template<typename _T1, typename _T2>
        struct common_reference_sub3<
            _T1, _T2,
            re_std::void_t<typename common_type<_T1, _T2>::type> >
        {
            typedef typename common_type<_T1, _T2>::type type;
        };

        // common_reference_sub4
        //   trait: bullet 4 -- COND-RES.
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_sub4
        {};

        template<typename _T1, typename _T2>
        struct common_reference_sub4<
            _T1, _T2,
            re_std::void_t<typename cond_res<_T1, _T2>::type> >
        {
            typedef typename cond_res<_T1, _T2>::type type;
        };

        // common_reference_2_4: chain link 4 (sub3 fallback to sub4)
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_2_4
            : common_reference_sub4<_T1, _T2>
        {};

        template<typename _T1, typename _T2>
        struct common_reference_2_4<
            _T1, _T2,
            re_std::void_t<typename common_reference_sub3<_T1, _T2>::type> >
            : common_reference_sub3<_T1, _T2>
        {};

        // common_reference_2_3: chain link 3 (sub2 fallback to 2_4)
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_2_3
            : common_reference_2_4<_T1, _T2>
        {};

        template<typename _T1, typename _T2>
        struct common_reference_2_3<
            _T1, _T2,
            re_std::void_t<typename common_reference_sub2<_T1, _T2>::type> >
            : common_reference_sub2<_T1, _T2>
        {};

        // common_reference_2_2: chain link 2 (sub1 fallback to 2_3)
        template<typename _T1, typename _T2, typename = void>
        struct common_reference_2_2
            : common_reference_2_3<_T1, _T2>
        {};

        template<typename _T1, typename _T2>
        struct common_reference_2_2<
            _T1, _T2,
            re_std::void_t<typename common_reference_sub1<_T1, _T2>::type> >
            : common_reference_sub1<_T1, _T2>
        {};

        // common_reference_n_impl
        //   trait: SFINAE-friendly recursive case for n >= 3 args.
        //          Mirrors common_type_n_impl in shape.
        template<typename _Void, typename _CR, typename... _Rest>
        struct common_reference_n_impl
        {};

        template<typename _CR, typename... _Rest>
        struct common_reference_n_impl<
            re_std::void_t<typename _CR::type>,
            _CR,
            _Rest...>
            : common_reference<typename _CR::type, _Rest...>
        {};

    NS_END  // internal


    // common_reference<_T>
    //   trait: 1-arg case; type = T (no decay).
    template<typename _T>
    struct common_reference<_T>
    {
        typedef _T type;
    };

    // common_reference<_T1, _T2>
    //   trait: binary case; runs the 4-bullet fallback chain.
    template<typename _T1, typename _T2>
    struct common_reference<_T1, _T2>
        : internal::common_reference_2_2<_T1, _T2>
    {};

    // common_reference<_T1, _T2, _R...>
    //   trait: n-arg case (n >= 3 by partial ordering).
    template<typename _T1,
             typename _T2,
             typename... _R>
    struct common_reference<_T1, _T2, _R...>
        : internal::common_reference_n_impl<
              void,
              common_reference<_T1, _T2>,
              _R... >
    {};


    // common_reference_t
    //   alias: type alias for the trait. Always available because the
    //          enclosing file is gated on alias-templates support.
    template<typename... _Ts>
    using common_reference_t = typename common_reference<_Ts...>::type;


NS_END  // re_std


#endif  // CPP11+ && ALIAS_TEMPLATES

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_COMMON_REFERENCE_
