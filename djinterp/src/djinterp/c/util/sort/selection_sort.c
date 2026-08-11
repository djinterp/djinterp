#include "./selection_sort.h"


/*
d_selection_sort
  Sorts the range in place by giving each position the minimum of everything
that remains.
  A single sweep is the whole sort, so like the insertion driver and unlike the
bubble one there is no loop here: d_selection_pass leaves [0, count) ordered
when it returns.
  Nothing is allocated. A malformed request leaves the range untouched, so a
caller may correct it and retry against unmodified input.

Parameter(s):
  _base:       the range; may be NULL only when `_count` is 0.
  _count:      element count.
  _elem_size:  element width in bytes; must be non-zero.
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
Return:
  D_SORT_STATUS_OK on success, otherwise the formal status from d_sort_verify
describing what is wrong with the request.
*/
enum d_sort_status
d_selection_sort
(
    void*                           _base,
    size_t                          _count,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    enum d_sort_status status;

    status = d_sort_verify(_base, _count, _elem_size, _comparator);

    // a malformed request leaves the range untouched
    if (status != D_SORT_STATUS_OK)
    {
        return status;
    }

    // one sweep over the whole range is the entire algorithm
    d_selection_pass(_base,
                     0,
                     _count,
                     _elem_size,
                     _comparator,
                     _order);

    return D_SORT_STATUS_OK;
}
