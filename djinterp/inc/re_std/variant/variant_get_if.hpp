/*******************************************************************************
* djinterp [re_std]                                           variant_get_if.hpp
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
* path:      /inc/re_std/variant/variant_get_if.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_GET_IF_HPP
#define RE_STD_VARIANT_VARIANT_GET_IF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include "./variant.hpp"


namespace re_std
{


// ===========================================================================
// I.   GET_IF<I> — BY INDEX
// ===========================================================================

template<std::size_t I,
         typename... Types>
typename internal::va_type_at<I, Types...>::type*
get_if(
    variant<Types...>* _v
) RE_STD_NOEXCEPT
{
    if (!_v || _v->index() != I)
    {
        return RE_STD_NULLPTR;
    }
    return &(_v->template _ref<I>());
}

template<std::size_t I,
         typename... Types>
typename internal::va_type_at<I, Types...>::type const*
get_if(
    variant<Types...> const* _v
) RE_STD_NOEXCEPT
{
    if (!_v || _v->index() != I)
    {
        return RE_STD_NULLPTR;
    }
    return &(_v->template _ref<I>());
}


// ===========================================================================
// II.  GET_IF<T> — BY TYPE
// ===========================================================================

template<typename T,
         typename... Types>
T*
get_if(
    variant<Types...>* _v
) RE_STD_NOEXCEPT
{
    return get_if<internal::index_of<T, Types...>::value>(_v);
}

template<typename T,
         typename... Types>
T const*
get_if(
    variant<Types...> const* _v
) RE_STD_NOEXCEPT
{
    return get_if<internal::index_of<T, Types...>::value>(_v);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_GET_IF_HPP
