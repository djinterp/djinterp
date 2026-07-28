/******************************************************************************
* djinterp [restd]                                          pair_tuple_size.hpp
*
* tuple_size<pair> specialisation header:
*   Specialises restd::tuple_size for restd::pair so that
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
* path:      /inc/djinterp/restd/utility/pair_tuple_size.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_UTILITY_PAIR_TUPLE_SIZE_
#define DJINTERP_RESTD_UTILITY_PAIR_TUPLE_SIZE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: requires variadic templates (matches tuple_size)
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// std
#include <cstddef>
// djinterp
#include "./pair.hpp"
#include "../tuple/tuple_size.hpp"
#include "../type_traits/integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   TUPLE_SIZE<PAIR>
// =============================================================================

// tuple_size<pair<_T1, _T2>>
//   trait: pair has fixed arity 2.
template<typename _T1,
         typename _T2>
struct tuple_size<pair<_T1, _T2> >
    : integral_constant<std::size_t, 2>
{};


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_UTILITY_PAIR_TUPLE_SIZE_
