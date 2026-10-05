/*******************************************************************************
* djinterp [c]                                              circular_array_mtx.c
*
* TBA
*
*
* path:      /src/djinterp/c/sync/container/array/circular_array_mtx.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/sync/container/array/circular_array_mtx.h"


// =============================================================================
// internal helper functions
// =============================================================================

/*
d_circular_array_mtx_internal_wrap
  Wraps an existing d_circular_array in a new d_circular_array_mtx, initializing
the mutex. On failure, the provided buffer is freed.

Parameter(s):
  _buffer: pointer to an already-constructed d_circular_array. Must not be
           NULL. Ownership is transferred to the returned wrapper; on failure
           the buffer is freed by this function.
Return:
  - Pointer to new d_circular_array_mtx on success
  - NULL if _buffer is NULL, wrapper allocation fails, or mutex init fails
*/
D_STATIC struct d_circular_array_mtx*
d_circular_array_mtx_internal_wrap
(
    struct d_circular_array* _buffer
)
{
    struct d_circular_array_mtx* result;

    if (!_buffer)
    {
        return NULL;
    }

    result = malloc(sizeof(struct d_circular_array_mtx));

    // ensure that memory allocation was successful
    if (!result)
    {
        d_circular_array_free(_buffer);

        return NULL;
    }

    // initialize the mutex
    if (d_mutex_init(&result->mutex) != D_MUTEX_SUCCESS)
    {
        d_circular_array_free(_buffer);
        free(result);

        return NULL;
    }

    result->buffer = _buffer;

    return result;
}


// =============================================================================
// constructor functions
// =============================================================================

/*
d_circular_array_mtx_new
  Creates and initializes a new thread-safe empty circular array with memory
allocated for the specified capacity and element size.

Parameter(s):
  _capacity:     maximum number of elements the buffer can contain. Must be
                 greater than 0.
  _element_size: size in bytes of each element. Must be > 0.
Return:
  - Pointer to new d_circular_array_mtx on success
  - NULL if either parameter is 0, memory allocation fails, or mutex init fails
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new
(
    size_t _capacity,
    size_t _element_size
)
{
    struct d_circular_array* buffer;

    buffer = d_circular_array_new(_capacity, _element_size);

    if (!buffer)
    {
        return NULL;
    }

    return d_circular_array_mtx_internal_wrap(buffer);
}

/*
d_circular_array_mtx_new_default_capacity
  Creates a new thread-safe circular array with the default capacity.

Parameter(s):
  _element_size: size in bytes of each element. Must be > 0.
Return:
  - Pointer to new d_circular_array_mtx on success
  - NULL if _element_size is 0, memory allocation fails, or mutex init fails
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new_default_capacity
(
    size_t _element_size
)
{
    return d_circular_array_mtx_new(D_CIRCULAR_ARRAY_DEFAULT_CAPACITY,
                                   _element_size);
}

/*
d_circular_array_mtx_new_from_arr
  Creates and initializes a new thread-safe circular array from an existing
array.

Parameter(s):
  _capacity:     maximum number of elements the buffer can contain. Must be > 0.
  _element_size: size in bytes of each element. Must be > 0.
  _source:       source array to copy elements from. Must not be NULL.
  _source_count: number of elements in the source array.
Return:
  - Pointer to new d_circular_array_mtx pre-populated with array data on success
  - NULL if any parameter is invalid, memory allocation fails, or mutex init
    fails
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new_from_arr
(
    size_t      _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    struct d_circular_array* buffer;

    buffer = d_circular_array_new_from_arr(_capacity,
                                           _element_size,
                                           _source,
                                           _source_count);

    if (!buffer)
    {
        return NULL;
    }

    return d_circular_array_mtx_internal_wrap(buffer);
}

/*
d_circular_array_mtx_new_from_args
  Creates a new thread-safe circular array from variadic arguments.
Note: collects variadic arguments into a temporary array, then delegates to
d_circular_array_new_from_arr. For element types larger than sizeof(void*),
each variadic argument should be a pointer to the element data.

Parameter(s):
  _capacity:     maximum number of elements the buffer can contain. Must be > 0.
  _element_size: size in bytes of each element. Must be > 0.
  _arg_count:    number of variadic arguments to process.
  ...:           variadic arguments containing the element data.
Return:
  - Pointer to new d_circular_array_mtx on success
  - NULL if allocation fails or parameters are invalid
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new_from_args
(
    size_t _capacity,
    size_t _element_size,
    size_t _arg_count,
    ...
)
{
    struct d_circular_array* buffer;
    va_list                  args;
    void*                    temp_arr;
    char*                    dest;
    size_t                   elements_to_copy;
    size_t                   i;

    // validate input parameters
    if ( (_capacity == 0)     ||
         (_element_size == 0) ||
         (_arg_count == 0) )
    {
        return NULL;
    }

    elements_to_copy = (_arg_count < _capacity) ? _arg_count : _capacity;

    // allocate temporary linear array for collecting variadic arguments
    temp_arr = malloc(elements_to_copy * _element_size);

    // ensure that memory allocation was successful
    if (!temp_arr)
    {
        return NULL;
    }

    dest = (char*)temp_arr;

    va_start(args, _arg_count);

    for (i = 0; i < elements_to_copy; i++)
    {
        if (_element_size <= sizeof(int))
        {
            int value = va_arg(args, int);

            d_memcpy(dest + (i * _element_size), &value, _element_size);
        }
        else if (_element_size <= sizeof(void*))
        {
            void* value = va_arg(args, void*);

            d_memcpy(dest + (i * _element_size), &value, _element_size);
        }
        else
        {
            void* value_ptr = va_arg(args, void*);

            if (value_ptr)
            {
                d_memcpy(dest + (i * _element_size),
                         value_ptr,
                         _element_size);
            }
            else
            {
                d_memset(dest + (i * _element_size), 0, _element_size);
            }
        }
    }

    va_end(args);

    // delegate to new_from_arr for the inner buffer
    buffer = d_circular_array_new_from_arr(_capacity,
                                            _element_size,
                                            temp_arr,
                                            elements_to_copy);

    free(temp_arr);

    if (!buffer)
    {
        return NULL;
    }

    return d_circular_array_mtx_internal_wrap(buffer);
}

/*
d_circular_array_mtx_new_copy
  Creates a complete thread-safe copy of an existing d_circular_array_mtx.
Note: acquires the source mutex for the duration of the copy.

Parameter(s):
  _other: pointer to the d_circular_array_mtx to copy. Must not be NULL.
Return:
  - Pointer to new d_circular_array_mtx that is a copy of the source
  - NULL if _other is NULL, memory allocation fails, or mutex init fails
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new_copy
(
    const struct d_circular_array_mtx* _other
)
{
    struct d_circular_array_mtx* mtx;
    struct d_circular_array*     buffer;

    if (!_other)
    {
        return NULL;
    }

    mtx = (struct d_circular_array_mtx*)_other;

    d_mutex_lock(&mtx->mutex);

    buffer = d_circular_array_new_copy(_other->buffer);

    d_mutex_unlock(&mtx->mutex);

    if (!buffer)
    {
        return NULL;
    }

    return d_circular_array_mtx_internal_wrap(buffer);
}

/*
d_circular_array_mtx_new_copy_resized
  Creates a thread-safe copy of an existing d_circular_array_mtx with a
different capacity. Acquires the source mutex for the duration of the copy.

Parameter(s):
  _other:        pointer to the d_circular_array_mtx to copy. Must not be NULL.
  _new_capacity: new capacity for the copy. Must be >= current count.
Return:
  - Pointer to new d_circular_array_mtx with the new capacity
  - NULL if parameters are invalid, memory allocation fails, or mutex init fails
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new_copy_resized
(
    const struct d_circular_array_mtx* _other,
    size_t                            _new_capacity
)
{
    struct d_circular_array_mtx* mt;
    struct d_circular_array*    buffer;

    if (!_other)
    {
        return NULL;
    }

    mt = (struct d_circular_array_mtx*)_other;

    d_mutex_lock(&mt->mutex);

    buffer = d_circular_array_new_copy_resized(_other->buffer, _new_capacity);

    d_mutex_unlock(&mt->mutex);

    if (!buffer)
    {
        return NULL;
    }

    return d_circular_array_mtx_internal_wrap(buffer);
}

/*
d_circular_array_mtx_new_fill
  Creates a new thread-safe circular array filled with a specified value.

Parameter(s):
  _capacity:     capacity of the new circular array
  _element_size: size of each element
  _fill_value:   pointer to the value to fill with
Return:
  - Pointer to new d_circular_array_mtx filled with the value
  - NULL if parameters are invalid, memory allocation fails, or mutex init fails
*/
struct d_circular_array_mtx*
d_circular_array_mtx_new_fill
(
    size_t      _capacity,
    size_t      _element_size,
    const void* _fill_value
)
{
    struct d_circular_array* buffer;

    buffer = d_circular_array_new_fill(_capacity,
                                       _element_size,
                                       _fill_value);

    if (!buffer)
    {
        return NULL;
    }

    return d_circular_array_mtx_internal_wrap(buffer);
}


// =============================================================================
// element access functions
// =============================================================================

/*
d_circular_array_mtx_get
  Copies the element at the specified logical index into the output buffer.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _index:     logical index (supports negative indexing)
  _out_value: pointer to buffer to receive the element copy
Return:
  - true if element was copied successfully
  - false if parameters are invalid or index is out of bounds
*/
bool
d_circular_array_mtx_get
(
    struct d_circular_array_mtx* _mtx,
    d_index                      _index,
    void*                        _out_value
)
{
    void* elem;

    if ( (!_mtx) ||
         (!_out_value) )
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    elem = d_circular_array_get(_mtx->buffer, _index);

    if (!elem)
    {
        d_mutex_unlock(&_mtx->mutex);

        return D_FAILURE;
    }

    d_memcpy(_out_value, elem, _mtx->buffer->element_size);

    d_mutex_unlock(&_mtx->mutex);

    return D_SUCCESS;
}

/*
d_circular_array_mtx_set
  Sets the value at the specified logical index.

Parameter(s):
  _mtx:    pointer to thread-safe circular array
  _index: logical index (supports negative indexing)
  _value: pointer to value to set
Return:
  - true if value was successfully set
  - false if parameters are invalid or index is out of bounds
*/
bool
d_circular_array_mtx_set
(
    struct d_circular_array_mtx* _mtx,
    d_index                     _index,
    const void*                 _value
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_set(_mtx->buffer, _index, _value);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_front
  Copies the front (oldest) element into the output buffer.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element copy
Return:
  - true if element was copied successfully
  - false if circular array is NULL, empty, or _out_value is NULL
*/
bool
d_circular_array_mtx_front
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    void* elem;

    if ( (!_mtx)        ||
         (!_out_value) )
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    elem = d_circular_array_front(_mtx->buffer);

    if (!elem)
    {
        d_mutex_unlock(&_mtx->mutex);

        return D_FAILURE;
    }

    d_memcpy(_out_value, elem, _mtx->buffer->element_size);

    d_mutex_unlock(&_mtx->mutex);

    return D_SUCCESS;
}

/*
d_circular_array_mtx_back
  Copies the back (newest) element into the output buffer.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element copy
Return:
  - true if element was copied successfully
  - false if circular array is NULL, empty, or _out_value is NULL
*/
bool
d_circular_array_mtx_back
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    void* elem;

    if ( (!_mtx)        ||
         (!_out_value) )
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    elem = d_circular_array_back(_mtx->buffer);

    if (!elem)
    {
        d_mutex_unlock(&_mtx->mutex);

        return D_FAILURE;
    }

    d_memcpy(_out_value, elem, _mtx->buffer->element_size);

    d_mutex_unlock(&_mtx->mutex);

    return D_SUCCESS;
}

/*
d_circular_array_mtx_peek
  Copies the front element into the output buffer without removing it.
Alias for d_circular_array_mtx_front.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element copy
Return:
  - true if element was copied successfully
  - false if circular array is NULL, empty, or _out_value is NULL
*/
bool
d_circular_array_mtx_peek
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    return d_circular_array_mtx_front(_mtx, _out_value);
}

/*
d_circular_array_mtx_peek_back
  Copies the back element into the output buffer without removing it.
Alias for d_circular_array_mtx_back.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element copy
Return:
  - true if element was copied successfully
  - false if circular array is NULL, empty, or _out_value is NULL
*/
bool
d_circular_array_mtx_peek_back
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    return d_circular_array_mtx_back(_mtx, _out_value);
}


// =============================================================================
// modification functions - push/pop operations
// =============================================================================

/*
d_circular_array_mtx_push
  Adds an element to the back of the circular array.
Alias for d_circular_array_mtx_push_back.

Parameter(s):
  _mtx:      pointer to thread-safe circular array
  _element: pointer to element to add
Return:
  - true if element was added successfully
  - false if buffer is full or parameters are invalid
*/
bool
d_circular_array_mtx_push
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _element
)
{
    return d_circular_array_mtx_push_back(_mtx, _element);
}

/*
d_circular_array_mtx_push_front
  Adds an element to the front of the circular array.

Parameter(s):
  _mtx:      pointer to thread-safe circular array
  _element: pointer to element to add
Return:
  - true if element was added successfully
  - false if buffer is full or parameters are invalid
*/
bool
d_circular_array_mtx_push_front
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _element
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_front(_mtx->buffer, _element);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_push_back
  Adds an element to the back of the circular array.

Parameter(s):
  _mtx:      pointer to thread-safe circular array
  _element: pointer to element to add
Return:
  - true if element was added successfully
  - false if buffer is full or parameters are invalid
*/
bool
d_circular_array_mtx_push_back
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _element
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_back(_mtx->buffer, _element);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_push_all
  Pushes multiple elements to the back of the circular array.
Atomic operation - either all elements are pushed or none.

Parameter(s):
  _mtx:       pointer to thread-safe circular array
  _elements: pointer to elements to add
  _count:    number of elements to add
Return:
  - true if all elements were added successfully
  - false if insufficient space or parameters are invalid
*/
bool
d_circular_array_mtx_push_all
(
    struct d_circular_array_mtx* _mtx,
    const void*                  _elements,
    size_t                       _count
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_all(_mtx->buffer, _elements, _count);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_push_all_front
  Pushes multiple elements to the front of the circular array.
Atomic operation - either all elements are pushed or none.
Elements are added in order, so first element becomes the new front.

Parameter(s):
  _mtx:       pointer to thread-safe circular array
  _elements: pointer to elements to add
  _count:    number of elements to add
Return:
  - true if all elements were added successfully
  - false if insufficient space or parameters are invalid
*/
bool
d_circular_array_mtx_push_all_front
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _elements,
    size_t                      _count
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_all_front(_mtx->buffer, _elements, _count);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_pop_to
  Removes the front element and copies it to the output buffer.
Alias for d_circular_array_mtx_pop_front_to.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element
Return:
  - true if element was removed and copied
  - false if buffer is empty or parameters are invalid
*/
bool
d_circular_array_mtx_pop_to
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    return d_circular_array_mtx_pop_front_to(_mtx, _out_value);
}

/*
d_circular_array_mtx_pop_front_to
  Removes the front element and copies it to the output buffer.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element
Return:
  - true if element was removed and copied
  - false if buffer is empty or parameters are invalid
*/
bool
d_circular_array_mtx_pop_front_to
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_pop_front_to(_mtx->buffer, _out_value);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_pop_back_to
  Removes the back element and copies it to the output buffer.

Parameter(s):
  _mtx:        pointer to thread-safe circular array
  _out_value: pointer to buffer to receive the element
Return:
  - true if element was removed and copied
  - false if buffer is empty or parameters are invalid
*/
bool
d_circular_array_mtx_pop_back_to
(
    struct d_circular_array_mtx* _mtx,
    void*                       _out_value
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_pop_back_to(_mtx->buffer, _out_value);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// modification functions - overwriting operations
// =============================================================================

/*
d_circular_array_mtx_push_overwrite
  Pushes an element to the back, overwriting the oldest if full.

Parameter(s):
  _mtx:      pointer to thread-safe circular array
  _element: pointer to element to add
Return:
  - true if element was added
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_push_overwrite
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _element
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_overwrite(_mtx->buffer, _element);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_push_front_overwrite
  Pushes an element to the front, overwriting the newest if full.

Parameter(s):
  _mtx:      pointer to thread-safe circular array
  _element: pointer to element to add
Return:
  - true if element was added
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_push_front_overwrite
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _element
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_front_overwrite(_mtx->buffer, _element);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_push_all_overwrite
  Pushes multiple elements, overwriting oldest elements if necessary.

Parameter(s):
  _mtx:       pointer to thread-safe circular array
  _elements: pointer to elements to add
  _count:    number of elements to add
Return:
  - true if elements were added
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_push_all_overwrite
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _elements,
    size_t                      _count
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_push_all_overwrite(_mtx->buffer,
                                                  _elements,
                                                  _count);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// modification functions - bulk operations
// =============================================================================

/*
d_circular_array_mtx_clear
  Resets the circular array to empty state without deallocating memory.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  - true if cleared successfully
  - false if _mtx is NULL
*/
bool
d_circular_array_mtx_clear
(
    struct d_circular_array_mtx* _mtx
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_clear(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_fill
  Fills all capacity with the specified value.

Parameter(s):
  _mtx:         pointer to thread-safe circular array
  _fill_value: pointer to value to fill with
Return:
  - true if filled successfully
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_fill
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _fill_value
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_fill(_mtx->buffer, _fill_value);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_rotate_left
  Rotates elements left by the specified amount.

Parameter(s):
  _mtx:     pointer to thread-safe circular array
  _amount: number of positions to rotate
Return:
  - true if rotated successfully
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_rotate_left
(
    struct d_circular_array_mtx* _mtx,
    size_t                      _amount
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_rotate_left(_mtx->buffer, _amount);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_rotate_right
  Rotates elements right by the specified amount.

Parameter(s):
  _mtx:     pointer to thread-safe circular array
  _amount: number of positions to rotate
Return:
  - true if rotated successfully
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_rotate_right
(
    struct d_circular_array_mtx* _mtx,
    size_t                      _amount
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_rotate_right(_mtx->buffer, _amount);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_reverse
  Reverses the order of elements in the circular array.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  - true if reversed successfully
  - false if parameters are invalid
*/
bool
d_circular_array_mtx_reverse
(
    struct d_circular_array_mtx* _mtx
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_reverse(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_swap
  Swaps two elements at the specified indices.

Parameter(s):
  _mtx:      pointer to thread-safe circular array
  _index_a: first index (supports negative indexing)
  _index_b: second index (supports negative indexing)
Return:
  - true if swapped successfully
  - false if parameters are invalid or indices out of bounds
*/
bool
d_circular_array_mtx_swap
(
    struct d_circular_array_mtx* _mtx,
    d_index                     _index_a,
    d_index                     _index_b
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_swap(_mtx->buffer, _index_a, _index_b);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// query functions
// =============================================================================

/*
d_circular_array_mtx_is_empty
  Checks if the circular array contains no elements.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  - true if empty
  - false if not empty or NULL
*/
bool
d_circular_array_mtx_is_empty
(
    struct d_circular_array_mtx* _mtx
)
{
    bool result;

    if (!_mtx)
    {
        return false;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_is_empty(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_is_full
  Checks if the circular array has reached capacity.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  - true if full
  - false if not full or NULL
*/
bool
d_circular_array_mtx_is_full
(
    struct d_circular_array_mtx* _mtx
)
{
    bool result;

    if (!_mtx)
    {
        return false;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_is_full(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_count
  Returns the number of elements in the circular array.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  Number of elements, or 0 if NULL
*/
size_t
d_circular_array_mtx_count
(
    struct d_circular_array_mtx* _mtx
)
{
    size_t result;

    if (!_mtx)
    {
        return 0;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_count(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_capacity
  Returns the capacity of the circular array.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  Capacity, or 0 if NULL
*/
size_t
d_circular_array_mtx_capacity
(
    struct d_circular_array_mtx* _mtx
)
{
    size_t result;

    if (!_mtx)
    {
        return 0;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_capacity(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_available_space
  Returns the number of available slots in the circular array.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  Available slots, or 0 if NULL
*/
size_t
d_circular_array_mtx_available_space
(
    struct d_circular_array_mtx* _mtx
)
{
    size_t result;

    if (!_mtx)
    {
        return 0;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_available_space(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_element_size
  Returns the element size of the circular array.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  Element size in bytes, or 0 if NULL
*/
size_t
d_circular_array_mtx_element_size
(
    struct d_circular_array_mtx* _mtx
)
{
    size_t result;

    if (!_mtx)
    {
        return 0;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_element_size(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// search functions
// =============================================================================

/*
d_circular_array_mtx_contains
  Checks if the circular array contains the specified value.

Parameter(s):
  _mtx:         pointer to thread-safe circular array
  _value:      pointer to value to search for
  _comparator: comparison function
Return:
  - true if value is found
  - false if not found or parameters are invalid
*/
bool
d_circular_array_mtx_contains
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _value,
    fn_comparator               _comparator
)
{
    bool result;

    if (!_mtx)
    {
        return false;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_contains(_mtx->buffer, _value, _comparator);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_find
  Finds the first occurrence of the specified value.

Parameter(s):
  _mtx:         pointer to thread-safe circular array
  _value:      pointer to value to search for
  _comparator: comparison function
Return:
  - Logical index of first occurrence (>= 0)
  - -1 if not found or parameters are invalid
*/
ssize_t
d_circular_array_mtx_find
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _value,
    fn_comparator               _comparator
)
{
    ssize_t result;

    if (!_mtx)
    {
        return -1;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_find(_mtx->buffer, _value, _comparator);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_find_last
  Finds the last occurrence of the specified value.

Parameter(s):
  _mtx:         pointer to thread-safe circular array
  _value:      pointer to value to search for
  _comparator: comparison function
Return:
  - Logical index of last occurrence (>= 0)
  - -1 if not found or parameters are invalid
*/
ssize_t
d_circular_array_mtx_find_last
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _value,
    fn_comparator               _comparator
)
{
    ssize_t result;

    if (!_mtx)
    {
        return -1;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_find_last(_mtx->buffer, _value, _comparator);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_count_value
  Counts occurrences of the specified value.

Parameter(s):
  _mtx:         pointer to thread-safe circular array
  _value:      pointer to value to count
  _comparator: comparison function
Return:
  Number of occurrences, or 0 if not found or parameters invalid
*/
size_t
d_circular_array_mtx_count_value
(
    struct d_circular_array_mtx* _mtx,
    const void*                 _value,
    fn_comparator               _comparator
)
{
    size_t result;

    if (!_mtx)
    {
        return 0;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_count_value(_mtx->buffer, _value, _comparator);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// conversion functions
// =============================================================================

/*
d_circular_array_mtx_to_linear_array
  Creates a new linear array containing copies of all elements.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  - Pointer to newly allocated array
  - NULL if empty or allocation fails
Notes:
  Caller is responsible for freeing the returned array.
*/
void*
d_circular_array_mtx_to_linear_array
(
    struct d_circular_array_mtx* _mtx
)
{
    void* result;

    if (!_mtx)
    {
        return NULL;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_to_linear_array(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}

/*
d_circular_array_mtx_copy_to
  Copies elements to an existing buffer.

Parameter(s):
  _mtx:            pointer to thread-safe circular array
  _destination:   pointer to destination buffer
  _dest_capacity: capacity of destination in elements
Return:
  - true if copied successfully
  - false if destination too small or parameters invalid
*/
bool
d_circular_array_mtx_copy_to
(
    struct d_circular_array_mtx* _mtx,
    void*                       _destination,
    size_t                      _dest_capacity
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_copy_to(_mtx->buffer,
                                       _destination,
                                       _dest_capacity);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// iteration helpers
// =============================================================================

/*
d_circular_array_mtx_foreach
  Applies a function to each element in order (front to back).
Note: the callback executes while the mutex is held. The callback must not
call any d_circular_array_mtx functions on the same instance, as this will
deadlock.

Parameter(s):
  _mtx:       pointer to thread-safe circular array
  _apply_fn: function to apply to each element
Return:
  none.
*/
void
d_circular_array_mtx_foreach
(
    struct d_circular_array_mtx* _mtx,
    fn_apply                    _apply_fn
)
{
    if (!_mtx)
    {
        return;
    }

    d_mutex_lock(&_mtx->mutex);

    d_circular_array_foreach(_mtx->buffer, _apply_fn);

    d_mutex_unlock(&_mtx->mutex);

    return;
}

/*
d_circular_array_mtx_foreach_reverse
  Applies a function to each element in reverse order (back to front).
Note: the callback executes while the mutex is held. The callback must not
call any d_circular_array_mtx functions on the same instance, as this will
deadlock.

Parameter(s):
  _mtx:       pointer to thread-safe circular array
  _apply_fn: function to apply to each element
Return:
  none.
*/
void
d_circular_array_mtx_foreach_reverse
(
    struct d_circular_array_mtx* _mtx,
    fn_apply                    _apply_fn
)
{
    if (!_mtx)
    {
        return;
    }

    d_mutex_lock(&_mtx->mutex);

    d_circular_array_foreach_reverse(_mtx->buffer, _apply_fn);

    d_mutex_unlock(&_mtx->mutex);

    return;
}


// =============================================================================
// utility functions
// =============================================================================

/*
d_circular_array_mtx_sort
  Sorts the elements using the provided comparator.
Note: this linearizes the buffer as a side effect.

Parameter(s):
  _mtx:         pointer to thread-safe circular array
  _comparator: comparison function
Return:
  none.
*/
void
d_circular_array_mtx_sort
(
    struct d_circular_array_mtx* _mtx,
    fn_comparator               _comparator
)
{
    if (!_mtx)
    {
        return;
    }

    d_mutex_lock(&_mtx->mutex);

    d_circular_array_sort(_mtx->buffer, _comparator);

    d_mutex_unlock(&_mtx->mutex);

    return;
}

/*
d_circular_array_mtx_linearize
  Rearranges internal storage so head is at index 0.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  - true if linearized successfully
  - false if allocation fails or parameter invalid
*/
bool
d_circular_array_mtx_linearize
(
    struct d_circular_array_mtx* _mtx
)
{
    bool result;

    if (!_mtx)
    {
        return D_FAILURE;
    }

    d_mutex_lock(&_mtx->mutex);

    result = d_circular_array_linearize(_mtx->buffer);

    d_mutex_unlock(&_mtx->mutex);

    return result;
}


// =============================================================================
// locking functions
// =============================================================================

/*
d_circular_array_mtx_lock
  Manually acquires the mutex. Use for compound operations that require
multiple accesses to remain atomic. Must be paired with a corresponding
call to d_circular_array_mtx_unlock.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  D_MUTEX_SUCCESS on success, or a negative error code on failure
*/
int
d_circular_array_mtx_lock
(
    struct d_circular_array_mtx* _mtx
)
{
    if (!_mtx)
    {
        return D_MUTEX_ERROR;
    }

    return d_mutex_lock(&_mtx->mutex);
}

/*
d_circular_array_mtx_trylock
  Attempts to acquire the mutex without blocking.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  D_MUTEX_SUCCESS if the lock was acquired, D_MUTEX_BUSY if the lock is
  already held by another thread, or D_MUTEX_ERROR on failure
*/
int
d_circular_array_mtx_trylock
(
    struct d_circular_array_mtx* _mtx
)
{
    if (!_mtx)
    {
        return D_MUTEX_ERROR;
    }

    return d_mutex_trylock(&_mtx->mutex);
}

/*
d_circular_array_mtx_unlock
  Releases the mutex previously acquired by d_circular_array_mtx_lock or
d_circular_array_mtx_trylock.

Parameter(s):
  _mtx: pointer to thread-safe circular array
Return:
  D_MUTEX_SUCCESS on success, or a negative error code on failure
*/
int
d_circular_array_mtx_unlock
(
    struct d_circular_array_mtx* _mtx
)
{
    if (!_mtx)
    {
        return D_MUTEX_ERROR;
    }

    return d_mutex_unlock(&_mtx->mutex);
}


// =============================================================================
// memory management
// =============================================================================

/*
d_circular_array_mtx_free
  Deallocates all memory associated with the thread-safe circular array,
including the mutex and the underlying buffer.

Parameter(s):
  _mtx: pointer to thread-safe circular array to free. May be NULL.
Return:
  none.
*/
void
d_circular_array_mtx_free
(
    struct d_circular_array_mtx* _mtx
)
{
    if (_mtx)
    {
        d_mutex_destroy(&_mtx->mutex);

        if (_mtx->buffer)
        {
            d_circular_array_free(_mtx->buffer);
        }

        free(_mtx);
    }

    return;
}

/*
d_circular_array_mtx_free_deep
  Deallocates the thread-safe circular array using a custom function to free
each element, then destroys the mutex and wrapper.

Parameter(s):
  _mtx:      pointer to thread-safe circular array to free. May be NULL.
  _free_fn: function to call for each element. May be NULL.
Return:
  none.
*/
void
d_circular_array_mtx_free_deep
(
    struct d_circular_array_mtx* _mtx,
    fn_free                     _free_fn
)
{
    if (_mtx)
    {
        d_mutex_destroy(&_mtx->mutex);

        if (_mtx->buffer)
        {
            d_circular_array_free_deep(_mtx->buffer, _free_fn);
        }

        free(_mtx);
    }

    return;
}
