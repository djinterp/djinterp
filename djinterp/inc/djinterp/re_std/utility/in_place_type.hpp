/***********************************************************************
* restd                                                 in_place_type.hpp
*
* in_place_type tag type and variable:
*   Disambiguating tag for type-tagged in-place construction in
* variant<Ts...>, expected<T,E> (for unexpected<E>::unexpected<U>(in_place_type<U>, ...))
* and similar containers. Where in_place_t (in optional/in_place.hpp)
* selects the held type implicitly, in_place_type<T> selects an explicit
* T from a variant's alternative list.
*
*   Provided as a class template (in_place_type_t<T>) plus a variable
* template (in_place_type<T>) on C++14+. The variable template is
* gated on D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES; on C++11 only
* the type is available, and callers must construct in_place_type_t<T>{}
* explicitly.
*
*   STANDARD STATUS:
*   Introduced in C++17 alongside <variant>. restd back-ports to C++11+
* since variant itself is planned at C++11+.
*
*
* path:      /inc/djinterp/re_std/utility/in_place_type.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_IN_PLACE_TYPE_
#define RESTD_UTILITY_IN_PLACE_TYPE_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_RESTD

// =============================================================================
// IN_PLACE_TYPE
// =============================================================================

// in_place_type_t
//   struct: tag type for type-tagged in-place construction. Has an
//   explicit constexpr default constructor so brace-initialisation of
//   a variant from {} cannot accidentally select an in_place_type_t
//   constructor.
template<typename _Type>
struct in_place_type_t
{
    explicit D_CONSTEXPR in_place_type_t() noexcept
    {}
};

// in_place_type
//   variable: template variable yielding a default-constructed
//   in_place_type_t<_Type>. Inline on C++17+ for single-instance
//   linkage; on C++14 each TU gets its own copy (harmless because
//   in_place_type_t is stateless).
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline
    #endif
    D_CONSTEXPR in_place_type_t<_Type> in_place_type{};

#endif

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_IN_PLACE_TYPE_
