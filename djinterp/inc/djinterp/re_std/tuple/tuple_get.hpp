/******************************************************************************
* djinterp [restd]                                              tuple_get.hpp
*
* tuple get<> overloads:
*   Index-based and type-based element access for restd::tuple.
*
*   INDEX-BASED (C++11+):
*     get<0>(tup)      -> reference to the first element.
*     get<I>(tup)      -> reference to the I-th element.
*   Returns the appropriate reference category for the tuple's value
*   category (tup&, const tup&, tup&&, const tup&&).
*
*   TYPE-BASED (C++14+):
*     get<T>(tup)      -> reference to the (unique) element of type T.
*   Ill-formed if T appears zero times or more than once in the
*   tuple's element types. Detection is via SFINAE on a count-of-T
*   metafunction.
*
*   PORTABILITY:
*   Requires C++11+ (tuple itself requires C++11+). The type-based
* form additionally requires C++14+ alias-template machinery for
* sane SFINAE expression on the count.
*
*
* path:      /inc/djinterp/re_std/tuple/tuple_get.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_TUPLE_GET_
#define DJINTERP_RESTD_TUPLE_TUPLE_GET_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: requires variadic templates + rvalue refs (same as tuple)
#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "./tuple.hpp"
#include "./tuple_element.hpp"
#include "../type_traits/integral_constant.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/enable_if.hpp"


NS_RESTD


// =============================================================================
// I.   GET BY INDEX
// =============================================================================

NS_INTERNAL

    // tuple_get_impl
    //   helper: recursively descends through the tail of a tuple to
    // locate the _I-th element. The base case (_I == 0) returns the
    // current head.

    template<std::size_t _I>
    struct tuple_get_impl
    {
        template<typename    _Head,
                 typename... _Tail>
        D_STATIC D_CONSTEXPR
        typename tuple_element<_I, tuple<_Head, _Tail...> >::type&
        get_lref(
            tuple<_Head, _Tail...>& _t
        ) D_NOEXCEPT
        {
            return tuple_get_impl<_I - 1>::get_lref(_t.tail_ref());
        }

        template<typename    _Head,
                 typename... _Tail>
        D_STATIC D_CONSTEXPR
        const typename tuple_element<_I, tuple<_Head, _Tail...> >::type&
        get_clref(
            const tuple<_Head, _Tail...>& _t
        ) D_NOEXCEPT
        {
            return tuple_get_impl<_I - 1>::get_clref(_t.tail_ref());
        }
    };

    template<>
    struct tuple_get_impl<0>
    {
        template<typename    _Head,
                 typename... _Tail>
        D_STATIC D_CONSTEXPR
        _Head&
        get_lref(
            tuple<_Head, _Tail...>& _t
        ) D_NOEXCEPT
        {
            return _t.head_ref();
        }

        template<typename    _Head,
                 typename... _Tail>
        D_STATIC D_CONSTEXPR
        const _Head&
        get_clref(
            const tuple<_Head, _Tail...>& _t
        ) D_NOEXCEPT
        {
            return _t.head_ref();
        }
    };

NS_END  // internal


// get<I>(tuple&)
//   function: yields lvalue reference to the I-th element.
template<std::size_t _I,
         typename... _Types>
D_CONSTEXPR
typename tuple_element<_I, tuple<_Types...> >::type&
get(
    tuple<_Types...>& _t
) D_NOEXCEPT
{
    return internal::tuple_get_impl<_I>::get_lref(_t);
}

// get<I>(const tuple&)
template<std::size_t _I,
         typename... _Types>
D_CONSTEXPR
const typename tuple_element<_I, tuple<_Types...> >::type&
get(
    const tuple<_Types...>& _t
) D_NOEXCEPT
{
    return internal::tuple_get_impl<_I>::get_clref(_t);
}

// get<I>(tuple&&)
template<std::size_t _I,
         typename... _Types>
D_CONSTEXPR
typename tuple_element<_I, tuple<_Types...> >::type&&
get(
    tuple<_Types...>&& _t
) D_NOEXCEPT
{
    typedef typename tuple_element<_I, tuple<_Types...> >::type _E;
    return static_cast<_E&&>(
        internal::tuple_get_impl<_I>::get_lref(_t));
}

// get<I>(const tuple&&)
template<std::size_t _I,
         typename... _Types>
D_CONSTEXPR
const typename tuple_element<_I, tuple<_Types...> >::type&&
get(
    const tuple<_Types...>&& _t
) D_NOEXCEPT
{
    typedef typename tuple_element<_I, tuple<_Types...> >::type _E;
    return static_cast<const _E&&>(
        internal::tuple_get_impl<_I>::get_clref(_t));
}


// =============================================================================
// II.  GET BY TYPE  (C++14+)
// =============================================================================
// Locate the unique element whose type is _T, then dispatch to the
// index-based get. Ambiguous (>1 match) or missing (0 matches) cases
// are SFINAE-rejected by enable_if on count == 1.

#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_INTERNAL

    // count_of
    //   trait: number of times _T appears in _Pack.
    template<typename    _T,
             typename... _Pack>
    struct count_of;

    template<typename _T>
    struct count_of<_T>
        : integral_constant<std::size_t, 0>
    {};

    template<typename    _T,
             typename    _Head,
             typename... _Tail>
    struct count_of<_T, _Head, _Tail...>
        : integral_constant<std::size_t,
              ( is_same<_T, _Head>::value ? 1 : 0 ) +
              count_of<_T, _Tail...>::value>
    {};

    // first_index_of
    //   trait: zero-based index of the first occurrence of _T in
    // _Pack. Caller must ensure _T appears.
    template<typename    _T,
             typename... _Pack>
    struct first_index_of;

    template<typename    _T,
             typename    _Head,
             typename... _Tail>
    struct first_index_of<_T, _Head, _Tail...>
        : integral_constant<std::size_t,
              is_same<_T, _Head>::value
                  ? 0
                  : 1 + first_index_of<_T, _Tail...>::value>
    {};

NS_END  // internal


// get<T>(tuple&)
template<typename    _T,
         typename... _Types>
D_CONSTEXPR
typename enable_if<
    internal::count_of<_T, _Types...>::value == 1,
    _T&
>::type
get(
    tuple<_Types...>& _t
) D_NOEXCEPT
{
    return get<internal::first_index_of<_T, _Types...>::value>(_t);
}

// get<T>(const tuple&)
template<typename    _T,
         typename... _Types>
D_CONSTEXPR
typename enable_if<
    internal::count_of<_T, _Types...>::value == 1,
    const _T&
>::type
get(
    const tuple<_Types...>& _t
) D_NOEXCEPT
{
    return get<internal::first_index_of<_T, _Types...>::value>(_t);
}

// get<T>(tuple&&)
template<typename    _T,
         typename... _Types>
D_CONSTEXPR
typename enable_if<
    internal::count_of<_T, _Types...>::value == 1,
    _T&&
>::type
get(
    tuple<_Types...>&& _t
) D_NOEXCEPT
{
    return get<internal::first_index_of<_T, _Types...>::value>(
        static_cast<tuple<_Types...>&&>(_t));
}

// get<T>(const tuple&&)
template<typename    _T,
         typename... _Types>
D_CONSTEXPR
typename enable_if<
    internal::count_of<_T, _Types...>::value == 1,
    const _T&&
>::type
get(
    const tuple<_Types...>&& _t
) D_NOEXCEPT
{
    return get<internal::first_index_of<_T, _Types...>::value>(
        static_cast<const tuple<_Types...>&&>(_t));
}


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_TUPLE_GET_
