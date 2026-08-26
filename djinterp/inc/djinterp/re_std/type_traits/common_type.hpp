/******************************************************************************
* djinterp [re_std]                                             common_type.hpp
*
* common_type trait:
*   The "type all of the inputs share" computation, used most prominently
* by std::min, std::max, and ternary-like generic code. Defined recursively:
*     common_type<>          -- no `type` member
*     common_type<T>         -- same as common_type<T, T>
*     common_type<T1, T2>    -- the rule (see below)
*     common_type<T1, T2, R...> -- common_type<common_type<T1,T2>::type, R...>
*
*   THE RULE (binary base case):
*   If T1 and T2 are both already "decayed" (i.e. is_same<T, decay<T>::type>
* for both), the result is computed in two stages:
*     1. Direct: decay<decltype(false ? declval<T1>() : declval<T2>())>::type
*     2. Fallback (P0898R3, C++20): if the direct rule is ill-formed, try
*        decay<decltype(false ? declval<CR<T1>>() : declval<CR<T2>>())>::type
*        where CR<T> is `remove_reference<T>::type const&`.
*   If neither is well-formed, no `type` member is defined.
*
*   If T1 or T2 is not yet decayed, recurse: the result is
* common_type<decay<T1>::type, decay<T2>::type>::type.
*
*   re_std applies the P0898R3 fallback rule on all C++11+ tiers, even though
* it was only standardized in C++20 -- the implementation needs only C++11
* features.
*
*   USER CUSTOMIZATION:
*   This trait is a customization point. Users may specialize the binary
* form (full or partial specialization) to extend behavior:
*     template<>
*     struct re_std::common_type<MyA, MyB>
*     { typedef Result type; };
*   Such specializations override re_std's default partial spec by being
* more specific in partial-order ranking. Per the standard, users should
* only specialize for cv-unqualified non-reference types.
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait (requires
* decltype, declval, variadic templates).
*
*   DEPENDENCIES:
*   decay, remove_reference, is_same, void_t, re_std::declval.
*
*
* path:      /inc/djinterp/re_std/type_traits/common_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_COMMON_TYPE_
#define DJINTERP_RE_STD_TYPE_TRAITS_COMMON_TYPE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./decay.hpp"
#include "./remove_reference.hpp"
#include "./is_same.hpp"
#include "./void_t.hpp"
#include "../utility/declval.hpp"


NS_RESTD


    // common_type
    //   trait: primary template -- has no `type` member, matching the
    //          standard's behavior for common_type<> (zero arguments).
    //          Defined with empty body (rather than as a bare forward
    //          declaration) so that `typename common_type<>::type` is
    //          reliably "no such member" -- which is SFINAE-eligible
    //          on all conforming compilers -- rather than "incomplete
    //          type", which can hard-error on stricter implementations.
    template<typename... _Ts>
    struct common_type
    {};


    NS_INTERNAL

        // common_type_2_direct
        //   trait: applies the direct rule to two already-decayed types.
        //          Has `type` only when the conditional expression is
        //          well-formed.
        template<typename _T1,
                 typename _T2,
                 typename = void>
        struct common_type_2_direct
        {};

        // common_type_2_direct<_T1, _T2, void>
        //   trait: specialization; selected when the direct conditional
        //          expression is well-formed.
        template<typename _T1,
                 typename _T2>
        struct common_type_2_direct<
            _T1,
            _T2,
            re_std::void_t<decltype(
                false ? re_std::declval<_T1>() : re_std::declval<_T2>() )> >
        {
            typedef typename decay<decltype(
                false ? re_std::declval<_T1>() : re_std::declval<_T2>()
                )>::type type;
        };

        // common_type_2_fallback
        //   trait: applies the P0898R3 fallback rule. Wraps both types
        //          as `remove_reference<T>::type const&` before the
        //          conditional. Has `type` only when that expression
        //          is well-formed.
        template<typename _T1,
                 typename _T2,
                 typename = void>
        struct common_type_2_fallback
        {};

        // common_type_2_fallback<_T1, _T2, void>
        //   trait: specialization; selected when the const-lvalue-ref
        //          conditional is well-formed.
        template<typename _T1,
                 typename _T2>
        struct common_type_2_fallback<
            _T1,
            _T2,
            re_std::void_t<decltype(
                false
                ? re_std::declval<typename remove_reference<_T1>::type const&>()
                : re_std::declval<typename remove_reference<_T2>::type const&>()
                )> >
        {
            typedef typename decay<decltype(
                false
                ? re_std::declval<typename remove_reference<_T1>::type const&>()
                : re_std::declval<typename remove_reference<_T2>::type const&>()
                )>::type type;
        };

        // common_type_2_resolve
        //   trait: try direct first; if it has no `type`, fall through
        //          to the fallback. Implemented via void_t-gated partial
        //          spec on the direct's `type` member.
        template<typename _T1,
                 typename _T2,
                 typename = void>
        struct common_type_2_resolve
            : common_type_2_fallback<_T1, _T2>
        {};

        // common_type_2_resolve<_T1, _T2, void>
        //   trait: specialization; selected when direct rule succeeded.
        template<typename _T1,
                 typename _T2>
        struct common_type_2_resolve<
            _T1,
            _T2,
            re_std::void_t<typename common_type_2_direct<_T1, _T2>::type> >
            : common_type_2_direct<_T1, _T2>
        {};

        // common_type_2_dispatch
        //   trait: top-level binary dispatch. When _BothDecayed is false,
        //          decay and recurse to common_type. When true, apply
        //          the rule.
        template<typename _T1,
                 typename _T2,
                 bool     _BothDecayed>
        struct common_type_2_dispatch
            : common_type<typename decay<_T1>::type,
                          typename decay<_T2>::type>
        {};

        // common_type_2_dispatch<_T1, _T2, true>
        //   trait: specialization; both already decayed, apply the rule.
        template<typename _T1,
                 typename _T2>
        struct common_type_2_dispatch<_T1, _T2, true>
            : common_type_2_resolve<_T1, _T2>
        {};

        // common_type_2_impl
        //   trait: entry point for the binary case. Computes whether
        //          both arguments are already decayed, then dispatches.
        template<typename _T1,
                 typename _T2>
        struct common_type_2_impl
            : common_type_2_dispatch<
                  _T1,
                  _T2,
                  (    is_same<_T1, typename decay<_T1>::type>::value
                    && is_same<_T2, typename decay<_T2>::type>::value ) >
        {};

        // common_type_n_impl
        //   trait: SFINAE-friendly recursive case. Has `type` only when
        //          the inner common_type<T1, T2> resolved to a `type`,
        //          in which case it recurses on common_type<that, R...>.
        template<typename _Void,
                 typename _CT,
                 typename... _Rest>
        struct common_type_n_impl
        {};

        // common_type_n_impl<void, _CT, _Rest...>
        //   trait: specialization; selected when _CT::type exists.
        template<typename _CT,
                 typename... _Rest>
        struct common_type_n_impl<
            re_std::void_t<typename _CT::type>,
            _CT,
            _Rest...>
            : common_type<typename _CT::type, _Rest...>
        {};

    NS_END  // internal


    // common_type<_T>
    //   trait: 1-arg case; same as common_type<_T, _T>.
    template<typename _T>
    struct common_type<_T>
        : common_type<_T, _T>
    {};

    // common_type<_T1, _T2>
    //   trait: binary case. Users specialize this form (full or partial)
    //          to extend behavior. Default behavior delegates to
    //          internal::common_type_2_impl.
    template<typename _T1,
             typename _T2>
    struct common_type<_T1, _T2>
        : internal::common_type_2_impl<_T1, _T2>
    {};

    // common_type<_T1, _T2, _R...>
    //   trait: n-arg case (n >= 3 by partial ordering against the binary
    //          spec above). Recurses via common_type_n_impl, which is
    //          SFINAE-friendly: if any pairwise step fails, no `type`
    //          member is defined.
    template<typename _T1,
             typename _T2,
             typename... _R>
    struct common_type<_T1, _T2, _R...>
        : internal::common_type_n_impl<
              void,
              common_type<_T1, _T2>,
              _R... >
    {};


    // common_type_t (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename... _Ts>
        using common_type_t = typename common_type<_Ts...>::type;
    #endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_COMMON_TYPE_
