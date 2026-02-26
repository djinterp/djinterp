#include "../../../../inc/c/container/vector/ptr_vector.h"


// =============================================================================
// constructor functions
// =============================================================================

/*
d_ptr_vector_new
  Creates a new pointer vector with the specified initial capacity.

Parameter(s):
  _initial_capacity: the initial capacity in number of pointer elements
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure, or
  - NULL, if memory allocation failed.
*/
struct d_ptr_vector*
d_ptr_vector_new
(
    size_t _initial_capacity
)
{
    struct d_ptr_vector* result;
    void*                elements;
    size_t               count;
    size_t               capacity;

    result = (struct d_ptr_vector*)malloc(sizeof(struct d_ptr_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init(&elements,
                              &count,
                              &capacity,
                              sizeof(void*),
                              _initial_capacity))
    {
        free(result);

        return NULL;
    }

    result->elements = (void**)elements;
    result->count    = count;
    result->capacity = capacity;

    return result;
}

/*
d_ptr_vector_new_default
  Creates a new pointer vector with the default initial capacity.

Parameter(s):
  none
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure, or
  - NULL, if memory allocation failed.
*/
struct d_ptr_vector*
d_ptr_vector_new_default
(
    void
)
{
    return d_ptr_vector_new(D_VECTOR_DEFAULT_CAPACITY);
}

/*
d_ptr_vector_new_from_array
  Creates a new pointer vector initialized with pointers from a source array.

Parameter(s):
  _source: pointer to array of pointers to copy from
  _count:  number of pointers in the source array
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure containing copies of the source
    pointers, or
  - NULL, if memory allocation failed or parameters are invalid.
*/
struct d_ptr_vector*
d_ptr_vector_new_from_array
(
    const void** _source,
    size_t       _count
)
{
    struct d_ptr_vector* result;
    void*                elements;
    size_t               count;
    size_t               capacity;

    if ( (!_source) &&
         (_count > 0) )
    {
        return NULL;
    }

    result = (struct d_ptr_vector*)malloc(sizeof(struct d_ptr_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init_from_array(&elements,
                                         &count,
                                         &capacity,
                                         sizeof(void*),
                                         _source,
                                         _count))
    {
        free(result);

        return NULL;
    }

    result->elements = (void**)elements;
    result->count    = count;
    result->capacity = capacity;

    return result;
}

/*
d_ptr_vector_new_from_args
  Creates a new pointer vector initialized with variadic pointer arguments.

Parameter(s):
  _arg_count: number of pointer arguments that follow
  ...:        variadic pointer arguments to initialize the vector with
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure, or
  - NULL, if memory allocation failed.
*/
struct d_ptr_vector*
d_ptr_vector_new_from_args
(
    size_t _arg_count,
    ...
)
{
    struct d_ptr_vector* result;
    void*                elements;
    size_t               count;
    size_t               capacity;
    va_list              args;

    result = (struct d_ptr_vector*)malloc(sizeof(struct d_ptr_vector));

    if (!result)
    {
        return NULL;
    }

    va_start(args, _arg_count);

    if (!d_vector_common_init_from_args(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(void*),
                                        _arg_count,
                                        args))
    {
        va_end(args);
        free(result);

        return NULL;
    }

    va_end(args);

    result->elements = (void**)elements;
    result->count    = count;
    result->capacity = capacity;

    return result;
}

/*
d_ptr_vector_new_copy
  Creates a new pointer vector as a shallow copy of an existing pointer vector.

Parameter(s):
  _other: pointer to the source `d_ptr_vector` to copy from
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure containing copies of the source
    pointers, or
  - NULL, if memory allocation failed or _other is NULL.
*/
struct d_ptr_vector*
d_ptr_vector_new_copy
(
    const struct d_ptr_vector* _other
)
{
    struct d_ptr_vector* result;
    void*                elements;
    size_t               count;
    size_t               capacity;

    if (!_other)
    {
        return NULL;
    }

    result = (struct d_ptr_vector*)malloc(sizeof(struct d_ptr_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init_copy(&elements,
                                   &count,
                                   &capacity,
                                   sizeof(void*),
                                   _other->elements,
                                   _other->count,
                                   _other->capacity))
    {
        free(result);

        return NULL;
    }

    result->elements = (void**)elements;
    result->count    = count;
    result->capacity = capacity;

    return result;
}

/*
d_ptr_vector_new_fill
  Creates a new pointer vector filled with a specified pointer value.

Parameter(s):
  _count: number of elements to create
  _value: pointer value to fill the vector with
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure, or
  - NULL, if memory allocation failed.
*/
struct d_ptr_vector*
d_ptr_vector_new_fill
(
    size_t _count,
    void*  _value
)
{
    struct d_ptr_vector* result;
    void*                elements;
    size_t               count;
    size_t               capacity;

    result = (struct d_ptr_vector*)malloc(sizeof(struct d_ptr_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init_fill(&elements,
                                   &count,
                                   &capacity,
                                   sizeof(void*),
                                   _count,
                                   &_value))
    {
        free(result);

        return NULL;
    }

    result->elements = (void**)elements;
    result->count    = count;
    result->capacity = capacity;

    return result;
}

/*
d_ptr_vector_new_merge
  Creates a new pointer vector by merging multiple pointer vectors.

Parameter(s):
  _count: number of `d_ptr_vector` arguments that follow
  ...:    variadic `d_ptr_vector*` arguments to merge together
Return:
  A pointer to either:
  - a newly allocated `d_ptr_vector` structure containing all pointers from all
    input vectors in order, or
  - NULL, if memory allocation failed.
*/
struct d_ptr_vector*
d_ptr_vector_new_merge
(
    size_t _count,
    ...
)
{
    struct d_ptr_vector* result;
    struct d_ptr_vector* current;
    va_list              args;
    size_t               total_count;
    size_t               i;

    if (_count == 0)
    {
        return d_ptr_vector_new_default();
    }

    // first pass: calculate total size needed
    total_count = 0;

    va_start(args, _count);

    for (i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_ptr_vector*);

        if (current)
        {
            total_count += current->count;
        }
    }

    va_end(args);

    result = d_ptr_vector_new(total_count);

    if (!result)
    {
        return NULL;
    }

    // second pass: copy all elements
    va_start(args, _count);

    for (i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_ptr_vector*);

        if ( (current)              &&
             (current->count > 0)   &&
             (current->elements) )
        {
            if (!d_ptr_vector_append(result,
                                     (const void**)current->elements,
                                     current->count))
            {
                va_end(args);
                d_ptr_vector_free(result);

                return NULL;
            }
        }
    }

    va_end(args);

    return result;
}


// =============================================================================
// capacity management functions
// =============================================================================

/*
d_ptr_vector_reserve
  Reserves storage for at least the specified number of pointers.

Parameter(s):
  _ptr_vector:   pointer to the `d_ptr_vector` to modify
  _new_capacity: minimum capacity to reserve
Return:
  A boolean value corresponding to either:
  - true, if reservation was successful, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_reserve
(
    struct d_ptr_vector* _ptr_vector,
    size_t               _new_capacity
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_reserve(&elements,
                                 _ptr_vector->count,
                                 &_ptr_vector->capacity,
                                 sizeof(void*),
                                 _new_capacity))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_shrink_to_fit
  Reduces capacity to match the current count.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to shrink
Return:
  A boolean value corresponding to either:
  - true, if shrink was successful, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_shrink_to_fit
(
    struct d_ptr_vector* _ptr_vector
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_shrink_to_fit(&elements,
                                       _ptr_vector->count,
                                       &_ptr_vector->capacity,
                                       sizeof(void*)))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_ensure_capacity
  Ensures the vector has at least the required capacity.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _required:   minimum capacity required
Return:
  A boolean value corresponding to either:
  - true, if sufficient capacity exists or was allocated, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_ensure_capacity
(
    struct d_ptr_vector* _ptr_vector,
    size_t               _required
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_ensure_capacity(&elements,
                                         _ptr_vector->count,
                                         &_ptr_vector->capacity,
                                         sizeof(void*),
                                         _required))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_available
  Returns the number of pointers that can be added without reallocation.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to query
Return:
  The number of available slots, or 0 if _ptr_vector is NULL.
*/
size_t
d_ptr_vector_available
(
    const struct d_ptr_vector* _ptr_vector
)
{
    if (!_ptr_vector)
    {
        return 0;
    }

    return d_vector_common_available(_ptr_vector->count, _ptr_vector->capacity);
}


// =============================================================================
// element manipulation functions
// =============================================================================

/*
d_ptr_vector_push_back
  Appends a pointer to the end of the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _value:      pointer value to append
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully appended, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_push_back
(
    struct d_ptr_vector* _ptr_vector,
    void*                _value
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_push_back(&elements,
                                   &_ptr_vector->count,
                                   &_ptr_vector->capacity,
                                   sizeof(void*),
                                   &_value))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_push_front
  Prepends a pointer to the beginning of the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _value:      pointer value to prepend
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully prepended, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_push_front
(
    struct d_ptr_vector* _ptr_vector,
    void*                _value
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_push_front(&elements,
                                    &_ptr_vector->count,
                                    &_ptr_vector->capacity,
                                    sizeof(void*),
                                    &_value))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_pop_back
  Removes and returns the last pointer from the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
Return:
  The removed pointer value, or NULL if the vector is empty or NULL.
*/
void*
d_ptr_vector_pop_back
(
    struct d_ptr_vector* _ptr_vector
)
{
    void* result;

    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return NULL;
    }

    result = NULL;

    if (!d_vector_common_pop_back(_ptr_vector->elements,
                                  &_ptr_vector->count,
                                  sizeof(void*),
                                  &result))
    {
        return NULL;
    }

    return result;
}

/*
d_ptr_vector_pop_front
  Removes and returns the first pointer from the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
Return:
  The removed pointer value, or NULL if the vector is empty or NULL.
*/
void*
d_ptr_vector_pop_front
(
    struct d_ptr_vector* _ptr_vector
)
{
    void* result;

    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return NULL;
    }

    result = NULL;

    if (!d_vector_common_pop_front(_ptr_vector->elements,
                                   &_ptr_vector->count,
                                   sizeof(void*),
                                   &result))
    {
        return NULL;
    }

    return result;
}

/*
d_ptr_vector_insert
  Inserts a pointer at the specified index.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _index:      index where to insert (supports negative indexing)
  _value:      pointer value to insert
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully inserted, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_ptr_vector_insert
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _index,
    void*                _value
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_insert_element(&elements,
                                        &_ptr_vector->count,
                                        &_ptr_vector->capacity,
                                        sizeof(void*),
                                        _index,
                                        &_value))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_insert_range
  Inserts multiple pointers at the specified index.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _index:      index where to insert (supports negative indexing)
  _source:     pointer to array of pointers to insert
  _count:      number of pointers to insert
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully inserted, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_ptr_vector_insert_range
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _index,
    const void**         _source,
    size_t               _count
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_insert_elements(&elements,
                                         &_ptr_vector->count,
                                         &_ptr_vector->capacity,
                                         sizeof(void*),
                                         _index,
                                         _source,
                                         _count))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_erase
  Removes the pointer at the specified index without returning it.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _index:      index of pointer to remove (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully removed, or
  - false, if index is invalid or _ptr_vector is NULL.
*/
bool
d_ptr_vector_erase
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _index
)
{
    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_erase(_ptr_vector->elements,
                                 &_ptr_vector->count,
                                 sizeof(void*),
                                 _index);
}

/*
d_ptr_vector_erase_range
  Removes pointers in the specified index range [_start, _end].

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _start:      starting index (supports negative indexing)
  _end:        ending index, inclusive (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully removed, or
  - false, if indices are invalid or _ptr_vector is NULL.
*/
bool
d_ptr_vector_erase_range
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _start,
    d_index              _end
)
{
    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_erase_range(_ptr_vector->elements,
                                       &_ptr_vector->count,
                                       sizeof(void*),
                                       _start,
                                       _end);
}

/*
d_ptr_vector_remove
  Removes and returns the pointer at the specified index.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _index:      index of pointer to remove (supports negative indexing)
Return:
  The removed pointer value, or NULL if index is invalid or _ptr_vector is NULL.
*/
void*
d_ptr_vector_remove
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _index
)
{
    size_t actual_idx;
    void*  result;

    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return NULL;
    }

    // convert negative index to positive
    if (!d_index_convert_safe(_index, _ptr_vector->count, &actual_idx))
    {
        return NULL;
    }

    // save the pointer before erasing
    result = _ptr_vector->elements[actual_idx];

    // erase the element
    if (!d_vector_common_erase(_ptr_vector->elements,
                               &_ptr_vector->count,
                               sizeof(void*),
                               (d_index)actual_idx))
    {
        return NULL;
    }

    return result;
}

/*
d_ptr_vector_clear
  Removes all pointers from the vector without deallocating storage.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to clear
Return:
  none
*/
void
d_ptr_vector_clear
(
    struct d_ptr_vector* _ptr_vector
)
{
    if (_ptr_vector)
    {
        _ptr_vector->count = 0;
    }

    return;
}


// =============================================================================
// append/extend functions
// =============================================================================

/*
d_ptr_vector_append
  Appends multiple pointers to the end of the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _source:     pointer to array of pointers to append
  _count:      number of pointers to append
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_ptr_vector_append
(
    struct d_ptr_vector* _ptr_vector,
    const void**         _source,
    size_t               _count
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_append_elements(&elements,
                                         &_ptr_vector->count,
                                         &_ptr_vector->capacity,
                                         sizeof(void*),
                                         _source,
                                         _count))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_append_vector
  Appends all pointers from a source vector to the destination vector.

Parameter(s):
  _destination: pointer to the destination `d_ptr_vector`
  _source:      pointer to the source `d_ptr_vector` to append from
Return:
  A boolean value corresponding to either:
  - true, if all pointers were successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_ptr_vector_append_vector
(
    struct d_ptr_vector*       _destination,
    const struct d_ptr_vector* _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return D_FAILURE;
    }

    return d_ptr_vector_append(_destination,
                               (const void**)_source->elements,
                               _source->count);
}

/*
d_ptr_vector_prepend
  Prepends multiple pointers to the beginning of the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _source:     pointer to array of pointers to prepend
  _count:      number of pointers to prepend
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_ptr_vector_prepend
(
    struct d_ptr_vector* _ptr_vector,
    const void**         _source,
    size_t               _count
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_prepend_elements(&elements,
                                          &_ptr_vector->count,
                                          &_ptr_vector->capacity,
                                          sizeof(void*),
                                          _source,
                                          _count))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_prepend_vector
  Prepends all pointers from a source vector to the destination vector.

Parameter(s):
  _destination: pointer to the destination `d_ptr_vector`
  _source:      pointer to the source `d_ptr_vector` to prepend from
Return:
  A boolean value corresponding to either:
  - true, if all pointers were successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_ptr_vector_prepend_vector
(
    struct d_ptr_vector*       _destination,
    const struct d_ptr_vector* _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return D_FAILURE;
    }

    return d_ptr_vector_prepend(_destination,
                                (const void**)_source->elements,
                                _source->count);
}


// =============================================================================
// resize functions
// =============================================================================

/*
d_ptr_vector_resize
  Resizes the vector to contain exactly _new_count pointers. New pointers are
initialized to NULL.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to resize
  _new_count:  new number of pointers
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_resize
(
    struct d_ptr_vector* _ptr_vector,
    size_t               _new_count
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_resize(&elements,
                                &_ptr_vector->count,
                                &_ptr_vector->capacity,
                                sizeof(void*),
                                _new_count))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}

/*
d_ptr_vector_resize_fill
  Resizes the vector to contain exactly _new_count pointers. New pointers are
initialized to the specified fill value.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to resize
  _new_count:  new number of pointers
  _fill_value: pointer value to fill new slots with
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if reallocation failed or _ptr_vector is NULL.
*/
bool
d_ptr_vector_resize_fill
(
    struct d_ptr_vector* _ptr_vector,
    size_t               _new_count,
    void*                _fill_value
)
{
    void* elements;

    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    elements = (void*)_ptr_vector->elements;

    if (!d_vector_common_resize_fill(&elements,
                                     &_ptr_vector->count,
                                     &_ptr_vector->capacity,
                                     sizeof(void*),
                                     _new_count,
                                     &_fill_value))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements = (void**)elements;

    return D_SUCCESS;
}


// =============================================================================
// access functions
// =============================================================================

/*
d_ptr_vector_at
  Returns the pointer at the specified index.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to access
  _index:      index of pointer to retrieve (supports negative indexing)
Return:
  The pointer at the specified index, or NULL if index is invalid.
*/
void*
d_ptr_vector_at
(
    const struct d_ptr_vector* _ptr_vector,
    d_index                    _index
)
{
    size_t actual_idx;

    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return NULL;
    }

    if (!d_index_convert_safe(_index, _ptr_vector->count, &actual_idx))
    {
        return NULL;
    }

    return _ptr_vector->elements[actual_idx];
}

/*
d_ptr_vector_front
  Returns the first pointer in the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to access
Return:
  The first pointer, or NULL if the vector is empty or NULL.
*/
void*
d_ptr_vector_front
(
    const struct d_ptr_vector* _ptr_vector
)
{
    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return NULL;
    }

    return _ptr_vector->elements[0];
}

/*
d_ptr_vector_back
  Returns the last pointer in the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to access
Return:
  The last pointer, or NULL if the vector is empty or NULL.
*/
void*
d_ptr_vector_back
(
    const struct d_ptr_vector* _ptr_vector
)
{
    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return NULL;
    }

    return _ptr_vector->elements[_ptr_vector->count - 1];
}

/*
d_ptr_vector_data
  Returns a pointer to the underlying array of pointers.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to access
Return:
  Pointer to the underlying void** array, or NULL if _ptr_vector is NULL.
*/
void**
d_ptr_vector_data
(
    const struct d_ptr_vector* _ptr_vector
)
{
    return _ptr_vector ? _ptr_vector->elements :
                         NULL;
}

/*
d_ptr_vector_get
  Returns the pointer at the specified index (alias for d_ptr_vector_at).

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to access
  _index:      index of pointer to retrieve (supports negative indexing)
Return:
  The pointer at the specified index, or NULL if index is invalid.
*/
void*
d_ptr_vector_get
(
    const struct d_ptr_vector* _ptr_vector,
    d_index                    _index
)
{
    return d_ptr_vector_at(_ptr_vector, _index);
}

/*
d_ptr_vector_set
  Sets the pointer at the specified index to a new value.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _index:      index of pointer to set (supports negative indexing)
  _value:      new pointer value to assign
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully set, or
  - false, if index is invalid or _ptr_vector is NULL.
*/
bool
d_ptr_vector_set
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _index,
    void*                _value
)
{
    size_t actual_idx;

    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return D_FAILURE;
    }

    if (!d_index_convert_safe(_index, _ptr_vector->count, &actual_idx))
    {
        return D_FAILURE;
    }

    _ptr_vector->elements[actual_idx] = _value;

    return D_SUCCESS;
}


// =============================================================================
// query functions
// =============================================================================

/*
d_ptr_vector_is_empty
  Checks if the vector contains no pointers.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to query
Return:
  A boolean value corresponding to either:
  - true, if the vector is empty or NULL, or
  - false, if the vector contains pointers.
*/
bool
d_ptr_vector_is_empty
(
    const struct d_ptr_vector* _ptr_vector
)
{
    return (!_ptr_vector) || (_ptr_vector->count == 0);
}

/*
d_ptr_vector_is_full
  Checks if the vector's count equals its capacity.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to query
Return:
  A boolean value corresponding to either:
  - true, if count equals capacity, or
  - false, if there is available capacity or _ptr_vector is NULL.
*/
bool
d_ptr_vector_is_full
(
    const struct d_ptr_vector* _ptr_vector
)
{
    if (!_ptr_vector)
    {
        return false;
    }

    return d_vector_common_is_full(_ptr_vector->count, _ptr_vector->capacity);
}

/*
d_ptr_vector_size
  Returns the number of pointers in the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to query
Return:
  The number of pointers, or 0 if _ptr_vector is NULL.
*/
size_t
d_ptr_vector_size
(
    const struct d_ptr_vector* _ptr_vector
)
{
    return _ptr_vector ? _ptr_vector->count : 0;
}

/*
d_ptr_vector_capacity
  Returns the current capacity of the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to query
Return:
  The current capacity, or 0 if _ptr_vector is NULL.
*/
size_t
d_ptr_vector_capacity
(
    const struct d_ptr_vector* _ptr_vector
)
{
    return _ptr_vector ? _ptr_vector->capacity : 0;
}


// =============================================================================
// search functions
// =============================================================================

/*
d_ptr_vector_find
  Finds the first occurrence of a value using a comparator function.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to search
  _value:      pointer to value to search for
  _comparator: function to compare pointed-to values
Return:
  Index of first occurrence, or -1 if not found or parameters are invalid.
*/
ssize_t
d_ptr_vector_find
(
    const struct d_ptr_vector* _ptr_vector,
    const void*                _value,
    fn_comparator              _comparator
)
{
    size_t i;

    if ( (!_ptr_vector)  ||
         (!_comparator) )
    {
        return -1;
    }

    for (i = 0; i < _ptr_vector->count; i++)
    {
        if (_comparator(_ptr_vector->elements[i], _value) == 0)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}

/*
d_ptr_vector_find_last
  Finds the last occurrence of a value using a comparator function.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to search
  _value:      pointer to value to search for
  _comparator: function to compare pointed-to values
Return:
  Index of last occurrence, or -1 if not found or parameters are invalid.
*/
ssize_t
d_ptr_vector_find_last
(
    const struct d_ptr_vector* _ptr_vector,
    const void*                _value,
    fn_comparator              _comparator
)
{
    size_t i;

    if ( (!_ptr_vector)            ||
         (!_comparator)            ||
         (_ptr_vector->count == 0) )
    {
        return -1;
    }

    for (i = _ptr_vector->count; i > 0; i--)
    {
        if (_comparator(_ptr_vector->elements[i - 1], _value) == 0)
        {
            return (ssize_t)(i - 1);
        }
    }

    return -1;
}

/*
d_ptr_vector_find_ptr
  Finds the first occurrence of a specific pointer by identity (address).

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to search
  _ptr:        pointer to search for (by identity)
Return:
  Index of first occurrence, or -1 if not found or _ptr_vector is NULL.
*/
ssize_t
d_ptr_vector_find_ptr
(
    const struct d_ptr_vector* _ptr_vector,
    const void*                _ptr
)
{
    size_t i;

    if (!_ptr_vector)
    {
        return -1;
    }

    for (i = 0; i < _ptr_vector->count; i++)
    {
        if (_ptr_vector->elements[i] == _ptr)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}

/*
d_ptr_vector_contains
  Checks if the vector contains a value using a comparator function.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to search
  _value:      pointer to value to search for
  _comparator: function to compare pointed-to values
Return:
  A boolean value corresponding to either:
  - true, if the value was found, or
  - false, if the value was not found or parameters are invalid.
*/
bool
d_ptr_vector_contains
(
    const struct d_ptr_vector* _ptr_vector,
    const void*                _value,
    fn_comparator              _comparator
)
{
    return (d_ptr_vector_find(_ptr_vector, _value, _comparator) != -1);
}

/*
d_ptr_vector_contains_ptr
  Checks if the vector contains a specific pointer by identity (address).

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to search
  _ptr:        pointer to search for (by identity)
Return:
  A boolean value corresponding to either:
  - true, if the pointer was found, or
  - false, if the pointer was not found or _ptr_vector is NULL.
*/
bool
d_ptr_vector_contains_ptr
(
    const struct d_ptr_vector* _ptr_vector,
    const void*                _ptr
)
{
    return (d_ptr_vector_find_ptr(_ptr_vector, _ptr) != -1);
}

/*
d_ptr_vector_count_value
  Counts occurrences of a value using a comparator function.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to search
  _value:      pointer to value to count
  _comparator: function to compare pointed-to values
Return:
  Number of occurrences, or 0 if not found or parameters are invalid.
*/
size_t
d_ptr_vector_count_value
(
    const struct d_ptr_vector* _ptr_vector,
    const void*                _value,
    fn_comparator              _comparator
)
{
    size_t i;
    size_t count;

    if ( (!_ptr_vector) ||
         (!_comparator) )
    {
        return 0;
    }

    count = 0;

    for (i = 0; i < _ptr_vector->count; i++)
    {
        if (_comparator(_ptr_vector->elements[i], _value) == 0)
        {
            count++;
        }
    }

    return count;
}


// =============================================================================
// utility functions
// =============================================================================

/*
d_ptr_vector_swap
  Swaps two pointers in the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to modify
  _index_a:    index of first pointer (supports negative indexing)
  _index_b:    index of second pointer (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if swap was successful, or
  - false, if indices are invalid or _ptr_vector is NULL.
*/
bool
d_ptr_vector_swap
(
    struct d_ptr_vector* _ptr_vector,
    d_index              _index_a,
    d_index              _index_b
)
{
    size_t idx_a;
    size_t idx_b;
    void*  temp;

    if ( (!_ptr_vector)            ||
         (_ptr_vector->count == 0) )
    {
        return D_FAILURE;
    }

    if ( (!d_index_convert_safe(_index_a, _ptr_vector->count, &idx_a)) ||
         (!d_index_convert_safe(_index_b, _ptr_vector->count, &idx_b)) )
    {
        return D_FAILURE;
    }

    // same index: no swap needed
    if (idx_a == idx_b)
    {
        return D_SUCCESS;
    }

    temp = _ptr_vector->elements[idx_a];
    _ptr_vector->elements[idx_a] = _ptr_vector->elements[idx_b];
    _ptr_vector->elements[idx_b] = temp;

    return D_SUCCESS;
}

/*
d_ptr_vector_reverse
  Reverses the order of pointers in the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to reverse
Return:
  A boolean value corresponding to either:
  - true, if reverse was successful, or
  - false, if _ptr_vector is NULL.
*/
bool
d_ptr_vector_reverse
(
    struct d_ptr_vector* _ptr_vector
)
{
    if (!_ptr_vector)
    {
        return D_FAILURE;
    }

    return d_array_common_reverse(_ptr_vector->elements,
                                  _ptr_vector->count,
                                  sizeof(void*));
}

/*
d_ptr_vector_sort
  Sorts the pointers in the vector using the provided comparator.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to sort
  _comparator: function to compare pointed-to values
Return:
  none
*/
void
d_ptr_vector_sort
(
    struct d_ptr_vector* _ptr_vector,
    fn_comparator        _comparator
)
{
    if ( (!_ptr_vector) ||
         (!_comparator) )
    {
        return;
    }

    d_array_common_sort(_ptr_vector->elements,
                        _ptr_vector->count,
                        sizeof(void*),
                        _comparator);

    return;
}

/*
d_ptr_vector_copy_to
  Copies the vector's pointers to a destination buffer.

Parameter(s):
  _ptr_vector:    pointer to the source `d_ptr_vector`
  _destination:   pointer to destination buffer
  _dest_capacity: capacity of destination buffer (in pointers)
Return:
  A boolean value corresponding to either:
  - true, if copy was successful, or
  - false, if destination is too small or parameters are invalid.
*/
bool
d_ptr_vector_copy_to
(
    const struct d_ptr_vector* _ptr_vector,
    void**                     _destination,
    size_t                     _dest_capacity
)
{
    if ( (!_ptr_vector)  ||
         (!_destination) )
    {
        return D_FAILURE;
    }

    return d_vector_common_copy_to(_ptr_vector->elements,
                                   _ptr_vector->count,
                                   sizeof(void*),
                                   _destination,
                                   _dest_capacity);
}


// =============================================================================
// iteration helpers
// =============================================================================

/*
d_ptr_vector_foreach
  Applies a function to each pointer in the vector.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to iterate
  _apply_fn:   function to apply to each pointer
Return:
  none
*/
void
d_ptr_vector_foreach
(
    struct d_ptr_vector* _ptr_vector,
    fn_apply             _apply_fn
)
{
    size_t i;

    if ( (!_ptr_vector) ||
         (!_apply_fn) )
    {
        return;
    }

    for (i = 0; i < _ptr_vector->count; i++)
    {
        _apply_fn(_ptr_vector->elements[i]);
    }

    return;
}


// =============================================================================
// destructor functions
// =============================================================================

/*
d_ptr_vector_free
  Deallocates a pointer vector and its internal storage. Does not free the
pointed-to objects.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to deallocate
Return:
  none
*/
void
d_ptr_vector_free
(
    struct d_ptr_vector* _ptr_vector
)
{
    if (_ptr_vector)
    {
        if (_ptr_vector->elements)
        {
            free(_ptr_vector->elements);
        }

        free(_ptr_vector);
    }

    return;
}

/*
d_ptr_vector_free_deep
  Deallocates a pointer vector, its internal storage, and all pointed-to
objects using the provided free function.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to deallocate
  _free_fn:    function to use for freeing each pointed-to object
Return:
  none
*/
void
d_ptr_vector_free_deep
(
    struct d_ptr_vector* _ptr_vector,
    fn_free              _free_fn
)
{
    size_t i;

    if ( (_ptr_vector) &&
         (_free_fn) )
    {
        if (_ptr_vector->elements)
        {
            for (i = 0; i < _ptr_vector->count; i++)
            {
                if (_ptr_vector->elements[i])
                {
                    _free_fn(_ptr_vector->elements[i]);
                }
            }

            free(_ptr_vector->elements);
        }

        free(_ptr_vector);
    }

    return;
}

/*
d_ptr_vector_clear_deep
  Clears the vector and frees all pointed-to objects using the provided free
function. The vector itself remains valid and can be reused.

Parameter(s):
  _ptr_vector: pointer to the `d_ptr_vector` to clear
  _free_fn:    function to use for freeing each pointed-to object
Return:
  none
*/
void
d_ptr_vector_clear_deep
(
    struct d_ptr_vector* _ptr_vector,
    fn_free              _free_fn
)
{
    size_t i;

    if ( (_ptr_vector) &&
         (_free_fn) )
    {
        if (_ptr_vector->elements)
        {
            for (i = 0; i < _ptr_vector->count; i++)
            {
                if (_ptr_vector->elements[i])
                {
                    _free_fn(_ptr_vector->elements[i]);

                    _ptr_vector->elements[i] = NULL;
                }
            }
        }

        _ptr_vector->count = 0;
    }

    return;
}