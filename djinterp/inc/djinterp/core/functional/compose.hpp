/*******************************************************************************
* djinterp [core]                                                    compose.hpp
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
*   RESULT TYPE: callable_result / callable_result_t come from
* functional_common.hpp, the module's one definition (SFINAE-friendly, as
* std::invoke_result is); this header once carried its own copy, which no
* unit could include beside the common header's.
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
*
* path:      /inc/djinterp/core/functional/compose.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COMPOSE_HPP
#define DJINTERP_FUNCTIONAL_COMPOSE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <map>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // void_t, self
#include "../meta/type_traits.hpp"   // is_invocable, is_invocable_r
#include "./functional_common.hpp"   // callable_result, callable_result_t

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// std (C++11+ primary path)
// std
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

    // is_composable_helper
    //   trait: detection sink for the composed call expression
    // Outer(Inner(Input)) (primary: false).
    template<typename AlwaysVoid,
             typename Outer,
             typename Inner,
             typename Input>
    struct is_composable_helper : std::false_type
    {};

    // is_composable_helper (well-formed specialization)
    //   trait: true when Outer(Inner(Input)) is a valid call
    // chain.
    template<typename Outer,
             typename Inner,
             typename Input>
    struct is_composable_helper<
        void_t<decltype(std::declval<Outer>()(
            std::declval<Inner>()(std::declval<Input>())))>,
        Outer,
        Inner,
        Input> : std::true_type
    {};

    // is_composed_transformer_helper
    //   trait: detection sink for the .first() / .second() const
    // accessor surface of composed_transformer_helper (primary:
    // false).
    template<typename AlwaysVoid,
             typename Type>
    struct is_composed_transformer_helper : std::false_type
    {};

    // is_composed_transformer_helper (well-formed specialization)
    //   trait: true when Type exposes both .first() and .second()
    // const accessors.
    template<typename Type>
    struct is_composed_transformer_helper<
        void_t<decltype(std::declval<const Type&>().first()),
               decltype(std::declval<const Type&>().second())>,
        Type> : std::true_type
    {};

    // is_memoized_helper
    //   trait: detection sink for the .clear_cache() / .cache_size()
    // const surface of memoize_helper (primary: false).
    template<typename AlwaysVoid,
             typename Type>
    struct is_memoized_helper : std::false_type
    {};

    // is_memoized_helper (well-formed specialization)
    //   trait: true when Type exposes both .clear_cache() and
    // .cache_size() const accessors.
    template<typename Type>
    struct is_memoized_helper<
        void_t<decltype(std::declval<const Type&>().clear_cache()),
               decltype(std::declval<const Type&>().cache_size())>,
        Type> : std::true_type
    {};

NS_END  // internal


// is_invocable, is_invocable_r
//   The callable traits are core/meta/type_traits.hpp's: std::is_invocable and
// std::is_invocable_r from C++17, its own C++11 forms below that. This header
// used to define both itself, which no translation unit could include beside
// core/meta/type_traits.hpp (math/functional.hpp, which includes both,
// failed on it).

NS_INTERNAL

    // is_unary_transformer_helper
    //   trait: non-void branch of is_unary_transformer, selected by the
    // Invocable flag. Primary (false flag): not invocable, so the
    // result type is never named -- guarantees SFINAE-safety.
    template<bool     Invocable,
             typename Function,
             typename Input>
    struct is_unary_transformer_helper : std::false_type
    {};

    // is_unary_transformer_helper (invocable branch)
    //   trait: when invocable, the value is whether the result is
    // non-void.
    template<typename Function,
             typename Input>
    struct is_unary_transformer_helper<true, Function, Input>
    {
        static constexpr bool value =
            !std::is_void<
                callable_result_t<Function, Input>>::value;
    };

NS_END  // internal

// is_unary_transformer
//   trait: true when Function is callable with a single Input and
// produces a non-void result (the shape a composition stage expects).
// The non-void check is gated behind invocability so the result type
// is never named in the non-invocable case (SFINAE-safe).
template<typename Function,
         typename Input>
struct is_unary_transformer
    : internal::is_unary_transformer_helper<
          is_invocable<Function, Input>::value,
          Function, Input>
{};

// is_composable
//   trait: true when the call chain Outer(Inner(Input)) is
// well-formed -- i.e. Inner accepts Input and Outer accepts the
// result of Inner.
template<typename Outer,
         typename Inner,
         typename Input>
struct is_composable
    : internal::is_composable_helper<void, Outer, Inner, Input>
{};

// composition_result
//   trait: result type of Outer(Inner(Input)).
template<typename Outer,
         typename Inner,
         typename Input>
struct composition_result
{
    using type =
        callable_result_t<Outer, callable_result_t<Inner, Input>>;
};

// composition_result_t
//   type: convenience alias for composition_result<...>::type.
template<typename Outer,
         typename Inner,
         typename Input>
using composition_result_t =
    typename composition_result<Outer, Inner, Input>::type;

// is_composed_transformer
//   trait: structural predicate -- true when Type exposes the
// .first() / .second() introspection surface produced by
// compose / pipe / compose_transformer.
template<typename Type>
struct is_composed_transformer
    : internal::is_composed_transformer_helper<void, Type>
{};

// is_memoized
//   trait: structural predicate -- true when Type exposes the
// .clear_cache() / .cache_size() surface produced by memoize.
template<typename Type>
struct is_memoized
    : internal::is_memoized_helper<void, Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_unary_transformer_v
    //   value: convenience alias for is_unary_transformer<...>::value.
    template<typename Function,
             typename Input>
    constexpr bool is_unary_transformer_v =
        is_unary_transformer<Function, Input>::value;

    // is_composable_v
    //   value: convenience alias for is_composable<...>::value.
    template<typename Outer,
             typename Inner,
             typename Input>
    constexpr bool is_composable_v =
        is_composable<Outer, Inner, Input>::value;

    // is_composed_transformer_v
    //   value: convenience alias for is_composed_transformer<...>::value.
    template<typename Type>
    constexpr bool is_composed_transformer_v =
        is_composed_transformer<Type>::value;

    // is_memoized_v
    //   value: convenience alias for is_memoized<...>::value.
    template<typename Type>
    constexpr bool is_memoized_v = is_memoized<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // invocable_with
    //   concept: satisfied when Function is callable with Args... .
    // Named to avoid colliding with the std::invocable re-export in
    // concepts.hpp.
    template<typename    Function,
             typename... Args>
    concept invocable_with = is_invocable<Function, Args...>::value;

    // unary_transformer
    //   concept: satisfied when Function maps a single Input to a
    // non-void result.
    template<typename Function,
             typename Input>
    concept unary_transformer = is_unary_transformer<Function, Input>::value;

    // composable
    //   concept: satisfied when Outer(Inner(Input)) is a valid call
    // chain.
    template<typename Outer,
             typename Inner,
             typename Input>
    concept composable = is_composable<Outer, Inner, Input>::value;

    // composed_transformer_like
    //   concept: satisfied when Type exposes the .first() / .second()
    // composed-transformer surface.
    template<typename Type>
    concept composed_transformer_like = is_composed_transformer<Type>::value;

    // memoized_like
    //   concept: satisfied when Type exposes the .clear_cache() /
    // .cache_size() memoize surface.
    template<typename Type>
    concept memoized_like = is_memoized<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             I.    COMPOSED TRANSFORMER                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // composed_transformer_helper
    //   helper: composition of two transformers with optional contexts.
    // Mirrors d_composed_transformer from compose.h but is fully typed.
    // compose(f, g)(x) = f(g(x)), so Second is applied first.
    template<typename First,
             typename Second>
    class composed_transformer_helper
    {
    public:
        template<typename F1Fwd,
                 typename F2Fwd>
        D_CONSTEXPR
        composed_transformer_helper
        (
            F1Fwd&& _first,
            F2Fwd&& _second
        ) : m_first(std::forward<F1Fwd>(_first))
          , m_second(std::forward<F2Fwd>(_second))
        {}

        template<typename Input>
        D_CONSTEXPR
        auto operator()(const Input& _input) const
            -> decltype(std::declval<const Second&>()(
                std::declval<const First&>()(_input)))
        {
            return m_second(m_first(_input));
        }

        template<typename Input>
        D_CONSTEXPR
        auto operator()(Input&& _input) const
            -> decltype(std::declval<const Second&>()(
                std::declval<const First&>()(
                    std::forward<Input>(_input))))
        {
            return m_second(m_first(std::forward<Input>(_input)));
        }

        // accessors for introspection
        D_CONSTEXPR const First&  first()  const { return m_first; }
        D_CONSTEXPR const Second& second() const { return m_second; }

    private:
        First   m_first;   // applied first (g in f(g(x)))
        Second m_second;  // applied second (f in f(g(x)))
    };

    // partial_consumer_helper
    //   helper: partially applied consumer with bound context.
    // Mirrors d_partial_consumer from compose.h.
    template<typename Function,
             typename BoundArg>
    class partial_consumer_helper
    {
    public:
        template<typename FunctionFwd,
                 typename ArgFwd>
        D_CONSTEXPR
        partial_consumer_helper(FunctionFwd&& _function, ArgFwd&& _arg)
            : m_fn(std::forward<FunctionFwd>(_function))
            , m_bound(std::forward<ArgFwd>(_arg))
        {}

        template<typename... Args>
        D_CONSTEXPR
        auto operator()(Args&&... _args) const
            -> decltype(std::declval<const Function&>()(
                std::forward<Args>(_args)...,
                std::declval<const BoundArg&>()))
        {
            return m_fn(std::forward<Args>(_args)..., m_bound);
        }

    private:
        Function m_fn;
        BoundArg m_bound;
    };

    // tap_helper
    //   helper: passes value through a side-effect function, returning
    // the original value unchanged.
    template<typename Function>
    class tap_helper
    {
    public:
        // the forwarding constructor is constrained out when the
        // argument is itself a tap_helper, so the compiler-generated
        // copy / move constructors are not hijacked (which would
        // otherwise try to store a tap_helper into m_fn).
        template<typename FunctionFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<FunctionFwd>::type,
                         tap_helper>::value>::type>
        explicit D_CONSTEXPR
        tap_helper(FunctionFwd&& _function)
            : m_fn(std::forward<FunctionFwd>(_function))
        {}

        template<typename Type>
        D_CONSTEXPR_CPP14
        Type operator()(Type _value) const
        {
            m_fn(_value);

            return _value;
        }

    private:
        Function m_fn;
    };

    // memoize_helper
    //   helper: caches results of a pure function.
    template<typename Function,
             typename Input,
             typename Output>
    class memoize_helper
    {
    public:
        template<typename FunctionFwd>
        explicit memoize_helper(FunctionFwd&& _function)
            : m_fn(std::forward<FunctionFwd>(_function))
        {}

        Output operator()(const Input& _input) const
        {
            auto it = m_cache.find(_input);

            if (it != m_cache.end())
            {
                return it->second;
            }

            Output result = m_fn(_input);

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
        Function                            m_fn;
        mutable std::map<Input, Output>   m_cache;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FACTORIES (compose_transformer / compose / pipe)      ///
///////////////////////////////////////////////////////////////////////////////

// compose_transformer
//   function: creates a composed transformer that applies _first then
// _second. compose_transformer(g, f)(x) = f(g(x))
// This is left-to-right composition (pipe order).
template<typename First,
         typename Second>
D_CONSTEXPR
internal::composed_transformer_helper<typename std::decay<First>::type,
                                      typename std::decay<Second>::type>
compose_transformer
(
    First&&  _first,
    Second&& _second
)
{
    return internal::composed_transformer_helper<
        typename std::decay<First>::type,
        typename std::decay<Second>::type>(
            std::forward<First>(_first),
            std::forward<Second>(_second));
}

// compose
//   function: math-order composition. compose(f, g)(x) = f(g(x)).
// _inner (g) is applied first, _outer (f) second.
template<typename Outer,
         typename Inner>
D_CONSTEXPR
internal::composed_transformer_helper<typename std::decay<Inner>::type,
                                      typename std::decay<Outer>::type>
compose
(
    Outer&& _outer,
    Inner&& _inner
)
{
    return internal::composed_transformer_helper<
        typename std::decay<Inner>::type,
        typename std::decay<Outer>::type>(
            std::forward<Inner>(_inner),
            std::forward<Outer>(_outer));
}

// pipe
//   function: left-to-right composition. pipe(f, g)(x) = g(f(x)).
// _first (f) is applied first, _second (g) second. Equivalent in shape
// to compose_transformer.
template<typename First,
         typename Second>
D_CONSTEXPR
internal::composed_transformer_helper<typename std::decay<First>::type,
                                      typename std::decay<Second>::type>
pipe
(
    First&&  _first,
    Second&& _second
)
{
    return internal::composed_transformer_helper<
        typename std::decay<First>::type,
        typename std::decay<Second>::type>(
            std::forward<First>(_first),
            std::forward<Second>(_second));
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
    template<typename... Functions>
    struct compose_all_fold;

    // compose_all_fold<Function>
    //   helper: base case -- a single function folds to itself.
    template<typename Function>
    struct compose_all_fold<Function>
    {
        using type = typename std::decay<Function>::type;

        template<typename Fwd>
        static D_CONSTEXPR type apply(Fwd&& _function)
        {
            return std::forward<Fwd>(_function);
        }
    };

    // compose_all_fold<Function1, Function2, Rest...>
    //   helper: recursive case -- compose(f1, fold(f2, rest...)).
    template<typename    Function1,
             typename    Function2,
             typename... Rest>
    struct compose_all_fold<Function1, Function2, Rest...>
    {
        using tail = compose_all_fold<Function2, Rest...>;
        using type = decltype(djinterp::compose(
            std::declval<Function1>(),
            std::declval<typename tail::type>()));

        template<typename    F1,
                 typename    F2,
                 typename... R>
        static D_CONSTEXPR type apply(F1&& _f1, F2&& _f2, R&&... _rest)
        {
            return djinterp::compose(
                std::forward<F1>(_f1),
                tail::apply(std::forward<F2>(_f2),
                            std::forward<R>(_rest)...));
        }
    };

    // pipe_all_fold
    //   helper: left-to-right fold of pipe over a function pack
    // (primary template; specialized below).
    template<typename... Functions>
    struct pipe_all_fold;

    // pipe_all_fold<Function>
    //   helper: base case -- a single function folds to itself.
    template<typename Function>
    struct pipe_all_fold<Function>
    {
        using type = typename std::decay<Function>::type;

        template<typename Fwd>
        static D_CONSTEXPR type apply(Fwd&& _function)
        {
            return std::forward<Fwd>(_function);
        }
    };

    // pipe_all_fold<Function1, Function2, Rest...>
    //   helper: recursive case -- fold(pipe(f1, f2), rest...).
    template<typename    Function1,
             typename    Function2,
             typename... Rest>
    struct pipe_all_fold<Function1, Function2, Rest...>
    {
        using piped = decltype(djinterp::pipe(std::declval<Function1>(),
                                              std::declval<Function2>()));
        using tail  = pipe_all_fold<piped, Rest...>;
        using type  = typename tail::type;

        template<typename    F1,
                 typename    F2,
                 typename... R>
        static D_CONSTEXPR type apply(F1&& _f1, F2&& _f2, R&&... _rest)
        {
            return tail::apply(djinterp::pipe(std::forward<F1>(_f1),
                                              std::forward<F2>(_f2)),
                               std::forward<R>(_rest)...);
        }
    };

NS_END  // internal

// compose_all (right-to-left)
//   function: composes N functions right-to-left.
// compose_all(f, g, h)(x) = f(g(h(x)))
template<typename... Functions>
D_CONSTEXPR
typename internal::compose_all_fold<Functions...>::type
compose_all
(
    Functions&&... _functions
)
{
    return internal::compose_all_fold<Functions...>::apply(
        std::forward<Functions>(_functions)...);
}

// pipe_all (left-to-right)
//   function: composes N functions left-to-right.
// pipe_all(f, g, h)(x) = h(g(f(x)))
template<typename... Functions>
D_CONSTEXPR
typename internal::pipe_all_fold<Functions...>::type
pipe_all
(
    Functions&&... _functions
)
{
    return internal::pipe_all_fold<Functions...>::apply(
        std::forward<Functions>(_functions)...);
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   PARTIAL APPLICATION                                   ///
///////////////////////////////////////////////////////////////////////////////

// partial_back
//   function: partially applies the last argument of a function.
// partial_back(f, z)(x, y) = f(x, y, z)
template<typename Function,
         typename Arg>
D_CONSTEXPR
internal::partial_consumer_helper<typename std::decay<Function>::type,
                                  typename std::decay<Arg>::type>
partial_back
(
    Function&& _function,
    Arg&&      _arg
)
{
    return internal::partial_consumer_helper<
        typename std::decay<Function>::type,
        typename std::decay<Arg>::type>(
            std::forward<Function>(_function),
            std::forward<Arg>(_arg)
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
template<typename Function>
D_CONSTEXPR
internal::tap_helper<typename std::decay<Function>::type>
tap(Function&& _function)
{
    return internal::tap_helper<typename std::decay<Function>::type>(
        std::forward<Function>(_function));
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   MEMOIZATION                                           ///
///////////////////////////////////////////////////////////////////////////////

// memoize
//   function: wraps a pure function with a cache.
// The function must be deterministic (same input -> same output).
// The input type must be comparable (for use as map key).
template<typename Function,
         typename Input,
         typename Output = callable_result_t<Function, const Input&>>
internal::memoize_helper<typename std::decay<Function>::type, Input, Output>
memoize
(
    Function&& _function
)
{
    return internal::memoize_helper<
        typename std::decay<Function>::type, Input, Output>(
            std::forward<Function>(_function)
        );
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  FIXED-POINT COMBINATOR                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // fix_helper
    //   helper: Y combinator for expressing recursive lambdas.
    template<typename Function>
    class fix_helper
    {
    public:
        // the forwarding constructor is constrained out when the
        // argument is itself a fix_helper, so the compiler-generated
        // copy / move constructors are not hijacked.
        template<typename FunctionFwd,
                 typename = typename std::enable_if<
                     !std::is_same<
                         typename std::decay<FunctionFwd>::type,
                         fix_helper>::value>::type>
        explicit D_CONSTEXPR
        fix_helper(FunctionFwd&& _function)
            : m_fn(std::forward<FunctionFwd>(_function))
        {}

        template<typename... Args>
        D_CONSTEXPR
        auto operator()(Args&&... _args) const
            -> decltype(std::declval<const Function&>()(
                std::declval<const fix_helper&>(),
                std::forward<Args>(_args)...))
        {
            return m_fn(*this, std::forward<Args>(_args)...);
        }

    private:
        Function m_fn;
    };

NS_END  // internal

// fix
//   function: Y combinator for recursive lambdas.
// Usage:
//   auto factorial = fix([](auto& self, int n) -> int {
//       return n <= 1 ? 1 : n * self(n - 1);
//   });
//   factorial(5);  // 120
template<typename Function>
D_CONSTEXPR internal::fix_helper<typename std::decay<Function>::type>
fix(
    Function&& _function
)
{
    return internal::fix_helper<typename std::decay<Function>::type>(
        std::forward<Function>(_function));
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
    //   helper: composition of two transformers. Applies First
    // first, then Second to the result. operator()(x) yields
    // m_second(m_first(x)).
    //
    //   Return type is Second::result_type, taken from the
    // Adaptable Function Object convention. Second must expose
    // a result_type typedef. First's result type need not be
    // exposed; it is computed at the call site by the compiler
    // and fed straight into m_second.
    template<typename First,
             typename Second>
    class composed_transformer_helper
    {
    public:
        typedef typename Second::result_type result_type;

        composed_transformer_helper(
            const First&  _first,
            const Second& _second
        )
            : m_first(_first)
            , m_second(_second)
        {}

        template<typename Input>
        result_type operator()(
            const Input& _input
        ) const
        {
            return m_second(m_first(_input));
        }

        // accessors for introspection
        const First&  first()  const { return m_first;  }
        const Second& second() const { return m_second; }

    private:
        First   m_first;
        Second m_second;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

// compose_transformer
//   function: builds a composed transformer that applies _first
// then _second. compose_transformer(g, f)(x) = f(g(x)).
// This is the pipe-order form: arguments read in execution order.
template<typename First,
         typename Second>
internal::composed_transformer_helper<First, Second>
compose_transformer(
    const First&  _first,
    const Second& _second
)
{
    return internal::composed_transformer_helper<First, Second>(_first,
                                                                  _second);
}


// compose
//   function: math-order composition. compose(f, g)(x) = f(g(x)).
// _g is applied first (inner), _f second (outer).
template<typename F,
         typename G>
internal::composed_transformer_helper<G, F>
compose(
    const F& _f,
    const G& _g
)
{
    return internal::composed_transformer_helper<G, F>(_g, _f);
}


// pipe
//   function: left-to-right composition. pipe(f, g)(x) = g(f(x)).
// _f is applied first, _g second. Equivalent in shape to
// compose_transformer.
template<typename F,
         typename G>
internal::composed_transformer_helper<F, G>
pipe(
    const F& _f,
    const G& _g
)
{
    return internal::composed_transformer_helper<F, G>(_f, _g);
}


///////////////////////////////////////////////////////////////////////////////
///             III.  MEMOIZE                                               ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // memoize_helper
    //   helper: caches results of a pure unary function. Input
    // must be LessThanComparable (required for std::map keys).
    //
    //   The cache is mutable; operator() is const so memoize_helper
    // satisfies the const-callable convention required by the
    // composition helpers above.
    template<typename Function,
             typename Input,
             typename Output>
    class memoize_helper
    {
    public:
        typedef Output result_type;
        typedef Input   argument_type;

        explicit memoize_helper(
            const Function& _function
        )
            : m_fn(_function)
            , m_cache()
        {}

        Output operator()(
            const Input& _input
        ) const
        {
            typename std::map<Input, Output>::iterator it
                = m_cache.find(_input);

            if (it != m_cache.end())
            {
                return it->second;
            }

            Output result = m_fn(_input);

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
        Function                           m_fn;
        mutable std::map<Input, Output>  m_cache;
    };

NS_END  // internal


// memoize
//   function: wraps a pure unary function with a cache. Because
// C++98 cannot deduce the return type from the function via
// decltype, Input and Output are explicit template parameters;
// Function is deduced from the argument.
//
//   The wrapped function must be deterministic (same input ->
// same output) for the cache to be sound, and Input must support
// operator< (it is used as a std::map key).
//
//   Usage:
//     int slow(int);
//     ...
//     djinterp::memoize<int, int>(&slow)  -> memoize_helper<...>
template<typename Input,
         typename Output,
         typename Function>
internal::memoize_helper<Function, Input, Output>
memoize(
    const Function& _function
)
{
    return internal::memoize_helper<Function,
                                    Input,
                                    Output>(_function);
}


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_COMPOSE_HPP
