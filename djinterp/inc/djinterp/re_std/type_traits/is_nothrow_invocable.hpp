/******************************************************************************
* djinterp [restd]                                      is_nothrow_invocable.hpp
*
* is_nothrow_invocable trait:
*   true_type if is_invocable<F, Args...> is true_type AND the underlying
* INVOKE expression is noexcept; false_type otherwise.
*
*   IMPLEMENTATION:
*   Two-step gate: first check is_invocable (does the call form?), and
* only when invocable does the helper specialization probe noexceptness.
* The noexcept probe goes directly through the dispatcher's do_invoke,
* whose noexcept specifier mirrors the underlying expression -- so
* `noexcept(invoker::do_invoke(...))` returns the noexceptness of the
* target INVOKE expression, not of the wrapper function itself.
*
*   This direct dispatcher access (rather than going via invoke_result)
* is necessary because invoke_result captures only the type, not the
* noexceptness, of the call.
*
*   PORTABILITY:
*   Available on C++11 and later. Standardized in C++17; restd backports
* to C++11+.
*
*   DEPENDENCIES:
*   is_invocable, invoke_result (for the dispatcher), restd::declval,
* integral_constant.
*
*
* path:      /inc/djinterp/restd/type_traits/is_nothrow_invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// restd
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_invocable.hpp"
#include "./invoke_result.hpp"  // for internal::invoker
#include "../utility/declval.hpp"


NS_RESTD


    NS_INTERNAL

        // is_nothrow_invocable_helper
        //   trait: primary; gated by the boolean parameter _Invocable.
        //          When false, short-circuits to false_type without
        //          instantiating the noexcept probe.
        template<bool     _Invocable,
                 typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_helper
            : false_type
        {};

        // is_nothrow_invocable_helper<true, _F, _Args...>
        //   trait: specialization; selected when the call is invocable.
        //          Probes noexceptness through the dispatcher.
        template<typename _F,
                 typename... _Args>
        struct is_nothrow_invocable_helper<true, _F, _Args...>
            : integral_constant<
                  bool,
                  noexcept(
                      invoker::do_invoke(
                          restd::declval<_F>(),
                          restd::declval<_Args>()... ) ) >
        {};

    NS_END  // internal


    // is_nothrow_invocable
    //   trait: true_type if INVOKE(F, Args...) is well-formed AND
    //          noexcept; false_type otherwise.
    template<typename _F,
             typename... _Args>
    struct is_nothrow_invocable
        : internal::is_nothrow_invocable_helper<
              is_invocable<_F, _Args...>::value,
              _F, _Args... >
    {};


    // is_nothrow_invocable_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _F,
                 typename... _Args>
        D_CONSTEXPR bool is_nothrow_invocable_v
            = is_nothrow_invocable<_F, _Args...>::value;
    #endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_
