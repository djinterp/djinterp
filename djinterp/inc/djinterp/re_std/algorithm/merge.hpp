/******************************************************************************
* djinterp [restd]                                                     merge.hpp
*
* merge algorithm header:
*   Merges two sorted input ranges into a single sorted output range.
* Stable: when an element of the first range compares equivalent to an
* element of the second range, the first-range element is written
* first. Returns the iterator one past the last element written.
*
*   PORTABILITY:
*   - std::merge is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - Stability hinges on a NON-strict comparison test: write *first2
*     only when it is STRICTLY less than *first1; otherwise write
*     *first1 (preserves left-before-right for equivalents).
*
*
* path:      /inc/djinterp/re_std/algorithm/merge.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_MERGE_
#define DJINTERP_RESTD_ALGORITHM_MERGE_ 1

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
// I.   MERGE (DEFAULT operator<)
// ===========================================================================

// merge
//   function: merges sorted [_first1, _last1) and [_first2, _last2)
// into _d_first. Returns one past the last element written.
template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
merge(
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

template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _OutputIt
merge(
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_MERGE_
