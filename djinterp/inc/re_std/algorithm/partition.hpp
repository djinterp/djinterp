/*******************************************************************************
* djinterp [re_std]                                                partition.hpp
*
* partition algorithm header:
*   Rearranges the elements of [_first, _last) so that every element
* for which _pred returns true precedes every element for which it
* returns false. Returns the iterator to the first false-element (the
* partition point). Order within each half is not preserved (see
* stable_partition for that contract).
*
*   ALGORITHM:
*   Hoare-style two-cursor scan, requiring BidirectionalIterator. Walks
* a front cursor forward looking for falses and a back cursor backward
* looking for trues; swaps the pair when both are found and crosses
* when they meet.
*
*   PORTABILITY:
*   - std::partition is C++98 (returned the partition point from the
*     outset).
*   - std::partition requires only ForwardIterator in C++11+ and was
*     ForwardIterator-only-with-Bidirectional-fast-path before that.
*     re_std ships the BidirectionalIterator path on every tier
*     (DEVIATION FROM STD C++11+); the forward-only flavour is rotate-
*     based and adds substantial complexity for a niche use case.
*     Forward-only callers will get a compile error on the back-cursor
*     decrement.
*   - constexpr in std from C++26; re_std lifts to C++14.
*   - C++11+ uses iter_swap (which delegates to swap with ADL); on
*     C++98 the same code path is used since iter_swap exists at C++98.
*
*
* path:      /inc/re_std/algorithm/partition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_PARTITION_HPP
#define RE_STD_ALGORITHM_PARTITION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./iter_swap.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   PARTITION
// ===========================================================================

// partition
//   function: rearranges [_first, _last) so that all elements
// satisfying _pred come first. Returns the iterator to the first
// element that does not satisfy _pred (the partition point); returns
// _last if every element satisfies _pred.
template<typename BidirIt,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 BidirIt
partition(
    BidirIt _first,
    BidirIt _last,
    Pred    _pred
)
{
    while (true)
    {
        // advance _first to the first false-element
        while ( (_first != _last) &&
                _pred(*_first) )
        {
            ++_first;
        }
        if (_first == _last)
        {
            return _first;
        }

        // retreat _last to the last true-element (one past it stays as
        // _last for the swap target)
        do
        {
            --_last;
            if (_first == _last)
            {
                return _first;
            }
        }
        while (!_pred(*_last));

        iter_swap(_first, _last);
        ++_first;
    }
}


}  // re_std


#endif  // RE_STD_ALGORITHM_PARTITION_HPP
