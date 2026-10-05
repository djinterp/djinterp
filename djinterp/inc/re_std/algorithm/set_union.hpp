/*******************************************************************************
* djinterp [re_std]                                                set_union.hpp
*
* set_union algorithm header:
*   Multiset union of two sorted ranges into a third. For elements
* equivalent between the two ranges, the output contains max(n1, n2)
* copies (where n1, n2 are the counts in each input), preferring
* range-1 elements for the overlap. Returns one past the last element
* written.
*
*   PORTABILITY:
*   - std::set_union is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/set_union.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SET_UNION_HPP
#define RE_STD_ALGORITHM_SET_UNION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SET_UNION (DEFAULT operator<)
// ===========================================================================

// set_union
//   function: writes the multiset union into _d_first. When elements
// compare equivalent, the one from range1 is written and both inputs
// advance (the equivalent in range2 is discarded; multiset count is
// max(n1, n2) because the excess in the longer side flushes when the
// other range exhausts).
template<typename InputIt1,
         typename InputIt2,
         typename OutputIt>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_union(
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
        }
        else if (*_first2 < *_first1)
        {
            *_d_first = *_first2;
            ++_first2;
        }
        else
        {
            // equivalent: emit range1's copy, advance both
            *_d_first = *_first1;
            ++_first1;
            ++_first2;
        }
        ++_d_first;
    }

    // drain
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
// II.  SET_UNION (COMPARATOR)
// ===========================================================================

template<typename InputIt1,
         typename InputIt2,
         typename OutputIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 OutputIt
set_union(
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
        }
        else if (_comp(*_first2, *_first1))
        {
            *_d_first = *_first2;
            ++_first2;
        }
        else
        {
            *_d_first = *_first1;
            ++_first1;
            ++_first2;
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


#endif  // RE_STD_ALGORITHM_SET_UNION_HPP
