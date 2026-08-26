/******************************************************************************
* djinterp [re_std]                                           tuple_compare.hpp
*
* tuple comparison operators header:
*   Provides ==, !=, <, <=, >, >= for re_std::tuple. Comparison is
* lexicographic over the elements: tuples are equal iff every pair of
* corresponding elements compares equal; less iff the first non-equal
* pair compares less.
*
*     tuple<int, int>(1, 2) == tuple<int, int>(1, 2)   -> true
*     tuple<int, int>(1, 2) <  tuple<int, int>(1, 3)   -> true
*     tuple<int, int>(2, 0) >  tuple<int, int>(1, 9)   -> true
*
*   PORTABILITY:
*   Requires variadic templates (C++11+). On C++20+ the spaceship
* operator could be added; this header sticks to the classic six
* operators which work uniformly on C++11 through C++26.
*
*   The implementation is recursive and avoids std::tuple_size /
* std::get of std::tuple, instead using re_std's own.
*
*
* path:      /inc/djinterp/re_std/tuple/tuple_compare.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TUPLE_TUPLE_COMPARE_
#define DJINTERP_RE_STD_TUPLE_TUPLE_COMPARE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "./tuple.hpp"
#include "./tuple_get.hpp"      // re_std::get<I>(tuple)


NS_RESTD


// =============================================================================
// I.   EQUALITY (==, !=)
// =============================================================================

NS_INTERNAL

    // tuple_eq_impl
    //   helper: recursive lexicographic equality. Index parameter
    // walks from 0 to N. Generic case compares head and recurses.
    template<std::size_t _I,
             std::size_t _N>
    struct tuple_eq_impl
    {
        template<typename _A,
                 typename _B>
        D_STATIC D_CONSTEXPR bool
        eq(
            const _A& _a,
            const _B& _b
        )
        {
            return ( get<_I>(_a) == get<_I>(_b) ) &&
                   tuple_eq_impl<_I + 1, _N>::eq(_a, _b);
        }
    };

    template<std::size_t _N>
    struct tuple_eq_impl<_N, _N>
    {
        template<typename _A,
                 typename _B>
        D_STATIC D_CONSTEXPR bool
        eq(
            const _A&,
            const _B&
        )
        {
            return true;
        }
    };

NS_END  // internal


// operator==
//   function: lexicographic equality of two equal-arity tuples.
template<typename... _A,
         typename... _B>
D_CONSTEXPR
bool
operator==(
    const tuple<_A...>& _lhs,
    const tuple<_B...>& _rhs
)
{
    static_assert(sizeof...(_A) == sizeof...(_B),
                  "tuple operator==: arity mismatch");
    return internal::tuple_eq_impl<0, sizeof...(_A)>::eq(_lhs, _rhs);
}

// operator!=
template<typename... _A,
         typename... _B>
D_CONSTEXPR
bool
operator!=(
    const tuple<_A...>& _lhs,
    const tuple<_B...>& _rhs
)
{
    return !(_lhs == _rhs);
}


// =============================================================================
// II.  ORDERING (<, <=, >, >=)
// =============================================================================

NS_INTERNAL

    // tuple_lt_impl
    //   helper: recursive lexicographic less-than. At each step:
    //   - if a < b at this element: true
    //   - else if b < a at this element: false
    //   - else recurse on tail
    template<std::size_t _I,
             std::size_t _N>
    struct tuple_lt_impl
    {
        template<typename _A,
                 typename _B>
        D_STATIC D_CONSTEXPR bool
        lt(
            const _A& _a,
            const _B& _b
        )
        {
            return ( get<_I>(_a) < get<_I>(_b) )
                ? true
                : ( get<_I>(_b) < get<_I>(_a) )
                    ? false
                    : tuple_lt_impl<_I + 1, _N>::lt(_a, _b);
        }
    };

    template<std::size_t _N>
    struct tuple_lt_impl<_N, _N>
    {
        template<typename _A,
                 typename _B>
        D_STATIC D_CONSTEXPR bool
        lt(
            const _A&,
            const _B&
        )
        {
            return false;
        }
    };

NS_END  // internal


// operator<
//   function: lexicographic less-than.
template<typename... _A,
         typename... _B>
D_CONSTEXPR
bool
operator<(
    const tuple<_A...>& _lhs,
    const tuple<_B...>& _rhs
)
{
    static_assert(sizeof...(_A) == sizeof...(_B),
                  "tuple operator<: arity mismatch");
    return internal::tuple_lt_impl<0, sizeof...(_A)>::lt(_lhs, _rhs);
}

// operator<=
template<typename... _A,
         typename... _B>
D_CONSTEXPR
bool
operator<=(
    const tuple<_A...>& _lhs,
    const tuple<_B...>& _rhs
)
{
    return !(_rhs < _lhs);
}

// operator>
template<typename... _A,
         typename... _B>
D_CONSTEXPR
bool
operator>(
    const tuple<_A...>& _lhs,
    const tuple<_B...>& _rhs
)
{
    return _rhs < _lhs;
}

// operator>=
template<typename... _A,
         typename... _B>
D_CONSTEXPR
bool
operator>=(
    const tuple<_A...>& _lhs,
    const tuple<_B...>& _rhs
)
{
    return !(_lhs < _rhs);
}


NS_END  // re_std


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RE_STD_TUPLE_TUPLE_COMPARE_
