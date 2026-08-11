#include "./selection_sort_common.h"


/*
d_selection_min_index
  The index of the element that belongs first in the run [`_begin`, `_end`).
  "Minimum" is with respect to the ordering AND the direction: under
D_SORT_ORDER_DESCENDING the element that belongs first is the largest, and the
scan finds it without a second code path, because direction is applied inside
d_sort_precedes.
  A candidate must STRICTLY precede the incumbent to displace it, so among
equal elements the earliest wins. That, together with the forward scan, is what
pins the arrangement this family produces -- see the note in the header. The
comparison is also why the run cannot be cut short: nothing learned about the
first k elements says anything about the remaining ones, which is the whole of
why this algorithm is not adaptive.
  Published separately from the pass because it is the piece a concurrent
driver partitions: the search is a reduction, so workers can scan chunks and a
single comparison per chunk settles the winner.

Parameter(s):
  _base:       the range; never NULL.
  _begin:      first index of the run.
  _end:        one past the run's last index.
  _elem_size:  element width in bytes; must be non-zero.
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
Return:
  the index of the run's minimum, or `_begin` when the run is empty.
*/
size_t
d_selection_min_index
(
    const void*                     _base,
    size_t                          _begin,
    size_t                          _end,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t      winner;
    size_t      index;
    const void* candidate;
    const void* incumbent;

    // an empty run has no minimum to report
    if (_end <= _begin)
    {
        return _begin;
    }

    winner = _begin;

    // scan forward; only a STRICT precedence displaces the incumbent, so the
    // earliest of several equal elements is the one that wins
    for (index = _begin + 1; index < _end; ++index)
    {
        candidate = d_sort_at_const(_base, index,  _elem_size);
        incumbent = d_sort_at_const(_base, winner, _elem_size);

        if (d_sort_precedes(_comparator, _order, candidate, incumbent))
        {
            winner = index;
        }
    }

    return winner;
}


/*
d_selection_pass
  Sorts the run [`_begin`, `_end`) by repeatedly selecting its minimum and
exchanging it to the front of the unsorted remainder.
  The exchange is skipped when the winner is already in place. That is not only
an optimisation: it is what holds the exchange count to "at most n-1", which is
this algorithm's entire reason for existing, and a self-exchange on a large
element would spend real writes to accomplish nothing.
  The final position needs no pass of its own -- once every other element has
been placed, the one remaining is the largest, and there is nothing left to
compare it against. So the loop stops one short of the end.
  The caller owns the range. A pass does not verify it, because it is called by
a driver that verified it once per sort.

Parameter(s):
  _base:       the range; never NULL.
  _begin:      first index of the run.
  _end:        one past the run's last index; must not exceed the element
               count.
  _elem_size:  element width in bytes; must be non-zero.
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
Return:
  none.
*/
void
d_selection_pass
(
    void*                           _base,
    size_t                          _begin,
    size_t                          _end,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t position;
    size_t winner;

    // a run of 0 or 1 elements is already sorted
    if ( (_end <= _begin)      ||
         ((_end - _begin) < 2) )
    {
        return;
    }

    // the last position takes whatever is left, so it needs no scan
    for (position = _begin; position < (_end - 1); ++position)
    {
        winner = d_selection_min_index(_base,
                                       position,
                                       _end,
                                       _elem_size,
                                       _comparator,
                                       _order);

        // skipping a self-exchange is what keeps the count at n-1
        if (winner != position)
        {
            d_memswap(d_sort_at(_base, position, _elem_size),
                      d_sort_at(_base, winner,   _elem_size),
                      _elem_size);
        }
    }

    return;
}
