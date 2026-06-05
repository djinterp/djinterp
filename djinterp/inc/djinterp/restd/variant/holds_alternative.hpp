/******************************************************************************
* djinterp [restd]                                          holds_alternative.hpp
*
* holds_alternative<T>(v) header:
*   Query: does variant v currently hold an alternative of type T?
* Requires T to appear EXACTLY ONCE in v's alternative list (the
* trait would be ambiguous otherwise — same rule std uses).
*
*     variant<int, string> v(42);
*     holds_alternative<int>(v)    -> true
*     holds_alternative<string>(v) -> false
*
*
* path:      /inc/djinterp/restd/variant/holds_alternative.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_HOLDS_ALTERNATIVE_
#define DJINTERP_RESTD_HOLDS_ALTERNATIVE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include "./variant.hpp"
#include "../type_traits/is_same.hpp"


NS_RESTD


NS_INTERNAL

    // count_of<T, Types...> — number of times T appears in Types.
    // Used to enforce the "exactly once" rule.
    template<typename _T, typename... _Types>
    struct count_of;

    template<typename _T>
    struct count_of<_T>
    {
        static const std::size_t value = 0;
    };

    template<typename _T, typename _Head, typename... _Tail>
    struct count_of<_T, _Head, _Tail...>
    {
        static const std::size_t value =
            (restd::is_same<_T, _Head>::value ? 1 : 0)
            + count_of<_T, _Tail...>::value;
    };

NS_END  // internal


// ===========================================================================
// I.   HOLDS_ALTERNATIVE
// ===========================================================================

template<typename _T,
         typename... _Types>
bool
holds_alternative(
    variant<_Types...> const& _v
) D_NOEXCEPT
{
    static_assert(internal::count_of<_T, _Types...>::value == 1,
                  "restd::holds_alternative<T>: T must appear exactly once "
                  "in the variant's alternative list");
    return _v.index() == internal::index_of<_T, _Types...>::value;
}


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_HOLDS_ALTERNATIVE_
