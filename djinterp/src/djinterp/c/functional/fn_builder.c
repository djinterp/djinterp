/*******************************************************************************
* djinterp [c]                                                      fn_builder.c
*
* The fluent function builder fn_builder.h declares.
*   The builder keeps its transformers and its predicates in separate arrays,
* so it cannot record how they interleaved. It therefore runs as a query:
* an element is kept when every predicate accepts it (WHERE), and a kept
* element then passes through every transformer in the order they were added
* (SELECT). Callbacks receive a NULL context, which the builder does not store.
*   The builder methods are spelled `d_funtional_builder_*`, as the header
* declares them.
*
*
* path:      /src/djinterp/c/functional/fn_builder.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/fn_builder.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, realloc, free
#include <string.h>   // memcpy
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX


struct d_fn_builder*
d_fn_builder_new(void)
{
    struct d_fn_builder* const builder = malloc(sizeof(*builder));

    if (!builder)
    {
        return NULL;
    }

    builder->transforms      = malloc(D_FN_BUILDER_INITIAL_CAPACITY *
                                      sizeof(fn_transformer));
    builder->predicates      = malloc(D_FN_BUILDER_INITIAL_CAPACITY *
                                      sizeof(fn_predicate));
    builder->transform_count = 0u;
    builder->predicate_count = 0u;
    builder->capacity        = D_FN_BUILDER_INITIAL_CAPACITY;

    // all or nothing
    if ( (!builder->transforms) ||
         (!builder->predicates) )
    {
        free(builder->transforms);
        free(builder->predicates);
        free(builder);

        return NULL;
    }

    return builder;
}

/*
d_internal_fn_builder_room
  Both arrays share one capacity, so both grow together, doubling. On a
failed growth the builder is unchanged.
*/
static bool
d_internal_fn_builder_room(
    struct d_fn_builder* _builder
)
{
    // room in both arrays already
    if ( (_builder->transform_count < _builder->capacity) &&
         (_builder->predicate_count < _builder->capacity) )
    {
        return true;
    }

    // doubling would overflow
    if (_builder->capacity > (SIZE_MAX / 2u / sizeof(fn_transformer)))
    {
        return false;
    }

    const size_t capacity = _builder->capacity * 2u;
    fn_transformer* const transforms =
        realloc(_builder->transforms, capacity * sizeof(fn_transformer));

    if (!transforms)
    {
        return false;
    }

    _builder->transforms = transforms;

    fn_predicate* const predicates =
        realloc(_builder->predicates, capacity * sizeof(fn_predicate));

    if (!predicates)
    {
        return false;
    }

    _builder->predicates = predicates;
    _builder->capacity   = capacity;

    return true;
}

/*
d_funtional_builder_map
  Appends a transformer. Returns NULL when the builder is missing or cannot
grow -- the builder itself is then unchanged and still the caller's to free,
so keep the original pointer when chaining. A missing transformer adds
nothing.
*/
struct d_fn_builder*
d_funtional_builder_map(
    struct d_fn_builder* _builder,
    fn_transformer       _transform
)
{
    // no builder, or nothing to add
    if ( (!_builder) ||
         (!_transform) )
    {
        return _builder;
    }

    // make room first
    if (!d_internal_fn_builder_room(_builder))
    {
        return NULL;
    }

    _builder->transforms[_builder->transform_count] = _transform;
    ++_builder->transform_count;

    return _builder;
}

/*
d_funtional_builder_and_then
  The same as map: a further transformer, applied after those added before.
*/
struct d_fn_builder*
d_funtional_builder_and_then(
    struct d_fn_builder* _builder,
    fn_transformer       _transform
)
{
    return d_funtional_builder_map(_builder, _transform);
}

/*
d_funtional_builder_filter
  Appends a predicate; failure as for map.
*/
struct d_fn_builder*
d_funtional_builder_filter(
    struct d_fn_builder* _builder,
    fn_predicate         _test
)
{
    // no builder, or nothing to add
    if ( (!_builder) ||
         (!_test) )
    {
        return _builder;
    }

    // make room first
    if (!d_internal_fn_builder_room(_builder))
    {
        return NULL;
    }

    _builder->predicates[_builder->predicate_count] = _test;
    ++_builder->predicate_count;

    return _builder;
}

/*
d_funtional_builder_where
  The same as filter.
*/
struct d_fn_builder*
d_funtional_builder_where(
    struct d_fn_builder* _builder,
    fn_predicate         _test
)
{
    return d_funtional_builder_filter(_builder, _test);
}

/*
d_fn_builder_execute
  Kept elements are written contiguously to `_output`, which must hold
`_count` elements of `_element_size` bytes (transformers keep the size).
Between transformers the value moves through two scratch elements. A
transformer that fails stops the run: false, with `*_out_count` the number of
elements completed.
*/
bool
d_fn_builder_execute(
    const struct d_fn_builder* _builder,
    const void*                _input,
    size_t                     _count,
    size_t                     _element_size,
    void*                      _output,
    size_t*                    _out_count
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

    // the scratch elements a chain of transformers passes values through
    unsigned char* scratch = NULL;

    if ( (_builder->transform_count > 1u) &&
         (_count > 0u) )
    {
        if (_element_size > (SIZE_MAX / 2u))
        {
            return false;
        }

        scratch = malloc(2u * _element_size);

        if (!scratch)
        {
            return false;
        }
    }

    bool ok = true;

    // WHERE, then SELECT, element by element
    for (size_t i = 0u; (ok) && (i < _count); ++i)
    {
        const void* const element =
            (const unsigned char*)_input + (i * _element_size);
        void* const       slot    =
            (unsigned char*)_output + (written * _element_size);
        bool              kept    = true;

        // every predicate must accept the element
        for (size_t p = 0u; (kept) && (p < _builder->predicate_count); ++p)
        {
            kept = _builder->predicates[p](element, NULL);
        }

        if (!kept)
        {
            continue;
        }

        // no transformer: the element itself
        if (_builder->transform_count == 0u)
        {
            memcpy(slot, element, _element_size);
            ++written;

            continue;
        }

        // each transformer reads the last one's result; the last one writes
        // the output slot
        const void* source = element;

        for (size_t t = 0u; (ok) && (t < _builder->transform_count); ++t)
        {
            void* const target = (t + 1u == _builder->transform_count)
                ? slot
                : (void*)(scratch + ((t % 2u) * _element_size));

            ok     = _builder->transforms[t](source, target, NULL);
            source = target;
        }

        if (ok)
        {
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

void
d_fn_builder_free(
    struct d_fn_builder* _builder
)
{
    // nothing to release
    if (!_builder)
    {
        return;
    }

    free(_builder->transforms);
    free(_builder->predicates);
    free(_builder);

    return;
}
