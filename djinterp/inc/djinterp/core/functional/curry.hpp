/*******************************************************************************
* djinterp [core]                                                      curry.hpp
*
* Function currying, uncurrying, and related combinators (C++).
*   Provides type-safe, SFINAE-constrained currying for callables of any
* arity. Unlike fixed-arity-only solutions, curry() auto-detects whether
* enough arguments have been supplied and either invokes the function or
* returns a new curried object awaiting more arguments.
*   For callables whose arity cannot be inferred (e.g. overloaded
* operator(), templates), curry_n<N>(f) takes an explicit arity.
*   Complementary primitives are provided: identity, always (a.k.a.
* constant), flip (swap first two arguments), uncurry (convert curried
* form back to multi-argument), and never (constant-false predicate).
*
* USAGE:
*   auto add = [](int a, int b, int c){ return a + b + c; };
*   auto c   = curry(add);
*   auto r1  = c(1)(2)(3);          // 6
*   auto r2  = c(1, 2)(3);          // 6
*   auto r3  = c(1, 2, 3);          // 6
*
*   // explicit arity (recommended for overloaded callables)
*   auto cn  = curry_n<3>(add);
*   auto r4  = cn(1)(2)(3);
*
*   // flip first two args
*   auto sub      = [](int a, int b){ return a - b; };
*   auto flipped  = flip(sub);
*   flipped(3, 10);                 // 7  (10 - 3)
*
*   // constant
*   auto five = always(5);
*   five();                         // 5
*   five(1, 2, "x");                // 5  (ignores all args)
*
*   DUAL-DOMAIN (compile-time + runtime): the curry helpers store the callable
* and accumulated arguments by value (decayed) and every operator()/dispatch is
* constexpr, forwarding without inspecting the operand domain.  With
* carrier-callable functions (see core/meta/carrier.hpp) curry / curry_n / flip /
* uncurry / always therefore FOLD AT COMPILE TIME over NTTP value carriers and
* type carriers, as well as running unchanged at runtime:
*     curry_n<2>(addv)(val<3>)(val<4>)()       -> val_t<7>      (constexpr)
*     curry(pair_fn)(type_c<int>)(type_c<char>) -> type_t<pair<int,char>>
* (auto-curry invokes on the final argument; the explicit curry_n terminal is
* invoked with a trailing () or via its implicit conversion to the result type.)
* Section II adds the arity concepts to the module's predicate traits, which
* are functional_common.hpp's (is_predicate and its arity forms).
*
*
* path:      /inc/djinterp/core/functional/curry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL MACHINERY
      ------------------
      1.    index_seq / make_index_seq          (C++11 index_sequence fallback)
      2.    apply_tuple                         (tuple-to-args invocation)
      3.    curry_helper                        (variadic curry accumulator)
      4.    curry_n_helper                      (fixed-arity curry accumulator)
      5.    flip_helper                         (argument swap)
      6.    always_helper                       (constant function)
      7.    uncurry_helper                      (curried -> n-ary)

II.   PREDICATE CONCEPTS                 (C++20; the traits are
      ------------------                  functional_common.hpp's)
      1.    nullary_predicate                   (arity-0 predicate)
      2.    unary_predicate                     (arity-1 predicate)
      3.    binary_predicate                    (arity-2 predicate)

III.  CURRY FACTORIES
      ---------------
      1.    curry                               (auto-arity)
      2.    curry_n                             (fixed-arity, by template)

IV.   UNCURRYING
      ----------
      1.    uncurry                             (back to multi-arg form)

V.    ARGUMENT TRANSFORMATIONS
      ------------------------
      1.    flip                                (swap first two args)

VI.   CONSTANT-VALUED COMBINATORS
      ---------------------------
      1.    identity                            (returns its argument)
      2.    always (constant)                   (returns a fixed value)
      3.    never                               (always-false predicate)
*/

#ifndef DJINTERP_FUNCTIONAL_CURRY_HPP
#define DJINTERP_FUNCTIONAL_CURRY_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./functional_common.hpp"  // is_predicate and its arity forms,
                                    // Predicate
// re_std
#include "../../../re_std/utility/make_integer_sequence.hpp"  // re_std::index_sequence,
                                                              // make_index_sequence


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    INTERNAL MACHINERY                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // index_seq
    //   helper: C++11 stand-in for re_std::index_sequence (C++14).
    template<std::size_t... Is>
    struct index_seq
    {
        using type = index_seq;
    };

    // make_index_seq_helper
    //   helper: recursive builder for index_seq<0, 1, ..., N - 1>.
    template<std::size_t    N,
             std::size_t... Is>
    struct make_index_seq_helper
        : make_index_seq_helper<N - 1, N - 1, Is...>
{};

    // make_index_seq_helper base case
    //   helper: terminates the recursion at zero.
    template<std::size_t... Is>
    struct make_index_seq_helper<0, Is...>
    {
        using type = index_seq<Is...>;
    };

    // make_index_seq
    //   alias: builds an index_seq<0, 1, ..., N - 1>.
    template<std::size_t N>
    using make_index_seq = typename make_index_seq_helper<N>::type;


    // apply_tuple_helper
    //   helper: invokes _fn with the elements of _tuple expanded as
    // arguments, using an index pack to perform the expansion.
    template<typename       Fn,
             typename       Tuple,
             std::size_t... Is>
    D_CONSTEXPR auto
    apply_tuple_helper(
        Fn&&    _fn,
        Tuple&& _tuple,
        index_seq<Is...>
    )
    -> decltype(std::forward<Fn>(_fn)(
           std::get<Is>(std::forward<Tuple>(_tuple))...))
    {
        return std::forward<Fn>(_fn)(
            std::get<Is>(std::forward<Tuple>(_tuple))...);
    }

    // apply_tuple
    //   helper: invokes _fn with the elements of _tuple as arguments.
    // C++11/14 stand-in for std::apply (C++17).
    template<typename Fn,
             typename Tuple>
    D_CONSTEXPR_CPP14
    auto apply_tuple(
        Fn&&    _fn,
        Tuple&& _tuple
    )
    -> decltype(apply_tuple_helper(
           std::forward<Fn>(_fn),
           std::forward<Tuple>(_tuple),
           make_index_seq<std::tuple_size<
               typename std::decay<Tuple>::type>::value>{}))
    {
        return apply_tuple_helper(
            std::forward<Fn>(_fn),
            std::forward<Tuple>(_tuple),
            make_index_seq<std::tuple_size<
                typename std::decay<Tuple>::type>::value>{});
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // curry_helper
    //   helper: stores a callable and a tuple of accumulated arguments.
    // On each invocation, dispatches via SFINAE: if the function is
    // invocable with the accumulated arguments plus the new one,
    // the function is called; otherwise a new curry_helper is returned
    // with the argument appended.
    template<typename    Fn,
             typename... Args>
    class curry_helper
    {
    public:
        // construct from forwarded function and an arg tuple
        template<typename FnFwd,
                 typename TupleFwd>
        D_CONSTEXPR
        curry_helper(
            FnFwd&&    _fn,
            TupleFwd&& _args
        )
            : m_fn(std::forward<FnFwd>(_fn))
            , m_args(std::forward<TupleFwd>(_args))
        {}

        // Return-type metafunctions for operator(). std::conditional is
        // eager -- it requires BOTH branch types to be well-formed -- so a
        // direct conditional on apply_tuple(...) fails when the callable is
        // not yet invocable with the given args (the invoke branch is still
        // instantiated). These wrappers defer each branch behind ::type so
        // only the selected one is evaluated. (fixed 2026-05-30)
        template<typename... New>
        struct invoke_result_of
        {
            using type = decltype(internal::apply_tuple(
                std::declval<const Fn&>(),
                std::tuple_cat(
                    std::declval<const std::tuple<Args...>&>(),
                    std::forward_as_tuple(std::declval<New>()...))));
        };

        template<typename... New>
        struct extend_result_of
        {
            using type = curry_helper<Fn, Args...,
                                      typename std::decay<New>::type...>;
        };

        // operator() (one new argument)
        //   chooses between invocation and extension via tag dispatch.
        template<typename Arg>
        D_CONSTEXPR
        auto operator()(Arg&& _arg) const
        -> typename std::conditional<
               is_invocable<Fn, Args..., Arg>::value,
               invoke_result_of<Arg>,
               extend_result_of<Arg> >::type::type
        {
            return dispatch(
                std::forward<Arg>(_arg),
                typename is_invocable<Fn, Args..., Arg>::type{});
        }

        // operator() (no new arguments)
        //   invokes with the currently-stored arguments. Valid only
        // when the stored arguments are already sufficient.
        template<typename F = Fn>
        D_CONSTEXPR
        auto operator()() const
        -> decltype(internal::apply_tuple(
               std::declval<const F&>(),
               std::declval<const std::tuple<Args...>&>()))
        {
            return internal::apply_tuple(m_fn, m_args);
        }

        // operator() (multiple new arguments)
        //   extension path: appends all new args at once, then either
        // invokes (if now callable) or returns the extended helper.
        template<typename A0,
                 typename A1,
                 typename... Rest>
        D_CONSTEXPR
        auto operator()(
            A0&&     _a0,
            A1&&     _a1,
            Rest&&...  _rest
        ) const
        -> typename std::conditional<
               is_invocable<Fn, Args..., A0, A1, Rest...>::value,
               invoke_result_of<A0, A1, Rest...>,
               extend_result_of<A0, A1, Rest...> >::type::type
        {
            return call_multi(
                typename is_invocable<
                    Fn, Args..., A0, A1, Rest...>::type{},
                std::forward<A0>(_a0),
                std::forward<A1>(_a1),
                std::forward<Rest>(_rest)...);
        }

    private:
        // dispatch (true_type)
        //   path taken when the stored args plus the new arg are
        // sufficient to invoke; the call happens here.
        template<typename Arg>
        D_CONSTEXPR
        auto dispatch(
            Arg&& _arg,
            std::true_type
        ) const
        -> decltype(internal::apply_tuple(
               std::declval<const Fn&>(),
               std::tuple_cat(
                   std::declval<const std::tuple<Args...>&>(),
                   std::make_tuple(std::forward<Arg>(_arg)))))
        {
            return internal::apply_tuple(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<Arg>(_arg))));
        }

        // dispatch (false_type)
        //   path taken when more arguments are still required; a new
        // curry_helper holding the appended arg is returned.
        template<typename Arg>
        D_CONSTEXPR
        curry_helper<Fn, Args..., typename std::decay<Arg>::type>
        dispatch(
            Arg&& _arg,
            std::false_type
        ) const
        {
            return curry_helper<Fn, Args...,
                                typename std::decay<Arg>::type>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<Arg>(_arg))));
        }

        // call_multi (true_type)
        //   invoke immediately with the full argument list.
        template<typename... NewArgs>
        D_CONSTEXPR
        auto call_multi(
            std::true_type,
            NewArgs&&... _new_args
        ) const
        -> decltype(internal::apply_tuple(
               std::declval<const Fn&>(),
               std::tuple_cat(
                   std::declval<const std::tuple<Args...>&>(),
                   std::forward_as_tuple(
                       std::forward<NewArgs>(_new_args)...))))
        {
            return internal::apply_tuple(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::forward_as_tuple(
                        std::forward<NewArgs>(_new_args)...)));
        }

        // call_multi (false_type)
        //   build a new helper with all new args appended.
        template<typename... NewArgs>
        D_CONSTEXPR
        curry_helper<Fn,
                     Args...,
                     typename std::decay<NewArgs>::type...>
        call_multi(
            std::false_type,
            NewArgs&&... _new_args
        ) const
        {
            return curry_helper<Fn,
                                Args...,
                                typename std::decay<NewArgs>::type...>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(
                        std::forward<NewArgs>(_new_args)...)));
        }

        Fn                   m_fn;
        std::tuple<Args...> m_args;
    };


    // curry_n_helper
    //   helper: explicit-arity curry. Tracks the number of remaining
    // arguments at compile time and invokes the wrapped function
    // exactly when that count reaches zero. Resolves ambiguity for
    // callables whose effective arity cannot be deduced (overloads,
    // generic lambdas, std::function with default args, etc.).
    template<std::size_t Remaining,
             typename    Fn,
             typename... Args>
    class curry_n_helper
    {
    public:
        template<typename FnFwd,
                 typename TupleFwd>
        D_CONSTEXPR
        curry_n_helper(
            FnFwd&&    _fn,
            TupleFwd&& _args
        )
            : m_fn(std::forward<FnFwd>(_fn))
            , m_args(std::forward<TupleFwd>(_args))
        {}

        // operator() (one argument)
        //   appends one argument and returns a helper with arity
        // Remaining - 1.
        template<typename Arg>
        D_CONSTEXPR
        curry_n_helper<Remaining - 1,
                       Fn,
                       Args...,
                       typename std::decay<Arg>::type>
        operator()(Arg&& _arg) const
        {
            return curry_n_helper<Remaining - 1, Fn, Args...,
                                  typename std::decay<Arg>::type>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<Arg>(_arg))));
        }

        // operator() (two or more arguments) -- implements the documented
        // c(1, 2)(3) / c(1, 2, 3) forms the single-arg overload omitted.
        // (added 2026-05-30)
        template<typename Arg0,
                 typename Arg1,
                 typename... More>
        D_CONSTEXPR
        curry_n_helper<Remaining - 2 - sizeof...(More),
                       Fn,
                       Args...,
                       typename std::decay<Arg0>::type,
                       typename std::decay<Arg1>::type,
                       typename std::decay<More>::type...>
        operator()(Arg0&& _a0, Arg1&& _a1, More&&... _more) const
        {
            return curry_n_helper<Remaining - 2 - sizeof...(More), Fn,
                       Args...,
                       typename std::decay<Arg0>::type,
                       typename std::decay<Arg1>::type,
                       typename std::decay<More>::type...>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<Arg0>(_a0),
                                    std::forward<Arg1>(_a1),
                                    std::forward<More>(_more)...)));
        }

    private:
        Fn                   m_fn;
        std::tuple<Args...> m_args;
    };


    // curry_n_helper (zero-arity terminal)
    //   helper: specialization for when no further arguments are
    // expected. Provides operator() that performs the invocation
    // with the collected arguments.
    template<typename    Fn,
             typename... Args>
    class curry_n_helper<0, Fn, Args...>
    {
    public:
        template<typename FnFwd,
                 typename TupleFwd>
        D_CONSTEXPR
        curry_n_helper(
            FnFwd&&    _fn,
            TupleFwd&& _args
        )
            : m_fn(std::forward<FnFwd>(_fn))
            , m_args(std::forward<TupleFwd>(_args))
        {}

        // operator() (no arguments)
        //   invokes the stored function with the accumulated args.
        D_CONSTEXPR
        auto operator()() const
        -> decltype(internal::apply_tuple(
               std::declval<const Fn&>(),
               std::declval<const std::tuple<Args...>&>()))
        {
            return internal::apply_tuple(m_fn, m_args);
        }

        // implicit conversion to the result type (for terminal helpers
        // that have already accumulated all args; lets users write
        // `int x = curry_n<2>(f)(1)(2);` without an explicit final ()).
        template<typename R = decltype(internal::apply_tuple(
                     std::declval<const Fn&>(),
                     std::declval<const std::tuple<Args...>&>()))>
        D_CONSTEXPR
        operator R() const
        {
            return internal::apply_tuple(m_fn, m_args);
        }

    private:
        Fn                   m_fn;
        std::tuple<Args...> m_args;
    };


    // flip_helper
    //   helper: holds a callable and exposes operator() that swaps the
    // first two arguments before forwarding. Additional arguments are
    // passed through in their original order.
    template<typename Fn>
    class flip_helper
    {
    public:
        template<typename FnFwd>
        explicit D_CONSTEXPR
        flip_helper(
            FnFwd&& _fn
        )
            : m_fn(std::forward<FnFwd>(_fn))
        {}

        // operator() (two arguments)
        template<typename A,
                 typename B>
        D_CONSTEXPR
        auto operator()(
            A&& _a,
            B&& _b
        ) const
        -> decltype(std::declval<const Fn&>()(
               std::forward<B>(_b),
               std::forward<A>(_a)))
        {
            return m_fn(std::forward<B>(_b), std::forward<A>(_a));
        }

        // operator() (three or more arguments)
        //   swaps only the first two; the remainder pass through.
        template<typename A,
                 typename B,
                 typename C,
                 typename... Rest>
        D_CONSTEXPR
        auto operator()(
            A&&        _a,
            B&&        _b,
            C&&        _c,
            Rest&&...  _rest
        ) const
        -> decltype(std::declval<const Fn&>()(
               std::forward<B>(_b),
               std::forward<A>(_a),
               std::forward<C>(_c),
               std::forward<Rest>(_rest)...))
        {
            return m_fn(std::forward<B>(_b),
                        std::forward<A>(_a),
                        std::forward<C>(_c),
                        std::forward<Rest>(_rest)...);
        }

    private:
        Fn m_fn;
    };


    // always_helper
    //   helper: ignores all arguments and returns a stored value. Used
    // for `always(x)` / `constant(x)`.
    template<typename Value>
    class always_helper
    {
    public:
        template<typename ValueFwd>
        explicit D_CONSTEXPR
        always_helper(
            ValueFwd&& _value
        )
            : m_value(std::forward<ValueFwd>(_value))
        {}

        template<typename... Args>
        D_CONSTEXPR
        const Value& operator()(Args&&...) const
        {
            return m_value;
        }

    private:
        Value m_value;
    };


    // uncurry_helper
    //   helper: wraps a fully-curried function so it can be called in
    // multi-argument form again.  `uncurry(f)(a, b, c)` becomes
    // `f(a)(b)(c)`.
    template<typename Fn>
    class uncurry_helper
    {
    public:
        template<typename FnFwd>
        explicit D_CONSTEXPR
        uncurry_helper(
            FnFwd&& _fn
        )
            : m_fn(std::forward<FnFwd>(_fn))
        {}

        // operator() (single argument)
        template<typename Arg>
        D_CONSTEXPR
        auto operator()(Arg&& _arg) const
        -> decltype(std::declval<const Fn&>()(std::forward<Arg>(_arg)))
        {
            return m_fn(std::forward<Arg>(_arg));
        }

        // operator() (two or more arguments)
        //   applies the function to the first argument, then recurses
        // on the result with the remaining arguments. This expresses
        // uncurry recursively in terms of itself.
        template<typename    A,
                 typename    B,
                 typename... Rest>
        D_CONSTEXPR
        auto operator()(
            A&&        _a,
            B&&        _b,
            Rest&&...  _rest
        ) const
        -> decltype(uncurry_helper<decltype(
                       std::declval<const Fn&>()(
                           std::forward<A>(_a)))>(
                       std::declval<const Fn&>()(std::forward<A>(_a)))(
                           std::forward<B>(_b),
                           std::forward<Rest>(_rest)...))
        {
            return uncurry_helper<decltype(
                m_fn(std::forward<A>(_a)))>(
                    m_fn(std::forward<A>(_a)))(
                        std::forward<B>(_b),
                        std::forward<Rest>(_rest)...);
        }

    private:
        Fn m_fn;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   PREDICATE CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////
// A predicate is any callable whose result is convertible to bool. The traits
// that classify one -- is_predicate and its arity forms is_nullary_predicate,
// is_unary_predicate and is_binary_predicate, with their _v shorthands -- are
// the functional module's, defined once in functional_common.hpp, which this
// header includes; the general concept is its Predicate. This header used to
// define the traits itself, and a predicate_for concept the same as Predicate,
// which no translation unit could include beside functional_common.hpp,
// consumer.hpp or comparator.hpp. What it adds is the arity concepts.
// (added 2026-05-31)

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// nullary_predicate
//   concept: satisfied when Fn is a predicate accepting no arguments.
template<typename Fn>
concept nullary_predicate = Predicate<Fn>;

// unary_predicate
//   concept: satisfied when Fn is a predicate accepting exactly one
// argument of type Arg.
template<typename Fn,
         typename Arg>
concept unary_predicate = Predicate<Fn, Arg>;

// binary_predicate
//   concept: satisfied when Fn is a predicate accepting exactly two
// arguments of types A and B (in that order).
template<typename Fn,
         typename A,
         typename B>
concept binary_predicate = Predicate<Fn, A, B>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             III.  CURRY FACTORIES                                       ///
///////////////////////////////////////////////////////////////////////////////

// curry
//   function: wraps _fn in an auto-arity curried form. Each call
// either invokes the function (when the accumulated arguments are
// sufficient) or returns a new curried object accepting more
// arguments. Resolution is by SFINAE on is_invocable.
//
//   Limitations: for callables whose invocability changes with arity
// (e.g. overloaded function objects, generic lambdas that work for
// multiple arities), prefer curry_n<N> to disambiguate.
template<typename Fn>
D_CONSTEXPR
internal::curry_helper<typename std::decay<Fn>::type>
curry
(
    Fn&& _fn
)
{
    return internal::curry_helper<typename std::decay<Fn>::type>(
        std::forward<Fn>(_fn),
        std::tuple<>{});
}


// curry_n
//   function: explicit-arity curry. Builds a curried form that
// requires exactly N more arguments before invocation. Useful
// when the wrapped callable's arity cannot be reliably inferred.
template<std::size_t N,
         typename    Fn>
D_CONSTEXPR
internal::curry_n_helper<N, typename std::decay<Fn>::type>
curry_n
(
    Fn&& _fn
)
{
    return internal::curry_n_helper<N, typename std::decay<Fn>::type>(
        std::forward<Fn>(_fn),
        std::tuple<>{});
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   UNCURRYING                                            ///
///////////////////////////////////////////////////////////////////////////////

// uncurry
//   function: produces a wrapper for a curried function that
// accepts all arguments at once. uncurry(f)(a, b, c) is equivalent
// to f(a)(b)(c). For functions that are not curried, the wrapper
// degenerates to a plain call.
template<typename Fn>
D_CONSTEXPR
internal::uncurry_helper<typename std::decay<Fn>::type>
uncurry
(
    Fn&& _fn
)
{
    return internal::uncurry_helper<typename std::decay<Fn>::type>(
        std::forward<Fn>(_fn));
}


///////////////////////////////////////////////////////////////////////////////
///             V.    ARGUMENT TRANSFORMATIONS                              ///
///////////////////////////////////////////////////////////////////////////////

// flip
//   function: returns a wrapper that swaps the first two arguments
// of the wrapped callable before invocation. Useful for adapting
// callables to expected argument orders (e.g. when a library wants
// `cmp(b, a)` but you have `cmp(a, b)`).
template<typename Fn>
D_CONSTEXPR
internal::flip_helper<typename std::decay<Fn>::type>
flip
(
    Fn&& _fn
)
{
    return internal::flip_helper<typename std::decay<Fn>::type>(
        std::forward<Fn>(_fn));
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   CONSTANT-VALUED COMBINATORS                           ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // identity_fn_helper
    //   helper: invokable type that returns its argument unchanged,
    // preserving value category and constness.
    //   NOTE: named identity_fn_helper rather than identity_helper to
    // avoid an ODR clash with extractor.hpp's internal::identity_helper
    // <Source> (a different template) when both headers are used in the
    // same translation unit. (renamed 2026-05-30)
    struct identity_fn_helper
    {
        template<typename Arg>
        D_CONSTEXPR
        Arg&& operator()(
            Arg&& _arg
        ) const noexcept
        {
            return std::forward<Arg>(_arg);
        }
    };


    // never_helper
    //   helper: invokable type that returns false for any arguments.
    // Useful as a predicate seed for combinators.
    struct never_helper
    {
        template<typename... Args>
        D_CONSTEXPR
        bool operator()(Args&&...) const noexcept
        {
            return false;
        }
    };

NS_END  // internal


// identity
//   constant: function object that returns its argument unchanged.
// Useful as a default transformer in higher-order code.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static constexpr internal::identity_fn_helper identity{};
#else
    static const internal::identity_fn_helper identity = internal::identity_fn_helper{};
#endif


// always
//   function: creates a callable that ignores its arguments and
// returns the stored value. Equivalent to the K combinator.
//   always(x)(...)  ==  x
template<typename Value>
D_CONSTEXPR
internal::always_helper<typename std::decay<Value>::type>
always
(
    Value&& _value
)
{
    return internal::always_helper<typename std::decay<Value>::type>(
        std::forward<Value>(_value));
}


// constant
//   function: alias for always, provided for readability in code
// that conceptually requires a constant function rather than a
// "always returns the same value" function.
template<typename Value>
D_CONSTEXPR
internal::always_helper<typename std::decay<Value>::type>
constant
(
    Value&& _value
)
{
    return always(std::forward<Value>(_value));
}


// never
//   constant: predicate that returns false for any input.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static constexpr internal::never_helper never{};
#else
    static const internal::never_helper never = internal::never_helper{};
#endif

NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_CURRY_HPP
