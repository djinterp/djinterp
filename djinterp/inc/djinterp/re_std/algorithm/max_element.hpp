/******************************************************************************
* djinterp [re_std]                                              max_element.hpp
*
* max_element algorithm header:
*   Returns an iterator to the largest element in [_first, _last),
* or _last if the range is empty.
*
*   PORTABILITY:
*   - std::max_element is C++98.
*   - constexpr in std from C++17 (P0202); re_std lifts to C++14.
*   - Two overloads: default operator< and custom comparator.
*   - The test is `*_largest < *_first`, i.e. the incumbent on the
*     LEFT, so an equal element does not displace it and the FIRST
*     occurrence of the maximum is returned.
*
*
* path:      /inc/djinterp/re_std/algorithm/max_element.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.24
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_MAX_ELEMENT_
#define DJINTERP_RE_STD_ALGORITHM_MAX_ELEMENT_ 1

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
// I.   MAX_ELEMENT (DEFAULT operator<)
// ===========================================================================

// max_element
//   function: iterator to the first largest element, or _last when the
// range is empty.
template<typename _ForwardIt>
D_CONSTEXPR_CPP14 _ForwardIt
max_element(
    _ForwardIt _first,
    _ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _largest = _first;
    ++_first;

    for (; _first != _last; ++_first)
    {
        // incumbent on the left: only a STRICTLY greater candidate
        // displaces it, so the first occurrence wins.
        if (*_largest < *_first)
        {
            _largest = _first;
        }
    }
    return _largest;
}


// ===========================================================================
// II.  MAX_ELEMENT (COMPARATOR)
// ===========================================================================

// max_element (comparator)
//   function: as above but ordering is decided by _comp.
template<typename _ForwardIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _ForwardIt
max_element(
    _ForwardIt _first,
    _ForwardIt _last,
    _Compare   _comp
)
{
    if (_first == _last)
    {
        return _last;
    }

    _ForwardIt _largest = _first;
    ++_first;

    for (; _first != _last; ++_first)
    {
        if (_comp(*_largest, *_first))
        {
            _largest = _first;
        }
    }
    return _largest;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_MAX_ELEMENT_
