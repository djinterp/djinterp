/******************************************************************************
* djinterp [restd]                                                 set_union.hpp
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
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/restd/algorithm/set_union.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SET_UNION_
#define DJINTERP_RESTD_ALGORITHM_SET_UNION_ 1

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
// I.   SET_UNION (DEFAULT operator<)
// ===========================================================================

// set_union
//   function: writes the multiset union into _d_first. When elements
// compare equivalent, the one from range1 is written and both inputs
// advance (the equivalent in range2 is discarded; multiset count is
// max(n1, n2) because the excess in the longer side flushes when the
// other range exhausts).
template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
set_union(
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

template<typename _InputIt1,
         typename _InputIt2,
         typename _OutputIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _OutputIt
set_union(
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SET_UNION_
