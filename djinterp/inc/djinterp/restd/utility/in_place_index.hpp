/***********************************************************************
* restd                                                in_place_index.hpp
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
* path:      /inc/restd/utility/in_place_index.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_IN_PLACE_INDEX_
#define RESTD_UTILITY_IN_PLACE_INDEX_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // std::size_t

NS_RESTD

// =============================================================================
// IN_PLACE_INDEX
// =============================================================================

// in_place_index_t
//   struct: tag type for index-tagged in-place construction.
template<std::size_t _Index>
struct in_place_index_t
{
    explicit D_CONSTEXPR in_place_index_t() noexcept
    {}
};

// in_place_index
//   variable: template variable yielding a default-constructed
//   in_place_index_t<_Index>.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<std::size_t _Index>
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline
    #endif
    D_CONSTEXPR in_place_index_t<_Index> in_place_index{};

#endif

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_IN_PLACE_INDEX_
