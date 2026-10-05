/*******************************************************************************
* djinterp [core]                                            function_traits.hpp
*
* Compile-time introspection of callable types (C++).
*   Extracts the arity, return type, argument types, and class type (for
* member functions) of any callable: free functions, function pointers,
* member function pointers, std::function, lambdas, and ordinary functor
* types. Provides a uniform interface across all these forms.
*
*   The primary template inspects T::operator() (the lambda case), and
* specializations handle every other shape. Generic lambdas and templated
* operator() are NOT inspectable -- their signature depends on the
* arguments and cannot be deduced ahead of call.
*
*   This module complements is_callable / callable_result_t in
* functional_traits.hpp: those answer "can I call this with these
* arguments?", while function_traits answers "what is the declared shape
* of this callable?".
*
* USAGE:
*   auto f = [](int a, double b) -> std::string { return ""; };
*
*   function_traits<decltype(f)>::arity;            // 2
*   function_traits<decltype(f)>::return_type;      // std::string
*   function_traits<decltype(f)>::arg<0>::type;     // int
*   function_traits<decltype(f)>::arg_t<1>;         // double
*   function_traits<decltype(f)>::args_tuple;       // std::tuple<int, double>
*
*   // Works on free functions too:
*   int g(double, char);
*   function_traits<decltype(&g)>::return_type;     // int
*
*   // Member functions:
*   struct S { int m(double) const; };
*   function_traits<decltype(&S::m)>::class_type;   // S
*
*
* path:      /inc/djinterp/core/functional/function_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PRIMARY TEMPLATE                   (inspects T::operator())
      -----------------------------------------------------------

II.   FUNCTION-TYPE SPECIALIZATION       (R(Args...))
      -----------------------------------------------

III.  FUNCTION-POINTER SPECIALIZATION    (R(*)(Args...))
      --------------------------------------------------

IV.   MEMBER-FUNCTION SPECIALIZATIONS    (R(C::*)(Args...) etc.)
      ----------------------------------------------------------

V.    STD::FUNCTION SPECIALIZATION
      ----------------------------

VI.   CONVENIENCE ALIASES
      -------------------
      1.    return_type_t<F>
      2.    arg_t<F, N>
      3.    arity_v<F>
      4.    args_tuple_t<F>

VII.  PREDICATE TRAITS
      ----------------
      1.    is_inspectable<F>              (can function_traits succeed?)

VIII. CALL DETECTION                     (works on generic / templated callables)
      ---------------------------------------------------------------------------
      1.    call_result_t<F, Args...>      (decltype of the call, or nonesuch)
      2.    is_invocable_with<F, Args...>  (can F be called on Args?)
      3.    is_invocable_r_with<R, F, Args...>
*/


#ifndef DJINTERP_FUNCTIONAL_FUNCTION_TRAITS_HPP
#define DJINTERP_FUNCTIONAL_FUNCTION_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///             I.    PRIMARY TEMPLATE                                      ///
///////////////////////////////////////////////////////////////////////////////

// function_traits
//   trait: primary template, used for any callable that is not a
// raw function, function pointer, member function pointer, or
// std::function. The primary inspects T::operator() and inherits
// from the corresponding member-function-pointer specialization.
//   This is the lambda case: a lambda is a class with an
// operator(), and decltype(&T::operator()) is a const member
// function pointer.
template<typename Type>
struct function_traits
    : function_traits<decltype(&Type::operator())>
{};

// function_traits (decay specialization)
//   strips references and cv qualifiers from Type so users do not
// have to manually decay before inspecting. Without this, passing
// a const lambda or an lvalue lambda would fail to match the
// primary template.
template<typename Type>
struct function_traits<Type&>
    : function_traits<typename std::remove_reference<Type>::type>
{};

template<typename Type>
struct function_traits<Type&&>
    : function_traits<typename std::remove_reference<Type>::type>
{};

template<typename Type>
struct function_traits<const Type>
    : function_traits<Type>
{};


///////////////////////////////////////////////////////////////////////////////
///             II.   FUNCTION-TYPE SPECIALIZATION                          ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (function type)
//   trait: specialization for the canonical function type
// R(Args...). All other specializations forward to this one, so
// changes to the interface need only be made here.
template<typename Return,
         typename... Args>
struct function_traits<Return(Args...)>
{
    // return_type
    //   type: the declared return type of the callable.
    using return_type = Return;

    // arity
    //   constant: the number of declared parameters. For variadic
    // C-style functions, this is the count of named parameters.
    static D_CONSTEXPR
    std::size_t arity = sizeof...(Args);

    // args_tuple
    //   type: the parameter list as a std::tuple. Useful for
    // pack-expansion-based metaprogramming.
    using args_tuple = std::tuple<Args...>;

    // arg
    //   trait: nested template that extracts the N-th argument
    // type. Bounds-check is delegated to std::tuple_element which
    // produces a static_assert failure for out-of-range indices.
    template<std::size_t N>
    struct arg
    {
        static_assert(N < sizeof...(Args),
            "function_traits::arg: index out of range");

        using type = typename std::tuple_element<
            N, std::tuple<Args...>>::type;
    };

    // arg_t
    //   alias: shorthand for arg<N>::type.
    template<std::size_t N>
    using arg_t = typename arg<N>::type;

    // is_noexcept
    //   constant: whether the callable is declared noexcept. The
    // primary function-type form has no noexcept information, so
    // it defaults to false. Specializations may override.
    static D_CONSTEXPR bool is_noexcept = false;
};


///////////////////////////////////////////////////////////////////////////////
///             III.  FUNCTION-POINTER SPECIALIZATION                       ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (function pointer)
//   trait: specialization for a pointer to a free function.
// Delegates to the bare-function-type specialization.
template<typename Return,
         typename... Args>
struct function_traits<Return(*)(Args...)>
    : function_traits<Return(Args...)>
{};

// function_traits (function reference)
//   trait: specialization for a reference to a free function.
template<typename Return,
         typename... Args>
struct function_traits<Return(&)(Args...)>
    : function_traits<Return(Args...)>
{};


///////////////////////////////////////////////////////////////////////////////
///             IV.   MEMBER-FUNCTION SPECIALIZATIONS                       ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (non-const member function pointer)
//   trait: specialization for &Class::member. Inherits return_type
// and args from the bare function type, and adds class_type.
template<typename Return,
         typename Class,
         typename... Args>
struct function_traits<Return(Class::*)(Args...)>
    : function_traits<Return(Args...)>
{
    // class_type
    //   type: the class that owns the member function.
    using class_type = Class;
};

// function_traits (const member function pointer)
//   trait: as above, but for `void m() const`. This is the form
// that lambdas resolve to (since the closure's operator() is
// implicitly const unless declared `mutable`).
template<typename Return,
         typename Class,
         typename... Args>
struct function_traits<Return(Class::*)(Args...) const>
    : function_traits<Return(Args...)>
{
    using class_type = Class;
};

// function_traits (volatile member function pointer)
template<typename Return,
         typename Class,
         typename... Args>
struct function_traits<Return(Class::*)(Args...) volatile>
    : function_traits<Return(Args...)>
{
    using class_type = Class;
};

// function_traits (const volatile member function pointer)
template<typename Return,
         typename Class,
         typename... Args>
struct function_traits<Return(Class::*)(Args...) const volatile>
    : function_traits<Return(Args...)>
{
    using class_type = Class;
};


///////////////////////////////////////////////////////////////////////////////
///             V.    STD::FUNCTION SPECIALIZATION                          ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (std::function)
//   trait: specialization for std::function<Sig>. Forwards to the
// bare-function-type specialization extracted from the signature.
template<typename Return,
         typename... Args>
struct function_traits<std::function<Return(Args...)>>
    : function_traits<Return(Args...)>
{};


///////////////////////////////////////////////////////////////////////////////
///             VI.   CONVENIENCE ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////

// return_type_t
//   alias: shorthand for function_traits<F>::return_type.
template<typename Fn>
using return_type_t = typename function_traits<Fn>::return_type;


// arg_t
//   alias: shorthand for function_traits<F>::template arg_t<N>.
// Note: template-template aliasing in C++11 requires the inner
// template to be accessed via template keyword in dependent
// contexts.
template<typename     Fn,
         std::size_t  N>
using arg_t = typename function_traits<Fn>::template arg<N>::type;


// args_tuple_t
//   alias: shorthand for function_traits<F>::args_tuple.
template<typename Fn>
using args_tuple_t = typename function_traits<Fn>::args_tuple;


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// arity_v
//   variable: shorthand for function_traits<F>::arity. Available
// only when variable templates are supported (C++14+).
template<typename Fn>
static constexpr std::size_t arity_v = function_traits<Fn>::arity;
#endif


///////////////////////////////////////////////////////////////////////////////
///             VII.  PREDICATE TRAITS                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_inspectable_helper
    //   helper: SFINAE-detects whether function_traits<F> has a
    // well-formed return_type member. Generic lambdas with
    // templated operator() fail to instantiate function_traits and
    // would be reported as not inspectable.
    template<typename Fn>
    struct is_inspectable_helper
    {
    private:
        template<typename F>
        static auto test(int)
            -> decltype(
                typename function_traits<F>::return_type{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Fn>(0));
    };

NS_END  // internal


// is_inspectable
//   trait: true if function_traits<Fn> is well-formed and yields
// usable type information. False for generic lambdas, overloaded
// callables, and other cases where the signature is not uniquely
// determined.
template<typename Fn>
struct is_inspectable
    : internal::is_inspectable_helper<
          typename std::decay<Fn>::type>::type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Fn>
static constexpr bool is_inspectable_v = is_inspectable<Fn>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             VIII. CALL DETECTION                                        ///
///////////////////////////////////////////////////////////////////////////////
//   function_traits (above) inspects the DECLARED shape of a callable and so
// cannot see through a templated / generic operator() -- exactly the shape a
// transducer's reducer-producing closures take. The traits here answer the
// complementary question by EXPRESSION probing: given concrete argument
// types, can Fn be called, and what does that call yield? These succeed on
// generic lambdas and other templated callables that is_inspectable rejects.

NS_INTERNAL

    // call_nonesuch
    //   sentinel: yielded by call_result when the call is ill-formed.
    struct call_nonesuch
    {};

    // call_result_helper
    //   helper: yields the result type of calling a const-lvalue Fn on
    // Args (preserving their value categories), or call_nonesuch when that
    // call is ill-formed.
    template<typename Fn,
             typename... Args>
    struct call_result_helper
    {
    private:
        template<typename F>
        static auto test(int) -> decltype(
            std::declval<const F&>()(std::declval<Args>()...));

        template<typename>
        static call_nonesuch test(...);

    public:
        using type = decltype(test<Fn>(0));
    };

NS_END  // internal


// call_result_t
//   alias: the result of calling a const-lvalue Fn on Args, or
// internal::call_nonesuch when the call is ill-formed. Unlike return_type_t,
// this works on generic lambdas and templated operator() because it probes a
// concrete call rather than inspecting a declared signature.
template<typename Fn,
         typename... Args>
using call_result_t =
    typename internal::call_result_helper<
        typename std::decay<Fn>::type, Args...>::type;


// is_invocable_with
//   trait: true when a const-lvalue Fn can be called on Args. The "_with"
// suffix avoids any clash with a std::is_invocable-style name and signals
// that the argument types are supplied explicitly.
template<typename Fn,
         typename... Args>
struct is_invocable_with
{
    static D_CONSTEXPR bool value =
        !std::is_same<call_result_t<Fn, Args...>,
                      internal::call_nonesuch>::value;
};


// is_invocable_r_with
//   trait: true when a const-lvalue Fn can be called on Args AND the
// result is convertible to Return. A void Return matches any successful
// call (mirroring the standard is_invocable_r treatment of void).
template<typename Return,
         typename Fn,
         typename... Args>
struct is_invocable_r_with
{
private:
    using result_t = call_result_t<Fn, Args...>;

    static D_CONSTEXPR bool callable =
        !std::is_same<result_t, internal::call_nonesuch>::value;

public:
    static D_CONSTEXPR bool value =
        ( callable &&
          ( std::is_void<Return>::value ||
            std::is_convertible<result_t, Return>::value ) );
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_invocable_with_v
//   constant: shorthand for is_invocable_with<Fn, Args...>::value.
template<typename Fn,
         typename... Args>
static constexpr bool is_invocable_with_v =
    is_invocable_with<Fn, Args...>::value;

// is_invocable_r_with_v
//   constant: shorthand for is_invocable_r_with<Return, Fn, Args...>.
template<typename Return,
         typename Fn,
         typename... Args>
static constexpr bool is_invocable_r_with_v =
    is_invocable_r_with<Return, Fn, Args...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_FUNCTION_TRAITS_HPP
