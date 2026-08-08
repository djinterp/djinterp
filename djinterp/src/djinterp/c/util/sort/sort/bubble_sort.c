#include "./bubble_sort.h"


/*
D_INTERNAL_BUBBLE_SORT_DEFINE_ENTRY
  One row of the definition pass: the natively ordered form and the
caller-ordered form of one scalar type. Walks the same list the header's
declaration pass walks, so the two cannot drift.
*/
#define D_INTERNAL_BUBBLE_SORT_DEFINE_ENTRY(_name,                          \
                                            _type)                          \
    D_BUBBLE_SORT_DEFINE(_name, _type)                                      \
    D_BUBBLE_SORT_DEFINE_BY(_name##_by, _type)

D_BUBBLE_SORT_SCALAR_LIST(D_INTERNAL_BUBBLE_SORT_DEFINE_ENTRY)

#undef D_INTERNAL_BUBBLE_SORT_DEFINE_ENTRY


/*
d_internal_bubble_exchange_bytes
  Drops the element at `_i` one place to its left, which on a run-time-width
range means exchanging the two slots outright: the carried element physically
occupies the left slot, so moving the smaller element into it would destroy the
thing being carried. The typed hook can be two assignments because it holds the
carried element in a register; here there is nowhere to hold it.
  A byte loop rather than a fixed buffer, because a buffer would need a size to
be fixed at and the whole point of this path is that there is not one.

Parameter(s):
  _left:  the slot to the left; never NULL.
  _right: the slot being dropped into it; never NULL.
  _width: element width in bytes.
Return:
  none.
*/
D_STATIC_INLINE void
d_internal_bubble_exchange_bytes
(
    char*  _left,
    char*  _right,
    size_t _width
)
{
    size_t k;
    char   byte;

    for (k = 0; k < _width; ++k)
    {
        byte      = _left[k];
        _left[k]  = _right[k];
        _right[k] = byte;
    }

    return;
}


/*
The erased entry's element operations.

  These four macros are private to this file and read `_element_size` from the
enclosing function by name, which is why they are defined here and undefined
below rather than published in a header. A hook that captures is acceptable
when its whole lifetime is one function; one that captures across a header is
not, which is why the published hooks take everything they use.

  The carried element degrades to memory. On a typed range the body holds it in
a register; here it is by definition the element at base[i-1], which is exactly
where it already sits -- so CARRY is a no-op, LESS reads base[i-1] instead of
the register, and EXCHANGE swaps the two slots. The control flow above them is
untouched, which is what makes this the same sort and not a second one.
*/
#define D_INTERNAL_BUBBLE_AT(_base,                                         \
                             _i)                                            \
    ( (void*)((_base) + ((size_t)(_i) * _element_size)) )

#define D_INTERNAL_BUBBLE_LESS(_cmp,                                        \
                               _carry,                                      \
                               _base,                                       \
                               _i)                                          \
    ( ( ((_cmp)->order == D_SORT_ORDER_DESCENDING)                          \
            ? (_cmp)->compare(D_INTERNAL_BUBBLE_AT(_base, (_i) - 1),        \
                              D_INTERNAL_BUBBLE_AT(_base, _i),              \
                              (_cmp)->context)                              \
            : (_cmp)->compare(D_INTERNAL_BUBBLE_AT(_base, _i),              \
                              D_INTERNAL_BUBBLE_AT(_base, (_i) - 1),        \
                              (_cmp)->context)                              \
      ) < 0 )

#define D_INTERNAL_BUBBLE_CARRY(_carry,                                     \
                                _base,                                      \
                                _i)                                         \
    ( (void)(_carry) )

#define D_INTERNAL_BUBBLE_EXCHANGE(_carry,                                  \
                                   _base,                                   \
                                   _i)                                      \
    d_internal_bubble_exchange_bytes(                                       \
        (char*)D_INTERNAL_BUBBLE_AT(_base, (_i) - 1),                       \
        (char*)D_INTERNAL_BUBBLE_AT(_base, _i),                             \
        _element_size)

/*
d_bubble_sort
  Sorts `_count` elements of `_element_size` bytes at `_base` in place, under
`_comparator`.
  The width is not known until the call, so the exchange is a byte loop and the
comparison an indirect call. The control flow is D_BUBBLE_SORT_BODY, the same
text the typed instantiations above expand and the same text the C++ face
expands, so this produces the permutation they produce -- which matters because
that is not implied by producing a sorted range.
  Nothing is allocated. An invalid argument leaves the range untouched, so a
caller may correct and retry against unmodified input.

Parameter(s):
  _base:         the range; may be NULL only when `_count` is 0.
  _count:        element count.
  _element_size: element width in bytes; must be non-zero.
  _comparator:   the ordering; must be non-NULL with a non-NULL compare.
Return:
  D_SORT_STATUS_OK on success, D_SORT_STATUS_INVALID_ARGUMENT otherwise.
*/
enum d_sort_status
d_bubble_sort
(
    void*                           _base,
    size_t                          _count,
    size_t                          _element_size,
    const struct d_sort_comparator* _comparator
)
{
    char* elements;

    // an ordering is the whole operation; a zero width is not a range
    if ( (!_comparator)          ||
         (!_comparator->compare) ||
         (_element_size == 0)    )
    {
        return D_SORT_STATUS_INVALID_ARGUMENT;
    }

    // a null base is a range only when it is an empty one
    if ( (!_base) &&
         (_count != 0) )
    {
        return D_SORT_STATUS_INVALID_ARGUMENT;
    }

    elements = (char*)_base;

    D_BUBBLE_SORT_BODY(size_t,
                       char,
                       D_INTERNAL_BUBBLE_LESS,
                       D_INTERNAL_BUBBLE_CARRY,
                       D_INTERNAL_BUBBLE_EXCHANGE,
                       _comparator,
                       elements,
                       _count);

    return D_SORT_STATUS_OK;
}

#undef D_INTERNAL_BUBBLE_EXCHANGE
#undef D_INTERNAL_BUBBLE_CARRY
#undef D_INTERNAL_BUBBLE_LESS
#undef D_INTERNAL_BUBBLE_AT
