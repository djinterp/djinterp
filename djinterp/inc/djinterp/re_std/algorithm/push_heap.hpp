/******************************************************************************
* djinterp [restd]                                                 push_heap.hpp
*
* push_heap algorithm header:
*   Treats [_first, _last - 1) as a max-heap and inserts *(_last - 1)
* into it by sifting that element upward toward the root until the heap
* property is restored.
*
*   PORTABILITY:
*   - std::push_heap is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*   - Implementation uses iter_swap-based sift-up; see header comment in
*     pop_heap.hpp for the swap-vs-hole-walking trade-off.
*
*
* path:      /inc/djinterp/restd/algorithm/push_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_PUSH_HEAP_
#define DJINTERP_RESTD_ALGORITHM_PUSH_HEAP_ 1

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
// 1.   INTERNAL: SIFT-UP
// ===========================================================================

// _push_heap_sift_up_
//   sifts the element at index _start in [_first, _first + _length)
// upward, swapping with its parent while it compares greater than that
// parent under _comp. The parent of index i is (i - 1) / 2.
template<typename _RandomIt,
         typename _Distance,
         typename _Compare>
D_CONSTEXPR_CPP14 void
_push_heap_sift_up_(
    _RandomIt _first,
    _Distance _start,
    _Compare  _comp
)
{
    _Distance _hole = _start;
    while (_hole > 0)
    {
        _Distance _parent = static_cast<_Distance>((_hole - 1) / 2);
        if (!_comp(*(_first + _parent), *(_first + _hole)))
        {
            break;
        }
        iter_swap(_first + _parent, _first + _hole);
        _hole = _parent;
    }
}


// ===========================================================================
// I.   PUSH_HEAP
// ===========================================================================

// push_heap (comparator)
//   function: inserts *(_last - 1) into the max-heap that [_first,
// _last - 1) is assumed to be. After return, [_first, _last) is a
// valid heap. No-op if the input range has fewer than two elements.
template<typename _RandomIt,
         typename _Compare>
D_CONSTEXPR_CPP14 void
push_heap(
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

    _push_heap_sift_up_<_RandomIt, _Diff, _Compare>(
        _first, _length - 1, _comp);
}


// push_heap (default operator<)
//   function: as above with restd::less<value_type>().
template<typename _RandomIt>
D_CONSTEXPR_CPP14 void
push_heap(
    _RandomIt _first,
    _RandomIt _last
)
{
    typedef typename iterator_traits<_RandomIt>::value_type _Value;
    push_heap(_first, _last, restd::less<_Value>());
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_PUSH_HEAP_
