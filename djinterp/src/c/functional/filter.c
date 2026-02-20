#include "..\..\inc\functional\filter.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    FILTER OPERATION CREATION                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_take_first
  Creates a filter operation that takes the first n elements.

Parameter(s):
  _n: the number of elements to take from the beginning.
Return:
  A d_filter_operation configured for take-first behavior.
*/
struct d_filter_operation*
d_filter_take_first
(
    size_t _n
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_TAKE_FIRST;
    op->params.count = _n;

    return op;
}

/*
d_filter_take_last
  Creates a filter operation that takes the last n elements.

Parameter(s):
  _n: the number of elements to take from the end.
Return:
  A d_filter_operation configured for take-last behavior.
*/
struct d_filter_operation*
d_filter_take_last
(
    size_t _n
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_TAKE_LAST;
    op->params.count = _n;

    return op;
}

/*
d_filter_take_nth
  Creates a filter operation that takes every nth element.

Parameter(s):
  _n: the step between selected elements (0 defaults to 1).
Return:
  A d_filter_operation configured for take-nth behavior.
*/
struct d_filter_operation*
d_filter_take_nth
(
    size_t _n
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type        = D_FILTER_OP_TAKE_NTH;
    op->params.step = (_n == 0) ? 1 : _n;

    return op;
}

/*
d_filter_head
  Creates a filter operation that takes only the first element.

Parameter(s):
  (none)
Return:
  A d_filter_operation configured for head behavior.
*/
struct d_filter_operation*
d_filter_head
(
    void
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_HEAD;
    op->params.count = 1;

    return op;
}

/*
d_filter_tail
  Creates a filter operation that takes only the last element.

Parameter(s):
  (none)
Return:
  A d_filter_operation configured for tail behavior.
*/
struct d_filter_operation*
d_filter_tail
(
    void
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_TAIL;
    op->params.count = 1;

    return op;
}

/*
d_filter_skip_first
  Creates a filter operation that skips the first n elements.

Parameter(s):
  _n: the number of elements to skip from the beginning.
Return:
  A d_filter_operation configured for skip-first behavior.
*/
struct d_filter_operation*
d_filter_skip_first
(
    size_t _n
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_SKIP_FIRST;
    op->params.count = _n;

    return op;
}

/*
d_filter_skip_last
  Creates a filter operation that skips the last n elements.

Parameter(s):
  _n: the number of elements to skip from the end.
Return:
  A d_filter_operation configured for skip-last behavior.
*/
struct d_filter_operation*
d_filter_skip_last
(
    size_t _n
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_SKIP_LAST;
    op->params.count = _n;

    return op;
}

/*
d_filter_init
  Creates a filter operation that takes all elements except the last.

Parameter(s):
  (none)
Return:
  A d_filter_operation configured for init behavior.
*/
struct d_filter_operation*
d_filter_init
(
    void
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type = D_FILTER_OP_INIT;

    return op;
}

/*
d_filter_rest
  Creates a filter operation that takes all elements except the first.

Parameter(s):
  (none)
Return:
  A d_filter_operation configured for rest behavior.
*/
struct d_filter_operation*
d_filter_rest
(
    void
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type = D_FILTER_OP_REST;

    return op;
}

/*
d_filter_range
  Creates a filter operation that selects elements in the range
[_start, _end).

Parameter(s):
  _start: the inclusive start index.
  _end:   the exclusive end index.
Return:
  A d_filter_operation configured for range selection.
*/
struct d_filter_operation*
d_filter_range
(
    size_t _start,
    size_t _end
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_RANGE;
    op->params.start = _start;
    op->params.end   = _end;

    return op;
}

/*
d_filter_slice
  Creates a filter operation that selects elements in a slice
[_start:_end:_step].

Parameter(s):
  _start: the inclusive start index.
  _end:   the exclusive end index.
  _step:  the step size between selected elements (0 defaults to 1).
Return:
  A d_filter_operation configured for slice selection.
*/
struct d_filter_operation*
d_filter_slice
(
    size_t _start,
    size_t _end,
    size_t _step
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_SLICE;
    op->params.start = _start;
    op->params.end   = _end;
    op->params.step  = (_step == 0) ? 1 : _step;

    return op;
}

/*
d_filter_where
  Creates a filter operation that selects elements matching a predicate.

Parameter(s):
  _test: the predicate function to test each element.
Return:
  A d_filter_operation configured for predicate-based filtering.
*/
struct d_filter_operation*
d_filter_where
(
    fn_predicate _test
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type           = D_FILTER_OP_WHERE;
    op->params.test    = _test;
    op->params.context = NULL;

    return op;
}

/*
d_filter_where_context
  Creates a filter operation that selects elements matching a predicate
with a context parameter.

Parameter(s):
  _test:    the predicate function to test each element.
  _context: the context pointer passed to the predicate.
Return:
  A d_filter_operation configured for context-predicate filtering.
*/
struct d_filter_operation*
d_filter_where_context
(
    fn_predicate _test,
    void*       _context
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type           = D_FILTER_OP_WHERE;
    op->params.test    = _test;
    op->params.context = _context;

    return op;
}

/*
d_filter_where_not
  Creates a filter operation that selects elements not matching a
predicate.

Parameter(s):
  _test: the predicate function whose result will be negated.
Return:
  A d_filter_operation configured for negated predicate filtering.
*/
struct d_filter_operation*
d_filter_where_not
(
    fn_predicate _test
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type           = D_FILTER_OP_WHERE_NOT;
    op->params.test    = _test;
    op->params.context = NULL;

    return op;
}

/*
d_filter_where_not_context
  Creates a filter operation that selects elements not matching a
predicate, with a context parameter.

Parameter(s):
  _test:    the predicate function whose result will be negated.
  _context: the context pointer passed to the predicate.
Return:
  A d_filter_operation configured for negated context-predicate
filtering.
*/
struct d_filter_operation*
d_filter_where_not_context
(
    fn_predicate _test,
    void*       _context
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type           = D_FILTER_OP_WHERE_NOT;
    op->params.test    = _test;
    op->params.context = _context;

    return op;
}

/*
d_filter_at
  Creates a filter operation that selects a single element by index.

Parameter(s):
  _index: the index of the element to select.
Return:
  A d_filter_operation configured for single-index selection.
*/
struct d_filter_operation*
d_filter_at
(
    size_t _index
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type         = D_FILTER_OP_INDICES;
    op->params.start = _index;
    op->params.count = 1;

    return op;
}

/*
d_filter_at_indices
  Creates a filter operation that selects elements at specific indices.
Copies the indices array.

Parameter(s):
  _indices: the array of indices to select.
  _count:   the number of indices.
Return:
  A d_filter_operation configured for multi-index selection.
*/
struct d_filter_operation*
d_filter_at_indices
(
    const size_t* _indices,
    size_t        _count
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type                = D_FILTER_OP_INDICES;
    op->params.indices       = NULL;
    op->params.indices_count = _count;

    if ( (_indices)  &&
         (_count > 0) )
    {
        op->params.indices = malloc(_count * sizeof(size_t));

        if (op->params.indices)
        {
            memcpy(op->params.indices,
                   _indices,
                   _count * sizeof(size_t));
        }
    }

    return op;
}

/*
d_filter_distinct
  Creates a filter operation that removes duplicate elements using a
comparator.

Parameter(s):
  _comparator: the comparator function for detecting duplicates.
Return:
  A d_filter_operation configured for deduplication.
*/
struct d_filter_operation*
d_filter_distinct
(
    fn_function_comparator _comparator
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    // ensure that memory allocation was successful
    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));

    op->type              = D_FILTER_OP_DISTINCT;
    op->params.comparator = _comparator;

    return op;
}

/*
d_filter_reverse
  Creates a filter operation that reverses element order.

Parameter(s):
  (none)
Return:
  A d_filter_operation configured for reversal.
*/
struct d_filter_operation*
d_filter_reverse
(
    void
)
{
    struct d_filter_operation* op;

    op = malloc(sizeof(struct d_filter_operation));

    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));
    op->type = D_FILTER_OP_REVERSE;

    return op;
}

/*
d_filter_operation_free
  Frees resources owned by a filter operation.

Parameter(s):
  _op: the filter operation to clean up; may be NULL.
Return:
  none.
*/
void
d_filter_operation_free
(
    struct d_filter_operation* _op
)
{
    if (!_op)
    {
        return;
    }

    if (_op->name)
    {
        free(_op->name);
        _op->name = NULL;
    }

    if (_op->params.indices)
    {
        free(_op->params.indices);
        _op->params.indices = NULL;
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             II.   FILTER CHAIN MANAGEMENT                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_chain_new
  Creates a new empty filter chain.

Parameter(s):
  (none)
Return:
  A pointer to a newly allocated d_filter_chain, or NULL if allocation
fails.
*/
struct d_filter_chain*
d_filter_chain_new
(
    void
)
{
    struct d_filter_chain* chain;

    chain = malloc(sizeof(struct d_filter_chain));

    if (!chain)
    {
        return NULL;
    }

    chain->operations      = NULL;
    chain->count           = 0;
    chain->capacity        = 0;
    chain->owns_operations = true;

    return chain;
}

/*
d_filter_chain_new_with_capacity
  Creates a new filter chain with a pre-allocated capacity.

Parameter(s):
  _capacity: the initial number of operations to allocate for.
Return:
  A pointer to a newly allocated d_filter_chain, or NULL if allocation
fails.
*/
struct d_filter_chain*
d_filter_chain_new_with_capacity
(
    size_t _capacity
)
{
    struct d_filter_chain* chain;

    chain = malloc(sizeof(struct d_filter_chain));

    if (!chain)
    {
        return NULL;
    }

    chain->operations      = NULL;
    chain->count           = 0;
    chain->capacity        = 0;
    chain->owns_operations = true;

    if (_capacity > 0)
    {
        chain->operations = malloc(_capacity * 
                                   sizeof(struct d_filter_operation));

        if (!chain->operations)
        {
            free(chain);

            return NULL;
        }

        chain->capacity = _capacity;
    }

    return chain;
}

/*
d_filter_chain_clone
  Creates a deep copy of a filter chain.

Parameter(s):
  _chain: the chain to clone; may be NULL.
Return:
  A pointer to a newly allocated copy, or NULL on failure.
*/
struct d_filter_chain*
d_filter_chain_clone
(
    const struct d_filter_chain* _chain
)
{
    struct d_filter_chain* clone;

    if (!_chain)
    {
        return NULL;
    }

    clone = d_filter_chain_new_with_capacity(_chain->capacity);

    if (!clone)
    {
        return NULL;
    }

    if ( (_chain->operations) &&
         (_chain->count > 0) )
    {
        memcpy(clone->operations,
               _chain->operations,
               _chain->count * sizeof(struct d_filter_operation));
        clone->count = _chain->count;
    }

    return clone;
}

/*
d_filter_chain_add
  Adds a filter operation to the end of a chain, growing capacity as
needed.

Parameter(s):
  _chain: the filter chain to add to.
  _op:    the filter operation to add.
Return:
  A boolean value corresponding to either:
  - true, if the operation was added successfully, or
  - false, if either parameter is NULL or allocation fails.
*/
bool
d_filter_chain_add
(
    struct d_filter_chain*           _chain,
    const struct d_filter_operation* _op
)
{
    struct d_filter_operation* new_ops;
    size_t                     new_capacity;

    if ( (!_chain) ||
         (!_op) )
    {
        return false;
    }

    // grow capacity if needed
    if (_chain->count >= _chain->capacity)
    {
        new_capacity = (_chain->capacity == 0)
                       ? 4
                       : (_chain->capacity * 2);
        new_ops = (struct d_filter_operation*)realloc(
                      _chain->operations,
                      new_capacity
                      * sizeof(struct d_filter_operation));

        if (!new_ops)
        {
            return false;
        }

        _chain->operations = new_ops;
        _chain->capacity   = new_capacity;
    }

    memcpy(&_chain->operations[_chain->count],
           _op,
           sizeof(*_op));
    _chain->count++;

    return true;
}

/*
d_filter_chain_add_take_first
  Convenience function to add a take-first operation to a chain.

Parameter(s):
  _chain: the filter chain to add to.
  _n:     the number of elements to take.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_take_first
(
    struct d_filter_chain* _chain,
    size_t                 _n
)
{
    return d_filter_chain_add(_chain, d_filter_take_first(_n));
}

/*
d_filter_chain_add_take_last
  Convenience function to add a take-last operation to a chain.

Parameter(s):
  _chain: the filter chain to add to.
  _n:     the number of elements to take.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_take_last
(
    struct d_filter_chain* _chain,
    size_t                 _n
)
{
    return d_filter_chain_add(_chain, d_filter_take_last(_n));
}

/*
d_filter_chain_add_skip_first
  Convenience function to add a skip-first operation to a chain.

Parameter(s):
  _chain: the filter chain to add to.
  _n:     the number of elements to skip.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_skip_first
(
    struct d_filter_chain* _chain,
    size_t                 _n
)
{
    return d_filter_chain_add(_chain, d_filter_skip_first(_n));
}

/*
d_filter_chain_add_skip_last
  Convenience function to add a skip-last operation to a chain.

Parameter(s):
  _chain: the filter chain to add to.
  _n:     the number of elements to skip.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_skip_last
(
    struct d_filter_chain* _chain,
    size_t                 _n
)
{
    return d_filter_chain_add(_chain, d_filter_skip_last(_n));
}

/*
d_filter_chain_add_range
  Convenience function to add a range operation to a chain.

Parameter(s):
  _chain: the filter chain to add to.
  _start: the inclusive start index.
  _end:   the exclusive end index.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_range
(
    struct d_filter_chain* _chain,
    size_t                 _start,
    size_t                 _end
)
{
    return d_filter_chain_add(_chain, d_filter_range(_start, _end));
}

/*
d_filter_chain_add_where
  Convenience function to add a where (predicate) operation to a chain.

Parameter(s):
  _chain: the filter chain to add to.
  _test:  the predicate function.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_where
(
    struct d_filter_chain* _chain,
    fn_predicate            _test
)
{
    return d_filter_chain_add(_chain, d_filter_where(_test));
}

/*
d_filter_chain_add_where_context
  Convenience function to add a context-predicate operation to a chain.

Parameter(s):
  _chain:   the filter chain to add to.
  _test:    the predicate function.
  _context: the context pointer for the predicate.
Return:
  A boolean value; true if the operation was added successfully.
*/
D_INLINE bool
d_filter_chain_add_where_context
(
    struct d_filter_chain* _chain,
    fn_predicate            _test,
    void*                  _context
)
{
    return d_filter_chain_add(_chain, d_filter_where_context(_test, _context));
}

/*
d_filter_chain_concat
  Creates a new chain by concatenating two chains.

Parameter(s):
  _first:  the first chain.
  _second: the second chain to append after the first.
Return:
  A pointer to a newly allocated concatenated chain, or NULL on
failure.
*/
struct d_filter_chain*
d_filter_chain_concat
(
    const struct d_filter_chain* _first,
    const struct d_filter_chain* _second
)
{
    struct d_filter_chain* result;
    size_t                 total;

    if ( (!_first) ||
         (!_second) )
    {
        return NULL;
    }

    total  = _first->count + _second->count;
    result = d_filter_chain_new_with_capacity(total);

    if (!result)
    {
        return NULL;
    }

    if (_first->count > 0)
    {
        memcpy(result->operations,
               _first->operations,
               _first->count * sizeof(struct d_filter_operation));
    }

    if (_second->count > 0)
    {
        memcpy(result->operations + _first->count,
               _second->operations,
               _second->count * sizeof(struct d_filter_operation));
    }

    result->count = total;

    return result;
}

/*
d_filter_chain_append
  Appends all operations from a source chain to a destination chain.

Parameter(s):
  _dest:   the destination chain to append to.
  _source: the source chain to copy operations from.
Return:
  A boolean value; true if all operations were appended.
*/
bool
d_filter_chain_append
(
    struct d_filter_chain*       _dest,
    const struct d_filter_chain* _source
)
{
    size_t i;

    if ( (!_dest) ||
         (!_source) )
    {
        return false;
    }

    for (i = 0; i < _source->count; i++)
    {
        if (!d_filter_chain_add(_dest, &_source->operations[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_filter_chain_insert
  Inserts a filter operation at a specific index in the chain.

Parameter(s):
  _chain: the chain to insert into.
  _index: the position to insert at.
  _op:    the operation to insert.
Return:
  A boolean value; true if the operation was inserted successfully.
*/
bool
d_filter_chain_insert
(
    struct d_filter_chain*           _chain,
    size_t                           _index,
    const struct d_filter_operation* _op
)
{
    struct d_filter_operation* new_ops;
    size_t                    new_capacity;

    if ( (!_chain) ||
         (!_op)    ||
         (_index > _chain->count) )
    {
        return false;
    }

    // grow capacity if needed
    if (_chain->count >= _chain->capacity)
    {
        new_capacity = (_chain->capacity == 0)
                       ? 4
                       : (_chain->capacity * 2);
        new_ops = (struct d_filter_operation*)realloc(
                      _chain->operations,
                      new_capacity
                      * sizeof(struct d_filter_operation));

        if (!new_ops)
        {
            return false;
        }

        _chain->operations = new_ops;
        _chain->capacity   = new_capacity;
    }

    // shift elements to make room
    if (_index < _chain->count)
    {
        memmove(&_chain->operations[_index + 1],
                &_chain->operations[_index],
                (_chain->count - _index)
                * sizeof(struct d_filter_operation));
    }

    memcpy(&_chain->operations[_index], _op, sizeof(*_op));
    _chain->count++;

    return true;
}

/*
d_filter_chain_remove
  Removes a filter operation at a specific index from the chain.

Parameter(s):
  _chain: the chain to remove from.
  _index: the position to remove at.
Return:
  A boolean value; true if the operation was removed.
*/
bool
d_filter_chain_remove
(
    struct d_filter_chain* _chain,
    size_t                 _index
)
{
    if ( (!_chain) ||
         (_index >= _chain->count) )
    {
        return false;
    }

    d_filter_operation_free(&_chain->operations[_index]);

    if (_index < _chain->count - 1)
    {
        memmove(&_chain->operations[_index],
                &_chain->operations[_index + 1],
                (_chain->count - _index - 1)
                * sizeof(struct d_filter_operation));
    }

    _chain->count--;

    return true;
}

/*
d_filter_chain_clear
  Removes all operations from a chain without freeing the chain itself.

Parameter(s):
  _chain: the chain to clear.
Return:
  none.
*/
void
d_filter_chain_clear
(
    struct d_filter_chain* _chain
)
{
    size_t i;

    if (!_chain)
    {
        return;
    }

    for (i = 0; i < _chain->count; i++)
    {
        d_filter_operation_free(&_chain->operations[i]);
    }

    _chain->count = 0;

    return;
}

/*
d_filter_chain_length
  Returns the number of operations in a chain.

Parameter(s):
  _chain: the chain to query.
Return:
  The number of operations, or 0 if _chain is NULL.
*/
size_t
d_filter_chain_length
(
    const struct d_filter_chain* _chain
)
{
    if (!_chain)
    {
        return 0;
    }

    return _chain->count;
}

/*
d_filter_chain_is_empty
  Tests whether a chain has no operations.

Parameter(s):
  _chain: the chain to query.
Return:
  true if the chain is NULL or has no operations; false otherwise.
*/
bool
d_filter_chain_is_empty
(
    const struct d_filter_chain* _chain
)
{
    if (!_chain)
    {
        return true;
    }

    return (_chain->count == 0);
}

/*
d_filter_chain_free
  Frees a filter chain and all owned operations.

Parameter(s):
  _chain: the filter chain to free; may be NULL.
Return:
  none.
*/
void
d_filter_chain_free
(
    struct d_filter_chain* _chain
)
{
    size_t i;

    if (!_chain)
    {
        return;
    }

    if (_chain->operations)
    {
        for (i = 0; i < _chain->count; i++)
        {
            d_filter_operation_free(&_chain->operations[i]);
        }

        free(_chain->operations);
    }

    free(_chain);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  FILTER APPLICATION ENGINE                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_apply_operation_internal
  Internal implementation for applying a single filter operation to an
input array. Handles all operation types. Allocates and returns a new
array containing the result.

Parameter(s):
  _op:           the filter operation to apply.
  _input:        the source array.
  _count:        the number of elements in the input.
  _element_size: the size in bytes of each element.
  _out_count:    output parameter for the result count.
Return:
  A pointer to a newly allocated array containing the filtered result,
or NULL on failure. Caller is responsible for freeing the result.
*/
static void*
d_filter_apply_operation_internal
(
    const struct d_filter_operation* _op,
    const void*                      _input,
    size_t                           _count,
    size_t                           _element_size,
    size_t*                          _out_count
)
{
    void*       output;
    size_t      i;
    size_t      actual_count;
    size_t      start;
    size_t      end;
    size_t      n;
    const char* in_bytes;
    char*       out_bytes;

    if ( (!_op)              ||
         (!_input)           ||
         (!_out_count)       ||
         (_element_size == 0) )
    {
        return NULL;
    }

    in_bytes    = (const char*)_input;
    *_out_count = 0;

    switch (_op->type)
    {
    case D_FILTER_OP_TAKE_FIRST:
    case D_FILTER_OP_HEAD:
        n = (_op->params.count < _count)
            ? _op->params.count
            : _count;
        output = malloc(n * _element_size);

        if (output)
        {
            memcpy(output, _input, n * _element_size);
            *_out_count = n;
        }

        return output;

    case D_FILTER_OP_TAKE_LAST:
    case D_FILTER_OP_TAIL:
        n = (_op->params.count < _count)
            ? _op->params.count
            : _count;
        output = malloc(n * _element_size);

        if (output)
        {
            start = _count - n;
            memcpy(output,
                   in_bytes + (start * _element_size),
                   n * _element_size);
            *_out_count = n;
        }

        return output;

    case D_FILTER_OP_SKIP_FIRST:
    case D_FILTER_OP_REST:
        n = (_op->type == D_FILTER_OP_REST)
            ? 1
            : _op->params.count;

        if (n >= _count)
        {
            // skip all -> empty result
            output = malloc(_element_size);
            *_out_count = 0;

            return output;
        }

        actual_count = _count - n;
        output       = malloc(actual_count * _element_size);

        if (output)
        {
            memcpy(output,
                   in_bytes + (n * _element_size),
                   actual_count * _element_size);
            *_out_count = actual_count;
        }

        return output;

    case D_FILTER_OP_SKIP_LAST:
    case D_FILTER_OP_INIT:
        n = (_op->type == D_FILTER_OP_INIT)
            ? 1
            : _op->params.count;

        if (n >= _count)
        {
            output = malloc(_element_size);
            *_out_count = 0;

            return output;
        }

        actual_count = _count - n;
        output       = malloc(actual_count * _element_size);

        if (output)
        {
            memcpy(output, _input, actual_count * _element_size);
            *_out_count = actual_count;
        }

        return output;

    case D_FILTER_OP_TAKE_NTH:
        n = _op->params.step;

        if (n == 0)
        {
            n = 1;
        }

        // count elements
        actual_count = 0;

        for (i = 0; i < _count; i += n)
        {
            actual_count++;
        }

        output = malloc(actual_count * _element_size);

        if (!output)
        {
            return NULL;
        }

        out_bytes    = (char*)output;
        actual_count = 0;

        for (i = 0; i < _count; i += n)
        {
            memcpy(out_bytes + (actual_count * _element_size),
                   in_bytes + (i * _element_size),
                   _element_size);
            actual_count++;
        }

        *_out_count = actual_count;

        return output;

    case D_FILTER_OP_RANGE:
        start = _op->params.start;
        end   = _op->params.end;

        if (start >= _count)
        {
            output = malloc(_element_size);
            *_out_count = 0;

            return output;
        }

        if (end > _count)
        {
            end = _count;
        }

        if (start >= end)
        {
            output = malloc(_element_size);
            *_out_count = 0;

            return output;
        }

        n      = end - start;
        output = malloc(n * _element_size);

        if (output)
        {
            memcpy(output,
                   in_bytes + (start * _element_size),
                   n * _element_size);
            *_out_count = n;
        }

        return output;

    case D_FILTER_OP_WHERE:
    case D_FILTER_OP_WHERE_NOT:
        // predicate must be set
        if (!_op->params.test)
        {
            return NULL;
        }

        output = malloc(_count * _element_size);

        if (!output)
        {
            return NULL;
        }

        out_bytes    = (char*)output;
        actual_count = 0;

        for (i = 0; i < _count; i++)
        {
            const void* element;
            bool        matches;

            element = in_bytes + (i * _element_size);
            matches = _op->params.test(element,
                                       _op->params.context);

            if (_op->type == D_FILTER_OP_WHERE_NOT)
            {
                matches = !matches;
            }

            if (matches)
            {
                memcpy(out_bytes + (actual_count * _element_size),
                       element,
                       _element_size);
                actual_count++;
            }
        }

        *_out_count = actual_count;

        return output;

    case D_FILTER_OP_INDICES:
        // single index (from d_filter_at)
        if ( (_op->params.indices == NULL) &&
             (_op->params.count == 1) )
        {
            if (_op->params.start >= _count)
            {
                output = malloc(_element_size);
                *_out_count = 0;

                return output;
            }

            output = malloc(_element_size);

            if (output)
            {
                memcpy(output,
                       in_bytes
                       + (_op->params.start * _element_size),
                       _element_size);
                *_out_count = 1;
            }

            return output;
        }

        // multi-index (from d_filter_at_indices)
        if ( (!_op->params.indices)        ||
             (_op->params.indices_count == 0) )
        {
            output = malloc(_element_size);
            *_out_count = 0;

            return output;
        }

        output = malloc(
            _op->params.indices_count * _element_size);

        if (!output)
        {
            return NULL;
        }

        out_bytes    = (char*)output;
        actual_count = 0;

        for (i = 0; i < _op->params.indices_count; i++)
        {
            size_t idx = _op->params.indices[i];

            if (idx < _count)
            {
                memcpy(out_bytes
                       + (actual_count * _element_size),
                       in_bytes + (idx * _element_size),
                       _element_size);
                actual_count++;
            }
        }

        *_out_count = actual_count;

        return output;

    case D_FILTER_OP_REVERSE:
        output = malloc(_count * _element_size);

        if (!output)
        {
            return NULL;
        }

        out_bytes = (char*)output;

        for (i = 0; i < _count; i++)
        {
            memcpy(out_bytes + (i * _element_size),
                   in_bytes
                   + ((_count - 1 - i) * _element_size),
                   _element_size);
        }

        *_out_count = _count;

        return output;

    case D_FILTER_OP_SLICE:
        start = _op->params.start;
        end   = _op->params.end;
        n     = _op->params.step;

        if (start >= _count)
        {
            output = malloc(_element_size);
            *_out_count = 0;

            return output;
        }

        if (end > _count)
        {
            end = _count;
        }

        // count elements in the slice
        actual_count = 0;

        for (i = start; i < end; i += n)
        {
            actual_count++;
        }

        output = malloc(actual_count * _element_size);

        if (!output)
        {
            return NULL;
        }

        out_bytes    = (char*)output;
        actual_count = 0;

        for (i = start; i < end; i += n)
        {
            memcpy(out_bytes + (actual_count * _element_size),
                   in_bytes + (i * _element_size),
                   _element_size);
            actual_count++;
        }

        *_out_count = actual_count;

        return output;

    case D_FILTER_OP_DISTINCT:
        if (!_op->params.comparator)
        {
            return NULL;
        }

        output = malloc(_count * _element_size);

        if (!output)
        {
            return NULL;
        }

        out_bytes    = (char*)output;
        actual_count = 0;

        for (i = 0; i < _count; i++)
        {
            const void* element;
            bool        is_duplicate;
            size_t      j;

            element      = in_bytes + (i * _element_size);
            is_duplicate = false;

            for (j = 0; j < actual_count; j++)
            {
                if (_op->params.comparator(
                        element,
                        out_bytes + (j * _element_size),
                        _op->params.context) == 0)
                {
                    is_duplicate = true;

                    break;
                }
            }

            if (!is_duplicate)
            {
                memcpy(out_bytes
                       + (actual_count * _element_size),
                       element,
                       _element_size);
                actual_count++;
            }
        }

        *_out_count = actual_count;

        return output;

    case D_FILTER_OP_NONE:
        // no-op: copy input unchanged
        output = malloc(_count * _element_size);

        if (output)
        {
            memcpy(output, _input, _count * _element_size);
            *_out_count = _count;
        }

        return output;

    default:

        return NULL;
    }
}

/*
d_filter_apply_operation
  Applies a single filter operation to an input array and returns a
result structure.

Parameter(s):
  _op:           the filter operation to apply.
  _input:        the source array.
  _count:        the number of elements in the input.
  _element_size: the size in bytes of each element.
Return:
  A d_filter_result containing the filtered elements and status.
*/
struct d_filter_result*
d_filter_apply_operation
(
    const struct d_filter_operation* _op,
    const void*                      _input,
    size_t                           _count,
    size_t                           _element_size
)
{
    struct d_filter_result* result;
    size_t                  out_count;
    void*                   output;

    result = malloc(sizeof(struct d_filter_result));

    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));

    if ( (!_op)              ||
         (!_input)           ||
         (_element_size == 0) )
    {
        result->status = D_FILTER_RESULT_INVALID;

        return result;
    }

    output = d_filter_apply_operation_internal(
                 _op, _input, _count, _element_size, &out_count);

    if (!output)
    {
        result->status = D_FILTER_RESULT_ERROR;

        return result;
    }

    result->elements = output;
    result->count    = out_count;
    result->status   = (out_count == 0)
                       ? D_FILTER_RESULT_EMPTY
                       : D_FILTER_RESULT_SUCCESS;

    return result;
}

/*
d_filter_apply_chain
  Applies a chain of filter operations sequentially to an input array.
Each operation's output becomes the next operation's input.

Parameter(s):
  _chain:        the filter chain to apply.
  _input:        the source array.
  _count:        the number of elements in the input.
  _element_size: the size in bytes of each element.
Return:
  A d_filter_result containing the final filtered elements and status.
*/
struct d_filter_result*
d_filter_apply_chain
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    struct d_filter_result* result;
    size_t                  i;
    void*                   current_data;
    size_t                  current_count;
    void*                   next_data;
    size_t                  next_count;

    result = malloc(sizeof(struct d_filter_result));

    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));

    if ( (!_chain)           ||
         (!_input)           ||
         (_element_size == 0) )
    {
        result->status = D_FILTER_RESULT_INVALID;

        return result;
    }

    // empty chain returns a copy of the input
    if (_chain->count == 0)
    {
        result->elements = malloc(_count * _element_size);

        if (!result->elements)
        {
            result->status = D_FILTER_RESULT_NO_MEMORY;

            return result;
        }

        memcpy(result->elements, _input, _count * _element_size);
        result->count  = _count;
        result->status = D_FILTER_RESULT_SUCCESS;

        return result;
    }

    // copy input for first operation
    current_data = malloc(_count * _element_size);

    if (!current_data)
    {
        result->status = D_FILTER_RESULT_NO_MEMORY;

        return result;
    }

    memcpy(current_data, _input, _count * _element_size);
    current_count = _count;

    // apply each operation in sequence
    for (i = 0; i < _chain->count; i++)
    {
        next_data = d_filter_apply_operation_internal(
                        &_chain->operations[i],
                        current_data,
                        current_count,
                        _element_size,
                        &next_count);
        free(current_data);

        if (!next_data)
        {
            result->status = D_FILTER_RESULT_ERROR;

            return result;
        }

        current_data  = next_data;
        current_count = next_count;
    }

    result->elements = current_data;
    result->count    = current_count;
    result->status   = (current_count == 0)
                       ? D_FILTER_RESULT_EMPTY
                       : D_FILTER_RESULT_SUCCESS;

    return result;
}

/*
d_filter_apply_in_place
  Applies a filter chain in-place, modifying the original array.

Parameter(s):
  _chain:        the filter chain to apply.
  _data:         the array to filter in place.
  _count:        the number of elements in the array.
  _element_size: the size in bytes of each element.
Return:
  The number of elements remaining after filtering. Returns 0 if any
parameter is invalid or the operation fails.
*/
size_t
d_filter_apply_in_place
(
    const struct d_filter_chain* _chain,
    void*                        _data,
    size_t                       _count,
    size_t                       _element_size
)
{
    struct d_filter_result* result;
    size_t                  new_count;

    if ( (!_chain)           ||
         (!_data)            ||
         (_element_size == 0) )
    {
        return 0;
    }

    result = d_filter_apply_chain(
                 _chain, _data, _count, _element_size);

    if (!result)
    {
        return 0;
    }

    if (result->status != D_FILTER_RESULT_SUCCESS &&
        result->status != D_FILTER_RESULT_EMPTY)
    {
        d_filter_result_free(result);
        free(result);

        return 0;
    }

    new_count = result->count;

    if (new_count > 0)
    {
        memcpy(_data, result->elements,
               new_count * _element_size);
    }

    d_filter_result_free(result);
    free(result);

    return new_count;
}

/*
d_filter_chain_matches_element
  Tests whether a single element passes a filter chain. Intended for
per-element evaluation in contexts where the caller iterates manually
(e.g. buffer_common filtering across non-contiguous storage).

For chains containing only predicate operations (WHERE / WHERE_NOT),
the predicates are evaluated directly without allocation. For chains
that also contain positional or structural operations, the element is
wrapped in a 1-element array and evaluated through d_filter_apply_chain.

Parameter(s):
  _chain:        the filter chain to test against.
  _element:      pointer to the element to test.
  _element_size: the size in bytes of the element.
Return:
  true if the element survives the entire chain, false otherwise.
*/
bool
d_filter_chain_matches_element
(
    const struct d_filter_chain* _chain,
    const void*                  _element,
    size_t                       _element_size
)
{
    size_t                           i;
    const struct d_filter_operation* op;
    bool                             matches;
    bool                             all_predicates;
    struct d_filter_result*          result;

    if ( (!_chain)          ||
         (!_element)        ||
         (_element_size == 0) )
    {
        return false;
    }

    // empty chain matches everything
    if (_chain->count == 0)
    {
        return true;
    }

    // fast path: if every operation is a predicate (WHERE / WHERE_NOT),
    // evaluate directly without allocation
    all_predicates = true;

    for (i = 0; i < _chain->count; i++)
    {
        if ( (_chain->operations[i].type != D_FILTER_OP_WHERE) &&
             (_chain->operations[i].type != D_FILTER_OP_WHERE_NOT) )
        {
            all_predicates = false;

            break;
        }
    }

    if (all_predicates)
    {
        for (i = 0; i < _chain->count; i++)
        {
            op = &_chain->operations[i];

            if (!op->params.test)
            {
                return false;
            }

            matches = op->params.test(_element, op->params.context);

            if (op->type == D_FILTER_OP_WHERE_NOT)
            {
                matches = !matches;
            }

            if (!matches)
            {
                return false;
            }
        }

        return true;
    }

    // general path: wrap in 1-element array and apply chain
    result = d_filter_apply_chain(_chain,
                                  _element,
                                  1,
                                  _element_size);

    if (!result)
    {
        return false;
    }

    if ( (result->status != D_FILTER_RESULT_SUCCESS) &&
         (result->status != D_FILTER_RESULT_EMPTY) )
    {
        d_filter_result_free(result);
        free(result);

        return false;
    }

    matches = (result->count == 1);

    d_filter_result_free(result);
    free(result);

    return matches;
}

/*
d_filter_result_free
  Frees all resources owned by a filter result.

Parameter(s):
  _result: the filter result to clean up; may be NULL.
Return:
  none.
*/
void
d_filter_result_free
(
    struct d_filter_result* _result
)
{
    if (!_result)
    {
        return;
    }

    if (_result->elements)
    {
        free(_result->elements);
        _result->elements = NULL;
    }

    if (_result->indices)
    {
        free(_result->indices);
        _result->indices = NULL;
    }

    if (_result->error_message)
    {
        free(_result->error_message);
        _result->error_message = NULL;
    }

    _result->count  = 0;
    _result->status = D_FILTER_RESULT_SUCCESS;

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   FILTER COMBINATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_union_new
  Creates a new filter union combinator with pre-allocated capacity.

Parameter(s):
  _capacity: the initial number of filters to allocate for.
Return:
  A pointer to a newly allocated d_filter_union, or NULL on failure.
*/
struct d_filter_union*
d_filter_union_new
(
    size_t _capacity
)
{
    struct d_filter_union* u;

    u = malloc(sizeof(struct d_filter_union));

    if (!u)
    {
        return NULL;
    }

    u->filters  = NULL;
    u->count    = 0;
    u->capacity = _capacity;

    if (_capacity > 0)
    {
        u->filters = malloc(_capacity * 
                            sizeof(struct d_filter_chain*));

        if (!u->filters)
        {
            free(u);

            return NULL;
        }
    }

    return u;
}

/*
d_filter_intersection_new
  Creates a new filter intersection combinator with pre-allocated
capacity.

Parameter(s):
  _capacity: the initial number of filters to allocate for.
Return:
  A pointer to a newly allocated d_filter_intersection, or NULL on
failure.
*/
struct d_filter_intersection*
d_filter_intersection_new
(
    size_t _capacity
)
{
    struct d_filter_intersection* inter;

    inter = malloc(sizeof(struct d_filter_intersection));

    if (!inter)
    {
        return NULL;
    }

    inter->filters  = NULL;
    inter->count    = 0;
    inter->capacity = _capacity;

    if (_capacity > 0)
    {
        inter->filters = malloc(_capacity * sizeof(struct d_filter_chain*));

        if (!inter->filters)
        {
            free(inter);

            return NULL;
        }
    }

    return inter;
}

/*
d_filter_difference_new
  Creates a new filter difference combinator (include - exclude).

Parameter(s):
  _include: the chain of elements to include.
  _exclude: the chain of elements to exclude.
Return:
  A pointer to a newly allocated d_filter_difference, or NULL on
failure.
*/
struct d_filter_difference*
d_filter_difference_new
(
    struct d_filter_chain* _include,
    struct d_filter_chain* _exclude
)
{
    struct d_filter_difference* diff;

    diff = malloc(sizeof(struct d_filter_difference));

    if (!diff)
    {
        return NULL;
    }

    diff->include = _include;
    diff->exclude = _exclude;

    return diff;
}

/*
d_filter_union_add
  Adds a filter chain to a union combinator.

Parameter(s):
  _union:  the union combinator.
  _filter: the filter chain to add.
Return:
  A boolean value; true if the chain was added.
*/
bool
d_filter_union_add
(
    struct d_filter_union*       _union,
    const struct d_filter_chain* _filter
)
{
    if ( (!_union)  ||
         (!_filter) ||
         (_union->count >= _union->capacity) )
    {
        return false;
    }

    _union->filters[_union->count] = (struct d_filter_chain*)_filter;
    _union->count++;

    return true;
}

/*
d_filter_intersection_add
  Adds a filter chain to an intersection combinator.

Parameter(s):
  _intersection: the intersection combinator.
  _filter:       the filter chain to add.
Return:
  A boolean value; true if the chain was added.
*/
bool
d_filter_intersection_add
(
    struct d_filter_intersection* _intersection,
    const struct d_filter_chain*  _filter
)
{
    if ( (!_intersection) ||
         (!_filter)       ||
         (_intersection->count >= _intersection->capacity) )
    {
        return false;
    }

    _intersection->filters[_intersection->count] =
        (struct d_filter_chain*)_filter;
    _intersection->count++;

    return true;
}

/*
d_filter_union_free
  Frees a filter union combinator. Does not free the contained chains.

Parameter(s):
  _union: the union to free; may be NULL.
Return:
  none.
*/
void
d_filter_union_free
(
    struct d_filter_union* _union
)
{
    if (!_union)
    {
        return;
    }

    if (_union->filters)
    {
        free(_union->filters);
    }

    free(_union);

    return;
}

/*
d_filter_intersection_free
  Frees a filter intersection combinator. Does not free the contained
chains.

Parameter(s):
  _intersection: the intersection to free; may be NULL.
Return:
  none.
*/
void
d_filter_intersection_free
(
    struct d_filter_intersection* _intersection
)
{
    if (!_intersection)
    {
        return;
    }

    if (_intersection->filters)
    {
        free(_intersection->filters);
    }

    free(_intersection);

    return;
}

/*
d_filter_difference_free
  Frees a filter difference combinator. Does not free the contained
chains.

Parameter(s):
  _difference: the difference to free; may be NULL.
Return:
  none.
*/
void
d_filter_difference_free
(
    struct d_filter_difference* _difference
)
{
    if (!_difference)
    {
        return;
    }

    free(_difference);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             V.    COMBINATOR APPLICATION                                ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_apply_union
  Applies a union combinator, returning elements matching any filter.
Uses byte comparison to map sub-filter results back to original input
indices, preserving original element order.

Parameter(s):
  _union:        the union combinator.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  A d_filter_result containing the union of all filter results.
*/
struct d_filter_result*
d_filter_apply_union
(
    const struct d_filter_union* _union,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    struct d_filter_result* result;
    struct d_filter_result* sub_result;
    bool*                   included;
    size_t                  i;
    size_t                  j;
    size_t                  k;
    size_t                  out_count;
    char*                   out_bytes;
    const char*             in_bytes;
    const char*             res_bytes;

    result = malloc(sizeof(struct d_filter_result));

    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));

    if ( (!_union)           ||
         (!_input)           ||
         (_element_size == 0) )
    {
        result->status = D_FILTER_RESULT_INVALID;

        return result;
    }

    included = (bool*)calloc(_count, sizeof(bool));

    if (!included)
    {
        result->status = D_FILTER_RESULT_NO_MEMORY;

        return result;
    }

    in_bytes = (const char*)_input;

    // for each sub-filter, apply and mark matching indices
    for (i = 0; i < _union->count; i++)
    {
        sub_result = d_filter_apply_chain(
                         _union->filters[i],
                         _input, _count, _element_size);

        if ( (!sub_result) ||
             (sub_result->status != D_FILTER_RESULT_SUCCESS &&
              sub_result->status != D_FILTER_RESULT_EMPTY) )
        {
            d_filter_result_free(sub_result);
            free(sub_result);

            continue;
        }

        // match result elements back to input by byte comparison
        res_bytes = (const char*)sub_result->elements;

        for (j = 0; j < sub_result->count; j++)
        {
            for (k = 0; k < _count; k++)
            {
                if ( (!included[k]) &&
                     (memcmp(
                          res_bytes + (j * _element_size),
                          in_bytes + (k * _element_size),
                          _element_size) == 0) )
                {
                    included[k] = true;

                    break;
                }
            }
        }

        d_filter_result_free(sub_result);
        free(sub_result);
    }

    // count and build result
    out_count = 0;

    for (i = 0; i < _count; i++)
    {
        if (included[i])
        {
            out_count++;
        }
    }

    if (out_count == 0)
    {
        free(included);
        result->status = D_FILTER_RESULT_EMPTY;
        result->elements = malloc(_element_size);

        return result;
    }

    result->elements = malloc(out_count * _element_size);

    if (!result->elements)
    {
        free(included);
        result->status = D_FILTER_RESULT_NO_MEMORY;

        return result;
    }

    out_bytes = (char*)result->elements;
    out_count = 0;

    for (i = 0; i < _count; i++)
    {
        if (included[i])
        {
            memcpy(out_bytes + (out_count * _element_size),
                   in_bytes + (i * _element_size),
                   _element_size);
            out_count++;
        }
    }

    result->count  = out_count;
    result->status = D_FILTER_RESULT_SUCCESS;

    free(included);

    return result;
}

/*
d_filter_apply_intersection
  Applies an intersection combinator, returning only elements that
pass all contained filter chains. Applies chains sequentially so
each narrows the result set.

Parameter(s):
  _inter:        the intersection combinator.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  A d_filter_result containing the intersection of all filter results.
*/
struct d_filter_result*
d_filter_apply_intersection
(
    const struct d_filter_intersection* _inter,
    const void*                         _input,
    size_t                              _count,
    size_t                              _element_size
)
{
    struct d_filter_result* result;
    struct d_filter_result* sub_result;
    void*                   current_data;
    size_t                  current_count;
    size_t                  i;

    result = malloc(sizeof(struct d_filter_result));

    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));

    if ( (!_inter)           ||
         (!_input)           ||
         (_element_size == 0) )
    {
        result->status = D_FILTER_RESULT_INVALID;

        return result;
    }

    // start with a copy of the input
    current_data = malloc(_count * _element_size);

    if (!current_data)
    {
        result->status = D_FILTER_RESULT_NO_MEMORY;

        return result;
    }

    memcpy(current_data, _input, _count * _element_size);
    current_count = _count;

    // apply each chain sequentially, narrowing the result
    for (i = 0; i < _inter->count; i++)
    {
        sub_result = d_filter_apply_chain(
                         _inter->filters[i],
                         current_data,
                         current_count,
                         _element_size);
        free(current_data);

        if ( (!sub_result) ||
             (sub_result->status != D_FILTER_RESULT_SUCCESS &&
              sub_result->status != D_FILTER_RESULT_EMPTY) )
        {
            d_filter_result_free(sub_result);
            free(sub_result);
            result->status = D_FILTER_RESULT_ERROR;

            return result;
        }

        current_data  = sub_result->elements;
        current_count = sub_result->count;

        // detach from sub_result so we don't double-free
        sub_result->elements = NULL;
        sub_result->count    = 0;
        d_filter_result_free(sub_result);
        free(sub_result);

        // early exit if empty
        if (current_count == 0)
        {
            break;
        }
    }

    result->elements = current_data;
    result->count    = current_count;
    result->status   = (current_count == 0)
                       ? D_FILTER_RESULT_EMPTY
                       : D_FILTER_RESULT_SUCCESS;

    return result;
}

/*
d_filter_apply_difference
  Applies a difference combinator (include - exclude). Returns elements
that pass the include chain but do not appear in the exclude chain's
results.

Parameter(s):
  _diff:         the difference combinator.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  A d_filter_result containing elements in include but not in exclude.
*/
struct d_filter_result*
d_filter_apply_difference
(
    const struct d_filter_difference* _diff,
    const void*                       _input,
    size_t                            _count,
    size_t                            _element_size
)
{
    struct d_filter_result*  result;
    struct d_filter_result* include_result;
    struct d_filter_result* exclude_result;
    size_t                  i;
    size_t                  j;
    size_t                  out_count;
    char*                   out_bytes;
    const char*             inc_bytes;
    const char*             exc_bytes;
    bool                    excluded;

    result = malloc(sizeof(struct d_filter_result));

    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));

    if ( (!_diff)  ||
         (!_input) ||
         (_element_size == 0) )
    {
        result->status = D_FILTER_RESULT_INVALID;

        return result;
    }

    // apply include chain
    include_result = d_filter_apply_chain(_diff->include, 
                                          _input, 
                                          _count,
                                          _element_size);

    if ( (!include_result) ||
         ((include_result->status != D_FILTER_RESULT_SUCCESS) &&
          (include_result->status != D_FILTER_RESULT_EMPTY)) )
    {
        d_filter_result_free(include_result);
        free(include_result);
        result->status = D_FILTER_RESULT_ERROR;

        return result;
    }

    if (include_result->count == 0)
    {
        result->status   = D_FILTER_RESULT_EMPTY;
        result->elements = include_result->elements;
        include_result->elements = NULL;
        d_filter_result_free(include_result);
        free(include_result);

        return result;
    }

    // apply exclude chain
    exclude_result = d_filter_apply_chain(_diff->exclude, 
                                          _input,
                                          _count,
                                          _element_size);

    if ( (!exclude_result) ||
         ((exclude_result->status != D_FILTER_RESULT_SUCCESS) &&
          (exclude_result->status != D_FILTER_RESULT_EMPTY)) )
    {
        // exclude failed — return include result as-is
        d_filter_result_free(exclude_result);
        free(exclude_result);

        // transfer include_result contents to result
        result->elements = include_result->elements;
        result->count    = include_result->count;
        result->status   = include_result->status;
        include_result->elements = NULL;
        d_filter_result_free(include_result);
        free(include_result);

        return result;
    }

    // subtract: keep include elements not in exclude
    result->elements = malloc(
        include_result->count * _element_size);

    if (!result->elements)
    {
        d_filter_result_free(include_result);
        free(include_result);
        d_filter_result_free(exclude_result);
        free(exclude_result);
        result->status = D_FILTER_RESULT_NO_MEMORY;

        return result;
    }

    out_bytes = (char*)result->elements;
    inc_bytes = (const char*)include_result->elements;
    exc_bytes = (const char*)exclude_result->elements;
    out_count = 0;

    for (i = 0; i < include_result->count; i++)
    {
        excluded = false;

        for (j = 0; j < exclude_result->count; j++)
        {
            if (memcmp(inc_bytes + (i * _element_size),
                       exc_bytes + (j * _element_size),
                       _element_size) == 0)
            {
                excluded = true;

                break;
            }
        }

        if (!excluded)
        {
            memcpy(out_bytes + (out_count * _element_size),
                   inc_bytes + (i * _element_size),
                   _element_size);

            out_count++;
        }
    }

    d_filter_result_free(include_result);
    free(include_result);
    d_filter_result_free(exclude_result);
    free(exclude_result);

    result->count  = out_count;
    result->status = (out_count == 0)
                     ? D_FILTER_RESULT_EMPTY
                     : D_FILTER_RESULT_SUCCESS;

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   COUNTING AND QUERYING                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_count_matches
  Counts elements matching a filter chain without returning them.

Parameter(s):
  _chain:        the filter chain to apply.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  The number of matching elements. Returns 0 on error.
*/
size_t
d_filter_count_matches
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    struct d_filter_result* result;
    size_t                 match_count;

    result = d_filter_apply_chain(_chain, 
                                  _input, 
                                  _count,
                                  _element_size);

    if ( (!result) ||
         ((result->status != D_FILTER_RESULT_SUCCESS) &&
          (result->status != D_FILTER_RESULT_EMPTY)) )
    {
        d_filter_result_free(result);
        free(result);

        return 0;
    }

    match_count = result->count;

    d_filter_result_free(result);
    free(result);

    return match_count;
}

/*
d_filter_any_match
  Tests whether any element matches a filter chain.

Parameter(s):
  _chain:        the filter chain to apply.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  true if at least one element matches.
*/
bool
d_filter_any_match
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    return (d_filter_count_matches(_chain, 
                                   _input, 
                                   _count, 
                                   _element_size) > 0);
}

/*
d_filter_all_match
  Tests whether all elements match a filter chain.

Parameter(s):
  _chain:        the filter chain to apply.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  true if all elements match.
*/
bool
d_filter_all_match
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    return (d_filter_count_matches(_chain, 
                                   _input, 
                                   _count, 
                                   _element_size) == _count);
}

/*
d_filter_none_match
  Tests whether no elements match a filter chain.

Parameter(s):
  _chain:        the filter chain to apply.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  true if no elements match.
*/
bool
d_filter_none_match
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    return (d_filter_count_matches(_chain,
                                   _input,
                                   _count,
                                   _element_size) == 0);
}

/*
d_filter_get_indices
  Returns indices of elements remaining after applying a filter chain.
Applies the chain, then matches result elements back to original input
indices by byte comparison.

Parameter(s):
  _chain:        the filter chain.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
  _out_count:    output parameter for the number of indices.
Return:
  A newly allocated array of indices, or NULL on error. Caller must
free the result.
*/
size_t*
d_filter_get_indices
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size,
    size_t*                      _out_count
)
{
    struct d_filter_result* result;
    size_t*                indices;
    size_t                 idx_count;
    size_t                 i;
    size_t                 k;
    const char*            in_bytes;
    const char*            res_bytes;
    bool*                  used;

    if (!_out_count)
    {
        return NULL;
    }

    *(_out_count) = 0;

    result = d_filter_apply_chain(_chain, _input, _count, _element_size);

    if ( (!result) ||
         (result->status != D_FILTER_RESULT_SUCCESS &&
          result->status != D_FILTER_RESULT_EMPTY) )
    {
        d_filter_result_free(result);
        free(result);

        return NULL;
    }

    if (result->count == 0)
    {
        d_filter_result_free(result);
        free(result);

        return NULL;
    }

    indices = malloc(result->count * sizeof(size_t));
    used    = calloc(_count, sizeof(bool));

    if ( (!indices) ||
         (!used) )
    {
        free(indices);
        free(used);

        d_filter_result_free(result);
        free(result);

        return NULL;
    }

    in_bytes  = (const char*)_input;
    res_bytes = (const char*)result->elements;
    idx_count = 0;

    for (i = 0; i < result->count; i++)
    {
        for (k = 0; k < _count; k++)
        {
            if ( (!used[k]) &&
                 (memcmp(res_bytes + (i * _element_size),
                         in_bytes + (k * _element_size),
                         _element_size) == 0) )
            {
                indices[idx_count] = k;
                idx_count++;
                used[k] = true;

                break;
            }
        }
    }

    free(used);
    d_filter_result_free(result);
    free(result);

    *(_out_count) = idx_count;

    return indices;
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  UTILITY FUNCTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_operation_is_valid
  Tests whether a filter operation is valid.

Parameter(s):
  _op: the operation to validate.
Return:
  true if the operation is valid.
*/
bool
d_filter_operation_is_valid
(
    const struct d_filter_operation* _op
)
{
    if (!_op)
    {
        return false;
    }

    if ( (_op->type < D_FILTER_OP_NONE) ||
         (_op->type > D_FILTER_OP_REST) )
    {
        return false;
    }

    // validate predicate ops have a test function
    if ( (_op->type == D_FILTER_OP_WHERE     ||
          _op->type == D_FILTER_OP_WHERE_NOT) &&
         (!_op->params.test) )
    {
        return false;
    }

    // validate distinct has a comparator
    if ( (_op->type == D_FILTER_OP_DISTINCT) &&
         (!_op->params.comparator) )
    {
        return false;
    }

    return true;
}

/*
d_filter_chain_is_valid
  Tests whether a filter chain is valid (all operations valid).

Parameter(s):
  _chain: the chain to validate.
Return:
  true if the chain is valid.
*/
bool
d_filter_chain_is_valid
(
    const struct d_filter_chain* _chain
)
{
    size_t i;

    if (!_chain)
    {
        return false;
    }

    for (i = 0; i < _chain->count; i++)
    {
        if (!d_filter_operation_is_valid(
                &_chain->operations[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_filter_op_type_name
  Internal helper returning the string name for an operation type.

Parameter(s):
  _type: the operation type.
Return:
  A static string name.
*/
static const char*
d_filter_op_type_name
(
    enum d_filter_op_type _type
)
{
    switch (_type)
    {
    case D_FILTER_OP_NONE:       return "none";
    case D_FILTER_OP_TAKE_FIRST: return "take_first";
    case D_FILTER_OP_TAKE_LAST:  return "take_last";
    case D_FILTER_OP_SKIP_FIRST: return "skip_first";
    case D_FILTER_OP_SKIP_LAST:  return "skip_last";
    case D_FILTER_OP_TAKE_NTH:   return "take_nth";
    case D_FILTER_OP_RANGE:      return "range";
    case D_FILTER_OP_WHERE:      return "where";
    case D_FILTER_OP_WHERE_NOT:  return "where_not";
    case D_FILTER_OP_INDICES:    return "indices";
    case D_FILTER_OP_DISTINCT:   return "distinct";
    case D_FILTER_OP_REVERSE:    return "reverse";
    case D_FILTER_OP_SLICE:      return "slice";
    case D_FILTER_OP_HEAD:       return "head";
    case D_FILTER_OP_TAIL:       return "tail";
    case D_FILTER_OP_INIT:       return "init";
    case D_FILTER_OP_REST:       return "rest";
    default:                     return "unknown";
    }
}

/*
d_filter_operation_to_string
  Creates a human-readable string description of a filter operation.

Parameter(s):
  _op: the operation to describe.
Return:
  A newly allocated string, or NULL on failure. Caller must free.
*/
char*
d_filter_operation_to_string
(
    const struct d_filter_operation* _op
)
{
    char*  buffer;
    size_t buf_size;

    if (!_op)
    {
        return NULL;
    }

    buf_size = 128;
    buffer   = malloc(buf_size);

    if (!buffer)
    {
        return NULL;
    }

    switch (_op->type)
    {
    case D_FILTER_OP_TAKE_FIRST:
    case D_FILTER_OP_TAKE_LAST:
    case D_FILTER_OP_SKIP_FIRST:
    case D_FILTER_OP_SKIP_LAST:
        snprintf(buffer, buf_size, "%s(%zu)",
                 d_filter_op_type_name(_op->type),
                 _op->params.count);

        break;

    case D_FILTER_OP_TAKE_NTH:
        snprintf(buffer, buf_size, "take_nth(%zu)",
                 _op->params.step);

        break;

    case D_FILTER_OP_RANGE:
        snprintf(buffer, buf_size, "range(%zu, %zu)",
                 _op->params.start, _op->params.end);

        break;

    case D_FILTER_OP_SLICE:
        snprintf(buffer, buf_size, "slice(%zu, %zu, %zu)",
                 _op->params.start, _op->params.end,
                 _op->params.step);

        break;

    case D_FILTER_OP_WHERE:
        snprintf(buffer, buf_size, "where(%p)",
                 (const void*)_op->params.test);

        break;

    case D_FILTER_OP_WHERE_NOT:
        snprintf(buffer, buf_size, "where_not(%p)",
                 (const void*)_op->params.test);

        break;

    case D_FILTER_OP_INDICES:
        snprintf(buffer, buf_size, "indices(%zu)",
                 _op->params.indices_count);

        break;

    default:
        snprintf(buffer, buf_size, "%s",
                 d_filter_op_type_name(_op->type));

        break;
    }

    return buffer;
}

/*
d_filter_chain_to_string
  Creates a human-readable string description of a filter chain.
Operations are separated by " -> ".

Parameter(s):
  _chain: the chain to describe.
Return:
  A newly allocated string, or NULL on failure. Caller must free.
*/
char*
d_filter_chain_to_string
(
    const struct d_filter_chain* _chain
)
{
    char*  buffer;
    size_t buf_size;
    size_t offset;
    size_t i;
    char*  op_str;
    size_t op_len;

    if (!_chain)
    {
        return NULL;
    }

    if (_chain->count == 0)
    {
        buffer = malloc(8);

        if (buffer)
        {
            snprintf(buffer, 8, "(empty)");
        }

        return buffer;
    }

    // allocate a generous buffer
    buf_size = _chain->count * 64;
    buffer   = malloc(buf_size);

    if (!buffer)
    {
        return NULL;
    }

    buffer[0] = '\0';
    offset    = 0;

    for (i = 0; i < _chain->count; i++)
    {
        if (i > 0)
        {
            op_len = strlen(" -> ");

            if (offset + op_len < buf_size)
            {
                memcpy(buffer + offset, " -> ", op_len);
                offset += op_len;
                buffer[offset] = '\0';
            }
        }

        op_str = d_filter_operation_to_string(
                     &_chain->operations[i]);

        if (op_str)
        {
            op_len = strlen(op_str);

            if (offset + op_len < buf_size)
            {
                memcpy(buffer + offset, op_str, op_len);
                offset += op_len;
                buffer[offset] = '\0';
            }

            free(op_str);
        }
    }

    return buffer;
}

/*
d_filter_operation_from_string
  Parses a string into a filter operation. Supports basic formats:
  "take_first(N)", "take_last(N)", "skip_first(N)", "skip_last(N)",
  "range(S, E)", "head", "tail", "init", "rest", "reverse".

Parameter(s):
  _str: the string to parse.
Return:
  A pointer to a newly allocated operation, or NULL on failure.
*/
struct d_filter_operation*
d_filter_operation_from_string
(
    const char* _str
)
{
    struct d_filter_operation* op;
    size_t                    n;
    size_t                    s;
    size_t                    e;
    size_t                    step;

    if (!_str)
    {
        return NULL;
    }

    op = malloc(sizeof(struct d_filter_operation));

    // ensure that memory allocation was successful
    if (!op)
    {
        return NULL;
    }

    memset(op, 0, sizeof(*op));

    // parse known formats
    if (d_sscanf_s(_str, "take_first(%zu)", &n) == 1)
    {
        op = d_filter_take_first(n);

        return op;
    }

    if (d_sscanf_s(_str, "take_last(%zu)", &n) == 1)
    {
        op = d_filter_take_last(n);

        return op;
    }

    if (d_sscanf_s(_str, "skip_first(%zu)", &n) == 1)
    {
        op = d_filter_skip_first(n);

        return op;
    }

    if (d_sscanf_s(_str, "skip_last(%zu)", &n) == 1)
    {
        op = d_filter_skip_last(n);

        return op;
    }

    if (d_sscanf_s(_str, "take_nth(%zu)", &n) == 1)
    {
        op = d_filter_take_nth(n);

        return op;
    }

    if (d_sscanf_s(_str, "range(%zu, %zu)", &s, &e) == 2)
    {
        op = d_filter_range(s, e);

        return op;
    }

    if (d_sscanf_s(_str, "slice(%zu, %zu, %zu)", &s, &e, &step) == 3)
    {
        op = d_filter_slice(s, e, step);

        return op;
    }

    if (strcmp(_str, "head") == 0)
    {
        op = d_filter_head();

        return op;
    }

    if (strcmp(_str, "tail") == 0)
    {
        op = d_filter_tail();

        return op;
    }

    if (strcmp(_str, "init") == 0)
    {
        op = d_filter_init();

        return op;
    }

    if (strcmp(_str, "rest") == 0)
    {
        op = d_filter_rest();

        return op;
    }

    if (strcmp(_str, "reverse") == 0)
    {
        op = d_filter_reverse();

        return op;
    }

    // unknown format
    free(op);

    return NULL;
}

/*
d_filter_chain_from_string
  Parses a string into a filter chain. Operations are separated by
" -> ". Each operation is parsed by d_filter_operation_from_string.

Parameter(s):
  _str: the string to parse.
Return:
  A pointer to a newly allocated chain, or NULL on failure.
*/
struct d_filter_chain*
d_filter_chain_from_string
(
    const char* _str
)
{
    struct d_filter_chain*     chain;
    struct d_filter_operation* op;
    char*                      copy;
    char*                      token;
    char*                      save_ptr;
    size_t                     len;

    if (!_str)
    {
        return NULL;
    }

    chain = d_filter_chain_new();

    if (!chain)
    {
        return NULL;
    }

    len  = strlen(_str);

    copy = malloc(len + 1);

    if (!copy)
    {
        d_filter_chain_free(chain);

        return NULL;
    }

    memcpy(copy, _str, len + 1);

    // split on " -> " delimiter
    token = d_strtok_r(copy, "|", &save_ptr);

    while (token)
    {
        // trim leading whitespace
        while (*token == ' ')
        {
            token++;
        }

        // trim " -> " prefix if present
        if (strncmp(token, "-> ", 3) == 0)
        {
            token += 3;
        }

        // trim trailing whitespace
        len = strlen(token);

        while ( (len > 0) &&
                (token[len - 1] == ' ') )
        {
            token[len - 1] = '\0';
            len--;
        }

        if (len > 0)
        {
            op = d_filter_operation_from_string(token);

            if (op)
            {
                d_filter_chain_add(chain, op);

                free(op);
            }
        }

        token = d_strtok_r(NULL, "|", &save_ptr);
    }

    free(copy);

    return chain;
}

/*
d_filter_chain_optimize
  Creates an optimized copy of a filter chain. Current optimizations:
  - Adjacent take_first ops are merged to the smaller count
  - Adjacent skip_first ops are merged to the sum
  - Remove no-ops

Parameter(s):
  _chain: the chain to optimize.
Return:
  A pointer to a newly allocated optimized chain, or NULL on failure.
*/
struct d_filter_chain*
d_filter_chain_optimize
(
    const struct d_filter_chain* _chain
)
{
    struct d_filter_chain*    result;
    struct d_filter_operation prev;
    struct d_filter_operation curr;
    size_t                    i;
    bool                      has_prev;

    if (!_chain)
    {
        return NULL;
    }

    result = d_filter_chain_new();

    if (!result)
    {
        return NULL;
    }

    has_prev = false;
    memset(&prev, 0, sizeof(prev));

    for (i = 0; i < _chain->count; i++)
    {
        curr = _chain->operations[i];

        // skip no-ops
        if (curr.type == D_FILTER_OP_NONE)
        {
            continue;
        }

        if (has_prev)
        {
            // merge adjacent take_first
            if ( (prev.type == D_FILTER_OP_TAKE_FIRST) &&
                 (curr.type == D_FILTER_OP_TAKE_FIRST) )
            {
                prev.params.count = (prev.params.count < curr.params.count)
                    ? prev.params.count
                    : curr.params.count;

                continue;
            }

            // merge adjacent skip_first
            if ( (prev.type == D_FILTER_OP_SKIP_FIRST) &&
                 (curr.type == D_FILTER_OP_SKIP_FIRST) )
            {
                prev.params.count += curr.params.count;

                continue;
            }

            // flush prev
            d_filter_chain_add(result, &prev);
        }

        prev     = curr;
        has_prev = true;
    }

    // flush last buffered op
    if (has_prev)
    {
        d_filter_chain_add(result, &prev);
    }

    return result;
}

/*
d_filter_estimate_result_size
  Estimates the result size for a filter chain. Walks the chain and
estimates conservatively based on operation types.

Parameter(s):
  _chain:       the filter chain.
  _input_count: the number of input elements.
Return:
  An estimated number of output elements.
*/
size_t
d_filter_estimate_result_size
(
    const struct d_filter_chain* _chain,
    size_t                       _input_count
)
{
    size_t estimated;
    size_t i;

    if (!_chain)
    {
        return 0;
    }

    estimated = _input_count;

    for (i = 0; i < _chain->count; i++)
    {
        const struct d_filter_operation* op;

        op = &_chain->operations[i];

        switch (op->type)
        {
        case D_FILTER_OP_TAKE_FIRST:
        case D_FILTER_OP_TAKE_LAST:
        case D_FILTER_OP_HEAD:
        case D_FILTER_OP_TAIL:
            if (op->params.count < estimated)
            {
                estimated = op->params.count;
            }

            break;

        case D_FILTER_OP_SKIP_FIRST:
        case D_FILTER_OP_SKIP_LAST:
            if (op->params.count < estimated)
            {
                estimated -= op->params.count;
            }
            else
            {
                estimated = 0;
            }

            break;

        case D_FILTER_OP_INIT:
        case D_FILTER_OP_REST:
            if (estimated > 0)
            {
                estimated--;
            }

            break;

        case D_FILTER_OP_RANGE:
            if ( (op->params.start < estimated) &&
                 (op->params.end > op->params.start) )
            {
                size_t range = op->params.end - op->params.start;

                if (range < estimated)
                {
                    estimated = range;
                }
            }
            else
            {
                estimated = 0;
            }

            break;

        case D_FILTER_OP_INDICES:
            if (op->params.indices_count < estimated)
            {
                estimated = op->params.indices_count;
            }

            break;

        default:
            // WHERE, DISTINCT, etc. are data-dependent;
            // keep current estimate
            break;
        }
    }

    return estimated;
}


///////////////////////////////////////////////////////////////////////////////
///             VIII. ITERATOR INTERFACE                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_iterator_new
  Creates a new filter iterator. Precomputes matching indices by
applying the filter chain and mapping results back to original
positions.

Parameter(s):
  _chain:        the filter chain to iterate.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  A pointer to a newly allocated iterator, or NULL on failure.
*/
struct d_filter_iterator*
d_filter_iterator_new
(
    const struct d_filter_chain* _chain,
    const void*                  _input,
    size_t                       _count,
    size_t                       _element_size
)
{
    struct d_filter_iterator* iter;
    size_t                    idx_count;

    iter = malloc(sizeof(struct d_filter_iterator));

    if (!iter)
    {
        return NULL;
    }

    iter->input         = _input;
    iter->input_count   = _count;
    iter->element_size  = _element_size;
    iter->chain         = _chain;
    iter->indices       = NULL;
    iter->indices_count = 0;
    iter->indices_pos   = 0;
    iter->exhausted     = true;

    // precompute matching indices
    if ( (_chain) &&
         (_input) &&
         (_count > 0)        &&
         (_element_size > 0) )
    {
        iter->indices = d_filter_get_indices(
                            _chain, _input, _count,
                            _element_size, &idx_count);
        iter->indices_count = idx_count;
        iter->exhausted     = (idx_count == 0);
    }

    return iter;
}

/*
d_filter_iterator_has_next
  Tests whether the iterator has more elements.

Parameter(s):
  _iter: the iterator to query.
Return:
  true if more elements are available.
*/
bool
d_filter_iterator_has_next
(
    const struct d_filter_iterator* _iter
)
{
    if (!_iter)
    {
        return false;
    }

    return (!_iter->exhausted);
}

/*
d_filter_iterator_next
  Advances the iterator and returns the next matching element.

Parameter(s):
  _iter: the iterator to advance.
Return:
  A pointer to the next matching element, or NULL if exhausted.
*/
void*
d_filter_iterator_next
(
    struct d_filter_iterator* _iter
)
{
    const char* bytes;
    size_t      idx;
    void*       element;

    if ( (!_iter) ||
         (_iter->exhausted) )
    {
        return NULL;
    }

    bytes   = (const char*)_iter->input;
    idx     = _iter->indices[_iter->indices_pos];
    element = (void*)(bytes + (idx * _iter->element_size));

    _iter->indices_pos++;

    if (_iter->indices_pos >= _iter->indices_count)
    {
        _iter->exhausted = true;
    }

    return element;
}

/*
d_filter_iterator_reset
  Resets the iterator to the beginning.

Parameter(s):
  _iter: the iterator to reset.
Return:
  none.
*/
void
d_filter_iterator_reset
(
    struct d_filter_iterator* _iter
)
{
    if (!_iter)
    {
        return;
    }

    _iter->indices_pos = 0;
    _iter->exhausted   = (_iter->indices_count == 0);

    return;
}

/*
d_filter_iterator_free
  Frees all resources owned by a filter iterator.

Parameter(s):
  _iter: the iterator to free; may be NULL.
Return:
  none.
*/
void
d_filter_iterator_free
(
    struct d_filter_iterator* _iter
)
{
    if (!_iter)
    {
        return;
    }

    if (_iter->indices)
    {
        free(_iter->indices);
    }

    free(_iter);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             IX.   FLUENT FILTER BUILDER                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_filter_builder_new
  Creates a new fluent filter builder.

Parameter(s):
  (none)
Return:
  A pointer to a newly allocated builder, or NULL on failure.
*/
struct d_filter_builder*
d_filter_builder_new
(
    void
)
{
    struct d_filter_builder* builder;

    builder = malloc(sizeof(struct d_filter_builder));

    if (!builder)
    {
        return NULL;
    }

    builder->chain         = d_filter_chain_new();
    builder->error_code    = 0;
    builder->error_message = NULL;

    if (!builder->chain)
    {
        free(builder);

        return NULL;
    }

    return builder;
}

/*
d_filter_builder_from_chain
  Creates a filter builder from an existing chain.

Parameter(s):
  _chain: the chain to wrap.
Return:
  A pointer to a newly allocated builder, or NULL on failure.
*/
struct d_filter_builder*
d_filter_builder_from_chain
(
    struct d_filter_chain* _chain
)
{
    struct d_filter_builder* builder;

    builder = malloc(sizeof(struct d_filter_builder));

    if (!builder)
    {
        return NULL;
    }

    builder->chain         = _chain;
    builder->error_code    = 0;
    builder->error_message = NULL;

    return builder;
}

/*
d_filter_builder_add_op_internal
  Internal helper that adds an operation to the builder's chain and
handles error propagation.

Parameter(s):
  _builder: the builder.
  _op:      the operation to add.
Return:
  The builder pointer for chaining.
*/
static struct d_filter_builder*
d_filter_builder_add_op_internal
(
    struct d_filter_builder*    _builder,
    struct d_filter_operation*  _op
)
{
    if ( (!_builder) ||
         (_builder->error_code != 0) )
    {
        return _builder;
    }

    if (!d_filter_chain_add(_builder->chain, _op))
    {
        _builder->error_code = -1;
    }

    return _builder;
}

/*
d_filter_builder_take_first
  Adds a take-first operation to the builder.

Parameter(s):
  _builder: the builder.
  _n:       the number of elements to take.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_take_first
(
    struct d_filter_builder* _builder,
    size_t                   _n
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_take_first(_n));
}

/*
d_filter_builder_take_last
  Adds a take-last operation to the builder.

Parameter(s):
  _builder: the builder.
  _n:       the number of elements to take.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_take_last
(
    struct d_filter_builder* _builder,
    size_t                   _n
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_take_last(_n));
}

/*
d_filter_builder_take_nth
  Adds a take-nth operation to the builder.

Parameter(s):
  _builder: the builder.
  _n:       the step between selected elements.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_take_nth
(
    struct d_filter_builder* _builder,
    size_t                   _n
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_take_nth(_n));
}

/*
d_filter_builder_skip_first
  Adds a skip-first operation to the builder.

Parameter(s):
  _builder: the builder.
  _n:       the number of elements to skip.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_skip_first
(
    struct d_filter_builder* _builder,
    size_t                   _n
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_skip_first(_n));
}

/*
d_filter_builder_skip_last
  Adds a skip-last operation to the builder.

Parameter(s):
  _builder: the builder.
  _n:       the number of elements to skip.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_skip_last
(
    struct d_filter_builder* _builder,
    size_t                   _n
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_skip_last(_n));
}

/*
d_filter_builder_where
  Adds a where (predicate) operation to the builder.

Parameter(s):
  _builder: the builder.
  _test:    the predicate function.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_where
(
    struct d_filter_builder* _builder,
    fn_predicate              _test
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_where(_test));
}

/*
d_filter_builder_where_context
  Adds a context-predicate operation to the builder.

Parameter(s):
  _builder: the builder.
  _test:    the predicate function.
  _context: the context pointer.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_where_context
(
    struct d_filter_builder* _builder,
    fn_predicate              _test,
    void*                    _context
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_where_context(_test, _context));
}

/*
d_filter_builder_where_not
  Adds a negated predicate operation to the builder.

Parameter(s):
  _builder: the builder.
  _test:    the predicate function.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_where_not
(
    struct d_filter_builder* _builder,
    fn_predicate              _test
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_where_not(_test));
}

/*
d_filter_builder_range
  Adds a range operation to the builder.

Parameter(s):
  _builder: the builder.
  _start:   the inclusive start index.
  _end:     the exclusive end index.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_range
(
    struct d_filter_builder* _builder,
    size_t                   _start,
    size_t                   _end
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_range(_start, _end));
}

/*
d_filter_builder_slice
  Adds a slice operation to the builder.

Parameter(s):
  _builder: the builder.
  _start:   the inclusive start index.
  _end:     the exclusive end index.
  _step:    the step between elements.
Return:
  The builder pointer for chaining.
*/

D_INLINE struct d_filter_builder*
d_filter_builder_slice
(
    struct d_filter_builder* _builder,
    size_t                   _start,
    size_t                   _end,
    size_t                   _step
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_slice(_start, _end, _step));
}

/*
d_filter_builder_distinct
  Adds a distinct operation to the builder.

Parameter(s):
  _builder: the builder.
  _cmp:     the comparator for detecting duplicates.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_distinct
(
    struct d_filter_builder* _builder,
    fn_function_comparator   _comparator
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_distinct(_comparator));
}

/*
d_filter_builder_reverse
  Adds a reverse operation to the builder.

Parameter(s):
  _builder: the builder.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_reverse
(
    struct d_filter_builder* _builder
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_reverse());
}

/*
d_filter_builder_at
  Adds a single-index selection operation to the builder.

Parameter(s):
  _builder: the builder.
  _index:   the index to select.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_at
(
    struct d_filter_builder* _builder,
    size_t                   _index
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_at(_index));
}

/*
d_filter_builder_at_indices
  Adds a multi-index selection operation to the builder.

Parameter(s):
  _builder: the builder.
  _indices: the array of indices.
  _n:       the number of indices.
Return:
  The builder pointer for chaining.
*/
D_INLINE struct d_filter_builder*
d_filter_builder_at_indices
(
    struct d_filter_builder* _builder,
    const size_t*            _indices,
    size_t                   _n
)
{
    return d_filter_builder_add_op_internal(_builder, d_filter_at_indices(_indices, _n));
}

/*
d_filter_builder_build
  Finalizes the builder and returns the constructed chain. The builder
no longer owns the chain after this call.

Parameter(s):
  _builder: the builder to finalize.
Return:
  The constructed filter chain, or NULL on error.
*/
struct d_filter_chain*
d_filter_builder_build
(
    struct d_filter_builder* _builder
)
{
    struct d_filter_chain* chain;

    if (!_builder)
    {
        return NULL;
    }

    chain           = _builder->chain;
    _builder->chain = NULL;

    return chain;
}

/*
d_filter_builder_apply
  Finalizes the builder and applies the chain to input data.

Parameter(s):
  _builder:      the builder to finalize and apply.
  _input:        the source array.
  _count:        the number of elements.
  _element_size: the size in bytes of each element.
Return:
  A d_filter_result containing the filtered output.
*/
struct d_filter_result*
d_filter_builder_apply
(
    struct d_filter_builder* _builder,
    const void*              _input,
    size_t                   _count,
    size_t                   _element_size
)
{
    struct d_filter_result* result;

    result = malloc(sizeof(struct d_filter_result));

    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));

    if ( (!_builder) ||
         (!_builder->chain) )
    {
        result->status = D_FILTER_RESULT_INVALID;

        return result;
    }

    // delegate to apply_chain which returns a pointer
    {
        struct d_filter_result* chain_result;

        chain_result = d_filter_apply_chain(
                           _builder->chain,
                           _input, _count, _element_size);

        if (!chain_result)
        {
            result->status = D_FILTER_RESULT_ERROR;

            return result;
        }

        // transfer ownership
        *(result) = *(chain_result);

        free(chain_result);
    }

    return result;
}

/*
d_filter_builder_has_error
  Tests whether the builder has encountered an error.

Parameter(s):
  _builder: the builder to query.
Return:
  true if an error has occurred.
*/
bool
d_filter_builder_has_error
(
    const struct d_filter_builder* _builder
)
{
    if (!_builder)
    {
        return true;
    }

    return (_builder->error_code != 0);
}

/*
d_filter_builder_get_error_code
  Returns the builder's error code.

Parameter(s):
  _builder: the builder to query.
Return:
  The error code, or -1 if _builder is NULL.
*/
int
d_filter_builder_get_error_code
(
    const struct d_filter_builder* _builder
)
{
    if (!_builder)
    {
        return -1;
    }

    return _builder->error_code;
}

/*
d_filter_builder_get_error_message
  Returns the builder's error message.

Parameter(s):
  _builder: the builder to query.
Return:
  The error message string, or NULL if no error.
*/
char*
d_filter_builder_get_error_message
(
    const struct d_filter_builder* _builder
)
{
    if (!_builder)
    {
        return NULL;
    }

    return _builder->error_message;
}

/*
d_filter_builder_free
  Frees all resources owned by a filter builder.

Parameter(s):
  _builder: the builder to free; may be NULL.
Return:
  none.
*/
void
d_filter_builder_free
(
    struct d_filter_builder* _builder
)
{
    if (!_builder)
    {
        return;
    }

    if (_builder->chain)
    {
        d_filter_chain_free(_builder->chain);
    }

    if (_builder->error_message)
    {
        free(_builder->error_message);
    }

    free(_builder);

    return;
}
