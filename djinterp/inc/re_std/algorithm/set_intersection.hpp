/*******************************************************************************
* djinterp [re_std]                                         set_intersection.hpp
*
* set_intersection algorithm header:
*   Multiset intersection of two sorted ranges. For each value, the
* output contains min(n1, n2) copies. Equivalent elements written come
* from range1 (preserves range1's identity for stable downstream
* processing). Returns one past the last element written.
*
*   PORTABILITY:
*   - std::set_intersection is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/set_intersection.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SET_INTERSECTION_HPP
#define RE_STD_ALGORITHM_SET_INTERSECTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SET_INTERSECTION (DEFAULT operator<)
// ===========================================================================

// set_intersection
//   function: writes the multiset intersection into _d_first. Either
// range exhausting ends the merge.
template<typename InputIt1,
         typename InputIt2,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_intersection(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2,
    OutputIt _d_first
)
{
    while ( (_first1 != _last1) &&
            (_first2 != _last2) )
    {
        if (*_first1 < *_first2)
        {
            ++_first1;
        }
        else if (*_first2 < *_first1)
        {
            ++_first2;
        }
        else
        {
            *_d_first = *_first1;
            ++_first1;
            ++_first2;
            ++_d_first;
        }
    }
    return _d_first;
}


// ===========================================================================
// II.  SET_INTERSECTION (COMPARATOR)
// ===========================================================================

template<typename InputIt1,
         typename InputIt2,
         typename OutputIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_intersection(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2,
    OutputIt _d_first,
    Compare  _comp
)
{
    while ( (_first1 != _last1) &&
            (_first2 != _last2) )
    {
        if (_comp(*_first1, *_first2))
        {
            ++_first1;
        }
        else if (_comp(*_first2, *_first1))
        {
            ++_first2;
        }
        else
        {
            *_d_first = *_first1;
            ++_first1;
            ++_first2;
            ++_d_first;
        }
    }
    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SET_INTERSECTION_HPP
