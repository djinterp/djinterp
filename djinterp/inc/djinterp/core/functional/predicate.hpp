/******************************************************************************
* djinterp [functional]                                          predicate.hpp
*
* Template predicate combinators for the functional module (C++).
*   Provides type-safe, SFINAE-constrained predicate combinators that compose
* predicates with AND, OR, XOR, and NOT operations. Unlike the C version
* which uses void* and function pointers, these combinators are fully typed
* and work with any callable (lambdas, function objects, function pointers,
* std::function, etc.).
*
*   Each combinator stores its predicates by value (decayed), supports
* perfect forwarding at call sites, and propagates noexcept.
*
* USAGE:
*   auto combo = predicate_and(is_positive, is_even);
*   bool result = combo(42);   // true if both return true
*
*   auto chain = predicate_or(
*       predicate_not(is_negative),
*       predicate_and(is_small, is_prime));
*
* 
* path:      /inc/functional/predicate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PREDICATE_
#define DJINTERP_FUNCTIONAL_PREDICATE_ 1

#include <cstddef>
#include <type_traits>
#include <utility>
#include "../djinterp.hpp"
#include "./functional_traits.hpp"


NS_DJINTERP
NS_FUNCTIONAL


// combinator classes
NS_INTERNAL
    // predicate_and_combinator
    //   helper: evaluates two predicates with logical AND (short-circuiting).
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_and_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
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

        // accessors for introspection
        D_CONSTEXPR const _Predicate1& first()  const { return m_predicate1; }
        D_CONSTEXPR const _Predicate2& second() const { return m_predicate2; }
    };

    // predicate_or_combinator
    //   helper: evaluates two predicates with logical OR (short-circuiting).
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_or_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
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

        D_CONSTEXPR const _Predicate1& first()  const { return m_predicate1; }
        D_CONSTEXPR const _Predicate2& second() const { return m_predicate2; }
    };

    // predicate_xor_combinator
    //   helper: evaluates two predicates with logical XOR.
    // Both predicates are always evaluated.
    template<typename _Predicate1,
             typename _Predicate2>
    class predicate_xor_combinator
    {
    private:
        _Predicate1 m_predicate1;
        _Predicate2 m_predicate2;

    public:
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
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_nand_combinator(_Predicate1Fwd&& _predicate1, _Predicate2Fwd&& _predicate2)
            : m_predicate1(std::forward<_Predicate1Fwd>(_predicate1))
            , m_predicate2(std::forward<_Predicate2Fwd>(_predicate2))
        {}

        template<typename... _Args>
        D_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return !(m_predicate1(std::forward<_Args>(_args)...) &&
                     m_predicate2(std::forward<_Args>(_args)...));
        }
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
        template<typename _Predicate1Fwd,
                 typename _Predicate2Fwd>
        D_CONSTEXPR
        predicate_nor_combinator(_Predicate1Fwd&& _predicate1, _Predicate2Fwd&& _predicate2)
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
    };

NS_END  // internal

// factory functions

// predicate_and
//   function: creates an AND combinator from two predicates.
// predicate_and(p1, p2)(x) = p1(x) && p2(x)
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR
internal::predicate_and_combinator<typename std::decay<_Predicate1>::type,
                                   typename std::decay<_Predicate2>::type>
predicate_and
(
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

// predicate_or
//   function: creates an OR combinator from two predicates.
// predicate_or(p1, p2)(x) = p1(x) || p2(x)
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

// predicate_xor
//   function: creates an XOR combinator from two predicates.
// predicate_xor(p1, p2)(x) = p1(x) != p2(x)
template<typename _Predicate1,
         typename _Predicate2>
D_CONSTEXPR
internal::predicate_xor_combinator<typename std::decay<_Predicate1>::type,
                                   typename std::decay<_Predicate2>::type>
predicate_xor
(
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

// predicate_not
//   function: creates a NOT combinator that negates a predicate.
// predicate_not(p)(x) = !p(x)
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

// predicate_nand
//   function: creates a NAND combinator from two predicates.
// predicate_nand(p1, p2)(x) = !(p1(x) && p2(x))
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

// predicate_nor
//   function: creates a NOR combinator from two predicates.
// predicate_nor(p1, p2)(x) = !(p1(x) || p2(x))
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


// variadic helpers

// all_of (variadic predicate AND)
//   function: creates a predicate that is true when all given predicates
// are true. Evaluates left-to-right with short-circuiting.
// all_of(p1, p2, p3)(x) = p1(x) && p2(x) && p3(x)
template<typename _Predicate>
D_CONSTEXPR
auto all_of(_Predicate&& _predicate)
    -> typename std::decay<_Predicate>::type
{
    return std::forward<_Predicate>(_predicate);
}

template<typename _Predicate1,
         typename _Predicate2,
         typename... _Predicates>
D_CONSTEXPR
auto all_of
(
    _Predicate1&&    _predicate1,
    _Predicate2&&    _predicate2,
    _Predicates&&... _predicates
)
    -> decltype(all_of(
           predicate_and(std::forward<_Predicate1>(_predicate1),
                         std::forward<_Predicate2>(_predicate2)),
           std::forward<_Predicates>(_predicates)...))
{
    return all_of(
        predicate_and(std::forward<_Predicate1>(_predicate1),
                      std::forward<_Predicate2>(_predicate2)),
        std::forward<_Predicates>(_predicates)...);
}

// any_of (variadic predicate OR)
//   function: creates a predicate that is true when any given predicate
// is true. Evaluates left-to-right with short-circuiting.
// any_of(p1, p2, p3)(x) = p1(x) || p2(x) || p3(x)
template<typename _Predicate>
D_CONSTEXPR
auto any_of(_Predicate&& _predicate)
    -> typename std::decay<_Predicate>::type
{
    return std::forward<_Predicate>(_predicate);
}

template<typename _Predicate1,
         typename _Predicate2,
         typename... _Predicates>
D_CONSTEXPR
auto any_of(_Predicate1&& _predicate1, _Predicate2&& _predicate2, _Predicates&&... _rest)
    -> decltype(any_of(
           predicate_or(std::forward<_Predicate1>(_predicate1),
                        std::forward<_Predicate2>(_predicate2)),
           std::forward<_Predicates>(_rest)...))
{
    return any_of(
        predicate_or(std::forward<_Predicate1>(_predicate1),
                     std::forward<_Predicate2>(_predicate2)),
        std::forward<_Predicates>(_rest)...);
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


// predicate trait detectors

NS_INTERNAL
    // is_predicate_and
    //   helper: detects if a type is a predicate_and_combinator.
    template<typename _Type>
    struct is_predicate_and_helper : std::false_type
    {};

    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_and_helper<predicate_and_combinator<_Predicate1, _Predicate2>>
        : std::true_type 
    {};

    // is_predicate_or
    //   helper: detects if a type is a predicate_or_combinator.
    template<typename _Type>
    struct is_predicate_or_helper : std::false_type
    {};

    template<typename _Predicate1,
             typename _Predicate2>
    struct is_predicate_or_helper<predicate_or_combinator<_Predicate1, _Predicate2>>
        : std::true_type 
    {};

    // is_predicate_not
    //   helper: detects if a type is a predicate_not_combinator.
    template<typename _Type>
    struct is_predicate_not_helper : std::false_type
    {};

    template<typename _P>
    struct is_predicate_not_helper<predicate_not_combinator<_P>>
        : std::true_type
    {};
};

NS_END  // internal

// is_predicate_combinator
//   type trait: checks if _Type is any predicate combinator.
template<typename _Type>
struct is_predicate_combinator
    : std::integral_constant<bool,
          internal::is_predicate_and_helper<
              typename std::decay<_Type>::type>::value ||
          internal::is_predicate_or_helper<
              typename std::decay<_Type>::type>::value ||
          internal::is_predicate_not_helper<
              typename std::decay<_Type>::type>::value>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename _Type>
    inline constexpr bool is_predicate_combinator_v =
        is_predicate_combinator<_Type>::value;
#endif


NS_END  // functional
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PREDICATE_
