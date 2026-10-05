/*******************************************************************************
* djinterp [re_std]                                              is_function.hpp
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
*     all current re_std consumers; extend as needed.
*
*   Note: this primary trait does NOT handle ref-qualified or cv-
* qualified function types (e.g. void() const, void() &). Those are
* unusual and only meaningful on member functions; a future is_function
* extension may add them.
*
*
* path:      /inc/re_std/type_traits/is_function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_FUNCTION_HPP
#define RE_STD_TYPE_TRAITS_IS_FUNCTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./true_type.hpp"
#include "./false_type.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_FUNCTION
// =============================================================================

// is_function
//   trait: false (primary template).
template<typename Type>
struct is_function : false_type
{};


#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES

// =============================================================================
// I-A. C++11+ variadic path
// =============================================================================

    // is_function<R(Args...)>
    //   trait: true for fixed-arity function types.
    template<typename    R,
             typename... Args>
    struct is_function<R(Args...)> : true_type
    {};

    // is_function<R(Args..., ...)>
    //   trait: true for ellipsis-variadic function types.
    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...)> : true_type
    {};

    // ---------------------------------------------------------------------
    // cv- and ref-qualified function types
    // ---------------------------------------------------------------------
    //   A cv- or ref-qualified function type IS a function type; these are the
    // types that appear as `_F` when a pointer-to-member-function is
    // decomposed (`_F _Class::*`), so INVOKE's member-function bullets depend
    // on them.  Omitting these makes is_function false for every const member
    // function -- and therefore makes re_std::invoke reject it.

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) volatile> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) volatile> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) volatile &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) volatile &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) volatile &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) volatile &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const volatile> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const volatile> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const volatile &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const volatile &> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const volatile &&> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const volatile &&> : true_type
    {};

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

    // noexcept became part of the type system in C++17, doubling the set.

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) volatile noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) volatile noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) volatile & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) volatile & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) volatile && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) volatile && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const volatile noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const volatile noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const volatile & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const volatile & noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args...) const volatile && noexcept> : true_type
    {};

    template<typename    R,
             typename... Args>
    struct is_function<R(Args..., ...) const volatile && noexcept> : true_type
    {};

#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER

#else  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES

// =============================================================================
// I-B. C++98/03 explicit-arity path (0 through 10)
// =============================================================================

    // arity 0
    template<typename R>
    struct is_function<R()> : true_type {};
    template<typename R>
    struct is_function<R(...)> : true_type {};

    // arity 1
    template<typename R, typename A1>
    struct is_function<R(A1)> : true_type {};
    template<typename R, typename A1>
    struct is_function<R(A1, ...)> : true_type {};

    // arity 2
    template<typename R, typename A1, typename A2>
    struct is_function<R(A1, A2)> : true_type {};
    template<typename R, typename A1, typename A2>
    struct is_function<R(A1, A2, ...)> : true_type {};

    // arity 3
    template<typename R, typename A1, typename A2, typename A3>
    struct is_function<R(A1, A2, A3)> : true_type {};
    template<typename R, typename A1, typename A2, typename A3>
    struct is_function<R(A1, A2, A3, ...)> : true_type {};

    // arity 4
    template<typename R, typename A1, typename A2, typename A3,
             typename A4>
    struct is_function<R(A1, A2, A3, A4)> : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4>
    struct is_function<R(A1, A2, A3, A4, ...)> : true_type {};

    // arity 5
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5>
    struct is_function<R(A1, A2, A3, A4, A5)> : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5>
    struct is_function<R(A1, A2, A3, A4, A5, ...)> : true_type {};

    // arity 6
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6>
    struct is_function<R(A1, A2, A3, A4, A5, A6)> : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6>
    struct is_function<R(A1, A2, A3, A4, A5, A6, ...)> : true_type {};

    // arity 7
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7)> : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, ...)>
        : true_type {};

    // arity 8
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7,
             typename A8>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, A8)>
        : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7,
             typename A8>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, A8, ...)>
        : true_type {};

    // arity 9
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7,
             typename A8, typename A9>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9)>
        : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7,
             typename A8, typename A9>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9, ...)>
        : true_type {};

    // arity 10
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7,
             typename A8, typename A9, typename A10>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)>
        : true_type {};
    template<typename R, typename A1, typename A2, typename A3,
             typename A4, typename A5, typename A6, typename A7,
             typename A8, typename A9, typename A10>
    struct is_function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10,
                          ...)>
        : true_type {};

#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// =============================================================================
// II.  IS_FUNCTION_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_function_v
    //   variable: convenience for is_function<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_function_v = is_function<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_FUNCTION_HPP
