#include "./insertion_sort_common.h"

// std
#include <string.h>


/*
d_internal_insertion_shift
  Moves the element at `_index` down to `_position`, sliding everything in
[`_position`, `_index`) one place up to make room.
  Two paths, one arrangement. When the element fits the stack block it is
lifted out once, the run is moved over it in a single memmove, and it is put
back -- (index - position) + 2 element writes. When it does not fit there is
nowhere to lift it to, so a chain of exchanges walks it down instead, which
needs no storage and costs three times the writes. Neither path allocates, and
the two are indistinguishable from outside.

Parameter(s):
  _base:      the range; never NULL.
  _position:  destination index; must not exceed `_index`.
  _index:     index of the element to move.
  _elem_size: element width in bytes.
Return:
  none.
*/
D_STATIC_INLINE void
d_internal_insertion_shift
(
    void*  _base,
    size_t _position,
    size_t _index,
    size_t _elem_size
)
{
    char   hold[D_INSERTION_HOLD_SIZE];
    size_t index;

    // an element already at its destination has nowhere to go
    if (_position >= _index)
    {
        return;
    }

    // wide elements: no room to lift, so walk it down by exchanges
    if (_elem_size > D_INSERTION_HOLD_SIZE)
    {
        for (index = _index; index > _position; --index)
        {
            d_memswap(d_sort_at(_base, index - 1, _elem_size),
                      d_sort_at(_base, index,     _elem_size),
                      _elem_size);
        }

        return;
    }

    // lift, slide the run over the vacated slot, put the element back
    memcpy(hold,
           d_sort_at(_base, _index, _elem_size),
           _elem_size);

    memmove(d_sort_at(_base, _position + 1, _elem_size),
            d_sort_at(_base, _position,     _elem_size),
            (_index - _position) * _elem_size);

    memcpy(d_sort_at(_base, _position, _elem_size),
           hold,
           _elem_size);

    return;
}


/*
d_insertion_place
  Places the element at `_index` into the ordered run [`_lower`, `_index`), and
reports where it landed.
  The scan walks back over every element the candidate strictly precedes and
stops at the first one it does not. Stopping on "does not strictly precede"
rather than "strictly follows" is what makes the family stable: an equivalent
element halts the scan, so the candidate settles after it rather than in front
of it.
  The candidate is read in place during the scan -- nothing has moved yet --
so no element is lifted until a destination is known, and an element already in
order is never touched at all. That early test is what gives ordered input its
O(n) best case.

Parameter(s):
  _base:       the range; never NULL.
  _lower:      first index of the ordered run; the candidate settles no
               earlier than this.
  _index:      index of the element to place; must not precede `_lower`.
  _elem_size:  element width in bytes; must be non-zero.
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
Return:
  the index the element now occupies, which is `_index` when it did not move.
*/
size_t
d_insertion_place
(
    void*                           _base,
    size_t                          _lower,
    size_t                          _index,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t position;
    void*  candidate;
    void*  preceding;

    // the run's first element has nothing behind it to be placed among
    if (_index <= _lower)
    {
        return _index;
    }

    candidate = d_sort_at(_base, _index,     _elem_size);
    preceding = d_sort_at(_base, _index - 1, _elem_size);

    // already in order relative to its predecessor: the O(n) best case, and
    // the reason ordered input is never written to
    if (!d_sort_precedes(_comparator, _order, candidate, preceding))
    {
        return _index;
    }

    position = _index;

    // walk back over everything the candidate strictly precedes
    while (position > _lower)
    {
        preceding = d_sort_at(_base, position - 1, _elem_size);

        if (!d_sort_precedes(_comparator, _order, candidate, preceding))
        {
            break;
        }

        --position;
    }

    d_internal_insertion_shift(_base, position, _index, _elem_size);

    return position;
}


/*
d_insertion_pass
  Sorts the run [`_begin`, `_end`) by placing each of its elements into the
ordered prefix growing behind it.
  One sweep is the whole sort. That is the difference between this family and
the bubble family: d_bubble_pass must be called until it reports no exchange,
where this is called once.
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
d_insertion_pass
(
    void*                           _base,
    size_t                          _begin,
    size_t                          _end,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    size_t index;

    // a run of 0 or 1 elements is already sorted
    if ( (_end <= _begin)      ||
         ((_end - _begin) < 2) )
    {
        return;
    }

    // the run's first element is the ordered prefix; place each of the rest
    for (index = _begin + 1; index < _end; ++index)
    {
        (void)d_insertion_place(_base,
                                _begin,
                                index,
                                _elem_size,
                                _comparator,
                                _order);
    }

    return;
}
