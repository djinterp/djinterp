/*******************************************************************************
* djinterp [re_std]                                 set_symmetric_difference.hpp
*
* set_symmetric_difference algorithm header:
*   Multiset symmetric difference (range1 XOR range2). For each value,
* the output contains |n1 - n2| copies, drawn from whichever range has
* the surplus. Returns one past the last element written.
*
*   PORTABILITY:
*   - std::set_symmetric_difference is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/set_symmetric_difference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SET_SYMMETRIC_DIFFERENCE_HPP
#define RE_STD_ALGORITHM_SET_SYMMETRIC_DIFFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SET_SYMMETRIC_DIFFERENCE (DEFAULT operator<)
// ===========================================================================

// set_symmetric_difference
//   function: writes elements that appear in exactly one of the two
// ranges (pairwise-cancelling equivalents). When one side exhausts,
// the remainder of the other is copied through.
template<typename InputIt1,
         typename InputIt2,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_symmetric_difference(
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
            *_d_first = *_first2;
            ++_first2;
            ++_d_first;
        }
        else
        {
            ++_first1;
            ++_first2;
        }
    }

    // drain whichever side still has elements
    while (_first1 != _last1)
    {
        *_d_first = *_first1;
        ++_first1;
        ++_d_first;
    }
    while (_first2 != _last2)
    {
        *_d_first = *_first2;
        ++_first2;
        ++_d_first;
    }
    return _d_first;
}


// ===========================================================================
// II.  SET_SYMMETRIC_DIFFERENCE (COMPARATOR)
// ===========================================================================

template<typename InputIt1,
         typename InputIt2,
         typename OutputIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_symmetric_difference(
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
            *_d_first = *_first2;
            ++_first2;
            ++_d_first;
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
    while (_first2 != _last2)
    {
        *_d_first = *_first2;
        ++_first2;
        ++_d_first;
    }
    return _d_first;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SET_SYMMETRIC_DIFFERENCE_HPP
