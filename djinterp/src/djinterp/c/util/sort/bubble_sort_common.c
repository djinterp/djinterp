#include "./bubble_sort_common.h"


/*
d_bubble_pass
  Sweeps the adjacent pairs (i-1, i) for i in [`_begin`, `_end`), exchanging
each pair that is out of order, and reports where it last had to.
  That report is what makes the family adaptive. Nothing at or after the last
exchange moved during the sweep, so every element from there on is already at
least as large as everything before it and will not move again -- a sequential
driver can take the returned index as its next bound rather than stepping down
by one. A sweep that exchanged nothing returns 0, which ends the driver's loop
without needing a separate flag: the two mechanisms are the same mechanism.
  The caller is responsible for the range. A pass does not verify it, because
it is called once per pass by a driver that verified it once per sort, and
paying for the check on every pass would be paying for the same answer n times.

Parameter(s):
  _begin:        index of the first pair's later element; must be at least 1.
  _end:          one past the last pair's later element; must not exceed the
                 element count.
  _base:         the range; never NULL.
  _elem_size: element width in bytes; must be non-zero.
  _comparator:   the ordering; must be non-NULL with a non-NULL compare.
  _order:        the direction; NONE behaves as ascending.
Return:
  the index of the later element of the last pair exchanged, or 0 when the
sweep exchanged nothing.
*/
size_t
d_bubble_pass
(
    void*                           _base,
    size_t                          _begin,
    size_t                          _end,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t last_exchange;
    size_t index;
    void*  earlier;
    void*  later;

    // an empty or degenerate run has no pairs to sweep
    if ( (_begin == 0)     ||
         (_end <= _begin)  )
    {
        return 0;
    }

    last_exchange = 0;
    earlier       = NULL;
    later         = NULL;

    // walk the run, carrying each out-of-order element one place to the right
    for (index = _begin; index < _end; ++index)
    {
        earlier = d_sort_at(_base, index - 1, _elem_size);
        later   = d_sort_at(_base, index,     _elem_size);

        if (d_bubble_compare_exchange(earlier,
                                      later,
                                      _elem_size,
                                      _comparator,
                                      _order))
        {
            last_exchange = index;
        }
    }

    return last_exchange;
}
