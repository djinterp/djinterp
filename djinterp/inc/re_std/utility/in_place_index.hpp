/*******************************************************************************
* djinterp [re_std]                                           in_place_index.hpp
*
* in_place_index tag type and variable:
*   Disambiguating tag for index-tagged in-place construction in
* variant<Ts...>. Where in_place_type<T> selects an alternative by
* type, in_place_index<I> selects by zero-based position in the
* alternative list. Useful when several alternatives share a common
* type.
*
*   Provided as a class template (in_place_index_t<I>) plus a variable
* template (in_place_index<I>) on C++14+. The index parameter is a
* std::size_t.
*
*   STANDARD STATUS:
*   Introduced in C++17 alongside <variant>.
*
*
* path:      /inc/re_std/utility/in_place_index.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_IN_PLACE_INDEX_HPP
#define RE_STD_UTILITY_IN_PLACE_INDEX_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t

namespace re_std
{

// =============================================================================
// IN_PLACE_INDEX
// =============================================================================

// in_place_index_t
//   struct: tag type for index-tagged in-place construction.
template<std::size_t Index>
struct in_place_index_t
{
    explicit RE_STD_CONSTEXPR in_place_index_t() noexcept
    {}
};

// in_place_index
//   variable: template variable yielding a default-constructed
//   in_place_index_t<Index>.
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<std::size_t Index>
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline
    #endif
    RE_STD_CONSTEXPR in_place_index_t<Index> in_place_index{};

#endif

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_UTILITY_IN_PLACE_INDEX_HPP
