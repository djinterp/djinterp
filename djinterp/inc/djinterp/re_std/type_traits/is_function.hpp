/******************************************************************************
* djinterp [restd]                                             is_function.hpp
*
* is_function trait header:
*   Detects whether a type is a function type (NOT a function pointer,
* NOT a functor, NOT a lambda, NOT a member function). Used by `any`
* to exclude function pointers from the pointer SBO category.
*
*     is_function<void(int)>::value     -> true
*     is_function<int(int, double)>::value -> true
*     is_function<void(*)(int)>::value  -> false  (pointer to function)
*     is_function<void(&)(int)>::value  -> false  (reference to function)
*     is_function<int>::value           -> false
*     struct C {}; is_function<C>::value -> false
*
*   PORTABILITY:
*   - C++11+: two variadic-template partial specializations cover all
*     arities - R(Args...) and R(Args..., ...).
*   - C++98/03: explicit specializations for arities 0 through 10, each
*     with and without C-style ellipsis. Functions with more than 10
*     parameters are not detected on C++98/03. This is sufficient for
*     all current restd consumers; extend as needed.
*
*   Note: this primary trait does NOT handle ref-qualified or cv-
* qualified function types (e.g. void() const, void() &). Those are
* unusual and only meaningful on member functions; a future is_function
* extension may add them.
*
*
* path:      /inc/djinterp/restd/type_traits/is_function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_FUNCTION_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_FUNCTION_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


NS_RESTD


// =============================================================================
// I.   IS_FUNCTION
// =============================================================================

// is_function
//   trait: false (primary template).
template<typename _Type>
struct is_function : false_type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

// =============================================================================
// I-A. C++11+ variadic path
// =============================================================================

    // is_function<_R(_Args...)>
    //   trait: true for fixed-arity function types.
    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...)> : true_type
    {};

    // is_function<_R(_Args..., ...)>
    //   trait: true for ellipsis-variadic function types.
    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...)> : true_type
    {};

    // ---------------------------------------------------------------------
    // cv- and ref-qualified function types
    // ---------------------------------------------------------------------
    //   A cv- or ref-qualified function type IS a function type; these are the
    // types that appear as `_F` when a pointer-to-member-function is
    // decomposed (`_F _Class::*`), so INVOKE's member-function bullets depend
    // on them.  Omitting these makes is_function false for every const member
    // function -- and therefore makes restd::invoke reject it.

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) volatile> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) volatile> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) volatile &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) volatile &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) volatile &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) volatile &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const volatile> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const volatile> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const volatile &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const volatile &> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const volatile &&> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const volatile &&> : true_type
    {};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // noexcept became part of the type system in C++17, doubling the set.

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) volatile noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) volatile noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) volatile & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) volatile & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) volatile && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) volatile && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const volatile noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const volatile noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const volatile & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const volatile & noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args...) const volatile && noexcept> : true_type
    {};

    template<typename    _R,
             typename... _Args>
    struct is_function<_R(_Args..., ...) const volatile && noexcept> : true_type
    {};

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#else  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

// =============================================================================
// I-B. C++98/03 explicit-arity path (0 through 10)
// =============================================================================

    // arity 0
    template<typename _R>
    struct is_function<_R()> : true_type {};
    template<typename _R>
    struct is_function<_R(...)> : true_type {};

    // arity 1
    template<typename _R, typename _A1>
    struct is_function<_R(_A1)> : true_type {};
    template<typename _R, typename _A1>
    struct is_function<_R(_A1, ...)> : true_type {};

    // arity 2
    template<typename _R, typename _A1, typename _A2>
    struct is_function<_R(_A1, _A2)> : true_type {};
    template<typename _R, typename _A1, typename _A2>
    struct is_function<_R(_A1, _A2, ...)> : true_type {};

    // arity 3
    template<typename _R, typename _A1, typename _A2, typename _A3>
    struct is_function<_R(_A1, _A2, _A3)> : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3>
    struct is_function<_R(_A1, _A2, _A3, ...)> : true_type {};

    // arity 4
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4>
    struct is_function<_R(_A1, _A2, _A3, _A4)> : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4>
    struct is_function<_R(_A1, _A2, _A3, _A4, ...)> : true_type {};

    // arity 5
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5)> : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, ...)> : true_type {};

    // arity 6
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6)> : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, ...)> : true_type {};

    // arity 7
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7)> : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, ...)>
        : true_type {};

    // arity 8
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7,
             typename _A8>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, _A8)>
        : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7,
             typename _A8>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, _A8, ...)>
        : true_type {};

    // arity 9
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7,
             typename _A8, typename _A9>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, _A8, _A9)>
        : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7,
             typename _A8, typename _A9>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, _A8, _A9, ...)>
        : true_type {};

    // arity 10
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7,
             typename _A8, typename _A9, typename _A10>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, _A8, _A9, _A10)>
        : true_type {};
    template<typename _R, typename _A1, typename _A2, typename _A3,
             typename _A4, typename _A5, typename _A6, typename _A7,
             typename _A8, typename _A9, typename _A10>
    struct is_function<_R(_A1, _A2, _A3, _A4, _A5, _A6, _A7, _A8, _A9, _A10,
                          ...)>
        : true_type {};

#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// =============================================================================
// II.  IS_FUNCTION_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_function_v
    //   variable: convenience for is_function<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_function_v = is_function<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_FUNCTION_
