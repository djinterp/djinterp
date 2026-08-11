#include "./bubble_sort.h"


/*
d_bubble_sort
  Sorts the range in place by repeated passes of adjacent exchanges.
  `end` is the exclusive bound of the unsorted region. Each pass sweeps it and
returns the index of its last exchange, which becomes the next bound: nothing
at or after it moved, so everything from there on is in final position. A pass
that exchanges nothing returns 0, which ends the loop -- so the O(n) best case
on ordered input needs no separate test for it.
  Nothing is allocated. A malformed request leaves the range untouched, so a
caller may correct it and retry against unmodified input.

Parameter(s):
  _base:         the range; may be NULL only when `_count` is 0.
  _count:        element count.
  _elem_size: element width in bytes; must be non-zero.
  _comparator:   the ordering; must be non-NULL with a non-NULL compare.
  _order:        the direction; NONE behaves as ascending.
Return:
  D_SORT_STATUS_OK on success, otherwise the formal status from d_sort_verify
describing what is wrong with the request.
*/
enum d_sort_status
d_bubble_sort
(
    void*                           _base,
    size_t                          _count,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    enum d_sort_status status;
    size_t             end;

    status = d_sort_verify(_base, _count, _elem_size, _comparator);

    // a malformed request leaves the range untouched
    if (status != D_SORT_STATUS_OK)
    {
        return status;
    }

    end = _count;

    // a range of 0 or 1 elements is already sorted; otherwise sweep, then
    // shrink the unsorted region to wherever the sweep last had to exchange
    while (end > 1)
    {
        end = d_bubble_pass(_base,
                            1,
                            end,
                            _elem_size,
                            _comparator,
                            _order);
    }

    return D_SORT_STATUS_OK;
}
