/******************************************************************************
* djinterp [re_std]                                   is_nothrow_invocable_r.hpp
*
* is_nothrow_invocable_r trait:
*   true_type if is_invocable_r<R, F, Args...> is true_type AND the entire
* INVOKE<R> expression -- the call AND, when R is non-void, the implicit
* conversion of the result to R -- is noexcept; false_type otherwise.
*
*   IMPLEMENTATION:
*   Three-piece check:
*     1. is_invocable_r<R, F, Args...> must be true (gates the helper).
*     2. The underlying INVOKE call must be noexcept (probed directly
*        through the dispatcher).
*     3. When R is non-void, the implicit conversion from the invoke
*        result type to R must be noexcept (via is_nothrow_convertible).
*
*   When R is cv void, the conversion check is short-circuited to true:
* the discarded-value conversion never throws. is_nothrow_convertible is
* the right primitive for the non-void case because INVOKE<R> uses
* implicit conversion (per [func.require]/2), not static_cast.
*
*   PORTABILITY:
*   Available on C++11 and later. Standardized in C++17 (initially LFTS,
* merged in C++17); re_std backports to C++11+.
*
*   DEPENDENCIES:
*   is_invocable_r, invoke_result (for the dispatcher), is_void,
* is_nothrow_convertible (Group B), re_std::declval, integral_constant.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_invocable_r.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_R_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_R_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_void.hpp"
#include "./is_invocable_r.hpp"
#include "./is_nothrow_convertible.hpp"
#include "./invoke_result.hpp"  // for internal::invoker and invoke_result
#include "../utility/declval.hpp"


NS_RESTD


    NS_INTERNAL

        // is_nothrow_invocable_r_conv
        //   trait: bypass shim for the convertibility check. When _R is
        //          cv void, yields true_type without ever instantiating
        //          is_nothrow_convertible -- avoiding the corner case
        //          where is_nothrow_convertible<X, void> would substitute
        //          void into a function-parameter position (a per-
        //          [temp.deduct]/8 substitution failure that is only
        //          reliably SFINAE-eligible in unevaluated contexts on
        //          CWG 1330-conforming compilers).
        template<bool     _IsRVoid,
                 typename _R,
                 typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_r_conv;

        // is_nothrow_invocable_r_conv<true, ...>
        //   trait: R is void; conversion is the discarded-value
        //          conversion, which never throws.
        template<typename _R,
                 typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_r_conv<true, _R, _F, _Args...>
            : true_type
        {};

        // is_nothrow_invocable_r_conv<false, ...>
        //   trait: R is non-void; check noexcept of implicit conversion.
        template<typename _R,
                 typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_r_conv<false, _R, _F, _Args...>
            : is_nothrow_convertible<
                  typename invoke_result<_F, _Args...>::type,
                  _R >
        {};

        // is_nothrow_invocable_r_helper
        //   trait: primary; gated by the boolean parameter _InvocableR.
        //          When false, short-circuits to false_type without
        //          instantiating any noexcept probes.
        template<bool     _InvocableR,
                 typename _R,
                 typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_r_helper
            : false_type
        {};

        // is_nothrow_invocable_r_helper<true, _R, _F, _Args...>
        //   trait: specialization; selected when invocable and
        //          convertible. Probes the call's noexceptness directly
        //          and delegates the conversion's noexceptness to
        //          is_nothrow_invocable_r_conv (which short-circuits
        //          for R = void).
        template<typename _R,
                 typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_r_helper<true, _R, _F, _Args...>
            : integral_constant<
                  bool,
                  (    noexcept(
                           invoker::do_invoke(
                               re_std::declval<_F>(),
                               re_std::declval<_Args>()... ) )
                    && is_nothrow_invocable_r_conv<
                           is_void<_R>::value,
                           _R, _F, _Args... >::value ) >
        {};

    NS_END  // internal


    // is_nothrow_invocable_r
    //   trait: true_type if INVOKE<R>(F, Args...) is well-formed AND
    //          noexcept (both call and conversion); false_type otherwise.
    template<typename _R,
             typename _F,
             typename... _Args>
    struct is_nothrow_invocable_r
        : internal::is_nothrow_invocable_r_helper<
              is_invocable_r<_R, _F, _Args...>::value,
              _R, _F, _Args... >
    {};


    // is_nothrow_invocable_r_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _R,
                 typename _F,
                 typename... _Args>
        D_CONSTEXPR bool is_nothrow_invocable_r_v
            = is_nothrow_invocable_r<_R, _F, _Args...>::value;
    #endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_R_
