/******************************************************************************
* djinterp [restd]                                              variant_get_if.hpp
*
* variant get_if<I>/get_if<T> header:
*   Non-throwing access: returns a pointer to the active alternative
* if the index/type matches, nullptr otherwise. Useful in code paths
* that prefer pointer-checks to exception-handling.
*
*     variant<int, string> v(42);
*     if (auto* p = get_if<int>(&v)) { ... use *p ... }
*
*   Two-overloads each (mutable / const). No rvalue overload — the
* standard intentionally doesn't provide one (you'd be left with a
* dangling pointer).
*
*
* path:      /inc/djinterp/restd/variant/variant_get_if.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_VARIANT_GET_IF_
#define DJINTERP_RESTD_VARIANT_GET_IF_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include "./variant.hpp"


NS_RESTD


// ===========================================================================
// I.   GET_IF<I> — BY INDEX
// ===========================================================================

template<std::size_t _I,
         typename... _Types>
typename internal::va_type_at<_I, _Types...>::type*
get_if(
    variant<_Types...>* _v
) D_NOEXCEPT
{
    if (!_v || _v->index() != _I)
    {
        return D_NULLPTR;
    }
    return &(_v->template _ref<_I>());
}

template<std::size_t _I,
         typename... _Types>
typename internal::va_type_at<_I, _Types...>::type const*
get_if(
    variant<_Types...> const* _v
) D_NOEXCEPT
{
    if (!_v || _v->index() != _I)
    {
        return D_NULLPTR;
    }
    return &(_v->template _ref<_I>());
}


// ===========================================================================
// II.  GET_IF<T> — BY TYPE
// ===========================================================================

template<typename _T,
         typename... _Types>
_T*
get_if(
    variant<_Types...>* _v
) D_NOEXCEPT
{
    return get_if<internal::index_of<_T, _Types...>::value>(_v);
}

template<typename _T,
         typename... _Types>
_T const*
get_if(
    variant<_Types...> const* _v
) D_NOEXCEPT
{
    return get_if<internal::index_of<_T, _Types...>::value>(_v);
}


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_VARIANT_GET_IF_
