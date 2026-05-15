/******************************************************************************
* djinterp [restd]                                                  search_n.hpp
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
*   - Uses operator< on _count for the counted loop; _Size needs only
*     integral-like comparison-with-zero and decrement (or, here,
*     incrementing a running counter).
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/restd/algorithm/search_n.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SEARCH_N_
#define DJINTERP_RESTD_ALGORITHM_SEARCH_N_ 1

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
// I.   SEARCH_N (DEFAULT ==)
// ===========================================================================

// search_n
//   function: returns an iterator to the first element of the first
// run of _count consecutive elements in [_first, _last) equal to
// _value via operator==. Returns _first for _count <= 0 (per LWG 426)
// and _last on no-match.
template<typename _ForwardIt,
         typename _Size,
         typename _Type>
D_CONSTEXPR_CPP14 _ForwardIt
search_n(
    _ForwardIt   _first,
    _ForwardIt   _last,
    _Size        _count,
    const _Type& _value
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
        _ForwardIt _candidate = _first;
        _Size      _matched   = 1;

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
template<typename _ForwardIt,
         typename _Size,
         typename _Type,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _ForwardIt
search_n(
    _ForwardIt   _first,
    _ForwardIt   _last,
    _Size        _count,
    const _Type& _value,
    _BinaryPred  _pred
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

        _ForwardIt _candidate = _first;
        _Size      _matched   = 1;

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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SEARCH_N_
