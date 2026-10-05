/*******************************************************************************
* djinterp [re_std]                                           set_difference.hpp
*
* set_difference algorithm header:
*   Multiset difference range1 - range2. For each value, the output
* contains max(0, n1 - n2) copies. All such copies come from range1.
* Returns one past the last element written.
*
*   PORTABILITY:
*   - std::set_difference is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/set_difference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SET_DIFFERENCE_HPP
#define RE_STD_ALGORITHM_SET_DIFFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SET_DIFFERENCE (DEFAULT operator<)
// ===========================================================================

// set_difference
//   function: writes range1 - range2 into _d_first. Elements smaller
// than the current range2 head are output; equivalents on both sides
// cancel pairwise; range2 elements smaller than range1's head are
// skipped. Once range2 exhausts, any remaining range1 elements are
// copied verbatim.
template<typename InputIt1,
         typename InputIt2,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_difference(
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
            *_d_first = *_first1;
            ++_first1;
            ++_d_first;
        }
        else if (*_first2 < *_first1)
        {
            ++_first2;
        }
        else
        {
            // equivalent: cancel pairwise
            ++_first1;
            ++_first2;
        }
    }

    // drain remainder of range1
    while (_first1 != _last1)
    {
        *_d_first = *_first1;
        ++_first1;
        ++_d_first;
    }
    return _d_first;
}


// ===========================================================================
// II.  SET_DIFFERENCE (COMPARATOR)
// ===========================================================================

template<typename InputIt1,
         typename InputIt2,
         typename OutputIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_difference(
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
            *_d_first = *_first1;
            ++_first1;
            ++_d_first;
        }
        else if (_comp(*_first2, *_first1))
        {
            ++_first2;
        }
        else
        {
            ++_first1;
            ++_first2;
        }
    }

    while (_first1 != _last1)
    {
        *_d_first = *_first1;
        ++_first1;
        ++_d_first;
    }
    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SET_DIFFERENCE_HPP
