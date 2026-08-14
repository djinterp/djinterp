/******************************************************************************
* djinterp [restd]                                  set_symmetric_difference.hpp
*
* set_symmetric_difference algorithm header:
*   Multiset symmetric difference (range1 XOR range2). For each value,
* the output contains |n1 - n2| copies, drawn from whichever range has
* the surplus. Returns one past the last element written.
*
*   PORTABILITY:
*   - std::set_symmetric_difference is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/set_symmetric_difference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SET_SYMMETRIC_DIFFERENCE_
#define DJINTERP_RESTD_ALGORITHM_SET_SYMMETRIC_DIFFERENCE_ 1

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
// I.   SET_SYMMETRIC_DIFFERENCE (DEFAULT operator<)
// ===========================================================================

// set_symmetric_difference
//   function: writes elements that appear in exactly one of the two
// ranges (pairwise-cancelling equivalents). When one side exhausts,
// the remainder of the other is copied through.
template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
set_symmetric_difference(
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

template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _OutputIt
set_symmetric_difference(
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SET_SYMMETRIC_DIFFERENCE_
