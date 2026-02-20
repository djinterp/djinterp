#include "..\..\inc\functional\fn_builder.h"


/*
d_fn_builder_new
  Creates a new fluent function chain builder with pre-allocated capacity
for transformers and predicates.

Parameter(s):
  (none)
Return:
  A pointer to a newly allocated d_fn_builder, or NULL if allocation
fails.
*/
struct d_fn_builder*
d_fn_builder_new
(
    void
)
{
    struct d_fn_builder* builder;

    builder = malloc(sizeof(struct d_fn_builder));

    // ensure that memory allocation was successful
    if (!builder)
    {
        return NULL;
    }

    builder->transforms      = NULL;
    builder->predicates      = NULL;
    builder->transform_count = 0;
    builder->predicate_count = 0;
    builder->capacity        = 0;

    // pre-allocate arrays
    builder->transforms = malloc(D_FN_BUILDER_INITIAL_CAPACITY * 
                                 sizeof(fn_transformer));

    if (!builder->transforms)
    {
        free(builder);

        return NULL;
    }

    builder->predicates = malloc(D_FN_BUILDER_INITIAL_CAPACITY *
                                 sizeof(fn_predicate));

    if (!builder->predicates)
    {
        free(builder->transforms);
        free(builder);

        return NULL;
    }

    builder->capacity = D_FN_BUILDER_INITIAL_CAPACITY;

    return builder;
}

/*
d_fn_builder_grow
  Internal helper to grow the builder's arrays when capacity is exceeded.

Parameter(s):
  _builder:   the builder to grow.
  _min_count: the minimum number of slots needed.
Return:
  A boolean value corresponding to either:
  - true, if the builder has sufficient capacity, or
  - false, if reallocation failed.
*/
static bool
d_fn_builder_grow
(
    struct d_fn_builder* _builder,
    size_t               _min_count
)
{
    fn_transformer* new_transforms;
    fn_predicate*   new_predicates;
    size_t         new_capacity;

    if (_min_count <= _builder->capacity)
    {
        return true;
    }

    new_capacity = _builder->capacity * 2;

    if (new_capacity < _min_count)
    {
        new_capacity = _min_count;
    }

    new_transforms = (fn_transformer*)realloc(
                         _builder->transforms,
                         new_capacity * sizeof(fn_transformer));

    if (!new_transforms)
    {
        return false;
    }

    _builder->transforms = new_transforms;

    new_predicates = (fn_predicate*)realloc(
                         _builder->predicates,
                         new_capacity * sizeof(fn_predicate));

    if (!new_predicates)
    {
        return false;
    }

    _builder->predicates = new_predicates;
    _builder->capacity   = new_capacity;

    return true;
}

/*
d_funtional_builder_map
  Appends a transformer to the builder's transform chain.

Parameter(s):
  _builder:   the builder to add to.
  _transform: the transformer function to append.
Return:
  The builder pointer for chaining, or NULL if _builder is NULL,
_transform is NULL, or allocation fails.
*/
struct d_fn_builder*
d_funtional_builder_map
(
    struct d_fn_builder* _builder,
    fn_transformer        _transform
)
{
    // validate parameters
    if ( (!_builder)   ||
         (!_transform) )
    {
        return NULL;
    }

    // grow if needed
    if (!d_fn_builder_grow(_builder,
                           _builder->transform_count + 1))
    {
        return NULL;
    }

    _builder->transforms[_builder->transform_count] = _transform;
    _builder->transform_count++;

    return _builder;
}

/*
d_funtional_builder_filter
  Appends a predicate to the builder's predicate chain.

Parameter(s):
  _builder: the builder to add to.
  _test:    the predicate function to append.
Return:
  The builder pointer for chaining, or NULL if _builder is NULL,
_test is NULL, or allocation fails.
*/
struct d_fn_builder*
d_funtional_builder_filter
(
    struct d_fn_builder* _builder,
    fn_predicate          _test
)
{
    // validate parameters
    if ( (!_builder) ||
         (!_test) )
    {
        return NULL;
    }

    // grow if needed
    if (!d_fn_builder_grow(_builder,
                           _builder->predicate_count + 1))
    {
        return NULL;
    }

    _builder->predicates[_builder->predicate_count] = _test;
    _builder->predicate_count++;

    return _builder;
}

/*
d_funtional_builder_and_then
  Appends a transformer to the builder's transform chain. Alias for
d_funtional_builder_map provided for readability in sequential chains.

Parameter(s):
  _builder:   the builder to add to.
  _transform: the transformer function to append.
Return:
  The builder pointer for chaining, or NULL if _builder is NULL,
_transform is NULL, or allocation fails.
*/
struct d_fn_builder*
d_funtional_builder_and_then
(
    struct d_fn_builder* _builder,
    fn_transformer        _transform
)
{
    return d_funtional_builder_map(_builder, _transform);
}

/*
d_funtional_builder_where
  Appends a predicate to the builder's predicate chain. Alias for
d_funtional_builder_filter provided for readability in query-style chains.

Parameter(s):
  _builder: the builder to add to.
  _test:    the predicate function to append.
Return:
  The builder pointer for chaining, or NULL if _builder is NULL,
_test is NULL, or allocation fails.
*/
struct d_fn_builder*
d_funtional_builder_where
(
    struct d_fn_builder* _builder,
    fn_predicate          _test
)
{
    return d_funtional_builder_filter(_builder, _test);
}

/*
d_fn_builder_execute
  Executes the accumulated function chain on an input array. First applies
all transformers in order to each element, then filters the transformed
results using all predicates (conjunction). Elements passing all predicates
are packed contiguously into the output.

Parameter(s):
  _builder:      the builder containing the function chain.
  _input:        pointer to the input array.
  _count:        number of elements in the input array.
  _element_size: size of each element in bytes.
  _output:       pointer to the output array; must be at least
                 _count * _element_size bytes.
  _out_count:    pointer to receive the number of output elements.
Return:
  A boolean value corresponding to either:
  - true, if execution completed successfully, or
  - false, if any parameter was invalid, allocation failed, or a
    transformation failed.
*/
bool
d_fn_builder_execute
(
    const struct d_fn_builder* _builder,
    const void*                _input,
    size_t                     _count,
    size_t                     _element_size,
    void*                      _output,
    size_t*                    _out_count
)
{
    unsigned char*       temp_a;
    unsigned char*       temp_b;
    unsigned char*       src;
    unsigned char*       destination;
    const unsigned char* in_bytes;
    unsigned char*       out_bytes;
    size_t               out_count;
    size_t               i;
    size_t               t;
    bool                 passes;

    // validate parameters
    if ( (!_builder)   ||
         (!_input)     ||
         (!_output)    ||
         (!_out_count) ||
         (_count == 0) ||
         (_element_size == 0) )
    {
        if (_out_count)
        {
            *(_out_count) = 0;
        }

        return false;
    }

    in_bytes  = (const unsigned char*)_input;
    out_bytes = (unsigned char*)_output;
    out_count = 0;

    // no transforms and no predicates: copy input to output
    if ( (_builder->transform_count == 0) &&
         (_builder->predicate_count == 0) )
    {
        memcpy(_output, _input, _count * _element_size);
        *(_out_count) = _count;

        return true;
    }

    // allocate temp buffers for transform ping-pong
    temp_a      = NULL;
    temp_b      = NULL;
    src         = NULL;
    destination = NULL;

    if (_builder->transform_count > 0)
    {
        temp_a = malloc(_element_size);

        if (!temp_a)
        {
            *(_out_count) = 0;

            return false;
        }

        if (_builder->transform_count > 1)
        {
            temp_b = malloc(_element_size);

            if (!temp_b)
            {
                free(temp_a);

                *(_out_count) = 0;

                return false;
            }
        }
    }

    // process each element
    for (i = 0; i < _count; i++)
    {
        // apply transform chain
        if (_builder->transform_count > 0)
        {
            // first transform: input -> temp_a
            memset(temp_a, 0, _element_size);

            if (!_builder->transforms[0](
                    in_bytes + (i * _element_size),
                    temp_a,
                    NULL))
            {
                free(temp_a);

                if (temp_b)
                {
                    free(temp_b);
                }

                *(_out_count) = 0;

                return false;
            }

            // subsequent transforms: ping-pong between temp buffers
            src = temp_a;
            destination = temp_b;

            for (t = 1; t < _builder->transform_count; t++)
            {
                if (destination)
                {
                    memset(destination, 0, _element_size);
                }

                if (!_builder->transforms[t](src, destination, NULL))
                {
                    free(temp_a);

                    if (temp_b)
                    {
                        free(temp_b);
                    }

                    *(_out_count) = 0;

                    return false;
                }

                // swap src and destination for next iteration
                if (destination)
                {
                    unsigned char* swap;

                    swap = src;
                    src  = destination;
                    destination  = swap;
                }
            }

            // src now points to the final transformed result
        }

        // apply predicate chain (conjunction)
        passes = true;

        if (_builder->predicate_count > 0)
        {
            const unsigned char* test_element;

            // use transformed result if transforms were applied
            if (_builder->transform_count > 0)
            {
                test_element = src;
            }
            else
            {
                test_element = in_bytes + (i * _element_size);
            }

            for (t = 0; t < _builder->predicate_count; t++)
            {
                if (!_builder->predicates[t](test_element, NULL))
                {
                    passes = false;
                    break;
                }
            }
        }

        // copy to output if element passes all predicates
        if (passes)
        {
            if (_builder->transform_count > 0)
            {
                memcpy(out_bytes + (out_count * _element_size),
                       src,
                       _element_size);
            }
            else
            {
                memcpy(out_bytes + (out_count * _element_size),
                       in_bytes + (i * _element_size),
                       _element_size);
            }

            out_count++;
        }
    }

    // cleanup
    if (temp_a)
    {
        free(temp_a);
    }

    if (temp_b)
    {
        free(temp_b);
    }

    *(_out_count) = out_count;

    return true;
}

/*
d_fn_builder_free
  Frees all resources owned by a function chain builder.

Parameter(s):
  _builder: the builder to free; may be NULL.
Return:
  none.
*/
void
d_fn_builder_free
(
    struct d_fn_builder* _builder
)
{
    if (!_builder)
    {
        return;
    }

    if (_builder->transforms)
    {
        free(_builder->transforms);
    }

    if (_builder->predicates)
    {
        free(_builder->predicates);
    }

    free(_builder);

    return;
}