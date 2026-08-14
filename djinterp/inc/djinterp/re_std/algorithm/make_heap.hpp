/******************************************************************************
* djinterp [restd]                                                 make_heap.hpp
*
* make_heap algorithm header:
*   Rearranges [_first, _last) into a max-heap per operator< (or _comp).
* Uses the bottom-up sift-down construction (Floyd's algorithm), which
* is O(N) — strictly faster than N successive push_heap calls (O(N log
* N)).
*
*   PORTABILITY:
*   - std::make_heap is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/djinterp/re_std/algorithm/make_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_MAKE_HEAP_
#define DJINTERP_RESTD_ALGORITHM_MAKE_HEAP_ 1

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

// _make_heap_sift_down_
//   sifts the element at index _start downward through the prefix
// [_first, _first + _length) until the heap property holds at and
// below _start.
template<typename _RandomIt,
         typename _Distance,
         typename _Compare>
D_CONSTEXPR_CPP14 void
_make_heap_sift_down_(
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
// I.   MAKE_HEAP
// ===========================================================================

// make_heap (comparator)
//   function: rearranges [_first, _last) into a max-heap per _comp.
// Builds bottom-up by sifting every non-leaf node down in reverse
// index order — O(N) total work.
template<typename _RandomIt,
         typename _Compare>
D_CONSTEXPR_CPP14 void
make_heap(
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

    // last non-leaf is at index (length / 2) - 1
    for (_Diff _i = _length / 2 - 1; _i >= 0; --_i)
    {
        _make_heap_sift_down_<_RandomIt, _Diff, _Compare>(
            _first, _i, _length, _comp);
    }
}


// make_heap (default operator<)
//   function: as above with restd::less<value_type>().
template<typename _RandomIt>
D_CONSTEXPR_CPP14 void
make_heap(
    _RandomIt _first,
    _RandomIt _last
)
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    make_heap(_first, _last, restd::less<_Value>());
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_MAKE_HEAP_
