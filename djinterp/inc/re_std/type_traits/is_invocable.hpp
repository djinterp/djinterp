/******************************************************************************
* djinterp [re_std]                                             is_invocable.hpp
*
* is_invocable trait:
*   true_type if INVOKE(F, Args...) is a well-formed expression in
* unevaluated context, false_type otherwise.
*
*   IMPLEMENTATION:
*   Detection is delegated to invoke_result via a void_t SFINAE probe on
* `typename invoke_result<F, Args...>::type`. This re-uses the dispatcher
* machinery in invoke_result.hpp rather than duplicating the SFINAE
* selection logic. The trade-off is one extra template instantiation per
* query; the gain is a single source of truth for "what counts as a valid
* INVOKE expression."
*
*   PORTABILITY:
*   Available on C++11 and later. Standardized in C++17; re_std backports
* to C++11+.
*
*   DEPENDENCIES:
*   invoke_result, void_t, true_type, false_type.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_INVOCABLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_INVOCABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./void_t.hpp"
#include "./invoke_result.hpp"


NS_RESTD


    NS_INTERNAL

        // is_invocable_impl
        //   trait: SFINAE-friendly invocability detection. Primary
        //          template defaults to false_type; the specialization
        //          fires when invoke_result<_F, _Args...>::type is
        //          well-formed (void_t collapses to void).
        template<typename _Void,
                 typename _F,
                 typename... _Args>
        struct is_invocable_impl
            : false_type
        {};

        // is_invocable_impl<void, _F, _Args...>
        //   trait: specialization; selected when INVOKE(F, Args...) is
        //          well-formed.
        template<typename _F,
                 typename... _Args>
        struct is_invocable_impl<
            re_std::void_t<typename invoke_result<_F, _Args...>::type>,
            _F, _Args...>
            : true_type
        {};

    NS_END  // internal


    // is_invocable
    //   trait: true_type if INVOKE(F, Args...) is well-formed in
    //          unevaluated context, false_type otherwise.
    template<typename _F,
             typename... _Args>
    struct is_invocable
        : internal::is_invocable_impl<void, _F, _Args...>
    {};


    // is_invocable_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _F,
                 typename... _Args>
        D_CONSTEXPR bool is_invocable_v = is_invocable<_F, _Args...>::value;
    #endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_INVOCABLE_
