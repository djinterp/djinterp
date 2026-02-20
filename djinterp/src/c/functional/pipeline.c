#include "..\..\inc\functional\pipeline.h"


/*
d_functional_pipeline_begin
  Creates a pipeline wrapping existing mutable data. The pipeline does NOT
take ownership of the data; the caller remains responsible for freeing it.

Parameter(s):
  _data:         pointer to the mutable data array.
  _count:        number of elements in the array.
  _element_size: size of each element in bytes.
Return:
  A d_functional_pipeline struct wrapping the data. If any parameter is
invalid (NULL data, zero count, or zero element_size), returns a pipeline
with error_code set to -1.
*/
struct d_functional_pipeline
d_functional_pipeline_begin
(
    void*  _data,
    size_t _count,
    size_t _element_size
)
{
    struct d_functional_pipeline pipe;

    // validate parameters
    if ( (!_data)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        pipe.data         = NULL;
        pipe.element_size = 0;
        pipe.count        = 0;
        pipe.owns_data    = false;
        pipe.error_code   = -1;

        return pipe;
    }

    pipe.data         = _data;
    pipe.element_size = _element_size;
    pipe.count        = _count;
    pipe.owns_data    = false;
    pipe.error_code   = 0;

    return pipe;
}

/*
d_functional_pipeline_begin_copy
  Creates a pipeline by copying immutable data into a newly allocated
buffer. The pipeline takes ownership of the copy.

Parameter(s):
  _data:         pointer to the immutable source data array.
  _count:        number of elements in the array.
  _element_size: size of each element in bytes.
Return:
  A d_functional_pipeline struct containing a copy of the data. If any
parameter is invalid or allocation fails, returns a pipeline with
error_code set to -1.
*/
struct d_functional_pipeline
d_functional_pipeline_begin_copy
(
    const void* _data,
    size_t      _count,
    size_t      _element_size
)
{
    struct d_functional_pipeline pipe;
    void*                        copy;

    // validate parameters
    if ( (!_data)             ||
         (_count == 0)        ||
         (_element_size == 0) )
    {
        pipe.data         = NULL;
        pipe.element_size = 0;
        pipe.count        = 0;
        pipe.owns_data    = false;
        pipe.error_code   = -1;

        return pipe;
    }

    copy = malloc(_count * _element_size);

    // check allocation
    if (!copy)
    {
        pipe.data         = NULL;
        pipe.element_size = _element_size;
        pipe.count        = 0;
        pipe.owns_data    = false;
        pipe.error_code   = -1;

        return pipe;
    }

    memcpy(copy, _data, _count * _element_size);

    pipe.data         = copy;
    pipe.element_size = _element_size;
    pipe.count        = _count;
    pipe.owns_data    = true;
    pipe.error_code   = 0;

    return pipe;
}

/*
d_functional_pipeline_map
  Applies a transformer to each element in the pipeline, producing a new
data buffer with the results. The old buffer is freed if the pipeline
owned it.

Parameter(s):
  _pipe:      the current pipeline state.
  _transform: transformer function to apply to each element.
  _context:   context forwarded to _transform; may be NULL.
Return:
  A new pipeline containing the transformed data. If the pipeline is in
an error state, _transform is NULL, or allocation fails, returns a
pipeline with the appropriate error_code.
*/
struct d_functional_pipeline
d_functional_pipeline_map
(
    struct d_functional_pipeline _pipe,
    fn_transformer                _transform,
    void*                        _context
)
{
    struct d_functional_pipeline result;
    void*                        new_data;
    const unsigned char*         src;
    unsigned char*               dst;
    size_t                       i;

    // propagate prior errors
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    // validate transformer
    if (!_transform)
    {
        _pipe.error_code = -1;

        return _pipe;
    }

    new_data = malloc(_pipe.count * _pipe.element_size);

    // check allocation
    if (!new_data)
    {
        _pipe.error_code = -1;

        return _pipe;
    }

    src = (const unsigned char*)_pipe.data;
    dst = (unsigned char*)new_data;

    // apply the transformer to each element
    for (i = 0; i < _pipe.count; i++)
    {
        if (!_transform(src + (i * _pipe.element_size),
                        dst + (i * _pipe.element_size),
                        _context))
        {
            free(new_data);
            _pipe.error_code = -1;

            return _pipe;
        }
    }

    // free old data if we owned it
    if (_pipe.owns_data && _pipe.data)
    {
        free(_pipe.data);
    }

    result.data         = new_data;
    result.element_size = _pipe.element_size;
    result.count        = _pipe.count;
    result.owns_data    = true;
    result.error_code   = 0;

    return result;
}

/*
d_functional_pipeline_filter
  Filters elements in the pipeline, keeping only those for which the
predicate returns true. Allocates a new buffer for the results. The old
buffer is freed if the pipeline owned it.

Parameter(s):
  _pipe:    the current pipeline state.
  _test:    predicate function to test each element.
  _context: context forwarded to _test; may be NULL.
Return:
  A new pipeline containing only the elements that passed the predicate.
If the pipeline is in an error state, _test is NULL, or allocation fails,
returns a pipeline with the appropriate error_code.
*/
struct d_functional_pipeline
d_functional_pipeline_filter
(
    struct d_functional_pipeline _pipe,
    fn_predicate                  _test,
    void*                        _context
)
{
    struct d_functional_pipeline result;
    void*                        new_data;
    const unsigned char*         src;
    unsigned char*               dst;
    size_t                       out_count;
    size_t                       i;

    // propagate prior errors
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    // validate predicate
    if (!_test)
    {
        _pipe.error_code = -1;

        return _pipe;
    }

    // allocate worst-case buffer (all elements pass)
    new_data = malloc(_pipe.count * _pipe.element_size);

    // check allocation
    if (!new_data)
    {
        _pipe.error_code = -1;

        return _pipe;
    }

    src       = (const unsigned char*)_pipe.data;
    dst       = (unsigned char*)new_data;
    out_count = 0;

    // copy elements that pass the predicate
    for (i = 0; i < _pipe.count; i++)
    {
        if (_test(src + (i * _pipe.element_size), _context))
        {
            memcpy(dst + (out_count * _pipe.element_size),
                   src + (i * _pipe.element_size),
                   _pipe.element_size);
            out_count++;
        }
    }

    // free old data if we owned it
    if (_pipe.owns_data && _pipe.data)
    {
        free(_pipe.data);
    }

    result.data         = new_data;
    result.element_size = _pipe.element_size;
    result.count        = out_count;
    result.owns_data    = true;
    result.error_code   = 0;

    return result;
}

/*
d_functional_pipeline_fold
  Folds (reduces) all elements in the pipeline into a single accumulated
value. The pipeline's data is freed if owned, and the result pipeline wraps
the accumulator.

Parameter(s):
  _pipe:             the current pipeline state.
  _initial:          pointer to the initial accumulator value; this buffer
                     is modified in-place with the result.
  _accumulator_size: size in bytes of the accumulator value.
  _combine:          accumulator function applied at each step.
  _context:          context forwarded to _combine; may be NULL.
Return:
  A new pipeline wrapping _initial with count 1 and element_size set to
_accumulator_size. The pipeline does NOT own _initial. If the pipeline is
in an error state, _initial is NULL, _combine is NULL, or accumulation
fails, returns a pipeline with the appropriate error_code.
*/
struct d_functional_pipeline
d_functional_pipeline_fold
(
    struct d_functional_pipeline _pipe,
    void*                        _initial,
    size_t                       _accumulator_size,
    fn_accumulator                _combine,
    void*                        _context
)
{
    struct d_functional_pipeline result;
    const unsigned char*         src;
    size_t                       i;

    // propagate prior errors
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    // validate parameters
    if ( (!_initial)              ||
         (!_combine)              ||
         (_accumulator_size == 0) )
    {
        _pipe.error_code = -1;

        return _pipe;
    }

    src = (const unsigned char*)_pipe.data;

    // accumulate from left to right
    for (i = 0; i < _pipe.count; i++)
    {
        if (!_combine(_initial,
                      src + (i * _pipe.element_size),
                      _context))
        {
            _pipe.error_code = -1;

            return _pipe;
        }
    }

    // free old data if we owned it
    if (_pipe.owns_data && _pipe.data)
    {
        free(_pipe.data);
    }

    result.data         = _initial;
    result.element_size = _accumulator_size;
    result.count        = 1;
    result.owns_data    = false;
    result.error_code   = 0;

    return result;
}

/*
d_functional_pipeline_for_each
  Applies a consumer function to each element in the pipeline. The data is
not modified or reallocated; the pipeline is passed through unchanged.

Parameter(s):
  _pipe:    the current pipeline state.
  _apply:   consumer function to apply to each element.
  _context: context forwarded to _apply; may be NULL.
Return:
  The same pipeline, unchanged. If the pipeline is in an error state or
_apply is NULL, returns a pipeline with the appropriate error_code.
*/
struct d_functional_pipeline
d_functional_pipeline_for_each
(
    struct d_functional_pipeline _pipe,
    fn_consumer                   _apply,
    void*                        _context
)
{
    unsigned char* src;
    size_t         i;

    // propagate prior errors
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    // validate consumer
    if (!_apply)
    {
        _pipe.error_code = -1;

        return _pipe;
    }

    src = (unsigned char*)_pipe.data;

    // apply to each element
    for (i = 0; i < _pipe.count; i++)
    {
        _apply(src + (i * _pipe.element_size), _context);
    }

    return _pipe;
}

/*
d_functional_pipeline_take
  Reduces the pipeline to at most the first _n elements. No data is copied
or reallocated; the count is simply clamped.

Parameter(s):
  _pipe: the current pipeline state.
  _n:    maximum number of elements to keep.
Return:
  The pipeline with count reduced to min(count, _n). If the pipeline is in
an error state, returns it unchanged.
*/
struct d_functional_pipeline
d_functional_pipeline_take
(
    struct d_functional_pipeline _pipe,
    size_t                       _n
)
{
    // propagate prior errors
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    // clamp count
    if (_n < _pipe.count)
    {
        _pipe.count = _n;
    }

    return _pipe;
}

/*
d_functional_pipeline_skip
  Advances the pipeline past the first _n elements. The data pointer is
adjusted forward and the count is reduced accordingly. No data is copied
or reallocated.

Parameter(s):
  _pipe: the current pipeline state.
  _n:    number of elements to skip.
Return:
  The pipeline with data pointer advanced by _n elements and count reduced.
If _n >= count, the pipeline becomes empty (count = 0). If the pipeline is
in an error state, returns it unchanged.
*/
struct d_functional_pipeline
d_functional_pipeline_skip
(
    struct d_functional_pipeline _pipe,
    size_t                       _n
)
{
    // propagate prior errors
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    // skip past all elements
    if (_n >= _pipe.count)
    {
        _pipe.count = 0;

        return _pipe;
    }

    // advance the data pointer
    _pipe.data = (unsigned char*)_pipe.data +
                 (_n * _pipe.element_size);
    _pipe.count -= _n;

    return _pipe;
}

/*
d_functional_pipeline_end
  Finalizes the pipeline, returning the data pointer and element count.
The caller takes ownership of the data if the pipeline owned it.

Parameter(s):
  _pipe:      the pipeline to finalize.
  _out_count: pointer to receive the number of elements; may be NULL.
Return:
  A pointer to the pipeline's data, or NULL if the pipeline was in an
error state. If _out_count is non-NULL, the element count is written to
it.
*/
void*
d_functional_pipeline_end
(
    struct d_functional_pipeline _pipe,
    size_t*                      _out_count
)
{
    // write count if requested
    if (_out_count)
    {
        *_out_count = _pipe.count;
    }

    // return NULL on error
    if (_pipe.error_code != 0)
    {
        return NULL;
    }

    return _pipe.data;
}

/*
d_functional_pipeline_free
  Frees the pipeline's data if the pipeline owns it, and resets the
pipeline to an empty state.

Parameter(s):
  _pipe: pointer to the pipeline to free; may be NULL.
Return:
  none.
*/
void
d_functional_pipeline_free
(
    struct d_functional_pipeline* _pipe
)
{
    if (!_pipe)
    {
        return;
    }

    // free data if we own it
    if (_pipe->owns_data && _pipe->data)
    {
        free(_pipe->data);
    }

    _pipe->data       = NULL;
    _pipe->count      = 0;
    _pipe->owns_data  = false;
    _pipe->error_code = 0;

    return;
}
