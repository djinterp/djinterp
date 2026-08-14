/******************************************************************************
* djinterp [restd]                                              tuple_cat.hpp
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
*   Uses an internal index_sequence machinery (private to this header
* to avoid polluting restd:: with utilities that std puts in <utility>).
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

#ifndef DJINTERP_RESTD_TUPLE_TUPLE_CAT_
#define DJINTERP_RESTD_TUPLE_TUPLE_CAT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "./tuple.hpp"
#include "./tuple_size.hpp"
#include "./tuple_element.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/decay.hpp"
#include "../type_traits/remove_reference.hpp"


NS_RESTD


// =============================================================================
// I.   INTERNAL: index_sequence
// =============================================================================
// A small index_sequence implementation private to tuple_cat. When
// restd::utility lands with the public version, this will switch to
// re-using that.

NS_INTERNAL

    template<std::size_t... _Is>
    struct index_seq {};

    template<std::size_t _N,
             std::size_t... _Is>
    struct make_index_seq
        : make_index_seq<_N - 1, _N - 1, _Is...>
    {};

    template<std::size_t... _Is>
    struct make_index_seq<0, _Is...>
    {
        typedef index_seq<_Is...> type;
    };


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
        index_seq<_Is...>
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
        index_seq<_IA...>,
        index_seq<_IB...>)
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
            typename internal::make_index_seq<
                tuple_size<typename remove_reference<_A>::type>::value
            >::type()))
{
    return internal::build_indexed_tuple(
        static_cast<_A&&>(_a),
        typename internal::make_index_seq<
            tuple_size<typename remove_reference<_A>::type>::value
        >::type());
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
            typename internal::make_index_seq<
                tuple_size<typename remove_reference<_A>::type>::value
            >::type(),
            typename internal::make_index_seq<
                tuple_size<typename remove_reference<_B>::type>::value
            >::type()))
{
    return internal::tuple_cat_impl_2(
        static_cast<_A&&>(_a),
        static_cast<_B&&>(_b),
        typename internal::make_index_seq<
            tuple_size<typename remove_reference<_A>::type>::value
        >::type(),
        typename internal::make_index_seq<
            tuple_size<typename remove_reference<_B>::type>::value
        >::type());
}

// tuple_cat(a, b, rest...)
//   function: N-argument case (N > 2): pair-wise reduction.
template<typename    _A,
         typename    _B,
         typename... _Rest>
D_CONSTEXPR
auto
tuple_cat(
    _A&&        _a,
    _B&&        _b,
    _Rest&&...  _rest
)
    -> decltype(
        tuple_cat(
            tuple_cat(static_cast<_A&&>(_a),
                      static_cast<_B&&>(_b)),
            static_cast<_Rest&&>(_rest)...))
{
    return tuple_cat(
        tuple_cat(static_cast<_A&&>(_a),
                  static_cast<_B&&>(_b)),
        static_cast<_Rest&&>(_rest)...);
}


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_TUPLE_CAT_
