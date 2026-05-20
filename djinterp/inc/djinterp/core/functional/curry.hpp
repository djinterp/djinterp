/******************************************************************************
* djinterp [functional]                                              curry.hpp
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
* path:      /inc/djinterp/core/functional/curry.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL MACHINERY
      1.  index_seq / make_index_seq          (C++11 index_sequence fallback)
      2.  apply_tuple                         (tuple-to-args invocation)
      3.  curry_helper                        (variadic curry accumulator)
      4.  curry_n_helper                      (fixed-arity curry accumulator)
      5.  flip_helper                         (argument swap)
      6.  always_helper                       (constant function)
      7.  uncurry_helper                      (curried -> n-ary)
II.   CURRY FACTORIES
      1.  curry                               (auto-arity)
      2.  curry_n                             (fixed-arity, by template)
III.  UNCURRYING
      1.  uncurry                             (back to multi-arg form)
IV.   ARGUMENT TRANSFORMATIONS
      1.  flip                                (swap first two args)
V.    CONSTANT-VALUED COMBINATORS
      1.  identity                            (returns its argument)
      2.  always (constant)                   (returns a fixed value)
      3.  never                               (always-false predicate)
*/

#ifndef DJINTERP_FUNCTIONAL_CURRY_
#define DJINTERP_FUNCTIONAL_CURRY_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    INTERNAL MACHINERY                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // index_seq
    //   helper: C++11 stand-in for std::index_sequence (C++14).
    template<std::size_t... _Is>
    struct index_seq
    {
        using type = index_seq;
    };

    // make_index_seq_helper
    //   helper: recursive builder for index_seq<0, 1, ..., _N - 1>.
    template<std::size_t    _N,
             std::size_t... _Is>
    struct make_index_seq_helper
        : make_index_seq_helper<_N - 1, _N - 1, _Is...>
    {
    };

    // make_index_seq_helper base case
    //   helper: terminates the recursion at zero.
    template<std::size_t... _Is>
    struct make_index_seq_helper<0, _Is...>
    {
        using type = index_seq<_Is...>;
    };

    // make_index_seq
    //   alias: builds an index_seq<0, 1, ..., _N - 1>.
    template<std::size_t _N>
    using make_index_seq = typename make_index_seq_helper<_N>::type;


    // apply_tuple_helper
    //   helper: invokes _fn with the elements of _tuple expanded as
    // arguments, using an index pack to perform the expansion.
    template<typename       _Fn,
             typename       _Tuple,
             std::size_t... _Is>
    D_CONSTEXPR auto
    apply_tuple_helper(
        _Fn&&    _fn,
        _Tuple&& _tuple,
        index_seq<_Is...>
    )
    -> decltype(std::forward<_Fn>(_fn)(
           std::get<_Is>(std::forward<_Tuple>(_tuple))...))
    {
        return std::forward<_Fn>(_fn)(
            std::get<_Is>(std::forward<_Tuple>(_tuple))...);
    }

    // apply_tuple
    //   helper: invokes _fn with the elements of _tuple as arguments.
    // C++11/14 stand-in for std::apply (C++17).
    template<typename _Fn,
             typename _Tuple>
    D_CONSTEXPR
    auto apply_tuple
    (
        _Fn&&    _fn,
        _Tuple&& _tuple
    )
    -> decltype(apply_tuple_helper(
           std::forward<_Fn>(_fn),
           std::forward<_Tuple>(_tuple),
           make_index_seq<std::tuple_size<
               typename std::decay<_Tuple>::type>::value>{}))
    {
        return apply_tuple_helper(
            std::forward<_Fn>(_fn),
            std::forward<_Tuple>(_tuple),
            make_index_seq<std::tuple_size<
                typename std::decay<_Tuple>::type>::value>{});
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
    template<typename    _Fn,
             typename... _Args>
    class curry_helper
    {
    public:
        // construct from forwarded function and an arg tuple
        template<typename _FnFwd,
                 typename _TupleFwd>
        D_CONSTEXPR
        curry_helper(
            _FnFwd&&    _fn,
            _TupleFwd&& _args
        )
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_args(std::forward<_TupleFwd>(_args))
        {}

        // operator() (one new argument)
        //   chooses between invocation and extension via tag dispatch.
        template<typename _Arg>
        D_CONSTEXPR
        auto operator()(_Arg&& _arg) const
        -> decltype(dispatch(
               std::forward<_Arg>(_arg),
               typename is_invocable<_Fn, _Args..., _Arg>::type{}))
        {
            return dispatch(
                std::forward<_Arg>(_arg),
                typename is_invocable<_Fn, _Args..., _Arg>::type{});
        }

        // operator() (no new arguments)
        //   invokes with the currently-stored arguments. Valid only
        // when the stored arguments are already sufficient.
        template<typename _F = _Fn>
        D_CONSTEXPR
        auto operator()() const
        -> decltype(internal::apply_tuple(
               std::declval<const _F&>(),
               std::declval<const std::tuple<_Args...>&>()))
        {
            return internal::apply_tuple(m_fn, m_args);
        }

        // operator() (multiple new arguments)
        //   extension path: appends all new args at once, then either
        // invokes (if now callable) or returns the extended helper.
        // This is the same behavior as repeated single-arg calls but
        // is faster and produces cleaner error messages.
        template<typename _A0,
                 typename _A1,
                 typename... _Rest>
        D_CONSTEXPR
        auto operator()(
            _A0&&     _a0,
            _A1&&     _a1,
            _Rest&&...  _rest
        ) const
        -> decltype(call_multi(
               typename is_invocable<
                   _Fn, _Args..., _A0, _A1, _Rest...>::type{},
               std::forward<_A0>(_a0),
               std::forward<_A1>(_a1),
               std::forward<_Rest>(_rest)...))
        {
            return call_multi(
                typename is_invocable<
                    _Fn, _Args..., _A0, _A1, _Rest...>::type{},
                std::forward<_A0>(_a0),
                std::forward<_A1>(_a1),
                std::forward<_Rest>(_rest)...);
        }

    private:
        // dispatch (true_type)
        //   path taken when the stored args plus the new arg are
        // sufficient to invoke; the call happens here.
        template<typename _Arg>
        D_CONSTEXPR
        auto dispatch(
            _Arg&& _arg,
            std::true_type
        ) const
        -> decltype(internal::apply_tuple(
               std::declval<const _Fn&>(),
               std::tuple_cat(
                   std::declval<const std::tuple<_Args...>&>(),
                   std::make_tuple(std::forward<_Arg>(_arg)))))
        {
            return internal::apply_tuple(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<_Arg>(_arg))));
        }

        // dispatch (false_type)
        //   path taken when more arguments are still required; a new
        // curry_helper holding the appended arg is returned.
        template<typename _Arg>
        D_CONSTEXPR
        curry_helper<_Fn, _Args..., typename std::decay<_Arg>::type>
        dispatch(
            _Arg&& _arg,
            std::false_type
        ) const
        {
            return curry_helper<_Fn, _Args...,
                                typename std::decay<_Arg>::type>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<_Arg>(_arg))));
        }

        // call_multi (true_type)
        //   invoke immediately with the full argument list.
        template<typename... _NewArgs>
        D_CONSTEXPR
        auto call_multi(
            std::true_type,
            _NewArgs&&... _new_args
        ) const
        -> decltype(internal::apply_tuple(
               std::declval<const _Fn&>(),
               std::tuple_cat(
                   std::declval<const std::tuple<_Args...>&>(),
                   std::forward_as_tuple(
                       std::forward<_NewArgs>(_new_args)...))))
        {
            return internal::apply_tuple(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::forward_as_tuple(
                        std::forward<_NewArgs>(_new_args)...)));
        }

        // call_multi (false_type)
        //   build a new helper with all new args appended.
        template<typename... _NewArgs>
        D_CONSTEXPR
        curry_helper<_Fn,
                     _Args...,
                     typename std::decay<_NewArgs>::type...>
        call_multi(
            std::false_type,
            _NewArgs&&... _new_args
        ) const
        {
            return curry_helper<_Fn,
                                _Args...,
                                typename std::decay<_NewArgs>::type...>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(
                        std::forward<_NewArgs>(_new_args)...)));
        }

        _Fn                  m_fn;
        std::tuple<_Args...> m_args;
    };


    // curry_n_helper
    //   helper: explicit-arity curry. Tracks the number of remaining
    // arguments at compile time and invokes the wrapped function
    // exactly when that count reaches zero. Resolves ambiguity for
    // callables whose effective arity cannot be deduced (overloads,
    // generic lambdas, std::function with default args, etc.).
    template<std::size_t _Remaining,
             typename    _Fn,
             typename... _Args>
    class curry_n_helper
    {
    public:
        template<typename _FnFwd,
                 typename _TupleFwd>
        D_CONSTEXPR
        curry_n_helper(
            _FnFwd&&    _fn,
            _TupleFwd&& _args
        )
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_args(std::forward<_TupleFwd>(_args))
        {}

        // operator() (one argument)
        //   appends one argument and returns a helper with arity
        // _Remaining - 1.
        template<typename _Arg>
        D_CONSTEXPR
        curry_n_helper<_Remaining - 1,
                       _Fn,
                       _Args...,
                       typename std::decay<_Arg>::type>
        operator()(_Arg&& _arg) const
        {
            return curry_n_helper<_Remaining - 1, _Fn, _Args...,
                                  typename std::decay<_Arg>::type>(
                m_fn,
                std::tuple_cat(
                    m_args,
                    std::make_tuple(std::forward<_Arg>(_arg))));
        }

    private:
        _Fn                  m_fn;
        std::tuple<_Args...> m_args;
    };


    // curry_n_helper (zero-arity terminal)
    //   helper: specialization for when no further arguments are
    // expected. Provides operator() that performs the invocation
    // with the collected arguments.
    template<typename    _Fn,
             typename... _Args>
    class curry_n_helper<0, _Fn, _Args...>
    {
    public:
        template<typename _FnFwd,
                 typename _TupleFwd>
        D_CONSTEXPR
        curry_n_helper(
            _FnFwd&&    _fn,
            _TupleFwd&& _args
        )
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_args(std::forward<_TupleFwd>(_args))
        {}

        // operator() (no arguments)
        //   invokes the stored function with the accumulated args.
        D_CONSTEXPR
        auto operator()() const
        -> decltype(internal::apply_tuple(
               std::declval<const _Fn&>(),
               std::declval<const std::tuple<_Args...>&>()))
        {
            return internal::apply_tuple(m_fn, m_args);
        }

        // implicit conversion to the result type (for terminal helpers
        // that have already accumulated all args; lets users write
        // `int x = curry_n<2>(f)(1)(2);` without an explicit final ()).
        template<typename _R = decltype(internal::apply_tuple(
                     std::declval<const _Fn&>(),
                     std::declval<const std::tuple<_Args...>&>()))>
        D_CONSTEXPR
        operator _R() const
        {
            return internal::apply_tuple(m_fn, m_args);
        }

    private:
        _Fn                  m_fn;
        std::tuple<_Args...> m_args;
    };


    // flip_helper
    //   helper: holds a callable and exposes operator() that swaps the
    // first two arguments before forwarding. Additional arguments are
    // passed through in their original order.
    template<typename _Fn>
    class flip_helper
    {
    public:
        template<typename _FnFwd>
        explicit D_CONSTEXPR
        flip_helper(
            _FnFwd&& _fn
        )
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        // operator() (two arguments)
        template<typename _A,
                 typename _B>
        D_CONSTEXPR
        auto operator()(
            _A&& _a,
            _B&& _b
        ) const
        -> decltype(std::declval<const _Fn&>()(
               std::forward<_B>(_b),
               std::forward<_A>(_a)))
        {
            return m_fn(std::forward<_B>(_b), std::forward<_A>(_a));
        }

        // operator() (three or more arguments)
        //   swaps only the first two; the remainder pass through.
        template<typename _A,
                 typename _B,
                 typename _C,
                 typename... _Rest>
        D_CONSTEXPR
        auto operator()(
            _A&&        _a,
            _B&&        _b,
            _C&&        _c,
            _Rest&&...  _rest
        ) const
        -> decltype(std::declval<const _Fn&>()(
               std::forward<_B>(_b),
               std::forward<_A>(_a),
               std::forward<_C>(_c),
               std::forward<_Rest>(_rest)...))
        {
            return m_fn(std::forward<_B>(_b),
                        std::forward<_A>(_a),
                        std::forward<_C>(_c),
                        std::forward<_Rest>(_rest)...);
        }

    private:
        _Fn m_fn;
    };


    // always_helper
    //   helper: ignores all arguments and returns a stored value. Used
    // for `always(x)` / `constant(x)`.
    template<typename _Value>
    class always_helper
    {
    public:
        template<typename _ValueFwd>
        explicit D_CONSTEXPR
        always_helper(
            _ValueFwd&& _value
        )
            : m_value(std::forward<_ValueFwd>(_value))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        const _Value& operator()(_Args&&...) const
        {
            return m_value;
        }

    private:
        _Value m_value;
    };


    // uncurry_helper
    //   helper: wraps a fully-curried function so it can be called in
    // multi-argument form again.  `uncurry(f)(a, b, c)` becomes
    // `f(a)(b)(c)`.
    template<typename _Fn>
    class uncurry_helper
    {
    public:
        template<typename _FnFwd>
        explicit D_CONSTEXPR
        uncurry_helper(
            _FnFwd&& _fn
        )
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        // operator() (single argument)
        template<typename _Arg>
        D_CONSTEXPR
        auto operator()(_Arg&& _arg) const
        -> decltype(std::declval<const _Fn&>()(std::forward<_Arg>(_arg)))
        {
            return m_fn(std::forward<_Arg>(_arg));
        }

        // operator() (two or more arguments)
        //   applies the function to the first argument, then recurses
        // on the result with the remaining arguments. This expresses
        // uncurry recursively in terms of itself.
        template<typename    _A,
                 typename    _B,
                 typename... _Rest>
        D_CONSTEXPR
        auto operator()(
            _A&&        _a,
            _B&&        _b,
            _Rest&&...  _rest
        ) const
        -> decltype(uncurry_helper<decltype(
                       std::declval<const _Fn&>()(
                           std::forward<_A>(_a)))>(
                       std::declval<const _Fn&>()(std::forward<_A>(_a)))(
                           std::forward<_B>(_b),
                           std::forward<_Rest>(_rest)...))
        {
            return uncurry_helper<decltype(
                m_fn(std::forward<_A>(_a)))>(
                    m_fn(std::forward<_A>(_a)))(
                        std::forward<_B>(_b),
                        std::forward<_Rest>(_rest)...);
        }

    private:
        _Fn m_fn;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   CURRY FACTORIES                                       ///
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
template<typename _Fn>
D_CONSTEXPR
internal::curry_helper<typename std::decay<_Fn>::type>
curry
(
    _Fn&& _fn
)
{
    return internal::curry_helper<typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn),
        std::tuple<>{});
}


// curry_n
//   function: explicit-arity curry. Builds a curried form that
// requires exactly _N more arguments before invocation. Useful
// when the wrapped callable's arity cannot be reliably inferred.
template<std::size_t _N,
         typename    _Fn>
D_CONSTEXPR
internal::curry_n_helper<_N, typename std::decay<_Fn>::type>
curry_n
(
    _Fn&& _fn
)
{
    return internal::curry_n_helper<_N, typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn),
        std::tuple<>{});
}


///////////////////////////////////////////////////////////////////////////////
///             III.  UNCURRYING                                            ///
///////////////////////////////////////////////////////////////////////////////

// uncurry
//   function: produces a wrapper for a curried function that
// accepts all arguments at once. uncurry(f)(a, b, c) is equivalent
// to f(a)(b)(c). For functions that are not curried, the wrapper
// degenerates to a plain call.
template<typename _Fn>
D_CONSTEXPR
internal::uncurry_helper<typename std::decay<_Fn>::type>
uncurry
(
    _Fn&& _fn
)
{
    return internal::uncurry_helper<typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   ARGUMENT TRANSFORMATIONS                              ///
///////////////////////////////////////////////////////////////////////////////

// flip
//   function: returns a wrapper that swaps the first two arguments
// of the wrapped callable before invocation. Useful for adapting
// callables to expected argument orders (e.g. when a library wants
// `cmp(b, a)` but you have `cmp(a, b)`).
template<typename _Fn>
D_CONSTEXPR
internal::flip_helper<typename std::decay<_Fn>::type>
flip
(
    _Fn&& _fn
)
{
    return internal::flip_helper<typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn));
}


///////////////////////////////////////////////////////////////////////////////
///             V.    CONSTANT-VALUED COMBINATORS                           ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // identity_helper
    //   helper: invokable type that returns its argument unchanged,
    // preserving value category and constness.
    struct identity_helper
    {
        template<typename _Arg>
        D_CONSTEXPR
        _Arg&& operator()(
            _Arg&& _arg
        ) const noexcept
        {
            return std::forward<_Arg>(_arg);
        }
    };


    // never_helper
    //   helper: invokable type that returns false for any arguments.
    // Useful as a predicate seed for combinators.
    struct never_helper
    {
        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&...) const noexcept
        {
            return false;
        }
    };

NS_END  // internal


// identity
//   constant: function object that returns its argument unchanged.
// Useful as a default transformer in higher-order code.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static constexpr internal::identity_helper identity{};
#else
    static const internal::identity_helper identity = internal::identity_helper{};
#endif


// always
//   function: creates a callable that ignores its arguments and
// returns the stored value. Equivalent to the K combinator.
//   always(x)(...)  ==  x
template<typename _Value>
D_CONSTEXPR
internal::always_helper<typename std::decay<_Value>::type>
always
(
    _Value&& _value
)
{
    return internal::always_helper<typename std::decay<_Value>::type>(
        std::forward<_Value>(_value));
}


// constant
//   function: alias for always, provided for readability in code
// that conceptually requires a constant function rather than a
// "always returns the same value" function.
template<typename _Value>
D_CONSTEXPR
internal::always_helper<typename std::decay<_Value>::type>
constant
(
    _Value&& _value
)
{
    return always(std::forward<_Value>(_value));
}


// never
//   constant: predicate that returns false for any input.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static constexpr internal::never_helper never{};
#else
    static const internal::never_helper never = internal::never_helper{};
#endif

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_CURRY_
