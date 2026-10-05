/*******************************************************************************
* djinterp [re_std]                                            in_place_type.hpp
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
* gated on RE_STD_LANG_HAS_VARIABLE_TEMPLATES; on C++11 only
* the type is available, and callers must construct in_place_type_t<T>{}
* explicitly.
*
*   STANDARD STATUS:
*   Introduced in C++17 alongside <variant>. re_std back-ports to C++11+
* since variant itself is planned at C++11+.
*
*
* path:      /inc/re_std/utility/in_place_type.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_IN_PLACE_TYPE_HPP
#define RE_STD_UTILITY_IN_PLACE_TYPE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

namespace re_std
{

// =============================================================================
// IN_PLACE_TYPE
// =============================================================================

// in_place_type_t
//   struct: tag type for type-tagged in-place construction. Has an
//   explicit constexpr default constructor so brace-initialisation of
//   a variant from {} cannot accidentally select an in_place_type_t
//   constructor.
template<typename Type>
struct in_place_type_t
{
    explicit RE_STD_CONSTEXPR in_place_type_t() noexcept
    {}
};

// in_place_type
//   variable: template variable yielding a default-constructed
//   in_place_type_t<Type>. Inline on C++17+ for single-instance
//   linkage; on C++14 each TU gets its own copy (harmless because
//   in_place_type_t is stateless).
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline
    #endif
    RE_STD_CONSTEXPR in_place_type_t<Type> in_place_type{};

#endif

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_UTILITY_IN_PLACE_TYPE_HPP
