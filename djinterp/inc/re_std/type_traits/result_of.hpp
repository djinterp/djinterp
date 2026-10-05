/*******************************************************************************
* djinterp [re_std]                                                result_of.hpp
*
* result_of trait:
*   The function-call-syntax variant of invoke_result. result_of<F(Args...)>
* yields the same `type` as invoke_result<F, Args...>. The trait is given
* a function type F(Args...) rather than separate F and Args parameters.
*
*   STANDARD STATUS:
*   Introduced in C++11. Deprecated in C++17 in favor of invoke_result.
* Removed from std in C++20. re_std provides it on all C++11+ tiers per
* the project's "available where the language permits" policy: code
* targeting C++11/14 may still use it, and porting code that already
* uses result_of should not be blocked by the std deprecation. When
* writing new code, prefer invoke_result directly.
*
*   IMPLEMENTATION:
*   The primary template is intentionally undefined; only the
* specialization for function types F(Args...) is provided. Passing a
* non-function-type to result_of yields a hard error (no `type` member),
* matching the standard's behavior. The function-type specialization
* simply inherits from invoke_result, so SFINAE-friendliness propagates
* automatically.
*
*   Function types with C-style ellipsis (e.g. `int(int, ...)`) and
* function types with cv- or ref-qualifiers are not handled; the
* standard's result_of behavior for those is unspecified or undefined.
*
*   PORTABILITY:
*   Available on C++11 and later.
*
*   DEPENDENCIES:
*   invoke_result.
*
*
* path:      /inc/re_std/type_traits/result_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_RESULT_OF_HPP
#define RE_STD_TYPE_TRAITS_RESULT_OF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./invoke_result.hpp"


namespace re_std
{


    // result_of
    //   trait: primary template -- intentionally undefined. Only the
    //          partial specialization for function types is provided.
    //          Passing a non-function-type yields no `type` member.
    template<typename T>
    struct result_of;

    // result_of<F(Args...)>
    //   trait: specialization for function types. Inherits from
    //          invoke_result, so `type` is the return type of
    //          INVOKE(F, Args...) when well-formed, and absent otherwise.
    template<typename F,
             typename... Args>
    struct result_of<F(Args...)>
        : invoke_result<F, Args...>
    {};


    // result_of_t (C++14+)
    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<typename T>
        using result_of_t = typename result_of<T>::type;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_RESULT_OF_HPP
