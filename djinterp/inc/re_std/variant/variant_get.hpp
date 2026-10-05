/*******************************************************************************
* djinterp [re_std]                                              variant_get.hpp
*
* variant get<I>/get<T> header:
*   Type-safe access to a variant's active alternative. Throws
* bad_variant_access if the requested alternative is not active.
*
*   FORMS:
*     get<I>(v)   — by index (I < sizeof...(Types))
*     get<T>(v)   — by type (T must appear exactly once in Types)
*
*   Each form ships in 4 ref-qualified overloads:
*     T&        get<.>(variant<...>&)
*     T const&  get<.>(variant<...> const&)
*     T&&       get<.>(variant<...>&&)
*     T const&& get<.>(variant<...> const&&)
*
*   get<T> simply dispatches to get<I> where I is the index of T in
* the alternative list — same approach std uses.
*
*
* path:      /inc/re_std/variant/variant_get.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_GET_HPP
#define RE_STD_VARIANT_VARIANT_GET_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include "./variant.hpp"
#include "./bad_variant_access.hpp"


namespace re_std
{


// ===========================================================================
// I.   GET<I> — BY INDEX
// ===========================================================================

template<std::size_t I,
         typename... Types>
typename internal::va_type_at<I, Types...>::type&
get(
    variant<Types...>& _v
)
{
    if (_v.index() != I)
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
#endif
    }
    return _v.template _ref<I>();
}

template<std::size_t I,
         typename... Types>
typename internal::va_type_at<I, Types...>::type const&
get(
    variant<Types...> const& _v
)
{
    if (_v.index() != I)
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
#endif
    }
    return _v.template _ref<I>();
}

template<std::size_t I,
         typename... Types>
typename internal::va_type_at<I, Types...>::type&&
get(
    variant<Types...>&& _v
)
{
    typedef typename internal::va_type_at<I, Types...>::type T;
    if (_v.index() != I)
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
#endif
    }
    return static_cast<T&&>(_v.template _ref<I>());
}

template<std::size_t I,
         typename... Types>
typename internal::va_type_at<I, Types...>::type const&&
get(
    variant<Types...> const&& _v
)
{
    typedef typename internal::va_type_at<I, Types...>::type T;
    if (_v.index() != I)
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
#endif
    }
    return static_cast<T const&&>(_v.template _ref<I>());
}


// ===========================================================================
// II.  GET<T> — BY TYPE (dispatches to get<I>)
// ===========================================================================

template<typename T,
         typename... Types>
T&
get(
    variant<Types...>& _v
)
{
    return get<internal::index_of<T, Types...>::value>(_v);
}

template<typename T,
         typename... Types>
T const&
get(
    variant<Types...> const& _v
)
{
    return get<internal::index_of<T, Types...>::value>(_v);
}

template<typename T,
         typename... Types>
T&&
get(
    variant<Types...>&& _v
)
{
    return get<internal::index_of<T, Types...>::value>(
        static_cast<variant<Types...>&&>(_v));
}

template<typename T,
         typename... Types>
T const&&
get(
    variant<Types...> const&& _v
)
{
    return get<internal::index_of<T, Types...>::value>(
        static_cast<variant<Types...> const&&>(_v));
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_GET_HPP
