/*******************************************************************************
* djinterp [c]                                                    array_filter.c
*
*   Implementation of zero-overhead filter operations for contiguous containers.
*   All functions operate on raw (void*, count, element_size) triples and
* delegate to the functional module's filter.h where appropriate.
*
*   Its definitions are compiled only when D_CFG_CONTAINER_ARRAY_FILTER is
* enabled: the same gate array_filter.h applies to their declarations.
*
*
* path:      /src/djinterp/c/container/array/array_filter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/array/array_filter.h"

// the header declares nothing unless the knob is on, so the definitions, and
// the headers only they need, sit behind the same gate
#if D_CFG_IS_ON(D_CFG_CONTAINER_ARRAY_FILTER)

// std
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // memmove
// djinterp
#include "../../../../../inc/djinterp/c/functional/filter.h"  // d_filter_*
#include "../../../../../inc/djinterp/c/memory/dmemory.h"     // d_memcpy
#include "../../../../../inc/djinterp/c/dckdint.h"            // d_ckd_add,
                                                              // d_ckd_mul


///////////////////////////////////////////////////////////////////////////////
///             INTERNAL HELPERS                                            ///
///////////////////////////////////////////////////////////////////////////////

// D_ARRAY_FILTER_ELEM
//   macro: returns a pointer to the element at index _i within a contiguous
// byte buffer.  Casts the base pointer to unsigned char* for byte arithmetic.
#define D_ARRAY_FILTER_ELEM(_base, _i, _elem_size) \
    ((const unsigned char*)(_base) + ((_i) * (_elem_size)))

// D_ARRAY_FILTER_ELEM_MUT
//   macro: mutable variant of D_ARRAY_FILTER_ELEM for in-place operations.
#define D_ARRAY_FILTER_ELEM_MUT(_base, _i, _elem_size) \
    ((unsigned char*)(_base) + ((_i) * (_elem_size)))


/*
d_cf_result_error
  Internal: constructs an error dj_contiguous_filter_result.

Parameter(s):
  _status:       error status code (must be < 0).
  _element_size: element size to record in the result.
Return:
  dj_contiguous_filter_result with NULL data, count 0, and the given status.
*/
static struct dj_contiguous_filter_result
d_cf_result_error
(
    enum dj_filter_result_type _status,
    size_t                    _element_size
)
{
    struct dj_contiguous_filter_result res = {0};

    res.data           = NULL;
    res.count          = 0;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = _status;

    return res;
}


/*
d_cf_result_empty
  Internal: constructs an empty (non-error) dj_contiguous_filter_result.

Parameter(s):
  _element_size: element size to record in the result.
Return:
  dj_contiguous_filter_result with NULL data, count 0, and EMPTY status.
*/
static
struct dj_contiguous_filter_result d_cf_result_empty(
    size_t _element_size
)
{
    struct dj_contiguous_filter_result res = {0};

    res.data           = NULL;
    res.count          = 0;
    res.element_size   = _element_size;
    res.source_indices = NULL;
    res.status         = D_FILTER_RESULT_EMPTY;

    return res;
}


/*
d_cf_copy_range
  Internal: allocates a new buffer and copies a contiguous range of elements
  from source [_start, _start + _out_count) into it.

Parameter(s):
  _elements:     source buffer.
  _element_size: size of each element.
  _start:        index of first element to copy.
  _out_count:    number of elements to copy.
Return:
  dj_contiguous_filter_result owning the new buffer, or a NO_MEMORY error.
*/
static
struct dj_contiguous_filter_result d_cf_copy_range(
    const void* _elements,
    size_t      _element_size,
    size_t      _start,
    size_t      _out_count
)
{
    struct dj_contiguous_filter_result res = {0};

    res.source_indices = NULL;
    res.element_size   = _element_size;

    if (_out_count == 0)
    {
        res.data   = NULL;
        res.count  = 0;
        res.status = D_FILTER_RESULT_EMPTY;

        return res;
    }

    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, _out_count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        res.count  = 0;
        res.status = D_FILTER_RESULT_NO_MEMORY;

        return res;
    }

    d_memcpy(res.data,
             D_ARRAY_FILTER_ELEM(_elements, _start, _element_size),
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
d_contiguous_filter_take_first
  Returns a new result containing the first _n elements.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to take.
Return:
  dj_contiguous_filter_result owning a copy of the first min(_n, _count) elements.
*/
struct dj_contiguous_filter_result d_contiguous_filter_take_first(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    size_t actual;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n == 0) )
    {
        return d_cf_result_empty(_element_size);
    }

    actual = (_n < _count) ? _n : _count;

    return d_cf_copy_range(_elements, _element_size, 0, actual);
}


/*
d_contiguous_filter_take_last
  Returns a new result containing the last _n elements.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to take from the end.
Return:
  dj_contiguous_filter_result owning a copy of the last min(_n, _count) elements.
*/
struct dj_contiguous_filter_result d_contiguous_filter_take_last(
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
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n == 0) )
    {
        return d_cf_result_empty(_element_size);
    }

    actual = (_n < _count) ? _n : _count;
    start  = _count - actual;

    return d_cf_copy_range(_elements, _element_size, start, actual);
}


/*
d_contiguous_filter_take_nth
  Returns every _n-th element (indices 0, _n, 2*_n, ...).

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            step interval (must be > 0).
Return:
  dj_contiguous_filter_result owning a copy of every _n-th element.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_take_nth(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       out_count;
    size_t                       src;
    size_t                       dst;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_n == 0)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // calculate output count: ceil(_count / _n)
    out_count = (_count + _n - 1) / _n;

    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, out_count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (src = 0; src < _count; src += _n)
    {
        d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, dst, _element_size),
                 D_ARRAY_FILTER_ELEM(_elements, src, _element_size),
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
d_contiguous_filter_head
  Returns a result containing only the first element.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  dj_contiguous_filter_result owning a copy of the first element, or empty if
  the source is empty.
*/
struct dj_contiguous_filter_result d_contiguous_filter_head(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_contiguous_filter_take_first(_elements, _count, _element_size, 1);
}


/*
d_contiguous_filter_tail
  Returns a result containing only the last element.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  dj_contiguous_filter_result owning a copy of the last element, or empty if
  the source is empty.
*/
struct dj_contiguous_filter_result d_contiguous_filter_tail(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_contiguous_filter_take_last(_elements, _count, _element_size, 1);
}


// ---------------------------------------------------------------------------
// ii.   skip operations
// ---------------------------------------------------------------------------

/*
d_contiguous_filter_skip_first
  Returns all elements except the first _n.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to skip from the front.
Return:
  dj_contiguous_filter_result owning a copy of elements [_n .. _count).
*/
struct dj_contiguous_filter_result d_contiguous_filter_skip_first(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n >= _count) )
    {
        return d_cf_result_empty(_element_size);
    }

    return d_cf_copy_range(_elements, _element_size, _n, _count - _n);
}


/*
d_contiguous_filter_skip_last
  Returns all elements except the last _n.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to skip from the end.
Return:
  dj_contiguous_filter_result owning a copy of elements [0 .. _count - _n).
*/
struct dj_contiguous_filter_result d_contiguous_filter_skip_last(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _n
)
{
    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if ( (_count == 0) ||
         (_n >= _count) )
    {
        return d_cf_result_empty(_element_size);
    }

    return d_cf_copy_range(_elements, _element_size, 0, _count - _n);
}


/*
d_contiguous_filter_init
  Returns all elements except the last (Haskell-style init).

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  dj_contiguous_filter_result owning elements [0 .. _count - 1).
*/
struct dj_contiguous_filter_result d_contiguous_filter_init(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_contiguous_filter_skip_last(_elements, _count, _element_size, 1);
}


/*
d_contiguous_filter_rest
  Returns all elements except the first (Haskell-style tail / rest).

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  dj_contiguous_filter_result owning elements [1 .. _count).
*/
struct dj_contiguous_filter_result d_contiguous_filter_rest(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    return d_contiguous_filter_skip_first(_elements, _count, _element_size, 1);
}


// ---------------------------------------------------------------------------
// iii.  range and slice operations
// ---------------------------------------------------------------------------

/*
d_contiguous_filter_range
  Returns elements in the half-open range [_start, _end).
  Clamps _end to _count.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _start:        start index (inclusive).
  _end:          end index (exclusive).
Return:
  dj_contiguous_filter_result owning a copy of the selected range.
*/
struct dj_contiguous_filter_result d_contiguous_filter_range(
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
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // clamp end to count
    clamped_end = (_end < _count) ? _end : _count;

    // inverted or empty range
    if (_start >= clamped_end)
    {
        return d_cf_result_empty(_element_size);
    }

    return d_cf_copy_range(_elements, _element_size,
                           _start, clamped_end - _start);
}


/*
d_contiguous_filter_slice
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
  dj_contiguous_filter_result owning a copy of the selected elements.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_slice(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    size_t      _start,
    size_t      _end,
    size_t      _step
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       clamped_end;
    size_t                       range_len;
    size_t                       out_count;
    size_t                       dst;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_step == 0)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    clamped_end = (_end < _count) ? _end : _count;

    if (_start >= clamped_end)
    {
        return d_cf_result_empty(_element_size);
    }

    // step == 1 is a contiguous copy
    if (_step == 1)
    {
        return d_cf_copy_range(_elements, _element_size,
                               _start, clamped_end - _start);
    }

    //   the elements at _start, _start + _step, ... before clamped_end:
    // counted and walked by their number, so that a step near SIZE_MAX
    // can wrap neither the count nor the index
    range_len = clamped_end - _start;
    out_count = ((range_len - 1) / _step) + 1;

    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, out_count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    for (dst = 0; dst < out_count; ++dst)
    {
        d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, dst, _element_size),
                 D_ARRAY_FILTER_ELEM(_elements,
                                     _start + (dst * _step),
                                     _element_size),
                 _element_size);
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
d_contiguous_filter_where
  Returns all elements for which _test returns true.
  Performs a two-pass approach: first counts matching elements, then copies.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function.
  _context:      opaque context passed to the predicate.
Return:
  dj_contiguous_filter_result owning a copy of all matching elements.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_where(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       match_count;
    size_t                       i;
    size_t                       dst;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_test)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // pass 1: count matches
    match_count = 0;

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            ++match_count;
        }
    }

    if (match_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // pass 2: copy matching elements
    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, match_count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (i = 0; i < _count; ++i)
    {
        if (_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, dst, _element_size),
                     D_ARRAY_FILTER_ELEM(_elements, i, _element_size),
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
d_contiguous_filter_where_not
  Returns all elements for which _test returns false.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function; elements that FAIL are kept.
  _context:      opaque context passed to the predicate.
Return:
  dj_contiguous_filter_result owning a copy of all non-matching elements.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_where_not(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       match_count;
    size_t                       i;
    size_t                       dst;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_test)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // pass 1: count non-matches
    match_count = 0;

    for (i = 0; i < _count; ++i)
    {
        if (!_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            ++match_count;
        }
    }

    if (match_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // pass 2: copy
    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, match_count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    dst = 0;

    for (i = 0; i < _count; ++i)
    {
        if (!_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, dst, _element_size),
                     D_ARRAY_FILTER_ELEM(_elements, i, _element_size),
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
d_contiguous_filter_at_indices
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
  dj_contiguous_filter_result owning copies of the selected elements.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_at_indices(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const size_t* _indices,
    size_t        _index_count
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       valid_count;
    size_t                       i;
    size_t                       dst;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_indices)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_index_count == 0)
    {
        return d_cf_result_empty(_element_size);
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
        return d_cf_result_empty(_element_size);
    }

    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, valid_count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    // pass 2: copy
    dst = 0;

    for (i = 0; i < _index_count; ++i)
    {
        if (_indices[i] < _count)
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, dst, _element_size),
                     D_ARRAY_FILTER_ELEM(_elements, _indices[i], _element_size),
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
d_contiguous_filter_distinct
  Returns a copy of the data with duplicate elements removed.
  Preserves the order of first occurrence.  Uses an O(n^2) scan suitable
  for moderate data sizes; a sort-based approach can be substituted for
  large datasets.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _comparator:   three-way comparator for element equality (0 == equal).
Return:
  dj_contiguous_filter_result owning a deduplicated copy.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_distinct(
    const void*            _elements,
    size_t                 _count,
    size_t                 _element_size,
    fn_function_comparator _comparator
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       unique_count;
    size_t                       i;
    size_t                       j;
    bool                         is_dup;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_comparator)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // worst case: all unique -> allocate for _count elements
    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, _count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
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
            if (_comparator(D_ARRAY_FILTER_ELEM(_elements, i, _element_size),
                            D_ARRAY_FILTER_ELEM(res.data, j, _element_size),
                            NULL) == 0)
            {
                is_dup = true;

                break;
            }
        }

        if (!is_dup)
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, unique_count, _element_size),
                     D_ARRAY_FILTER_ELEM(_elements, i, _element_size),
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
d_contiguous_filter_reverse
  Returns a copy of the data with elements in reverse order.

Parameter(s):
  _elements:     source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
Return:
  dj_contiguous_filter_result owning a reversed copy.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_reverse(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    struct dj_contiguous_filter_result res = {0};
    size_t                       i;

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    res.data = (d_ckd_mul(&bytes, _count, _element_size))
                   ? NULL
                   : malloc(bytes);

    if (!res.data)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    for (i = 0; i < _count; ++i)
    {
        d_memcpy(D_ARRAY_FILTER_ELEM_MUT(res.data, i, _element_size),
                 D_ARRAY_FILTER_ELEM(_elements, _count - 1 - i, _element_size),
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
d_contiguous_filter_in_place
  Filters the data in-place, keeping only elements where _test returns true.
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
size_t d_contiguous_filter_in_place(
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
        if (_test(D_ARRAY_FILTER_ELEM(_elements, read, _element_size), _context))
        {
            if (write != read)
            {
                memmove(D_ARRAY_FILTER_ELEM_MUT(_elements, write, _element_size),
                          D_ARRAY_FILTER_ELEM(_elements, read, _element_size),
                          _element_size);
            }

            ++write;
        }
    }

    return write;
}


/*
d_contiguous_filter_in_place_not
  Filters the data in-place, keeping only elements where _test returns false.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _test:         predicate function; elements that FAIL are kept.
  _context:      opaque context passed to the predicate.
Return:
  New count of surviving elements.
*/
size_t d_contiguous_filter_in_place_not(
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
        if (!_test(D_ARRAY_FILTER_ELEM(_elements, read, _element_size), _context))
        {
            if (write != read)
            {
                memmove(D_ARRAY_FILTER_ELEM_MUT(_elements, write, _element_size),
                          D_ARRAY_FILTER_ELEM(_elements, read, _element_size),
                          _element_size);
            }

            ++write;
        }
    }

    return write;
}


/*
d_contiguous_filter_in_place_take_first
  Truncates the data in-place to the first _n elements.
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
size_t d_contiguous_filter_in_place_take_first(
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
d_contiguous_filter_in_place_skip_first
  Removes the first _n elements by shifting the remainder to the front.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _n:            number of elements to skip.
Return:
  New count after shifting (_count - _n, or 0 if _n >= _count).
*/
size_t d_contiguous_filter_in_place_skip_first(
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
              D_ARRAY_FILTER_ELEM(_elements, _n, _element_size),
              remaining * _element_size);

    return remaining;
}


/*
d_contiguous_filter_in_place_distinct
  Removes duplicate elements in-place.  Preserves order of first occurrence.

Parameter(s):
  _elements:     mutable source array.
  _count:        number of elements in source.
  _element_size: size of each element in bytes.
  _comparator:   three-way comparator (0 == equal).
Return:
  New count of unique elements.
*/
size_t d_contiguous_filter_in_place_distinct(
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
            if (_comparator(D_ARRAY_FILTER_ELEM(_elements, i, _element_size),
                            D_ARRAY_FILTER_ELEM(_elements, j, _element_size),
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
                memmove(D_ARRAY_FILTER_ELEM_MUT(_elements, unique, _element_size),
                          D_ARRAY_FILTER_ELEM(_elements, i, _element_size),
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
d_cf_apply_single_op
  Internal: applies a single dj_filter_operation to contiguous data, producing
  a new dj_contiguous_filter_result.  Used as the inner step of chain execution.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _op:           filter operation to apply.
Return:
  dj_contiguous_filter_result for the single operation.
*/
static
struct dj_contiguous_filter_result d_cf_apply_single_op(
    const void*                       _elements,
    size_t                            _count,
    size_t                            _element_size,
    const struct dj_filter_operation* _op
)
{
    if (!_op)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    switch (_op->type)
    {
        case D_FILTER_OP_TAKE_FIRST:
            return d_contiguous_filter_take_first(_elements, _count,
                                            _element_size,
                                            _op->params.count);

        case D_FILTER_OP_TAKE_LAST:
            return d_contiguous_filter_take_last(_elements, _count,
                                           _element_size,
                                           _op->params.count);

        case D_FILTER_OP_TAKE_NTH:
            return d_contiguous_filter_take_nth(_elements, _count,
                                          _element_size,
                                          _op->params.step);

        case D_FILTER_OP_HEAD:
            return d_contiguous_filter_head(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_TAIL:
            return d_contiguous_filter_tail(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_SKIP_FIRST:
            return d_contiguous_filter_skip_first(_elements, _count,
                                            _element_size,
                                            _op->params.count);

        case D_FILTER_OP_SKIP_LAST:
            return d_contiguous_filter_skip_last(_elements, _count,
                                           _element_size,
                                           _op->params.count);

        case D_FILTER_OP_INIT:
            return d_contiguous_filter_init(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_REST:
            return d_contiguous_filter_rest(_elements, _count,
                                      _element_size);

        case D_FILTER_OP_RANGE:
            return d_contiguous_filter_range(_elements, _count,
                                       _element_size,
                                       _op->params.start,
                                       _op->params.end);

        case D_FILTER_OP_SLICE:
            return d_contiguous_filter_slice(_elements, _count,
                                       _element_size,
                                       _op->params.start,
                                       _op->params.end,
                                       _op->params.step);

        case D_FILTER_OP_WHERE:
            return d_contiguous_filter_where(_elements, _count,
                                       _element_size,
                                       _op->params.test,
                                       _op->params.context);

        case D_FILTER_OP_WHERE_NOT:
            return d_contiguous_filter_where_not(_elements, _count,
                                           _element_size,
                                           _op->params.test,
                                           _op->params.context);

        case D_FILTER_OP_INDICES:
            return d_contiguous_filter_at_indices(_elements, _count,
                                            _element_size,
                                            _op->params.indices,
                                            _op->params.indices_count);

        case D_FILTER_OP_DISTINCT:
            return d_contiguous_filter_distinct(_elements, _count,
                                          _element_size,
                                          _op->params.comparator);

        case D_FILTER_OP_REVERSE:
            return d_contiguous_filter_reverse(_elements, _count,
                                         _element_size);

        case D_FILTER_OP_NONE:
        default:
            // identity: copy everything
            return d_cf_copy_range(_elements, _element_size, 0, _count);
    }
}


/*
d_contiguous_filter_apply_chain
  Sequentially applies each operation in a filter chain, piping the output
  of one step as the input of the next.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _chain:        chain of operations to apply.
Return:
  dj_contiguous_filter_result for the final output.
*/
struct dj_contiguous_filter_result d_contiguous_filter_apply_chain(
    const void*                   _elements,
    size_t                        _count,
    size_t                        _element_size,
    const struct dj_filter_chain* _chain
)
{
    struct dj_contiguous_filter_result current = {0};
    struct dj_contiguous_filter_result next = {0};
    const void*                  input_data;
    size_t                       input_count;
    size_t                       i;
    bool                         owns_current;

    if (!_chain)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_elements)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    // empty chain -> copy everything
    if (_chain->count == 0)
    {
        return d_cf_copy_range(_elements, _element_size, 0, _count);
    }

    // first operation: input is the original array (not owned by us)
    input_data   = _elements;
    input_count  = _count;
    owns_current = false;

    for (i = 0; i < _chain->count; ++i)
    {
        next = d_cf_apply_single_op(input_data,
                                    input_count,
                                    _element_size,
                                    &_chain->operations[i]);

        // free the intermediate buffer (but not the original input)
        if (owns_current)
        {
            d_contiguous_filter_result_free(&current);
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


/**
 * @brief Sums the chains' counts and their size in bytes, refusing a sum or
 *        a size that wraps.
 *
 * @param[in]  _results       the chains' results.
 * @param[in]  _count         the number of results.
 * @param[in]  _element_size  the size of each element, in bytes.
 * @param[out] _total         receives the elements in all the results.
 * @param[out] _bytes         receives their size, in bytes.
 * @return true when the total or its size does not fit in a size_t.
 *
 * Each chain's count is at most the source's, so it fits; their sum, over
 * many chains, may not, and neither may its size.
 */
static bool
d_internal_cf_union_bytes(
    const struct dj_contiguous_filter_result* _results,
    size_t                                    _count,
    size_t                                    _element_size,
    size_t*                                   _total,
    size_t*                                   _bytes
)
{
    size_t i;

    *_total = 0;
    *_bytes = 0;

    // a sum that wraps cannot be allocated below
    for (i = 0; i < _count; ++i)
    {
        if (d_ckd_add(_total, *_total, _results[i].count))
        {
            return true;
        }
    }

    return d_ckd_mul(_bytes, *_total, _element_size);
}

/*
d_cf_results_ok
  Whether each of _n chain results succeeded or came out empty, so that a
set operation over them answers rather than leaves a chain out.

Parameter(s):
  _results: the chain results.
  _n:       their number.
Return:
  true if every result can be used, false if one failed.
*/
static bool
d_cf_results_ok(
    const struct dj_contiguous_filter_result* _results,
    size_t                                    _n
)
{
    size_t i;

    for (i = 0; i < _n; ++i)
    {
        if ( (_results[i].status != D_FILTER_RESULT_SUCCESS) &&
             (_results[i].status != D_FILTER_RESULT_EMPTY) )
        {
            return false;
        }
    }

    return true;
}

/*
d_cf_results_release
  Frees each of _n chain results, then the array that holds them.

Parameter(s):
  _results: the chain results, in an array from calloc.
  _n:       their number.
Return:
  none.
*/
static void
d_cf_results_release(
    struct dj_contiguous_filter_result* _results,
    size_t                              _n
)
{
    size_t i;

    for (i = 0; i < _n; ++i)
    {
        d_contiguous_filter_result_free(&_results[i]);
    }

    free(_results);

    return;
}

/*
d_cf_contains
  Whether _result holds an element equal to _wanted, by _comparator.

Parameter(s):
  _result:       the chain result searched.
  _wanted:       the element looked for.
  _element_size: the size in bytes of each element.
  _comparator:   the equality test, 0 for equal.
Return:
  true if an equal element is found, false otherwise.
*/
static bool
d_cf_contains(
    const struct dj_contiguous_filter_result* _result,
    const void*                               _wanted,
    size_t                                    _element_size,
    fn_function_comparator                    _comparator
)
{
    size_t k;

    for (k = 0; k < _result->count; ++k)
    {
        if (_comparator(_wanted,
                        D_ARRAY_FILTER_ELEM(_result->data, k, _element_size),
                        NULL) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_cf_in_all
  Whether element _j of the first chain result has an equal, by
_comparator, in every other.

Parameter(s):
  _results:      the chain results; the first is the one searched from.
  _n:            their number.
  _j:            the element of the first result.
  _element_size: the size in bytes of each element.
  _comparator:   the equality test, 0 for equal.
Return:
  true if every other result holds an equal element, false otherwise.
*/
static bool
d_cf_in_all(
    const struct dj_contiguous_filter_result* _results,
    size_t                                    _n,
    size_t                                    _j,
    size_t                                    _element_size,
    fn_function_comparator                    _comparator
)
{
    const void* wanted;
    size_t      i;

    wanted = D_ARRAY_FILTER_ELEM(_results[0].data, _j, _element_size);

    for (i = 1; i < _n; ++i)
    {
        if (!d_cf_contains(&_results[i], wanted, _element_size, _comparator))
        {
            return false;
        }
    }

    return true;
}

/*
d_contiguous_filter_apply_union
  Applies multiple filter chains and produces their union (elements that
  appear in any chain's output).  Deduplication uses the provided comparator.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _combo:        union combinator containing multiple chains.
  _comparator:   comparator for deduplication.
Return:
  dj_contiguous_filter_result owning the union of all chain outputs.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_apply_union(
    const void*                   _elements,
    size_t                        _count,
    size_t                        _element_size,
    const struct dj_filter_union* _combo,
    fn_function_comparator        _comparator
)
{
    struct dj_contiguous_filter_result* chain_results = NULL;
    struct dj_contiguous_filter_result  merged = {0};
    size_t                        total;
    size_t                        pos;
    size_t                        i;

    if ( (!_combo) ||
         (!_elements) )
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_combo->count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // apply each chain independently
    chain_results = (struct dj_contiguous_filter_result*)
                    calloc(_combo->count,
                             sizeof(struct dj_contiguous_filter_result));

    if (!chain_results)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    for (i = 0; i < _combo->count; ++i)
    {
        chain_results[i] = d_contiguous_filter_apply_chain(_elements,
                                                      _count,
                                                      _element_size,
                                                      _combo->filters[i]);
    }

    // a chain that failed leaves the union unknown: the failure is the
    // result, as in d_filter_apply_union
    if (!d_cf_results_ok(chain_results, _combo->count))
    {
        d_cf_results_release(chain_results, _combo->count);

        return d_cf_result_error(D_FILTER_RESULT_ERROR, _element_size);
    }

    size_t     bytes = 0;
    const bool wraps = d_internal_cf_union_bytes(chain_results,
                                                 _combo->count,
                                                 _element_size,
                                                 &total,
                                                 &bytes);

    // merge all results into one buffer
    merged.data = (wraps) ? NULL : malloc(bytes);

    if ( (!merged.data) &&
         ( (total > 0) ||
           (wraps) ) )
    {
        d_cf_results_release(chain_results, _combo->count);

        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    pos = 0;

    for (i = 0; i < _combo->count; ++i)
    {
        if (chain_results[i].data && chain_results[i].count > 0)
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(merged.data, pos, _element_size),
                     chain_results[i].data,
                     chain_results[i].count * _element_size);
            pos += chain_results[i].count;
        }

        d_contiguous_filter_result_free(&chain_results[i]);
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
        merged.count = d_contiguous_filter_in_place_distinct(merged.data,
                                                        merged.count,
                                                        _element_size,
                                                        _comparator);
    }

    return merged;
}


/*
d_contiguous_filter_apply_intersection
  Applies multiple filter chains and produces their intersection (elements
  that appear in every chain's output).

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _combo:        intersection combinator.
  _comparator:   comparator for element equality.
Return:
  dj_contiguous_filter_result owning the intersection.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_apply_intersection(
    const void*                          _elements,
    size_t                               _count,
    size_t                               _element_size,
    const struct dj_filter_intersection* _combo,
    fn_function_comparator               _comparator
)
{
    struct dj_contiguous_filter_result* chain_results = NULL;
    struct dj_contiguous_filter_result  result = {0};
    size_t                              bytes;
    size_t                              i;
    size_t                              j;
    size_t                              out_count;

    if ( (!_combo)     ||
         (!_elements)  ||
         (!_comparator) )
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (_combo->count == 0)
    {
        return d_cf_result_empty(_element_size);
    }

    // apply each chain
    chain_results = (struct dj_contiguous_filter_result*)
                    calloc(_combo->count,
                           sizeof(struct dj_contiguous_filter_result));

    if (!chain_results)
    {
        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    for (i = 0; i < _combo->count; ++i)
    {
        chain_results[i] = d_contiguous_filter_apply_chain(_elements,
                                                      _count,
                                                      _element_size,
                                                      _combo->filters[i]);
    }

    // a chain that failed leaves the intersection unknown: the failure is
    // the result, as in d_filter_apply_intersection
    if (!d_cf_results_ok(chain_results, _combo->count))
    {
        d_cf_results_release(chain_results, _combo->count);

        return d_cf_result_error(D_FILTER_RESULT_ERROR, _element_size);
    }

    // intersect: iterate over first chain's results, keep elements
    // present in all other chains
    bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    result.data = (d_ckd_mul(&bytes,
                             chain_results[0].count,
                             _element_size))
                      ? NULL
                      : malloc(bytes);

    if ( (!result.data) &&
         (chain_results[0].count > 0) )
    {
        d_cf_results_release(chain_results, _combo->count);

        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    out_count = 0;

    for (j = 0; j < chain_results[0].count; ++j)
    {
        if (d_cf_in_all(chain_results, _combo->count, j,
                        _element_size, _comparator))
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(result.data,
                                             out_count,
                                             _element_size),
                     D_ARRAY_FILTER_ELEM(chain_results[0].data,
                                         j,
                                         _element_size),
                     _element_size);
            ++out_count;
        }
    }

    d_cf_results_release(chain_results, _combo->count);

    result.count          = out_count;
    result.element_size   = _element_size;
    result.source_indices = NULL;
    result.status         = (out_count > 0) ? D_FILTER_RESULT_SUCCESS
                                            : D_FILTER_RESULT_EMPTY;

    return result;
}


/*
d_contiguous_filter_apply_difference
  Applies two filter chains (A and B) and returns A - B (elements in A's
  output that do not appear in B's output).

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _diff:         difference combinator (contains include and exclude chains).
  _comparator:   comparator for element equality.
Return:
  dj_contiguous_filter_result owning A minus B.
  On failure, an INVALID error for bad arguments, or a NO_MEMORY error if
  the bytes needed do not fit in a size_t or allocation fails.
*/
struct dj_contiguous_filter_result d_contiguous_filter_apply_difference(
    const void*                        _elements,
    size_t                             _count,
    size_t                             _element_size,
    const struct dj_filter_difference* _diff,
    fn_function_comparator             _comparator
)
{
    struct dj_contiguous_filter_result result_a = {0};
    struct dj_contiguous_filter_result result_b = {0};
    struct dj_contiguous_filter_result result = {0};
    size_t                       out_count;
    size_t                       i;

    if ( (!_diff)      ||
         (!_elements)  ||
         (!_comparator) )
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    result_a = d_contiguous_filter_apply_chain(_elements, _count,
                                          _element_size,
                                          _diff->include);
    result_b = d_contiguous_filter_apply_chain(_elements, _count,
                                          _element_size,
                                          _diff->exclude);

    // a chain that failed leaves A minus B unknown: the failure is the
    // result, as in d_filter_apply_difference
    if ( (!d_cf_results_ok(&result_a, 1)) ||
         (!d_cf_results_ok(&result_b, 1)) )
    {
        d_contiguous_filter_result_free(&result_a);
        d_contiguous_filter_result_free(&result_b);

        return d_cf_result_error(D_FILTER_RESULT_ERROR, _element_size);
    }

    // if A is empty, result is empty
    if (result_a.count == 0)
    {
        d_contiguous_filter_result_free(&result_b);

        return result_a;
    }

    // if B is empty, result is all of A
    if (result_b.count == 0)
    {
        d_contiguous_filter_result_free(&result_b);

        return result_a;
    }

    size_t bytes = 0;

    // a size that wraps cannot be allocated: it fails as an allocation does
    result.data = (d_ckd_mul(&bytes, result_a.count, _element_size))
                      ? NULL
                      : malloc(bytes);

    if (!result.data)
    {
        d_contiguous_filter_result_free(&result_a);
        d_contiguous_filter_result_free(&result_b);

        return d_cf_result_error(D_FILTER_RESULT_NO_MEMORY,
                                 _element_size);
    }

    out_count = 0;

    for (i = 0; i < result_a.count; ++i)
    {
        const void* element = D_ARRAY_FILTER_ELEM(result_a.data,
                                                  i,
                                                  _element_size);

        if (!d_cf_contains(&result_b, element, _element_size, _comparator))
        {
            d_memcpy(D_ARRAY_FILTER_ELEM_MUT(result.data,
                                             out_count,
                                             _element_size),
                     element,
                     _element_size);
            ++out_count;
        }
    }

    d_contiguous_filter_result_free(&result_a);
    d_contiguous_filter_result_free(&result_b);

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
d_contiguous_filter_count_where
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
size_t d_contiguous_filter_count_where(
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
        if (_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            ++matches;
        }
    }

    return matches;
}


/*
d_contiguous_filter_any_match
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
bool d_contiguous_filter_any_match(
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
        if (_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            return true;
        }
    }

    return false;
}


/*
d_contiguous_filter_all_match
  Returns true if every element satisfies the predicate (vacuously true
  for empty inputs).  Short-circuits on first non-match.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  true if all elements match, false otherwise.
*/
bool d_contiguous_filter_all_match(
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
        if (!_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            return false;
        }
    }

    return true;
}


/*
d_contiguous_filter_none_match
  Returns true if no element satisfies the predicate (vacuously true
  for empty inputs).  Short-circuits on first match.

Parameter(s):
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
  _test:         predicate function.
  _context:      opaque context.
Return:
  true if no elements match, false otherwise.
*/
bool d_contiguous_filter_none_match(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _test,
    void*        _context
)
{
    return !d_contiguous_filter_any_match(_elements, _count,
                                     _element_size, _test, _context);
}


/*
d_contiguous_filter_find_first
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
void* d_contiguous_filter_find_first(
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
        if (_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            // cast away const: caller received const void* but
            // the returned pointer is to the original mutable source
            return (void*)D_ARRAY_FILTER_ELEM(_elements, i, _element_size);
        }
    }

    return NULL;
}


/*
d_contiguous_filter_find_last
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
void* d_contiguous_filter_find_last(
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

        if (_test(D_ARRAY_FILTER_ELEM(_elements, i, _element_size), _context))
        {
            return (void*)D_ARRAY_FILTER_ELEM(_elements, i, _element_size);
        }
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   RESULT MANAGEMENT                                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_contiguous_filter_result_data
  Returns the data pointer from a filter result.

Parameter(s):
  _result: pointer to the result.
Return:
  The internal data pointer, or NULL if _result is NULL or has no data.
*/
void* d_contiguous_filter_result_data(
    const struct dj_contiguous_filter_result* _result
)
{
    if (!_result)
    {
        return NULL;
    }

    return _result->data;
}


/*
d_contiguous_filter_result_count
  Returns the element count from a filter result.

Parameter(s):
  _result: pointer to the result.
Return:
  Number of elements, or 0 if _result is NULL.
*/
size_t d_contiguous_filter_result_count(
    const struct dj_contiguous_filter_result* _result
)
{
    if (!_result)
    {
        return 0;
    }

    return _result->count;
}


/*
d_contiguous_filter_result_ok
  Returns true if the result status is non-negative (SUCCESS or EMPTY).

Parameter(s):
  _result: pointer to the result.
Return:
  true if OK, false on error or NULL.
*/
bool d_contiguous_filter_result_ok(
    const struct dj_contiguous_filter_result* _result
)
{
    if (!_result)
    {
        return false;
    }

    return _result->status >= 0;
}


/*
d_contiguous_filter_result_release
  Transfers ownership of the data buffer to the caller.
  After this call, the result's data pointer is set to NULL.

Parameter(s):
  _result:    pointer to the result.
  _out_count: (optional) receives the element count.
Return:
  The data pointer (caller must free), or NULL.
*/
void* d_contiguous_filter_result_release(
    struct dj_contiguous_filter_result* _result,
    size_t*                             _out_count
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
d_contiguous_filter_result_free
  Frees the data buffer and source_indices owned by the result.
  Safe to call on NULL or already-freed results.

Parameter(s):
  _result: pointer to the result (may be NULL).
Return:
  none.
*/
void d_contiguous_filter_result_free(
    struct dj_contiguous_filter_result* _result
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
d_contiguous_filter_apply_builder
  Bridges the fluent builder API to contiguous_filter semantics.
  Builds a chain from the builder, applies it, and returns an
  contiguous_filter_result. The builder is consumed: it is freed on every
  path but a NULL builder.

Parameter(s):
  _builder:      the fluent filter builder.
  _elements:     source array.
  _count:        number of elements.
  _element_size: element size in bytes.
Return:
  dj_contiguous_filter_result for the composed operation.
*/
struct dj_contiguous_filter_result d_contiguous_filter_apply_builder(
    struct dj_filter_builder* _builder,
    const void*               _elements,
    size_t                    _count,
    size_t                    _element_size
)
{
    struct dj_filter_chain*       chain;
    struct dj_contiguous_filter_result result = {0};

    if (!_builder)
    {
        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    if (!_elements)
    {
        d_filter_builder_free(_builder);

        return d_cf_result_error(D_FILTER_RESULT_INVALID,
                                 _element_size);
    }

    //   take the chain out, then free the builder: d_filter_builder_build
    // detaches the chain but leaves the builder itself to its owner, and this
    // function owns it, as the NULL-elements path above shows
    chain = d_filter_builder_build(_builder);
    d_filter_builder_free(_builder);

    if (!chain)
    {
        return d_cf_result_error(D_FILTER_RESULT_ERROR,
                                 _element_size);
    }

    result = d_contiguous_filter_apply_chain(_elements, _count,
                                        _element_size, chain);

    d_filter_chain_free(chain);

    return result;
}

#endif  // D_CFG_CONTAINER_ARRAY_FILTER
