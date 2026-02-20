/******************************************************************************
* djinterp [core]                                                functional.hpp
*
* Functional programming types and templates for C++.
*   This header provides common functional programming patterns (predicate,
* consumer, producer, etc.) and composition utilities.
*   All types are compile-time only with zero runtime overhead.
*
* NAMING CONVENTIONS:
*   predicate<T>       - returns bool, takes T
*   consumer<T>        - returns void, takes T
*   producer<T>        - returns T, takes nothing
*   supplier<T>        - alias for producer<T>
*   transformer<T,R>   - returns R, takes T
*   binary_op<T,R>     - returns R, takes T,T
*   comparator<T>      - returns int/bool, takes T,T
*
* DEPENDENCIES:
*   stl_functional.hpp - backported STL utilities (invoke, not_fn, etc.)
*
* 
* path:      \inc\functional.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_
#define DJINTERP_FUNCTIONAL_ 1

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include ".\djinterp.h"
#include ".\env.h"
#include ".\cpp_features.h"
#include ".\stl_functional.hpp"


// D_FUNCTIONAL_CONSTEXPR
//   macro: expands to constexpr if C++14 or higher (relaxed constexpr).
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_FUNCTIONAL_CONSTEXPR constexpr
#else
    #define D_FUNCTIONAL_CONSTEXPR
#endif

// D_FUNCTIONAL_CONSTEXPR_17
//   macro: expands to constexpr if C++17 or higher.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_FUNCTIONAL_CONSTEXPR_17 constexpr
#else
    #define D_FUNCTIONAL_CONSTEXPR_17
#endif

// D_FUNCTIONAL_CONSTEXPR_20
//   macro: expands to constexpr if C++20 or higher.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_FUNCTIONAL_CONSTEXPR_20 constexpr
#else
    #define D_FUNCTIONAL_CONSTEXPR_20
#endif

// D_FUNCTIONAL_NODISCARD
//   macro: expands to [[nodiscard]] if C++17 or higher.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_FUNCTIONAL_NODISCARD [[nodiscard]]
#else
    #define D_FUNCTIONAL_NODISCARD
#endif


NS_DJINTERP     // namespace djinterp
NS_FUNCTIONAL   // namespace functional

// I.   import STL backports into djinterp::functional namespace
//   (backported utilities from STL namespace for convenience)
using stl::invoke;
using stl::invoke_result;
using stl::invoke_result_t;
using stl::is_invocable;
using stl::is_invocable_r;
using stl::is_nothrow_invocable;
using stl::identity;
using stl::not_fn;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    using stl::is_invocable_v;
    using stl::is_invocable_r_v;
    using stl::is_nothrow_invocable_v;
    using stl::bind_front;
    using stl::bind_back;
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using stl::is_nothrow_invocable_r;
    using stl::is_nothrow_invocable_r_v;
#endif


// II.  functional programming type aliases

// predicate
//   type alias: callable that returns bool given a single argument.
// Common functional programming pattern for filtering and testing.
template<typename _Type>
using predicate = std::function<bool(const _Type&)>;

// binary_predicate
//   type alias: callable that returns bool given two arguments.
// Used for comparing, matching, and binary testing operations.
template<typename _Type1,
         typename _Type2 = _Type1>
using binary_predicate = std::function<bool(const _Type1&,
                                            const _Type2&)>;

// consumer
//   type alias: callable that accepts a value but returns nothing.
// Common for side-effect operations like printing or logging.
template<typename _Type>
using consumer = std::function<void(_Type)>;

// const_consumer
//   type alias: callable that accepts a const reference and returns nothing.
template<typename _Type>
using const_consumer = std::function<void(const _Type&)>;

// binary_consumer
//   type alias: callable that consumes two values and returns nothing.
template<typename _Type1,
         typename _Type2 = _Type1>
using binary_consumer = std::function<void(const _Type1&, const _Type2&)>;

// producer
//   type alias: callable that produces a value with no input.
// Also known as a supplier or generator.
template<typename _Type>
using producer = std::function<_Type()>;

// supplier
//   type alias: alias for producer (common Java naming convention).
template<typename _Type>
using supplier = producer<_Type>;

// generator
//   type alias: alias for producer (common C++ naming convention).
template<typename _Type>
using generator = producer<_Type>;

// transformer
//   type alias: callable that transforms input to output.
// Also known as a mapper or unary function.
template<typename _Input,
         typename _Output>
using transformer = std::function<_Output(const _Input&)>;

// mapper
//   type alias: alias for transformer (common Java naming convention).
template<typename _Input,
         typename _Output>
using mapper = transformer<_Input, _Output>;

// unary_function
//   type alias: callable with one input and one output.
// Replacement for deprecated std::unary_function.
template<typename _Arg,
         typename _Result>
using unary_function = std::function<_Result(_Arg)>;

// binary_function
//   type alias: callable with two inputs and one output.
// Replacement for deprecated std::binary_function.
template<typename _Arg1,
         typename _Arg2,
         typename _Result>
using binary_function = std::function<_Result(_Arg1, _Arg2)>;

// binary_op
//   type alias: binary operation producing result of same type.
template<typename _Type,
         typename _Result = _Type>
using binary_op = std::function<_Result(const _Type&, const _Type&)>;

// unary_op
//   type alias: unary operation producing result of same type.
template<typename _Type,
         typename _Result = _Type>
using unary_op = std::function<_Result(const _Type&)>;

// accumulator
//   type alias: combines accumulated value with new element.
// Used in fold/reduce operations.
template<typename _Accumulated,
         typename _Element,
         typename _Result = _Accumulated>
using accumulator = std::function<_Result(const _Accumulated&, 
                                          const _Element&)>;

// reducer
//   type alias: alias for accumulator with same input/output type.
template<typename _Type>
using reducer = accumulator<_Type, _Type, _Type>;

// comparator
//   type alias: three-way comparison returning negative/zero/positive.
// For sorting and ordering operations.
template<typename _Type>
using comparator = std::function<int(const _Type&, 
                                     const _Type&)>;

// equality_comparer
//   type alias: tests two values for equality.
template<typename _Type>
using equality_comparer = binary_predicate<_Type, _Type>;

// hasher
//   type alias: computes hash value for an object.
template<typename _Type>
using hasher = std::function<std::size_t(const _Type&)>;

// indexer
//   type alias: retrieves element by index from a collection.
template<typename _Collection,
         typename _Index,
         typename _Element>
using indexer = std::function<_Element(const _Collection&, 
                              _Index)>;

// factory
//   type alias: creates instances with given arguments.
template<typename _Type,
         typename... _Args>
using factory = std::function<_Type(_Args...)>;

// destructor_fn
//   type alias: cleanup function for a type.
template<typename _Type>
using destructor_fn = std::function<void(_Type*)>;

// deleter
//   type alias: alias for destructor_fn (common STL naming).
template<typename _Type>
using deleter = destructor_fn<_Type>;

// callback
//   type alias: generic callback with optional context.
template<typename _Result = void,
         typename... _Args>
using callback = std::function<_Result(_Args...)>;

// action
//   type alias: parameterless operation returning void.
using action = std::function<void()>;

// runnable
//   type alias: alias for action (common Java naming convention).
using runnable = action;

// thunk
//   type alias: alias for action (functional programming convention).
using thunk = action;

// procedure
//   type alias: alias for action.
using procedure = action;


// III.   composition utility

NS_INTERNAL

    // compose_helper
    //   helper: implementation for function composition.
    template<typename _Fn1,
             typename _Fn2>
    class compose_helper
    {
    private:
        _Fn1 m_fn1;
        _Fn2 m_fn2;

    public:
        template<typename _Fn1Fwd,
                 typename _Fn2Fwd>
        D_FUNCTIONAL_CONSTEXPR
            compose_helper(_Fn1Fwd&& _fn1, _Fn2Fwd&& _fn2)
            : m_fn1(std::forward<_Fn1Fwd>(_fn1))
            , m_fn2(std::forward<_Fn2Fwd>(_fn2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) const
            -> decltype(std::declval<const _Fn1&>()(
                std::declval<const _Fn2&>()(std::forward<_Args>(_args)...)))
        {
            return m_fn1(m_fn2(std::forward<_Args>(_args)...));
        }

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args)
            -> decltype(std::declval<_Fn1&>()(
                std::declval<_Fn2&>()(std::forward<_Args>(_args)...)))
        {
            return m_fn1(m_fn2(std::forward<_Args>(_args)...));
        }
    };

NS_END  // internal

// compose
//   function: composes two functions such that compose(f, g)(x) = f(g(x)).
// Mathematical function composition.
template<typename _Fn1,
         typename _Fn2>
D_FUNCTIONAL_CONSTEXPR
internal::compose_helper<typename std::decay<_Fn1>::type,
                         typename std::decay<_Fn2>::type>
compose(_Fn1&& _fn1, _Fn2&& _fn2)
{
    return internal::compose_helper<typename std::decay<_Fn1>::type,
                                    typename std::decay<_Fn2>::type>(
            std::forward<_Fn1>(_fn1),
            std::forward<_Fn2>(_fn2));
}

// pipe
//   function: composes functions in left-to-right order.
// pipe(f, g)(x) = g(f(x)), opposite of compose.
template<typename _Fn1,
         typename _Fn2>
D_FUNCTIONAL_CONSTEXPR
auto pipe(_Fn1&& _fn1, _Fn2&& _fn2)
    -> decltype(compose(std::forward<_Fn2>(_fn2), std::forward<_Fn1>(_fn1)))
{
    return compose(std::forward<_Fn2>(_fn2), std::forward<_Fn1>(_fn1));
}


// IV.    constant and projection utilities
NS_INTERNAL

    // constant_helper
    //   helper: implementation for constant function.
    template<typename _Type>
    class constant_helper
    {
    private:
        _Type m_value;

    public:
        template<typename _TypeFwd>
        explicit D_FUNCTIONAL_CONSTEXPR
        constant_helper(_TypeFwd&& _value)
            : m_value(std::forward<_TypeFwd>(_value))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        const _Type& operator()(_Args&&...) const noexcept
        {
            return m_value;
        }
    };

NS_END  // internal

// constant
//   function: creates a function that always returns the given value.
// K combinator in lambda calculus: K x y = x
template<typename _Type>
D_FUNCTIONAL_CONSTEXPR
internal::constant_helper<typename std::decay<_Type>::type>
constant(_Type&& _value)
{
    return internal::constant_helper<typename std::decay<_Type>::type>(
        std::forward<_Type>(_value));
}

// always
//   function: alias for constant.
template<typename _Type>
D_FUNCTIONAL_CONSTEXPR
auto always(_Type&& _value)
    -> decltype(constant(std::forward<_Type>(_value)))
{
    return constant(std::forward<_Type>(_value));
}

// V.   partial application and argument manipulation

NS_INTERNAL
    // flip_helper
    //   helper: implementation for flipping binary function arguments.
    template<typename _Fn>
    class flip_helper
    {
    private:
        _Fn m_fn;

    public:
        template<typename _FnFwd>
        explicit D_FUNCTIONAL_CONSTEXPR
        flip_helper(_FnFwd&& _fn)
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        template<typename _Arg1,
                 typename _Arg2,
                 typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Arg1&& _arg1, _Arg2&& _arg2, _Args&&... _args) const
            -> decltype(std::declval<const _Fn&>()(
                std::forward<_Arg2>(_arg2),
                std::forward<_Arg1>(_arg1),
                std::forward<_Args>(_args)...))
        {
            return m_fn(std::forward<_Arg2>(_arg2),
                       std::forward<_Arg1>(_arg1),
                       std::forward<_Args>(_args)...);
        }

        template<typename _Arg1,
                 typename _Arg2,
                 typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Arg1&& _arg1, _Arg2&& _arg2, _Args&&... _args)
            -> decltype(std::declval<_Fn&>()(
                std::forward<_Arg2>(_arg2),
                std::forward<_Arg1>(_arg1),
                std::forward<_Args>(_args)...))
        {
            return m_fn(std::forward<_Arg2>(_arg2),
                       std::forward<_Arg1>(_arg1),
                       std::forward<_Args>(_args)...);
        }
    };

    // curry_helper
    //   helper: implementation for currying a binary function.
    template<typename _Fn,
             typename _Arg1>
    class curry_helper
    {
    private:
        _Fn   m_fn;
        _Arg1 m_arg1;

    public:
        template<typename _FnFwd,
                 typename _Arg1Fwd>
        D_FUNCTIONAL_CONSTEXPR
        curry_helper(_FnFwd&& _fn, _Arg1Fwd&& _arg1)
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_arg1(std::forward<_Arg1Fwd>(_arg1))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) const
            -> decltype(std::declval<const _Fn&>()(
                std::declval<const _Arg1&>(),
                std::forward<_Args>(_args)...))
        {
            return m_fn(m_arg1, std::forward<_Args>(_args)...);
        }

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args)
            -> decltype(std::declval<_Fn&>()(
                std::declval<_Arg1&>(),
                std::forward<_Args>(_args)...))
        {
            return m_fn(m_arg1, std::forward<_Args>(_args)...);
        }
    };
NS_END  // internal

// flip
//   function: creates a function with its first two arguments swapped.
// flip(f)(a, b, c...) = f(b, a, c...)
template<typename _Fn>
D_FUNCTIONAL_CONSTEXPR
internal::flip_helper<typename std::decay<_Fn>::type>
flip(_Fn&& _fn)
{
    return internal::flip_helper<typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn));
}

// curry
//   function: partially applies the first argument of a function.
// curry(f, x)(y, z...) = f(x, y, z...)
// Note: this is a simplified curry that binds only the first argument.
// For full currying, use bind_front.
template<typename _Fn,
         typename _Arg1>
D_FUNCTIONAL_CONSTEXPR
internal::curry_helper<typename std::decay<_Fn>::type,
                       typename std::decay<_Arg1>::type>
curry(_Fn&& _fn, _Arg1&& _arg1)
{
    return internal::curry_helper<
        typename std::decay<_Fn>::type,
        typename std::decay<_Arg1>::type>(
            std::forward<_Fn>(_fn),
            std::forward<_Arg1>(_arg1));
}


// VI.  function combinations

NS_INTERNAL
    // apply_all_helper
    //   helper: applies a function to all arguments and returns a tuple.
    template<typename _Fn>
    class apply_all_helper
    {
    private:
        _Fn m_fn;

    public:
        template<typename _FnFwd>
        explicit D_FUNCTIONAL_CONSTEXPR
        apply_all_helper(_FnFwd&& _fn)
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) const
            -> std::tuple<decltype(std::declval<const _Fn&>()(
                std::forward<_Args>(_args)))...>
        {
            return std::make_tuple(m_fn(std::forward<_Args>(_args))...);
        }
    };

    // on_helper
    //   helper: applies a projection before a binary operation.
    template<typename _Fn,
             typename _Proj>
    class on_helper
    {
    private:
        _Fn   m_fn;
        _Proj m_proj;

    public:
        template<typename _FnFwd,
                 typename _ProjFwd>
        D_FUNCTIONAL_CONSTEXPR
        on_helper(_FnFwd&& _fn, _ProjFwd&& _proj)
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_proj(std::forward<_ProjFwd>(_proj))
        {}

        template<typename _Arg1,
                 typename _Arg2>
        D_FUNCTIONAL_CONSTEXPR
        auto operator()(_Arg1&& _arg1, _Arg2&& _arg2) const
            -> decltype(std::declval<const _Fn&>()(
                std::declval<const _Proj&>()(std::forward<_Arg1>(_arg1)),
                std::declval<const _Proj&>()(std::forward<_Arg2>(_arg2))))
        {
            return m_fn(m_proj(std::forward<_Arg1>(_arg1)),
                       m_proj(std::forward<_Arg2>(_arg2)));
        }
    };
NS_END  // internal

// apply_all
//   function: creates a function that applies fn to each argument.
// apply_all(f)(a, b, c) = tuple(f(a), f(b), f(c))
template<typename _Fn>
D_FUNCTIONAL_CONSTEXPR
internal::apply_all_helper<typename std::decay<_Fn>::type>
apply_all(_Fn&& _fn)
{
    return internal::apply_all_helper<typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn));
}

// on
//   function: applies a projection to both arguments before a binary operation.
// on(f, p)(a, b) = f(p(a), p(b))
// Useful for comparing by a specific member/property.
template<typename _Fn,
         typename _Proj>
D_FUNCTIONAL_CONSTEXPR
internal::on_helper<typename std::decay<_Fn>::type,
                    typename std::decay<_Proj>::type>
on(_Fn&& _fn, _Proj&& _proj)
{
    return internal::on_helper<
        typename std::decay<_Fn>::type,
        typename std::decay<_Proj>::type>(
            std::forward<_Fn>(_fn),
            std::forward<_Proj>(_proj));
}


// VII. LOGICAL COMBINATORS

NS_INTERNAL

    // both_helper
    //   helper: implementation for logical AND of two predicates.
    template<typename _Pred1,
             typename _Pred2>
    class both_helper
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _Pred1Fwd,
                 typename _Pred2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        both_helper(_Pred1Fwd&& _pred1, _Pred2Fwd&& _pred2)
            : m_pred1(std::forward<_Pred1Fwd>(_pred1))
            , m_pred2(std::forward<_Pred2Fwd>(_pred2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return m_pred1(std::forward<_Args>(_args)...) &&
                   m_pred2(std::forward<_Args>(_args)...);
        }
    };

    // either_helper
    //   helper: implementation for logical OR of two predicates.
    template<typename _Pred1,
             typename _Pred2>
    class either_helper
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _Pred1Fwd,
                 typename _Pred2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        either_helper(_Pred1Fwd&& _pred1, _Pred2Fwd&& _pred2)
            : m_pred1(std::forward<_Pred1Fwd>(_pred1))
            , m_pred2(std::forward<_Pred2Fwd>(_pred2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return m_pred1(std::forward<_Args>(_args)...) ||
                   m_pred2(std::forward<_Args>(_args)...);
        }
    };

NS_END  // internal

// both
//   function: creates a predicate that is true when both predicates are true.
// both(p1, p2)(x) = p1(x) && p2(x)
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::both_helper<typename std::decay<_Pred1>::type,
                      typename std::decay<_Pred2>::type>
both(_Pred1&& _pred1, _Pred2&& _pred2)
{
    return internal::both_helper<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_pred1),
            std::forward<_Pred2>(_pred2));
}

// either
//   function: creates a predicate that is true when either predicate is true.
// either(p1, p2)(x) = p1(x) || p2(x)
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::either_helper<typename std::decay<_Pred1>::type,
                        typename std::decay<_Pred2>::type>
either(_Pred1&& _pred1, _Pred2&& _pred2)
{
    return internal::either_helper<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_pred1),
            std::forward<_Pred2>(_pred2));
}

// complement
//   function: alias for not_fn (negates a predicate).
// complement(p)(x) = !p(x)
template<typename _Pred>
D_FUNCTIONAL_CONSTEXPR
auto complement(_Pred&& _pred)
    -> decltype(not_fn(std::forward<_Pred>(_pred)))
{
    return not_fn(std::forward<_Pred>(_pred));
}


NS_END  // functional
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_
