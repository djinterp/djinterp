/******************************************************************************
* djinterp [re_std]                                              min_element.hpp
*
* min_element algorithm header:
*   Returns an iterator to the smallest element in [_first, _last),
* or _last if the range is empty.
*
*   PORTABILITY:
*   - std::min_element is C++98.
*   - constexpr in std from C++17 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - The update test is STRICTLY less, so on a tie the FIRST
*     occurrence is retained, as the standard requires.
*
*
* path:      /inc/djinterp/re_std/algorithm/min_element.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.24
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_MIN_ELEMENT_
#define DJINTERP_RE_STD_ALGORITHM_MIN_ELEMENT_ 1

// djinterp
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
// I.   MIN_ELEMENT (DEFAULT operator<)
// ===========================================================================

// min_element
//   function: iterator to the first smallest element, or _last when the
// range is empty.
template<typename _ForwardIt>
D_CONSTEXPR_CPP14 _ForwardIt
min_element(
    _ForwardIt _first,
    _ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _smallest = _first;
    ++_first;

    for (; _first != _last; ++_first)
    {
        // strictly-less: ties leave _smallest alone, so the first
        // occurrence wins.
        if (*_first < *_smallest)
        {
            _smallest = _first;
        }
    }
    return _smallest;
}


// ===========================================================================
// II.  MIN_ELEMENT (COMPARATOR)
// ===========================================================================

// min_element (comparator)
//   function: as above but ordering is decided by _comp.
template<typename _ForwardIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _ForwardIt
min_element(
    _ForwardIt _first,
    _ForwardIt _last,
    _Compare   _comp
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _smallest = _first;
    ++_first;

    for (; _first != _last; ++_first)
    {
        if (_comp(*_first, *_smallest))
        {
            _smallest = _first;
        }
    }
    return _smallest;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_MIN_ELEMENT_
