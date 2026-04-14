#include "../../../../inc/c/container/vector/vector.h"


// =============================================================================
// constructor functions
// =============================================================================

/*
d_vector_new
  Creates a new generic `d_vector` with the specified element size and initial
capacity.

Parameter(s):
  _element_size:     the size, in bytes, of each individual element.
  _initial_capacity: the initial capacity in number of elements.
Return:
  A pointer to either:
  - a newly allocated `d_vector` structure, or
  - NULL, if memory allocation failed or _element_size is 0.
*/
struct d_vector*
d_vector_new
(
    size_t _element_size,
    size_t _initial_capacity
)
{
    struct d_vector* result;
    void*            elements;
    size_t           count;
    size_t           capacity;

    if (!_element_size)
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init(&elements,
                              &count,
                              &capacity,
                              _element_size,
                              _initial_capacity))
    {
        free(result);

        return NULL;
    }

    result->elements     = elements;
    result->element_size = _element_size;
    result->count        = count;
    result->capacity     = capacity;

    return result;
}

/*
d_vector_new_default
  Creates a new generic vector with the specified element size and default
initial capacity.

Parameter(s):
  _element_size: size in bytes of each element
Return:
  A pointer to either:
  - a newly allocated `d_vector` structure, or
  - NULL, if memory allocation failed or _element_size is 0.
*/
D_INLINE struct d_vector*
d_vector_new_default
(
    size_t _element_size
)
{
    return d_vector_new(_element_size, D_VECTOR_DEFAULT_CAPACITY);
}

/*
d_vector_new_from_array
  Creates a new generic vector initialized with elements from a source array.

Parameter(s):
  _element_size: size in bytes of each element
  _source:       pointer to array of elements to copy from
  _count:        number of elements in the source array
Return:
  A pointer to either:
  - a newly allocated `d_vector` structure containing copies of the source
    elements, or
  - NULL, if memory allocation failed or parameters are invalid.
*/
struct d_vector*
d_vector_new_from_array
(
    size_t      _element_size,
    const void* _source,
    size_t      _count
)
{
    struct d_vector* result;
    void*            elements;
    size_t           count;
    size_t           capacity;

    if (!_element_size)
    {
        return NULL;
    }

    if ( (!_source) &&
         (_count > 0) )
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init_from_array(&elements,
                                         &count,
                                         &capacity,
                                         _element_size,
                                         _source,
                                         _count))
    {
        free(result);

        return NULL;
    }

    result->elements     = elements;
    result->element_size = _element_size;
    result->count        = count;
    result->capacity     = capacity;

    return result;
}

/*
d_vector_new_from_args
  Creates a new generic vector initialized with variadic arguments.

Parameter(s):
  _element_size: size in bytes of each element
  _arg_count:    number of arguments that follow
  ...:           variadic arguments to initialize the vector with
Return:
  A pointer to either:
  - a newly allocated `d_vector` structure, or
  - NULL, if memory allocation failed or _element_size is 0.
Note:
  For elements larger than sizeof(void*), pass pointers to the data.
*/
struct d_vector*
d_vector_new_from_args
(
    size_t _element_size,
    size_t _arg_count,
    ...
)
{
    struct d_vector* result;
    void*            elements;
    size_t           count;
    size_t           capacity;
    va_list          args;

    if (!_element_size)
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_vector));

    if (!result)
    {
        return NULL;
    }

    va_start(args, _arg_count);

    if (!d_vector_common_init_from_args(&elements,
                                        &count,
                                        &capacity,
                                        _element_size,
                                        _arg_count,
                                        args))
    {
        va_end(args);
        free(result);

        return NULL;
    }

    va_end(args);

    result->elements     = elements;
    result->element_size = _element_size;
    result->count        = count;
    result->capacity     = capacity;

    return result;
}

/*
d_vector_new_copy
  Creates a new generic vector as a copy of an existing vector.

Parameter(s):
  _other: pointer to the source `d_vector` to copy from
Return:
  A pointer to either:
  - a newly allocated `d_vector` structure containing copies of the source
    elements, or
  - NULL, if memory allocation failed or _other is NULL.
*/
struct d_vector*
d_vector_new_copy
(
    const struct d_vector* _other
)
{
    struct d_vector* result;
    void*            elements;
    size_t           count;
    size_t           capacity;

    if (!_other)
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init_copy(&elements,
                                   &count,
                                   &capacity,
                                   _other->element_size,
                                   _other->elements,
                                   _other->count,
                                   _other->capacity))
    {
        free(result);

        return NULL;
    }

    result->elements     = elements;
    result->element_size = _other->element_size;
    result->count        = count;
    result->capacity     = capacity;

    return result;
}

/*
d_vector_new_fill
  Creates a new generic vector filled with a specified value.

Parameter(s):
  _element_size: size in bytes of each element
  _count:        number of elements to create
  _value:        pointer to value to fill the vector with
Return:
  A pointer to either:
  - a newly allocated `d_vector` structure, or
  - NULL, if memory allocation failed or parameters are invalid.
*/
struct d_vector*
d_vector_new_fill
(
    size_t      _element_size,
    size_t      _count,
    const void* _value
)
{
    struct d_vector* result;
    void*            elements;
    size_t           count;
    size_t           capacity;

    if (!_element_size)
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_vector));

    if (!result)
    {
        return NULL;
    }

    if (!d_vector_common_init_fill(&elements,
                                   &count,
                                   &capacity,
                                   _element_size,
                                   _count,
                                   _value))
    {
        free(result);

        return NULL;
    }

    result->elements     = elements;
    result->element_size = _element_size;
    result->count        = count;
    result->capacity     = capacity;

    return result;
}


// =============================================================================
// capacity management functions
// =============================================================================

/*
d_vector_reserve
  Reserves storage for at least the specified number of elements.

Parameter(s):
  _vector:       pointer to the `d_vector` to modify
  _new_capacity: minimum capacity to reserve
Return:
  A boolean value corresponding to either:
  - true, if reservation was successful, or
  - false, if reallocation failed or _vector is NULL.
*/
D_INLINE bool
d_vector_reserve
(
    struct d_vector* _vector,
    size_t           _new_capacity
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_reserve(&_vector->elements,
                                   _vector->count,
                                   &_vector->capacity,
                                   _vector->element_size,
                                   _new_capacity);
}

/*
d_vector_shrink_to_fit
  Reduces capacity to match the current count.

Parameter(s):
  _vector: pointer to the `d_vector` to shrink
Return:
  A boolean value corresponding to either:
  - true, if shrink was successful, or
  - false, if reallocation failed or _vector is NULL.
*/
D_INLINE bool
d_vector_shrink_to_fit
(
    struct d_vector* _vector
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_shrink_to_fit(&_vector->elements,
                                         _vector->count,
                                         &_vector->capacity,
                                         _vector->element_size);
}

/*
d_vector_ensure_capacity
  Ensures the vector has at least the required capacity.

Parameter(s):
  _vector:   pointer to the `d_vector` to modify
  _required: minimum capacity required
Return:
  A boolean value corresponding to either:
  - true, if sufficient capacity exists or was allocated, or
  - false, if reallocation failed or _vector is NULL.
*/
D_INLINE bool
d_vector_ensure_capacity
(
    struct d_vector* _vector,
    size_t           _required
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_ensure_capacity(&_vector->elements,
                                           _vector->count,
                                           &_vector->capacity,
                                           _vector->element_size,
                                           _required);
}

/*
d_vector_grow
  Grows the vector's capacity using the default growth factor.

Parameter(s):
  _vector: pointer to the `d_vector` to grow
Return:
  A boolean value corresponding to either:
  - true, if growth was successful, or
  - false, if reallocation failed or _vector is NULL.
*/
D_INLINE bool
d_vector_grow
(
    struct d_vector* _vector
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_grow(&_vector->elements,
                                _vector->count,
                                &_vector->capacity,
                                _vector->element_size);
}

/*
d_vector_maybe_shrink
  Shrinks the vector's capacity if usage is below the shrink threshold.

Parameter(s):
  _vector: pointer to the `d_vector` to potentially shrink
Return:
  A boolean value corresponding to either:
  - true, if operation completed successfully (shrink may or may not occur), or
  - false, if _vector is NULL.
*/
D_INLINE bool
d_vector_maybe_shrink
(
    struct d_vector* _vector
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_maybe_shrink(&_vector->elements,
                                        _vector->count,
                                        &_vector->capacity,
                                        _vector->element_size);
}

/*
d_vector_available
  Returns the number of elements that can be added without reallocation.

Parameter(s):
  _vector: pointer to the `d_vector` to query
Return:
  The number of available slots, or 0 if _vector is NULL.
*/
D_INLINE size_t
d_vector_available
(
    const struct d_vector* _vector
)
{
    if (!_vector)
    {
        return 0;
    }

    return d_vector_common_available(_vector->count, _vector->capacity);
}


// =============================================================================
// element manipulation functions
// =============================================================================

/*
d_vector_push_back
  Appends an element to the end of the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _value:  pointer to element value to append
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_push_back
(
    struct d_vector* _vector,
    const void*      _value
)
{
    if ( (!_vector) ||
         (!_value) )
    {
        return D_FAILURE;
    }

    return d_vector_common_push_back(&_vector->elements,
                                     &_vector->count,
                                     &_vector->capacity,
                                     _vector->element_size,
                                     _value);
}

/*
d_vector_push_front
  Prepends an element to the beginning of the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _value:  pointer to element value to prepend
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_push_front
(
    struct d_vector* _vector,
    const void*      _value
)
{
    if ( (!_vector) ||
         (!_value) )
    {
        return D_FAILURE;
    }

    return d_vector_common_push_front(&_vector->elements,
                                      &_vector->count,
                                      &_vector->capacity,
                                      _vector->element_size,
                                      _value);
}

/*
d_vector_pop_back
  Removes and optionally returns the last element from the vector.

Parameter(s):
  _vector:    pointer to the `d_vector` to modify
  _out_value: optional pointer to receive the removed element (may be NULL)
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully removed, or
  - false, if the vector is empty or _vector is NULL.
*/
D_INLINE bool
d_vector_pop_back
(
    struct d_vector* _vector,
    void*            _out_value
)
{
    if ( (!_vector) ||
         (_vector->count == 0) )
    {
        return D_FAILURE;
    }

    return d_vector_common_pop_back(_vector->elements,
                                    &_vector->count,
                                    _vector->element_size,
                                    _out_value);
}

/*
d_vector_pop_front
  Removes and optionally returns the first element from the vector.

Parameter(s):
  _vector:    pointer to the `d_vector` to modify
  _out_value: optional pointer to receive the removed element (may be NULL)
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully removed, or
  - false, if the vector is empty or _vector is NULL.
*/
D_INLINE bool
d_vector_pop_front
(
    struct d_vector* _vector,
    void*            _out_value
)
{
    if ( (!_vector)            ||
         (_vector->count == 0) )
    {
        return D_FAILURE;
    }

    return d_vector_common_pop_front(_vector->elements,
                                     &_vector->count,
                                     _vector->element_size,
                                     _out_value);
}

/*
d_vector_insert_element
  Inserts an element at the specified index.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _index:  index where to insert (supports negative indexing)
  _value:  pointer to element value to insert
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully inserted, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_insert_element
(
    struct d_vector* _vector,
    d_index          _index,
    const void*      _value
)
{
    if ( (!_vector) ||
         (!_value) )
    {
        return D_FAILURE;
    }

    return d_vector_common_insert_element(&_vector->elements,
                                          &_vector->count,
                                          &_vector->capacity,
                                          _vector->element_size,
                                          _index,
                                          _value);
}

/*
d_vector_insert_elements
  Inserts multiple elements at the specified index.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _index:  index where to insert (supports negative indexing)
  _source: pointer to array of elements to insert
  _count:  number of elements to insert
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully inserted, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_insert_elements
(
    struct d_vector* _vector,
    d_index          _index,
    const void*      _source,
    size_t           _count
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_insert_elements(&(_vector->elements),
                                           &(_vector->count),
                                           &(_vector->capacity),
                                           _vector->element_size,
                                           _index,
                                           _source,
                                           _count);
}

/*
d_vector_erase
  Removes the element at the specified index.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _index:  index of element to remove (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully removed, or
  - false, if index is invalid or _vector is NULL.
*/
D_INLINE bool
d_vector_erase
(
    struct d_vector* _vector,
    d_index          _index
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_erase(_vector->elements,
                                 &_vector->count,
                                 _vector->element_size,
                                 _index);
}

/*
d_vector_erase_range
  Removes elements in the specified index range [_start, _end].

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _start:  starting index (supports negative indexing)
  _end:    ending index, inclusive (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully removed, or
  - false, if indices are invalid or _vector is NULL.
*/
D_INLINE bool
d_vector_erase_range
(
    struct d_vector* _vector,
    d_index          _start,
    d_index          _end
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_erase_range(_vector->elements,
                                       &_vector->count,
                                       _vector->element_size,
                                       _start,
                                       _end);
}

/*
d_vector_clear
  Removes all elements from the vector without deallocating storage.

Parameter(s):
  _vector: pointer to the `d_vector` to clear
Return:
  none
*/
D_INLINE void
d_vector_clear
(
    struct d_vector* _vector
)
{
    if (_vector)
    {
        _vector->count = 0;
    }

    return;
}


// =============================================================================
// append/prepend functions
// =============================================================================

/*
d_vector_append_element
  Appends a single element to the end of the vector.

Parameter(s):
  _vector:  pointer to the `d_vector` to modify
  _element: pointer to element to append
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_append_element
(
    struct d_vector* _vector,
    const void*      _element
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_append_element(&_vector->elements,
                                          &_vector->count,
                                          &_vector->capacity,
                                          _vector->element_size,
                                          _element);
}

/*
d_vector_append_elements
  Appends multiple elements to the end of the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _source: pointer to array of elements to append
  _count:  number of elements to append
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_append_elements
(
    struct d_vector* _vector,
    const void*      _source,
    size_t           _count
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_append_elements(&(_vector->elements),
                                           &(_vector->count),
                                           &(_vector->capacity),
                                           _vector->element_size,
                                           _source,
                                           _count);
}

/*
d_vector_append_vector
  Appends all elements from a source vector to the destination vector.

Parameter(s):
  _destination: pointer to the destination `d_vector`
  _source:      pointer to the source `d_vector` to append from
Return:
  A boolean value corresponding to either:
  - true, if all elements were successfully appended, or
  - false, if reallocation failed, parameters are invalid, or element sizes
    don't match.
*/
D_INLINE bool
d_vector_append_vector
(
    struct d_vector*       _destination,
    const struct d_vector* _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return D_FAILURE;
    }

    if (_destination->element_size != _source->element_size)
    {
        return D_FAILURE;
    }

    return d_vector_append_elements(_destination,
                                    _source->elements,
                                    _source->count);
}

/*
d_vector_prepend_element
  Prepends a single element to the beginning of the vector.

Parameter(s):
  _vector:  pointer to the `d_vector` to modify
  _element: pointer to element to prepend
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_prepend_element
(
    struct d_vector* _vector,
    const void*      _element
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_prepend_element(&(_vector->elements),
                                           &(_vector->count),
                                           &(_vector->capacity),
                                           _vector->element_size,
                                           _element);
}

/*
d_vector_prepend_elements
  Prepends multiple elements to the beginning of the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _source: pointer to array of elements to prepend
  _count:  number of elements to prepend
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_prepend_elements
(
    struct d_vector* _vector,
    const void*      _source,
    size_t           _count
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_prepend_elements(&(_vector->elements),
                                            &(_vector->count),
                                            &(_vector->capacity),
                                            _vector->element_size,
                                            _source,
                                            _count);
}

/*
d_vector_prepend_vector
  Prepends all elements from a source vector to the destination vector.

Parameter(s):
  _destination: pointer to the destination `d_vector`
  _source:      pointer to the source `d_vector` to prepend from
Return:
  A boolean value corresponding to either:
  - true, if all elements were successfully prepended, or
  - false, if reallocation failed, parameters are invalid, or element sizes
    don't match.
*/
D_INLINE bool
d_vector_prepend_vector
(
    struct d_vector*       _destination,
    const struct d_vector* _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return D_FAILURE;
    }

    if (_destination->element_size != _source->element_size)
    {
        return D_FAILURE;
    }

    return d_vector_prepend_elements(_destination,
                                     _source->elements,
                                     _source->count);
}


// =============================================================================
// resize functions
// =============================================================================

/*
d_vector_resize
  Resizes the vector to contain exactly _new_count elements. New elements are
zero-initialized.

Parameter(s):
  _vector:    pointer to the `d_vector` to resize
  _new_count: new number of elements
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if reallocation failed or _vector is NULL.
*/
D_INLINE bool
d_vector_resize
(
    struct d_vector* _vector,
    size_t           _new_count
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_resize(&_vector->elements,
                                  &_vector->count,
                                  &_vector->capacity,
                                  _vector->element_size,
                                  _new_count);
}

/*
d_vector_resize_fill
  Resizes the vector to contain exactly _new_count elements. New elements are
initialized to the specified fill value.

Parameter(s):
  _vector:     pointer to the `d_vector` to resize
  _new_count:  new number of elements
  _fill_value: pointer to value to fill new elements with
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_resize_fill
(
    struct d_vector* _vector,
    size_t           _new_count,
    const void*      _fill_value
)
{
    if (!_vector)
    {
        return D_FAILURE;
    }

    return d_vector_common_resize_fill(&_vector->elements,
                                       &_vector->count,
                                       &_vector->capacity,
                                       _vector->element_size,
                                       _new_count,
                                       _fill_value);
}


// =============================================================================
// access functions
// =============================================================================

/*
d_vector_at
  Returns a pointer to the element at the specified index.

Parameter(s):
  _vector: pointer to the `d_vector` to access
  _index:  index of element to retrieve (supports negative indexing)
Return:
  Pointer to the element, or NULL if index is invalid or _vector is NULL.
*/
D_INLINE void*
d_vector_at
(
    const struct d_vector* _vector,
    d_index                _index
)
{
    if ( (!_vector)            ||
         (_vector->count == 0) )
    {
        return NULL;
    }

    return d_vector_common_at(_vector->elements,
                              _vector->count,
                              _vector->element_size,
                              _index);
}

/*
d_vector_front
  Returns a pointer to the first element in the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to access
Return:
  Pointer to the first element, or NULL if the vector is empty or NULL.
*/
D_INLINE void*
d_vector_front
(
    const struct d_vector* _vector
)
{
    if ( (!_vector)            ||
         (_vector->count == 0) )
    {
        return NULL;
    }

    return d_vector_common_front(_vector->elements, _vector->count);
}

/*
d_vector_back
  Returns a pointer to the last element in the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to access
Return:
  Pointer to the last element, or NULL if the vector is empty or NULL.
*/
D_INLINE void*
d_vector_back
(
    const struct d_vector* _vector
)
{
    if ( (!_vector)            ||
         (_vector->count == 0) )
    {
        return NULL;
    }

    return d_vector_common_back(_vector->elements,
                                _vector->count,
                                _vector->element_size);
}

/*
d_vector_data
  Returns a pointer to the underlying array data.

Parameter(s):
  _vector: pointer to the `d_vector` to access
Return:
  Pointer to the underlying array, or NULL if _vector is NULL.
*/
D_INLINE void*
d_vector_data
(
    const struct d_vector* _vector
)
{
    if (!_vector)
    {
        return NULL;
    }

    return d_vector_common_data(_vector->elements);
}

/*
d_vector_get
  Copies the element at the specified index to the output buffer.

Parameter(s):
  _vector:    pointer to the `d_vector` to access
  _index:     index of element to retrieve (supports negative indexing)
  _out_value: pointer to buffer to receive the element
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully copied, or
  - false, if index is invalid or parameters are invalid.
*/
D_INLINE bool
d_vector_get
(
    const struct d_vector* _vector,
    d_index                _index,
    void*                  _out_value
)
{
    void* elem_ptr;

    if ( (!_vector)            ||
         (!_out_value)         ||
         (_vector->count == 0) )
    {
        return D_FAILURE;
    }

    elem_ptr = d_vector_common_at(_vector->elements,
                                  _vector->count,
                                  _vector->element_size,
                                  _index);

    if (!elem_ptr)
    {
        return D_FAILURE;
    }

    d_memcpy(_out_value, elem_ptr, _vector->element_size);

    return D_SUCCESS;
}

/*
d_vector_set
  Sets the element at the specified index to a new value.

Parameter(s):
  _vector: pointer to the `d_vector` to modify
  _index:  index of element to set (supports negative indexing)
  _value:  pointer to new value to assign
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully set, or
  - false, if index is invalid or parameters are invalid.
*/
bool
d_vector_set
(
    struct d_vector* _vector,
    d_index          _index,
    const void*      _value
)
{
    void* elem_ptr;

    if ( (!_vector)            ||
         (!_value)             ||
         (_vector->count == 0) )
    {
        return D_FAILURE;
    }

    elem_ptr = d_vector_common_at(_vector->elements,
                                  _vector->count,
                                  _vector->element_size,
                                  _index);

    if (!elem_ptr)
    {
        return D_FAILURE;
    }

    d_memcpy(elem_ptr, _value, _vector->element_size);

    return D_SUCCESS;
}


// =============================================================================
// query functions
// =============================================================================

/*
d_vector_is_empty
  Checks if the vector contains no elements.

Parameter(s):
  _vector: pointer to the `d_vector` to query
Return:
  A boolean value corresponding to either:
  - true, if the vector is empty or NULL, or
  - false, if the vector contains elements.
*/
D_INLINE bool
d_vector_is_empty
(
    const struct d_vector* _vector
)
{
    if (!_vector)
    {
        return true;
    }

    return d_vector_common_is_empty(_vector->count);
}

/*
d_vector_is_full
  Checks if the vector's count equals its capacity.

Parameter(s):
  _vector: pointer to the `d_vector` to query
Return:
  A boolean value corresponding to either:
  - true, if count equals capacity, or
  - false, if there is available capacity or _vector is NULL.
*/
D_INLINE bool
d_vector_is_full
(
    const struct d_vector* _vector
)
{
    if (!_vector)
    {
        return false;
    }

    return d_vector_common_is_full(_vector->count, _vector->capacity);
}

/*
d_vector_size
  Returns the number of elements in the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to query
Return:
  The number of elements, or 0 if _vector is NULL.
*/
D_INLINE size_t
d_vector_size
(
    const struct d_vector* _vector
)
{
    if (!_vector)
    {
        return 0;
    }

    return d_vector_common_size(_vector->count);
}

/*
d_vector_capacity
  Returns the current capacity of the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to query
Return:
  The current capacity, or 0 if _vector is NULL.
*/
D_INLINE size_t
d_vector_capacity
(
    const struct d_vector* _vector
)
{
    return (_vector)
        ? d_vector_common_capacity(_vector->capacity)
        : 0;
}

/*
d_vector_element_size
  Returns the size in bytes of each element.

Parameter(s):
  _vector: pointer to the `d_vector` to query
Return:
  The element size, or 0 if _vector is NULL.
*/
D_INLINE size_t
d_vector_element_size
(
    const struct d_vector* _vector
)
{
    return (_vector)
        ? _vector->element_size
        : 0;
}


// =============================================================================
// search functions
// =============================================================================

/*
d_vector_find
  Finds the first occurrence of a value using a comparator function.

Parameter(s):
  _vector:     pointer to the `d_vector` to search
  _value:      pointer to value to search for
  _comparator: function to compare elements
Return:
  Index of first occurrence, or -1 if not found or parameters are invalid.
*/
D_INLINE ssize_t
d_vector_find
(
    const struct d_vector* _vector,
    const void*            _value,
    fn_comparator          _comparator
)
{
    return ( (_vector) &&
             (_comparator) )
    ? d_array_common_find(_vector->elements,
                          _vector->count,
                          _vector->element_size,
                          _value,
                          _comparator)
    : -1;
}

/*
d_vector_find_last
  Finds the last occurrence of a value using a comparator function.

Parameter(s):
  _vector:     pointer to the `d_vector` to search
  _value:      pointer to value to search for
  _comparator: function to compare elements
Return:
  Index of last occurrence, or -1 if not found or parameters are invalid.
*/
ssize_t
d_vector_find_last
(
    const struct d_vector* _vector,
    const void*            _value,
    fn_comparator          _comparator
)
{
    size_t      i;
    const char* elem_ptr;

    if ( (!_vector)            ||
         (!_comparator)        ||
         (_vector->count == 0) )
    {
        return -1;
    }

    elem_ptr = (const char*)_vector->elements;

    for (i = _vector->count; i > 0; i--)
    {
        if (_comparator(elem_ptr + ((i - 1) * _vector->element_size), _value) == 0)
        {
            return (ssize_t)(i - 1);
        }
    }

    return -1;
}

/*
d_vector_contains
  Checks if the vector contains a value using a comparator function.

Parameter(s):
  _vector:     pointer to the `d_vector` to search
  _value:      pointer to value to search for
  _comparator: function to compare elements
Return:
  A boolean value corresponding to either:
  - true, if the value was found, or
  - false, if the value was not found or parameters are invalid.
*/
D_INLINE bool
d_vector_contains
(
    const struct d_vector* _vector,
    const void*            _value,
    fn_comparator          _comparator
)
{
    return (d_vector_find(_vector, _value, _comparator) != -1);
}

/*
d_vector_count_value
  Counts occurrences of a value using a comparator function.

Parameter(s):
  _vector:     pointer to the `d_vector` to search
  _value:      pointer to value to count
  _comparator: function to compare elements
Return:
  Number of occurrences, or 0 if not found or parameters are invalid.
*/
size_t
d_vector_count_value
(
    const struct d_vector* _vector,
    const void*            _value,
    fn_comparator          _comparator
)
{
    size_t      i;
    size_t      count;
    const char* elem_ptr;

    if ( (!_vector)     ||
         (!_comparator) )
    {
        return 0;
    }

    count    = 0;
    elem_ptr = (const char*)_vector->elements;

    for (i = 0; i < _vector->count; i++)
    {
        if (_comparator(elem_ptr + (i * _vector->element_size), _value) == 0)
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
d_vector_swap
  Swaps two elements in the vector.

Parameter(s):
  _vector:  pointer to the `d_vector` to modify
  _index_a: index of first element (supports negative indexing)
  _index_b: index of second element (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if swap was successful, or
  - false, if indices are invalid or _vector is NULL.
*/
D_INLINE bool
d_vector_swap
(
    struct d_vector* _vector,
    d_index          _index_a,
    d_index          _index_b
)
{
    return ( (_vector) &&
             (_vector->count) )
    ? d_vector_common_swap(_vector->elements,
                                _vector->count,
                                _vector->element_size,
                                _index_a,
                                _index_b)
    : D_FAILURE;
}

/*
d_vector_reverse
  Reverses the order of elements in the vector.

Parameter(s):
  _vector: pointer to the `d_vector` to reverse
Return:
  A boolean value corresponding to either:
  - true, if reverse was successful, or
  - false, if _vector is NULL.
*/
D_INLINE bool
d_vector_reverse
(
    struct d_vector* _vector
)
{
    return (_vector)
    ? d_array_common_reverse(_vector->elements,
                             _vector->count,
                             _vector->element_size)
    : D_FAILURE;
}

/*
d_vector_sort
  Sorts the elements in the vector using the provided comparator.

Parameter(s):
  _vector:     pointer to the `d_vector` to sort
  _comparator: function to compare elements
Return:
  none
*/
D_INLINE void
d_vector_sort
(
    struct d_vector* _vector,
    fn_comparator    _comparator
)
{
    if ( (!_vector)     ||
         (!_comparator) )
    {
        return;
    }

    d_array_common_sort(_vector->elements,
                        _vector->count,
                        _vector->element_size,
                        _comparator);

    return;
}

/*
d_vector_copy_to
  Copies the vector's elements to a destination buffer.

Parameter(s):
  _vector:        pointer to the source `d_vector`
  _destination:   pointer to destination buffer
  _dest_capacity: capacity of destination buffer (in elements)
Return:
  A boolean value corresponding to either:
  - true, if copy was successful, or
  - false, if destination is too small or parameters are invalid.
*/
D_INLINE bool
d_vector_copy_to
(
    const struct d_vector* _vector,
    void*                  _destination,
    size_t                 _dest_capacity
)
{
    return ( (_vector) &&
             (_vector->count) )
    ? d_vector_common_copy_to(_vector->elements,
                              _vector->count,
                              _vector->element_size,
                              _destination,
                              _dest_capacity)
    : D_FAILURE;
}


// =============================================================================
// destructor functions
// =============================================================================

/*
d_vector_free
  Deallocates a vector and its internal storage.

Parameter(s):
  _vector: pointer to the `d_vector` to deallocate
Return:
  none
*/
void
d_vector_free
(
    struct d_vector* _vector
)
{
    if (_vector)
    {
        d_vector_common_free_elements(_vector->elements);

        free(_vector);
    }

    return;
}

/*
d_vector_free_deep
  Deallocates a vector, its internal storage, and all pointed-to objects.
This function treats each element as a pointer and frees what it points to.

Parameter(s):
  _vector:  pointer to the `d_vector` to deallocate
  _free_fn: function to use for freeing each pointed-to object
Return:
  none
Note:
  This function assumes each element is a pointer (element_size == sizeof(void*)).
  Behavior is undefined if used with non-pointer element types.
*/
void
d_vector_free_deep
(
    struct d_vector* _vector,
    fn_free          _free_fn
)
{
    size_t i;
    void** elements;

    if ( (_vector) &&
         (_free_fn) )
    {
        // only valid for pointer-sized elements
        if ( (_vector->elements)                       &&
             (_vector->element_size == sizeof(void*)) )
        {
            elements = (void**)_vector->elements;

            for (i = 0; i < _vector->count; i++)
            {
                if (elements[i])
                {
                    _free_fn(elements[i]);
                }
            }
        }

        d_vector_common_free_elements(_vector->elements);

        free(_vector);
    }

    return;
}
