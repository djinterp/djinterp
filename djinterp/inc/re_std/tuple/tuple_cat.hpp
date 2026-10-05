/*******************************************************************************
* djinterp [re_std]                                                tuple_cat.hpp
*
* tuple_cat factory header:
*   Concatenates any number of tuple-like objects into a single tuple
* whose element types are the concatenation of the input element type
* sequences.
*
*     tuple_cat(make_tuple(1, 'a'),
*               make_tuple(3.14, "x"))
*       -> tuple<int, char, double, const char*>
*
*   IMPLEMENTATION:
*   Uses re_std::index_sequence / make_index_sequence from <utility>,
* shared with apply, to_array and make_from_tuple rather than kept
* private to this header.
* The recursive cat reduces the variadic input to two tuples at a
* time and dispatches to a make_from_indices that gathers all
* elements into a single new tuple.
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/re_std/tuple/tuple_cat.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_CAT_HPP
#define RE_STD_TUPLE_TUPLE_CAT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// std
#include <cstddef>
// re_std
#include "./tuple.hpp"
#include "../utility/integer_sequence.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "./tuple_size.hpp"
#include "./tuple_element.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/decay.hpp"
#include "../type_traits/remove_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   INTERNAL HELPERS
// =============================================================================
// The private index_seq / make_index_seq pair that used to live here has
// been retired in favour of re_std::index_sequence and
// re_std::make_index_sequence from <utility> (completed 2026-08-25).
// The public versions are linear-depth rather than the O(N) recursion
// this header carried, and sharing them means apply, to_array, make_from_tuple
// and tuple_cat all instantiate the SAME specialisations instead of four
// mutually incompatible copies.

namespace internal
{


    // build_indexed_tuple
    //   function: given a tuple-like _T and an index pack, materialise
    // a new tuple whose elements are get<I>(forwarded _T)... .
    template<typename       Tup,
             std::size_t... Is>
    RE_STD_CONSTEXPR
    tuple<typename tuple_element<Is,
              typename remove_reference<Tup>::type>::type...>
    build_indexed_tuple(
        Tup&&         _t,
        re_std::index_sequence<Is...>
    )
    {
        return tuple<typename tuple_element<Is,
                  typename remove_reference<Tup>::type>::type...>(
            get<Is>(static_cast<Tup&&>(_t))...);
    }


    // tuple_cat_impl_2
    //   function: concatenates exactly two tuples. Builds two index
    // sequences -- one for each input -- and constructs a fresh
    // tuple from get<i>(a)... get<j>(b)... .
    template<typename       A,
             typename       B,
             std::size_t... IA,
             std::size_t... IB>
    RE_STD_CONSTEXPR
    tuple<
        typename tuple_element<IA,
            typename remove_reference<A>::type>::type...,
        typename tuple_element<IB,
            typename remove_reference<B>::type>::type...>
    tuple_cat_impl_2(
        A&&             _a,
        B&&             _b,
        re_std::index_sequence<IA...>,
        re_std::index_sequence<IB...>)
    {
        return tuple<
            typename tuple_element<IA,
                typename remove_reference<A>::type>::type...,
            typename tuple_element<IB,
                typename remove_reference<B>::type>::type...>(
            get<IA>(static_cast<A&&>(_a))...,
            get<IB>(static_cast<B&&>(_b))...);
    }

}  // internal


// =============================================================================
// II.  TUPLE_CAT
// =============================================================================

// tuple_cat()
//   function: zero-argument case yields an empty tuple.
RE_STD_INLINE RE_STD_CONSTEXPR
tuple<>
tuple_cat() RE_STD_NOEXCEPT
{
    return tuple<>();
}

// tuple_cat(t)
//   function: one-argument case: rebuild the input as a fresh tuple
// (decaying the element types per the standard's behaviour).
template<typename A>
RE_STD_CONSTEXPR
auto
tuple_cat(
    A&& _a
)
    -> decltype(
        internal::build_indexed_tuple(
            static_cast<A&&>(_a),
            re_std::make_index_sequence<
                tuple_size<typename remove_reference<A>::type>::value
            >()))
{
    return internal::build_indexed_tuple(
        static_cast<A&&>(_a),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<A>::type>::value
        >());
}

// tuple_cat(t, u)
//   function: two-argument case: dispatch to tuple_cat_impl_2.
template<typename A,
         typename B>
RE_STD_CONSTEXPR
auto
tuple_cat(
    A&& _a,
    B&& _b
)
    -> decltype(
        internal::tuple_cat_impl_2(
            static_cast<A&&>(_a),
            static_cast<B&&>(_b),
            re_std::make_index_sequence<
                tuple_size<typename remove_reference<A>::type>::value
            >(),
            re_std::make_index_sequence<
                tuple_size<typename remove_reference<B>::type>::value
            >()))
{
    return internal::tuple_cat_impl_2(
        static_cast<A&&>(_a),
        static_cast<B&&>(_b),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<A>::type>::value
        >(),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<B>::type>::value
        >());
}

// tuple_cat(a, b, c, rest...)
//   function: N-argument case (N >= 3): pair-wise reduction.
//
//   Spelled with an explicit THIRD parameter rather than as
// (a, b, rest...) with a variadic tail. That earlier shape was also a
// viable candidate for a TWO-argument call, so the inner
// `tuple_cat(tuple_cat(a,b))` in its own trailing return type
// re-selected this same template, re-instantiated its own return type,
// and recursed until the compiler hit its instantiation-depth limit.
// Requiring a third named argument removes it from the two-argument
// overload set entirely, so the reduction terminates on the 2-arg
// overload above. A structural fix rather than an enable_if, because
// the trailing return type is what recurses -- an enable_if in the
// return type would still have to name tuple_cat to compute it.
template<typename    A,
         typename    B,
         typename    C,
         typename... Rest>
RE_STD_CONSTEXPR
auto
tuple_cat(
    A&&        _a,
    B&&        _b,
    C&&        _c,
    Rest&&...  _rest
)
    -> decltype(
        tuple_cat(
            tuple_cat(static_cast<A&&>(_a),
                      static_cast<B&&>(_b)),
            static_cast<C&&>(_c),
            static_cast<Rest&&>(_rest)...))
{
    return tuple_cat(
        tuple_cat(static_cast<A&&>(_a),
                  static_cast<B&&>(_b)),
        static_cast<C&&>(_c),
        static_cast<Rest&&>(_rest)...);
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_TUPLE_CAT_HPP
