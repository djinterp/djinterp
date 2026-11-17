/*******************************************************************************
* djinterp [c]                                                      fn_builder.c
*
* The fluent function builder fn_builder.h declares.
*   The builder keeps one array of steps, each a transformer or a predicate,
* in the order they were added, and runs it in that order: a predicate tests
* the value the steps before it produced. (It kept two arrays until
* 2026.11.11, and so ran every predicate on the original element before any
* transformer, which the C++ face did not; the owner ruled for call order.)
* Callbacks receive a NULL context, which the builder does not store.
*
*
* path:      /src/djinterp/c/functional/fn_builder.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/fn_builder.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, realloc, free
#include <string.h>   // memmove
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_mul


struct dj_fn_builder* d_fn_builder_new(void)
{
    struct dj_fn_builder* const builder = malloc(sizeof(*builder));

    if (!builder)
    {
        return NULL;
    }

    builder->steps    = malloc(D_FN_BUILDER_INITIAL_CAPACITY *
                               sizeof(struct dj_fn_builder_step));
    builder->count    = 0u;
    builder->capacity = D_FN_BUILDER_INITIAL_CAPACITY;

    // all or nothing
    if (!builder->steps)
    {
        free(builder);

        return NULL;
    }

    return builder;
}

/*
d_internal_fn_builder_append
  Grows the chain by doubling when it is full, then records the step. Every
fluent operation comes through here, so a failure leaves the builder exactly
as it was: the growth is checked (d_ckd_mul) before realloc, and realloc
keeps the old array on failure.
*/
static
struct dj_fn_builder* d_internal_fn_builder_append(
    struct dj_fn_builder*            _builder,
    const struct dj_fn_builder_step* _step
)
{
    // room for one more step
    if (_builder->count == _builder->capacity)
    {
        size_t capacity = 0u;
        size_t bytes    = 0u;

        // a doubled capacity, or its size in bytes, that does not fit
        if ( (d_ckd_mul(&capacity, _builder->capacity, 2u)) ||
             (d_ckd_mul(&bytes, capacity, sizeof(struct dj_fn_builder_step))) )
        {
            return NULL;
        }

        struct dj_fn_builder_step* const steps =
            realloc(_builder->steps, bytes);

        if (!steps)
        {
            return NULL;
        }

        _builder->steps    = steps;
        _builder->capacity = capacity;
    }

    _builder->steps[_builder->count] = *_step;
    ++_builder->count;

    return _builder;
}

/*
d_fn_builder_map
  A missing transformer adds nothing, and the builder comes back unchanged.
*/
struct dj_fn_builder* d_fn_builder_map(
    struct dj_fn_builder* _builder,
    fn_transformer        _transform
)
{
    struct dj_fn_builder_step step;

    // no builder, or nothing to add
    if ( (!_builder) ||
         (!_transform) )
    {
        return _builder;
    }

    step.kind         = D_FN_BUILDER_STEP_TRANSFORM;
    step.fn.transform = _transform;

    return d_internal_fn_builder_append(_builder, &step);
}

/*
d_fn_builder_and_then
  The same as map: a further transformer, applied after the steps before it.
*/
struct dj_fn_builder* d_fn_builder_and_then(
    struct dj_fn_builder* _builder,
    fn_transformer        _transform
)
{
    return d_fn_builder_map(_builder, _transform);
}

/*
d_fn_builder_filter
  A missing predicate adds nothing, as for map.
*/
struct dj_fn_builder* d_fn_builder_filter(
    struct dj_fn_builder* _builder,
    fn_predicate          _test
)
{
    struct dj_fn_builder_step step;

    // no builder, or nothing to add
    if ( (!_builder) ||
         (!_test) )
    {
        return _builder;
    }

    step.kind         = D_FN_BUILDER_STEP_PREDICATE;
    step.fn.predicate = _test;

    return d_internal_fn_builder_append(_builder, &step);
}

/*
d_fn_builder_where
  The same as filter.
*/
struct dj_fn_builder* d_fn_builder_where(
    struct dj_fn_builder* _builder,
    fn_predicate          _test
)
{
    return d_fn_builder_filter(_builder, _test);
}

/*
d_internal_fn_builder_has_transform
  Whether any step is a transformer, which is when a run needs working
memory for the values it computes.
*/
static
bool d_internal_fn_builder_has_transform(
    const struct dj_fn_builder* _builder
)
{
    for (size_t s = 0u; s < _builder->count; ++s)
    {
        if (_builder->steps[s].kind == D_FN_BUILDER_STEP_TRANSFORM)
        {
            return true;
        }
    }

    return false;
}

/*
d_internal_fn_builder_run_one
  Takes one element through the chain. The value in hand starts as the
element itself; each transformer writes its image to the scratch element the
previous transformer did not write, so a transformer never reads and writes
the same memory, and then that image is the value in hand. A rejecting
predicate drops the element, and the remaining steps do not run.
  *_value is the value in hand at the end, valid when the element is kept.
Returns false when a transformer fails.
*/
static
bool d_internal_fn_builder_run_one(
    const struct dj_fn_builder* _builder,
    const void*                 _element,
    unsigned char*              _scratch,
    size_t                      _element_size,
    const void**                _value,
    bool*                       _kept
)
{
    const void* value = _element;
    size_t      next  = 0u;

    *_kept = true;

    for (size_t s = 0u; s < _builder->count; ++s)
    {
        const struct dj_fn_builder_step* const step = &_builder->steps[s];

        // a predicate on the value in hand
        if (step->kind == D_FN_BUILDER_STEP_PREDICATE)
        {
            if (!step->fn.predicate(value, NULL))
            {
                *_kept = false;

                return true;
            }

            continue;
        }

        // a transformer, into the other scratch element
        void* const target = _scratch + (next * _element_size);

        if (!step->fn.transform(value, target, NULL))
        {
            return false;
        }

        value = target;
        next  = 1u - next;
    }

    *_value = value;

    return true;
}

/*
d_fn_builder_execute
  A kept element's final value is moved into the next output slot only after
the whole chain has run, so a rejected element touches nothing past
*_out_count; memmove, not memcpy, because with no transformer the value is the
input element itself, which is the slot when the chain runs in place.
*/
bool d_fn_builder_execute(
    const struct dj_fn_builder* _builder,
    const void*                 _input,
    size_t                      _count,
    size_t                      _element_size,
    void*                       _output,
    size_t*                     _out_count
)
{
    size_t written = 0u;

    if (_out_count)
    {
        *_out_count = 0u;
    }

    // a builder, and both arrays whenever there is anything to run
    if ( (!_builder) ||
         ( (_count > 0u) &&
           ( (!_input) || (!_output) || (_element_size == 0u) ) ) )
    {
        return false;
    }

    // two scratch elements, for the values transformers compute
    unsigned char* scratch = NULL;

    if ( (_count > 0u) &&
         (d_internal_fn_builder_has_transform(_builder)) )
    {
        size_t scratch_bytes = 0u;

        // refused where their size does not fit
        if (d_ckd_mul(&scratch_bytes, 2u, _element_size))
        {
            return false;
        }

        scratch = malloc(scratch_bytes);

        if (!scratch)
        {
            return false;
        }
    }

    bool ok = true;

    for (size_t i = 0u; (ok) && (i < _count); ++i)
    {
        const void* const element =
            (const unsigned char*)_input + (i * _element_size);
        const void*       value   = NULL;
        bool              kept    = false;

        ok = d_internal_fn_builder_run_one(_builder,
                                           element,
                                           scratch,
                                           _element_size,
                                           &value,
                                           &kept);

        if ( (ok) &&
             (kept) )
        {
            memmove((unsigned char*)_output + (written * _element_size),
                    value,
                    _element_size);
            ++written;
        }
    }

    free(scratch);

    if (_out_count)
    {
        *_out_count = written;
    }

    return ok;
}

void d_fn_builder_free(
    struct dj_fn_builder* _builder
)
{
    // nothing to release
    if (!_builder)
    {
        return;
    }

    free(_builder->steps);
    free(_builder);

    return;
}
