/*******************************************************************************
* djinterp [re_std]                                                tuple_get.hpp
*
* tuple get<> overloads:
*   Index-based and type-based element access for re_std::tuple.
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
* path:      /inc/re_std/tuple/tuple_get.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_GET_HPP
#define RE_STD_TUPLE_TUPLE_GET_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: requires variadic templates + rvalue refs (same as tuple)
#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// std
#include <cstddef>
// re_std
#include "./tuple.hpp"
#include "./tuple_element.hpp"
#include "../type_traits/integral_constant.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/enable_if.hpp"


namespace re_std
{


// =============================================================================
// I.   GET BY INDEX
// =============================================================================

namespace internal
{

    // tuple_get_impl
    //   trait: recursively descends through the tail of a tuple to
    // locate the I-th element. The base case (I == 0) returns the
    // current head.

    template<std::size_t I>
    struct tuple_get_impl
    {
        template<typename    Head,
                 typename... Tail>
        static RE_STD_CONSTEXPR
        typename tuple_element<I, tuple<Head, Tail...> >::type&
        get_lref(
            tuple<Head, Tail...>& _t
        ) RE_STD_NOEXCEPT
        {
            return tuple_get_impl<I - 1>::get_lref(_t.tail_ref());
        }

        template<typename    Head,
                 typename... Tail>
        static RE_STD_CONSTEXPR
        const typename tuple_element<I, tuple<Head, Tail...> >::type&
        get_clref(
            const tuple<Head, Tail...>& _t
        ) RE_STD_NOEXCEPT
        {
            return tuple_get_impl<I - 1>::get_clref(_t.tail_ref());
        }
    };

    template<>
    struct tuple_get_impl<0>
    {
        template<typename    Head,
                 typename... Tail>
        static RE_STD_CONSTEXPR
        Head&
        get_lref(
            tuple<Head, Tail...>& _t
        ) RE_STD_NOEXCEPT
        {
            return _t.head_ref();
        }

        template<typename    Head,
                 typename... Tail>
        static RE_STD_CONSTEXPR
        const Head&
        get_clref(
            const tuple<Head, Tail...>& _t
        ) RE_STD_NOEXCEPT
        {
            return _t.head_ref();
        }
    };

}  // internal


// get<I>(tuple&)
//   function: yields lvalue reference to the I-th element.
template<std::size_t I,
         typename... Types>
RE_STD_CONSTEXPR
typename tuple_element<I, tuple<Types...> >::type&
get(
    tuple<Types...>& _t
) RE_STD_NOEXCEPT
{
    return internal::tuple_get_impl<I>::get_lref(_t);
}

// get<I>(const tuple&)
template<std::size_t I,
         typename... Types>
RE_STD_CONSTEXPR
const typename tuple_element<I, tuple<Types...> >::type&
get(
    const tuple<Types...>& _t
) RE_STD_NOEXCEPT
{
    return internal::tuple_get_impl<I>::get_clref(_t);
}

// get<I>(tuple&&)
template<std::size_t I,
         typename... Types>
RE_STD_CONSTEXPR
typename tuple_element<I, tuple<Types...> >::type&&
get(
    tuple<Types...>&& _t
) RE_STD_NOEXCEPT
{
    typedef typename tuple_element<I, tuple<Types...> >::type _E;
    return static_cast<_E&&>(
        internal::tuple_get_impl<I>::get_lref(_t));
}

// get<I>(const tuple&&)
template<std::size_t I,
         typename... Types>
RE_STD_CONSTEXPR
const typename tuple_element<I, tuple<Types...> >::type&&
get(
    const tuple<Types...>&& _t
) RE_STD_NOEXCEPT
{
    typedef typename tuple_element<I, tuple<Types...> >::type _E;
    return static_cast<const _E&&>(
        internal::tuple_get_impl<I>::get_clref(_t));
}


// =============================================================================
// II.  GET BY TYPE  (C++14+)
// =============================================================================
// Locate the unique element whose type is T, then dispatch to the
// index-based get. Ambiguous (>1 match) or missing (0 matches) cases
// are SFINAE-rejected by enable_if on count == 1.

#if RE_STD_LANG_IS_CPP14_OR_HIGHER


namespace internal
{

    // count_of
    //   trait: number of times T appears in Pack.
    template<typename    T,
             typename... Pack>
    struct count_of;

    template<typename T>
    struct count_of<T>
        : integral_constant<std::size_t, 0>
    {};

    template<typename    T,
             typename    Head,
             typename... Tail>
    struct count_of<T, Head, Tail...>
        : integral_constant<std::size_t,
              ( is_same<T, Head>::value ? 1 : 0 ) +
              count_of<T, Tail...>::value>
    {};

    // first_index_of
    //   trait: zero-based index of the first occurrence of T in
    // Pack. Caller must ensure T appears.
    template<typename    T,
             typename... Pack>
    struct first_index_of;

    template<typename    T,
             typename    Head,
             typename... Tail>
    struct first_index_of<T, Head, Tail...>
        : integral_constant<std::size_t,
              is_same<T, Head>::value
                  ? 0
                  : 1 + first_index_of<T, Tail...>::value>
    {};

}  // internal


// get<T>(tuple&)
template<typename    T,
         typename... Types>
RE_STD_CONSTEXPR
typename enable_if<
    internal::count_of<T, Types...>::value == 1,
    T&
>::type
get(
    tuple<Types...>& _t
) RE_STD_NOEXCEPT
{
    return get<internal::first_index_of<T, Types...>::value>(_t);
}

// get<T>(const tuple&)
template<typename    T,
         typename... Types>
RE_STD_CONSTEXPR
typename enable_if<
    internal::count_of<T, Types...>::value == 1,
    const T&
>::type
get(
    const tuple<Types...>& _t
) RE_STD_NOEXCEPT
{
    return get<internal::first_index_of<T, Types...>::value>(_t);
}

// get<T>(tuple&&)
template<typename    T,
         typename... Types>
RE_STD_CONSTEXPR
typename enable_if<
    internal::count_of<T, Types...>::value == 1,
    T&&
>::type
get(
    tuple<Types...>&& _t
) RE_STD_NOEXCEPT
{
    return get<internal::first_index_of<T, Types...>::value>(
        static_cast<tuple<Types...>&&>(_t));
}

// get<T>(const tuple&&)
template<typename    T,
         typename... Types>
RE_STD_CONSTEXPR
typename enable_if<
    internal::count_of<T, Types...>::value == 1,
    const T&&
>::type
get(
    const tuple<Types...>&& _t
) RE_STD_NOEXCEPT
{
    return get<internal::first_index_of<T, Types...>::value>(
        static_cast<const tuple<Types...>&&>(_t));
}


#endif  // RE_STD_LANG_IS_CPP14_OR_HIGHER


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_TUPLE_GET_HPP
