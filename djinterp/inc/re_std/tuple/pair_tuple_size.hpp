/*******************************************************************************
* djinterp [re_std]                                          pair_tuple_size.hpp
*
* tuple_size<pair> specialisation header:
*   Specialises re_std::tuple_size for re_std::pair so that
* tuple_size<pair<T1, T2>>::value == 2. This enables structured
* bindings on pair (auto [a, b] = somepair) and lets pair flow through
* generic tuple-protocol code such as apply, make_from_tuple, and the
* tuple-protocol bindings on <ranges> views (elements_view, enumerate
* etc., once they ship).
*
*   The cv-qualified passthrough specialisations are inherited from
* the primary tuple_size partial specs in tuple/tuple_size.hpp, so
* tuple_size<const pair<T1, T2>>::value == 2 also resolves correctly.
*
*   PORTABILITY:
*   Requires the same minimum as tuple_size itself (variadic templates,
* C++11+). Header expands to nothing on C++98/03.
*
*
* path:      /inc/re_std/tuple/pair_tuple_size.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_PAIR_TUPLE_SIZE_HPP
#define RE_STD_TUPLE_PAIR_TUPLE_SIZE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: requires variadic templates (matches tuple_size)
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// std
#include <cstddef>
// re_std
#include "../utility/pair.hpp"
#include "tuple_size.hpp"
#include "../type_traits/integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   TUPLE_SIZE<PAIR>
// =============================================================================

// tuple_size<pair<T1, T2>>
//   trait: pair has fixed arity 2.
template<typename T1,
         typename T2>
struct tuple_size<pair<T1, T2> >
    : integral_constant<std::size_t, 2>
{};


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TUPLE_PAIR_TUPLE_SIZE_HPP
