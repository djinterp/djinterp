/******************************************************************************
* djinterp [restd]                                       pair_tuple_element.hpp
*
* tuple_element<I, pair> specialisation header:
*   Specialises restd::tuple_element so that:
*     tuple_element<0, pair<_T1, _T2> >::type -> _T1
*     tuple_element<1, pair<_T1, _T2> >::type -> _T2
*
*   The cv-qualified pass-through specialisations are inherited from
* tuple_element's primary partial specs in tuple/tuple_element.hpp,
* so tuple_element<0, const pair<int, char>>::type resolves to
* `const int`.
*
*   Indices outside {0, 1} are SFINAE-rejected (no matching partial
* specialisation), matching the std behaviour where the trait is
* ill-formed (no `type` member) for out-of-range indices.
*
*   PORTABILITY:
*   Same gate as tuple_element (C++11+ variadic templates).
*
*
* path:      /inc/djinterp/re_std/tuple/pair_tuple_element.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_UTILITY_PAIR_TUPLE_ELEMENT_
#define DJINTERP_RESTD_UTILITY_PAIR_TUPLE_ELEMENT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// std
#include <cstddef>
// djinterp
#include "./pair.hpp"
#include "../tuple/tuple_element.hpp"


NS_RESTD


// =============================================================================
// I.   TUPLE_ELEMENT<I, PAIR>
// =============================================================================

// tuple_element<0, pair<_T1, _T2>>
template<typename _T1,
         typename _T2>
struct tuple_element<0, pair<_T1, _T2> >
{
    typedef _T1 type;
};

// tuple_element<1, pair<_T1, _T2>>
template<typename _T1,
         typename _T2>
struct tuple_element<1, pair<_T1, _T2> >
{
    typedef _T2 type;
};


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_UTILITY_PAIR_TUPLE_ELEMENT_
