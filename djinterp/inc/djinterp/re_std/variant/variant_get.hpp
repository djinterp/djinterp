/******************************************************************************
* djinterp [re_std]                                               variant_get.hpp
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
* path:      /inc/djinterp/re_std/variant/variant_get.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RE_STD_VARIANT_GET_
#define DJINTERP_RE_STD_VARIANT_GET_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include "./variant.hpp"
#include "./bad_variant_access.hpp"


NS_RESTD


// ===========================================================================
// I.   GET<I> — BY INDEX
// ===========================================================================

template<std::size_t _I,
         typename... _Types>
typename internal::va_type_at<_I, _Types...>::type&
get(
    variant<_Types...>& _v
)
{
    if (_v.index() != _I)
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_variant_access();
#endif
    }
    return _v.template _ref<_I>();
}

template<std::size_t _I,
         typename... _Types>
typename internal::va_type_at<_I, _Types...>::type const&
get(
    variant<_Types...> const& _v
)
{
    if (_v.index() != _I)
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_variant_access();
#endif
    }
    return _v.template _ref<_I>();
}

template<std::size_t _I,
         typename... _Types>
typename internal::va_type_at<_I, _Types...>::type&&
get(
    variant<_Types...>&& _v
)
{
    typedef typename internal::va_type_at<_I, _Types...>::type _T;
    if (_v.index() != _I)
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_variant_access();
#endif
    }
    return static_cast<_T&&>(_v.template _ref<_I>());
}

template<std::size_t _I,
         typename... _Types>
typename internal::va_type_at<_I, _Types...>::type const&&
get(
    variant<_Types...> const&& _v
)
{
    typedef typename internal::va_type_at<_I, _Types...>::type _T;
    if (_v.index() != _I)
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_variant_access();
#endif
    }
    return static_cast<_T const&&>(_v.template _ref<_I>());
}


// ===========================================================================
// II.  GET<T> — BY TYPE (dispatches to get<I>)
// ===========================================================================

template<typename _T,
         typename... _Types>
_T&
get(
    variant<_Types...>& _v
)
{
    return get<internal::index_of<_T, _Types...>::value>(_v);
}

template<typename _T,
         typename... _Types>
_T const&
get(
    variant<_Types...> const& _v
)
{
    return get<internal::index_of<_T, _Types...>::value>(_v);
}

template<typename _T,
         typename... _Types>
_T&&
get(
    variant<_Types...>&& _v
)
{
    return get<internal::index_of<_T, _Types...>::value>(
        static_cast<variant<_Types...>&&>(_v));
}

template<typename _T,
         typename... _Types>
_T const&&
get(
    variant<_Types...> const&& _v
)
{
    return get<internal::index_of<_T, _Types...>::value>(
        static_cast<variant<_Types...> const&&>(_v));
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_VARIANT_GET_
