/*******************************************************************************
* djinterp [core]                                                  predicate.hpp
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
*
* path:      /inc/djinterp/core/functional/predicate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PREDICATE_HPP
#define DJINTERP_FUNCTIONAL_PREDICATE_HPP 1

// std
#include <cstddef>
#include <utility>
// djinterp
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// std
#  include <type_traits>              // std::true_type, std::decay
// djinterp
#  include "./functional_common.hpp"  // is_predicate, Predicate
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
    template<typename Predicate1,
             typename Predicate2>
    class predicate_and_combinator
    {
    private:
        Predicate1 m_predicate1;
        Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename Predicate1Fwd,
                 typename Predicate2Fwd>
        D_CONSTEXPR
        predicate_and_combinator
        (
            Predicate1Fwd&& _predicate1,
            Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<Predicate1Fwd>(_predicate1)),
              m_predicate2(std::forward<Predicate2Fwd>(_predicate2))
        {}

        template<typename... Args>
        D_CONSTEXPR
        bool operator()(Args&&... _args) const
        {
            return m_predicate1(std::forward<Args>(_args)...) &&
                   m_predicate2(std::forward<Args>(_args)...);
        }
#else
        // C++98 fallback: const-ref ctor + fixed-arity overloads.
        predicate_and_combinator(
            const Predicate1& _predicate1,
            const Predicate2& _predicate2
        )
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename Arg>
        bool operator()(const Arg& _arg) const
        {
            return m_predicate1(_arg) && m_predicate2(_arg);
        }

        template<typename A,
                 typename B>
        bool operator()(const A& _a, const B& _b) const
        {
            return m_predicate1(_a, _b) && m_predicate2(_a, _b);
        }
#endif

        // accessors for introspection
        D_CONSTEXPR const Predicate1&
        first()  const
        {
            return m_predicate1;
        }

        D_CONSTEXPR const Predicate2&
        second() const
        {
            return m_predicate2;
        }
    };

    // predicate_or_combinator
    //   helper: evaluates two predicates with logical OR
    // (short-circuiting).
    template<typename Predicate1,
             typename Predicate2>
    class predicate_or_combinator
    {
    private:
        Predicate1 m_predicate1;
        Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename Predicate1Fwd,
                 typename Predicate2Fwd>
        D_CONSTEXPR
        predicate_or_combinator
        (
            Predicate1Fwd&& _predicate1,
            Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<Predicate1Fwd>(_predicate1)),
              m_predicate2(std::forward<Predicate2Fwd>(_predicate2))
        {}

        template<typename... Args>
        D_CONSTEXPR
        bool operator()(Args&&... _args) const
        {
            return m_predicate1(std::forward<Args>(_args)...) ||
                   m_predicate2(std::forward<Args>(_args)...);
        }
#else
        predicate_or_combinator(
            const Predicate1& _predicate1,
            const Predicate2& _predicate2
        )
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename Arg>
        bool operator()(const Arg& _arg) const
        {
            return m_predicate1(_arg) || m_predicate2(_arg);
        }

        template<typename A,
                 typename B>
        bool operator()(const A& _a, const B& _b) const
        {
            return m_predicate1(_a, _b) || m_predicate2(_a, _b);
        }
#endif

        D_CONSTEXPR const Predicate1& first()  const { return m_predicate1; }
        D_CONSTEXPR const Predicate2& second() const { return m_predicate2; }
    };

    // predicate_xor_combinator
    //   helper: evaluates two predicates with logical XOR.
    // Both predicates are always evaluated (XOR has no
    // short-circuit).
    template<typename Predicate1,
             typename Predicate2>
    class predicate_xor_combinator
    {
    private:
        Predicate1 m_predicate1;
        Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename Predicate1Fwd,
                 typename Predicate2Fwd>
        D_CONSTEXPR
        predicate_xor_combinator
        (
            Predicate1Fwd&& _predicate1,
            Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<Predicate1Fwd>(_predicate1)),
              m_predicate2(std::forward<Predicate2Fwd>(_predicate2))
        {}

        template<typename... Args>
        D_CONSTEXPR_CPP14
        bool operator()(Args&&... _args) const
        {
            bool a = m_predicate1(std::forward<Args>(_args)...);
            bool b = m_predicate2(std::forward<Args>(_args)...);

            return a != b;
        }
#else
        predicate_xor_combinator(
            const Predicate1& _predicate1,
            const Predicate2& _predicate2
        )
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename Arg>
        bool operator()(const Arg& _arg) const
        {
            const bool a = m_predicate1(_arg);
            const bool b = m_predicate2(_arg);
            return a != b;
        }

        template<typename A,
                 typename B>
        bool operator()(const A& _a, const B& _b) const
        {
            const bool a = m_predicate1(_a, _b);
            const bool b = m_predicate2(_a, _b);
            return a != b;
        }
#endif

        D_CONSTEXPR const Predicate1& first()  const { return m_predicate1; }
        D_CONSTEXPR const Predicate2& second() const { return m_predicate2; }
    };

    // predicate_not_combinator
    //   helper: negates a single predicate.
    template<typename Predicate>
    class predicate_not_combinator
    {
    private:
        Predicate m_pred;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename PredicateFwd>
        explicit D_CONSTEXPR
        predicate_not_combinator(PredicateFwd&& _predicate)
            : m_pred(std::forward<PredicateFwd>(_predicate))
        {}

        template<typename... Args>
        D_CONSTEXPR
        bool operator()(Args&&... _args) const
        {
            return !m_pred(std::forward<Args>(_args)...);
        }
#else
        explicit
        predicate_not_combinator(const Predicate& _predicate)
            : m_pred(_predicate)
        {}

        template<typename Arg>
        bool operator()(const Arg& _arg) const
        {
            return !m_pred(_arg);
        }

        template<typename A,
                 typename B>
        bool operator()(const A& _a, const B& _b) const
        {
            return !m_pred(_a, _b);
        }
#endif

        D_CONSTEXPR const Predicate& inner() const { return m_pred; }
    };

    // predicate_nand_combinator
    //   helper: evaluates two predicates with logical NAND.
    template<typename Predicate1,
             typename Predicate2>
    class predicate_nand_combinator
    {
    private:
        Predicate1 m_predicate1;
        Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename Predicate1Fwd,
                 typename Predicate2Fwd>
        D_CONSTEXPR
        predicate_nand_combinator(
            Predicate1Fwd&& _predicate1,
            Predicate2Fwd&& _predicate2
        )
            : m_predicate1(std::forward<Predicate1Fwd>(_predicate1))
            , m_predicate2(std::forward<Predicate2Fwd>(_predicate2))
        {}

        template<typename... Args>
        D_CONSTEXPR bool
        operator()(
            Args&&... _args
        ) const
        {
            return !(m_predicate1(std::forward<Args>(_args)...) &&
                     m_predicate2(std::forward<Args>(_args)...));
        }
#else
        predicate_nand_combinator(const Predicate1& _predicate1,
                                  const Predicate2& _predicate2)
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename Arg>
        bool operator()(const Arg& _arg) const
        {
            return !(m_predicate1(_arg) && m_predicate2(_arg));
        }

        template<typename A,
                 typename B>
        bool operator()(const A& _a, const B& _b) const
        {
            return !(m_predicate1(_a, _b) && m_predicate2(_a, _b));
        }
#endif
    };

    // predicate_nor_combinator
    //   helper: evaluates two predicates with logical NOR.
    template<typename Predicate1,
             typename Predicate2>
    class predicate_nor_combinator
    {
    private:
        Predicate1 m_predicate1;
        Predicate2 m_predicate2;

    public:
        typedef bool result_type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        template<typename Predicate1Fwd,
                 typename Predicate2Fwd>
        D_CONSTEXPR
        predicate_nor_combinator(Predicate1Fwd&& _predicate1,
                                 Predicate2Fwd&& _predicate2)
            : m_predicate1(std::forward<Predicate1Fwd>(_predicate1))
            , m_predicate2(std::forward<Predicate2Fwd>(_predicate2))
        {}

        template<typename... Args>
        D_CONSTEXPR
        bool operator()(Args&&... _args) const
        {
            return !(m_predicate1(std::forward<Args>(_args)...) ||
                     m_predicate2(std::forward<Args>(_args)...));
        }
#else
        predicate_nor_combinator(const Predicate1& _predicate1,
                                 const Predicate2& _predicate2)
            : m_predicate1(_predicate1),
              m_predicate2(_predicate2)
        {}

        template<typename Arg>
        bool operator()(const Arg& _arg) const
        {
            return !(m_predicate1(_arg) || m_predicate2(_arg));
        }

        template<typename A,
                 typename B>
        bool operator()(
            const A& _a,
            const B& _b
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
template<typename Predicate1,
         typename Predicate2>
D_CONSTEXPR
internal::predicate_and_combinator<typename std::decay<Predicate1>::type,
                                   typename std::decay<Predicate2>::type>
predicate_and(
    Predicate1&& _predicate1,
    Predicate2&& _predicate2
)
{
    return internal::predicate_and_combinator<
        typename std::decay<Predicate1>::type,
        typename std::decay<Predicate2>::type>(
            std::forward<Predicate1>(_predicate1),
            std::forward<Predicate2>(_predicate2));
}
#else
template<typename Predicate1,
         typename Predicate2>
internal::predicate_and_combinator<Predicate1, Predicate2>
predicate_and
(
    const Predicate1& _predicate1,
    const Predicate2& _predicate2
)
{
    return internal::predicate_and_combinator<Predicate1, Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_or
//   function: creates an OR combinator from two predicates.
// predicate_or(p1, p2)(x) = p1(x) || p2(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Predicate1,
         typename Predicate2>
D_CONSTEXPR
internal::predicate_or_combinator<typename std::decay<Predicate1>::type,
                                  typename std::decay<Predicate2>::type>
predicate_or
(
    Predicate1&& _predicate1,
    Predicate2&& _predicate2
)
{
    return internal::predicate_or_combinator<
        typename std::decay<Predicate1>::type,
        typename std::decay<Predicate2>::type>(
            std::forward<Predicate1>(_predicate1),
            std::forward<Predicate2>(_predicate2));
}
#else
template<typename Predicate1,
         typename Predicate2>
internal::predicate_or_combinator<Predicate1, Predicate2>
predicate_or
(
    const Predicate1& _predicate1,
    const Predicate2& _predicate2
)
{
    return internal::predicate_or_combinator<Predicate1, Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_xor
//   function: creates an XOR combinator from two predicates.
// predicate_xor(p1, p2)(x) = p1(x) != p2(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Predicate1,
         typename Predicate2>
D_CONSTEXPR internal::predicate_xor_combinator<
    typename std::decay<Predicate1>::type,
    typename std::decay<Predicate2>::type>
predicate_xor(
    Predicate1&& _predicate1,
    Predicate2&& _predicate2
)
{
    return internal::predicate_xor_combinator<
        typename std::decay<Predicate1>::type,
        typename std::decay<Predicate2>::type>(
            std::forward<Predicate1>(_predicate1),
            std::forward<Predicate2>(_predicate2));
}
#else
template<typename Predicate1,
         typename Predicate2>
internal::predicate_xor_combinator<Predicate1, Predicate2>
predicate_xor
(
    const Predicate1& _predicate1,
    const Predicate2& _predicate2
)
{
    return internal::predicate_xor_combinator<Predicate1, Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_not
//   function: creates a NOT combinator that negates a predicate.
// predicate_not(p)(x) = !p(x)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Predicate>
D_CONSTEXPR
internal::predicate_not_combinator<typename std::decay<Predicate>::type>
predicate_not
(
    Predicate&& _predicate
)
{
    return internal::predicate_not_combinator<
        typename std::decay<Predicate>::type>(
            std::forward<Predicate>(_predicate));
}
#else
template<typename Predicate>
internal::predicate_not_combinator<Predicate>
predicate_not
(
    const Predicate& _predicate
)
{
    return internal::predicate_not_combinator<Predicate>(_predicate);
}
#endif

// predicate_nand
//   function: creates a NAND combinator from two predicates.
// predicate_nand(p1, p2)(x) = !(p1(x) && p2(x))
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Predicate1,
         typename Predicate2>
D_CONSTEXPR
internal::predicate_nand_combinator<typename std::decay<Predicate1>::type,
                                    typename std::decay<Predicate2>::type>
predicate_nand
(
    Predicate1&& _predicate1,
    Predicate2&& _predicate2
)
{
    return internal::predicate_nand_combinator<
        typename std::decay<Predicate1>::type,
        typename std::decay<Predicate2>::type>(
            std::forward<Predicate1>(_predicate1),
            std::forward<Predicate2>(_predicate2));
}
#else
template<typename Predicate1,
         typename Predicate2>
internal::predicate_nand_combinator<Predicate1, Predicate2>
predicate_nand
(
    const Predicate1& _predicate1,
    const Predicate2& _predicate2
)
{
    return internal::predicate_nand_combinator<Predicate1, Predicate2>(
        _predicate1, _predicate2);
}
#endif

// predicate_nor
//   function: creates a NOR combinator from two predicates.
// predicate_nor(p1, p2)(x) = !(p1(x) || p2(x))
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Predicate1,
         typename Predicate2>
D_CONSTEXPR
internal::predicate_nor_combinator<typename std::decay<Predicate1>::type,
                                   typename std::decay<Predicate2>::type>
predicate_nor(Predicate1&& _predicate1, Predicate2&& _predicate2)
{
    return internal::predicate_nor_combinator<
        typename std::decay<Predicate1>::type,
        typename std::decay<Predicate2>::type>(
            std::forward<Predicate1>(_predicate1),
            std::forward<Predicate2>(_predicate2));
}
#else
template<typename Predicate1,
         typename Predicate2>
internal::predicate_nor_combinator<Predicate1, Predicate2>
predicate_nor(const Predicate1& _predicate1, const Predicate2& _predicate2)
{
    return internal::predicate_nor_combinator<Predicate1, Predicate2>(
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
    template<typename First,
             typename... Rest>
    struct all_of_fold;

    // base case: a single predicate folds to itself (decayed).
    template<typename Only>
    struct all_of_fold<Only>
    {
        typedef typename std::decay<Only>::type type;

        template<typename OnlyFwd>
        static D_CONSTEXPR
        type apply(OnlyFwd&& _only)
        {
            return std::forward<OnlyFwd>(_only);
        }
    };

    // recursive case: fold (p1 AND p2) with the rest.
    template<typename First,
             typename Second,
             typename... Rest>
    struct all_of_fold<First, Second, Rest...>
    {
        // the combinator produced by anding the first two
        typedef internal::predicate_and_combinator<
            typename std::decay<First>::type,
            typename std::decay<Second>::type> combined_type;

        // recurse on (combined, rest...)
        typedef all_of_fold<combined_type, Rest...> next_fold;
        typedef typename next_fold::type             type;

        template<typename FirstFwd,
                 typename SecondFwd,
                 typename... RestFwd>
        static D_CONSTEXPR
        type apply(FirstFwd&&  _first,
                   SecondFwd&& _second,
                   RestFwd&&...  _rest)
        {
            return next_fold::apply(
                predicate_and(std::forward<FirstFwd>(_first),
                              std::forward<SecondFwd>(_second)),
                std::forward<RestFwd>(_rest)...);
        }
    };

    // any_of_fold
    //   helper: same shape as all_of_fold but folds with predicate_or.
    template<typename First,
             typename... Rest>
    struct any_of_fold;

    template<typename Only>
    struct any_of_fold<Only>
    {
        typedef typename std::decay<Only>::type type;

        template<typename OnlyFwd>
        static D_CONSTEXPR
        type apply(OnlyFwd&& _only)
        {
            return std::forward<OnlyFwd>(_only);
        }
    };

    template<typename First,
             typename Second,
             typename... Rest>
    struct any_of_fold<First, Second, Rest...>
    {
        typedef internal::predicate_or_combinator<
            typename std::decay<First>::type,
            typename std::decay<Second>::type> combined_type;

        typedef any_of_fold<combined_type, Rest...> next_fold;
        typedef typename next_fold::type             type;

        template<typename FirstFwd,
                 typename SecondFwd,
                 typename... RestFwd>
        static D_CONSTEXPR
        type apply(FirstFwd&&  _first,
                   SecondFwd&& _second,
                   RestFwd&&...  _rest)
        {
            return next_fold::apply(
                predicate_or(std::forward<FirstFwd>(_first),
                             std::forward<SecondFwd>(_second)),
                std::forward<RestFwd>(_rest)...);
        }
    };

NS_END  // internal


// all_of (variadic predicate AND)
//   function: creates a predicate that is true when all given predicates
// are true. Evaluates left-to-right with short-circuiting.
// all_of(p1, p2, p3)(x) = p1(x) && p2(x) && p3(x)
template<typename First,
         typename... Rest>
D_CONSTEXPR typename internal::all_of_fold<First, Rest...>::type
all_of(First&& _first, Rest&&... _rest)
{
    return internal::all_of_fold<First, Rest...>::apply(
        std::forward<First>(_first),
        std::forward<Rest>(_rest)...);
}


// any_of (variadic predicate OR)
//   function: creates a predicate that is true when any given predicate
// is true. Evaluates left-to-right with short-circuiting.
// any_of(p1, p2, p3)(x) = p1(x) || p2(x) || p3(x)
template<typename First,
         typename... Rest>
D_CONSTEXPR
typename internal::any_of_fold<First, Rest...>::type
any_of(First&& _first, Rest&&... _rest)
{
    return internal::any_of_fold<First, Rest...>::apply(
        std::forward<First>(_first),
        std::forward<Rest>(_rest)...);
}


// none_of (variadic predicate NOR)
//   function: creates a predicate that is true when none of the given
// predicates are true.
// none_of(p1, p2)(x) = !p1(x) && !p2(x)
template<typename... Predicates>
D_CONSTEXPR
auto none_of(Predicates&&... _preds)
    -> decltype(predicate_not(any_of(std::forward<Predicates>(_preds)...)))
{
    return predicate_not(any_of(std::forward<Predicates>(_preds)...));
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
// These traits answer "is Type a specialization of <this> combinator
// template?".  Each is a pure structural match on the class template; the
// input is decayed first so that cv-qualified and reference forms answer
// identically to the bare type. Every public trait pairs with a `_v`
// variable-template alias on C++14+.

NS_INTERNAL

    // is_predicate_and_helper
    //   trait: detects if a type is a predicate_and_combinator (primary).
    template<typename Type>
    struct is_predicate_and_helper : std::false_type
    {};

    // is_predicate_and_helper<predicate_and_combinator<...>>
    //   trait: success specialization for predicate_and_combinator.
    template<typename Predicate1,
             typename Predicate2>
    struct is_predicate_and_helper<
        predicate_and_combinator<Predicate1, Predicate2>>
        : std::true_type
    {};

    // is_predicate_or_helper
    //   trait: detects if a type is a predicate_or_combinator (primary).
    template<typename Type>
    struct is_predicate_or_helper : std::false_type
    {};

    // is_predicate_or_helper<predicate_or_combinator<...>>
    //   trait: success specialization for predicate_or_combinator.
    template<typename Predicate1,
             typename Predicate2>
    struct is_predicate_or_helper<
        predicate_or_combinator<Predicate1, Predicate2>>
        : std::true_type
    {};

    // is_predicate_xor_helper
    //   trait: detects if a type is a predicate_xor_combinator (primary).
    template<typename Type>
    struct is_predicate_xor_helper : std::false_type
    {};

    // is_predicate_xor_helper<predicate_xor_combinator<...>>
    //   trait: success specialization for predicate_xor_combinator.
    template<typename Predicate1,
             typename Predicate2>
    struct is_predicate_xor_helper<
        predicate_xor_combinator<Predicate1, Predicate2>>
        : std::true_type
    {};

    // is_predicate_not_helper
    //   trait: detects if a type is a predicate_not_combinator (primary).
    template<typename Type>
    struct is_predicate_not_helper : std::false_type
    {};

    // is_predicate_not_helper<predicate_not_combinator<...>>
    //   trait: success specialization for predicate_not_combinator.
    template<typename Predicate>
    struct is_predicate_not_helper<predicate_not_combinator<Predicate>>
        : std::true_type
    {};

    // is_predicate_nand_helper
    //   trait: detects if a type is a predicate_nand_combinator (primary).
    template<typename Type>
    struct is_predicate_nand_helper : std::false_type
    {};

    // is_predicate_nand_helper<predicate_nand_combinator<...>>
    //   trait: success specialization for predicate_nand_combinator.
    template<typename Predicate1,
             typename Predicate2>
    struct is_predicate_nand_helper<
        predicate_nand_combinator<Predicate1, Predicate2>>
        : std::true_type
    {};

    // is_predicate_nor_helper
    //   trait: detects if a type is a predicate_nor_combinator (primary).
    template<typename Type>
    struct is_predicate_nor_helper : std::false_type
    {};

    // is_predicate_nor_helper<predicate_nor_combinator<...>>
    //   trait: success specialization for predicate_nor_combinator.
    template<typename Predicate1,
             typename Predicate2>
    struct is_predicate_nor_helper<
        predicate_nor_combinator<Predicate1, Predicate2>>
        : std::true_type
    {};

NS_END  // internal

// is_predicate_and
//   trait: true if Type (decayed) is a predicate_and_combinator.
template<typename Type>
struct is_predicate_and
    : internal::is_predicate_and_helper<typename std::decay<Type>::type>
{};

// is_predicate_or
//   trait: true if Type (decayed) is a predicate_or_combinator.
template<typename Type>
struct is_predicate_or
    : internal::is_predicate_or_helper<typename std::decay<Type>::type>
{};

// is_predicate_xor
//   trait: true if Type (decayed) is a predicate_xor_combinator.
template<typename Type>
struct is_predicate_xor
    : internal::is_predicate_xor_helper<typename std::decay<Type>::type>
{};

// is_predicate_not
//   trait: true if Type (decayed) is a predicate_not_combinator.
template<typename Type>
struct is_predicate_not
    : internal::is_predicate_not_helper<typename std::decay<Type>::type>
{};

// is_predicate_nand
//   trait: true if Type (decayed) is a predicate_nand_combinator.
template<typename Type>
struct is_predicate_nand
    : internal::is_predicate_nand_helper<typename std::decay<Type>::type>
{};

// is_predicate_nor
//   trait: true if Type (decayed) is a predicate_nor_combinator.
template<typename Type>
struct is_predicate_nor
    : internal::is_predicate_nor_helper<typename std::decay<Type>::type>
{};

// is_predicate_combinator
//   trait: true if Type is any predicate combinator produced by this
// header (and / or / xor / not / nand / nor).
template<typename Type>
struct is_predicate_combinator
    : std::integral_constant<bool,
          ( is_predicate_and<Type>::value  ||
            is_predicate_or<Type>::value   ||
            is_predicate_xor<Type>::value  ||
            is_predicate_not<Type>::value  ||
            is_predicate_nand<Type>::value ||
            is_predicate_nor<Type>::value )>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // NOTE: no `inline` here. Variable templates are C++14, but `inline`
    // variables are C++17; a templated variable does not need `inline`
    // to be ODR-safe (each specialization is already treated inline).
    // Adding `inline` would make this block ill-formed under -std=c++14.
    // (fixed 2026-05-29)

    // is_predicate_and_v
    //   value: convenience alias for is_predicate_and<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_and_v = is_predicate_and<Type>::value;

    // is_predicate_or_v
    //   value: convenience alias for is_predicate_or<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_or_v = is_predicate_or<Type>::value;

    // is_predicate_xor_v
    //   value: convenience alias for is_predicate_xor<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_xor_v = is_predicate_xor<Type>::value;

    // is_predicate_not_v
    //   value: convenience alias for is_predicate_not<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_not_v = is_predicate_not<Type>::value;

    // is_predicate_nand_v
    //   value: convenience alias for is_predicate_nand<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_nand_v = is_predicate_nand<Type>::value;

    // is_predicate_nor_v
    //   value: convenience alias for is_predicate_nor<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_nor_v = is_predicate_nor<Type>::value;

    // is_predicate_combinator_v
    //   value: convenience alias for is_predicate_combinator<Type>::value.
    template<typename Type>
    constexpr bool is_predicate_combinator_v =
        is_predicate_combinator<Type>::value;
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


// -----------------------------------------------------------------------------
//  II.  BEHAVIORAL DETECTION  (is a type usable as a predicate over Args...)
// -----------------------------------------------------------------------------
// Structural detection above only recognizes the combinators this header
// builds. The behavioral question -- can an arbitrary callable be invoked with
// Args... and answer true or false -- is the functional module's is_predicate,
// with its is_predicate_v shorthand and its Predicate concept, all defined
// once in functional_common.hpp and included above. Every factory here relies
// on that contract; this header used to define its own is_predicate, which no
// translation unit could include beside functional_common.hpp's.


// -----------------------------------------------------------------------------
//  III. C++20 CONCEPTS
// -----------------------------------------------------------------------------
// Concept wrappers over the traits above, for use in requires-clauses and
// abbreviated function templates. Available only on C++20 and later.

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // PredicateCombinator
    //   concept: satisfied by any combinator this header builds. (Predicate,
    // satisfied by any predicate over Args..., is functional_common.hpp's.)
    template<typename Type>
    concept PredicateCombinator = is_predicate_combinator<Type>::value;

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER (trait detection block)


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PREDICATE_HPP
