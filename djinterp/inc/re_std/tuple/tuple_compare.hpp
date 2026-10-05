/*******************************************************************************
* djinterp [re_std]                                            tuple_compare.hpp
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
* path:      /inc/re_std/tuple/tuple_compare.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_COMPARE_HPP
#define RE_STD_TUPLE_TUPLE_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// std
#include <cstddef>
// re_std
#include "./tuple.hpp"
#include "./tuple_get.hpp"      // re_std::get<I>(tuple)


namespace re_std
{


// =============================================================================
// I.   EQUALITY (==, !=)
// =============================================================================

namespace internal
{

    // tuple_eq_impl
    //   trait: recursive lexicographic equality. Index parameter
    // walks from 0 to N. Generic case compares head and recurses.
    template<std::size_t I,
             std::size_t N>
    struct tuple_eq_impl
    {
        template<typename A,
                 typename B>
        static RE_STD_CONSTEXPR bool
        eq(
            const A& _a,
            const B& _b
        )
        {
            return ( get<I>(_a) == get<I>(_b) ) &&
                   tuple_eq_impl<I + 1, N>::eq(_a, _b);
        }
    };

    template<std::size_t N>
    struct tuple_eq_impl<N, N>
    {
        template<typename A,
                 typename B>
        static RE_STD_CONSTEXPR bool
        eq(
            const A&,
            const B&
        )
        {
            return true;
        }
    };

}  // internal


// operator==
//   function: lexicographic equality of two equal-arity tuples.
template<typename... A,
         typename... B>
RE_STD_CONSTEXPR
bool
operator==(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    static_assert(sizeof...(A) == sizeof...(B),
                  "tuple operator==: arity mismatch");
    return internal::tuple_eq_impl<0, sizeof...(A)>::eq(_lhs, _rhs);
}

// operator!=
template<typename... A,
         typename... B>
RE_STD_CONSTEXPR
bool
operator!=(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    return !(_lhs == _rhs);
}


// =============================================================================
// II.  ORDERING (<, <=, >, >=)
// =============================================================================

namespace internal
{

    // tuple_lt_impl
    //   trait: recursive lexicographic less-than. At each step:
    //   - if a < b at this element: true
    //   - else if b < a at this element: false
    //   - else recurse on tail
    template<std::size_t I,
             std::size_t N>
    struct tuple_lt_impl
    {
        template<typename A,
                 typename B>
        static RE_STD_CONSTEXPR bool
        lt(
            const A& _a,
            const B& _b
        )
        {
            return ( get<I>(_a) < get<I>(_b) )
                ? true
                : ( get<I>(_b) < get<I>(_a) )
                    ? false
                    : tuple_lt_impl<I + 1, N>::lt(_a, _b);
        }
    };

    template<std::size_t N>
    struct tuple_lt_impl<N, N>
    {
        template<typename A,
                 typename B>
        static RE_STD_CONSTEXPR bool
        lt(
            const A&,
            const B&
        )
        {
            return false;
        }
    };

}  // internal


// operator<
//   function: lexicographic less-than.
template<typename... A,
         typename... B>
RE_STD_CONSTEXPR
bool
operator<(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    static_assert(sizeof...(A) == sizeof...(B),
                  "tuple operator<: arity mismatch");
    return internal::tuple_lt_impl<0, sizeof...(A)>::lt(_lhs, _rhs);
}

// operator<=
template<typename... A,
         typename... B>
RE_STD_CONSTEXPR
bool
operator<=(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    return !(_rhs < _lhs);
}

// operator>
template<typename... A,
         typename... B>
RE_STD_CONSTEXPR
bool
operator>(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    return _rhs < _lhs;
}

// operator>=
template<typename... A,
         typename... B>
RE_STD_CONSTEXPR
bool
operator>=(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    return !(_lhs < _rhs);
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_TUPLE_COMPARE_HPP
