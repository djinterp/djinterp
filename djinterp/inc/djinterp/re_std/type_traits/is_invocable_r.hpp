/******************************************************************************
* djinterp [restd]                                            is_invocable_r.hpp
*
* is_invocable_r trait:
*   true_type if INVOKE<R>(F, Args...) is well-formed in unevaluated
* context, false_type otherwise. INVOKE<R> is INVOKE implicitly converted
* to R; when R is cv void, the conversion is the discarded-value
* conversion (which is always valid for any type).
*
*   IMPLEMENTATION:
*   Two-step gate: first check is_invocable (does the call form?), and
* only when invocable does the helper specialization check the result
* convertibility. This avoids instantiating invoke_result<F, Args...>
* (which is ill-formed for non-invocable F) when the call would not
* succeed in the first place.
*
*   When _R is cv void, the convertibility check is short-circuited to
* true (any expression can be converted to void via discarded-value
* conversion). For non-void _R, is_convertible<invoke_result_type, _R>
* yields the answer.
*
*   PORTABILITY:
*   Available on C++11 and later. Standardized in C++17; restd backports
* to C++11+.
*
*   DEPENDENCIES:
*   is_invocable, invoke_result, is_void, is_convertible, integral_constant.
*
*
* path:      /inc/djinterp/restd/type_traits/is_invocable_r.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_INVOCABLE_R_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_INVOCABLE_R_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// restd
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_void.hpp"
#include "./is_convertible.hpp"
#include "./is_invocable.hpp"
#include "./invoke_result.hpp"


NS_RESTD


    NS_INTERNAL

        // is_invocable_r_helper
        //   trait: primary; gated by the boolean parameter _Invocable.
        //          When false, short-circuits to false_type without
        //          instantiating invoke_result (which would be
        //          ill-formed for non-invocable F).
        template<bool     _Invocable,
                 typename _R,
                 typename _F,
                 typename... _Args>
        struct is_invocable_r_helper
            : false_type
        {};

        // is_invocable_r_helper<true, _R, _F, _Args...>
        //   trait: specialization; selected when the call is invocable.
        //          R = void short-circuits to true; otherwise tests
        //          convertibility from the invoke result type to R.
        template<typename _R,
                 typename _F,
                 typename... _Args>
        struct is_invocable_r_helper<true, _R, _F, _Args...>
            : integral_constant<
                  bool,
                  (    is_void<_R>::value
                    || is_convertible<
                           typename invoke_result<_F, _Args...>::type,
                           _R >::value ) >
        {};

    NS_END  // internal


    // is_invocable_r
    //   trait: true_type if INVOKE<R>(F, Args...) is well-formed,
    //          false_type otherwise.
    template<typename _R,
             typename _F,
             typename... _Args>
    struct is_invocable_r
        : internal::is_invocable_r_helper<
              is_invocable<_F, _Args...>::value,
              _R, _F, _Args... >
    {};


    // is_invocable_r_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _R,
                 typename _F,
                 typename... _Args>
        D_CONSTEXPR bool is_invocable_r_v
            = is_invocable_r<_R, _F, _Args...>::value;
    #endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_INVOCABLE_R_
