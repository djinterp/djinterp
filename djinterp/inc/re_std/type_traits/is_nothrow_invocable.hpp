/*******************************************************************************
* djinterp [re_std]                                     is_nothrow_invocable.hpp
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
*   Available on C++11 and later. Standardized in C++17; re_std backports
* to C++11+.
*
*   DEPENDENCIES:
*   is_invocable, invoke_result (for the dispatcher), re_std::declval,
* integral_constant.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_invocable.hpp"
#include "./invoke_result.hpp"  // for internal::invoker
#include "../utility/declval.hpp"


namespace re_std
{


    namespace internal
    {

        // is_nothrow_invocable_helper
        //   trait: primary; gated by the boolean parameter Invocable.
        //          When false, short-circuits to false_type without
        //          instantiating the noexcept probe.
        template<bool     Invocable,
                 typename F,
                 typename... Args>
        struct is_nothrow_invocable_helper
            : false_type
        {};

        // is_nothrow_invocable_helper<true, F, Args...>
        //   trait: specialization; selected when the call is invocable.
        //          Probes noexceptness through the dispatcher.
        template<typename F,
                 typename... Args>
        struct is_nothrow_invocable_helper<true, F, Args...>
            : integral_constant<
                  bool,
                  noexcept(
                      invoker::do_invoke(
                          re_std::declval<F>(),
                          re_std::declval<Args>()... ) ) >
        {};

    }  // internal


    // is_nothrow_invocable
    //   trait: true_type if INVOKE(F, Args...) is well-formed AND
    //          noexcept; false_type otherwise.
    template<typename F,
             typename... Args>
    struct is_nothrow_invocable
        : internal::is_nothrow_invocable_helper<
              is_invocable<F, Args...>::value,
              F, Args... >
    {};


    // is_nothrow_invocable_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename F,
                 typename... Args>
        RE_STD_CONSTEXPR bool is_nothrow_invocable_v
            = is_nothrow_invocable<F, Args...>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_INVOCABLE_HPP
