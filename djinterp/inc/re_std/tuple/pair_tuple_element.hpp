/*******************************************************************************
* djinterp [re_std]                                       pair_tuple_element.hpp
*
* tuple_element<I, pair> specialisation header:
*   Specialises re_std::tuple_element so that:
*     tuple_element<0, pair<T1, T2> >::type -> T1
*     tuple_element<1, pair<T1, T2> >::type -> T2
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
* path:      /inc/re_std/tuple/pair_tuple_element.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_PAIR_TUPLE_ELEMENT_HPP
#define RE_STD_TUPLE_PAIR_TUPLE_ELEMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// std
#include <cstddef>
// re_std
#include "../utility/pair.hpp"
#include "tuple_element.hpp"


namespace re_std
{


// =============================================================================
// I.   TUPLE_ELEMENT<I, PAIR>
// =============================================================================

// tuple_element<0, pair<T1, T2>>
template<typename T1,
         typename T2>
struct tuple_element<0, pair<T1, T2> >
{
    typedef T1 type;
};

// tuple_element<1, pair<T1, T2>>
template<typename T1,
         typename T2>
struct tuple_element<1, pair<T1, T2> >
{
    typedef T2 type;
};


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TUPLE_PAIR_TUPLE_ELEMENT_HPP
