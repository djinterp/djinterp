/******************************************************************************
* djinterp [functional]                              predicate_combinators.hpp
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
* path:      \inc\functional\predicate_combinators.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.19
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PREDICATE_COMBINATORS_
#define DJINTERP_FUNCTIONAL_PREDICATE_COMBINATORS_ 1

#include <cstddef>
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
///             I.    COMBINATOR CLASSES                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // predicate_and_combinator
    //   helper: evaluates two predicates with logical AND (short-circuiting).
    template<typename _Pred1,
             typename _Pred2>
    class predicate_and_combinator
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _P1Fwd,
                 typename _P2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        predicate_and_combinator(_P1Fwd&& _p1, _P2Fwd&& _p2)
            : m_pred1(std::forward<_P1Fwd>(_p1))
            , m_pred2(std::forward<_P2Fwd>(_p2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return m_pred1(std::forward<_Args>(_args)...) &&
                   m_pred2(std::forward<_Args>(_args)...);
        }

        // accessors for introspection
        D_FUNCTIONAL_CONSTEXPR const _Pred1& first()  const { return m_pred1; }
        D_FUNCTIONAL_CONSTEXPR const _Pred2& second() const { return m_pred2; }
    };

    // predicate_or_combinator
    //   helper: evaluates two predicates with logical OR (short-circuiting).
    template<typename _Pred1,
             typename _Pred2>
    class predicate_or_combinator
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _P1Fwd,
                 typename _P2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        predicate_or_combinator(_P1Fwd&& _p1, _P2Fwd&& _p2)
            : m_pred1(std::forward<_P1Fwd>(_p1))
            , m_pred2(std::forward<_P2Fwd>(_p2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return m_pred1(std::forward<_Args>(_args)...) ||
                   m_pred2(std::forward<_Args>(_args)...);
        }

        D_FUNCTIONAL_CONSTEXPR const _Pred1& first()  const { return m_pred1; }
        D_FUNCTIONAL_CONSTEXPR const _Pred2& second() const { return m_pred2; }
    };

    // predicate_xor_combinator
    //   helper: evaluates two predicates with logical XOR.
    // Both predicates are always evaluated.
    template<typename _Pred1,
             typename _Pred2>
    class predicate_xor_combinator
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _P1Fwd,
                 typename _P2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        predicate_xor_combinator(_P1Fwd&& _p1, _P2Fwd&& _p2)
            : m_pred1(std::forward<_P1Fwd>(_p1))
            , m_pred2(std::forward<_P2Fwd>(_p2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            bool a = m_pred1(std::forward<_Args>(_args)...);
            bool b = m_pred2(std::forward<_Args>(_args)...);

            return a != b;
        }

        D_FUNCTIONAL_CONSTEXPR const _Pred1& first()  const { return m_pred1; }
        D_FUNCTIONAL_CONSTEXPR const _Pred2& second() const { return m_pred2; }
    };

    // predicate_not_combinator
    //   helper: negates a single predicate.
    template<typename _Pred>
    class predicate_not_combinator
    {
    private:
        _Pred m_pred;

    public:
        template<typename _PFwd>
        explicit D_FUNCTIONAL_CONSTEXPR
        predicate_not_combinator(_PFwd&& _p)
            : m_pred(std::forward<_PFwd>(_p))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return !m_pred(std::forward<_Args>(_args)...);
        }

        D_FUNCTIONAL_CONSTEXPR const _Pred& inner() const { return m_pred; }
    };

    // predicate_nand_combinator
    //   helper: evaluates two predicates with logical NAND.
    template<typename _Pred1,
             typename _Pred2>
    class predicate_nand_combinator
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _P1Fwd,
                 typename _P2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        predicate_nand_combinator(_P1Fwd&& _p1, _P2Fwd&& _p2)
            : m_pred1(std::forward<_P1Fwd>(_p1))
            , m_pred2(std::forward<_P2Fwd>(_p2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return !(m_pred1(std::forward<_Args>(_args)...) &&
                     m_pred2(std::forward<_Args>(_args)...));
        }
    };

    // predicate_nor_combinator
    //   helper: evaluates two predicates with logical NOR.
    template<typename _Pred1,
             typename _Pred2>
    class predicate_nor_combinator
    {
    private:
        _Pred1 m_pred1;
        _Pred2 m_pred2;

    public:
        template<typename _P1Fwd,
                 typename _P2Fwd>
        D_FUNCTIONAL_CONSTEXPR
        predicate_nor_combinator(_P1Fwd&& _p1, _P2Fwd&& _p2)
            : m_pred1(std::forward<_P1Fwd>(_p1))
            , m_pred2(std::forward<_P2Fwd>(_p2))
        {}

        template<typename... _Args>
        D_FUNCTIONAL_CONSTEXPR
        bool operator()(_Args&&... _args) const
        {
            return !(m_pred1(std::forward<_Args>(_args)...) ||
                     m_pred2(std::forward<_Args>(_args)...));
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   FACTORY FUNCTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// predicate_and
//   function: creates an AND combinator from two predicates.
// predicate_and(p1, p2)(x) = p1(x) && p2(x)
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::predicate_and_combinator<typename std::decay<_Pred1>::type,
                                   typename std::decay<_Pred2>::type>
predicate_and(_Pred1&& _p1, _Pred2&& _p2)
{
    return internal::predicate_and_combinator<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_p1),
            std::forward<_Pred2>(_p2));
}

// predicate_or
//   function: creates an OR combinator from two predicates.
// predicate_or(p1, p2)(x) = p1(x) || p2(x)
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::predicate_or_combinator<typename std::decay<_Pred1>::type,
                                  typename std::decay<_Pred2>::type>
predicate_or(_Pred1&& _p1, _Pred2&& _p2)
{
    return internal::predicate_or_combinator<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_p1),
            std::forward<_Pred2>(_p2));
}

// predicate_xor
//   function: creates an XOR combinator from two predicates.
// predicate_xor(p1, p2)(x) = p1(x) != p2(x)
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::predicate_xor_combinator<typename std::decay<_Pred1>::type,
                                   typename std::decay<_Pred2>::type>
predicate_xor(_Pred1&& _p1, _Pred2&& _p2)
{
    return internal::predicate_xor_combinator<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_p1),
            std::forward<_Pred2>(_p2));
}

// predicate_not
//   function: creates a NOT combinator that negates a predicate.
// predicate_not(p)(x) = !p(x)
template<typename _Pred>
D_FUNCTIONAL_CONSTEXPR
internal::predicate_not_combinator<typename std::decay<_Pred>::type>
predicate_not(_Pred&& _p)
{
    return internal::predicate_not_combinator<
        typename std::decay<_Pred>::type>(
            std::forward<_Pred>(_p));
}

// predicate_nand
//   function: creates a NAND combinator from two predicates.
// predicate_nand(p1, p2)(x) = !(p1(x) && p2(x))
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::predicate_nand_combinator<typename std::decay<_Pred1>::type,
                                    typename std::decay<_Pred2>::type>
predicate_nand(_Pred1&& _p1, _Pred2&& _p2)
{
    return internal::predicate_nand_combinator<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_p1),
            std::forward<_Pred2>(_p2));
}

// predicate_nor
//   function: creates a NOR combinator from two predicates.
// predicate_nor(p1, p2)(x) = !(p1(x) || p2(x))
template<typename _Pred1,
         typename _Pred2>
D_FUNCTIONAL_CONSTEXPR
internal::predicate_nor_combinator<typename std::decay<_Pred1>::type,
                                   typename std::decay<_Pred2>::type>
predicate_nor(_Pred1&& _p1, _Pred2&& _p2)
{
    return internal::predicate_nor_combinator<
        typename std::decay<_Pred1>::type,
        typename std::decay<_Pred2>::type>(
            std::forward<_Pred1>(_p1),
            std::forward<_Pred2>(_p2));
}


///////////////////////////////////////////////////////////////////////////////
///             III.  VARIADIC COMBINATOR HELPERS                           ///
///////////////////////////////////////////////////////////////////////////////

// all_of (variadic predicate AND)
//   function: creates a predicate that is true when all given predicates
// are true. Evaluates left-to-right with short-circuiting.
// all_of(p1, p2, p3)(x) = p1(x) && p2(x) && p3(x)
template<typename _Pred>
D_FUNCTIONAL_CONSTEXPR
auto all_of(_Pred&& _p)
    -> typename std::decay<_Pred>::type
{
    return std::forward<_Pred>(_p);
}

template<typename _Pred1,
         typename _Pred2,
         typename... _Preds>
D_FUNCTIONAL_CONSTEXPR
auto all_of(_Pred1&& _p1, _Pred2&& _p2, _Preds&&... _rest)
    -> decltype(all_of(
           predicate_and(std::forward<_Pred1>(_p1),
                         std::forward<_Pred2>(_p2)),
           std::forward<_Preds>(_rest)...))
{
    return all_of(
        predicate_and(std::forward<_Pred1>(_p1),
                      std::forward<_Pred2>(_p2)),
        std::forward<_Preds>(_rest)...);
}

// any_of (variadic predicate OR)
//   function: creates a predicate that is true when any given predicate
// is true. Evaluates left-to-right with short-circuiting.
// any_of(p1, p2, p3)(x) = p1(x) || p2(x) || p3(x)
template<typename _Pred>
D_FUNCTIONAL_CONSTEXPR
auto any_of(_Pred&& _p)
    -> typename std::decay<_Pred>::type
{
    return std::forward<_Pred>(_p);
}

template<typename _Pred1,
         typename _Pred2,
         typename... _Preds>
D_FUNCTIONAL_CONSTEXPR
auto any_of(_Pred1&& _p1, _Pred2&& _p2, _Preds&&... _rest)
    -> decltype(any_of(
           predicate_or(std::forward<_Pred1>(_p1),
                        std::forward<_Pred2>(_p2)),
           std::forward<_Preds>(_rest)...))
{
    return any_of(
        predicate_or(std::forward<_Pred1>(_p1),
                     std::forward<_Pred2>(_p2)),
        std::forward<_Preds>(_rest)...);
}

// none_of (variadic predicate NOR)
//   function: creates a predicate that is true when none of the given
// predicates are true.
// none_of(p1, p2)(x) = !p1(x) && !p2(x)
template<typename... _Preds>
D_FUNCTIONAL_CONSTEXPR
auto none_of(_Preds&&... _preds)
    -> decltype(predicate_not(any_of(std::forward<_Preds>(_preds)...)))
{
    return predicate_not(any_of(std::forward<_Preds>(_preds)...));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   PREDICATE TRAIT DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_predicate_and
    //   helper: detects if a type is a predicate_and_combinator.
    template<typename _Type>
    struct is_predicate_and_impl : std::false_type {};

    template<typename _P1, typename _P2>
    struct is_predicate_and_impl<predicate_and_combinator<_P1, _P2>>
        : std::true_type {};

    // is_predicate_or
    //   helper: detects if a type is a predicate_or_combinator.
    template<typename _Type>
    struct is_predicate_or_impl : std::false_type {};

    template<typename _P1, typename _P2>
    struct is_predicate_or_impl<predicate_or_combinator<_P1, _P2>>
        : std::true_type {};

    // is_predicate_not
    //   helper: detects if a type is a predicate_not_combinator.
    template<typename _Type>
    struct is_predicate_not_impl : std::false_type {};

    template<typename _P>
    struct is_predicate_not_impl<predicate_not_combinator<_P>>
        : std::true_type {};

NS_END  // internal

// is_predicate_combinator
//   type trait: checks if _Type is any predicate combinator.
template<typename _Type>
struct is_predicate_combinator
    : std::integral_constant<bool,
          internal::is_predicate_and_impl<
              typename std::decay<_Type>::type>::value ||
          internal::is_predicate_or_impl<
              typename std::decay<_Type>::type>::value ||
          internal::is_predicate_not_impl<
              typename std::decay<_Type>::type>::value>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Type>
inline constexpr bool is_predicate_combinator_v =
    is_predicate_combinator<_Type>::value;
#endif


NS_END  // functional
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PREDICATE_COMBINATORS_
