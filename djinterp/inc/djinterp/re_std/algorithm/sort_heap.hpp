/******************************************************************************
* djinterp [restd]                                                 sort_heap.hpp
*
* sort_heap algorithm header:
*   Converts the max-heap [_first, _last) into a non-descending sorted
* range. Repeatedly pops the heap's max to the end of the shrinking
* heap; O(N log N).
*
*   PORTABILITY:
*   - std::sort_heap is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*   - Inlines a private _sift_down_ rather than depending on pop_heap
*     to avoid the per-pop range-length recheck and to keep
*     translation-unit dependencies orthogonal. The two implementations
*     are observably equivalent.
*
*
* path:      /inc/djinterp/restd/algorithm/sort_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_SORT_HEAP_
#define DJINTERP_RESTD_ALGORITHM_SORT_HEAP_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"


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
// 1.   INTERNAL: SIFT-DOWN
// ===========================================================================

// _sort_heap_sift_down_
//   max-heap sift-down. Identical in shape to the helpers in the
// other heap files; duplicated to keep this file standalone.
template<typename _RandomIt,
         typename _Distance,
         typename _Compare>
D_CONSTEXPR_CPP14 void
_sort_heap_sift_down_(
    _RandomIt _first,
    _Distance _start,
    _Distance _length,
    _Compare  _comp
)
{
    _Distance _parent = _start;
    while (true)
    {
        _Distance _child = static_cast<_Distance>(2 * _parent + 1);
        if (_child >= _length)
        {
            break;
        }
        if ( ((_child + 1) < _length) &&
             _comp(*(_first + _child), *(_first + _child + 1)) )
        {
            ++_child;
        }
        if (!_comp(*(_first + _parent), *(_first + _child)))
        {
            break;
        }
        iter_swap(_first + _parent, _first + _child);
        _parent = _child;
    }
}


// ===========================================================================
// I.   SORT_HEAP
// ===========================================================================

// sort_heap (comparator)
//   function: converts the max-heap [_first, _last) into a sorted
// range. Each iteration: swap root with last-active, decrement the
// active size, sift the new root down. After N - 1 iterations the
// range is non-descending under _comp.
template<typename _RandomIt,
         typename _Compare>
D_CONSTEXPR_CPP14 void
sort_heap(
    _RandomIt _first,
    _RandomIt _last,
    _Compare  _comp
)
{
    typedef typename iterator_traits<_RandomIt>::difference_type _Diff;

    _Diff _length = _last - _first;
    if (_length < 2)
    {
        return;
    }

    for (_Diff _i = _length - 1; _i > 0; --_i)
    {
        iter_swap(_first, _first + _i);
        _sort_heap_sift_down_<_RandomIt, _Diff, _Compare>(
            _first, static_cast<_Diff>(0), _i, _comp);
    }
}


// sort_heap (default operator<)
//   function: as above with restd::less<value_type>().
template<typename _RandomIt>
D_CONSTEXPR_CPP14 void
sort_heap(
    _RandomIt _first,
    _RandomIt _last
)
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    sort_heap(_first, _last, restd::less<_Value>());
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_SORT_HEAP_
