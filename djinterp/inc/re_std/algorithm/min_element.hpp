/*******************************************************************************
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
* path:      /inc/re_std/algorithm/min_element.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MIN_ELEMENT_HPP
#define RE_STD_ALGORITHM_MIN_ELEMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MIN_ELEMENT (DEFAULT operator<)
// ===========================================================================

// min_element
//   function: iterator to the first smallest element, or _last when the
// range is empty.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 ForwardIt
min_element(
    ForwardIt _first,
    ForwardIt _last
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _smallest = _first;
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
template<typename ForwardIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 ForwardIt
min_element(
    ForwardIt _first,
    ForwardIt _last,
    Compare   _comp
)
{
    if (_first == _last)
    {
        return _last;
    }

    ForwardIt _smallest = _first;
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


}  // re_std


#endif  // RE_STD_ALGORITHM_MIN_ELEMENT_HPP
