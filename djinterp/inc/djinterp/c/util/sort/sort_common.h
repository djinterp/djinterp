/******************************************************************************
* djinterp [utility]                                             sort_common.h
*
*   The C sort subsystem's shared base.
* Everything here is vocabulary the algorithms have in common: how an ordering
* is expressed, how a call reports failure, and how one element is reached and
* compared.  No algorithm lives here, and nothing here knows which algorithms
* exist.
*
*   DIRECTION IS A PARAMETER, NOT A PROPERTY OF THE ORDERING.
* A comparator answers "how do these two relate"; ascending or descending is
* the caller's question about a particular call.  Keeping them separate means
* one comparator serves both directions without being copied or rebuilt, and
* d_sort_precedes applies the direction once, by exchanging its operands, so no
* algorithm needs a second code path for descending.
*
*   THREE-WAY, NOT A PREDICATE.
* d_sort_compare_fn is identical in signature to the framework's
* fn_function_comparator (functional/functional_common.h), so a comparator
* written for either is accepted by the other.  When the functional layer is in
* the build this typedef should become an alias for it; it is spelled out here
* so the sort subsystem builds standalone.
*
*   NOTHING IS ALLOCATED, BY ANYONE.
* No function in the C sort subsystem calls malloc.  In-place algorithms need
* no storage beyond an exchange temporary, which lives on the stack; an
* algorithm needing scratch takes the buffer from its caller and reports
* D_SORT_STATUS_BUFFER_TOO_SMALL rather than allocating one.
*
*
* path:      /djinterp/c/util/sort/sort_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.07
*                                                         revised: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_COMMON_
#define DJINTERP_UTILITY_SORT_COMMON_ 1

// std
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../../djinterp.h"
#include "../swap.h"


D_EXTERN_C_BEGIN


// I.     status

// d_sort_status
//   enum: the outcome of a sort call. Values are pinned and must not be
// reordered. The set is split: a FORMAL status says the operation was not
// defined and no amount of storage would help, a MECHANICAL status says it
// was defined and the machinery ran short. The first is a bug, the second a
// resource decision.
enum d_sort_status
{
    D_SORT_STATUS_OK               = 0,

    D_SORT_STATUS_INVALID_ARGUMENT = 0x001,
    D_SORT_STATUS_OVERFLOW         = 0x002,

    D_SORT_STATUS_BUFFER_TOO_SMALL = 0x100
};

// D_SORT_STATUS_MECHANICAL_FLOOR
//   macro: the first mechanical status. A status at or above this value
// describes the machinery; below it, the request itself.
#define D_SORT_STATUS_MECHANICAL_FLOOR  0x100

// D_SORT_STATUS_IS_FORMAL
//   macro: 1 when the status reports that the operation is not defined.
#define D_SORT_STATUS_IS_FORMAL(_status)                                    \
    ( ((_status) != D_SORT_STATUS_OK) &&                                    \
      ((_status) <  D_SORT_STATUS_MECHANICAL_FLOOR) )

// D_SORT_STATUS_IS_MECHANICAL
//   macro: 1 when the status reports that the machinery ran short.
#define D_SORT_STATUS_IS_MECHANICAL(_status)                                \
    ( (_status) >= D_SORT_STATUS_MECHANICAL_FLOOR )


// II.    ordering

// d_sort_order
//   enum: the direction a sort arranges its range in. NONE is treated as
// ascending everywhere; it exists so a zero-initialised request is usable
// rather than meaningless.
enum d_sort_order
{
    D_SORT_ORDER_NONE       = 0,
    D_SORT_ORDER_ASCENDING  = 1,
    D_SORT_ORDER_DESCENDING = 2
};

// d_sort_compare_fn
//   type: a three-way comparison returning negative when the first operand
// precedes the second, zero when neither precedes the other, and positive
// otherwise. The context may be NULL.
typedef int (*d_sort_compare_fn)(const void* _first,
                                 const void* _second,
                                 void*       _context);

// d_sort_comparator
//   struct: an ordering -- a three-way comparison and the context it reads.
// A comparison that is not a strict weak ordering is a caller error that no
// algorithm detects; the consequence is an arbitrary arrangement, not an
// out-of-range access, since every loop is bounded by the element count.
struct d_sort_comparator
{
    d_sort_compare_fn compare;   // three-way ordering; never NULL
    void*             context;   // instance configuration; may be NULL
};


// III.   ordering construction
struct d_sort_comparator d_sort_comparator_make(d_sort_compare_fn _compare,
                                                void*             _context);

// IV.    request validation
enum d_sort_status d_sort_verify(const void*                     _base,
                                 size_t                          _count,
                                 size_t                          _elem_size,
                                 const struct d_sort_comparator* _comparator);

// V.     range predicates
bool d_sort_is_sorted(const void*                     _base,
                      size_t                          _count,
                      size_t                          _elem_size,
                      const struct d_sort_comparator* _comparator,
                      enum d_sort_order               _order);


// VI.    element access

/*
d_sort_at
  The address of element `_index` in a range of `_elem_size`-byte elements
based at `_base`.

Parameter(s):
  _base:         the range; never NULL.
  _index:        element index; must be within the range.
  _elem_size: element width in bytes.
Return:
  the address of the element.
*/
D_STATIC_INLINE void*
d_sort_at
(
    void*  _base,
    size_t _index,
    size_t _elem_size
)
{
    return (void*)( ((char*)_base) + (_index * _elem_size) );
}

/*
d_sort_at_const
  d_sort_at over a range the caller only reads.

Parameter(s):
  _base:         the range; never NULL.
  _index:        element index; must be within the range.
  _elem_size: element width in bytes.
Return:
  the address of the element.
*/
D_STATIC_INLINE const void*
d_sort_at_const
(
    const void* _base,
    size_t      _index,
    size_t      _elem_size
)
{
    return (const void*)( ((const char*)_base) + (_index * _elem_size) );
}

/*
d_sort_precedes
  Whether `_first` strictly precedes `_second` under the ordering, in the
requested direction.
  Descending asks the same question with the operands exchanged, which is the
whole cost of direction: every algorithm asks only this, and none of them
contains a second code path for it.
  STRICT is the operative word. Returning false for equivalent elements is what
makes a stable algorithm stable, so no caller should relax it.

Parameter(s):
  _comparator: the ordering; must be non-NULL with a non-NULL compare.
  _order:      the direction; NONE behaves as ascending.
  _first:      left operand; never NULL.
  _second:     right operand; never NULL.
Return:
  true when `_first` strictly precedes `_second` in the requested direction.
*/
D_STATIC_INLINE bool
d_sort_precedes
(
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order,
    const void*                     _first,
    const void*                     _second
)
{
    if (_order == D_SORT_ORDER_DESCENDING)
    {
        return (_comparator->compare(_second,
                                     _first,
                                     _comparator->context) < 0);
    }

    return (_comparator->compare(_first,
                                 _second,
                                 _comparator->context) < 0);
}


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_COMMON_
