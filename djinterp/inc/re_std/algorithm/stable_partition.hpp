/*******************************************************************************
* djinterp [re_std]                                         stable_partition.hpp
*
* stable_partition algorithm header:
*   Like partition, but preserves the relative order within each half:
* all elements satisfying _pred come first (in their original relative
* order), then all elements not satisfying _pred (also in their
* original relative order). Returns the iterator to the first
* false-element.
*
*   ALGORITHM:
*   Classical recursive divide-and-rotate, mirroring the in-place
* stable merge in stable_sort.hpp:
*     1. Skip leading trues; skip trailing falses (cheap base case).
*     2. Otherwise split the range at its midpoint.
*     3. Recursively stable_partition each half.
*     4. Rotate the inner segment [boundary_left, middle, boundary_right)
*        to bring the two trues blocks together.
*   Stack depth O(log N); each level does O(N) rotate work; total
*   O(N log N).
*
*   STABILITY:
*   The leaf case (range size 1) is trivially stable. The rotate of
* the inner [Lt | Lf | Rt | Rf] segment yields [Lt | Rt | Lf | Rf]:
* Lf and Rt are swapped block-wise, which is exactly the stability-
* preserving operation.
*
*   PORTABILITY:
*   - std::stable_partition is C++98 but typically uses get_temporary_buffer
*     for O(N) when allocation succeeds. re_std is allocator-free and
*     always runs the O(N log N) rotate-based path.
*   - constexpr in std from C++26; re_std does NOT add constexpr —
*     rotate's stack-recursion depth is data-dependent (matches std).
*   - Requires BidirectionalIterator.
*   - Recursive; stack depth O(log N).
*
*
* path:      /inc/re_std/algorithm/stable_partition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_STABLE_PARTITION_HPP
#define RE_STD_ALGORITHM_STABLE_PARTITION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "./rotate.hpp"
#include "../iterator/iterator_traits.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL
// ===========================================================================

// _stable_partition_impl_
//   function: recursive worker. Assumes [_first, _last) is non-empty AND already
// trimmed (no leading trues, no trailing falses); the public entry
// performs the trim. Returns the partition point in the post-rotate
// range.
template<typename BidirIt,
         typename Pred,
         typename Distance>
BidirIt
_stable_partition_impl_(
    BidirIt  _first,
    BidirIt  _last,
    Pred     _pred,
    Distance _len
)
{
    // single element: it must be a false (the leading-trues trim
    // already stripped any leading trues); leave in place
    if (_len == 1)
    {
        return _first;
    }

    // split at the midpoint
    Distance _half = _len / 2;
    BidirIt  _mid  = _first;
    for (Distance _i = 0; _i < _half; ++_i)
    {
        ++_mid;
    }

    // recurse on each half — but a half might be all-true or all-false
    // (no internal mixed elements); handle those without recursion
    BidirIt _left_cut;
    {
        // left half: [_first, _mid)
        // skip leading trues
        BidirIt  _lf = _first;
        Distance _i  = 0;
        while ( (_lf != _mid) &&
                _pred(*_lf) )
        {
            ++_lf;
            ++_i;
        }
        if (_lf == _mid)
        {
            // all true
            _left_cut = _mid;
        }
        else
        {
            // skip trailing falses in left half (walk back from _mid)
            BidirIt  _ll = _mid;
            Distance _ll_idx = _half;
            while (true)
            {
                BidirIt _prev = _ll;
                --_prev;
                if (_pred(*_prev))
                {
                    break;
                }
                _ll = _prev;
                --_ll_idx;
                if (_ll == _lf)
                {
                    break;
                }
            }
            if (_ll == _lf)
            {
                _left_cut = _lf;
            }
            else
            {
                _left_cut = _stable_partition_impl_(_lf, _ll, _pred,
                                                    _ll_idx - _i);
            }
        }
    }

    BidirIt _right_cut;
    {
        // right half: [_mid, _last), length _len - _half
        Distance _right_len = _len - _half;

        // skip leading trues
        BidirIt  _rf = _mid;
        Distance _i  = 0;
        while ( (_rf != _last) &&
                _pred(*_rf) )
        {
            ++_rf;
            ++_i;
        }
        if (_rf == _last)
        {
            _right_cut = _last;
        }
        else
        {
            // skip trailing falses
            BidirIt  _rl = _last;
            Distance _rl_idx = _right_len;
            while (true)
            {
                BidirIt _prev = _rl;
                --_prev;
                if (_pred(*_prev))
                {
                    break;
                }
                _rl = _prev;
                --_rl_idx;
                if (_rl == _rf)
                {
                    break;
                }
            }
            if (_rl == _rf)
            {
                _right_cut = _rf;
            }
            else
            {
                _right_cut = _stable_partition_impl_(_rf, _rl, _pred,
                                                     _rl_idx - _i);
            }
        }
    }

    // rotate [_left_cut, _mid, _right_cut): brings the right-half
    // trues forward, the left-half falses backward. Returns the new
    // partition point.
    return rotate(_left_cut, _mid, _right_cut);
}


// ===========================================================================
// I.   STABLE_PARTITION
// ===========================================================================

// stable_partition
//   function: rearranges [_first, _last) so that all elements
// satisfying _pred come first, preserving relative order within each
// half. Returns the iterator to the first false-element.
template<typename BidirIt,
         typename Pred>
BidirIt
stable_partition(
    BidirIt _first,
    BidirIt _last,
    Pred    _pred
)
{
    typedef typename iterator_traits<BidirIt>::difference_type _Diff;

    // trim leading trues — these stay in place
    while ( (_first != _last) &&
            _pred(*_first) )
    {
        ++_first;
    }
    if (_first == _last)
    {
        return _first;
    }

    // trim trailing falses — these also stay in place
    while (true)
    {
        BidirIt _prev = _last;
        --_prev;
        if (_pred(*_prev))
        {
            break;
        }
        _last = _prev;
        if (_first == _last)
        {
            return _first;
        }
    }

    // measure the trimmed mid-segment
    _Diff _len = 0;
    for (BidirIt _it = _first; _it != _last; ++_it)
    {
        ++_len;
    }

    return _stable_partition_impl_(_first, _last, _pred, _len);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_STABLE_PARTITION_HPP
