#include "./sort_common.h"


/*
d_sort_comparator_make
  Builds an ordering from a comparison and the context it reads.
  A convenience over aggregate initialisation, and the place a future field
would be defaulted, so call sites do not have to be revisited when one is
added.

Parameter(s):
  _compare: the three-way comparison; must be non-NULL to be usable.
  _context: passed to `_compare` unchanged; may be NULL.
Return:
  the ordering, by value.
*/
struct d_sort_comparator
d_sort_comparator_make
(
    d_sort_compare_fn _compare,
    void*             _context
)
{
    struct d_sort_comparator comparator;

    comparator.compare = _compare;
    comparator.context = _context;

    return comparator;
}


/*
d_sort_verify
  The request check every entry point performs first, factored out so that all
of them report the same status for the same mistake.
  Checks, in order: an ordering is present and callable; the element width is
non-zero; the range is present unless it is empty; and the range is addressable
end to end. The overflow test is last because it is the only one that can cost
a division, and it is reached only when a count or a width is large enough that
their product might not fit -- which is to say, only when the caller has
already misdescribed one of them.

Parameter(s):
  _base:         the range; may be NULL only when `_count` is 0.
  _count:        element count.
  _elem_size: element width in bytes.
  _comparator:   the ordering.
Return:
  D_SORT_STATUS_OK when the request is well formed, otherwise the formal status
describing what is wrong with it.
*/
enum d_sort_status
d_sort_verify
(
    const void*                     _base,
    size_t                          _count,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator
)
{
    // an ordering is the whole operation; there is nothing to fall back to,
    // and a zero width describes no element, so it describes no range
    if ( (!_comparator)          ||
         (!_comparator->compare) ||
         (_elem_size == 0)    )
    {
        return D_SORT_STATUS_INVALID_ARGUMENT;
    }

    // a null base is a range only when it is an empty one
    if ( (!_base)      &&
         (_count != 0) )
    {
        return D_SORT_STATUS_INVALID_ARGUMENT;
    }

    // a count and a width that both fit in half a size_t cannot overflow when
    // multiplied, which is every range that exists
    if ( ((_count | _elem_size) >> (sizeof(size_t) * 4)) != 0 )
    {
        if (_count > (SIZE_MAX / _elem_size))
        {
            return D_SORT_STATUS_OVERFLOW;
        }
    }

    return D_SORT_STATUS_OK;
}


/*
d_sort_is_sorted
  Whether the range is arranged in the requested order.
  Asks only whether any element strictly precedes the one before it, so a range
of equivalents is sorted and the answer agrees with what a stable algorithm
would have produced. The postcondition of every algorithm in the subsystem, and
cheap enough to assert in a debug build.

Parameter(s):
  _base:         the range; may be NULL only when `_count` is 0.
  _count:        element count.
  _elem_size: element width in bytes; must be non-zero.
  _comparator:   the ordering; must be non-NULL with a non-NULL compare.
  _order:        the direction; NONE behaves as ascending.
Return:
  true when the range is ordered, and true for a malformed request, since an
unsortable range is not an unsorted one.
*/
bool
d_sort_is_sorted
(
    const void*                     _base,
    size_t                          _count,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t      index;
    const void* earlier;
    const void* later;

    // a range that cannot be sorted cannot be out of order either
    if (d_sort_verify(_base, _count, _elem_size, _comparator)
            != D_SORT_STATUS_OK)
    {
        return true;
    }

    index   = 1;
    earlier = NULL;
    later   = NULL;

    // walk the adjacent pairs, looking for one that is out of order
    for (; index < _count; ++index)
    {
        earlier = d_sort_at_const(_base, index - 1, _elem_size);
        later   = d_sort_at_const(_base, index,     _elem_size);

        if (d_sort_precedes(_comparator, _order, later, earlier))
        {
            return false;
        }
    }

    return true;
}
