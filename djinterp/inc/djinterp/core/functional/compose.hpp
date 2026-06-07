/******************************************************************************
* djinterp [functional]                                            compose.hpp
*
* Template function composition and partial application (C++).
*   SFINAE-constrained composed transformers, partial application
* helpers, variadic composition chains, and a self-contained set of
* predicate SFINAE structural traits + C++20 concepts describing the
* composition vocabulary (invocability, composability, and the
* structural surfaces of the composed-transformer and memoize helpers).
*
*   DUAL-STANDARD: the C++11+ implementation is the primary path. It
* provides the binary primitives (compose / pipe / compose_transformer),
* partial application, variadic compose_all / pipe_all chains, memoize,
* tap, and the inline fix() Y-combinator, together with the predicate
* traits and concepts in Section 0. A C++98 fallback under #else provides
* the binary composition primitives (compose / pipe / compose_transformer)
* and memoize only; variadic compose_all / pipe_all, the inline fix()
* Y-combinator, and the predicate traits/concepts are RED in C++98
* (parameter packs / generic lambdas / concepts) and are therefore
* C++11+ only. See the `cpp98 roadmap` workbook.
*
*   SELF-CONTAINED RESULT TYPE: callable_result / callable_result_t are
* defined here directly in djinterp:: (Section 0) so this header carries
* no dependency on a separate functional-traits aggregator.
*
*   DUAL-DOMAIN (compile-time + runtime): the composition path is constexpr
* over by-value (decayed) literal storage, so a single compose body forwards
* and applies without inspecting its operand domain.  With carrier-callable
* leaves (see core/meta/carrier.hpp) the same compose / pipe / compose_all /
* pipe_all / partial_back / tap / fix therefore FOLD AT COMPILE TIME over NTTP
* value carriers and over type carriers, as well as running unchanged at
* runtime - the "two-lift" resolution requires no second set of combinators:
*     compose(add_ptr, add_const)(type_c<int>)  -> type_t<const int*>
*     compose(dbl, inc)(val<10>)                -> val_t<22>
*     compose(inc, dbl)(10)                     == 21   (runtime / constexpr)
* compose stays carrier-agnostic (it composes ANY callables); the carriers
* live at the call site.  The sole non-constexpr factory is memoize, whose
* std::map result cache is inherently a runtime construct.
*
* path:      /inc/functional/compose.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COMPOSE_
#define DJINTERP_FUNCTIONAL_COMPOSE_ 1

// std
#include <cstddef>
#include <map>
#include <utility>
// djinterp
#include "../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// std (C++11+ primary path)
#  include <functional>
#  include <type_traits>
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
///////////////////////////////////////////////////////////////////////////////
//   C++11+ PRIMARY IMPLEMENTATION                                            //
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             0.    PREDICATE SFINAE STRUCTURAL TRAITS & CONCEPTS         ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for the composition machinery.
// Every predicate reduces to a `static constexpr bool value` (or, for
// the type-yielding traits, a `::type`), built on the core ::void_t
// SFINAE sink declared in djinterp.hpp. The C++20 concept mirrors
// follow at the end of the section, gated on concept support.

NS_INTERNAL

    // callable_result_helper
    //   trait: SFINAE result-type extractor (primary: no `type`,
    // so substitution into callable_result_t is a soft failure).
    template<typename    _AlwaysVoid,
             typename    _Function,
             typename... _Args>
    struct callable_result_helper
    {};

    // callable_result_helper (well-formed specialization)
    //   trait: yields the result type of _Function(_Args...) when the
    // call expression is well-formed.
    template<typename    _Function,
             typename... _Args>
    struct callable_result_helper<
        void_t<decltype(std::declval<_Function>()(
            std::declval<_Args>()...))>,
        _Function,
        _Args...>
    {
        using type = decltype(std::declval<_Function>()(
            std::declval<_Args>()...));
    };

    // is_invocable_helper
    //   trait: detection sink for the _Function(_Args...) call
    // expression (primary: false).
    template<typename    _AlwaysVoid,
             typename    _Function,
             typename... _Args>
    struct is_invocable_helper : std::false_type
    {};

    // is_invocable_helper (well-formed specialization)
    //   trait: true when _Function(_Args...) is a valid call.
    template<typename    _Function,
             typename... _Args>
    struct is_invocable_helper<
        void_t<decltype(std::declval<_Function>()(
            std::declval<_Args>()...))>,
        _Function,
        _Args...> : std::true_type
    {};

    // is_composable_helper
    //   trait: detection sink for the composed call expression
    // _Outer(_Inner(_Input)) (primary: false).
    template<typename _AlwaysVoid,
             typename _Outer,
             typename _Inner,
             typename _Input>
    struct is_composable_helper : std::false_type
    {};

    // is_composable_helper (well-formed specialization)
    //   trait: true when _Outer(_Inner(_Input)) is a valid call
    // chain.
    template<typename _Outer,
             typename _Inner,
             typename _Input>
    struct is_composable_helper<
        void_t<decltype(std::declval<_Outer>()(
            std::declval<_Inner>()(std::declval<_Input>())))>,
        _Outer,
        _Inner,
        _Input> : std::true_type
    {};

    // is_composed_transformer_helper
    //   trait: detection sink for the .first() / .second() const
    // accessor surface of composed_transformer_helper (primary:
    // false).
    template<typename _AlwaysVoid,
             typename _Type>
    struct is_composed_transformer_helper : std::false_type
    {};

    // is_composed_transformer_helper (well-formed specialization)
    //   trait: true when _Type exposes both .first() and .second()
    // const accessors.
    template<typename _Type>
    struct is_composed_transformer_helper<
        void_t<decltype(std::declval<const _Type&>().first()),
               decltype(std::declval<const _Type&>().second())>,
        _Type> : std::true_type
    {};

    // is_memoized_helper
    //   trait: detection sink for the .clear_cache() / .cache_size()
    // const surface of memoize_helper (primary: false).
    template<typename _AlwaysVoid,
             typename _Type>
    struct is_memoized_helper : std::false_type
    {};

    // is_memoized_helper (well-formed specialization)
    //   trait: true when _Type exposes both .clear_cache() and
    // .cache_size() const accessors.
    template<typename _Type>
    struct is_memoized_helper<
        void_t<decltype(std::declval<const _Type&>().clear_cache()),
               decltype(std::declval<const _Type&>().cache_size())>,
        _Type> : std::true_type
    {};

NS_END  // internal

// callable_result
//   trait: result type of invoking _Function with _Args... . Has a
// `::type` member only when the call expression is well-formed,
// making callable_result_t SFINAE-friendly as a default argument.
template<typename    _Function,
         typename... _Args>
struct callable_result
{
    using type =
        typename internal::callable_result_helper<void,
                                                  _Function,
                                                  _Args...>::type;
};

// callable_result_t
//   type: convenience alias for callable_result<...>::type.
template<typename    _Function,
         typename... _Args>
using callable_result_t = typename callable_result<_Function, _Args...>::type;

// is_invocable
//   trait: true when _Function is callable with _Args... .
template<typename    _Function,
         typename... _Args>
struct is_invocable
    : internal::is_invocable_helper<void, _Function, _Args...>
{};

NS_INTERNAL

    // is_invocable_r_helper
    //   trait: convertibility branch of is_invocable_r, selected by the
    // _Invocable flag. Primary (false flag): not invocable, so the
    // result type is never named -- guarantees SFINAE-safety.
    template<bool        _Invocable,
             typename    _Result,
             typename    _Function,
             typename... _Args>
    struct is_invocable_r_helper : std::false_type
    {};

    // is_invocable_r_helper (invocable branch)
    //   trait: when invocable, the value is whether the call result is
    // convertible to _Result.
    template<typename    _Result,
             typename    _Function,
             typename... _Args>
    struct is_invocable_r_helper<true, _Result, _Function, _Args...>
    {
        static constexpr bool value =
            std::is_convertible<
                callable_result_t<_Function, _Args...>, _Result>::value;
    };

    // is_unary_transformer_helper
    //   trait: non-void branch of is_unary_transformer, selected by the
    // _Invocable flag. Primary (false flag): not invocable, so the
    // result type is never named -- guarantees SFINAE-safety.
    template<bool     _Invocable,
             typename _Function,
             typename _Input>
    struct is_unary_transformer_helper : std::false_type
    {};

    // is_unary_transformer_helper (invocable branch)
    //   trait: when invocable, the value is whether the result is
    // non-void.
    template<typename _Function,
             typename _Input>
    struct is_unary_transformer_helper<true, _Function, _Input>
    {
        static constexpr bool value =
            !std::is_void<
                callable_result_t<_Function, _Input>>::value;
    };

NS_END  // internal

// is_invocable_r
//   trait: true when _Function is callable with _Args... and the
// result is convertible to _Result. The convertibility check is gated
// behind invocability so the result type is never named in the
// non-invocable case (SFINAE-safe).
template<typename    _Result,
         typename    _Function,
         typename... _Args>
struct is_invocable_r
    : internal::is_invocable_r_helper<
          is_invocable<_Function, _Args...>::value,
          _Result, _Function, _Args...>
{};

// is_unary_transformer
//   trait: true when _Function is callable with a single _Input and
// produces a non-void result (the shape a composition stage expects).
// The non-void check is gated behind invocability so the result type
// is never named in the non-invocable case (SFINAE-safe).
template<typename _Function,
         typename _Input>
struct is_unary_transformer
    : internal::is_unary_transformer_helper<
          is_invocable<_Function, _Input>::value,
          _Function, _Input>
{};

// is_composable
//   trait: true when the call chain _Outer(_Inner(_Input)) is
// well-formed -- i.e. _Inner accepts _Input and _Outer accepts the
// result of _Inner.
template<typename _Outer,
         typename _Inner,
         typename _Input>
struct is_composable
    : internal::is_composable_helper<void, _Outer, _Inner, _Input>
{};

// composition_result
//   trait: result type of _Outer(_Inner(_Input)).
template<typename _Outer,
         typename _Inner,
         typename _Input>
struct composition_result
{
    using type =
        callable_result_t<_Outer, callable_result_t<_Inner, _Input>>;
};

// composition_result_t
//   type: convenience alias for composition_result<...>::type.
template<typename _Outer,
         typename _Inner,
         typename _Input>
using composition_result_t =
    typename composition_result<_Outer, _Inner, _Input>::type;

// is_composed_transformer
//   trait: structural predicate -- true when _Type exposes the
// .first() / .second() introspection surface produced by
// compose / pipe / compose_transformer.
template<typename _Type>
struct is_composed_transformer
    : internal::is_composed_transformer_helper<void, _Type>
{};

// is_memoized
//   trait: structural predicate -- true when _Type exposes the
// .clear_cache() / .cache_size() surface produced by memoize.
template<typename _Type>
struct is_memoized
    : internal::is_memoized_helper<void, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_invocable_v
    //   value: convenience alias for is_invocable<...>::value.
    template<typename    _Function,
             typename... _Args>
    constexpr bool is_invocable_v = is_invocable<_Function, _Args...>::value;

    // is_invocable_r_v
    //   value: convenience alias for is_invocable_r<...>::value.
    template<typename    _Result,
             typename    _Function,
             typename... _Args>
    constexpr bool is_invocable_r_v =
        is_invocable_r<_Result, _Function, _Args...>::value;

    // is_unary_transformer_v
    //   value: convenience alias for is_unary_transformer<...>::value.
    template<typename _Function,
             typename _Input>
    constexpr bool is_unary_transformer_v =
        is_unary_transformer<_Function, _Input>::value;

    // is_composable_v
    //   value: convenience alias for is_composable<...>::value.
    template<typename _Outer,
             typename _Inner,
             typename _Input>
    constexpr bool is_composable_v =
        is_composable<_Outer, _Inner, _Input>::value;

    // is_composed_transformer_v
    //   value: convenience alias for is_composed_transformer<...>::value.
    template<typename _Type>
    constexpr bool is_composed_transformer_v =
        is_composed_transformer<_Type>::value;

    // is_memoized_v
    //   value: convenience alias for is_memoized<...>::value.
    template<typename _Type>
    constexpr bool is_memoized_v = is_memoized<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // invocable_with
    //   concept: satisfied when _Function is callable with _Args... .
    // Named to avoid colliding with the std::invocable re-export in
    // concepts.hpp.
    template<typename    _Function,
             typename... _Args>
    concept invocable_with = is_invocable<_Function, _Args...>::value;

    // unary_transformer
    //   concept: satisfied when _Function maps a single _Input to a
    // non-void result.
    template<typename _Function,
             typename _Input>
    concept unary_transformer = is_unary_transformer<_Function, _Input>::value;

    // composable
    //   concept: satisfied when _Outer(_Inner(_Input)) is a valid call
    // chain.
    template<typename _Outer,
             typename _Inner,
             typename _Input>
    concept composable = is_composable<_Outer, _Inner, _Input>::value;

    // composed_transformer_like
    //   concept: satisfied when _Type exposes the .first() / .second()
    // composed-transformer surface.
    template<typename _Type>
    concept composed_transformer_like = is_composed_transformer<_Type>::value;

    // memoized_like
    //   concept: satisfied when _Type exposes the .clear_cache() /
    // .cache_size() memoize surface.
    template<typename _Type>
    concept memoized_like = is_memoized<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             I.    COMPOSED TRANSFORMER                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // composed_transformer_helper
    //   helper: composition of two transformers with optional contexts.
    // Mirrors d_composed_transformer from compose.h but is fully typed.
    // compose(f, g)(x) = f(g(x)), so _Second is applied first.
    template<typename _First,
             typename _Second>
    class composed_transformer_helper
    {
    public:
        template<typename _F1Fwd,
                 typename _F2Fwd>
        D_CONSTEXPR
        composed_transformer_helper
        (
            _F1Fwd&& _first,
            _F2Fwd&& _second
        ) : m_first(std::forward<_F1Fwd>(_first))
          , m_second(std::forward<_F2Fwd>(_second))
        {}

        template<typename _Input>
        D_CONSTEXPR
        auto operator()(const _Input& _input) const
            -> decltype(std::declval<const _Second&>()(
                std::declval<const _First&>()(_input)))
        {
            return m_second(m_first(_input));
        }

        template<typename _Input>
        D_CONSTEXPR
        auto operator()(_Input&& _input) const
            -> decltype(std::declval<const _Second&>()(
                std::declval<const _First&>()(
                    std::forward<_Input>(_input))))
        {
            return m_second(m_first(std::forward<_Input>(_input)));
        }

        // accessors for introspection
        D_CONSTEXPR const _First&  first()  const { return m_first; }
        D_CONSTEXPR const _Second& second() const { return m_second; }

    private:
        _First  m_first;   // applied first (g in f(g(x)))
        _Second m_second;  // applied second (f in f(g(x)))
    };

    // partial_consumer_helper
    //   helper: partially applied consumer with bound context.
    // Mirrors d_partial_consumer from compose.h.
    template<typename _Function,
             typename _BoundArg>
    class partial_consumer_helper
    {
    public:
        template<typename _FunctionFwd,
                 typename _ArgFwd>
        D_CONSTEXPR
        partial_consumer_helper(_FunctionFwd&& _function, _ArgFwd&& _arg)
            : m_fn(std::forward<_FunctionFwd>(_function))
            , m_bound(std::forward<_ArgFwd>(_arg))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        auto operator()(_Args&&... _args) const
            -> decltype(std::declval<const _Function&>()(
                std::forward<_Args>(_args)...,
                std::declval<const _BoundArg&>()))
        {
            return m_fn(std::forward<_Args>(_args)..., m_bound);
        }

    private:
        _Function m_fn;
        _BoundArg m_bound;
    };

    // tap_helper
    //   helper: passes value through a side-effect function, returning
    // the original value unchanged.
    template<typename _Function>
    class tap_helper
    {
    public:
        // the forwarding constructor is constrained out when the
        // argument is itself a tap_helper, so the compiler-generated
        // copy / move constructors are not hijacked (which would
        // otherwise try to store a tap_helper into m_fn).
        template<typename _FunctionFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<_FunctionFwd>::type,
                         tap_helper>::value>::type>
        explicit D_CONSTEXPR
        tap_helper(_FunctionFwd&& _function)
            : m_fn(std::forward<_FunctionFwd>(_function))
        {}

        template<typename _Type>
        D_CONSTEXPR
        _Type operator()(_Type _value) const
        {
            m_fn(_value);

            return _value;
        }

    private:
        _Function m_fn;
    };

    // memoize_helper
    //   helper: caches results of a pure function.
    template<typename _Function,
             typename _Input,
             typename _Output>
    class memoize_helper
    {
    public:
        template<typename _FunctionFwd>
        explicit memoize_helper(_FunctionFwd&& _function)
            : m_fn(std::forward<_FunctionFwd>(_function))
        {}

        _Output operator()(const _Input& _input) const
        {
            auto it = m_cache.find(_input);

            if (it != m_cache.end())
            {
                return it->second;
            }

            _Output result = m_fn(_input);

            m_cache[_input] = result;

            return result;
        }

        void clear_cache() const
        {
            m_cache.clear();

            return;
        }

        std::size_t cache_size() const
        {
            return m_cache.size();
        }

    private:
        _Function                           m_fn;
        mutable std::map<_Input, _Output>   m_cache;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FACTORIES (compose_transformer / compose / pipe)      ///
///////////////////////////////////////////////////////////////////////////////

// compose_transformer
//   function: creates a composed transformer that applies _first then
// _second. compose_transformer(g, f)(x) = f(g(x))
// This is left-to-right composition (pipe order).
template<typename _First,
         typename _Second>
D_CONSTEXPR
internal::composed_transformer_helper<typename std::decay<_First>::type,
                                      typename std::decay<_Second>::type>
compose_transformer
(
    _First&&  _first,
    _Second&& _second
)
{
    return internal::composed_transformer_helper<
        typename std::decay<_First>::type,
        typename std::decay<_Second>::type>(
            std::forward<_First>(_first),
            std::forward<_Second>(_second));
}

// compose
//   function: math-order composition. compose(f, g)(x) = f(g(x)).
// _inner (g) is applied first, _outer (f) second.
template<typename _Outer,
         typename _Inner>
D_CONSTEXPR
internal::composed_transformer_helper<typename std::decay<_Inner>::type,
                                      typename std::decay<_Outer>::type>
compose
(
    _Outer&& _outer,
    _Inner&& _inner
)
{
    return internal::composed_transformer_helper<
        typename std::decay<_Inner>::type,
        typename std::decay<_Outer>::type>(
            std::forward<_Inner>(_inner),
            std::forward<_Outer>(_outer));
}

// pipe
//   function: left-to-right composition. pipe(f, g)(x) = g(f(x)).
// _first (f) is applied first, _second (g) second. Equivalent in shape
// to compose_transformer.
template<typename _First,
         typename _Second>
D_CONSTEXPR
internal::composed_transformer_helper<typename std::decay<_First>::type,
                                      typename std::decay<_Second>::type>
pipe
(
    _First&&  _first,
    _Second&& _second
)
{
    return internal::composed_transformer_helper<
        typename std::decay<_First>::type,
        typename std::decay<_Second>::type>(
            std::forward<_First>(_first),
            std::forward<_Second>(_second));
}


///////////////////////////////////////////////////////////////////////////////
///             III.  VARIADIC COMPOSITION                                  ///
///////////////////////////////////////////////////////////////////////////////
//   compose_all / pipe_all are expressed as type-level folds over the
// binary compose / pipe primitives. The fold structs decouple return-
// type computation (ordinary class-template instantiation) from
// overload resolution, which is what makes the recursion well-formed
// on the C++11 baseline (a plain auto + trailing-decltype self-
// recursive overload set does not resolve).

NS_INTERNAL

    // compose_all_fold
    //   helper: right-to-left fold of compose over a function pack
    // (primary template; specialized below).
    template<typename... _Functions>
    struct compose_all_fold;

    // compose_all_fold<_Function>
    //   helper: base case -- a single function folds to itself.
    template<typename _Function>
    struct compose_all_fold<_Function>
    {
        using type = typename std::decay<_Function>::type;

        template<typename _Fwd>
        static D_CONSTEXPR type apply(_Fwd&& _function)
        {
            return std::forward<_Fwd>(_function);
        }
    };

    // compose_all_fold<_Function1, _Function2, _Rest...>
    //   helper: recursive case -- compose(f1, fold(f2, rest...)).
    template<typename    _Function1,
             typename    _Function2,
             typename... _Rest>
    struct compose_all_fold<_Function1, _Function2, _Rest...>
    {
        using tail = compose_all_fold<_Function2, _Rest...>;
        using type = decltype(djinterp::compose(
            std::declval<_Function1>(),
            std::declval<typename tail::type>()));

        template<typename    _F1,
                 typename    _F2,
                 typename... _R>
        static D_CONSTEXPR type apply(_F1&& _f1, _F2&& _f2, _R&&... _rest)
        {
            return djinterp::compose(
                std::forward<_F1>(_f1),
                tail::apply(std::forward<_F2>(_f2),
                            std::forward<_R>(_rest)...));
        }
    };

    // pipe_all_fold
    //   helper: left-to-right fold of pipe over a function pack
    // (primary template; specialized below).
    template<typename... _Functions>
    struct pipe_all_fold;

    // pipe_all_fold<_Function>
    //   helper: base case -- a single function folds to itself.
    template<typename _Function>
    struct pipe_all_fold<_Function>
    {
        using type = typename std::decay<_Function>::type;

        template<typename _Fwd>
        static D_CONSTEXPR type apply(_Fwd&& _function)
        {
            return std::forward<_Fwd>(_function);
        }
    };

    // pipe_all_fold<_Function1, _Function2, _Rest...>
    //   helper: recursive case -- fold(pipe(f1, f2), rest...).
    template<typename    _Function1,
             typename    _Function2,
             typename... _Rest>
    struct pipe_all_fold<_Function1, _Function2, _Rest...>
    {
        using piped = decltype(djinterp::pipe(std::declval<_Function1>(),
                                              std::declval<_Function2>()));
        using tail  = pipe_all_fold<piped, _Rest...>;
        using type  = typename tail::type;

        template<typename    _F1,
                 typename    _F2,
                 typename... _R>
        static D_CONSTEXPR type apply(_F1&& _f1, _F2&& _f2, _R&&... _rest)
        {
            return tail::apply(djinterp::pipe(std::forward<_F1>(_f1),
                                              std::forward<_F2>(_f2)),
                               std::forward<_R>(_rest)...);
        }
    };

NS_END  // internal

// compose_all (right-to-left)
//   function: composes N functions right-to-left.
// compose_all(f, g, h)(x) = f(g(h(x)))
template<typename... _Functions>
D_CONSTEXPR
typename internal::compose_all_fold<_Functions...>::type
compose_all
(
    _Functions&&... _functions
)
{
    return internal::compose_all_fold<_Functions...>::apply(
        std::forward<_Functions>(_functions)...);
}

// pipe_all (left-to-right)
//   function: composes N functions left-to-right.
// pipe_all(f, g, h)(x) = h(g(f(x)))
template<typename... _Functions>
D_CONSTEXPR
typename internal::pipe_all_fold<_Functions...>::type
pipe_all
(
    _Functions&&... _functions
)
{
    return internal::pipe_all_fold<_Functions...>::apply(
        std::forward<_Functions>(_functions)...);
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   PARTIAL APPLICATION                                   ///
///////////////////////////////////////////////////////////////////////////////

// partial_back
//   function: partially applies the last argument of a function.
// partial_back(f, z)(x, y) = f(x, y, z)
template<typename _Function,
         typename _Arg>
D_CONSTEXPR
internal::partial_consumer_helper<typename std::decay<_Function>::type,
                                  typename std::decay<_Arg>::type>
partial_back
(
    _Function&& _function,
    _Arg&&      _arg
)
{
    return internal::partial_consumer_helper<
        typename std::decay<_Function>::type,
        typename std::decay<_Arg>::type>(
            std::forward<_Function>(_function),
            std::forward<_Arg>(_arg)
        );
}

// Note: partial_front (bind_front) is already provided in functional.hpp
// via stl::bind_front. partial_back is the complement.


///////////////////////////////////////////////////////////////////////////////
///             V.    TAP (SIDE-EFFECT INJECTION)                           ///
///////////////////////////////////////////////////////////////////////////////

// tap
//   function: creates a pass-through function that executes a side-effect.
// tap(f)(x) calls f(x) then returns x unchanged.
// Useful for debugging in composition chains.
template<typename _Function>
D_CONSTEXPR
internal::tap_helper<typename std::decay<_Function>::type>
tap(_Function&& _function)
{
    return internal::tap_helper<typename std::decay<_Function>::type>(
        std::forward<_Function>(_function));
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   MEMOIZATION                                           ///
///////////////////////////////////////////////////////////////////////////////

// memoize
//   function: wraps a pure function with a cache.
// The function must be deterministic (same input -> same output).
// The input type must be comparable (for use as map key).
template<typename _Function,
         typename _Input,
         typename _Output = callable_result_t<_Function, const _Input&>>
internal::memoize_helper<typename std::decay<_Function>::type, _Input, _Output>
memoize
(
    _Function&& _function
)
{
    return internal::memoize_helper<
        typename std::decay<_Function>::type, _Input, _Output>(
            std::forward<_Function>(_function)
        );
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  FIXED-POINT COMBINATOR                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // fix_helper
    //   helper: Y combinator for expressing recursive lambdas.
    template<typename _Function>
    class fix_helper
    {
    public:
        // the forwarding constructor is constrained out when the
        // argument is itself a fix_helper, so the compiler-generated
        // copy / move constructors are not hijacked.
        template<typename _FunctionFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<_FunctionFwd>::type,
                         fix_helper>::value>::type>
        explicit D_CONSTEXPR
        fix_helper(_FunctionFwd&& _function)
            : m_fn(std::forward<_FunctionFwd>(_function))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        auto operator()(_Args&&... _args) const
            -> decltype(std::declval<const _Function&>()(
                std::declval<const fix_helper&>(),
                std::forward<_Args>(_args)...))
        {
            return m_fn(*this, std::forward<_Args>(_args)...);
        }

    private:
        _Function m_fn;
    };

NS_END  // internal

// fix
//   function: Y combinator for recursive lambdas.
// Usage:
//   auto factorial = fix([](auto& self, int n) -> int {
//       return n <= 1 ? 1 : n * self(n - 1);
//   });
//   factorial(5);  // 120
template<typename _Function>
D_CONSTEXPR internal::fix_helper<typename std::decay<_Function>::type>
fix(
    _Function&& _function
)
{
    return internal::fix_helper<typename std::decay<_Function>::type>(
        std::forward<_Function>(_function));
}


NS_END  // djinterp

#else  // !D_ENV_LANG_IS_CPP11_OR_HIGHER
///////////////////////////////////////////////////////////////////////////////
//   C++98 FALLBACK IMPLEMENTATION                                            //
//   Binary compose / pipe / compose_transformer + memoize. Variadic,       //
//   fix(), and the predicate traits/concepts are unavailable in C++98      //
//   and are omitted from this arm.                                         //
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    COMPOSED TRANSFORMER                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // composed_transformer_helper
    //   helper: composition of two transformers. Applies _First
    // first, then _Second to the result. operator()(x) yields
    // m_second(m_first(x)).
    //
    //   Return type is _Second::result_type, taken from the
    // Adaptable Function Object convention. _Second must expose
    // a result_type typedef. _First's result type need not be
    // exposed; it is computed at the call site by the compiler
    // and fed straight into m_second.
    template<typename _First,
             typename _Second>
    class composed_transformer_helper
    {
    public:
        typedef typename _Second::result_type result_type;

        composed_transformer_helper(
            const _First&  _first,
            const _Second& _second
        )
            : m_first(_first)
            , m_second(_second)
        {}

        template<typename _Input>
        result_type operator()(
            const _Input& _input
        ) const
        {
            return m_second(m_first(_input));
        }

        // accessors for introspection
        const _First&  first()  const { return m_first;  }
        const _Second& second() const { return m_second; }

    private:
        _First  m_first;
        _Second m_second;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

// compose_transformer
//   function: builds a composed transformer that applies _first
// then _second. compose_transformer(g, f)(x) = f(g(x)).
// This is the pipe-order form: arguments read in execution order.
template<typename _First,
         typename _Second>
internal::composed_transformer_helper<_First, _Second>
compose_transformer(
    const _First&  _first,
    const _Second& _second
)
{
    return internal::composed_transformer_helper<_First, _Second>(_first,
                                                                  _second);
}


// compose
//   function: math-order composition. compose(f, g)(x) = f(g(x)).
// _g is applied first (inner), _f second (outer).
template<typename _F,
         typename _G>
internal::composed_transformer_helper<_G, _F>
compose(
    const _F& _f,
    const _G& _g
)
{
    return internal::composed_transformer_helper<_G, _F>(_g, _f);
}


// pipe
//   function: left-to-right composition. pipe(f, g)(x) = g(f(x)).
// _f is applied first, _g second. Equivalent in shape to
// compose_transformer.
template<typename _F,
         typename _G>
internal::composed_transformer_helper<_F, _G>
pipe(
    const _F& _f,
    const _G& _g
)
{
    return internal::composed_transformer_helper<_F, _G>(_f, _g);
}


///////////////////////////////////////////////////////////////////////////////
///             III.  MEMOIZE                                               ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // memoize_helper
    //   helper: caches results of a pure unary function. _Input
    // must be LessThanComparable (required for std::map keys).
    //
    //   The cache is mutable; operator() is const so memoize_helper
    // satisfies the const-callable convention required by the
    // composition helpers above.
    template<typename _Function,
             typename _Input,
             typename _Output>
    class memoize_helper
    {
    public:
        typedef _Output result_type;
        typedef _Input  argument_type;

        explicit memoize_helper(
            const _Function& _function
        )
            : m_fn(_function)
            , m_cache()
        {}

        _Output operator()(
            const _Input& _input
        ) const
        {
            typename std::map<_Input, _Output>::iterator it
                = m_cache.find(_input);

            if (it != m_cache.end())
            {
                return it->second;
            }

            _Output result = m_fn(_input);

            m_cache[_input] = result;

            return result;
        }

        // clear_cache
        //   method: empties the cache. const because m_cache is
        // mutable.
        void clear_cache() const
        {
            m_cache.clear();
        }

        // cache_size
        //   method: current number of cached entries.
        std::size_t cache_size() const
        {
            return m_cache.size();
        }

    private:
        _Function                          m_fn;
        mutable std::map<_Input, _Output>  m_cache;
    };

NS_END  // internal


// memoize
//   function: wraps a pure unary function with a cache. Because
// C++98 cannot deduce the return type from the function via
// decltype, _Input and _Output are explicit template parameters;
// _Function is deduced from the argument.
//
//   The wrapped function must be deterministic (same input ->
// same output) for the cache to be sound, and _Input must support
// operator< (it is used as a std::map key).
//
//   Usage:
//     int slow(int);
//     ...
//     djinterp::memoize<int, int>(&slow)  -> memoize_helper<...>
template<typename _Input,
         typename _Output,
         typename _Function>
internal::memoize_helper<_Function, _Input, _Output>
memoize(
    const _Function& _function
)
{
    return internal::memoize_helper<_Function,
                                    _Input,
                                    _Output>(_function);
}


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_FUNCTIONAL_COMPOSE_
