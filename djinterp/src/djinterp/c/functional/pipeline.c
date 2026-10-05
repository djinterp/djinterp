/*******************************************************************************
* djinterp [c]                                                        pipeline.c
*
* The chainable pipeline pipeline.h declares.
*   A pipeline either views the caller's array (begin) or owns a copy (every
* operation that has to produce new elements, and begin_copy). It never
* writes to a view's array except through for_each, whose consumer the
* caller supplied for that purpose. The first failure is recorded in
* error_code (EINVAL for a bad argument or a callback that failed, ENOMEM for
* an allocation) and every later operation passes the pipeline through
* unchanged, so a chain needs one check at its end.
*
*
* path:      /src/djinterp/c/functional/pipeline.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/pipeline.h"  // corresponding header
// std
#include <errno.h>    // EINVAL, ENOMEM
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memmove, memset
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX


/*
d_internal_pipeline_failed
  Records the first failure; the pipeline keeps whatever data it had, so
free still releases it.
*/
static struct d_functional_pipeline
d_internal_pipeline_failed(
    struct d_functional_pipeline _pipe,
    int                          _error
)
{
    if (_pipe.error_code == 0)
    {
        _pipe.error_code = _error;
    }

    return _pipe;
}

/*
d_internal_pipeline_bytes
  The bytes `_count` elements take, or SIZE_MAX when that would overflow.
*/
static size_t
d_internal_pipeline_bytes(
    size_t _count,
    size_t _element_size
)
{
    if ( (_element_size > 0u) &&
         (_count > (SIZE_MAX / _element_size)) )
    {
        return SIZE_MAX;
    }

    return _count * _element_size;
}

/*
d_internal_pipeline_adopt
  Replaces the pipeline's data with a new owned block, releasing the old one
if the pipeline owned it.
*/
static struct d_functional_pipeline
d_internal_pipeline_adopt(
    struct d_functional_pipeline _pipe,
    void*                        _data,
    size_t                       _count,
    size_t                       _element_size
)
{
    if (_pipe.owns_data)
    {
        free(_pipe.data);
    }

    _pipe.data         = _data;
    _pipe.count        = _count;
    _pipe.element_size = _element_size;
    _pipe.owns_data    = true;

    return _pipe;
}

struct d_functional_pipeline
d_functional_pipeline_begin(
    void*  _data,
    size_t _count,
    size_t _element_size
)
{
    struct d_functional_pipeline pipe;

    pipe.data         = _data;
    pipe.element_size = _element_size;
    pipe.count        = _count;
    pipe.owns_data    = false;
    pipe.error_code   = 0;

    // elements need an array, and a size
    if ( (_count > 0u) &&
         ( (!_data) || (_element_size == 0u) ) )
    {
        pipe.count      = 0u;
        pipe.error_code = EINVAL;
    }

    return pipe;
}

struct d_functional_pipeline
d_functional_pipeline_begin_copy(
    const void* _data,
    size_t      _count,
    size_t      _element_size
)
{
    struct d_functional_pipeline pipe =
        d_functional_pipeline_begin((void*)_data, _count, _element_size);

    // nothing to copy
    if ( (pipe.error_code != 0) ||
         (_count == 0u) )
    {
        pipe.data = NULL;

        return pipe;
    }

    const size_t bytes = d_internal_pipeline_bytes(_count, _element_size);
    void* const  copy  = (bytes == SIZE_MAX) ? NULL : malloc(bytes);

    if (!copy)
    {
        pipe.data  = NULL;
        pipe.count = 0u;

        return d_internal_pipeline_failed(pipe, ENOMEM);
    }

    memcpy(copy, _data, bytes);
    pipe.data      = copy;
    pipe.owns_data = true;

    return pipe;
}

/*
d_functional_pipeline_map
  Writes every result to a new owned block of the same element size, so a
transformer never reads and writes one element in place.
*/
struct d_functional_pipeline
d_functional_pipeline_map(
    struct d_functional_pipeline _pipe,
    fn_transformer               _transform,
    void*                        _context
)
{
    // an earlier failure, or nothing to apply
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    if (!_transform)
    {
        return d_internal_pipeline_failed(_pipe, EINVAL);
    }

    if (_pipe.count == 0u)
    {
        return _pipe;
    }

    const size_t bytes = d_internal_pipeline_bytes(_pipe.count,
                                                   _pipe.element_size);
    unsigned char* const out = (bytes == SIZE_MAX) ? NULL : malloc(bytes);

    if (!out)
    {
        return d_internal_pipeline_failed(_pipe, ENOMEM);
    }

    // transform each element into its slot
    for (size_t i = 0u; i < _pipe.count; ++i)
    {
        const size_t at = i * _pipe.element_size;

        if (!_transform((const unsigned char*)_pipe.data + at,
                        out + at,
                        _context))
        {
            free(out);

            return d_internal_pipeline_failed(_pipe, EINVAL);
        }
    }

    return d_internal_pipeline_adopt(_pipe,
                                     out,
                                     _pipe.count,
                                     _pipe.element_size);
}

/*
d_functional_pipeline_filter
  An owned block is compacted in place; a view is copied first, since the
caller's array is not the pipeline's to rearrange.
*/
struct d_functional_pipeline
d_functional_pipeline_filter(
    struct d_functional_pipeline _pipe,
    fn_predicate                 _test,
    void*                        _context
)
{
    // an earlier failure, or nothing to test with
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    if (!_test)
    {
        return d_internal_pipeline_failed(_pipe, EINVAL);
    }

    if (_pipe.count == 0u)
    {
        return _pipe;
    }

    unsigned char* target = _pipe.data;

    // a view gets its own block before anything moves
    if (!_pipe.owns_data)
    {
        const size_t bytes = d_internal_pipeline_bytes(_pipe.count,
                                                       _pipe.element_size);

        target = (bytes == SIZE_MAX) ? NULL : malloc(bytes);

        if (!target)
        {
            return d_internal_pipeline_failed(_pipe, ENOMEM);
        }
    }

    size_t kept = 0u;

    // keep the accepted elements, in order
    for (size_t i = 0u; i < _pipe.count; ++i)
    {
        const unsigned char* const element =
            (const unsigned char*)_pipe.data + (i * _pipe.element_size);

        if (_test(element, _context))
        {
            memmove(target + (kept * _pipe.element_size),
                    element,
                    _pipe.element_size);
            ++kept;
        }
    }

    // compacted in place: only the count changes
    if (_pipe.owns_data)
    {
        _pipe.count = kept;

        return _pipe;
    }

    return d_internal_pipeline_adopt(_pipe, target, kept, _pipe.element_size);
}

/*
d_functional_pipeline_fold
  Reduces the pipeline to one owned element of `_accumulator_size` bytes,
starting from `*_initial` (zero when `_initial` is NULL), combining in order.
*/
struct d_functional_pipeline
d_functional_pipeline_fold(
    struct d_functional_pipeline _pipe,
    void*                        _initial,
    size_t                       _accumulator_size,
    fn_accumulator               _combine,
    void*                        _context
)
{
    // an earlier failure, or nothing to fold with
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    if ( (!_combine) ||
         (_accumulator_size == 0u) )
    {
        return d_internal_pipeline_failed(_pipe, EINVAL);
    }

    unsigned char* const accumulator = malloc(_accumulator_size);

    if (!accumulator)
    {
        return d_internal_pipeline_failed(_pipe, ENOMEM);
    }

    // start from the initial value, or from zero
    if (_initial)
    {
        memcpy(accumulator, _initial, _accumulator_size);
    }
    else
    {
        memset(accumulator, 0, _accumulator_size);
    }

    // combine every element, first to last
    for (size_t i = 0u; i < _pipe.count; ++i)
    {
        if (!_combine(accumulator,
                      (const unsigned char*)_pipe.data +
                          (i * _pipe.element_size),
                      _context))
        {
            free(accumulator);

            return d_internal_pipeline_failed(_pipe, EINVAL);
        }
    }

    return d_internal_pipeline_adopt(_pipe, accumulator, 1u,
                                     _accumulator_size);
}

struct d_functional_pipeline
d_functional_pipeline_for_each(
    struct d_functional_pipeline _pipe,
    fn_consumer                  _apply,
    void*                        _context
)
{
    // an earlier failure, or nothing to apply
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    if (!_apply)
    {
        return d_internal_pipeline_failed(_pipe, EINVAL);
    }

    // apply to each element, in place
    for (size_t i = 0u; i < _pipe.count; ++i)
    {
        _apply((unsigned char*)_pipe.data + (i * _pipe.element_size),
               _context);
    }

    return _pipe;
}

/*
d_functional_pipeline_skip
  A view moves its start; an owned block keeps its start, which free needs,
and moves the kept elements down instead.
*/
struct d_functional_pipeline
d_functional_pipeline_skip(
    struct d_functional_pipeline _pipe,
    size_t                       _n
)
{
    // an earlier failure
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    const size_t skipped = (_n < _pipe.count) ? _n : _pipe.count;
    const size_t offset  = skipped * _pipe.element_size;

    if (skipped == 0u)
    {
        return _pipe;
    }

    if (_pipe.owns_data)
    {
        memmove(_pipe.data,
                (unsigned char*)_pipe.data + offset,
                (_pipe.count - skipped) * _pipe.element_size);
    }
    else
    {
        _pipe.data = (unsigned char*)_pipe.data + offset;
    }

    _pipe.count -= skipped;

    return _pipe;
}

struct d_functional_pipeline
d_functional_pipeline_take(
    struct d_functional_pipeline _pipe,
    size_t                       _n
)
{
    // an earlier failure
    if (_pipe.error_code != 0)
    {
        return _pipe;
    }

    if (_n < _pipe.count)
    {
        _pipe.count = _n;
    }

    return _pipe;
}

/*
d_functional_pipeline_end
  Always hands the caller a block of its own, to release with free(): an
owned block is handed over, a view is copied. So after end, the pipeline
must not also be freed. A failed pipeline, or an empty result, yields NULL
and a count of 0 -- check error_code first to tell them apart. A failed
pipeline's own block is released here.
*/
void*
d_functional_pipeline_end(
    struct d_functional_pipeline _pipe,
    size_t*                      _out_count
)
{
    if (_out_count)
    {
        *_out_count = 0u;
    }

    // a failure or an empty result: nothing to hand over
    if ( (_pipe.error_code != 0) ||
         (_pipe.count == 0u) )
    {
        if (_pipe.owns_data)
        {
            free(_pipe.data);
        }

        return NULL;
    }

    void* result = _pipe.data;

    // a view is copied, so the caller always owns what it receives
    if (!_pipe.owns_data)
    {
        const size_t bytes = d_internal_pipeline_bytes(_pipe.count,
                                                       _pipe.element_size);

        result = (bytes == SIZE_MAX) ? NULL : malloc(bytes);

        if (!result)
        {
            return NULL;
        }

        memcpy(result, _pipe.data, bytes);
    }

    if (_out_count)
    {
        *_out_count = _pipe.count;
    }

    return result;
}

void
d_functional_pipeline_free(
    struct d_functional_pipeline* _pipe
)
{
    // nothing to release
    if (!_pipe)
    {
        return;
    }

    if (_pipe->owns_data)
    {
        free(_pipe->data);
    }

    memset(_pipe, 0, sizeof(*_pipe));

    return;
}
