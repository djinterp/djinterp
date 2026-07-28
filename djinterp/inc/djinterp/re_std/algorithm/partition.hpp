/******************************************************************************
* djinterp [restd]                                                 partition.hpp
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
*     restd ships the BidirectionalIterator path on every tier
*     (DEVIATION FROM STD C++11+); the forward-only flavour is rotate-
*     based and adds substantial complexity for a niche use case.
*     Forward-only callers will get a compile error on the back-cursor
*     decrement.
*   - constexpr in std from C++26; restd lifts to C++14.
*   - C++11+ uses iter_swap (which delegates to swap with ADL); on
*     C++98 the same code path is used since iter_swap exists at C++98.
*
*
* path:      /inc/djinterp/restd/algorithm/partition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_PARTITION_
#define DJINTERP_RESTD_ALGORITHM_PARTITION_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"


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
// I.   PARTITION
// ===========================================================================

// partition
//   function: rearranges [_first, _last) so that all elements
// satisfying _pred come first. Returns the iterator to the first
// element that does not satisfy _pred (the partition point); returns
// _last if every element satisfies _pred.
template<typename _BidirIt,
         typename _Pred>
D_CONSTEXPR_CPP14 _BidirIt
partition(
    _BidirIt _first,
    _BidirIt _last,
    _Pred    _pred
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


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_PARTITION_
