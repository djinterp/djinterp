/*******************************************************************************
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
* path:      /inc/re_std/type_traits/is_invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_INVOCABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_INVOCABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./void_t.hpp"
#include "./invoke_result.hpp"


namespace re_std
{


    namespace internal
    {

        // is_invocable_impl
        //   trait: SFINAE-friendly invocability detection. Primary
        //          template defaults to false_type; the specialization
        //          fires when invoke_result<F, Args...>::type is
        //          well-formed (void_t collapses to void).
        template<typename Void,
                 typename F,
                 typename... Args>
        struct is_invocable_impl
            : false_type
        {};

        // is_invocable_impl<void, F, Args...>
        //   trait: specialization; selected when INVOKE(F, Args...) is
        //          well-formed.
        template<typename F,
                 typename... Args>
        struct is_invocable_impl<
            re_std::void_t<typename invoke_result<F, Args...>::type>,
            F, Args...>
            : true_type
        {};

    }  // internal


    // is_invocable
    //   trait: true_type if INVOKE(F, Args...) is well-formed in
    //          unevaluated context, false_type otherwise.
    template<typename F,
             typename... Args>
    struct is_invocable
        : internal::is_invocable_impl<void, F, Args...>
    {};


    // is_invocable_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename F,
                 typename... Args>
        RE_STD_CONSTEXPR bool is_invocable_v = is_invocable<F, Args...>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_INVOCABLE_HPP
