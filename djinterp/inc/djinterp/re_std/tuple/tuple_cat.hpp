/******************************************************************************
* djinterp [re_std]                                             tuple_cat.hpp
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
* path:      /inc/djinterp/re_std/tuple/tuple_cat.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TUPLE_TUPLE_CAT_
#define DJINTERP_RE_STD_TUPLE_TUPLE_CAT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "./tuple.hpp"
#include "../utility/integer_sequence.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "./tuple_size.hpp"
#include "./tuple_element.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/decay.hpp"
#include "../type_traits/remove_reference.hpp"


NS_RESTD


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

NS_INTERNAL


    // build_indexed_tuple
    //   helper: given a tuple-like _T and an index pack, materialise
    // a new tuple whose elements are get<I>(forwarded _T)... .
    template<typename       _Tup,
             std::size_t... _Is>
    D_CONSTEXPR
    tuple<typename tuple_element<_Is,
              typename remove_reference<_Tup>::type>::type...>
    build_indexed_tuple(
        _Tup&&         _t,
        re_std::index_sequence<_Is...>
    )
    {
        return tuple<typename tuple_element<_Is,
                  typename remove_reference<_Tup>::type>::type...>(
            get<_Is>(static_cast<_Tup&&>(_t))...);
    }


    // tuple_cat_impl_2
    //   helper: concatenates exactly two tuples. Builds two index
    // sequences -- one for each input -- and constructs a fresh
    // tuple from get<i>(a)... get<j>(b)... .
    template<typename       _A,
             typename       _B,
             std::size_t... _IA,
             std::size_t... _IB>
    D_CONSTEXPR
    tuple<
        typename tuple_element<_IA,
            typename remove_reference<_A>::type>::type...,
        typename tuple_element<_IB,
            typename remove_reference<_B>::type>::type...>
    tuple_cat_impl_2(
        _A&&             _a,
        _B&&             _b,
        re_std::index_sequence<_IA...>,
        re_std::index_sequence<_IB...>)
    {
        return tuple<
            typename tuple_element<_IA,
                typename remove_reference<_A>::type>::type...,
            typename tuple_element<_IB,
                typename remove_reference<_B>::type>::type...>(
            get<_IA>(static_cast<_A&&>(_a))...,
            get<_IB>(static_cast<_B&&>(_b))...);
    }

NS_END  // internal


// =============================================================================
// II.  TUPLE_CAT
// =============================================================================

// tuple_cat()
//   function: zero-argument case yields an empty tuple.
D_INLINE D_CONSTEXPR
tuple<>
tuple_cat() D_NOEXCEPT
{
    return tuple<>();
}

// tuple_cat(t)
//   function: one-argument case: rebuild the input as a fresh tuple
// (decaying the element types per the standard's behaviour).
template<typename _A>
D_CONSTEXPR
auto
tuple_cat(
    _A&& _a
)
    -> decltype(
        internal::build_indexed_tuple(
            static_cast<_A&&>(_a),
            re_std::make_index_sequence<
                tuple_size<typename remove_reference<_A>::type>::value
            >()))
{
    return internal::build_indexed_tuple(
        static_cast<_A&&>(_a),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<_A>::type>::value
        >());
}

// tuple_cat(t, u)
//   function: two-argument case: dispatch to tuple_cat_impl_2.
template<typename _A,
         typename _B>
D_CONSTEXPR
auto
tuple_cat(
    _A&& _a,
    _B&& _b
)
    -> decltype(
        internal::tuple_cat_impl_2(
            static_cast<_A&&>(_a),
            static_cast<_B&&>(_b),
            re_std::make_index_sequence<
                tuple_size<typename remove_reference<_A>::type>::value
            >(),
            re_std::make_index_sequence<
                tuple_size<typename remove_reference<_B>::type>::value
            >()))
{
    return internal::tuple_cat_impl_2(
        static_cast<_A&&>(_a),
        static_cast<_B&&>(_b),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<_A>::type>::value
        >(),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<_B>::type>::value
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
template<typename    _A,
         typename    _B,
         typename    _C,
         typename... _Rest>
D_CONSTEXPR
auto
tuple_cat(
    _A&&        _a,
    _B&&        _b,
    _C&&        _c,
    _Rest&&...  _rest
)
    -> decltype(
        tuple_cat(
            tuple_cat(static_cast<_A&&>(_a),
                      static_cast<_B&&>(_b)),
            static_cast<_C&&>(_c),
            static_cast<_Rest&&>(_rest)...))
{
    return tuple_cat(
        tuple_cat(static_cast<_A&&>(_a),
                  static_cast<_B&&>(_b)),
        static_cast<_C&&>(_c),
        static_cast<_Rest&&>(_rest)...);
}


NS_END  // re_std


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RE_STD_TUPLE_TUPLE_CAT_
