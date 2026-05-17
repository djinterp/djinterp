/******************************************************************************
* djinterp [restd]                                            set_difference.hpp
*
* set_difference algorithm header:
*   Multiset difference range1 - range2. For each value, the output
* contains max(0, n1 - n2) copies. All such copies come from range1.
* Returns one past the last element written.
*
*   PORTABILITY:
*   - std::set_difference is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/restd/algorithm/set_difference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SET_DIFFERENCE_
#define DJINTERP_RESTD_ALGORITHM_SET_DIFFERENCE_ 1

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
// I.   SET_DIFFERENCE (DEFAULT operator<)
// ===========================================================================

// set_difference
//   function: writes range1 - range2 into _d_first. Elements smaller
// than the current range2 head are output; equivalents on both sides
// cancel pairwise; range2 elements smaller than range1's head are
// skipped. Once range2 exhausts, any remaining range1 elements are
// copied verbatim.
template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
set_difference(
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

template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _OutputIt
set_difference(
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SET_DIFFERENCE_
