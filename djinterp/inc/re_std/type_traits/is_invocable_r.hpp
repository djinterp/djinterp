/*******************************************************************************
* djinterp [re_std]                                           is_invocable_r.hpp
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
*   When R is cv void, the convertibility check is short-circuited to
* true (any expression can be converted to void via discarded-value
* conversion). For non-void R, is_convertible<invoke_result_type, R>
* yields the answer.
*
*   PORTABILITY:
*   Available on C++11 and later. Standardized in C++17; re_std backports
* to C++11+.
*
*   DEPENDENCIES:
*   is_invocable, invoke_result, is_void, is_convertible, integral_constant.
*
*
* path:      /inc/re_std/type_traits/is_invocable_r.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_INVOCABLE_R_HPP
#define RE_STD_TYPE_TRAITS_IS_INVOCABLE_R_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_void.hpp"
#include "./is_convertible.hpp"
#include "./is_invocable.hpp"
#include "./invoke_result.hpp"


namespace re_std
{


    namespace internal
    {

        // is_invocable_r_helper
        //   trait: primary; gated by the boolean parameter Invocable.
        //          When false, short-circuits to false_type without
        //          instantiating invoke_result (which would be
        //          ill-formed for non-invocable F).
        template<bool     Invocable,
                 typename R,
                 typename F,
                 typename... Args>
        struct is_invocable_r_helper
            : false_type
        {};

        // is_invocable_r_helper<true, R, F, Args...>
        //   trait: specialization; selected when the call is invocable.
        //          R = void short-circuits to true; otherwise tests
        //          convertibility from the invoke result type to R.
        template<typename R,
                 typename F,
                 typename... Args>
        struct is_invocable_r_helper<true, R, F, Args...>
            : integral_constant<
                  bool,
                  (    is_void<R>::value
                    || is_convertible<
                           typename invoke_result<F, Args...>::type,
                           R >::value ) >
        {};

    }  // internal


    // is_invocable_r
    //   trait: true_type if INVOKE<R>(F, Args...) is well-formed,
    //          false_type otherwise.
    template<typename R,
             typename F,
             typename... Args>
    struct is_invocable_r
        : internal::is_invocable_r_helper<
              is_invocable<F, Args...>::value,
              R, F, Args... >
    {};


    // is_invocable_r_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename R,
                 typename F,
                 typename... Args>
        RE_STD_CONSTEXPR bool is_invocable_r_v
            = is_invocable_r<R, F, Args...>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_INVOCABLE_R_HPP
