/******************************************************************************
* djinterp [restd]                                             inplace_merge.hpp
*
* inplace_merge algorithm header:
*   Merges two consecutive sorted subranges [_first, _middle) and
* [_middle, _last) into a single sorted range [_first, _last), in
* place. Stable: equivalent elements from the left subrange precede
* equivalent elements from the right subrange.
*
*   ALGORITHM:
*   No-buffer divide-and-rotate (Knuth, TAOCP 5.2.4). Recursive; stack
* depth O(log N). The merge step:
*     1. If either half is empty, done. Single 1+1 case is one swap.
*     2. Otherwise split the LARGER half at its midpoint, binary-search
*        the mirror position in the other half (lower_bound or
*        upper_bound depending on which half was split — to preserve
*        stability), rotate the inner segment, recurse on the two
*        resulting halves.
*   The asymmetric binary-search choice (lower_bound when probing the
* right with a left-side element; upper_bound when probing the left
* with a right-side element) is what keeps the merge stable.
*
*   COMPLEXITY:
*   O((N1 + N2) * log(min(N1, N2))) — allocator-free. std's typical
* implementation uses get_temporary_buffer for O(N1 + N2) when
* allocation succeeds; restd accepts the asymptotic loss for the
* allocator-free guarantee.
*
*   PORTABILITY:
*   - std::inplace_merge is C++98.
*   - constexpr in std from C++26; restd does NOT lift (recursion depth
*     is data-dependent and rotate is a heavy primitive — mirrors std).
*   - Requires BidirectionalIterator.
*   - Two overloads: default operator< and custom comparator.
*   - This file IS the source of truth for the in-place merge
*     algorithm. stable_sort.hpp currently inlines a private copy
*     (_stable_in_place_merge_); when that file refactors it should
*     delete the private version and call restd::inplace_merge.
*
*
* path:      /inc/djinterp/restd/algorithm/inplace_merge.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_INPLACE_MERGE_
#define DJINTERP_RESTD_ALGORITHM_INPLACE_MERGE_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "./iter_swap.hpp"
#include "./lower_bound.hpp"
#include "./upper_bound.hpp"
#include "./rotate.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/distance.hpp"
#include "../iterator/advance.hpp"
#include "../functional/less.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL DRIVER
// ===========================================================================

// _inplace_merge_recurse_
//   recursive worker. Distances are passed in to avoid repeated
// distance() calls on non-random-access iterators.
template<typename _BidirIt,
         typename _Distance,
         typename _Compare>
void
_inplace_merge_recurse_(
    _BidirIt  _first,
    _BidirIt  _middle,
    _BidirIt  _last,
    _Distance _len1,
    _Distance _len2,
    _Compare  _comp
)
{
    // empty half: nothing to merge
    if ( (_len1 == 0) ||
         (_len2 == 0) )
    {
        return;
    }

    // 1 + 1 base case: at most one swap
    if ((_len1 + _len2) == 2)
    {
        if (_comp(*_middle, *_first))
        {
            iter_swap(_first, _middle);
        }
        return;
    }

    _BidirIt  _cut1;
    _BidirIt  _cut2;
    _Distance _len11;
    _Distance _len22;

    if (_len1 > _len2)
    {
        // split the larger (left) half at its midpoint
        _len11 = _len1 / 2;
        _cut1  = _first;
        restd::advance(_cut1, _len11);
        // *_cut1 is a left-side element; lower_bound on right preserves
        // left-before-right for equivalents
        _cut2  = restd::lower_bound(_middle, _last, *_cut1, _comp);
        _len22 = restd::distance(_middle, _cut2);
    }
    else
    {
        _len22 = _len2 / 2;
        _cut2  = _middle;
        restd::advance(_cut2, _len22);
        // *_cut2 is a right-side element; upper_bound on left preserves
        // left-before-right for equivalents
        _cut1  = restd::upper_bound(_first, _middle, *_cut2, _comp);
        _len11 = restd::distance(_first, _cut1);
    }

    // rotate the inner segment to bring matching halves together
    restd::rotate(_cut1, _middle, _cut2);

    _BidirIt _new_middle = _first;
    restd::advance(_new_middle, _len11 + _len22);

    _inplace_merge_recurse_(_first, _cut1, _new_middle,
                            _len11, _len22, _comp);
    _inplace_merge_recurse_(_new_middle, _cut2, _last,
                            _len1 - _len11, _len2 - _len22, _comp);
}


// ===========================================================================
// I.   INPLACE_MERGE (DEFAULT operator<)
// ===========================================================================

// inplace_merge
//   function: merges sorted [_first, _middle) with sorted
// [_middle, _last) so that [_first, _last) becomes sorted, in place,
// stably.
template<typename _BidirIt>
void
inplace_merge(
    _BidirIt _first,
    _BidirIt _middle,
    _BidirIt _last
)
{
    typedef typename iterator_traits<_BidirIt>::value_type _Value;
    typedef typename iterator_traits<_BidirIt>::difference_type _Diff;

    _Diff _len1 = restd::distance(_first,  _middle);
    _Diff _len2 = restd::distance(_middle, _last);

    _inplace_merge_recurse_(_first, _middle, _last, _len1, _len2,
                            restd::less<_Value>());
}


// ===========================================================================
// II.  INPLACE_MERGE (COMPARATOR)
// ===========================================================================

template<typename _BidirIt,
         typename _Compare>
void
inplace_merge(
    _BidirIt _first,
    _BidirIt _middle,
    _BidirIt _last,
    _Compare _comp
)
{
    typedef typename iterator_traits<_BidirIt>::difference_type _Diff;

    _Diff _len1 = restd::distance(_first,  _middle);
    _Diff _len2 = restd::distance(_middle, _last);

    _inplace_merge_recurse_(_first, _middle, _last, _len1, _len2, _comp);
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_INPLACE_MERGE_
