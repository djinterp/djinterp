/******************************************************************************
* djinterp [restd]                                          make_from_tuple.hpp
*
* make_from_tuple function header:
*   Constructs an object of type _T using the elements of a tuple-like
* object as constructor arguments. C++17 standard library function,
* shimmed to C++11+.
*
*     struct point { point(int, int, int); };
*     auto p = make_from_tuple<point>(make_tuple(1, 2, 3));
*     // equivalent to: point p(1, 2, 3);
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/djinterp/restd/tuple/make_from_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_MAKE_FROM_TUPLE_
#define DJINTERP_RESTD_TUPLE_MAKE_FROM_TUPLE_ 1

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
// I.   MAKE_FROM_TUPLE
// =============================================================================

NS_INTERNAL

    // mft_index_seq + mft_make_index_seq
    //   helpers: local index_seq machinery (parallels apply.hpp's).

    template<std::size_t... _Is>
    struct mft_index_seq {};

    template<std::size_t _N,
             std::size_t... _Is>
    struct mft_make_index_seq
        : mft_make_index_seq<_N - 1, _N - 1, _Is...>
    {};

    template<std::size_t... _Is>
    struct mft_make_index_seq<0, _Is...>
    {
        typedef mft_index_seq<_Is...> type;
    };


    // make_from_tuple_impl
    //   helper: expands the index pack, calling _T's ctor with
    // get<I>(_t)... .
    template<typename       _T,
             typename       _Tup,
             std::size_t... _Is>
    D_CONSTEXPR
    _T
    make_from_tuple_impl(
        _Tup&&  _t,
        mft_index_seq<_Is...>
    )
    {
        return _T(get<_Is>(static_cast<_Tup&&>(_t))...);
    }

NS_END  // internal


// make_from_tuple<T>
//   function: constructs a _T using the elements of _t as ctor args.
template<typename _T,
         typename _Tup>
D_CONSTEXPR
_T
make_from_tuple(
    _Tup&& _t
)
{
    return internal::make_from_tuple_impl<_T>(
        static_cast<_Tup&&>(_t),
        typename internal::mft_make_index_seq<
            tuple_size<typename remove_reference<_Tup>::type>::value
        >::type());
}


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_MAKE_FROM_TUPLE_
