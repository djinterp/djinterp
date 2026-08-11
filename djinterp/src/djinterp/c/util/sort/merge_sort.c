#include "./merge_sort.h"

// std
#include <string.h>


/*
d_merge_sort_scratch_size
  The scratch, in bytes, that d_merge_sort needs for a range of `_count`
elements of `_elem_size` bytes.
  The measure half of the two-call protocol. A range of fewer than two elements
is already sorted and needs none, so 0 is a real answer rather than a failure.
  0 is also returned when the product would not fit, which is the same answer
d_sort_verify gives that request when d_merge_sort is called with it: the range
cannot exist, so no buffer serves it.

Parameter(s):
  _count:     element count.
  _elem_size: element width in bytes.
Return:
  the required scratch in bytes, or 0 when none is needed or the range is not
addressable.
*/
size_t
d_merge_sort_scratch_size
(
    size_t _count,
    size_t _elem_size
)
{
    // a range of 0 or 1 elements is already sorted
    if ( (_count < 2)        ||
         (_elem_size == 0)   )
    {
        return 0;
    }

    // a range that cannot be addressed cannot be buffered either
    if (_count > (SIZE_MAX / _elem_size))
    {
        return 0;
    }

    return (_count * _elem_size);
}


/*
d_merge_sort
  Sorts the range in place, using the caller's scratch, by merging runs of
doubling width until one run covers it.

  THE PING-PONG. A merge cannot write into what it is reading, so each pass
reads one buffer and writes the other, and the two exchange roles between
passes. The alternative -- merge into scratch, copy back, repeat -- is correct
and copies every element twice per pass instead of once. After the loop the
sorted elements are wherever the last pass wrote them, so the range gets one
final copy only when that was the scratch.

  The buffer check comes after d_sort_verify and after the trivial-range exit,
so a caller sorting one element is never told its absent buffer is too small,
and a caller who got the arguments wrong hears about that first. BUFFER_TOO_
SMALL is the subsystem's only mechanical status: the request was well formed
and the machinery ran short, so retrying with more is meaningful, which is not
true of any other failure here.

Parameter(s):
  _base:         the range; may be NULL only when `_count` is 0.
  _count:        element count.
  _elem_size:    element width in bytes; must be non-zero.
  _comparator:   the ordering; must be non-NULL with a non-NULL compare.
  _order:        the direction; NONE behaves as ascending.
  _scratch:      auxiliary storage of at least d_merge_sort_scratch_size bytes,
                 aligned for the element type; may be NULL only when the range
                 holds fewer than two elements.
  _scratch_size: the size of `_scratch` in bytes.
Return:
  D_SORT_STATUS_OK on success; D_SORT_STATUS_BUFFER_TOO_SMALL when the scratch
does not cover the range, with the range left untouched; otherwise the formal
status from d_sort_verify.
*/
enum d_sort_status
d_merge_sort
(
    void*                           _base,
    size_t                          _count,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order,
    void*                           _scratch,
    size_t                          _scratch_size
)
{
    enum d_sort_status status;
    size_t             required;
    size_t             width;
    void*              source;
    void*              target;
    void*              exchange;

    status = d_sort_verify(_base, _count, _elem_size, _comparator);

    // a malformed request leaves the range untouched
    if (status != D_SORT_STATUS_OK)
    {
        return status;
    }

    // a range of 0 or 1 elements is already sorted, and needs no buffer to
    // say so
    if (_count < 2)
    {
        return D_SORT_STATUS_OK;
    }

    required = d_merge_sort_scratch_size(_count, _elem_size);

    // the one failure a caller can act on: supply more and retry
    if ( (!_scratch)                ||
         (_scratch_size < required) )
    {
        return D_SORT_STATUS_BUFFER_TOO_SMALL;
    }

    source = _base;
    target = _scratch;

    // runs of width 1 are ordered by definition; each pass doubles the width
    // until one run covers the range
    for (width = 1; width < _count; width *= 2)
    {
        d_merge_pass(source,
                     target,
                     _count,
                     width,
                     _elem_size,
                     _comparator,
                     _order);

        // the pass's output is the next pass's input
        exchange = source;
        source   = target;
        target   = exchange;
    }

    // the result is wherever the last pass wrote it; bring it home only if
    // that was not the caller's range
    if (source != _base)
    {
        memcpy(_base, source, _count * _elem_size);
    }

    return D_SORT_STATUS_OK;
}
