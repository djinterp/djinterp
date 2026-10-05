/*******************************************************************************
* djinterp [c]                                                        sequence.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/sequence.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/sequence.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcpy
// djinterp
#include "../../../../inc/djinterp/c/functional/reducer.h"     // d_reducing_state
#include "../../../../inc/djinterp/c/functional/transducer.h"  // d_transducer_emit


/*
d_internal_sequence_swap
  Exchanges two elements byte-wise, so no element-sized temporary is needed and
`reverse` and `sorted` stay allocation-free at any element width.

Parameter(s):
  _left:         the first element.
  _right:        the second element.
  _element_size: the width to exchange.
Return:
  none.
*/
static void
d_internal_sequence_swap
(
    unsigned char* _left,
    unsigned char* _right,
    size_t         _element_size
)
{
    unsigned char scratch;
    size_t        index;

    // swapping an element with itself would be wasted work
    if (_left == _right)
    {
        return;
    }

    // a byte at a time, so the width may be anything
    for (index = 0; index < _element_size; index = index + 1)
    {
        scratch    = _left[index];
        _left[index]  = _right[index];
        _right[index] = scratch;
    }

    return;
}

/*
d_internal_sequence_sift
  Restores the heap property at `_root` over a heap of `_count` elements. The
loop is iterative rather than recursive, so the sort needs no stack bound.

Parameter(s):
  _base:         the array's first element.
  _count:        the heap's size.
  _root:         the index to sift down from.
  _element_size: the stride between elements.
  _precedes:     the strict ordering.
  _context:      context forwarded to `_precedes`.
Return:
  none.
*/
static void
d_internal_sequence_sift
(
    unsigned char*      _base,
    size_t              _count,
    size_t              _root,
    size_t              _element_size,
    fn_binary_predicate _precedes,
    void*               _context
)
{
    size_t root;
    size_t child;
    size_t largest;

    root = _root;

    // walk down while the node still has at least one child
    while (((2 * root) + 1) < _count)
    {
        child   = (2 * root) + 1;
        largest = root;

        // the left child outranks the root when the root precedes it
        if (_precedes(_base + (largest * _element_size),
                      _base + (child * _element_size),
                      _context))
        {
            largest = child;
        }

        // and the right child, when it exists, may outrank both
        if ((child + 1) < _count)
        {
            if (_precedes(_base + (largest * _element_size),
                          _base + ((child + 1) * _element_size),
                          _context))
            {
                largest = child + 1;
            }
        }

        // the heap property already holds from here down
        if (largest == root)
        {
            return;
        }

        d_internal_sequence_swap(_base + (root * _element_size),
                                 _base + (largest * _element_size),
                                 _element_size);

        root = largest;
    }

    return;
}

/*
d_internal_sequence_distinct_admits
  Reports whether an element is novel, recording it when there is room.

Parameter(s):
  _state:   the seen-buffer state.
  _element: the element to test.
Return:
  A boolean value corresponding to either:
  - true, if the element had not been seen, or
  - false, if it had, or the state is unusable.
*/
static bool
d_internal_sequence_distinct_admits
(
    struct d_distinct_state* _state,
    const void*              _element
)
{
    unsigned char* cursor;
    size_t         index;

    // an unusable state admits nothing
    if ( (!_state)          ||
         (!_state->seen)    ||
         (!_state->equals)  ||
         (!_element)        )
    {
        return false;
    }

    cursor = (unsigned char*)_state->seen;

    // a linear scan of what has been kept so far
    for (index = 0; index < _state->count; index = index + 1)
    {
        if (_state->equals((const void*)(cursor +
                                         (index * _state->element_size)),
                           _element,
                           _state->context))
        {
            return false;
        }
    }

    // a saturated buffer still admits novel elements, but stops recording them
    if (_state->count >= _state->capacity)
    {
        _state->overflow = _state->overflow + 1;

        return true;
    }

    memcpy(cursor + (_state->count * _state->element_size),
           _element,
           _state->element_size);

    _state->count = _state->count + 1;

    return true;
}

/*
d_internal_sequence_step_flat_map
  The Expand stage: expands one value into up to `capacity` outputs and emits
each downstream in turn, stopping early if a downstream stage latches.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_sequence_step_flat_map
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;
    struct d_flat_map_state*   binding;
    unsigned char*             cursor;
    size_t                     produced;
    size_t                     emitted;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // the expander and its buffer travel together in the stage's context
    if (!stage->context)
    {
        return;
    }

    binding = (struct d_flat_map_state*)stage->context;

    // without an expander or somewhere to expand into, the value is dropped
    if ( (!binding->expander) ||
         (!binding->scratch)  ||
         (binding->capacity == 0) )
    {
        return;
    }

    produced = binding->expander(_element,
                                 binding->scratch,
                                 binding->capacity,
                                 binding->context);

    // an expander must not overrun the capacity it was given
    if (produced > binding->capacity)
    {
        produced = binding->capacity;
    }

    cursor = (unsigned char*)binding->scratch;

    // emit each produced value, honouring a downstream that latches early
    for (emitted = 0; emitted < produced; emitted = emitted + 1)
    {
        if (_state->done)
        {
            return;
        }

        d_transducer_emit(_transducer,
                          _index + 1,
                          _state,
                          (const void*)(cursor +
                                        (emitted * binding->out_size)));
    }

    return;
}

/*
d_internal_sequence_step_distinct
  The distinct stage: emits a value only the first time it is seen.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_sequence_step_distinct
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // the seen-buffer lives in the stage's context
    if (!stage->context)
    {
        return;
    }

    // only a novel value reaches the next stage
    if (d_internal_sequence_distinct_admits(
            (struct d_distinct_state*)stage->context,
            _element))
    {
        d_transducer_emit(_transducer, _index + 1, _state, _element);
    }

    return;
}


/*
d_reverse
  Reverses an array in place, byte-wise, so no temporary is needed.

Parameter(s):
  _elements:     the array to reverse; may be NULL only if `_count` is 0.
  _count:        the number of elements.
  _element_size: the stride between elements; must be non-zero.
Return:
  A boolean value corresponding to either:
  - true, if the array was reversed (a count below 2 is already reversed), or
  - false, if the parameters were unusable.
*/
bool
d_reverse
(
    void*  _elements,
    size_t _count,
    size_t _element_size
)
{
    unsigned char* base;
    size_t         low;
    size_t         high;

    // an unusable request reverses nothing
    if ( (_element_size == 0)             ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    // zero or one element is its own reversal
    if (_count < 2)
    {
        return true;
    }

    base = (unsigned char*)_elements;
    low  = 0;
    high = _count - 1;

    // walk the two ends toward each other
    while (low < high)
    {
        d_internal_sequence_swap(base + (low * _element_size),
                                 base + (high * _element_size),
                                 _element_size);

        low  = low + 1;
        high = high - 1;
    }

    return true;
}

/*
d_reverse_into
  Writes an array's elements in reverse into a separate destination, leaving the
source untouched.

Parameter(s):
  _elements:     the source array.
  _count:        the number of elements.
  _element_size: the stride between elements; must be non-zero.
  _out_array:    a destination of at least `_count` elements; must not overlap
                 the source.
Return:
  A boolean value corresponding to either:
  - true, if the reversal was written, or
  - false, if the parameters were unusable.
*/
bool
d_reverse_into
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    void*       _out_array
)
{
    const unsigned char* source;
    unsigned char*       destination;
    size_t               index;

    // an unusable request writes nothing
    if ( (_element_size == 0)               ||
         ( (!_elements) && (_count > 0) )   ||
         ( (!_out_array) && (_count > 0) )  )
    {
        return false;
    }

    source      = (const unsigned char*)_elements;
    destination = (unsigned char*)_out_array;

    // copy from the back of the source to the front of the destination
    for (index = 0; index < _count; index = index + 1)
    {
        memcpy(destination + (index * _element_size),
               source + ((_count - 1 - index) * _element_size),
               _element_size);
    }

    return true;
}

/*
d_sorted
  Sorts an array in place under a strict ordering, by heapsort: O(n log n)
comparisons, no scratch buffer, and no recursion, so it neither allocates nor
bounds the stack.
  Heapsort is not stable. Where stability matters the caller should carry an
index and break ties on it, which is the allocation-free way to get it.

Parameter(s):
  _elements:     the array to sort; may be NULL only if `_count` is 0.
  _count:        the number of elements.
  _element_size: the stride between elements; must be non-zero.
  _precedes:     the strict ordering, read as "left strictly precedes right".
  _context:      context forwarded to `_precedes`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the array was sorted (a count below 2 is already sorted), or
  - false, if the parameters were unusable.
*/
bool
d_sorted
(
    void*               _elements,
    size_t              _count,
    size_t              _element_size,
    fn_binary_predicate _precedes,
    void*               _context
)
{
    unsigned char* base;
    size_t         index;
    size_t         remaining;

    // an unusable request sorts nothing
    if ( (!_precedes)                     ||
         (_element_size == 0)             ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    // zero or one element is already sorted
    if (_count < 2)
    {
        return true;
    }

    base = (unsigned char*)_elements;

    // build the heap from the last internal node upward
    index = _count / 2;

    while (index > 0)
    {
        index = index - 1;

        d_internal_sequence_sift(base, _count, index, _element_size,
                                 _precedes, _context);
    }

    // repeatedly move the largest element to the end and re-sift
    remaining = _count;

    while (remaining > 1)
    {
        remaining = remaining - 1;

        d_internal_sequence_swap(base,
                                 base + (remaining * _element_size),
                                 _element_size);

        d_internal_sequence_sift(base, remaining, 0, _element_size,
                                 _precedes, _context);
    }

    return true;
}

/*
d_is_sorted
  Reports whether an array is already in order under a strict ordering. An empty
or single-element array is vacuously sorted.

Parameter(s):
  _elements:     the array to inspect.
  _count:        the number of elements.
  _element_size: the stride between elements; must be non-zero.
  _precedes:     the strict ordering.
  _context:      context forwarded to `_precedes`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if no element strictly precedes its predecessor, or
  - false, if one does or the parameters were unusable.
*/
bool
d_is_sorted
(
    const void*         _elements,
    size_t              _count,
    size_t              _element_size,
    fn_binary_predicate _precedes,
    void*               _context
)
{
    const unsigned char* base;
    size_t               index;

    // an unusable request cannot be judged sorted
    if ( (!_precedes)                     ||
         (_element_size == 0)             ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    // zero or one element is vacuously sorted
    if (_count < 2)
    {
        return true;
    }

    base = (const unsigned char*)_elements;

    // any element strictly preceding its predecessor breaks the order
    for (index = 1; index < _count; index = index + 1)
    {
        if (_precedes((const void*)(base + (index * _element_size)),
                      (const void*)(base + ((index - 1) * _element_size)),
                      _context))
        {
            return false;
        }
    }

    return true;
}

/*
d_zip
  Pairs two arrays elementwise, writing each output as the left element's bytes
followed immediately by the right element's. The destination stride is therefore
`_left_size + _right_size`.

Parameter(s):
  _left:       the first array.
  _left_size:  the stride of the first array; must be non-zero.
  _right:      the second array.
  _right_size: the stride of the second array; must be non-zero.
  _count:      the number of pairs to form.
  _out_array:  the destination.
  _capacity:   the number of pairs the destination can hold.
Return:
  The number of pairs written.
*/
size_t
d_zip
(
    const void* _left,
    size_t      _left_size,
    const void* _right,
    size_t      _right_size,
    size_t      _count,
    void*       _out_array,
    size_t      _capacity
)
{
    const unsigned char* left;
    const unsigned char* right;
    unsigned char*       destination;
    size_t               limit;
    size_t               index;

    // an unusable request writes nothing
    if ( (!_left)             ||
         (!_right)            ||
         (!_out_array)        ||
         (_left_size == 0)    ||
         (_right_size == 0)   )
    {
        return 0;
    }

    left        = (const unsigned char*)_left;
    right       = (const unsigned char*)_right;
    destination = (unsigned char*)_out_array;

    // the shorter of the request and the capacity bounds the run
    limit = (_count < _capacity) ? _count : _capacity;

    // each pair is the two elements laid end to end
    for (index = 0; index < limit; index = index + 1)
    {
        memcpy(destination + (index * (_left_size + _right_size)),
               left + (index * _left_size),
               _left_size);

        memcpy(destination + (index * (_left_size + _right_size)) + _left_size,
               right + (index * _right_size),
               _right_size);
    }

    return limit;
}

/*
d_zip_with
  Combines two arrays elementwise through a zipper, which is `zip` followed by
`map` without materialising the pairs. A zipper reporting failure drops that
position rather than ending the run, so the result may be shorter than the
inputs.

Parameter(s):
  _left:       the first array.
  _left_size:  the stride of the first array; must be non-zero.
  _right:      the second array.
  _right_size: the stride of the second array; must be non-zero.
  _count:      the number of positions to combine.
  _out_array:  the destination.
  _out_size:   the stride of the destination; must be non-zero.
  _capacity:   the number of outputs the destination can hold.
  _zipper:     the combining function.
  _context:    context forwarded to `_zipper`; may be NULL.
Return:
  The number of outputs written.
*/
size_t
d_zip_with
(
    const void* _left,
    size_t      _left_size,
    const void* _right,
    size_t      _right_size,
    size_t      _count,
    void*       _out_array,
    size_t      _out_size,
    size_t      _capacity,
    fn_zipper   _zipper,
    void*       _context
)
{
    const unsigned char* left;
    const unsigned char* right;
    unsigned char*       destination;
    size_t               limit;
    size_t               index;
    size_t               written;

    // an unusable request writes nothing
    if ( (!_left)            ||
         (!_right)           ||
         (!_out_array)       ||
         (!_zipper)          ||
         (_left_size == 0)   ||
         (_right_size == 0)  ||
         (_out_size == 0)    )
    {
        return 0;
    }

    left        = (const unsigned char*)_left;
    right       = (const unsigned char*)_right;
    destination = (unsigned char*)_out_array;
    written     = 0;

    limit = (_count < _capacity) ? _count : _capacity;

    // a zipper that declines a position simply produces no output for it
    for (index = 0; index < limit; index = index + 1)
    {
        if (_zipper((const void*)(left + (index * _left_size)),
                    (const void*)(right + (index * _right_size)),
                    (void*)(destination + (written * _out_size)),
                    _context))
        {
            written = written + 1;
        }
    }

    return written;
}

/*
d_flat_map
  Expands every element into zero or more outputs and concatenates them. This is
the Expand row: n elements in, m out, with m unrelated to n.

Parameter(s):
  _elements:     the source array.
  _count:        the number of source elements.
  _element_size: the stride of the source; must be non-zero.
  _expander:     the one-to-many arrow.
  _context:      context forwarded to `_expander`; may be NULL.
  _out_array:    the destination.
  _out_size:     the stride of the destination; must be non-zero.
  _capacity:     the number of outputs the destination can hold.
Return:
  The number of outputs written, which stops at the capacity.
*/
size_t
d_flat_map
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    fn_expander _expander,
    void*       _context,
    void*       _out_array,
    size_t      _out_size,
    size_t      _capacity
)
{
    const unsigned char* source;
    unsigned char*       destination;
    size_t               index;
    size_t               written;
    size_t               produced;

    // an unusable request writes nothing
    if ( (!_expander)         ||
         (!_out_array)        ||
         (_element_size == 0) ||
         (_out_size == 0)     ||
         ( (!_elements) && (_count > 0) ) )
    {
        return 0;
    }

    source      = (const unsigned char*)_elements;
    destination = (unsigned char*)_out_array;
    written     = 0;

    // expand straight into the remaining capacity, so nothing is buffered twice
    for (index = 0; index < _count; index = index + 1)
    {
        if (written >= _capacity)
        {
            break;
        }

        produced = _expander((const void*)(source + (index * _element_size)),
                             (void*)(destination + (written * _out_size)),
                             _capacity - written,
                             _context);

        // an expander must not overrun the capacity it was given
        if (produced > (_capacity - written))
        {
            produced = _capacity - written;
        }

        written = written + produced;
    }

    return written;
}

/*
d_slice
  Copies the half-open index range [_begin, _end) into a destination. An
inverted or out-of-range request yields nothing rather than clamping silently in
a way the caller cannot detect.

Parameter(s):
  _elements:     the source array.
  _count:        the number of source elements.
  _element_size: the stride; must be non-zero.
  _begin:        the first index copied.
  _end:          one past the last index copied.
  _out_array:    the destination.
  _capacity:     the number of elements the destination can hold.
Return:
  The number of elements written.
*/
size_t
d_slice
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _begin,
    size_t      _end,
    void*       _out_array,
    size_t      _capacity
)
{
    const unsigned char* source;
    size_t               span;

    // an unusable or inverted request copies nothing
    if ( (!_elements)         ||
         (!_out_array)        ||
         (_element_size == 0) ||
         (_begin >= _end)     ||
         (_begin >= _count)   )
    {
        return 0;
    }

    source = (const unsigned char*)_elements;

    // the range is clipped to the array and then to the capacity
    span = ((_end < _count) ? _end : _count) - _begin;

    if (span > _capacity)
    {
        span = _capacity;
    }

    memcpy(_out_array,
           source + (_begin * _element_size),
           span * _element_size);

    return span;
}

/*
d_distinct_state_init
  Initialises a seen-buffer over caller-owned storage.

Parameter(s):
  _state:        the state to initialise.
  _seen:         caller-owned buffer of `_capacity` slots.
  _capacity:     the number of distinct elements that can be remembered.
  _element_size: the stride between slots; must be non-zero.
  _equals:       the equality test.
  _context:      context forwarded to `_equals`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the state was initialised, or
  - false, if the parameters were unusable.
*/
bool
d_distinct_state_init
(
    struct d_distinct_state* _state,
    void*                    _seen,
    size_t                   _capacity,
    size_t                   _element_size,
    fn_binary_predicate      _equals,
    void*                    _context
)
{
    // an unusable request leaves the state untouched
    if ( (!_state)            ||
         (!_seen)             ||
         (!_equals)           ||
         (_element_size == 0) ||
         (_capacity == 0)     )
    {
        return false;
    }

    _state->seen         = _seen;
    _state->capacity     = _capacity;
    _state->count        = 0;
    _state->element_size = _element_size;
    _state->equals       = _equals;
    _state->context      = _context;
    _state->overflow     = 0;

    return true;
}

/*
d_distinct
  Copies the first occurrence of each distinct element into a destination.

Parameter(s):
  _elements:     the source array.
  _count:        the number of source elements.
  _state:        an initialised seen-buffer.
  _out_array:    the destination.
  _out_capacity: the number of elements the destination can hold.
Return:
  The number of distinct elements written.
*/
size_t
d_distinct
(
    const void*              _elements,
    size_t                   _count,
    struct d_distinct_state* _state,
    void*                    _out_array,
    size_t                   _out_capacity
)
{
    const unsigned char* source;
    unsigned char*       destination;
    size_t               index;
    size_t               written;

    // an unusable request writes nothing
    if ( (!_state)      ||
         (!_out_array)  ||
         ( (!_elements) && (_count > 0) ) )
    {
        return 0;
    }

    source      = (const unsigned char*)_elements;
    destination = (unsigned char*)_out_array;
    written     = 0;

    // keep the first occurrence of each distinct element
    for (index = 0; index < _count; index = index + 1)
    {
        if (written >= _out_capacity)
        {
            break;
        }

        if (d_internal_sequence_distinct_admits(
                _state,
                (const void*)(source + (index * _state->element_size))))
        {
            memcpy(destination + (written * _state->element_size),
                   source + (index * _state->element_size),
                   _state->element_size);

            written = written + 1;
        }
    }

    return written;
}

/*
d_find_last
  Returns the last element satisfying a predicate, walking backwards so the
search short-circuits at the first match from the end.

Parameter(s):
  _elements:     the array to search.
  _count:        the number of elements.
  _element_size: the stride; must be non-zero.
  _predicate:    the test.
  _context:      context forwarded to `_predicate`; may be NULL.
Return:
  A pointer to the last satisfying element, or NULL if there is none.
*/
void*
d_find_last
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context
)
{
    const unsigned char* base;
    size_t               remaining;

    // an unusable request finds nothing
    if ( (!_predicate)        ||
         (_element_size == 0) ||
         ( (!_elements) && (_count > 0) ) )
    {
        return NULL;
    }

    base      = (const unsigned char*)_elements;
    remaining = _count;

    // walk backwards, stopping at the first match from the end
    while (remaining > 0)
    {
        remaining = remaining - 1;

        if (_predicate((const void*)(base + (remaining * _element_size)),
                       _context))
        {
            return (void*)(base + (remaining * _element_size));
        }
    }

    return NULL;
}

/*
d_index_of
  Reports the index of the first element satisfying a predicate. The index is
returned through a parameter rather than as a sentinel, because every candidate
sentinel is a legitimate index.

Parameter(s):
  _elements:     the array to search.
  _count:        the number of elements.
  _element_size: the stride; must be non-zero.
  _predicate:    the test.
  _context:      context forwarded to `_predicate`; may be NULL.
  _out_index:    receives the index when one is found.
Return:
  A boolean value corresponding to either:
  - true, if a satisfying element was found, or
  - false, otherwise.
*/
bool
d_index_of
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context,
    size_t*      _out_index
)
{
    const unsigned char* base;
    size_t               index;

    // an unusable request finds nothing
    if ( (!_predicate)        ||
         (!_out_index)        ||
         (_element_size == 0) ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    base = (const unsigned char*)_elements;

    // stop at the first match
    for (index = 0; index < _count; index = index + 1)
    {
        if (_predicate((const void*)(base + (index * _element_size)),
                       _context))
        {
            *_out_index = index;

            return true;
        }
    }

    return false;
}

/*
d_last_index_of
  Reports the index of the last element satisfying a predicate.

Parameter(s):
  _elements:     the array to search.
  _count:        the number of elements.
  _element_size: the stride; must be non-zero.
  _predicate:    the test.
  _context:      context forwarded to `_predicate`; may be NULL.
  _out_index:    receives the index when one is found.
Return:
  A boolean value corresponding to either:
  - true, if a satisfying element was found, or
  - false, otherwise.
*/
bool
d_last_index_of
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context,
    size_t*      _out_index
)
{
    const unsigned char* base;
    size_t               remaining;

    // an unusable request finds nothing
    if ( (!_predicate)        ||
         (!_out_index)        ||
         (_element_size == 0) ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    base      = (const unsigned char*)_elements;
    remaining = _count;

    // walk backwards, stopping at the first match from the end
    while (remaining > 0)
    {
        remaining = remaining - 1;

        if (_predicate((const void*)(base + (remaining * _element_size)),
                       _context))
        {
            *_out_index = remaining;

            return true;
        }
    }

    return false;
}

/*
d_transducer_distinct
  Builds a stage emitting each value only the first time it is seen. The state
is borrowed and carries the mutable run state, so a chain reused across drives
must have its distinct state re-initialised as well as being reset.

Parameter(s):
  _state: an initialised, caller-owned seen-buffer.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_distinct
(
    struct d_distinct_state* _state
)
{
    struct d_transducer_stage stage;

    stage.step               = &d_internal_sequence_step_distinct;
    stage.callable.predicate = NULL;
    stage.context            = _state;
    stage.scratch            = NULL;
    stage.bound              = 0;
    stage.seen               = 0;
    stage.latched            = false;

    return stage;
}

/*
d_precedes_from_comparator
  Reads a three-way comparator as the bool-valued ordering the chapter
specifies: "left precedes right" is "the comparison is negative".

Parameter(s):
  _left:                the left operand.
  _right:               the right operand.
  _comparator_binding:  a caller-owned `d_comparator_binding`.
Return:
  A boolean value corresponding to either:
  - true, if the comparison is negative, or
  - false, otherwise.
*/
bool
d_precedes_from_comparator
(
    const void* _left,
    const void* _right,
    void*       _comparator_binding
)
{
    struct d_comparator_binding* binding;

    // an unusable binding orders nothing
    if (!_comparator_binding)
    {
        return false;
    }

    binding = (struct d_comparator_binding*)_comparator_binding;

    // an unset comparison leaves every pair unordered
    if (!binding->compare)
    {
        return false;
    }

    return (binding->compare(_left, _right, binding->context) < 0);
}

/*
d_flat_map_state_init
  Initialises the binding backing a `flat_map` stage.

Parameter(s):
  _state:    the binding to initialise.
  _expander: the one-to-many arrow.
  _context:  context forwarded to `_expander`; may be NULL.
  _scratch:  caller-owned buffer of `_capacity` outputs.
  _capacity: the number of outputs the buffer can hold; must be non-zero.
  _out_size: the stride of an output; must be non-zero.
Return:
  A boolean value corresponding to either:
  - true, if the binding was initialised, or
  - false, if the parameters were unusable.
*/
bool
d_flat_map_state_init
(
    struct d_flat_map_state* _state,
    fn_expander              _expander,
    void*                    _context,
    void*                    _scratch,
    size_t                   _capacity,
    size_t                   _out_size
)
{
    // an unusable request leaves the binding untouched
    if ( (!_state)        ||
         (!_expander)     ||
         (!_scratch)      ||
         (_capacity == 0) ||
         (_out_size == 0) )
    {
        return false;
    }

    _state->expander = _expander;
    _state->context  = _context;
    _state->scratch  = _scratch;
    _state->capacity = _capacity;
    _state->out_size = _out_size;

    return true;
}

/*
d_transducer_flat_map
  Builds a stage expanding each value into zero or more outputs. This is the one
stage that can make a chain longer than its input, so a downstream `take` is the
only thing bounding it.

Parameter(s):
  _state: an initialised, caller-owned binding.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_flat_map
(
    struct d_flat_map_state* _state
)
{
    struct d_transducer_stage stage;

    stage.step               = &d_internal_sequence_step_flat_map;
    stage.callable.predicate = NULL;
    stage.context            = _state;
    stage.scratch            = NULL;
    stage.bound              = 0;
    stage.seen               = 0;
    stage.latched            = false;

    return stage;
}
