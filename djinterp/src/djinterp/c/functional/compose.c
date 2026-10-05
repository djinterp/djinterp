/*******************************************************************************
* djinterp [c]                                                         compose.c
*
* Transformer composition and partial consumers, as compose.h declares.
*   A composed transformer owns one scratch block for the intermediate value,
* so applying one is not reentrant: two threads, or a transformer that
* applies its own composition, need a composition each.
*
*
* path:      /src/djinterp/c/functional/compose.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/compose.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free


/*
d_functional_compose_new
  Composes `_second` after `_first`: apply(x) is second(first(x)). The
intermediate value lives in a block of `_temp_size` bytes, which must hold
whatever `_first` writes.
*/
struct d_composed_transformer*
d_functional_compose_new(
    fn_transformer _first,
    void*          _context1,
    fn_transformer _second,
    void*          _context2,
    size_t         _temp_size
)
{
    // both stages, and room for what passes between them
    if ( (!_first)  ||
         (!_second) ||
         (_temp_size == 0u) )
    {
        return NULL;
    }

    struct d_composed_transformer* const composed = malloc(sizeof(*composed));

    if (!composed)
    {
        return NULL;
    }

    composed->temp_buf = malloc(_temp_size);

    if (!composed->temp_buf)
    {
        free(composed);

        return NULL;
    }

    composed->first     = _first;
    composed->second    = _second;
    composed->context1  = _context1;
    composed->context2  = _context2;
    composed->temp_size = _temp_size;

    return composed;
}

bool
d_functional_compose_apply(
    const struct d_composed_transformer* _composed,
    const void*                          _input,
    void*                                _output
)
{
    // nothing to apply
    if (!_composed)
    {
        return false;
    }

    // the second stage runs only on a first stage that succeeded
    return ( (_composed->first(_input,
                               _composed->temp_buf,
                               _composed->context1)) &&
             (_composed->second(_composed->temp_buf,
                                _output,
                                _composed->context2)) );
}

void
d_functional_compose_free(
    struct d_composed_transformer* _composed
)
{
    // nothing to release
    if (!_composed)
    {
        return;
    }

    free(_composed->temp_buf);
    free(_composed);

    return;
}

struct d_partial_consumer*
d_functional_partial_consumer_new(
    fn_consumer _consumer,
    void*       _context
)
{
    // the consumer is required
    if (!_consumer)
    {
        return NULL;
    }

    struct d_partial_consumer* const partial = malloc(sizeof(*partial));

    if (partial)
    {
        partial->consumer = _consumer;
        partial->context  = _context;
    }

    return partial;
}

void
d_functional_partial_consumer_apply(
    const struct d_partial_consumer* _partial,
    void*                            _element
)
{
    // apply the bound consumer with its bound context
    if (_partial)
    {
        _partial->consumer(_element, _partial->context);
    }

    return;
}

void
d_functional_partial_consumer_free(
    struct d_partial_consumer* _partial
)
{
    free(_partial);

    return;
}
