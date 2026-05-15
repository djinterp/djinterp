/***********************************************************************
* restd                                                  in_place_type.hpp
*
* in_place_type_t<T> / in_place_type<T>:
*   tag types/values for selecting type-disambiguated overloads. Used
* mainly by variant<Ts...>: variant(in_place_type<T>, args...)
* unambiguously selects the alternative of type T.
*
* in_place_index_t<I> / in_place_index<I>:
*   the index-disambiguated variant. variant(in_place_index<2>, args...)
* selects the alternative at index 2.
*
*   The bare in_place_t / in_place tags ship under <optional>
* (restd/optional/in_place.hpp); the standard puts all of them in
* <utility>. We split because <optional> ships earlier in the
* dependency order. This file adds the typed and indexed variants
* needed by variant.
*
* added in std C++17.
*
*
* path:      /inc/restd/utility/in_place_type.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_IN_PLACE_TYPE_
#define RESTD_UTILITY_IN_PLACE_TYPE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>


namespace restd
{

// =====================================================================
// in_place_type_t<T> + in_place_type<T>
// =====================================================================

template<typename _T>
struct in_place_type_t
{
    explicit D_CONSTEXPR in_place_type_t() = default;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // Variable-template form (C++17+ in std). One instance per T.
    template<typename _T>
    D_CONSTEXPR in_place_type_t<_T> in_place_type{};
#endif


// =====================================================================
// in_place_index_t<I> + in_place_index<I>
// =====================================================================

template<std::size_t _I>
struct in_place_index_t
{
    explicit D_CONSTEXPR in_place_index_t() = default;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<std::size_t _I>
    D_CONSTEXPR in_place_index_t<_I> in_place_index{};
#endif


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_IN_PLACE_TYPE_
