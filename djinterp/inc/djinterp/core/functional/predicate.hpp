/******************************************************************************
* djinterp [functional]                                          predicate.hpp
*
* Template predicate combinators for the functional module (C++).
*   Provides type-safe, SFINAE-constrained predicate combinators that compose
* predicates with AND, OR, XOR, and NOT operations. Unlike the C version
* which uses void* and function pointers, these combinators are fully typed
* and work with any callable (lambdas, function objects, function pointers,
* std::function, etc.).
*   Each combinator stores its predicates by value (decayed), supports
* perfect forwarding at call sites in C++11+ mode, and propagates noexcept.
*   In C++98 mode, perfect forwarding and variadic operator() are
* unavailable. Each combinator instead exposes fixed-arity operator()
* overloads (unary and binary) and takes its predicates by const&. The
* variadic factories (all_of / any_of / none_of) and the trait-detection
* block at the bottom of the file are gated to C++11+ only. See the
* `cpp98 roadmap` workbook for the full feature inventory.
*
* USAGE:
*   auto combo = predicate_and(is_positive, is_even);
*   bool result = combo(42);   // true if both return true
*   auto chain = predicate_or(
*       predicate_not(is_negative),
*       predicate_and(is_small, is_prime));
*
*   DUAL-DOMAIN (compile-time + runtime): each combinator stores its
* predicates by value (decayed) and its operator() is constexpr, forwarding to
* the stored predicates without inspecting the operand domain.  With
* carrier-callable predicate leaves (see core/meta/carrier.hpp) the AND/OR/XOR/
* NOT/NAND/NOR combinators and the variadic all_of / any_of / none_of therefore
* evaluate AT COMPILE TIME over NTTP value carriers and over type carriers,
* yielding a constexpr bool, as well as running unchanged at runtime:
*     predicate_and(is_even, is_positive)(val<10>)  -> true  (constexpr)
*     predicate_not(is_pointer)(type_c<int>)        -> true  (constexpr)
* predicate combinators stay carrier-agnostic; the carriers live at the call
* site, so this header needs no dependency on carrier.hpp.
*
* path:      /inc/djinterp/core/functional/predicate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PREDICATE_
#define DJINTERP_FUNCTIONAL_PREDICATE_ 1

// std
#include <cstddef>
#include <utility>
// djinterp
#include "../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#  include <type_traits>
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP


// combinator classes
NS_INTERNAL
    // predicate_and_combinator
    //   helper: evaluates two predicates with logical AND
    // (short-circuiting). In C++11+ mode operator() is variadic
    // and perfect-forwards arguments to both predicates; in C++98
    // mode operator() is overloaded for fixed arity (unary and
    // binary). The arity-1 and arity-2 forms cover the typical
    // uses (element predicates and binary relations); higher
    // arities can be added by hand if needed.
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_and_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_and_combinator
        (
            _Predicate1Fwd&& _predicate1,
            _Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<_Predicate1Fwd>(_predicate1)),
              m_predicate2(std::forward<_Predicate2Fwd>(_predicate2))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return m_predicate1(std::forward<_Args>(_args)...) &&
                   m_predicate2(std::forward<_Args>(_args)...);
        }
#else
        // C++98 fallback: const-ref ctor + fixed-arity overloads.
        predicate_and_combinator(
            const _Predicate1& _predicate1,
            const _Predicate2& _predicate2
        )
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename _Arg>
        bool operator()(const _Arg& _arg) const
        {
            return m_predicate1(_arg) && m_predicate2(_arg);
        }

        template<typename _A,
                 typename _B>
        bool operator()(const _A& _a, const _B& _b) const
        {
            return m_predicate1(_a, _b) && m_predicate2(_a, _b);
        }
#endif

        // accessors for introspection
        D_CONSTEXPR const _Predicate1&
        first()  const
        {
            return m_predicate1;
        }

        D_CONSTEXPR const _Predicate2&
        second() const
        {
            return m_predicate2;
        }
    };

    // predicate_or_combinator
    //   helper: evaluates two predicates with logical OR
    // (short-circuiting).
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_or_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_or_combinator
        (
            _Predicate1Fwd&& _predicate1,
            _Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<_Predicate1Fwd>(_predicate1)),
              m_predicate2(std::forward<_Predicate2Fwd>(_predicate2))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return m_predicate1(std::forward<_Args>(_args)...) ||
                   m_predicate2(std::forward<_Args>(_args)...);
        }
#else
        predicate_or_combinator(
            const _Predicate1& _predicate1,
            const _Predicate2& _predicate2
        )
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename _Arg>
        bool operator()(const _Arg& _arg) const
        {
            return m_predicate1(_arg) || m_predicate2(_arg);
        }

        template<typename _A,
                 typename _B>
        bool operator()(const _A& _a, const _B& _b) const
        {
            return m_predicate1(_a, _b) || m_predicate2(_a, _b);
        }
#endif

        D_CONSTEXPR const _Predicate1& first()  const { return m_predicate1; }
        D_CONSTEXPR const _Predicate2& second() const { return m_predicate2; }
    };

    // predicate_xor_combinator
    //   helper: evaluates two predicates with logical XOR.
    // Both predicates are always evaluated (XOR has no
    // short-circuit).
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_xor_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_xor_combinator
        (
            _Predicate1Fwd&& _predicate1,
            _Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<_Predicate1Fwd>(_predicate1)),
              m_predicate2(std::forward<_Predicate2Fwd>(_predicate2))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            bool a = m_predicate1(std::forward<_Args>(_args)...);
            bool b = m_predicate2(std::forward<_Args>(_args)...);

            return a != b;
        }
#else
        predicate_xor_combinator(
            const _Predicate1& _predicate1,
            const _Predicate2& _predicate2
        )
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename _Arg>
        bool operator()(const _Arg& _arg) const
        {
            const bool a = m_predicate1(_arg);
            const bool b = m_predicate2(_arg);
            return a != b;
        }

        template<typename _A,
                 typename _B>
        bool operator()(const _A& _a, const _B& _b) const
        {
            const bool a = m_predicate1(_a, _b);
            const bool b = m_predicate2(_a, _b);
            return a != b;
        }
#endif

        D_CONSTEXPR const _Predicate1& first()  const { return m_predicate1; }
        D_CONSTEXPR const _Predicate2& second() const { return m_predicate2; }
    };

    // predicate_not_combinator
    //   helper: negates a single predicate.
    template<typename _Predicate>
    class predicate_not_combinator
    {
    private:
        _Predicate m_pred;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename _PredicateFwd>
        explicit D_CONSTEXPR
        predicate_not_combinator(_PredicateFwd&& _predicate)
            : m_pred(std::forward<_PredicateFwd>(_predicate))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return !m_pred(std::forward<_Args>(_args)...);
        }
#else
        explicit
        predicate_not_combinator(const _Predicate& _predicate)
            : m_pred(_predicate)
        {}

        template<typename _Arg>
        bool operator()(const _Arg& _arg) const
        {
            return !m_pred(_arg);
        }

        template<typename _A,
                 typename _B>
        bool operator()(const _A& _a, const _B& _b) const
        {
            return !m_pred(_a, _b);
        }
#endif

        D_CONSTEXPR const _Predicate& inner() const { return m_pred; }
    };

    // predicate_nand_combinator
    //   helper: evaluates two predicates with logical NAND.
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_nand_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_nand_combinator(
            _Predicate1Fwd&& _predicate1,
            _Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<_Predicate1Fwd>(_predicate1))
            , m_predicate2(std::forward<_Predicate2Fwd>(_predicate2))
        {}

        template<typename... _Args>
        D_CONSTEXPR bool
        operator()(
            _Args&&... _args
        ) const
        {
            return !(m_predicate1(std::forward<_Args>(_args)...) &&
                     m_predicate2(std::forward<_Args>(_args)...));
        }
#else
        predicate_nand_combinator(const _Predicate1& _predicate1,
                                  const _Predicate2& _predicate2)
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename _Arg>
        bool operator()(const _Arg& _arg) const
        {
            return !(m_predicate1(_arg) && m_predicate2(_arg));
        }

        template<typename _A,
                 typename _B>
        bool operator()(const _A& _a, const _B& _b) const
        {
            return !(m_predicate1(_a, _b) && m_predicate2(_a, _b));
        }
#endif
    };

    // predicate_nor_combinator
    //   helper: evaluates two predicates with logical NOR.
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_nor_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_nor_combinator(_Predicate1Fwd&& _predicate1,
                                 _Predicate2Fwd&& _predicate2)
            : m_predicate1(std::forward<_Predicate1Fwd>(_predicate1))
            , m_predicate2(std::forward<_Predicate2Fwd>(_predicate2))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return !(m_predicate1(std::forward<_Args>(_args)...) ||
                     m_predicate2(std::forward<_Args>(_args)...));
        }
#else
        predicate_nor_combinator(const _Predicate1& _predicate1,
                                 const _Predicate2& _predicate2)
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename _Arg>
        bool operator()(const _Arg& _arg) const
        {
            return !(m_predicate1(_arg) || m_predicate2(_arg));
        }

        template<typename _A,
                 typename _B>
        bool operator()(
            const _A& _a, 
            const _B& _b
        ) const
        {
            return !(m_predicate1(_a, _b) || m_predicate2(_a, _b));
        }
#endif
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        FACTORIES                                                        ///
///////////////////////////////////////////////////////////////////////////////

// predicate_and
//   function: creates an AND combinator from two predicates.
// predicate_and(p1, p2)(x) = p1(x) && p2(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR
internal::predicate_and_combinator<typename std::decay<_Predicate1>::type,
                                   typename std::decay<_Predicate2>::type>
predicate_and(
    _Predicate1&& _predicate1,
    _Predicate2&& _predicate2
)
{
    return internal::predicate_and_combinator<
        typename std::decay<_Predicate1>::type,
        typename std::decay<_Predicate2>::type>(
            std::forward<_Predicate1>(_predicate1),
            std::forward<_Predicate2>(_predicate2));
}
#else
template<typename _Predicate1,
         typename _Predicate2>
internal::predicate_and_combinator<_Predicate1, _Predicate2>
predicate_and
(
    const _Predicate1& _predicate1,
    const _Predicate2& _predicate2
)
{
    return internal::predicate_and_combinator<_Predicate1, _Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_or
//   function: creates an OR combinator from two predicates.
// predicate_or(p1, p2)(x) = p1(x) || p2(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR
internal::predicate_or_combinator<typename std::decay<_Predicate1>::type,
                                  typename std::decay<_Predicate2>::type>
predicate_or
(
    _Predicate1&& _predicate1,
    _Predicate2&& _predicate2
)
{
    return internal::predicate_or_combinator<
        typename std::decay<_Predicate1>::type,
        typename std::decay<_Predicate2>::type>(
            std::forward<_Predicate1>(_predicate1),
            std::forward<_Predicate2>(_predicate2));
}
#else
template<typename _Predicate1,
         typename _Predicate2>
internal::predicate_or_combinator<_Predicate1, _Predicate2>
predicate_or
(
    const _Predicate1& _predicate1,
    const _Predicate2& _predicate2
)
{
    return internal::predicate_or_combinator<_Predicate1, _Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_xor
//   function: creates an XOR combinator from two predicates.
// predicate_xor(p1, p2)(x) = p1(x) != p2(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR internal::predicate_xor_combinator<
    typename std::decay<_Predicate1>::type,
    typename std::decay<_Predicate2>::type>
predicate_xor(
    _Predicate1&& _predicate1,
    _Predicate2&& _predicate2
)
{
    return internal::predicate_xor_combinator<
        typename std::decay<_Predicate1>::type,
        typename std::decay<_Predicate2>::type>(
            std::forward<_Predicate1>(_predicate1),
            std::forward<_Predicate2>(_predicate2));
}
#else
template<typename _Predicate1,
         typename _Predicate2>
internal::predicate_xor_combinator<_Predicate1, _Predicate2>
predicate_xor
(
    const _Predicate1& _predicate1,
    const _Predicate2& _predicate2
)
{
    return internal::predicate_xor_combinator<_Predicate1, _Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_not
//   function: creates a NOT combinator that negates a predicate.
// predicate_not(p)(x) = !p(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _Predicate>
D_CONSTEXPR
internal::predicate_not_combinator<typename std::decay<_Predicate>::type>
predicate_not
(
    _Predicate&& _predicate
)
{
    return internal::predicate_not_combinator<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}
#else
template<typename _Predicate>
internal::predicate_not_combinator<_Predicate>
predicate_not
(
    const _Predicate& _predicate
)
{
    return internal::predicate_not_combinator<_Predicate>(_predicate);
}
#endif

// predicate_nand
//   function: creates a NAND combinator from two predicates.
// predicate_nand(p1, p2)(x) = !(p1(x) && p2(x))
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR
internal::predicate_nand_combinator<typename std::decay<_Predicate1>::type,
                                    typename std::decay<_Predicate2>::type>
predicate_nand
(
    _Predicate1&& _predicate1,
    _Predicate2&& _predicate2
)
{
    return internal::predicate_nand_combinator<
        typename std::decay<_Predicate1>::type,
        typename std::decay<_Predicate2>::type>(
            std::forward<_Predicate1>(_predicate1),
            std::forward<_Predicate2>(_predicate2));
}
#else
template<typename _Predicate1,
         typename _Predicate2>
internal::predicate_nand_combinator<_Predicate1, _Predicate2>
predicate_nand
(
    const _Predicate1& _predicate1,
    const _Predicate2& _predicate2
)
{
    return internal::predicate_nand_combinator<_Predicate1, _Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_nor
//   function: creates a NOR combinator from two predicates.
// predicate_nor(p1, p2)(x) = !(p1(x) || p2(x))
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR
internal::predicate_nor_combinator<typename std::decay<_Predicate1>::type,
                                   typename std::decay<_Predicate2>::type>
predicate_nor(_Predicate1&& _predicate1, _Predicate2&& _predicate2)
{
    return internal::predicate_nor_combinator<
        typename std::decay<_Predicate1>::type,
        typename std::decay<_Predicate2>::type>(
            std::forward<_Predicate1>(_predicate1),
            std::forward<_Predicate2>(_predicate2));
}
#else
template<typename _Predicate1,
         typename _Predicate2>
internal::predicate_nor_combinator<_Predicate1, _Predicate2>
predicate_nor(const _Predicate1& _predicate1, const _Predicate2& _predicate2)
{
    return internal::predicate_nor_combinator<_Predicate1, _Predicate2>(
        _predicate1, _predicate2);
}
#endif


///////////////////////////////////////////////////////////////////////////////
///        VARIADIC HELPERS  (C++11+ only)                                  ///
///////////////////////////////////////////////////////////////////////////////
// Variadic packs are RED in C++98; all_of / any_of / none_of are gated
// to C++11+. In C++98 mode, callers must nest binary combinators by
// hand:  predicate_and(p1, predicate_and(p2, p3)).

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// The variadic folds below route their recursion through helper structs
// rather than through self-referential trailing-return decltype on the
// function templates themselves. The original formulation,
//
//     auto all_of(p1, p2, rest...) -> decltype(all_of(and(p1,p2), rest...))
//
// is ill-formed for three or more arguments: deducing this overload's
// return type requires naming all_of recursively, but for the 2-or-more
// case that recursion resolves back to *this same overload*, whose
// return type is still being computed. (Two arguments happen to work
// because the recursion lands on the single-argument base case, which is
// already complete.) The helper structs give each recursion depth a
// distinct, fully-defined type to name, so the fold type is computable
// for any arity. (fixed 2026-05-29)

NS_INTERNAL

    // all_of_fold
    //   helper: computes the type of, and builds, the left-associated
    // predicate_and fold of a non-empty pack.
    template<typename _First,
             typename... _Rest>
    struct all_of_fold;

    // base case: a single predicate folds to itself (decayed).
    template<typename _Only>
    struct all_of_fold<_Only>
    {
        typedef typename std::decay<_Only>::type type;

        template<typename _OnlyFwd>
        static D_CONSTEXPR
        type apply(_OnlyFwd&& _only)
        {
            return std::forward<_OnlyFwd>(_only);
        }
    };

    // recursive case: fold (p1 AND p2) with the rest.
    template<typename _First,
             typename _Second,
             typename... _Rest>
    struct all_of_fold<_First, _Second, _Rest...>
    {
        // the combinator produced by anding the first two
        typedef internal::predicate_and_combinator<
            typename std::decay<_First>::type,
            typename std::decay<_Second>::type> combined_type;

        // recurse on (combined, rest...)
        typedef all_of_fold<combined_type, _Rest...> next_fold;
        typedef typename next_fold::type             type;

        template<typename _FirstFwd,
                 typename _SecondFwd,
                 typename... _RestFwd>
        static D_CONSTEXPR
        type apply(_FirstFwd&&  _first,
                   _SecondFwd&& _second,
                   _RestFwd&&...  _rest)
        {
            return next_fold::apply(
                predicate_and(std::forward<_FirstFwd>(_first),
                              std::forward<_SecondFwd>(_second)),
                std::forward<_RestFwd>(_rest)...);
        }
    };

    // any_of_fold
    //   helper: same shape as all_of_fold but folds with predicate_or.
    template<typename _First,
             typename... _Rest>
    struct any_of_fold;

    template<typename _Only>
    struct any_of_fold<_Only>
    {
        typedef typename std::decay<_Only>::type type;

        template<typename _OnlyFwd>
        static D_CONSTEXPR
        type apply(_OnlyFwd&& _only)
        {
            return std::forward<_OnlyFwd>(_only);
        }
    };

    template<typename _First,
             typename _Second,
             typename... _Rest>
    struct any_of_fold<_First, _Second, _Rest...>
    {
        typedef internal::predicate_or_combinator<
            typename std::decay<_First>::type,
            typename std::decay<_Second>::type> combined_type;

        typedef any_of_fold<combined_type, _Rest...> next_fold;
        typedef typename next_fold::type             type;

        template<typename _FirstFwd,
                 typename _SecondFwd,
                 typename... _RestFwd>
        static D_CONSTEXPR
        type apply(_FirstFwd&&  _first,
                   _SecondFwd&& _second,
                   _RestFwd&&...  _rest)
        {
            return next_fold::apply(
                predicate_or(std::forward<_FirstFwd>(_first),
                             std::forward<_SecondFwd>(_second)),
                std::forward<_RestFwd>(_rest)...);
        }
    };

NS_END  // internal


// all_of (variadic predicate AND)
//   function: creates a predicate that is true when all given predicates
// are true. Evaluates left-to-right with short-circuiting.
// all_of(p1, p2, p3)(x) = p1(x) && p2(x) && p3(x)
template<typename _First,
         typename... _Rest>
D_CONSTEXPR
typename internal::all_of_fold<_First, _Rest...>::type
all_of(_First&& _first, _Rest&&... _rest)
{
    return internal::all_of_fold<_First, _Rest...>::apply(
        std::forward<_First>(_first),
        std::forward<_Rest>(_rest)...);
}


// any_of (variadic predicate OR)
//   function: creates a predicate that is true when any given predicate
// is true. Evaluates left-to-right with short-circuiting.
// any_of(p1, p2, p3)(x) = p1(x) || p2(x) || p3(x)
template<typename _First,
         typename... _Rest>
D_CONSTEXPR
typename internal::any_of_fold<_First, _Rest...>::type
any_of(_First&& _first, _Rest&&... _rest)
{
    return internal::any_of_fold<_First, _Rest...>::apply(
        std::forward<_First>(_first),
        std::forward<_Rest>(_rest)...);
}


// none_of (variadic predicate NOR)
//   function: creates a predicate that is true when none of the given
// predicates are true.
// none_of(p1, p2)(x) = !p1(x) && !p2(x)
template<typename... _Predicates>
D_CONSTEXPR
auto none_of(_Predicates&&... _preds)
    -> decltype(predicate_not(any_of(std::forward<_Predicates>(_preds)...)))
{
    return predicate_not(any_of(std::forward<_Predicates>(_preds)...));
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///        TRAIT DETECTION  (C++11+ only)                                   ///
///////////////////////////////////////////////////////////////////////////////
// Trait detection uses std::true_type / std::false_type / std::decay
// (all C++11 type-traits machinery). Gated to C++11+. C++98 callers
// who need an "is this a combinator" predicate can write one with the
// hand-rolled integral_constant pattern; not provided here.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// -----------------------------------------------------------------------------
//  I.   STRUCTURAL DETECTION  (which combinator template a type came from)
// -----------------------------------------------------------------------------
// These traits answer "is _Type a specialization of <this> combinator
// template?".  Each is a pure structural match on the class template; the
// input is decayed first so that cv-qualified and reference forms answer
// identically to the bare type. Every public trait pairs with a `_v`
// variable-template alias on C++14+.

NS_INTERNAL

    // is_predicate_and_helper
    //   trait: detects if a type is a predicate_and_combinator (primary).
    template<typename _Type>
    struct is_predicate_and_helper : std::false_type
    {};

    // is_predicate_and_helper<predicate_and_combinator<...>>
    //   trait: success specialization for predicate_and_combinator.
    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_and_helper<
        predicate_and_combinator<_Predicate1, _Predicate2>>
        : std::true_type
    {};

    // is_predicate_or_helper
    //   trait: detects if a type is a predicate_or_combinator (primary).
    template<typename _Type>
    struct is_predicate_or_helper : std::false_type
    {};

    // is_predicate_or_helper<predicate_or_combinator<...>>
    //   trait: success specialization for predicate_or_combinator.
    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_or_helper<
        predicate_or_combinator<_Predicate1, _Predicate2>>
        : std::true_type
    {};

    // is_predicate_xor_helper
    //   trait: detects if a type is a predicate_xor_combinator (primary).
    template<typename _Type>
    struct is_predicate_xor_helper : std::false_type
    {};

    // is_predicate_xor_helper<predicate_xor_combinator<...>>
    //   trait: success specialization for predicate_xor_combinator.
    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_xor_helper<
        predicate_xor_combinator<_Predicate1, _Predicate2>>
        : std::true_type
    {};

    // is_predicate_not_helper
    //   trait: detects if a type is a predicate_not_combinator (primary).
    template<typename _Type>
    struct is_predicate_not_helper : std::false_type
    {};

    // is_predicate_not_helper<predicate_not_combinator<...>>
    //   trait: success specialization for predicate_not_combinator.
    template<typename _Predicate>
    struct is_predicate_not_helper<predicate_not_combinator<_Predicate>>
        : std::true_type
    {};

    // is_predicate_nand_helper
    //   trait: detects if a type is a predicate_nand_combinator (primary).
    template<typename _Type>
    struct is_predicate_nand_helper : std::false_type
    {};

    // is_predicate_nand_helper<predicate_nand_combinator<...>>
    //   trait: success specialization for predicate_nand_combinator.
    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_nand_helper<
        predicate_nand_combinator<_Predicate1, _Predicate2>>
        : std::true_type
    {};

    // is_predicate_nor_helper
    //   trait: detects if a type is a predicate_nor_combinator (primary).
    template<typename _Type>
    struct is_predicate_nor_helper : std::false_type
    {};

    // is_predicate_nor_helper<predicate_nor_combinator<...>>
    //   trait: success specialization for predicate_nor_combinator.
    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_nor_helper<
        predicate_nor_combinator<_Predicate1, _Predicate2>>
        : std::true_type
    {};

NS_END  // internal

// is_predicate_and
//   trait: true if _Type (decayed) is a predicate_and_combinator.
template<typename _Type>
struct is_predicate_and
    : internal::is_predicate_and_helper<typename std::decay<_Type>::type>
{};

// is_predicate_or
//   trait: true if _Type (decayed) is a predicate_or_combinator.
template<typename _Type>
struct is_predicate_or
    : internal::is_predicate_or_helper<typename std::decay<_Type>::type>
{};

// is_predicate_xor
//   trait: true if _Type (decayed) is a predicate_xor_combinator.
template<typename _Type>
struct is_predicate_xor
    : internal::is_predicate_xor_helper<typename std::decay<_Type>::type>
{};

// is_predicate_not
//   trait: true if _Type (decayed) is a predicate_not_combinator.
template<typename _Type>
struct is_predicate_not
    : internal::is_predicate_not_helper<typename std::decay<_Type>::type>
{};

// is_predicate_nand
//   trait: true if _Type (decayed) is a predicate_nand_combinator.
template<typename _Type>
struct is_predicate_nand
    : internal::is_predicate_nand_helper<typename std::decay<_Type>::type>
{};

// is_predicate_nor
//   trait: true if _Type (decayed) is a predicate_nor_combinator.
template<typename _Type>
struct is_predicate_nor
    : internal::is_predicate_nor_helper<typename std::decay<_Type>::type>
{};

// is_predicate_combinator
//   trait: true if _Type is any predicate combinator produced by this
// header (and / or / xor / not / nand / nor).
template<typename _Type>
struct is_predicate_combinator
    : std::integral_constant<bool,
          ( is_predicate_and<_Type>::value  ||
            is_predicate_or<_Type>::value   ||
            is_predicate_xor<_Type>::value  ||
            is_predicate_not<_Type>::value  ||
            is_predicate_nand<_Type>::value ||
            is_predicate_nor<_Type>::value )>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // NOTE: no `inline` here. Variable templates are C++14, but `inline`
    // variables are C++17; a templated variable does not need `inline`
    // to be ODR-safe (each specialization is already treated inline).
    // Adding `inline` would make this block ill-formed under -std=c++14.
    // (fixed 2026-05-29)

    // is_predicate_and_v
    //   value: convenience alias for is_predicate_and<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_and_v = is_predicate_and<_Type>::value;

    // is_predicate_or_v
    //   value: convenience alias for is_predicate_or<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_or_v = is_predicate_or<_Type>::value;

    // is_predicate_xor_v
    //   value: convenience alias for is_predicate_xor<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_xor_v = is_predicate_xor<_Type>::value;

    // is_predicate_not_v
    //   value: convenience alias for is_predicate_not<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_not_v = is_predicate_not<_Type>::value;

    // is_predicate_nand_v
    //   value: convenience alias for is_predicate_nand<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_nand_v = is_predicate_nand<_Type>::value;

    // is_predicate_nor_v
    //   value: convenience alias for is_predicate_nor<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_nor_v = is_predicate_nor<_Type>::value;

    // is_predicate_combinator_v
    //   value: convenience alias for is_predicate_combinator<_Type>::value.
    template<typename _Type>
    constexpr bool is_predicate_combinator_v =
        is_predicate_combinator<_Type>::value;
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


// -----------------------------------------------------------------------------
//  II.  BEHAVIORAL DETECTION  (is a type usable as a predicate over Args...)
// -----------------------------------------------------------------------------
// Structural detection above only recognizes the combinators this header
// builds. The behavioral trait below is broader: it asks whether an
// arbitrary callable _Predicate can be invoked with _Args... and yields a
// result that is contextually convertible to bool. This is the structural
// contract every factory in this header silently relies on, surfaced as a
// first-class, SFINAE-friendly trait.

NS_INTERNAL

    // predicate_make_void
    //   trait: header-local map from any type sequence to void, the
    // foundation for the SFINAE detection below. Defined locally so the
    // trait block carries no dependency on an external void_t facility.
    template<typename...>
    struct predicate_make_void
    {
        typedef void type;
    };

    // predicate_void_t
    //   type: header-local alias for predicate_make_void<...>::type.
    template<typename... _Types>
    using predicate_void_t = typename predicate_make_void<_Types...>::type;

    // is_predicate_invocable_helper
    //   trait: detects whether _Predicate(_Args...) is well-formed and
    // produces a bool-convertible result (primary / failure case).
    template<typename _Predicate,
             typename _AlwaysVoid,
             typename... _Args>
    struct is_predicate_invocable_helper : std::false_type
    {};

    // is_predicate_invocable_helper (success case)
    //   trait: specialization for when the call expression is well-formed
    // and its result is contextually convertible to bool.
    template<typename _Predicate,
             typename... _Args>
    struct is_predicate_invocable_helper<
        _Predicate,
        predicate_void_t<decltype(static_cast<bool>(
            std::declval<const _Predicate&>()(std::declval<_Args>()...)))>,
        _Args...>
        : std::true_type
    {};

NS_END  // internal

// is_predicate
//   trait: true if _Predicate is callable with _Args... and its result is
// contextually convertible to bool. Models the structural contract a
// callable must satisfy to be combined by this header's factories.
template<typename _Predicate,
         typename... _Args>
struct is_predicate
    : internal::is_predicate_invocable_helper<_Predicate, void, _Args...>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // is_predicate_v
    //   value: convenience alias for is_predicate<_Predicate, _Args...>::value.
    template<typename _Predicate,
             typename... _Args>
    constexpr bool is_predicate_v = is_predicate<_Predicate, _Args...>::value;
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


// -----------------------------------------------------------------------------
//  III. C++20 CONCEPTS
// -----------------------------------------------------------------------------
// Concept wrappers over the traits above, for use in requires-clauses and
// abbreviated function templates. Available only on C++20 and later.

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // predicate_combinator
    //   concept: satisfied by any combinator this header builds.
    template<typename _Type>
    concept predicate_combinator = is_predicate_combinator<_Type>::value;

    // predicate
    //   concept: satisfied by a callable invocable with _Args... whose
    // result is contextually convertible to bool.
    template<typename _Predicate,
             typename... _Args>
    concept predicate = is_predicate<_Predicate, _Args...>::value;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER (trait detection block)


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PREDICATE_