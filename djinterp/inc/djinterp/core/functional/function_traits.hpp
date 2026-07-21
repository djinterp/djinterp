/******************************************************************************
* djinterp [functional]                                    function_traits.hpp
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PRIMARY TEMPLATE                   (inspects T::operator())
II.   FUNCTION-TYPE SPECIALIZATION       (R(Args...))
III.  FUNCTION-POINTER SPECIALIZATION    (R(*)(Args...))
IV.   MEMBER-FUNCTION SPECIALIZATIONS    (R(C::*)(Args...) etc.)
V.    STD::FUNCTION SPECIALIZATION       
VI.   CONVENIENCE ALIASES                
      1.  return_type_t<F>               
      2.  arg_t<F, N>                    
      3.  arity_v<F>                     
      4.  args_tuple_t<F>                
VII.  PREDICATE TRAITS                   
      1.  is_inspectable<F>              (can function_traits succeed?)
VIII. CALL DETECTION                     (works on generic / templated callables)
      1.  call_result_t<F, Args...>      (decltype of the call, or nonesuch)
      2.  is_invocable_with<F, Args...>  (can F be called on Args?)
      3.  is_invocable_r_with<R, F, Args...>
*/


#ifndef DJINTERP_FUNCTIONAL_FUNCTION_TRAITS_
#define DJINTERP_FUNCTIONAL_FUNCTION_TRAITS_ 1

// std
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"


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
template<typename _Type>
struct function_traits
    : function_traits<decltype(&_Type::operator())>
{};

// function_traits (decay specialization)
//   strips references and cv qualifiers from _Type so users do not
// have to manually decay before inspecting. Without this, passing
// a const lambda or an lvalue lambda would fail to match the
// primary template.
template<typename _Type>
struct function_traits<_Type&>
    : function_traits<typename std::remove_reference<_Type>::type>
{};

template<typename _Type>
struct function_traits<_Type&&>
    : function_traits<typename std::remove_reference<_Type>::type>
{};

template<typename _Type>
struct function_traits<const _Type>
    : function_traits<_Type>
{};


///////////////////////////////////////////////////////////////////////////////
///             II.   FUNCTION-TYPE SPECIALIZATION                          ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (function type)
//   trait: specialization for the canonical function type
// R(Args...). All other specializations forward to this one, so
// changes to the interface need only be made here.
template<typename _Return,
         typename... _Args>
struct function_traits<_Return(_Args...)>
{
    // return_type
    //   type: the declared return type of the callable.
    using return_type = _Return;

    // arity
    //   constant: the number of declared parameters. For variadic
    // C-style functions, this is the count of named parameters.
    static D_CONSTEXPR
    std::size_t arity = sizeof...(_Args);

    // args_tuple
    //   type: the parameter list as a std::tuple. Useful for
    // pack-expansion-based metaprogramming.
    using args_tuple = std::tuple<_Args...>;

    // arg
    //   trait: nested template that extracts the N-th argument
    // type. Bounds-check is delegated to std::tuple_element which
    // produces a static_assert failure for out-of-range indices.
    template<std::size_t _N>
    struct arg
    {
        static_assert(_N < sizeof...(_Args),
            "function_traits::arg: index out of range");

        using type = typename std::tuple_element<
            _N, std::tuple<_Args...>>::type;
    };

    // arg_t
    //   alias: shorthand for arg<N>::type.
    template<std::size_t _N>
    using arg_t = typename arg<_N>::type;

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
template<typename _Return,
         typename... _Args>
struct function_traits<_Return(*)(_Args...)>
    : function_traits<_Return(_Args...)>
{};

// function_traits (function reference)
//   trait: specialization for a reference to a free function.
template<typename _Return,
         typename... _Args>
struct function_traits<_Return(&)(_Args...)>
    : function_traits<_Return(_Args...)>
{};


///////////////////////////////////////////////////////////////////////////////
///             IV.   MEMBER-FUNCTION SPECIALIZATIONS                       ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (non-const member function pointer)
//   trait: specialization for &Class::member. Inherits return_type
// and args from the bare function type, and adds class_type.
template<typename _Return,
         typename _Class,
         typename... _Args>
struct function_traits<_Return(_Class::*)(_Args...)>
    : function_traits<_Return(_Args...)>
{
    // class_type
    //   type: the class that owns the member function.
    using class_type = _Class;
};

// function_traits (const member function pointer)
//   trait: as above, but for `void m() const`. This is the form
// that lambdas resolve to (since the closure's operator() is
// implicitly const unless declared `mutable`).
template<typename _Return,
         typename _Class,
         typename... _Args>
struct function_traits<_Return(_Class::*)(_Args...) const>
    : function_traits<_Return(_Args...)>
{
    using class_type = _Class;
};

// function_traits (volatile member function pointer)
template<typename _Return,
         typename _Class,
         typename... _Args>
struct function_traits<_Return(_Class::*)(_Args...) volatile>
    : function_traits<_Return(_Args...)>
{
    using class_type = _Class;
};

// function_traits (const volatile member function pointer)
template<typename _Return,
         typename _Class,
         typename... _Args>
struct function_traits<_Return(_Class::*)(_Args...) const volatile>
    : function_traits<_Return(_Args...)>
{
    using class_type = _Class;
};


///////////////////////////////////////////////////////////////////////////////
///             V.    STD::FUNCTION SPECIALIZATION                          ///
///////////////////////////////////////////////////////////////////////////////

// function_traits (std::function)
//   trait: specialization for std::function<Sig>. Forwards to the
// bare-function-type specialization extracted from the signature.
template<typename _Return,
         typename... _Args>
struct function_traits<std::function<_Return(_Args...)>>
    : function_traits<_Return(_Args...)>
{};


///////////////////////////////////////////////////////////////////////////////
///             VI.   CONVENIENCE ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////

// return_type_t
//   alias: shorthand for function_traits<F>::return_type.
template<typename _Fn>
using return_type_t = typename function_traits<_Fn>::return_type;


// arg_t
//   alias: shorthand for function_traits<F>::template arg_t<N>.
// Note: template-template aliasing in C++11 requires the inner
// template to be accessed via template keyword in dependent
// contexts.
template<typename     _Fn,
         std::size_t  _N>
using arg_t = typename function_traits<_Fn>::template arg<_N>::type;


// args_tuple_t
//   alias: shorthand for function_traits<F>::args_tuple.
template<typename _Fn>
using args_tuple_t = typename function_traits<_Fn>::args_tuple;


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// arity_v
//   variable: shorthand for function_traits<F>::arity. Available
// only when variable templates are supported (C++14+).
template<typename _Fn>
static constexpr std::size_t arity_v = function_traits<_Fn>::arity;
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
    template<typename _Fn>
    struct is_inspectable_helper
    {
    private:
        template<typename _F>
        static auto test(int)
            -> decltype(
                typename function_traits<_F>::return_type{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Fn>(0));
    };

NS_END  // internal


// is_inspectable
//   trait: true if function_traits<_Fn> is well-formed and yields
// usable type information. False for generic lambdas, overloaded
// callables, and other cases where the signature is not uniquely
// determined.
template<typename _Fn>
struct is_inspectable
    : internal::is_inspectable_helper<
          typename std::decay<_Fn>::type>::type
{};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Fn>
static constexpr bool is_inspectable_v = is_inspectable<_Fn>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             VIII. CALL DETECTION                                        ///
///////////////////////////////////////////////////////////////////////////////
//   function_traits (above) inspects the DECLARED shape of a callable and so
// cannot see through a templated / generic operator() -- exactly the shape a
// transducer's reducer-producing closures take. The traits here answer the
// complementary question by EXPRESSION probing: given concrete argument
// types, can _Fn be called, and what does that call yield? These succeed on
// generic lambdas and other templated callables that is_inspectable rejects.

NS_INTERNAL

    // call_nonesuch
    //   sentinel: yielded by call_result when the call is ill-formed.
    struct call_nonesuch
    {};

    // call_result_helper
    //   helper: yields the result type of calling a const-lvalue _Fn on
    // _Args (preserving their value categories), or call_nonesuch when that
    // call is ill-formed.
    template<typename _Fn,
             typename... _Args>
    struct call_result_helper
    {
    private:
        template<typename _F>
        static auto test(int) -> decltype(
            std::declval<const _F&>()(std::declval<_Args>()...));

        template<typename>
        static call_nonesuch test(...);

    public:
        using type = decltype(test<_Fn>(0));
    };

NS_END  // internal


// call_result_t
//   alias: the result of calling a const-lvalue _Fn on _Args, or
// internal::call_nonesuch when the call is ill-formed. Unlike return_type_t,
// this works on generic lambdas and templated operator() because it probes a
// concrete call rather than inspecting a declared signature.
template<typename _Fn,
         typename... _Args>
using call_result_t =
    typename internal::call_result_helper<
        typename std::decay<_Fn>::type, _Args...>::type;


// is_invocable_with
//   trait: true when a const-lvalue _Fn can be called on _Args. The "_with"
// suffix avoids any clash with a std::is_invocable-style name and signals
// that the argument types are supplied explicitly.
template<typename _Fn,
         typename... _Args>
struct is_invocable_with
{
    static D_CONSTEXPR bool value =
        !std::is_same<call_result_t<_Fn, _Args...>,
                      internal::call_nonesuch>::value;
};


// is_invocable_r_with
//   trait: true when a const-lvalue _Fn can be called on _Args AND the
// result is convertible to _Return. A void _Return matches any successful
// call (mirroring the standard is_invocable_r treatment of void).
template<typename _Return,
         typename _Fn,
         typename... _Args>
struct is_invocable_r_with
{
private:
    using result_t = call_result_t<_Fn, _Args...>;

    static D_CONSTEXPR bool callable =
        !std::is_same<result_t, internal::call_nonesuch>::value;

public:
    static D_CONSTEXPR bool value =
        ( callable &&
          ( std::is_void<_Return>::value ||
            std::is_convertible<result_t, _Return>::value ) );
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_invocable_with_v
//   constant: shorthand for is_invocable_with<_Fn, _Args...>::value.
template<typename _Fn,
         typename... _Args>
static constexpr bool is_invocable_with_v =
    is_invocable_with<_Fn, _Args...>::value;

// is_invocable_r_with_v
//   constant: shorthand for is_invocable_r_with<_Return, _Fn, _Args...>.
template<typename _Return,
         typename _Fn,
         typename... _Args>
static constexpr bool is_invocable_r_with_v =
    is_invocable_r_with<_Return, _Fn, _Args...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FUNCTION_TRAITS_
