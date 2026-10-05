/*******************************************************************************
* djinterp [re_std]                                                 includes.hpp
*
* includes algorithm header:
*   Returns true if every element of the sorted range [_first2, _last2)
* appears as a SUBSEQUENCE of the sorted range [_first1, _last1), with
* multiset semantics — each element of the second range must be
* matched by a distinct element of the first. Both ranges must be
* sorted under operator< (or _comp).
*
*   PORTABILITY:
*   - std::includes is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/includes.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_INCLUDES_HPP
#define RE_STD_ALGORITHM_INCLUDES_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   INCLUDES (DEFAULT operator<)
// ===========================================================================

// includes
//   function: returns true if range2 is a multiset-subsequence of
// range1. Walks both ranges; if range1 exhausts before range2, the
// answer is false.
template<typename InputIt1,
         typename InputIt2>
RE_STD_CONSTEXPR_CPP14 bool
includes(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2
)
{
    while (_first2 != _last2)
    {
        if (_first1 == _last1)
        {
            // range1 exhausted with elements remaining in range2
            return false;
        }
        if (*_first2 < *_first1)
        {
            // range2 has an element smaller than anything left in range1
            return false;
        }
        if (!(*_first1 < *_first2))
        {
            // equivalent: consume one from each
            ++_first2;
        }
        ++_first1;
    }
    return true;
}


// ===========================================================================
// II.  INCLUDES (COMPARATOR)
// ===========================================================================

template<typename InputIt1,
         typename InputIt2,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 bool
includes(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2,
    Compare  _comp
)
{
    while (_first2 != _last2)
    {
        if (_first1 == _last1)
        {
            return false;
        }
        if (_comp(*_first2, *_first1))
        {
            return false;
        }
        if (!_comp(*_first1, *_first2))
        {
            ++_first2;
        }
        ++_first1;
    }
    return true;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_INCLUDES_HPP
