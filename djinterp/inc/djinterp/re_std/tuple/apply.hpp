/******************************************************************************
* djinterp [restd]                                                    apply.hpp
*
* apply function header:
*   Invokes a callable with the elements of a tuple-like object as
* arguments. C++17 standard library function, shimmed to C++11+.
*
*     auto sum = [](int a, int b, int c){ return a + b + c; };
*     apply(sum, make_tuple(1, 2, 3));   // -> 6
*
*   IMPLEMENTATION:
*   Uses the same internal index_seq machinery as tuple_cat.
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+). The
* current implementation does NOT use restd::invoke (which would
* enable correct treatment of pointer-to-member callables); when
* restd::invoke lands, this header will be updated to delegate to it.
* Plain function pointers, function objects, and lambdas work today.
*
*
* path:      /inc/djinterp/re_std/tuple/apply.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_APPLY_
#define DJINTERP_RESTD_TUPLE_APPLY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "./tuple.hpp"
#include "./tuple_size.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/remove_reference.hpp"


NS_RESTD


// =============================================================================
// I.   APPLY
// =============================================================================

NS_INTERNAL

    // apply_index_seq + apply_make_index_seq
    //   helpers: local copies of the index_seq machinery to keep apply
    // self-contained. These will be unified with a future
    // restd::index_sequence when <utility> lands.

    template<std::size_t... _Is>
    struct apply_index_seq {};

    template<std::size_t _N,
             std::size_t... _Is>
    struct apply_make_index_seq
        : apply_make_index_seq<_N - 1, _N - 1, _Is...>
    {};

    template<std::size_t... _Is>
    struct apply_make_index_seq<0, _Is...>
    {
        typedef apply_index_seq<_Is...> type;
    };


    // apply_impl
    //   helper: expands the index pack, calling _f with get<I>(_t)... .
    template<typename       _F,
             typename       _Tup,
             std::size_t... _Is>
    D_CONSTEXPR
    auto
    apply_impl(
        _F&&    _f,
        _Tup&&  _t,
        apply_index_seq<_Is...>
    ) -> decltype(static_cast<_F&&>(_f)(get<_Is>(static_cast<_Tup&&>(_t))...))
    {
        return static_cast<_F&&>(_f)(get<_Is>(static_cast<_Tup&&>(_t))...);
    }

NS_END  // internal


// apply
//   function: invokes _f with the elements of _t as arguments.
template<typename _F,
         typename _Tup>
D_CONSTEXPR
auto
apply(
    _F&&    _f,
    _Tup&&  _t
)
    -> decltype(internal::apply_impl(
        static_cast<_F&&>(_f),
        static_cast<_Tup&&>(_t),
        typename internal::apply_make_index_seq<
            tuple_size<typename remove_reference<_Tup>::type>::value
        >::type()))
{
    return internal::apply_impl(
        static_cast<_F&&>(_f),
        static_cast<_Tup&&>(_t),
        typename internal::apply_make_index_seq<
            tuple_size<typename remove_reference<_Tup>::type>::value
        >::type());
}


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_APPLY_
