/******************************************************************************
* djinterp [re_std]                                             is_heap_until.hpp
*
* is_heap_until algorithm header:
*   Returns the largest prefix iterator end such that [_first, end) is
* a max-heap under operator< (or _comp). When the entire range is a
* heap, returns _last; when it isn't, returns the first iterator at
* which the heap property breaks (i.e., the first child that is
* strictly greater than its parent).
*
*   PORTABILITY:
*   - std::is_heap_until is C++11; re_std back-ports to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/is_heap_until.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ALGORITHM_IS_HEAP_UNTIL_
#define DJINTERP_RE_STD_ALGORITHM_IS_HEAP_UNTIL_ 1

// djinterp
#include "../../core/djinterp.hpp"
// re_std
#include "../iterator/iterator_traits.hpp"


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
// I.   IS_HEAP_UNTIL (DEFAULT operator<)
// ===========================================================================

// is_heap_until
//   function: walks indices i = 1..N-1 and returns _first + i for the
// first index whose parent (at (i - 1) / 2) compares less than it.
// Returns _last if every parent-child pair upholds the heap property.
template<typename _RandomIt>
D_CONSTEXPR_CPP14 _RandomIt
is_heap_until(
    _RandomIt _first,
    _RandomIt _last
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

    _Diff _length = _last - _first;
    for (_Diff _i = 1; _i < _length; ++_i)
    {
        _Diff _parent = (_i - 1) / 2;
        if (*(_first + _parent) < *(_first + _i))
        {
            return _first + _i;
        }
    }
    return _last;
}


// ===========================================================================
// II.  IS_HEAP_UNTIL (COMPARATOR)
// ===========================================================================

// is_heap_until (comparator)
//   function: as above but parent-child comparison is via _comp.
template<typename _RandomIt,
         typename _Compare>
D_CONSTEXPR_CPP14 _RandomIt
is_heap_until(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

    _Diff _length = _last - _first;
    for (_Diff _i = 1; _i < _length; ++_i)
    {
        _Diff _parent = (_i - 1) / 2;
        if (_comp(*(_first + _parent), *(_first + _i)))
        {
            return _first + _i;
        }
    }
    return _last;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ALGORITHM_IS_HEAP_UNTIL_
