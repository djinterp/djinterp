/*******************************************************************************
* djinterp [re_std]                                          make_from_tuple.hpp
*
* make_from_tuple function header:
*   Constructs an object of type T using the elements of a tuple-like
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
* path:      /inc/re_std/tuple/make_from_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_MAKE_FROM_TUPLE_HPP
#define RE_STD_TUPLE_MAKE_FROM_TUPLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// std
#include <cstddef>
// re_std
#include "./tuple.hpp"
#include "./tuple_size.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/remove_reference.hpp"


namespace re_std
{


// =============================================================================
// I.   MAKE_FROM_TUPLE
// =============================================================================

namespace internal
{

    // mft_index_seq + mft_make_index_seq
    //   trait: local index_seq machinery (parallels apply.hpp's).

    template<std::size_t... Is>
    struct mft_index_seq {};

    template<std::size_t N,
             std::size_t... Is>
    struct mft_make_index_seq
        : mft_make_index_seq<N - 1, N - 1, Is...>
    {};

    template<std::size_t... Is>
    struct mft_make_index_seq<0, Is...>
    {
        typedef mft_index_seq<Is...> type;
    };


    // make_from_tuple_impl
    //   function: expands the index pack, calling T's ctor with
    // get<I>(_t)... .
    template<typename       T,
             typename       Tup,
             std::size_t... Is>
    RE_STD_CONSTEXPR
    T
    make_from_tuple_impl(
        Tup&&  _t,
        mft_index_seq<Is...>
    )
    {
        return T(get<Is>(static_cast<Tup&&>(_t))...);
    }

}  // internal


// make_from_tuple<T>
//   function: constructs a T using the elements of _t as ctor args.
template<typename T,
         typename Tup>
RE_STD_CONSTEXPR
T
make_from_tuple(
    Tup&& _t
)
{
    return internal::make_from_tuple_impl<T>(
        static_cast<Tup&&>(_t),
        typename internal::mft_make_index_seq<
            tuple_size<typename remove_reference<Tup>::type>::value
        >::type());
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_MAKE_FROM_TUPLE_HPP
