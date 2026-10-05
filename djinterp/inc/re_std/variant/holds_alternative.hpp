/*******************************************************************************
* djinterp [re_std]                                        holds_alternative.hpp
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
* path:      /inc/re_std/variant/holds_alternative.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_HOLDS_ALTERNATIVE_HPP
#define RE_STD_VARIANT_HOLDS_ALTERNATIVE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include "./variant.hpp"
#include "../type_traits/is_same.hpp"


namespace re_std
{


namespace internal
{

    // va_count_of<T, Types...> — number of times T appears in Types.
    // Used to enforce the "exactly once" rule. Prefixed, as variant's other
    // helpers are (va_type_at): tuple_get.hpp's internal::count_of is a
    // different template, and sharing the name redefined it wherever
    // <tuple> and <variant> met.
    template<typename T, typename... Types>
    struct va_count_of;

    template<typename T>
    struct va_count_of<T>
    {
        static const std::size_t value = 0;
    };

    template<typename T, typename Head, typename... Tail>
    struct va_count_of<T, Head, Tail...>
    {
        static const std::size_t value =
            (re_std::is_same<T, Head>::value ? 1 : 0)
            + va_count_of<T, Tail...>::value;
    };

}  // internal


// ===========================================================================
// I.   HOLDS_ALTERNATIVE
// ===========================================================================

template<typename T,
         typename... Types>
bool
holds_alternative(
    variant<Types...> const& _v
) RE_STD_NOEXCEPT
{
    static_assert(internal::va_count_of<T, Types...>::value == 1,
                  "re_std::holds_alternative<T>: T must appear exactly once "
                  "in the variant's alternative list");
    return _v.index() == internal::index_of<T, Types...>::value;
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_HOLDS_ALTERNATIVE_HPP
