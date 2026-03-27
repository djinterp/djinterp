/******************************************************************************
* djinterp [functional]                                            compose.hpp
*
* Template function composition and partial application (C++).
*   Provides SFINAE-constrained composed transformers, partial application
* helpers, and variadic composition chains. Unlike the C version which uses
* void* and temp buffers, these are fully typed with zero runtime overhead
* from type erasure.
*
*   Extends the existing compose/pipe utilities in functional.hpp with:
*     - composed_transformer class (stateful composition with context)
*     - partial application for any callable
*     - variadic compose/pipe chains
*     - memoization wrapper
*     - tap (side-effect injection)
*
* path:      /inc/functional/compose.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COMPOSE_
#define DJINTERP_FUNCTIONAL_COMPOSE_ 1

#include <cstddef>
#include <functional>
#include <map>
#include <type_traits>
#include <utility>
#include "./djinterp.h"
#include "./env.h"
#include "./cpp_features.h"
#include "./functional.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP
NS_FUNCTIONAL


///////////////////////////////////////////////////////////////////////////////
///             I.    COMPOSED TRANSFORMER                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // composed_transformer
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
        template<typename _FunctionFwd>
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
///             II.   COMPOSED TRANSFORMER FACTORY                          ///
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


///////////////////////////////////////////////////////////////////////////////
///             III.  VARIADIC COMPOSITION                                  ///
///////////////////////////////////////////////////////////////////////////////

// compose_all (right-to-left)
//   function: composes N functions right-to-left.
// compose_all(f, g, h)(x) = f(g(h(x)))
template<typename _Function>
D_CONSTEXPR
auto compose_all(
    _Function&& _function
) -> typename std::decay<_Function>::type
{
    return std::forward<_Function>(_function);
}

template<typename    _Function1,
         typename    _Function2,
         typename... _Functions>
D_CONSTEXPR
auto compose_all(
    _Function1&&    _function1,
    _Function2&&    _function2,
    _Functions&&... _rest
)
-> decltype(compose(std::forward<_Function1>(_function1),
                    compose_all(std::forward<_Function2>(_function2),
                                std::forward<_Functions>(_rest)...)))
{
    return compose(std::forward<_Function1>(_function1),
                   compose_all(std::forward<_Function2>(_function2),
                               std::forward<_Functions>(_rest)...));
}

// pipe_all (left-to-right)
//   function: composes N functions left-to-right.
// pipe_all(f, g, h)(x) = h(g(f(x)))
template<typename _Function>
D_CONSTEXPR
auto pipe_all(
    _Function&& _function
) -> typename std::decay<_Function>::type
{
    return std::forward<_Function>(_function);
}

template<typename    _Function1,
         typename    _Function2,
         typename... _Functions>
D_CONSTEXPR
auto pipe_all
(
    _Function1&&    _function1,
    _Function2&&    _function2,
    _Functions&&... _rest
) -> decltype(pipe_all(pipe(std::forward<_Function1>(_function1),
                            std::forward<_Function2>(_function2)),
                            std::forward<_Functions>(_rest)...))
{
    return pipe_all(pipe(std::forward<_Function1>(_function1),
                         std::forward<_Function2>(_function2)),
                    std::forward<_Functions>(_rest)...);
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
        template<typename _FunctionFwd>
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
D_CONSTEXPR
internal::fix_helper<typename std::decay<_Function>::type>
fix(_Function&& _function)
{
    return internal::fix_helper<typename std::decay<_Function>::type>(
        std::forward<_Function>(_function));
}


NS_END  // functional
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_COMPOSE_HPP_
