#include "..\..\inc\functional\compose.h"


/*
d_functional_compose_new
  Creates a heap-allocated composed transformer that applies two transformers
in sequence: first (g), then second (f), implementing f(g(x)).
  Allocates an internal temporary buffer of _temp_size bytes for the
intermediate result.

Parameter(s):
  _first:     the transformer to apply first (g).
  _context1:  context forwarded to _first; may be NULL.
  _second:    the transformer to apply second (f).
  _context2:  context forwarded to _second; may be NULL.
  _temp_size: size in bytes of the intermediate result between _first and
              _second.
Return:
  A pointer to a newly allocated d_composed_transformer, or NULL if _first
was NULL, _second was NULL, _temp_size was 0, or allocation failed.
*/
struct d_composed_transformer*
d_functional_compose_new
(
    fn_transformer _first,
    void*          _context1,
    fn_transformer _second,
    void*          _context2,
    size_t         _temp_size
)
{
    struct d_composed_transformer* result;
    void*                          temp_buf;

    // validate parameters
    if ( (!_first)  ||
         (!_second) ||
         (_temp_size == 0) )
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_composed_transformer));

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    temp_buf = malloc(_temp_size);

    // ensure that memory allocation was successful
    if (!temp_buf)
    {
        free(result);

        return NULL;
    }

    result->first     = _first;
    result->second    = _second;
    result->context1  = _context1;
    result->context2  = _context2;
    result->temp_size = _temp_size;
    result->temp_buf  = temp_buf;

    return result;
}

/*
d_functional_compose_apply
  Applies a composed transformer to an input, writing the final result to
_output. Internally applies the first transformer to _input, stores the
intermediate result in the composed transformer's temporary buffer, then
applies the second transformer to produce _output.

Parameter(s):
  _composed: pointer to the composed transformer.
  _input:    pointer to the input element.
  _output:   pointer to the output destination.
Return:
  A boolean value corresponding to either:
  - true, if both transformations succeeded, or
  - false, if _composed was NULL, any internal pointer was NULL, or either
    transformation failed.
*/
bool
d_functional_compose_apply
(
    const struct d_composed_transformer* _composed,
    const void*                          _input,
    void*                                _output
)
{
    // validate parameters
    if ( (!_composed)           ||
         (!_composed->first)    ||
         (!_composed->second)   ||
         (!_composed->temp_buf) ||
         (!_input)              ||
         (!_output) )
    {
        return false;
    }

    // zero the temp buffer before use
    memset(_composed->temp_buf, 0, _composed->temp_size);

    // apply first transformer: input -> temp
    if (!_composed->first(_input,
                          _composed->temp_buf,
                          _composed->context1))
    {
        return false;
    }

    // apply second transformer: temp -> output
    if (!_composed->second(_composed->temp_buf,
                           _output,
                           _composed->context2))
    {
        return false;
    }

    return true;
}

/*
d_functional_compose_free
  Frees a composed transformer and its internal temporary buffer.

Parameter(s):
  _composed: pointer to the composed transformer to free; may be NULL.
Return:
  none.
*/
void
d_functional_compose_free
(
    struct d_composed_transformer* _composed
)
{
    if (_composed)
    {
        if (_composed->temp_buf)
        {
            free(_composed->temp_buf);
        }

        free(_composed);
    }

    return;
}

/*
d_functional_partial_consumer_new
  Creates a heap-allocated partial consumer that binds a context value to a
consumer function. When applied, the bound context is passed to the consumer
as its second argument.

Parameter(s):
  _consumer: the consumer function to partially apply.
  _context:  context to bind to the consumer; may be NULL.
Return:
  A pointer to a newly allocated d_partial_consumer, or NULL if _consumer
was NULL or allocation failed.
*/
struct d_partial_consumer*
d_functional_partial_consumer_new
(
    fn_consumer _consumer,
    void*      _context
)
{
    struct d_partial_consumer* result;

    // validate parameters
    if (!_consumer)
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_partial_consumer));

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    result->consumer = _consumer;
    result->context  = _context;

    return result;
}

/*
d_functional_partial_consumer_apply
  Applies a partial consumer to an element, forwarding the bound context.

Parameter(s):
  _partial: pointer to the partial consumer.
  _element: pointer to the element to consume.
Return:
  none.
*/
void
d_functional_partial_consumer_apply
(
    const struct d_partial_consumer* _partial,
    void*                            _element
)
{
    // validate parameters
    if ( (!_partial) ||
         (!_partial->consumer) )
    {
        return;
    }

    _partial->consumer(_element, _partial->context);

    return;
}

/*
d_functional_partial_consumer_free
  Frees a partial consumer. Does not free the bound context.

Parameter(s):
  _partial: pointer to the partial consumer to free; may be NULL.
Return:
  none.
*/
void
d_functional_partial_consumer_free
(
    struct d_partial_consumer* _partial
)
{
    if (_partial)
    {
        free(_partial);
    }

    return;
}
