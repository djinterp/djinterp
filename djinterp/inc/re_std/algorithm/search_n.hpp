/*******************************************************************************
* djinterp [re_std]                                                 search_n.hpp
*
* search_n algorithm header:
*   Returns an iterator to the first element of the first run of
* _count consecutive elements in [_first, _last) that compare equal to
* _value (or for which _pred(*it, _value) holds). Two overloads:
*   - default operator==
*   - custom binary predicate
*
*   PORTABILITY:
*   - std::search_n is C++98.
*   - _count <= 0 returns _first per LWG 426 (matches every modern
*     standard library).
*   - Uses operator< on _count for the counted loop; Size needs only
*     integral-like comparison-with-zero and decrement (or, here,
*     incrementing a running counter).
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/search_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_SEARCH_N_HPP
#define RE_STD_ALGORITHM_SEARCH_N_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   SEARCH_N (DEFAULT ==)
// ===========================================================================

// search_n
//   function: returns an iterator to the first element of the first
// run of _count consecutive elements in [_first, _last) equal to
// _value via operator==. Returns _first for _count <= 0 (per LWG 426)
// and _last on no-match.
template<typename ForwardIt,
         typename Size,
         typename Type>
RE_STD_CONSTEXPR_CPP14 ForwardIt
search_n(
    ForwardIt   _first,
    ForwardIt   _last,
    Size        _count,
    const Type& _value
)
{
    // LWG 426: count <= 0 -> return first unchanged
    if (_count <= 0)
    {
        return _first;
    }

    while (_first != _last)
    {
        if (!(*_first == _value))
        {
            ++_first;
            continue;
        }

        // found a candidate run; try to extend to _count
        ForwardIt _candidate = _first;
        Size      _matched   = 1;

        ++_first;

        while (true)
        {
            if (_matched >= _count)
            {
                return _candidate;
            }
            if (_first == _last)
            {
                return _last;
            }
            if (!(*_first == _value))
            {
                break;
            }
            ++_first;
            ++_matched;
        }
    }

    return _last;
}


// ===========================================================================
// II.  SEARCH_N (CUSTOM PRED)
// ===========================================================================

// search_n (predicate)
//   function: as above but each element is compared to _value via the
// user-supplied binary predicate _pred(elem, _value).
template<typename ForwardIt,
         typename Size,
         typename Type,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 ForwardIt
search_n(
    ForwardIt   _first,
    ForwardIt   _last,
    Size        _count,
    const Type& _value,
    BinaryPred  _pred
)
{
    if (_count <= 0)
    {
        return _first;
    }

    while (_first != _last)
    {
        if (!_pred(*_first, _value))
        {
            ++_first;
            continue;
        }

        ForwardIt _candidate = _first;
        Size      _matched   = 1;

        ++_first;

        while (true)
        {
            if (_matched >= _count)
            {
                return _candidate;
            }
            if (_first == _last)
            {
                return _last;
            }
            if (!_pred(*_first, _value))
            {
                break;
            }
            ++_first;
            ++_matched;
        }
    }

    return _last;
}


}  // re_std


#endif  // RE_STD_ALGORITHM_SEARCH_N_HPP
