/*******************************************************************************
* djinterp [re_std]                                                 pop_heap.hpp
*
* pop_heap algorithm header:
*   Swaps the root (*_first) with the last element (*(_last - 1)) of
* the max-heap [_first, _last), then re-establishes the heap property
* on the shrunken range [_first, _last - 1). After return, the former
* maximum sits at *(_last - 1) and [_first, _last - 1) is a valid heap.
*
*   IMPLEMENTATION:
*   Uses iter_swap-based sift-down. The classical alternative is
* "hole-walking": save the displaced last value, sift the hole down
* (one move per level instead of three reads + writes per swap), then
* place the saved value into the final hole position. Hole-walking is
* ~30% faster on heaps of non-trivial element types and is what
* libstdc++/libc++ ship. re_std's swap-based form is simpler and
* matches the private _sift_down_ helpers already used in sort.hpp,
* partial_sort.hpp, etc.; a perf-pass refactor can introduce the
* hole-walking form later.
*
*   PORTABILITY:
*   - std::pop_heap is C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - Requires RandomAccessIterator.
*   - Two overloads: default operator< and custom comparator.
*
*
* path:      /inc/re_std/algorithm/pop_heap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_POP_HEAP_HPP
#define RE_STD_ALGORITHM_POP_HEAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../functional/less.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// 1.   INTERNAL: SIFT-DOWN
// ===========================================================================

// _pop_heap_sift_down_
//   function: sifts the element at index _start in [_first, _first + _length)
// downward, swapping with the larger child while it compares less
// under _comp. Children of index i are at 2i + 1 and 2i + 2.
template<typename RandomIt,
         typename Distance,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
_pop_heap_sift_down_(
    RandomIt _first,
    Distance _start,
    Distance _length,
    Compare  _comp
)
{
    Distance _parent = _start;
    while (true)
    {
        Distance _child = static_cast<Distance>(2 * _parent + 1);
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
// I.   POP_HEAP
// ===========================================================================

// pop_heap (comparator)
//   function: moves the heap's maximum to *(_last - 1) and restores
// the heap property on [_first, _last - 1). No-op if the input range
// has fewer than two elements.
template<typename RandomIt,
         typename Compare>
RE_STD_CONSTEXPR_CPP14 void
pop_heap(
    RandomIt _first,
    RandomIt _last,
    Compare  _comp
)
{
    typedef typename iterator_traits<RandomIt>::difference_type _Diff;

    _Diff _length = _last - _first;
    if (_length < 2)
    {
        return;
    }

    iter_swap(_first, _last - 1);
    _pop_heap_sift_down_<RandomIt, _Diff, Compare>(
        _first, static_cast<_Diff>(0), _length - 1, _comp);
}


// pop_heap (default operator<)
//   function: as above with re_std::less<value_type>().
template<typename RandomIt>
RE_STD_CONSTEXPR_CPP14 void
pop_heap(
    RandomIt _first,
    RandomIt _last
)
{
    typedef typename iterator_traits<RandomIt>::value_type _Value;
    pop_heap(_first, _last, re_std::less<_Value>());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_POP_HEAP_HPP
