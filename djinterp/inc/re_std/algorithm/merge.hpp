/*******************************************************************************
* djinterp [re_std]                                                    merge.hpp
*
* merge algorithm header:
*   Merges two sorted input ranges into a single sorted output range.
* Stable: when an element of the first range compares equivalent to an
* element of the second range, the first-range element is written
* first. Returns the iterator one past the last element written.
*
*   PORTABILITY:
*   - std::merge is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - Stability hinges on a NON-strict comparison test: write *first2
*     only when it is STRICTLY less than *first1; otherwise write
*     *first1 (preserves left-before-right for equivalents).
*
*
* path:      /inc/re_std/algorithm/merge.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MERGE_HPP
#define RE_STD_ALGORITHM_MERGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MERGE (DEFAULT operator<)
// ===========================================================================

// merge
//   function: merges sorted [_first1, _last1) and [_first2, _last2)
// into _d_first. Returns one past the last element written.
template<typename InputIt1,
         typename InputIt2,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
merge(
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
        if (*_first2 < *_first1)
        {
            *_d_first = *_first2;
            ++_first2;
        }
        else
        {
            *_d_first = *_first1;
            ++_first1;
        }
        ++_d_first;
    }

    // drain whichever range is non-empty
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
// II.  MERGE (COMPARATOR)
// ===========================================================================

template<typename InputIt1,
         typename InputIt2,
         typename OutputIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 OutputIt
merge(
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
        if (_comp(*_first2, *_first1))
        {
            *_d_first = *_first2;
            ++_first2;
        }
        else
        {
            *_d_first = *_first1;
            ++_first1;
        }
        ++_d_first;
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


#endif  // RE_STD_ALGORITHM_MERGE_HPP
