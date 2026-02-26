/******************************************************************************
* djinterp [container]                                         circular_array.c
*
*   Implementation of the circular array (ring buffer) data structure.
*
*
* path:      \src\container\array\circular_array.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.05.08
******************************************************************************/
#include "../../../../inc/c/container/array/circular_array.h"


// =============================================================================
// internal helper functions
// =============================================================================

/*
d_circular_array_internal_get_physical_index
  Converts a logical index (0 = oldest element) to physical index in buffer.

Parameter(s):
  _circular_array: pointer to circular array
  _logical_index:  logical index (0-based from head)
Return:
  Physical index in the elements buffer
*/
D_STATIC_INLINE size_t
d_circular_array_internal_get_physical_index
(
    const struct d_circular_array* _circular_array,
    size_t                         _logical_index
)
{
    return (_circular_array->head + _logical_index) % _circular_array->capacity;
}


// =============================================================================
// constructor functions
// =============================================================================

/*
d_circular_array_new
  Creates and initializes a new empty circular array with memory allocated
for the specified capacity and element size.

Parameter(s):
  _capacity:     maximum number of elements the circular array can contain.
                 Must be greater than 0.
  _element_size: size in bytes of each element. Must be > 0.
Return:
  - Pointer to new `d_circular_array` on success
  - NULL if either parameter is 0 or memory allocation fails
Notes:
  - Caller is responsible for calling d_circular_array_free() to release memory
  - Circular array is initialized to empty state (count = 0, head = tail = 0)
  - Uses calloc() to zero-initialize the internal buffer
*/
struct d_circular_array*
d_circular_array_new
(
    size_t _capacity,
    size_t _element_size
)
{
    struct d_circular_array* result;

    // validate input parameters
    if ( (_capacity == 0)     ||
         (_element_size == 0) )
    {
        return NULL;
    }

    // check for potential overflow in total allocation size
    if (_capacity > SIZE_MAX / _element_size)
    {
        return NULL;
    }

    // allocate memory for the circular array structure
    result = malloc(sizeof(struct d_circular_array));

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // allocate memory for the buffer data
    result->elements = calloc(_capacity, _element_size);

    if (!result->elements)
    {
        free(result);

        return NULL;
    }

    // initialize the circular buffer fields
    result->capacity     = _capacity;
    result->element_size = _element_size;
    result->head         = 0;
    result->tail         = 0;
    result->count        = 0;

    return result;
}

/*
d_circular_array_new_default_capacity
  Creates a new circular array with the default capacity.

Parameter(s):
  _element_size: size in bytes of each element. Must be > 0.
Return:
  - Pointer to new `d_circular_array` on success
  - NULL if _element_size is 0 or memory allocation fails
*/
struct d_circular_array*
d_circular_array_new_default_capacity
(
    size_t _element_size
)
{
    return d_circular_array_new(D_CIRCULAR_ARRAY_DEFAULT_CAPACITY,
                                _element_size);
}

/*
d_circular_array_new_from_arr
  Creates and initializes a new circular array from an existing array.
The array will have the specified capacity and will be pre-populated with
elements from the source array up to the minimum of capacity and source count.

Parameter(s):
  _capacity:     maximum number of elements the buffer can contain. Must be > 0.
  _element_size: size in bytes of each element. Must be > 0.
  _source:       source array to copy elements from. Must not be NULL.
  _source_count: number of elements in the source array.
Return:
  - Pointer to new d_circular_array pre-populated with array data on success
  - NULL if any parameter is invalid or memory allocation fails
Notes:
  - If _source_count > _capacity, only the first _capacity elements are copied
  - If _source_count < _capacity, buffer will be partially filled
*/
struct d_circular_array*
d_circular_array_new_from_arr
(
    size_t      _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    struct d_circular_array* result;
    size_t                   elements_to_copy;

    // validate input parameters
    if ( (_capacity == 0)     ||
         (_element_size == 0) ||
         (!_source)           ||
         (_source_count == 0) )
    {
        return NULL;
    }

    // create the buffer structure
    result = d_circular_array_new(_capacity, _element_size);

    if (!result)
    {
        return NULL;
    }

    // determine how many elements to copy
    elements_to_copy = (_source_count < _capacity) ? _source_count : _capacity;

    // copy elements from source array
    d_memcpy(result->elements, _source, elements_to_copy * _element_size);

    // update buffer state
    result->count = elements_to_copy;
    result->head  = 0;
    result->tail  = (elements_to_copy == _capacity) ? 0 : elements_to_copy;

    return result;
}

/*
d_circular_array_new_from_args
  Creates a new circular array from variadic arguments.

Parameter(s):
  _capacity:     maximum number of elements the buffer can contain. Must be > 0.
  _element_size: size in bytes of each element. Must be > 0.
  _arg_count:    number of variadic arguments to process.
  ...:           variadic arguments containing the element data.
Return:
  - Pointer to new d_circular_array on success
  - NULL if allocation fails or parameters are invalid
*/
struct d_circular_array*
d_circular_array_new_from_args
(
    size_t _capacity,
    size_t _element_size,
    size_t _arg_count,
    ...
)
{
    struct d_circular_array* result;
    va_list                  args;
    size_t                   elements_to_copy;
    size_t                   i;
    char*                    elem_ptr;

    // validate input parameters
    if ( (_capacity == 0)     ||
         (_element_size == 0) ||
         (_arg_count == 0) )
    {
        return NULL;
    }

    result = d_circular_array_new(_capacity, _element_size);

    if (!result)
    {
        return NULL;
    }

    elements_to_copy = (_arg_count < _capacity) ? _arg_count : _capacity;
    elem_ptr = (char*)result->elements;

    va_start(args, _arg_count);

    for (i = 0; i < elements_to_copy; i++)
    {
        if (_element_size <= sizeof(int))
        {
            int value = va_arg(args, int);

            d_memcpy(elem_ptr + (i * _element_size), &value, _element_size);
        }
        else if (_element_size <= sizeof(void*))
        {
            void* value = va_arg(args, void*);

            d_memcpy(elem_ptr + (i * _element_size), &value, _element_size);
        }
        else
        {
            void* value_ptr = va_arg(args, void*);

            if (value_ptr)
            {
                d_memcpy(elem_ptr + (i * _element_size),
                         value_ptr,
                         _element_size);
            }
            else
            {
                d_memset(elem_ptr + (i * _element_size), 0, _element_size);
            }
        }
    }

    va_end(args);

    result->count = elements_to_copy;
    result->tail  = (elements_to_copy == _capacity) ? 0 : elements_to_copy;

    return result;
}

/*
d_circular_array_new_copy
  Creates a complete copy of an existing circular array.

Parameter(s):
  _other: pointer to the circular array to copy. Must not be NULL.
Return:
  - Pointer to new d_circular_array that is a copy of the source
  - NULL if _other is NULL or memory allocation fails
*/
struct d_circular_array*
d_circular_array_new_copy
(
    const struct d_circular_array* _other
)
{
    struct d_circular_array* result;
    size_t                   i;
    size_t                   src_index;
    const char*              src_data;
    char*                    dest_data;

    if (!_other)
    {
        return NULL;
    }

    result = d_circular_array_new(_other->capacity, _other->element_size);

    if (!result)
    {
        return NULL;
    }

    // if source is empty, return empty copy
    if (_other->count == 0)
    {
        return result;
    }

    // copy elements maintaining logical order
    src_data  = (const char*)_other->elements;
    dest_data = (char*)result->elements;

    for (i = 0; i < _other->count; i++)
    {
        src_index = d_circular_array_internal_get_physical_index(_other, i);

        d_memcpy(dest_data + (i * _other->element_size),
                 src_data + (src_index * _other->element_size),
                 _other->element_size);
    }

    // update new buffer state (linearized layout)
    result->count = _other->count;
    result->head  = 0;
    result->tail  = (_other->count == _other->capacity) ? 0 : _other->count;

    return result;
}

/*
d_circular_array_new_copy_resized
  Creates a copy of an existing circular array with a different capacity.

Parameter(s):
  _other:        pointer to the circular array to copy. Must not be NULL.
  _new_capacity: new capacity for the copy. Must be >= _other->count.
Return:
  - Pointer to new d_circular_array with the new capacity
  - NULL if parameters are invalid or memory allocation fails
*/
struct d_circular_array*
d_circular_array_new_copy_resized
(
    const struct d_circular_array* _other,
    size_t                         _new_capacity
)
{
    struct d_circular_array* result;
    size_t                   i;
    size_t                   src_index;
    const char*              src_data;
    char*                    dest_data;

    if ( (!_other)                       ||
         (_new_capacity < _other->count) )
    {
        return NULL;
    }

    result = d_circular_array_new(_new_capacity, _other->element_size);

    if (!result)
    {
        return NULL;
    }

    if (_other->count == 0)
    {
        return result;
    }

    src_data  = (const char*)_other->elements;
    dest_data = (char*)result->elements;

    for (i = 0; i < _other->count; i++)
    {
        src_index = d_circular_array_internal_get_physical_index(_other, i);

        d_memcpy(dest_data + (i * _other->element_size),
                 src_data + (src_index * _other->element_size),
                 _other->element_size);
    }

    result->count = _other->count;
    result->head  = 0;
    result->tail  = _other->count;

    return result;
}

/*
d_circular_array_new_fill
  Creates a new circular array filled with a specified value.

Parameter(s):
  _capacity:   capacity of the new circular array
  _element_size: size of each element
  _fill_value: pointer to the value to fill with
Return:
  - Pointer to new d_circular_array filled with the value
  - NULL if parameters are invalid or memory allocation fails
*/
struct d_circular_array*
d_circular_array_new_fill
(
    size_t      _capacity,
    size_t      _element_size,
    const void* _fill_value
)
{
    struct d_circular_array* result;
    size_t                   i;
    char*                    elem_ptr;

    if ( (_capacity == 0)     ||
         (_element_size == 0) ||
         (!_fill_value) )
    {
        return NULL;
    }

    result = d_circular_array_new(_capacity, _element_size);

    if (!result)
    {
        return NULL;
    }

    elem_ptr = (char*)result->elements;

    for (i = 0; i < _capacity; i++)
    {
        d_memcpy(elem_ptr + (i * _element_size), _fill_value, _element_size);
    }

    result->count = _capacity;
    result->tail  = 0;  // full buffer wraps tail to 0

    return result;
}


// =============================================================================
// element access functions
// =============================================================================

/*
d_circular_array_get
  Returns a pointer to the element at the specified logical index.

Parameter(s):
  _circular_array: pointer to circular array
  _index:          logical index (supports negative indexing)
Return:
  - Pointer to element at the specified index
  - NULL if circular array is NULL, empty, or index is out of bounds
*/
void*
d_circular_array_get
(
    const struct d_circular_array* _circular_array,
    d_index                        _index
)
{
    size_t logical_index;
    size_t physical_index;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return NULL;
    }

    if (!d_index_convert_safe(_index, _circular_array->count, &logical_index))
    {
        return NULL;
    }

    physical_index = d_circular_array_internal_get_physical_index(_circular_array,
                                                                   logical_index);

    return (char*)_circular_array->elements +
           (physical_index * _circular_array->element_size);
}

/*
d_circular_array_set
  Sets the value at the specified logical index.

Parameter(s):
  _circular_array: pointer to circular array
  _index:          logical index (supports negative indexing)
  _value:          pointer to value to set
Return:
  - true if value was successfully set
  - false if parameters are invalid or index is out of bounds
*/
bool
d_circular_array_set
(
    struct d_circular_array* _circular_array,
    d_index                  _index,
    const void*              _value
)
{
    size_t logical_index;
    size_t physical_index;
    char*  dest;

    if ( (!_circular_array)            ||
         (!_value)                     ||
         (_circular_array->count == 0) )
    {
        return D_FAILURE;
    }

    if (!d_index_convert_safe(_index, _circular_array->count, &logical_index))
    {
        return D_FAILURE;
    }

    physical_index = d_circular_array_internal_get_physical_index(_circular_array,
                                                                   logical_index);
    dest = (char*)_circular_array->elements +
           (physical_index * _circular_array->element_size);

    d_memcpy(dest, _value, _circular_array->element_size);

    return D_SUCCESS;
}

/*
d_circular_array_front
  Returns a pointer to the front (oldest) element.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to front element
  - NULL if circular array is NULL or empty
*/
void*
d_circular_array_front
(
    const struct d_circular_array* _circular_array
)
{
    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return NULL;
    }

    return (char*)_circular_array->elements +
           (_circular_array->head * _circular_array->element_size);
}

/*
d_circular_array_back
  Returns a pointer to the back (newest) element.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to back element
  - NULL if circular array is NULL or empty
*/
void*
d_circular_array_back
(
    const struct d_circular_array* _circular_array
)
{
    size_t back_index;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return NULL;
    }

    // tail points to next write position, so back is one before
    back_index = (_circular_array->tail == 0) 
                 ? _circular_array->capacity - 1
                 : _circular_array->tail - 1;

    return (char*)_circular_array->elements +
           (back_index * _circular_array->element_size);
}

/*
d_circular_array_peek
  Returns a pointer to the front element without removing it.
Alias for d_circular_array_front.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to front element
  - NULL if circular array is NULL or empty
*/
void*
d_circular_array_peek
(
    const struct d_circular_array* _circular_array
)
{
    return d_circular_array_front(_circular_array);
}

/*
d_circular_array_peek_back
  Returns a pointer to the back element without removing it.
Alias for d_circular_array_back.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to back element
  - NULL if circular array is NULL or empty
*/
void*
d_circular_array_peek_back
(
    const struct d_circular_array* _circular_array
)
{
    return d_circular_array_back(_circular_array);
}


// =============================================================================
// modification functions - push/pop operations
// =============================================================================

/*
d_circular_array_push
  Adds an element to the back of the circular array.
Alias for d_circular_array_push_back.

Parameter(s):
  _circular_array: pointer to circular array
  _element:        pointer to element to add
Return:
  - true if element was added successfully
  - false if buffer is full or parameters are invalid
*/
bool
d_circular_array_push
(
    struct d_circular_array* _circular_array,
    const void*              _element
)
{
    return d_circular_array_push_back(_circular_array, _element);
}

/*
d_circular_array_push_front
  Adds an element to the front of the circular array.

Parameter(s):
  _circular_array: pointer to circular array
  _element:        pointer to element to add
Return:
  - true if element was added successfully
  - false if buffer is full or parameters are invalid
*/
bool
d_circular_array_push_front
(
    struct d_circular_array* _circular_array,
    const void*              _element
)
{
    size_t new_head;
    char*  dest;

    if ( (!_circular_array) ||
         (!_element) )
    {
        return D_FAILURE;
    }

    if (d_circular_array_is_full(_circular_array))
    {
        return D_FAILURE;
    }

    // move head back one position (with wrap)
    new_head = (_circular_array->head == 0)
               ? _circular_array->capacity - 1
               : _circular_array->head - 1;

    dest = (char*)_circular_array->elements +
           (new_head * _circular_array->element_size);

    d_memcpy(dest, _element, _circular_array->element_size);

    _circular_array->head = new_head;
    _circular_array->count++;

    return D_SUCCESS;
}

/*
d_circular_array_push_back
  Adds an element to the back of the circular array.

Parameter(s):
  _circular_array: pointer to circular array
  _element:        pointer to element to add
Return:
  - true if element was added successfully
  - false if buffer is full or parameters are invalid
*/
bool
d_circular_array_push_back
(
    struct d_circular_array* _circular_array,
    const void*              _element
)
{
    char* dest;

    if ( (!_circular_array) ||
         (!_element) )
    {
        return D_FAILURE;
    }

    if (d_circular_array_is_full(_circular_array))
    {
        return D_FAILURE;
    }

    dest = (char*)_circular_array->elements +
           (_circular_array->tail * _circular_array->element_size);

    d_memcpy(dest, _element, _circular_array->element_size);

    _circular_array->tail = (_circular_array->tail + 1) %
                            _circular_array->capacity;
    _circular_array->count++;

    return D_SUCCESS;
}

/*
d_circular_array_push_all
  Pushes multiple elements to the back of the circular array.
Atomic operation - either all elements are pushed or none.

Parameter(s):
  _circular_array: pointer to circular array
  _elements:       pointer to elements to add
  _count:          number of elements to add
Return:
  - true if all elements were added successfully
  - false if insufficient space or parameters are invalid
*/
bool
d_circular_array_push_all
(
    struct d_circular_array* _circular_array,
    const void*              _elements,
    size_t                   _count
)
{
    size_t       available;
    size_t       current_tail;
    size_t       dest_pos;
    size_t       i;
    const char*  src;
    char*        dest;

    if ( (!_circular_array) ||
         (!_elements)       ||
         (_count == 0) )
    {
        return D_FAILURE;
    }

    available = d_circular_array_available_space(_circular_array);

    if (_count > available)
    {
        return D_FAILURE;
    }

    src          = (const char*)_elements;
    dest         = (char*)_circular_array->elements;
    current_tail = _circular_array->tail;

    for (i = 0; i < _count; i++)
    {
        dest_pos = (current_tail + i) % _circular_array->capacity;

        d_memcpy(dest + (dest_pos * _circular_array->element_size),
                 src + (i * _circular_array->element_size),
                 _circular_array->element_size);
    }

    _circular_array->tail = (current_tail + _count) % _circular_array->capacity;
    _circular_array->count += _count;

    return D_SUCCESS;
}

/*
d_circular_array_push_all_front
  Pushes multiple elements to the front of the circular array.
Atomic operation - either all elements are pushed or none.
Elements are added in order, so first element becomes the new front.

Parameter(s):
  _circular_array: pointer to circular array
  _elements:       pointer to elements to add
  _count:          number of elements to add
Return:
  - true if all elements were added successfully
  - false if insufficient space or parameters are invalid
*/
bool
d_circular_array_push_all_front
(
    struct d_circular_array* _circular_array,
    const void*              _elements,
    size_t                   _count
)
{
    size_t       available;
    size_t       new_head;
    size_t       dest_pos;
    size_t       i;
    const char*  src;
    char*        dest;

    if ( (!_circular_array) ||
         (!_elements)       ||
         (_count == 0) )
    {
        return D_FAILURE;
    }

    available = d_circular_array_available_space(_circular_array);

    if (_count > available)
    {
        return D_FAILURE;
    }

    src  = (const char*)_elements;
    dest = (char*)_circular_array->elements;

    // calculate new head position
    new_head = (_circular_array->head >= _count)
               ? _circular_array->head - _count
               : _circular_array->capacity - (_count - _circular_array->head);

    // copy elements
    for (i = 0; i < _count; i++)
    {
        dest_pos = (new_head + i) % _circular_array->capacity;

        d_memcpy(dest + (dest_pos * _circular_array->element_size),
                 src + (i * _circular_array->element_size),
                 _circular_array->element_size);
    }

    _circular_array->head = new_head;
    _circular_array->count += _count;

    return D_SUCCESS;
}

/*
d_circular_array_pop
  Removes and returns a pointer to the front element.
Alias for d_circular_array_pop_front.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to the removed element (valid until next push)
  - NULL if buffer is empty or parameter is invalid
*/
void*
d_circular_array_pop
(
    struct d_circular_array* _circular_array
)
{
    return d_circular_array_pop_front(_circular_array);
}

/*
d_circular_array_pop_front
  Removes and returns a pointer to the front element.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to the removed element (valid until next push)
  - NULL if buffer is empty or parameter is invalid
*/
void*
d_circular_array_pop_front
(
    struct d_circular_array* _circular_array
)
{
    void* item;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return NULL;
    }

    item = (char*)_circular_array->elements +
           (_circular_array->head * _circular_array->element_size);

    _circular_array->head = (_circular_array->head + 1) %
                            _circular_array->capacity;
    _circular_array->count--;

    return item;
}

/*
d_circular_array_pop_back
  Removes and returns a pointer to the back element.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to the removed element (valid until next push)
  - NULL if buffer is empty or parameter is invalid
*/
void*
d_circular_array_pop_back
(
    struct d_circular_array* _circular_array
)
{
    size_t back_index;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return NULL;
    }

    _circular_array->tail = (_circular_array->tail == 0)
                            ? _circular_array->capacity - 1
                            : _circular_array->tail - 1;
    _circular_array->count--;

    back_index = _circular_array->tail;

    return (char*)_circular_array->elements +
           (back_index * _circular_array->element_size);
}

/*
d_circular_array_pop_to
  Removes the front element and copies it to the output buffer.

Parameter(s):
  _circular_array: pointer to circular array
  _out_value:      pointer to buffer to receive the element
Return:
  - true if element was removed and copied
  - false if buffer is empty or parameters are invalid
*/
bool
d_circular_array_pop_to
(
    struct d_circular_array* _circular_array,
    void*                    _out_value
)
{
    return d_circular_array_pop_front_to(_circular_array, _out_value);
}

/*
d_circular_array_pop_front_to
  Removes the front element and copies it to the output buffer.

Parameter(s):
  _circular_array: pointer to circular array
  _out_value:      pointer to buffer to receive the element
Return:
  - true if element was removed and copied
  - false if buffer is empty or parameters are invalid
*/
bool
d_circular_array_pop_front_to
(
    struct d_circular_array* _circular_array,
    void*                    _out_value
)
{
    void* item;

    if ( (!_circular_array) ||
         (!_out_value)      ||
         (_circular_array->count == 0) )
    {
        return D_FAILURE;
    }

    item = (char*)_circular_array->elements +
           (_circular_array->head * _circular_array->element_size);

    d_memcpy(_out_value, item, _circular_array->element_size);

    _circular_array->head = (_circular_array->head + 1) %
                            _circular_array->capacity;
    _circular_array->count--;

    return D_SUCCESS;
}

/*
d_circular_array_pop_back_to
  Removes the back element and copies it to the output buffer.

Parameter(s):
  _circular_array: pointer to circular array
  _out_value:      pointer to buffer to receive the element
Return:
  - true if element was removed and copied
  - false if buffer is empty or parameters are invalid
*/
bool
d_circular_array_pop_back_to
(
    struct d_circular_array* _circular_array,
    void*                    _out_value
)
{
    size_t back_index;
    void*  item;

    if ( (!_circular_array) ||
         (!_out_value)      ||
         (_circular_array->count == 0) )
    {
        return D_FAILURE;
    }

    _circular_array->tail = (_circular_array->tail == 0)
                            ? _circular_array->capacity - 1
                            : _circular_array->tail - 1;

    back_index = _circular_array->tail;
    item       = (char*)_circular_array->elements +
                 (back_index * _circular_array->element_size);

    d_memcpy(_out_value, item, _circular_array->element_size);

    _circular_array->count--;

    return D_SUCCESS;
}


// =============================================================================
// modification functions - overwriting operations
// =============================================================================

/*
d_circular_array_push_overwrite
  Pushes an element to the back, overwriting the oldest if full.

Parameter(s):
  _circular_array: pointer to circular array
  _element:        pointer to element to add
Return:
  - true if element was added
  - false if parameters are invalid
*/
bool
d_circular_array_push_overwrite
(
    struct d_circular_array* _circular_array,
    const void*              _element
)
{
    char* dest;

    if ( (!_circular_array) ||
         (!_element) )
    {
        return D_FAILURE;
    }

    dest = (char*)_circular_array->elements +
           (_circular_array->tail * _circular_array->element_size);

    d_memcpy(dest, _element, _circular_array->element_size);

    _circular_array->tail = (_circular_array->tail + 1) %
                            _circular_array->capacity;

    if (_circular_array->count < _circular_array->capacity)
    {
        _circular_array->count++;
    }
    else
    {
        // overwriting oldest element, move head forward
        _circular_array->head = _circular_array->tail;
    }

    return D_SUCCESS;
}

/*
d_circular_array_push_front_overwrite
  Pushes an element to the front, overwriting the newest if full.

Parameter(s):
  _circular_array: pointer to circular array
  _element:        pointer to element to add
Return:
  - true if element was added
  - false if parameters are invalid
*/
bool
d_circular_array_push_front_overwrite
(
    struct d_circular_array* _circular_array,
    const void*              _element
)
{
    size_t new_head;
    char*  dest;

    if ( (!_circular_array) ||
         (!_element) )
    {
        return D_FAILURE;
    }

    new_head = (_circular_array->head == 0)
               ? _circular_array->capacity - 1
               : _circular_array->head - 1;

    dest = (char*)_circular_array->elements +
           (new_head * _circular_array->element_size);

    d_memcpy(dest, _element, _circular_array->element_size);

    _circular_array->head = new_head;

    if (_circular_array->count < _circular_array->capacity)
    {
        _circular_array->count++;
    }
    else
    {
        // overwriting newest element, move tail back
        _circular_array->tail = new_head;
    }

    return D_SUCCESS;
}

/*
d_circular_array_push_all_overwrite
  Pushes multiple elements, overwriting oldest elements if necessary.

Parameter(s):
  _circular_array: pointer to circular array
  _elements:       pointer to elements to add
  _count:          number of elements to add
Return:
  - true if elements were added
  - false if parameters are invalid
*/
bool
d_circular_array_push_all_overwrite
(
    struct d_circular_array* _circular_array,
    const void*              _elements,
    size_t                   _count
)
{
    const char* src;
    size_t      i;

    if ( (!_circular_array) ||
         (!_elements)       ||
         (_count == 0) )
    {
        return D_FAILURE;
    }

    src = (const char*)_elements;

    // if pushing more than capacity, only keep the last 'capacity' elements
    if (_count >= _circular_array->capacity)
    {
        size_t skip = _count - _circular_array->capacity;

        d_memcpy(_circular_array->elements,
                 src + (skip * _circular_array->element_size),
                 _circular_array->capacity * _circular_array->element_size);

        _circular_array->head  = 0;
        _circular_array->tail  = 0;
        _circular_array->count = _circular_array->capacity;

        return D_SUCCESS;
    }

    // push each element with overwrite
    for (i = 0; i < _count; i++)
    {
        d_circular_array_push_overwrite(_circular_array,
                                        src + (i * _circular_array->element_size));
    }

    return D_SUCCESS;
}


// =============================================================================
// modification functions - bulk operations
// =============================================================================

/*
d_circular_array_clear
  Resets the circular array to empty state without deallocating memory.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - true if cleared successfully
  - false if _circular_array is NULL
*/
bool
d_circular_array_clear
(
    struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return D_FAILURE;
    }

    _circular_array->head  = 0;
    _circular_array->tail  = 0;
    _circular_array->count = 0;

    return D_SUCCESS;
}

/*
d_circular_array_fill
  Fills all capacity with the specified value.

Parameter(s):
  _circular_array: pointer to circular array
  _fill_value:     pointer to value to fill with
Return:
  - true if filled successfully
  - false if parameters are invalid
*/
bool
d_circular_array_fill
(
    struct d_circular_array* _circular_array,
    const void*              _fill_value
)
{
    size_t i;
    char*  dest;

    if ( (!_circular_array) ||
         (!_fill_value) )
    {
        return D_FAILURE;
    }

    dest = (char*)_circular_array->elements;

    for (i = 0; i < _circular_array->capacity; i++)
    {
        d_memcpy(dest + (i * _circular_array->element_size),
                 _fill_value,
                 _circular_array->element_size);
    }

    _circular_array->head  = 0;
    _circular_array->tail  = 0;
    _circular_array->count = _circular_array->capacity;

    return D_SUCCESS;
}

/*
d_circular_array_rotate_left
  Rotates elements left by the specified amount.

Parameter(s):
  _circular_array: pointer to circular array
  _amount:         number of positions to rotate
Return:
  - true if rotated successfully
  - false if parameters are invalid
*/
bool
d_circular_array_rotate_left
(
    struct d_circular_array* _circular_array,
    size_t                   _amount
)
{
    size_t effective_amount;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return (_circular_array != NULL);
    }

    if (_amount == 0)
    {
        return D_SUCCESS;
    }

    effective_amount = _amount % _circular_array->count;

    if (effective_amount == 0)
    {
        return D_SUCCESS;
    }

    // rotating left just moves the head forward
    _circular_array->head = (_circular_array->head + effective_amount) %
                            _circular_array->capacity;
    _circular_array->tail = (_circular_array->tail + effective_amount) %
                            _circular_array->capacity;

    return D_SUCCESS;
}

/*
d_circular_array_rotate_right
  Rotates elements right by the specified amount.

Parameter(s):
  _circular_array: pointer to circular array
  _amount:         number of positions to rotate
Return:
  - true if rotated successfully
  - false if parameters are invalid
*/
bool
d_circular_array_rotate_right
(
    struct d_circular_array* _circular_array,
    size_t                   _amount
)
{
    size_t effective_amount;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return (_circular_array != NULL);
    }

    if (_amount == 0)
    {
        return D_SUCCESS;
    }

    effective_amount = _amount % _circular_array->count;

    if (effective_amount == 0)
    {
        return D_SUCCESS;
    }

    // rotating right moves head backward
    _circular_array->head = (_circular_array->head >= effective_amount)
                            ? _circular_array->head - effective_amount
                            : _circular_array->capacity -
                              (effective_amount - _circular_array->head);
    _circular_array->tail = (_circular_array->tail >= effective_amount)
                            ? _circular_array->tail - effective_amount
                            : _circular_array->capacity -
                              (effective_amount - _circular_array->tail);

    return D_SUCCESS;
}

/*
d_circular_array_reverse
  Reverses the order of elements in the circular array.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - true if reversed successfully
  - false if parameters are invalid
*/
bool
d_circular_array_reverse
(
    struct d_circular_array* _circular_array
)
{
    size_t i;
    size_t left_idx;
    size_t right_idx;
    char*  left_ptr;
    char*  right_ptr;
    char*  temp;

    if (!_circular_array)
    {
        return D_FAILURE;
    }

    if (_circular_array->count <= 1)
    {
        return D_SUCCESS;
    }

    temp = malloc(_circular_array->element_size);

    // ensure that memory allocation was successful
    if (!temp)
    {
        return D_FAILURE;
    }

    for (i = 0; i < _circular_array->count / 2; i++)
    {
        left_idx = d_circular_array_internal_get_physical_index(_circular_array,
                                                                 i);
        right_idx = d_circular_array_internal_get_physical_index(
                        _circular_array,
                        _circular_array->count - 1 - i);

        left_ptr  = (char*)_circular_array->elements +
                    (left_idx * _circular_array->element_size);
        right_ptr = (char*)_circular_array->elements +
                    (right_idx * _circular_array->element_size);

        d_memcpy(temp, left_ptr, _circular_array->element_size);
        d_memcpy(left_ptr, right_ptr, _circular_array->element_size);
        d_memcpy(right_ptr, temp, _circular_array->element_size);
    }

    free(temp);

    return D_SUCCESS;
}

/*
d_circular_array_swap
  Swaps two elements at the specified indices.

Parameter(s):
  _circular_array: pointer to circular array
  _index_a:        first index (supports negative indexing)
  _index_b:        second index (supports negative indexing)
Return:
  - true if swapped successfully
  - false if parameters are invalid or indices out of bounds
*/
bool
d_circular_array_swap
(
    struct d_circular_array* _circular_array,
    d_index                  _index_a,
    d_index                  _index_b
)
{
    size_t logical_a;
    size_t logical_b;
    size_t physical_a;
    size_t physical_b;
    char*  ptr_a;
    char*  ptr_b;
    char*  temp;

    if ( (!_circular_array)            ||
         (_circular_array->count == 0) )
    {
        return D_FAILURE;
    }

    if ( (!d_index_convert_safe(_index_a, _circular_array->count, &logical_a)) ||
         (!d_index_convert_safe(_index_b, _circular_array->count, &logical_b)) )
    {
        return D_FAILURE;
    }

    if (logical_a == logical_b)
    {
        return D_SUCCESS;
    }

    physical_a = d_circular_array_internal_get_physical_index(_circular_array,
                                                               logical_a);
    physical_b = d_circular_array_internal_get_physical_index(_circular_array,
                                                               logical_b);

    ptr_a = (char*)_circular_array->elements +
            (physical_a * _circular_array->element_size);
    ptr_b = (char*)_circular_array->elements +
            (physical_b * _circular_array->element_size);

    temp = malloc(_circular_array->element_size);

    // ensure that memory allocation was successful
    if (!temp)
    {
        return D_FAILURE;
    }

    d_memcpy(temp, ptr_a, _circular_array->element_size);
    d_memcpy(ptr_a, ptr_b, _circular_array->element_size);
    d_memcpy(ptr_b, temp, _circular_array->element_size);

    free(temp);

    return D_SUCCESS;
}


// =============================================================================
// query functions
// =============================================================================

/*
d_circular_array_is_empty
  Checks if the circular array contains no elements.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - true if empty
  - false if not empty or NULL
*/
bool
d_circular_array_is_empty
(
    const struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return false;
    }

    return (_circular_array->count == 0);
}

/*
d_circular_array_is_full
  Checks if the circular array has reached capacity.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - true if full
  - false if not full or NULL
*/
bool
d_circular_array_is_full
(
    const struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return false;
    }

    return (_circular_array->count == _circular_array->capacity);
}

/*
d_circular_array_count
  Returns the number of elements in the circular array.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  Number of elements, or 0 if NULL
*/
size_t
d_circular_array_count
(
    const struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return 0;
    }

    return _circular_array->count;
}

/*
d_circular_array_capacity
  Returns the capacity of the circular array.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  Capacity, or 0 if NULL
*/
size_t
d_circular_array_capacity
(
    const struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return 0;
    }

    return _circular_array->capacity;
}

/*
d_circular_array_available_space
  Returns the number of available slots in the circular array.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  Available slots, or 0 if NULL
*/
size_t
d_circular_array_available_space
(
    const struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return 0;
    }

    return _circular_array->capacity - _circular_array->count;
}

/*
d_circular_array_element_size
  Returns the element size of the circular array.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  Element size in bytes, or 0 if NULL
*/
size_t
d_circular_array_element_size
(
    const struct d_circular_array* _circular_array
)
{
    if (!_circular_array)
    {
        return 0;
    }

    return _circular_array->element_size;
}


// =============================================================================
// search functions
// =============================================================================

/*
d_circular_array_contains
  Checks if the circular array contains the specified value.

Parameter(s):
  _circular_array: pointer to circular array
  _value:          pointer to value to search for
  _comparator:     comparison function
Return:
  - true if value is found
  - false if not found or parameters are invalid
*/
bool
d_circular_array_contains
(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator
)
{
    return (d_circular_array_find(_circular_array, _value, _comparator) >= 0);
}

/*
d_circular_array_find
  Finds the first occurrence of the specified value.

Parameter(s):
  _circular_array: pointer to circular array
  _value:          pointer to value to search for
  _comparator:     comparison function
Return:
  - Logical index of first occurrence (>= 0)
  - -1 if not found or parameters are invalid
*/
ssize_t
d_circular_array_find
(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator
)
{
    size_t      i;
    size_t      physical_idx;
    const char* elem_ptr;

    if ( (!_circular_array) ||
         (!_value)          ||
         (!_comparator)     ||
         (_circular_array->count == 0) )
    {
        return -1;
    }

    for (i = 0; i < _circular_array->count; i++)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i);
        elem_ptr = (const char*)_circular_array->elements +
                   (physical_idx * _circular_array->element_size);

        if (_comparator(elem_ptr, _value) == 0)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}

/*
d_circular_array_find_last
  Finds the last occurrence of the specified value.

Parameter(s):
  _circular_array: pointer to circular array
  _value:          pointer to value to search for
  _comparator:     comparison function
Return:
  - Logical index of last occurrence (>= 0)
  - -1 if not found or parameters are invalid
*/
ssize_t
d_circular_array_find_last
(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator
)
{
    size_t      i;
    size_t      physical_idx;
    const char* elem_ptr;

    if ( (!_circular_array) ||
         (!_value)          ||
         (!_comparator)     ||
         (_circular_array->count == 0) )
    {
        return -1;
    }

    for (i = _circular_array->count; i > 0; i--)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i - 1);
        elem_ptr = (const char*)_circular_array->elements +
                   (physical_idx * _circular_array->element_size);

        if (_comparator(elem_ptr, _value) == 0)
        {
            return (ssize_t)(i - 1);
        }
    }

    return -1;
}

/*
d_circular_array_count_value
  Counts occurrences of the specified value.

Parameter(s):
  _circular_array: pointer to circular array
  _value:          pointer to value to count
  _comparator:     comparison function
Return:
  Number of occurrences, or 0 if not found or parameters invalid
*/
size_t
d_circular_array_count_value
(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator
)
{
    size_t      i;
    size_t      physical_idx;
    size_t      count;
    const char* elem_ptr;

    if ( (!_circular_array) ||
         (!_value)          ||
         (!_comparator) )
    {
        return 0;
    }

    count = 0;

    for (i = 0; i < _circular_array->count; i++)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i);
        elem_ptr = (const char*)_circular_array->elements +
                   (physical_idx * _circular_array->element_size);

        if (_comparator(elem_ptr, _value) == 0)
        {
            count++;
        }
    }

    return count;
}


// =============================================================================
// conversion functions
// =============================================================================

/*
d_circular_array_to_linear_array
  Creates a new linear array containing copies of all elements.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - Pointer to newly allocated array
  - NULL if empty or allocation fails
Notes:
  Caller is responsible for freeing the returned array.
*/
void*
d_circular_array_to_linear_array
(
    const struct d_circular_array* _circular_array
)
{
    void*       result;
    size_t      i;
    size_t      physical_idx;
    const char* src;
    char*       dest;

    if ( (!_circular_array) ||
         (_circular_array->count == 0) )
    {
        return NULL;
    }

    result = malloc(_circular_array->count * _circular_array->element_size);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    dest = (char*)result;
    src  = (const char*)_circular_array->elements;

    for (i = 0; i < _circular_array->count; i++)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i);

        d_memcpy(dest + (i * _circular_array->element_size),
                 src + (physical_idx * _circular_array->element_size),
                 _circular_array->element_size);
    }

    return result;
}

/*
d_circular_array_copy_to
  Copies elements to an existing buffer.

Parameter(s):
  _circular_array: pointer to circular array
  _destination:    pointer to destination buffer
  _dest_capacity:  capacity of destination in elements
Return:
  - true if copied successfully
  - false if destination too small or parameters invalid
*/
bool
d_circular_array_copy_to
(
    const struct d_circular_array* _circular_array,
    void*                          _destination,
    size_t                         _dest_capacity
)
{
    size_t      i;
    size_t      physical_idx;
    const char* src;
    char*       dest;

    if ( (!_circular_array) ||
         (!_destination) )
    {
        return D_FAILURE;
    }

    if (_dest_capacity < _circular_array->count)
    {
        return D_FAILURE;
    }

    if (_circular_array->count == 0)
    {
        return D_SUCCESS;
    }

    dest = (char*)_destination;
    src  = (const char*)_circular_array->elements;

    for (i = 0; i < _circular_array->count; i++)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i);

        d_memcpy(dest + (i * _circular_array->element_size),
                 src + (physical_idx * _circular_array->element_size),
                 _circular_array->element_size);
    }

    return D_SUCCESS;
}


// =============================================================================
// iteration helpers
// =============================================================================

/*
d_circular_array_foreach
  Applies a function to each element in order (front to back).

Parameter(s):
  _circular_array: pointer to circular array
  _apply_fn:       function to apply to each element
Return:
  none
*/
void
d_circular_array_foreach
(
    struct d_circular_array* _circular_array,
    fn_apply                 _apply_fn
)
{
    size_t i;
    size_t physical_idx;
    void*  elem_ptr;

    if ( (!_circular_array) ||
         (!_apply_fn) )
    {
        return;
    }

    for (i = 0; i < _circular_array->count; i++)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i);
        elem_ptr = (char*)_circular_array->elements +
                   (physical_idx * _circular_array->element_size);

        _apply_fn(elem_ptr);
    }

    return;
}

/*
d_circular_array_foreach_reverse
  Applies a function to each element in reverse order (back to front).

Parameter(s):
  _circular_array: pointer to circular array
  _apply_fn:       function to apply to each element
Return:
  none
*/
void
d_circular_array_foreach_reverse
(
    struct d_circular_array* _circular_array,
    fn_apply                 _apply_fn
)
{
    size_t i;
    size_t physical_idx;
    void*  elem_ptr;

    if ( (!_circular_array) ||
         (!_apply_fn) )
    {
        return;
    }

    for (i = _circular_array->count; i > 0; i--)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i - 1);
        elem_ptr = (char*)_circular_array->elements +
                   (physical_idx * _circular_array->element_size);

        _apply_fn(elem_ptr);
    }

    return;
}

// =============================================================================
// utility functions
// =============================================================================

/*
d_circular_array_sort
  Sorts the elements using the provided comparator.
Note: This linearizes the buffer as a side effect.

Parameter(s):
  _circular_array: pointer to circular array
  _comparator:     comparison function
Return:
  none
*/
void
d_circular_array_sort
(
    struct d_circular_array* _circular_array,
    fn_comparator            _comparator
)
{
    if ( (!_circular_array) ||
         (!_comparator)     ||
         (_circular_array->count <= 1) )
    {
        return;
    }

    // first linearize, then sort
    if (!d_circular_array_linearize(_circular_array))
    {
        return;
    }

    qsort(_circular_array->elements,
          _circular_array->count,
          _circular_array->element_size,
          _comparator);

    return;
}

/*
d_circular_array_linearize
  Rearranges internal storage so head is at index 0.

Parameter(s):
  _circular_array: pointer to circular array
Return:
  - true if linearized successfully
  - false if allocation fails or parameter invalid
Notes:
  This may allocate temporary memory.
*/
bool
d_circular_array_linearize
(
    struct d_circular_array* _circular_array
)
{
    void*       temp;
    size_t      i;
    size_t      physical_idx;
    const char* src;
    char*       dest;

    if (!_circular_array)
    {
        return D_FAILURE;
    }

    // already linearized
    if ( (_circular_array->head == 0)         ||
         (_circular_array->count == 0) )
    {
        return D_SUCCESS;
    }

    // allocate temporary buffer
    temp = malloc(_circular_array->count * _circular_array->element_size);

    // ensure that memory allocation was successful
    if (!temp)
    {
        return D_FAILURE;
    }

    src  = (const char*)_circular_array->elements;
    dest = (char*)temp;

    // copy elements in logical order to temp
    for (i = 0; i < _circular_array->count; i++)
    {
        physical_idx = d_circular_array_internal_get_physical_index(
                           _circular_array, i);

        d_memcpy(dest + (i * _circular_array->element_size),
                 src + (physical_idx * _circular_array->element_size),
                 _circular_array->element_size);
    }

    // copy back to elements
    d_memcpy(_circular_array->elements,
             temp,
             _circular_array->count * _circular_array->element_size);

    free(temp);

    // update indices
    _circular_array->head = 0;
    _circular_array->tail = (_circular_array->count == _circular_array->capacity)
                            ? 0
                            : _circular_array->count;

    return D_SUCCESS;
}


// =============================================================================
// memory management
// =============================================================================

/*
d_circular_array_free
  Deallocates all memory associated with the circular array.

Parameter(s):
  _circular_array: pointer to circular array to free. May be NULL.
Return:
  none
*/
void
d_circular_array_free
(
    struct d_circular_array* _circular_array
)
{
    if (_circular_array)
    {
        if (_circular_array->elements)
        {
            free(_circular_array->elements);
        }

        free(_circular_array);
    }

    return;
}

/*
d_circular_array_free_deep
  Deallocates circular array using a custom function to free each element.

Parameter(s):
  _circular_array: pointer to circular array to free. May be NULL.
  _free_fn:        function to call for each element. May be NULL.
Return:
  none
*/
void
d_circular_array_free_deep
(
    struct d_circular_array* _circular_array,
    fn_free                  _free_fn
)
{
    size_t i;
    size_t physical_idx;
    void*  elem_ptr;

    if (_circular_array)
    {
        if ( (_free_fn)                       &&
             (_circular_array->elements)      &&
             (_circular_array->count > 0) )
        {
            for (i = 0; i < _circular_array->count; i++)
            {
                physical_idx = d_circular_array_internal_get_physical_index(
                                   _circular_array, i);
                elem_ptr = (char*)_circular_array->elements +
                           (physical_idx * _circular_array->element_size);

                _free_fn(elem_ptr);
            }
        }

        if (_circular_array->elements)
        {
            free(_circular_array->elements);
        }

        free(_circular_array);
    }

    return;
}
