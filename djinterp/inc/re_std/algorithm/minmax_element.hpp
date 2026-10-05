/*******************************************************************************
* djinterp [re_std]                                           minmax_element.hpp
*
* minmax_element algorithm header:
*   Returns pair(min_it, max_it) for [_first, _last) in a single pass
* costing at most 3N/2 comparisons, versus the 2N of calling
* min_element and max_element separately.
*
*   TIE-BREAKING IS ASYMMETRIC, and deliberately so:
*     min -> FIRST occurrence      (test: *cand <  *min)
*     max -> LAST  occurrence      (test: !(*cand < *max))
*   The standard mandates this precisely so that minmax_element is
* distinguishable from the pair (min_element, max_element), whose max
* is the first occurrence.
*
*   PORTABILITY:
*   - std::minmax_element is C++11; re_std back-ports it to C++98.
*   - constexpr in std from C++17 (P0202); re_std lifts to C++14.
*   - Elements are consumed in pairs: the two are compared to each
*     other first (1 comparison), then the smaller is tried against
*     the running min and the larger against the running max (2 more),
*     giving 3 comparisons per 2 elements.
*
*
* path:      /inc/re_std/algorithm/minmax_element.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MINMAX_ELEMENT_HPP
#define RE_STD_ALGORITHM_MINMAX_ELEMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../utility/pair.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MINMAX_ELEMENT (DEFAULT operator<)
// ===========================================================================

// minmax_element
//   function: pair(first-smallest, last-largest). Both members are
// _last when the range is empty.
template<typename ForwardIt>
RE_STD_CONSTEXPR_CPP14 pair<ForwardIt, ForwardIt>
minmax_element(
    ForwardIt _first,
    ForwardIt _last
)
{
    ForwardIt _min = _first;
    ForwardIt _max = _first;

    if (_first == _last)
    {
        return pair<ForwardIt, ForwardIt>(_last, _last);
    }

    ++_first;
    if (_first == _last)
    {
        return pair<ForwardIt, ForwardIt>(_min, _max);
    }

    // second element: equal goes to _max, keeping max at the later index
    if (*_first < *_min)
    {
        _min = _first;
    }
    else
    {
        _max = _first;
    }
    ++_first;

    while (_first != _last)
    {
        ForwardIt _lhs = _first;
        ++_first;

        if (_first == _last)
        {
            // odd tail element
            if (*_lhs < *_min)
            {
                _min = _lhs;
            }
            else if (!(*_lhs < *_max))
            {
                _max = _lhs;
            }
            break;
        }

        // compare the pair to each other first, then one against each
        // running extreme -- 3 comparisons per 2 elements.
        if (*_first < *_lhs)
        {
            if (*_first < *_min)
            {
                _min = _first;
            }
            if (!(*_lhs < *_max))
            {
                _max = _lhs;
            }
        }
        else
        {
            if (*_lhs < *_min)
            {
                _min = _lhs;
            }
            if (!(*_first < *_max))
            {
                _max = _first;
            }
        }
        ++_first;
    }

    return pair<ForwardIt, ForwardIt>(_min, _max);
}


// ===========================================================================
// II.  MINMAX_ELEMENT (COMPARATOR)
// ===========================================================================

// minmax_element (comparator)
//   function: as above but ordering is decided by _comp.
template<typename ForwardIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 pair<ForwardIt, ForwardIt>
minmax_element(
    ForwardIt _first,
    ForwardIt _last,
    Compare   _comp
)
{
    ForwardIt _min = _first;
    ForwardIt _max = _first;

    if (_first == _last)
    {
        return pair<ForwardIt, ForwardIt>(_last, _last);
    }

    ++_first;
    if (_first == _last)
    {
        return pair<ForwardIt, ForwardIt>(_min, _max);
    }

    if (_comp(*_first, *_min))
    {
        _min = _first;
    }
    else
    {
        _max = _first;
    }
    ++_first;

    while (_first != _last)
    {
        ForwardIt _lhs = _first;
        ++_first;

        if (_first == _last)
        {
            if (_comp(*_lhs, *_min))
            {
                _min = _lhs;
            }
            else if (!_comp(*_lhs, *_max))
            {
                _max = _lhs;
            }
            break;
        }

        if (_comp(*_first, *_lhs))
        {
            if (_comp(*_first, *_min))
            {
                _min = _first;
            }
            if (!_comp(*_lhs, *_max))
            {
                _max = _lhs;
            }
        }
        else
        {
            if (_comp(*_lhs, *_min))
            {
                _min = _lhs;
            }
            if (!_comp(*_first, *_max))
            {
                _max = _first;
            }
        }
        ++_first;
    }

    return pair<ForwardIt, ForwardIt>(_min, _max);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_MINMAX_ELEMENT_HPP
