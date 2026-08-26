/******************************************************************************
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
* path:      /inc/djinterp/re_std/algorithm/includes.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_INCLUDES_
#define DJINTERP_RE_STD_ALGORITHM_INCLUDES_ 1

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
// I.   INCLUDES (DEFAULT operator<)
// ===========================================================================

// includes
//   function: returns true if range2 is a multiset-subsequence of
// range1. Walks both ranges; if range1 exhausts before range2, the
// answer is false.
template<typename _InputIt1,
         typename _InputIt2>
D_CONSTEXPR_CPP14 bool
includes(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _InputIt2 _last2
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

template<typename _InputIt1,
         typename _InputIt2,
         typename _Compare>
D_CONSTEXPR_CPP14 bool
includes(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _InputIt2 _last2,
    _Compare  _comp
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


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_INCLUDES_
