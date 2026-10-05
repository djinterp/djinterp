/*******************************************************************************
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
* path:      /inc/re_std/algorithm/max_element.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MAX_ELEMENT_HPP
#define RE_STD_ALGORITHM_MAX_ELEMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MAX_ELEMENT (DEFAULT operator<)
// ===========================================================================

// max_element
//   function: iterator to the first largest element, or _last when the
// range is empty.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
max_element(
    ForwardIt _first,
    ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _largest = _first;
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
template<typename ForwardIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 ForwardIt
max_element(
    ForwardIt _first,
    ForwardIt _last,
    Compare   _comp
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _largest = _first;
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


}  // re_std


#endif  // RE_STD_ALGORITHM_MAX_ELEMENT_HPP
