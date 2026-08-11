#include "./merge_sort_common.h"

// std
#include <string.h>


/*
d_merge_runs
  Merges the ordered runs [`_begin`, `_mid`) and [`_mid`, `_end`) of `_src`
into [`_begin`, `_end`) of `_dst`.
  The two runs must each already be ordered; the caller guarantees that, since
in a bottom-up sweep they are the output of the previous pass.
  Taking from the right run only on a STRICT precedence is the whole of this
family's stability: on equivalence the left element goes first, and the left
run holds the earlier elements.
  Nothing is compared when one run is empty, and nothing is compared at all
when the pair is already in order -- one test settles that, and the span
becomes a single block copy. That test is why a range arriving ordered costs
O(n) comparisons rather than O(n log n), which matters most here, where each
comparison is an indirect call.
  `_src` and `_dst` must not overlap over [`_begin`, `_end`).

Parameter(s):
  _src:        the range being read; never NULL.
  _dst:        the range being written; never NULL, and distinct from `_src`.
  _begin:      first index of the left run.
  _mid:        one past the left run, and the first index of the right run;
               must lie in [`_begin`, `_end`].
  _end:        one past the right run.
  _elem_size:  element width in bytes; must be non-zero.
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
Return:
  none.
*/
void
d_merge_runs
(
    const void*                     _src,
    void*                           _dst,
    size_t                          _begin,
    size_t                          _mid,
    size_t                          _end,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t left;
    size_t right;
    size_t out;

    // an empty span has nothing to merge
    if (_end <= _begin)
    {
        return;
    }

    // one run only, or a pair already in order: the span is a block copy.
    // The second test asks whether the right run's first element precedes the
    // left run's last; when it does not, every left element belongs first
    if ( (_mid <= _begin) ||
         (_mid >= _end)   ||
         (!d_sort_precedes(_comparator,
                           _order,
                           d_sort_at_const(_src, _mid,     _elem_size),
                           d_sort_at_const(_src, _mid - 1, _elem_size))) )
    {
        memcpy(d_sort_at(_dst, _begin, _elem_size),
               d_sort_at_const(_src, _begin, _elem_size),
               (_end - _begin) * _elem_size);

        return;
    }

    left  = _begin;
    right = _mid;
    out   = _begin;

    // interleave while both runs have elements left
    while ( (left < _mid) &&
            (right < _end) )
    {
        // take from the right only on a STRICT precedence, so equivalent
        // elements are taken from the left first and stability holds
        if (d_sort_precedes(_comparator,
                            _order,
                            d_sort_at_const(_src, right, _elem_size),
                            d_sort_at_const(_src, left,  _elem_size)))
        {
            memcpy(d_sort_at(_dst, out, _elem_size),
                   d_sort_at_const(_src, right, _elem_size),
                   _elem_size);

            ++right;
        }
        else
        {
            memcpy(d_sort_at(_dst, out, _elem_size),
                   d_sort_at_const(_src, left, _elem_size),
                   _elem_size);

            ++left;
        }

        ++out;
    }

    // whichever run still has elements is already ordered: copy it whole
    if (left < _mid)
    {
        memcpy(d_sort_at(_dst, out, _elem_size),
               d_sort_at_const(_src, left, _elem_size),
               (_mid - left) * _elem_size);
    }

    if (right < _end)
    {
        memcpy(d_sort_at(_dst, out, _elem_size),
               d_sort_at_const(_src, right, _elem_size),
               (_end - right) * _elem_size);
    }

    return;
}


/*
d_merge_pass
  Merges every adjacent pair of `_width`-element runs from `_src` into `_dst`,
covering all `_count` elements.
  A range whose length is not a multiple of twice the width ends in a short
run, or in a lone run with no partner. Both fall out of clamping `_mid` and
`_end` to the count: a lone run has an empty right side, which d_merge_runs
copies through untouched.
  The run pairs are DISJOINT, which is the property a concurrent driver
exploits: any subset of the iterations below may be run in parallel with any
other, because no two of them read or write the same element.
  The caller owns the ranges. A pass does not verify them, because it is called
once per width by a driver that verified once per sort.

Parameter(s):
  _src:        the range being read; never NULL.
  _dst:        the range being written; never NULL, and distinct from `_src`.
  _count:      element count of both ranges.
  _width:      run width for this pass; must be non-zero.
  _elem_size:  element width in bytes; must be non-zero.
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
Return:
  none.
*/
void
d_merge_pass
(
    const void*                     _src,
    void*                           _dst,
    size_t                          _count,
    size_t                          _width,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t begin;
    size_t mid;
    size_t end;

    // a zero width would not advance, and an empty range has no runs
    if ( (_width == 0) ||
         (_count == 0) )
    {
        return;
    }

    // each iteration is independent of every other one
    for (begin = 0; begin < _count; begin += (2 * _width))
    {
        mid = begin + _width;
        end = begin + (2 * _width);

        // a short or absent partner run clamps to the end of the range
        if (mid > _count)
        {
            mid = _count;
        }

        if (end > _count)
        {
            end = _count;
        }

        d_merge_runs(_src,
                     _dst,
                     begin,
                     mid,
                     end,
                     _elem_size,
                     _comparator,
                     _order);
    }

    return;
}
