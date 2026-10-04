/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/set_intersection.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_SET_INTERSECTION_
#define DJINTERP_RE_STD_ALGORITHM_SET_INTERSECTION_ 1

#include "../../core/djinterp.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   SET_INTERSECTION (DEFAULT operator<)
// ===========================================================================

// set_intersection
//   function: writes the multiset intersection into _d_first. Either
// range exhausting ends the merge.
template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
set_intersection(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _InputIt2 _last2,
    _OutputIt _d_first
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

template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _OutputIt
set_intersection(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _InputIt2 _last2,
    _OutputIt _d_first,
    _Compare  _comp
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_SET_INTERSECTION_
