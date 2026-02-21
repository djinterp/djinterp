/******************************************************************************
* djinterp [container]                                          array_filter.c
*
*   Implementation of zero-overhead filter operations for contiguous arrays.
*   All functions operate on raw (void*, count, element_size) triples and
* delegate to the functional module's filter.h where appropriate.
*
*   This file is compiled only when D_CFG_CONTAINER_FILTER_ARRAY is enabled.
*
*
* path:      \src\container\array\array_filter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.20
******************************************************************************/
#include "..\..\..\inc\container\array\array_filter.h"


///////////////////////////////////////////////////////////////////////////////
///             INTERNAL HELPERS                                            ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_ARRAY_FILTER_ELEMENT
//   macro: returns a pointer to the element at index _i within a contiguous
// byte buffer.  Casts the base pointer to unsigned char* for byte arithmetic.
#define D_INTERNAL_ARRAY_FILTER_ELEMENT(base, i, elem_size)                 \
    ((const unsigned char*)(base) + ((i) * (elem_size)))

// D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE
//   macro: mutable variant of D_INTERNAL_ARRAY_FILTER_ELEMENT for in-place operations.
#define D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(base, i, elem_size)         \
    ((unsigned char*)(base) + ((i) * (elem_size)))


/*
d_array_filter_result_error
  Internal: constructs an error d_array_filter_result.

Parameter(s):
  _status:       error status code (must be < 0).
  _element_size: element size to record in the result.
Return:
  d_array_filter_result with NULL data, count 0, and the given status.
*/
static struct d_array_filter_result
d_array_filter_result_error
(
    enum d_filter_result_type _status,
    size_t                    _element_size
)
{
    struct d_array_filter_result res = {0};

    res.data           = NULL;
    res.count          = 0;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = _status;

    return res;
}


/*
d_array_filter_result_empty
  Internal: constructs an empty (non-error) d_array_filter_result.

Parameter(s):
  _element_size: element size to record in the result.
Return:
  d_array_filter_result with NULL data, count 0, and EMPTY status.
*/
static struct d_array_filter_result
d_array_filter_result_empty
(
    size_t _element_size
)
{
    struct d_array_filter_result res = {0};

    res.data           = NULL;
    res.count          = 0;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_EMPTY;

    return res;
}


/*
d_array_filter_copy_range
  Internal: allocates a new buffer and copies a contiguous range of elements
  from source [_start, _start + _out_count) into it.

Parameter(s):
  _elements:     source buffer.
  _element_size: size of each element.
  _start:        index of first element to copy.
  _out_count:    number of elements to copy.
Return:
  d_array_filter_result owning the new buffer, or a NO_MEMORY error.
*/
static struct d_array_filter_result
d_array_filter_copy_range
(
    const void* _elements,
    size_t      _element_size,
    size_t      _start,
    size_t      _out_count
)
{
    struct d_array_filter_result res = {0};

    res.source_indices = NULL;
    res.element_size   = _element_size;

    if (_out_count == 0)
    {
        res.data   = NULL;
        res.count  = 0;
        res.status = D_FILTER_RESULT_EMPTY;

        return res;
    }

    res.data = malloc(_out_count * _element_size);

    if (!res.data)
    {
        res.count  = 0;
        res.status = D_FILTER_RESULT_NO_MEMORY;

        return res;
    }

    d_memcpy(res.data,
             D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, _start, _element_size),
             _out_count * _element_size);

    res.count  = _out_count;
    res.status = D_FILTER_RESULT_SUCCESS;

    return res;
}


///////////////////////////////////////////////////////////////////////////////
///             II.   SINGLE-OPERATION FILTER FUNCTIONS                     ///
///////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------------
// i.    take operations
// ---------------------------------------------------------------------------

/*
d_array_filter_take_first
  Returns a new result containing the first _n elements.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to take.
Return:
  d_array_filter_result owning a copy of the first min(_n, _count) elements.
*/
struct d_array_filter_result
d_array_filter_take_first
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    size_t actual;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n == 0) )
    {
        return d_array_filter_result_empty(_element_size);
    }

    actual = (_n < _count) ? _n : _count;

    return d_array_filter_copy_range(_elements, _element_size, 0, actual);
}


/*
d_array_filter_take_last
  Returns a new result containing the last _n elements.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to take from the end.
Return:
  d_array_filter_result owning a copy of the last min(_n, _count) elements.
*/
struct d_array_filter_result
d_array_filter_take_last
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    size_t actual;
    size_t start;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n == 0) )
    {
        return d_array_filter_result_empty(_element_size);
    }

    actual = (_n < _count) ? _n : _count;
    start  = _count - actual;

    return d_array_filter_copy_range(_elements, _element_size, start, actual);
}


/*
d_array_filter_take_nth
  Returns every _n-th element (indices 0, _n, 2*_n, ...).

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            step interval (must be > 0).
Return:
  d_array_filter_result owning a copy of every _n-th element.
*/
struct d_array_filter_result
d_array_filter_take_nth
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    struct d_array_filter_result res = {0};
    size_t                       out_count;
    size_t                       src;
    size_t                       dst;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_n == 0)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // calculate output count: ceil(_count / _n)
    out_count = (_count + _n - 1) / _n;

    res.data = malloc(out_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (src = 0; src < _count; src += _n)
    {
        d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, dst, _element_size),
                 D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, src, _element_size),
                 _element_size);
        ++dst;
    }

    res.count          = out_count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


/*
d_array_filter_head
  Returns a result containing only the first element.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  d_array_filter_result owning a copy of the first element, or empty if
  the source is empty.
*/
struct d_array_filter_result
d_array_filter_head
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_array_filter_take_first(_elements, _count, _element_size, 1);
}


/*
d_array_filter_tail
  Returns a result containing only the last element.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  d_array_filter_result owning a copy of the last element, or empty if
  the source is empty.
*/
struct d_array_filter_result
d_array_filter_tail
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_array_filter_take_last(_elements, _count, _element_size, 1);
}


// ---------------------------------------------------------------------------
// ii.   skip operations
// ---------------------------------------------------------------------------

/*
d_array_filter_skip_first
  Returns all elements except the first _n.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to skip from the front.
Return:
  d_array_filter_result owning a copy of elements [_n .. _count).
*/
struct d_array_filter_result
d_array_filter_skip_first
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n >= _count) )
    {
        return d_array_filter_result_empty(_element_size);
    }

    return d_array_filter_copy_range(_elements, _element_size, _n, _count - _n);
}


/*
d_array_filter_skip_last
  Returns all elements except the last _n.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to skip from the end.
Return:
  d_array_filter_result owning a copy of elements [0 .. _count - _n).
*/
struct d_array_filter_result
d_array_filter_skip_last
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n >= _count) )
    {
        return d_array_filter_result_empty(_element_size);
    }

    return d_array_filter_copy_range(_elements, _element_size, 0, _count - _n);
}


/*
d_array_filter_init
  Returns all elements except the last (Haskell-style init).

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  d_array_filter_result owning elements [0 .. _count - 1).
*/
struct d_array_filter_result
d_array_filter_init
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_array_filter_skip_last(_elements, _count, _element_size, 1);
}


/*
d_array_filter_rest
  Returns all elements except the first (Haskell-style tail / rest).

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  d_array_filter_result owning elements [1 .. _count).
*/
struct d_array_filter_result
d_array_filter_rest
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_array_filter_skip_first(_elements, _count, _element_size, 1);
}


// ---------------------------------------------------------------------------
// iii.  range and slice operations
// ---------------------------------------------------------------------------

/*
d_array_filter_range
  Returns elements in the half-open range [_start, _end).
  Clamps _end to _count.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _start:        start index (inclusive).
  _end:          end index (exclusive).
Return:
  d_array_filter_result owning a copy of the selected range.
*/
struct d_array_filter_result
d_array_filter_range
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _start,
    size_t      _end
)
{
    size_t clamped_end;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // clamp end to count
    clamped_end = (_end < _count) ? _end : _count;

    // inverted or empty range
    if (_start >= clamped_end)
    {
        return d_array_filter_result_empty(_element_size);
    }

    return d_array_filter_copy_range(_elements, _element_size,
                           _start, clamped_end - _start);
}


/*
d_array_filter_slice
  Returns elements selected by [_start : _end : _step] semantics.
  Clamps _end to _count.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _start:        start index (inclusive).
  _end:          end index (exclusive).
  _step:         step interval (must be > 0).
Return:
  d_array_filter_result owning a copy of the selected elements.
*/
struct d_array_filter_result
d_array_filter_slice
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _start,
    size_t      _end,
    size_t      _step
)
{
    struct d_array_filter_result res = {0};
    size_t                       clamped_end;
    size_t                       range_len;
    size_t                       out_count;
    size_t                       src;
    size_t                       dst;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_step == 0)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    clamped_end = (_end < _count) ? _end : _count;

    if (_start >= clamped_end)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // step == 1 is a contiguous copy
    if (_step == 1)
    {
        return d_array_filter_copy_range(_elements, _element_size,
                               _start, clamped_end - _start);
    }

    range_len = clamped_end - _start;
    out_count = (range_len + _step - 1) / _step;

    res.data = malloc(out_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (src = _start; src < clamped_end; src += _step)
    {
        d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, dst, _element_size),
                 D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, src, _element_size),
                 _element_size);
        ++dst;
    }

    res.count          = out_count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


// ---------------------------------------------------------------------------
// iv.   predicate-based operations
// ---------------------------------------------------------------------------

/*
d_array_filter_where
  Returns all elements for which _test returns true.
  Performs a two-pass approach: first counts matching elements, then copies.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function.
  _context:      opaque context passed to the predicate.
Return:
  d_array_filter_result owning a copy of all matching elements.
*/
struct d_array_filter_result
d_array_filter_where
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    struct d_array_filter_result res = {0};
    size_t                       match_count;
    size_t                       i;
    size_t                       dst;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_test)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // pass 1: count matches
    match_count = 0;

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            ++match_count;
        }
    }

    if (match_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // pass 2: copy matching elements
    res.data = malloc(match_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, dst, _element_size),
                     D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size),
                     _element_size);
            ++dst;
        }
    }

    res.count          = match_count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


/*
d_array_filter_where_not
  Returns all elements for which _test returns false.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function; elements that FAIL are kept.
  _context:      opaque context passed to the predicate.
Return:
  d_array_filter_result owning a copy of all non-matching elements.
*/
struct d_array_filter_result
d_array_filter_where_not
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    struct d_array_filter_result res = {0};
    size_t                       match_count;
    size_t                       i;
    size_t                       dst;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_test)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // pass 1: count non-matches
    match_count = 0;

    for (i = 0; i < _count; ++i)
    {
        if (!_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            ++match_count;
        }
    }

    if (match_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // pass 2: copy
    res.data = malloc(match_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (i = 0; i < _count; ++i)
    {
        if (!_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, dst, _element_size),
                     D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size),
                     _element_size);
            ++dst;
        }
    }

    res.count          = match_count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


// ---------------------------------------------------------------------------
// v.    index-based operations
// ---------------------------------------------------------------------------

/*
d_array_filter_at_indices
  Returns elements at the specified indices.  Duplicate indices are honoured
  (the same element is copied multiple times).  Out-of-bounds indices are
  silently skipped.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _indices:      array of indices to select.
  _index_count:  number of indices.
Return:
  d_array_filter_result owning copies of the selected elements.
*/
struct d_array_filter_result
d_array_filter_at_indices
(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const size_t* _indices,
    size_t        _index_count
)
{
    struct d_array_filter_result res = {0};
    size_t                       valid_count;
    size_t                       i;
    size_t                       dst;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_indices)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_index_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // pass 1: count valid (in-bounds) indices
    valid_count = 0;

    for (i = 0; i < _index_count; ++i)
    {
        if (_indices[i] < _count)
        {
            ++valid_count;
        }
    }

    if (valid_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    res.data = malloc(valid_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    // pass 2: copy
    dst = 0;

    for (i = 0; i < _index_count; ++i)
    {
        if (_indices[i] < _count)
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, dst, _element_size),
                     D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, _indices[i], _element_size),
                     _element_size);
            ++dst;
        }
    }

    res.count          = valid_count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


// ---------------------------------------------------------------------------
// vi.   transformation operations
// ---------------------------------------------------------------------------

/*
d_array_filter_distinct
  Returns a copy of the array with duplicate elements removed.
  Preserves the order of first occurrence.  Uses an O(n^2) scan suitable
  for moderate array sizes; a sort-based approach can be substituted for
  large datasets.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _comparator:   three-way comparator for element equality (0 == equal).
Return:
  d_array_filter_result owning a deduplicated copy.
*/
struct d_array_filter_result
d_array_filter_distinct
(
    const void*            _elements,
    size_t                 _count,
    size_t                 _element_size,
    fn_function_comparator _comparator
)
{
    struct d_array_filter_result res = {0};
    size_t                       unique_count;
    size_t                       i;
    size_t                       j;
    bool                         is_dup;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_comparator)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // worst case: all unique -> allocate for _count elements
    res.data = malloc(_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    // always include the first element
    d_memcpy(res.data,
             _elements,
             _element_size);
    unique_count = 1;

    for (i = 1; i < _count; ++i)
    {
        is_dup = false;

        // check against all previously accepted unique elements
        for (j = 0; j < unique_count; ++j)
        {
            if (_comparator(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size),
                            D_INTERNAL_ARRAY_FILTER_ELEMENT(res.data, j, _element_size),
                            NULL) == 0)
            {
                is_dup = true;

                break;
            }
        }

        if (!is_dup)
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, unique_count, _element_size),
                     D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size),
                     _element_size);
            ++unique_count;
        }
    }

    res.count          = unique_count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


/*
d_array_filter_reverse
  Returns a copy of the array with elements in reverse order.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  d_array_filter_result owning a reversed copy.
*/
struct d_array_filter_result
d_array_filter_reverse
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    struct d_array_filter_result res = {0};
    size_t                       i;

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    res.data = malloc(_count * _element_size);

    if (!res.data)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    for (i = 0; i < _count; ++i)
    {
        d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(res.data, i, _element_size),
                 D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, _count - 1 - i, _element_size),
                 _element_size);
    }

    res.count          = _count;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_SUCCESS;

    return res;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  IN-PLACE FILTER OPERATIONS                           ///
///////////////////////////////////////////////////////////////////////////////

/*
d_array_filter_in_place
  Filters the array in-place, keeping only elements where _test returns true.
  Surviving elements are compacted to the front of the buffer.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function.
  _context:      opaque context passed to the predicate.
Return:
  New count of surviving elements.
*/
size_t
d_array_filter_in_place
(
    void*        _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t write;
    size_t read;

    if ( (!_elements) ||
         (!_test)     ||
         (_count == 0) )
    {
        return 0;
    }

    write = 0;

    for (read = 0; read < _count; ++read)
    {
        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, read, _element_size), _context))
        {
            if (write != read)
            {
                memmove(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(_elements, write, _element_size),
                          D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, read, _element_size),
                          _element_size);
            }

            ++write;
        }
    }

    return write;
}


/*
d_array_filter_in_place_not
  Filters the array in-place, keeping only elements where _test returns false.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function; elements that FAIL are kept.
  _context:      opaque context passed to the predicate.
Return:
  New count of surviving elements.
*/
size_t
d_array_filter_in_place_not
(
    void*        _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t write;
    size_t read;

    if ( (!_elements) ||
         (!_test)     ||
         (_count == 0) )
    {
        return 0;
    }

    write = 0;

    for (read = 0; read < _count; ++read)
    {
        if (!_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, read, _element_size), _context))
        {
            if (write != read)
            {
                memmove(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(_elements, write, _element_size),
                          D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, read, _element_size),
                          _element_size);
            }

            ++write;
        }
    }

    return write;
}


/*
d_array_filter_in_place_take_first
  Truncates the array in-place to the first _n elements.
  (No data is moved; the caller simply uses the returned count.)

Parameter(s):
  _elements:     mutable source array (unused but validated).
  _count:        number of elements in source.
  _element_size: size of each element in bytes (unused but present for API
                 consistency).
  _n:            number of elements to keep.
Return:
  min(_n, _count).
*/
size_t
d_array_filter_in_place_take_first
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    size_t _n
)
{
    (void)_element_size;

    if (!_elements)
    {
        return 0;
    }

    return (_n < _count) ? _n : _count;
}


/*
d_array_filter_in_place_skip_first
  Removes the first _n elements by shifting the remainder to the front.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to skip.
Return:
  New count after shifting (_count - _n, or 0 if _n >= _count).
*/
size_t
d_array_filter_in_place_skip_first
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    size_t _n
)
{
    size_t remaining;

    if (!_elements)
    {
        return 0;
    }

    if (_n >= _count)
    {
        return 0;
    }

    if (_n == 0)
    {
        return _count;
    }

    remaining = _count - _n;

    memmove(_elements,
              D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, _n, _element_size),
              remaining * _element_size);

    return remaining;
}


/*
d_array_filter_in_place_distinct
  Removes duplicate elements in-place.  Preserves order of first occurrence.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _comparator:   three-way comparator (0 == equal).
Return:
  New count of unique elements.
*/
size_t
d_array_filter_in_place_distinct
(
    void*                  _elements,
    size_t                 _count,
    size_t                 _element_size,
    fn_function_comparator _comparator
)
{
    size_t unique;
    size_t i;
    size_t j;
    bool   is_dup;

    if ( (!_elements)  ||
         (!_comparator) )
    {
        return 0;
    }

    if (_count <= 1)
    {
        return _count;
    }

    unique = 1;  // first element is always unique

    for (i = 1; i < _count; ++i)
    {
        is_dup = false;

        for (j = 0; j < unique; ++j)
        {
            if (_comparator(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size),
                            D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, j, _element_size),
                            NULL) == 0)
            {
                is_dup = true;

                break;
            }
        }

        if (!is_dup)
        {
            if (unique != i)
            {
                memmove(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(_elements, unique, _element_size),
                          D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size),
                          _element_size);
            }

            ++unique;
        }
    }

    return unique;
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   CHAIN AND COMBINATOR APPLICATION                      ///
///////////////////////////////////////////////////////////////////////////////

/*
d_array_filter_apply_single_op
  Internal: applies a single d_filter_operation to array data, producing
  a new d_array_filter_result.  Used as the inner step of chain execution.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _op:           filter operation to apply.
Return:
  d_array_filter_result for the single operation.
*/
static struct d_array_filter_result
d_array_filter_apply_single_op
(
    const void*                    _elements,
    size_t                         _count,
    size_t                         _element_size,
    const struct d_filter_operation* _op
)
{
    if (!_op)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    switch (_op->type)
    {
        case D_FILTER_OP_TAKE_FIRST:
            return d_array_filter_take_first(_elements, _count,
                                            _element_size,
                                            _op->params.count);

        case D_FILTER_OP_TAKE_LAST:
            return d_array_filter_take_last(_elements, _count,
                                           _element_size,
                                           _op->params.count);

        case D_FILTER_OP_TAKE_NTH:
            return d_array_filter_take_nth(_elements, _count,
                                          _element_size,
                                          _op->params.step);

        case D_FILTER_OP_HEAD:
            return d_array_filter_head(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_TAIL:
            return d_array_filter_tail(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_SKIP_FIRST:
            return d_array_filter_skip_first(_elements, _count,
                                            _element_size,
                                            _op->params.count);

        case D_FILTER_OP_SKIP_LAST:
            return d_array_filter_skip_last(_elements, _count,
                                           _element_size,
                                           _op->params.count);

        case D_FILTER_OP_INIT:
            return d_array_filter_init(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_REST:
            return d_array_filter_rest(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_RANGE:
            return d_array_filter_range(_elements, _count,
                                       _element_size,
                                       _op->params.start,
                                       _op->params.end);

        case D_FILTER_OP_SLICE:
            return d_array_filter_slice(_elements, _count,
                                       _element_size,
                                       _op->params.start,
                                       _op->params.end,
                                       _op->params.step);

        case D_FILTER_OP_WHERE:
            return d_array_filter_where(_elements, _count,
                                       _element_size,
                                       _op->params.test,
                                       _op->params.context);

        case D_FILTER_OP_WHERE_NOT:
            return d_array_filter_where_not(_elements, _count,
                                           _element_size,
                                           _op->params.test,
                                           _op->params.context);

        case D_FILTER_OP_INDICES:
            return d_array_filter_at_indices(_elements, _count,
                                            _element_size,
                                            _op->params.indices,
                                            _op->params.indices_count);

        case D_FILTER_OP_DISTINCT:
            return d_array_filter_distinct(_elements, _count,
                                          _element_size,
                                          _op->params.comparator);

        case D_FILTER_OP_REVERSE:
            return d_array_filter_reverse(_elements, _count,
                                         _element_size);

        case D_FILTER_OP_NONE:
        default:
            // identity: copy everything
            return d_array_filter_copy_range(_elements, _element_size, 0, _count);
    }
}


/*
d_array_filter_apply_chain
  Sequentially applies each operation in a filter chain, piping the output
  of one step as the input of the next.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _chain:        chain of operations to apply.
Return:
  d_array_filter_result for the final output.
*/
struct d_array_filter_result
d_array_filter_apply_chain
(
    const void*                  _elements,
    size_t                       _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain
)
{
    struct d_array_filter_result current = {0};
    struct d_array_filter_result next = {0};
    const void*                  input_data;
    size_t                       input_count;
    size_t                       i;
    bool                         owns_current;

    if (!_chain)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_elements)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    // empty chain -> copy everything
    if (_chain->count == 0)
    {
        return d_array_filter_copy_range(_elements, _element_size, 0, _count);
    }

    // first operation: input is the original array (not owned by us)
    input_data   = _elements;
    input_count  = _count;
    owns_current = false;

    for (i = 0; i < _chain->count; ++i)
    {
        next = d_array_filter_apply_single_op(input_data,
                                    input_count,
                                    _element_size,
                                    &_chain->operations[i]);

        // free the intermediate buffer (but not the original input)
        if (owns_current)
        {
            d_array_filter_result_free(&current);
        }

        // propagate errors immediately
        if (next.status < 0)
        {
            return next;
        }

        current      = next;
        input_data   = current.data;
        input_count  = current.count;
        owns_current = true;

        // short-circuit on empty intermediate result
        if (current.count == 0)
        {
            break;
        }
    }

    return current;
}


/*
d_array_filter_apply_union
  Applies multiple filter chains and produces their union (elements that
  appear in any chain's output).  Deduplication uses the provided comparator.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _combo:        union combinator containing multiple chains.
  _comparator:   comparator for deduplication.
Return:
  d_array_filter_result owning the union of all chain outputs.
*/
struct d_array_filter_result
d_array_filter_apply_union
(
    const void*                 _elements,
    size_t                      _count,
    size_t                      _element_size,
    const struct d_filter_union* _combo,
    fn_function_comparator      _comparator
)
{
    struct d_array_filter_result* chain_results = NULL;
    struct d_array_filter_result  merged = {0};
    size_t                        total;
    size_t                        pos;
    size_t                        i;

    if ( (!_combo) ||
         (!_elements) )
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_combo->count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // apply each chain independently
    chain_results = (struct d_array_filter_result*)
                    calloc(_combo->count,
                             sizeof(struct d_array_filter_result));

    if (!chain_results)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    total = 0;

    for (i = 0; i < _combo->count; ++i)
    {
        chain_results[i] = d_array_filter_apply_chain(_elements,
                                                      _count,
                                                      _element_size,
                                                      _combo->filters[i]);
        total += chain_results[i].count;
    }

    // merge all results into one buffer
    merged.data = malloc(total * _element_size);

    if ( (!merged.data) &&
         (total > 0) )
    {
        for (i = 0; i < _combo->count; ++i)
        {
            d_array_filter_result_free(&chain_results[i]);
        }

        free(chain_results);

        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    pos = 0;

    for (i = 0; i < _combo->count; ++i)
    {
        if (chain_results[i].data && chain_results[i].count > 0)
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(merged.data, pos, _element_size),
                     chain_results[i].data,
                     chain_results[i].count * _element_size);
            pos += chain_results[i].count;
        }

        d_array_filter_result_free(&chain_results[i]);
    }

    free(chain_results);

    merged.count        = pos;
    merged.element_size = _element_size;
    merged.source_indices = NULL;
    merged.status       = (pos > 0) ? D_FILTER_RESULT_SUCCESS
                                    : D_FILTER_RESULT_EMPTY;

    // deduplicate in-place if comparator is provided
    if ( (_comparator) &&
         (merged.count > 1) )
    {
        merged.count = d_array_filter_in_place_distinct(merged.data,
                                                        merged.count,
                                                        _element_size,
                                                        _comparator);
    }

    return merged;
}


/*
d_array_filter_apply_intersection
  Applies multiple filter chains and produces their intersection (elements
  that appear in every chain's output).

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _combo:        intersection combinator.
  _comparator:   comparator for element equality.
Return:
  d_array_filter_result owning the intersection.
*/
struct d_array_filter_result
d_array_filter_apply_intersection
(
    const void*                         _elements,
    size_t                              _count,
    size_t                              _element_size,
    const struct d_filter_intersection* _combo,
    fn_function_comparator              _comparator
)
{
    struct d_array_filter_result* chain_results = NULL;
    struct d_array_filter_result  result = {0};
    size_t                        i;
    size_t                        j;
    size_t                        k;
    size_t                        out_count;
    bool                          found;

    if ( (!_combo)     ||
         (!_elements)  ||
         (!_comparator) )
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_combo->count == 0)
    {
        return d_array_filter_result_empty(_element_size);
    }

    // apply each chain
    chain_results = (struct d_array_filter_result*)
                    calloc(_combo->count,
                             sizeof(struct d_array_filter_result));

    if (!chain_results)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    for (i = 0; i < _combo->count; ++i)
    {
        chain_results[i] = d_array_filter_apply_chain(_elements,
                                                      _count,
                                                      _element_size,
                                                      _combo->filters[i]);
    }

    // intersect: iterate over first chain's results, keep elements
    // present in all other chains
    result.data = malloc(chain_results[0].count * _element_size);

    if ( (!result.data) &&
         (chain_results[0].count > 0) )
    {
        for (i = 0; i < _combo->count; ++i)
        {
            d_array_filter_result_free(&chain_results[i]);
        }

        free(chain_results);

        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    out_count = 0;

    for (j = 0; j < chain_results[0].count; ++j)
    {
        bool in_all;

        in_all = true;

        for (i = 1; i < _combo->count; ++i)
        {
            found = false;

            for (k = 0; k < chain_results[i].count; ++k)
            {
                if (_comparator(
                        D_INTERNAL_ARRAY_FILTER_ELEMENT(chain_results[0].data, j, _element_size),
                        D_INTERNAL_ARRAY_FILTER_ELEMENT(chain_results[i].data, k, _element_size),
                        NULL) == 0)
                {
                    found = true;

                    break;
                }
            }

            if (!found)
            {
                in_all = false;

                break;
            }
        }

        if (in_all)
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(result.data, out_count, _element_size),
                     D_INTERNAL_ARRAY_FILTER_ELEMENT(chain_results[0].data, j, _element_size),
                     _element_size);
            ++out_count;
        }
    }

    for (i = 0; i < _combo->count; ++i)
    {
        d_array_filter_result_free(&chain_results[i]);
    }

    free(chain_results);

    result.count          = out_count;
    result.element_size   = _element_size;
    result.source_indices = NULL;
    result.status         = (out_count > 0) ? D_FILTER_RESULT_SUCCESS
                                            : D_FILTER_RESULT_EMPTY;

    return result;
}


/*
d_array_filter_apply_difference
  Applies two filter chains (A and B) and returns A - B (elements in A's
  output that do not appear in B's output).

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _diff:         difference combinator (contains include and exclude chains).
  _comparator:   comparator for element equality.
Return:
  d_array_filter_result owning A minus B.
*/
struct d_array_filter_result
d_array_filter_apply_difference
(
    const void*                       _elements,
    size_t                            _count,
    size_t                            _element_size,
    const struct d_filter_difference* _diff,
    fn_function_comparator            _comparator
)
{
    struct d_array_filter_result result_a = {0};
    struct d_array_filter_result result_b = {0};
    struct d_array_filter_result result = {0};
    size_t                       out_count;
    size_t                       i;
    size_t                       j;
    bool                         found;

    if ( (!_diff)      ||
         (!_elements)  ||
         (!_comparator) )
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    result_a = d_array_filter_apply_chain(_elements, _count,
                                          _element_size,
                                          _diff->include);
    result_b = d_array_filter_apply_chain(_elements, _count,
                                          _element_size,
                                          _diff->exclude);

    // if A is empty, result is empty
    if (result_a.count == 0)
    {
        d_array_filter_result_free(&result_b);

        return result_a;
    }

    // if B is empty, result is all of A
    if (result_b.count == 0)
    {
        d_array_filter_result_free(&result_b);

        return result_a;
    }

    result.data = malloc(result_a.count * _element_size);

    if (!result.data)
    {
        d_array_filter_result_free(&result_a);
        d_array_filter_result_free(&result_b);

        return d_array_filter_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    out_count = 0;

    for (i = 0; i < result_a.count; ++i)
    {
        found = false;

        for (j = 0; j < result_b.count; ++j)
        {
            if (_comparator(
                    D_INTERNAL_ARRAY_FILTER_ELEMENT(result_a.data, i, _element_size),
                    D_INTERNAL_ARRAY_FILTER_ELEMENT(result_b.data, j, _element_size),
                    NULL) == 0)
            {
                found = true;

                break;
            }
        }

        if (!found)
        {
            d_memcpy(D_INTERNAL_ARRAY_FILTER_ELEMENT_MUTABLE(result.data, out_count, _element_size),
                     D_INTERNAL_ARRAY_FILTER_ELEMENT(result_a.data, i, _element_size),
                     _element_size);
            ++out_count;
        }
    }

    d_array_filter_result_free(&result_a);
    d_array_filter_result_free(&result_b);

    result.count          = out_count;
    result.element_size   = _element_size;
    result.source_indices = NULL;
    result.status         = (out_count > 0) ? D_FILTER_RESULT_SUCCESS
                                            : D_FILTER_RESULT_EMPTY;

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///             V.    QUERY FUNCTIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_array_filter_count_where
  Counts the number of elements satisfying a predicate.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  Number of elements for which _test returns true.
*/
size_t
d_array_filter_count_where
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t matches;
    size_t i;

    if ( (!_elements) ||
         (!_test) )
    {
        return 0;
    }

    matches = 0;

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            ++matches;
        }
    }

    return matches;
}


/*
d_array_filter_any_match
  Returns true if at least one element satisfies the predicate.
  Short-circuits on first match.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  true if any element matches, false otherwise.
*/
bool
d_array_filter_any_match
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t i;

    if ( (!_elements) ||
         (!_test)     ||
         (_count == 0) )
    {
        return false;
    }

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            return true;
        }
    }

    return false;
}


/*
d_array_filter_all_match
  Returns true if every element satisfies the predicate (vacuously true
  for empty arrays).  Short-circuits on first non-match.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  true if all elements match, false otherwise.
*/
bool
d_array_filter_all_match
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t i;

    // vacuous truth
    if (_count == 0)
    {
        return true;
    }

    if ( (!_elements) ||
         (!_test) )
    {
        return false;
    }

    for (i = 0; i < _count; ++i)
    {
        if (!_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            return false;
        }
    }

    return true;
}


/*
d_array_filter_none_match
  Returns true if no element satisfies the predicate (vacuously true
  for empty arrays).  Short-circuits on first match.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  true if no elements match, false otherwise.
*/
bool
d_array_filter_none_match
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    return !d_array_filter_any_match(_elements, _count,
                                     _element_size, _test, _context);
}


/*
d_array_filter_find_first
  Returns a pointer to the first matching element in the source array.
  Does NOT allocate; the returned pointer points into the original array.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  Pointer into _elements, or NULL if no match.
*/
void*
d_array_filter_find_first
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t i;

    if ( (!_elements) ||
         (!_test)     ||
         (_count == 0) )
    {
        return NULL;
    }

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            // cast away const: caller received const void* but
            // the returned pointer is to the original mutable source
            return (void*)D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size);
        }
    }

    return NULL;
}


/*
d_array_filter_find_last
  Returns a pointer to the last matching element in the source array.
  Does NOT allocate; the returned pointer points into the original array.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  Pointer into _elements, or NULL if no match.
*/
void*
d_array_filter_find_last
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    size_t i;

    if ( (!_elements) ||
         (!_test)     ||
         (_count == 0) )
    {
        return NULL;
    }

    // iterate backwards
    i = _count;

    while (i > 0)
    {
        --i;

        if (_test(D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size), _context))
        {
            return (void*)D_INTERNAL_ARRAY_FILTER_ELEMENT(_elements, i, _element_size);
        }
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   RESULT MANAGEMENT                                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_array_filter_result_data
  Returns the data pointer from a filter result.

Parameter(s):
  _result: pointer to the result.
Return:
  The internal data pointer, or NULL if _result is NULL or has no data.
*/
void*
d_array_filter_result_data
(
    const struct d_array_filter_result* _result
)
{
    if (!_result)
    {
        return NULL;
    }

    return _result->data;
}


/*
d_array_filter_result_count
  Returns the element count from a filter result.

Parameter(s):
  _result: pointer to the result.
Return:
  Number of elements, or 0 if _result is NULL.
*/
size_t
d_array_filter_result_count
(
    const struct d_array_filter_result* _result
)
{
    if (!_result)
    {
        return 0;
    }

    return _result->count;
}


/*
d_array_filter_result_ok
  Returns true if the result status is non-negative (SUCCESS or EMPTY).

Parameter(s):
  _result: pointer to the result.
Return:
  true if OK, false on error or NULL.
*/
bool
d_array_filter_result_ok
(
    const struct d_array_filter_result* _result
)
{
    if (!_result)
    {
        return false;
    }

    return _result->status >= 0;
}


/*
d_array_filter_result_release
  Transfers ownership of the data buffer to the caller.
  After this call, the result's data pointer is set to NULL.

Parameter(s):
  _result:    pointer to the result.
  _out_count: (optional) receives the element count.
Return:
  The data pointer (caller must free), or NULL.
*/
void*
d_array_filter_result_release
(
    struct d_array_filter_result* _result,
    size_t*                       _out_count
)
{
    void* data;

    if (!_result)
    {
        if (_out_count)
        {
            *_out_count = 0;
        }

        return NULL;
    }

    data = _result->data;

    if (_out_count)
    {
        *_out_count = _result->count;
    }

    // null out the result so it won't double-free
    _result->data  = NULL;
    _result->count = 0;

    return data;
}


/*
d_array_filter_result_free
  Frees the data buffer and source_indices owned by the result.
  Safe to call on NULL or already-freed results.

Parameter(s):
  _result: pointer to the result (may be NULL).
Return:
  none.
*/
void
d_array_filter_result_free
(
    struct d_array_filter_result* _result
)
{
    if (!_result)
    {
        return;
    }

    if (_result->data)
    {
        free(_result->data);
        _result->data = NULL;
    }

    if (_result->source_indices)
    {
        free(_result->source_indices);
        _result->source_indices = NULL;
    }

    _result->count = 0;

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             VIII. FLUENT BUILDER HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

/*
d_array_filter_apply_builder
  Bridges the fluent builder API to array_filter semantics.
  Builds a chain from the builder, applies it, and returns an
  array_filter_result.

Parameter(s):
  _builder:      the fluent filter builder.
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
Return:
  d_array_filter_result for the composed operation.
*/
struct d_array_filter_result
d_array_filter_apply_builder
(
    struct d_filter_builder* _builder,
    const void*              _elements,
    size_t                   _count,
    size_t                   _element_size
)
{
    struct d_filter_chain*       chain;
    struct d_array_filter_result result = {0};

    if (!_builder)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_elements)
    {
        d_filter_builder_free(_builder);

        return d_array_filter_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    // build the chain, which consumes the builder
    chain = d_filter_builder_build(_builder);

    if (!chain)
    {
        return d_array_filter_result_error(D_FILTER_RESULT_ERROR,
                                 _element_size);
    }

    result = d_array_filter_apply_chain(_elements, _count,
                                        _element_size, chain);

    d_filter_chain_free(chain);

    return result;
}