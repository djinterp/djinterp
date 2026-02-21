#include "..\..\..\inc\container\array\array_common.h"


/*
d_array_common_init_sized
  Allocates and initializes elements array with specified size.

Parameter(s):
  _destination:  pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _element_size: the size, in bytes, of each individual element.
  _size:         initial size to allocate
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed.
*/
bool
d_array_common_init_sized
(
    void**  _destination,
    size_t* _count,
    size_t  _element_size,
    size_t  _size
)
{
    if (_size == 0)
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_SUCCESS;
    }

    *(_destination) = calloc(_size, _element_size);

    if (!*(_destination))
    {
        return D_FAILURE;
    }

    *(_count) = 0;  // no elements initially, just allocated space

    return D_SUCCESS;
}

/*
d_array_common_init_from_array
  Initialize elements array from existing array data.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to the source array data to copy from
  _source_count:  number of elements in the source array
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or _source is NULL.
*/
bool
d_array_common_init_from_array
(
    void**      _destination,
    size_t*     _count,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    if ( (!_source) ||
         (!_source_count) )
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_SUCCESS;
    }

    *(_destination) = malloc(_source_count * _element_size);

    if ( (!*(_destination)) ||
         (!d_memcpy(*(_destination), 
                    _source,
                    (_source_count * _element_size))) )
    {
        return D_FAILURE;
    }

    *(_count) = _source_count;

    return D_SUCCESS;
}

/*
d_array_common_init_from_args
  Initialize elements array from variadic arguments.

Parameter(s):
  _destination:  pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _element_size: the size, in bytes, of each individual element.
  _arg_count:    number of variadic arguments to process
  _args:         va_list containing the variadic arguments
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed.
*/
bool
d_array_common_init_from_args
(
    void**  _destination,
    size_t* _count,
    size_t  _element_size,
    size_t  _arg_count,
    va_list _args
)
{
    size_t i;

    if (_arg_count == 0)
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_SUCCESS;
    }

    *(_destination) = malloc(_arg_count * _element_size);

    // ensure that memory allocation was successful
    if (!*(_destination))
    {
        return D_FAILURE;
    }

    char* elem_ptr = (char*)*(_destination);

    for (i = 0; i < _arg_count; i++)
    {
        // WARNING: this assumes all arguments fit in element_size bytes
        // and are passed by value. For complex types, you may need
        // to pass pointers and dereference them.
        if (_element_size <= sizeof(int))
        {
            int value = va_arg(_args, int);

            d_memcpy(elem_ptr + (i * _element_size), &value, _element_size);
        }
        else if (_element_size <= sizeof(void*))
        {
            void* value = va_arg(_args, void*);

            d_memcpy(elem_ptr + (i * _element_size), &value, _element_size);
        }
        else
        {
            // for larger types, expect pointer to the data
            void* value_ptr = va_arg(_args, void*);

            if (value_ptr)
            {
                d_memcpy(elem_ptr + (i * _element_size), value_ptr, _element_size);
            }
            else
            {
                d_memset(elem_ptr + (i * _element_size), 0, _element_size);
            }
        }
    }

    *(_count) = _arg_count;

    return D_SUCCESS;
}

/*
d_array_common_init_copy
  Copy initialization from another container's elements.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to the source elements to copy from
  _source_count:  number of elements in the source
Return:
  A boolean value corresponding to either:
  - true, if copy initialization was successful, or
  - false, if memory allocation failed.
*/
D_INLINE bool
d_array_common_init_copy
(
    void**      _destination,
    size_t*     _count,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    return d_array_common_init_from_array(_destination,
                                          _count,
                                          _element_size,
                                          _source,
                                          _source_count);
}

/*
d_array_common_init_copy_reverse
  Initialize a new array by copying elements from source array in reverse order.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
  _start:         starting index (supports negative indexing)
  _end:           ending index (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_array_common_init_copy_reverse
(
    void**      _destination,
    size_t*     _count,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count,
    d_index     _start,
    d_index     _end
)
{
    size_t start_idx, end_idx, copy_count, i;
    const char* src_ptr;
    char* dest_ptr;

    if ( (!_destination)  ||
         (!_count)        ||
         (!_element_size) ||
         (!_source)       || 
         (!_source_count) )
    {
        if (_destination)
        {
            *(_destination) = NULL;
        }

        if (_count)
        {
            *(_count) = 0;
        }

        return D_FAILURE;
    }

    // convert negative indices to positive
    if ( (!d_index_convert_safe(_start, _source_count, &start_idx)) ||
         (!d_index_convert_safe(_end, _source_count, &end_idx)) )
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_FAILURE;
    }

    if (start_idx > end_idx)
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_FAILURE;
    }

    copy_count = end_idx - start_idx + 1;

    *(_destination) = malloc(copy_count * _element_size);

    if (!*(_destination))
    {
        *(_count) = 0;
        return D_FAILURE;
    }

    dest_ptr = (char*)*(_destination);
    
    // copy elements in reverse order
    for (i = 0; i < copy_count; i++)
    {
        src_ptr = (const char*)_source + ((end_idx - i) * _element_size);
        d_memcpy(dest_ptr + (i * _element_size), src_ptr, _element_size);
    }

    *(_count) = copy_count;

    return D_SUCCESS;
}

/*
d_array_common_init_copy_range
  Initialize a new array by copying a range of elements from source array.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
  _start:         starting index (supports negative indexing)
  _end:           ending index (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_array_common_init_copy_range
(
    void**      _destination,
    size_t*     _count,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count,
    d_index     _start,
    d_index     _end
)
{
    size_t start_idx, 
           end_idx,
           copy_count;
    const char* src_ptr;

    if ( (!_destination)  ||
         (!_count)        ||
         (!_element_size) ||
         (!_source)       ||
         (!_source_count) )
    {
        if (_destination)
        {
            *(_destination) = NULL;
        }

        if (_count)
        {
            *(_count) = 0;
        }

        return D_FAILURE;
    }

    // convert negative indices to positive
    if ( (!d_index_convert_safe(_start, _source_count, &start_idx)) ||
         (!d_index_convert_safe(_end, _source_count, &end_idx)) )
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_FAILURE;
    }

    if (start_idx > end_idx)
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_FAILURE;
    }

    copy_count = end_idx - start_idx + 1;

    *(_destination) = malloc(copy_count * _element_size);
    if (!*(_destination))
    {
        *(_count) = 0;

        return D_FAILURE;
    }

    src_ptr = (const char*)_source + (start_idx * _element_size);
    d_memcpy(*(_destination), src_ptr, copy_count * _element_size);

    *(_count) = copy_count;

    return D_SUCCESS;
}

/*
d_array_common_init_copy_range_reverse
  Initialize a new array by copying a range from source array in reverse order.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
  _start:         starting index (supports negative indexing)
  _end:           ending index (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
D_INLINE bool
d_array_common_init_copy_range_reverse
(
    void**      _destination,
    size_t*     _count,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count,
    d_index     _start,
    d_index     _end
)
{
    return d_array_common_init_copy_reverse(_destination,
                                            _count,
                                            _element_size,
                                            _source,
                                            _source_count,
                                            _start,
                                            _end);
}

/*
d_array_common_init_fill
  Initialize an array with a specified value repeated.

Parameter(s):
  _destination:  pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _element_size: the size, in bytes, of each individual element.
  _size:         number of elements to create
  _value:        pointer to value to fill with
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed.
*/
bool
d_array_common_init_fill
(
    void**      _destination,
    size_t*     _count,
    size_t      _element_size,
    size_t      _size,
    const void* _value
)
{
    size_t i;
    char* elem_ptr;

    if (_size == 0)
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_SUCCESS;
    }

    if (!_value)
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_FAILURE;
    }

    *(_destination) = malloc(_size * _element_size);
    if (!*(_destination))
    {
        *(_count) = 0;

        return D_FAILURE;
    }

    elem_ptr = (char*)*(_destination);

    for (i = 0; i < _size; i++)
    {
        d_memcpy(elem_ptr + (i * _element_size), _value, _element_size);
    }

    *(_count) = _size;

    return D_SUCCESS;
}

/*
d_array_common_init_slice
  Initialize an array by slicing from a source starting at a given index.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
  _start:         starting index (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_array_common_init_slice
(
    void**        _destination,
    size_t*       _count,
    size_t        _element_size,
    const void**  _source,
    size_t        _source_count,
    d_index       _start
)
{
    size_t start_idx, 
           copy_count;
    const char* src_ptr;

    if ( (!_destination)  ||
         (!_count)        ||
         (!_element_size) ||
         (!_source)       ||
         (!_source_count) )
    {
        if (_destination)
        {
            *(_destination) = NULL;
        }

        if (_count)
        {
            *(_count) = 0;
        }

        return D_FAILURE;
    }

    // convert negative index to positive
    if (!d_index_convert_safe(_start, _source_count, &start_idx))
    {
        *(_destination) = NULL;
        *(_count) = 0;

        return D_FAILURE;
    }

    copy_count = _source_count - start_idx;

    *(_destination) = malloc(copy_count * _element_size);
    if (!*(_destination))
    {
        *(_count) = 0;

        return D_FAILURE;
    }

    src_ptr = (const char*)_source + (start_idx * _element_size);
    d_memcpy(*(_destination), src_ptr, copy_count * _element_size);

    *(_count) = copy_count;

    return D_SUCCESS;
}

/*
d_array_common_init_slice_reverse
  Initialize an array by slicing from source in reverse order.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
D_INLINE bool
d_array_common_init_slice_reverse
(
    void**       _destination,
    size_t*      _count,
    size_t       _element_size,
    const void** _source,
    size_t       _source_count
)
{
    return d_array_common_init_copy_reverse(_destination,
                                            _count,
                                            _element_size,
                                            (const void*)_source,
                                            _source_count,
                                            0,
                                            _source_count - 1);
}

/*
d_array_common_init_slice_range
  Initialize an array by slicing a range from source.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
  _start:         starting index (supports negative indexing)
  _end:           ending index (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
D_INLINE bool
d_array_common_init_slice_range
(
    void**       _destination,
    size_t*      _count,
    size_t       _element_size,
    const void** _source,
    size_t       _source_count,
    d_index      _start,
    d_index      _end
)
{
    return d_array_common_init_copy_range(_destination,
                                          _count,
                                          _element_size,
                                          (const void*)_source,
                                          _source_count,
                                          _start,
                                          _end);
}

/*
d_array_common_init_slice_range_reverse
  Initialize an array by slicing a range from source in reverse order.

Parameter(s):
  _destination:   pointer to elements pointer to be initialized
  _count:         pointer to count variable to be set
  _element_size:  the size, in bytes, of each individual element.
  _source:        pointer to source array
  _source_count:  total number of elements in the source array
  _start:         starting index (supports negative indexing)
  _end:           ending index (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
D_INLINE bool
d_array_common_init_slice_range_reverse
(
    void**       _destination,
    size_t*      _count,
    size_t       _element_size,
    const void** _source,
    size_t       _source_count,
    d_index      _start,
    d_index      _end
)
{
    return d_array_common_init_copy_range_reverse(_destination,
                                                  _count,
                                                  _element_size,
                                                  (const void*)_source,
                                                  _source_count,
                                                  _start,
                                                  _end);
}

/*
d_array_common_alloc
  Allocate memory for a container structure.

Parameter(s):
  _struct_size: size in bytes of each individual array element.
Return:
  Pointer to allocated memory, or NULL if allocation failed.
*/
D_INLINE void*
d_array_common_alloc
(
    size_t _element_size
)
{
    if (!_element_size)
    {
        return NULL;
    }

    return calloc(1, _element_size);
}

/*
d_array_common_append_element
  Append a single element to the end of the array.

Parameter(s):
  _elements:     pointer to pointer to elements array
  _count:        pointer to current number of elements in array
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to value to append
Return:
  A boolean value corresponding to either:
  - true, if element was successfully appended, or
  - false, if parameters are invalid.
*/
D_INLINE bool
d_array_common_append_element
(
    void**      _elements,
    size_t*     _count,
    size_t      _element_size,
    const void* _value
)
{
    char* elem_ptr;
    void* new_elements;

    if ( (!_elements)     ||
         (!*(_elements))  ||
         (!_count)        ||
         (!_element_size) ||
         (!_value) )
    {
        return D_FAILURE;
    }

    // reallocate to hold one more element
    new_elements = realloc(*(_elements), (*(_count) + 1) * _element_size);

    // ensure that memory allocation was successful
    if (!new_elements)
    {
        return D_FAILURE;
    }

    *(_elements) = new_elements;

    elem_ptr = (char*)(*_elements) + (*_count * _element_size);

    d_memcpy(elem_ptr, _value, _element_size);

    (*_count)++;

    return D_SUCCESS;
}

/*
d_array_common_append_elements
  Append multiple elements to the end of the array.

Parameter(s):
  _elements:     pointer to pointer to elements array
  _count:        pointer to current number of elements in array
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to source array to append
  _source_count: number of elements to append
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully appended, or
  - false, if parameters are invalid.
*/
D_INLINE bool
d_array_common_append_elements
(
    void**      _elements,
    size_t*     _count,
    size_t      _element_size,
    const void* _value,
    size_t      _source_count
)
{
    char* elem_ptr;
    void* new_elements;

    if ( (!_elements)     ||
         (!*(_elements))  ||
         (!_count)        ||
         (!_element_size) ||
         (!_value)        ||
         (!_source_count) )
    {
        return (_source_count == 0);
    }

    // reallocate to hold additional elements
    new_elements = realloc(*(_elements), (*(_count)+_source_count) * _element_size);

    // reallocate to hold one more element
    if (!new_elements)
    {
        return D_FAILURE;
    }

    *(_elements) = new_elements;

    elem_ptr = (char*)(*_elements) + (*(_count) * _element_size);
    d_memcpy(elem_ptr, _value, (_source_count * _element_size) );

    *(_count) += _source_count;

    return D_SUCCESS;
}

/*
d_array_common_calc_capacity
  Calculate appropriate capacity for a requested size.

Parameter(s):
  _requested_size: the minimum size needed
Return:
  Calculated capacity (typically larger than requested size for efficiency).
*/
size_t
d_array_common_calc_capacity
(
    size_t _requested_size
)
{
    // Use a simple growth strategy: next power of 2 greater than requested size
    size_t capacity = 1;

    if (_requested_size == 0)
    {
        return 1;
    }

    while (capacity < _requested_size)
    {
        if (capacity > SIZE_MAX / 2)
        {
            return SIZE_MAX;  // Prevent overflow
        }
        
        capacity *= 2;
    }

    return capacity;
}

/*
d_array_common_contains
  Check if array contains a specific value.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to value to search for
  _comparator:   function to compare elements
Return:
  A boolean value corresponding to either:
  - true, if the value was found in the array, or
  - false, if the value was not found or parameters are invalid.
*/
D_INLINE bool
d_array_common_contains
(
    void*         _elements,
    size_t        _count,
    size_t        _element_size,
    const void*   _value,
    fn_comparator _comparator
)
{
    return (d_array_common_find(_elements, _count, _element_size, _value, _comparator) != -1);
}

/*
d_array_common_fill
  Fill array with a specified value.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: the size, in bytes, of each individual element.
  _fill_value:   pointer to value to fill with
Return:
  Number of elements filled, or -1 if parameters are invalid.
*/
int
d_array_common_fill
(
    void*       _elements,
    size_t      _count,
    size_t      _element_size,
    const void* _fill_value
)
{
    size_t i;
    char* elem_ptr;

    if ( (!_elements)         ||
         (!_element_size) ||
         (!_fill_value) )
    {
        return -1;
    }

    elem_ptr = (char*)_elements;

    for (i = 0; i < _count; i++)
    {
        d_memcpy(elem_ptr + (i * _element_size), _fill_value, _element_size);
    }

    return (int)_count;
}

/*
d_array_common_find
  Find first occurrence of a value in the array.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to value to search for
  _comparator:   function to compare elements
Return:
  Index of first occurrence, or -1 if not found or parameters are invalid.
*/
ssize_t
d_array_common_find
(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const void*   _value,
    fn_comparator _comparator
)
{
    size_t i;
    const char* elem_ptr;

    if ( (!_elements)     ||
         (!_element_size) ||
         (!_value)        ||
         (!_comparator) )
    {
        return -1;
    }

    elem_ptr = (const char*)_elements;

    for (i = 0; i < _count; i++)
    {
        if (_comparator(elem_ptr + (i * _element_size), _value) == 0)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}

/*
d_array_common_find_closest
  Find the closest matching value in a sorted array.

Parameter(s):
  _elements:     pointer to elements array (must be sorted)
  _count:        number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to value to search for
  _comparator:   function to compare elements
Return:
  Index of closest match, or -1 if parameters are invalid.
*/
ssize_t
d_array_common_find_closest
(
    const void*   _elements,
    size_t        _count,
    size_t        _element_size,
    const void*   _value,
    fn_comparator _comparator
)
{
    size_t left, right, mid;
    const char* elem_ptr;
    int cmp_result;

    if ( (!_elements)     ||
         (!_element_size) ||
         (!_value)        ||
         (!_comparator)   ||
         (_count == 0) )
    {
        return -1;
    }

    elem_ptr = (const char*)_elements;
    left = 0;
    right = _count - 1;

    while (left <= right)
    {
        mid = left + (right - left) / 2;
        cmp_result = _comparator(elem_ptr + (mid * _element_size), _value);

        if (cmp_result == 0)
        {
            return (ssize_t)mid;
        }
        else if (cmp_result < 0)
        {
            left = mid + 1;
        }
        else
        {
            if (mid == 0)
            {
                break;
            }

            right = mid - 1;
        }
    }

    // Return the closest index
    return (ssize_t)((left < _count) ? left : _count - 1);
}

/*
d_array_common_insert_element
  Insert a single element at the specified index.

Parameter(s):
  _elements:     pointer to pointer to elements array
  _count:        pointer to current number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to value to insert
  _index:        index where to insert (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if element was successfully inserted, or
  - false, if parameters are invalid.
*/
bool
d_array_common_insert_element
(
    void**      _elements,
    size_t*     _count,
    size_t      _element_size,
    const void* _value,
    d_index     _index
)
{
    size_t insert_idx;
    char*  elem_ptr;
    void*  new_elements;

    if ( (!_elements)     ||
         (!*(_elements))  ||
         (!_count)        ||
         (!_element_size) ||
         (!_value)        ||
         // convert negative index to positive
        (!d_index_convert_safe(_index, *(_count), &insert_idx)))
    {
        return D_FAILURE;
    }
    
    // reallocate to hold one more element
    new_elements = realloc(*_elements, (*_count + 1) * _element_size);

    // ensure that memory reallocation was successful
    if (!new_elements)
    {
        return D_FAILURE;
    }

    *(_elements) = new_elements;
    elem_ptr = (char*)(*_elements);

    // Shift elements to the right to make space
    if (insert_idx < *_count)
    {
        memmove(elem_ptr + ((insert_idx + 1) * _element_size),
                elem_ptr + (insert_idx * _element_size),
                (*_count - insert_idx) * _element_size);
    }

    // Insert the new element
    d_memcpy(elem_ptr + (insert_idx * _element_size), _value, _element_size);

    (*_count)++;

    return D_SUCCESS;
}

/*
d_array_common_insert_elements
  Insert multiple elements at the specified index.

Parameter(s):
  _elements:     pointer to pointer to elements array
  _count:        pointer to current number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to source array to insert
  _source_count: number of elements to insert
  _index:        index where to insert (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully inserted, or
  - false, if parameters are invalid.
*/
bool
d_array_common_insert_elements
(
    void**      _elements,
    size_t*     _count,
    size_t      _element_size,
    const void* _value,
    size_t      _source_count,
    d_index     _index
)
{
    size_t insert_idx;
    char*  elem_ptr;
    void*  new_elements;

    if ( (!_elements)     ||
         (!*(_elements))  ||
         (!_count)        ||
         (!_element_size) ||
         (!_value)        ||
         (!_source_count) )
    {
        return (_source_count == 0);
    }

    // convert negative index to positive
    if (!d_index_convert_safe(_index, *(_count) +_source_count, &insert_idx))
    {
        return D_FAILURE;
    }

    // reallocate to hold additional elements
    new_elements = realloc(*_elements, (*(_count) +_source_count) * _element_size);

    // ensure that memory reallocation was successful
    if (!new_elements)
    {
        return D_FAILURE;
    }

    *(_elements) = new_elements;
    elem_ptr = (char*)(*_elements);

    // Shift elements to the right to make space
    if (insert_idx < *_count)
    {
        memmove(elem_ptr + ((insert_idx + _source_count) * _element_size),
                elem_ptr + (insert_idx * _element_size),
                (*(_count) - insert_idx) * _element_size);
    }

    // Insert the new elements
    d_memcpy(elem_ptr + (insert_idx * _element_size), 
             _value, 
             (_source_count * _element_size));

    *(_count) += _source_count;

    return D_SUCCESS;
}

/*
d_array_common_is_valid_resize_amount
  Check if a resize amount is valid and calculate the result.

Parameter(s):
  _count:  current count
  _amount: amount to resize by (can be negative)
  _result: pointer to store the calculated result
Return:
  A boolean value corresponding to either:
  - true, if the resize amount is valid, or
  - false, if the resize would result in overflow/underflow.
*/
bool
d_array_common_is_valid_resize_amount
(
    size_t  _count,
    ssize_t _amount,
    size_t* _result
)
{
    size_t abs_amount;

    if (!_result)
    {
        return D_FAILURE;
    }

    if (_amount >= 0)
    {
        // growing: check for overflow
        if (_count > (SIZE_MAX - (size_t)_amount))
        {
            return D_FAILURE;
        }

        *(_result) = _count + (size_t)_amount;

        return D_SUCCESS;
    }
    else
    {
        // shrinking: check for underflow
        abs_amount = (size_t)(-_amount);

        if (_count < abs_amount)
        {
            return D_FAILURE;
        }

        *(_result) = _count - abs_amount;

        return D_SUCCESS;
    }
}

/*
d_array_common_is_valid_resize_factor
  Check if a resize factor is valid and calculate the result.

Parameter(s):
  _count:      current count
  _factor:     multiplication factor
  _result:     pointer to store the calculated result
  _round_down: whether to round down fractional results
Return:
  A boolean value corresponding to either:
  - true, if the resize factor is valid, or
  - false, if the resize would result in overflow or invalid factor.
*/
bool
d_array_common_is_valid_resize_factor
(
    size_t  _count,
    double  _factor,
    double* _result,
    bool    _round_down
)
{
    double calculated_size;

    if ( (!_result) || 
         (_factor < 0.0) )
    {
        return D_FAILURE;
    }

    calculated_size = (double)_count * _factor;

    if (calculated_size > (double)SIZE_MAX)
    {
        return D_FAILURE;
    }

    if (_round_down)
    {
        *(_result) = floor(calculated_size);
    }
    else
    {
        *(_result) = calculated_size;
    }

    return D_SUCCESS;
}
/*
d_array_common_prepend_element
  Prepend a single element to the beginning of the array.

Parameter(s):
  _elements:     pointer to pointer to elements array
  _count:        pointer to current number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to value to prepend
Return:
  A boolean value corresponding to either:
  - true, if element was successfully prepended, or
  - false, if parameters are invalid.
*/
D_INLINE bool
d_array_common_prepend_element
(
    void**      _elements,
    size_t*     _count,
    size_t      _element_size,
    const void* _value
)
{
    return d_array_common_insert_element(_elements, 
                                         _count, 
                                         _element_size, 
                                         _value, 
                                         0);
}

/*
d_array_common_prepend_elements
  Prepend multiple elements to the beginning of the array.

Parameter(s):
  _elements:     pointer to pointer to elements array
  _count:        pointer to current number of elements
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to source array to prepend
  _source_count: number of elements to prepend
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully prepended, or
  - false, if parameters are invalid.
*/
D_INLINE bool
d_array_common_prepend_elements
(
    void**      _elements,
    size_t*     _count,
    size_t      _element_size,
    const void* _value,
    size_t      _source_count
)
{
    return d_array_common_insert_elements(_elements,
                                          _count,
                                          _element_size,
                                          _value,
                                          _source_count,
                                          0);
}

/*
d_array_common_resize_amount
  Resize array by a specified amount.

Parameter(s):
  _elements:     pointer to elements array
  _count:        current number of elements
  _element_size: the size, in bytes, of each individual element.
  _amount:       amount to resize by (can be negative)
Return:
  New size if successful, or -1 if failed.
*/
ssize_t
d_array_common_resize_amount
(
    void*   _elements,
    size_t  _count,
    size_t  _element_size,
    ssize_t _amount
)
{
    size_t result;

    if ( (!_elements) || 
         (!_element_size) )
    {
        return -1;
    }

    if (d_array_common_is_valid_resize_amount(_count, _amount, &result))
    {
        return (ssize_t)result;
    }

    return -1;
}

/*
d_array_common_resize_factor
  Resize array by a multiplication factor.

Parameter(s):
  _elements:     pointer to elements array
  _count:        current number of elements
  _element_size: the size, in bytes, of each individual element.
  _factor:       multiplication factor
Return:
  New size if successful, or -1 if failed.
*/
ssize_t
d_array_common_resize_factor
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    double _factor
)
{
    double result;

    // p
    if ( (!_elements) || 
         (!_element_size) )
    {
        return -1;
    }

    if (d_array_common_is_valid_resize_factor(_count, 
                                              _factor,
                                              &result,
                                              true))
    {
        return (ssize_t)result;
    }

    return -1;
}

/*
d_array_common_reverse
  Reverse the order of elements in the array.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: the size, in bytes, of each individual element.
Return:
  A boolean value corresponding to either:
  - true, if reverse was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_common_reverse
(
    void*  _elements,
    size_t _count,
    size_t _element_size
)
{
    size_t i;
    char*  left_ptr;
    char*  right_ptr;
    char*  temp_buffer;

    if ( (!_elements)     ||
         (!_element_size) ||
         (_count <= 1) )
    {
        return (_count <= 1);  // trivially successful for 0 or 1 elements
    }

    temp_buffer = malloc(_element_size);

    if (!temp_buffer)
    {
        return D_FAILURE;
    }

    left_ptr  = (char*)_elements;
    right_ptr = (char*)_elements + ((_count - 1) * _element_size);

    for (i = 0; i < _count / 2; i++)
    {
        // swap elements
        if ( d_memcpy(temp_buffer, left_ptr, _element_size) &&
             d_memcpy(left_ptr, right_ptr, _element_size)   &&
             d_memcpy(right_ptr, temp_buffer, _element_size) )
        {
            left_ptr  += _element_size;
            right_ptr -= _element_size;
        }
        else
        {
            free(temp_buffer);

            return D_FAILURE;
        }
    }

    free(temp_buffer);

    return D_SUCCESS;
}

/*
d_array_common_shift_left
  Shift elements left within existing array (lossy).

Parameter(s):
  _elements:     pointer to elements array
  _count:        current number of elements
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_common_shift_left
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    size_t _amount
)
{
    if ( (!_elements)     || 
         (!_element_size) || 
         (_amount == 0) )
    {
        return (_amount == 0);
    } 
    else if (_amount >= _count)
    {
        return D_FAILURE;  // would shift all elements out
    }

    memmove(_elements, 
            (char*)_elements + (_amount * _element_size),
            (_count - _amount) * _element_size);

    return D_SUCCESS;
}


/*
d_array_common_shift_left_circular
  Circularly shift elements left by the specified amount. Elements that would 
  be shifted out of the left end wrap around to the right end.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements in the array
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift left
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_common_shift_left_circular
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    size_t _amount
)
{
    size_t effective_shift;
    char* temp_buffer;
    char* elem_ptr;

    if ( (!_elements)     ||
         (!_element_size) ||
         (_count == 0) )
    {
        return (_count == 0);
    }

    if (_amount == 0)
    {
        return D_SUCCESS;
    }

    // reduce amount to within array bounds using modulo
    effective_shift = _amount % _count;
    
    if (effective_shift == 0)
    {
        return D_SUCCESS;  // no actual shift needed
    }

    // allocate temporary buffer for the elements that will wrap around
    temp_buffer = malloc(effective_shift * _element_size);

    if (!temp_buffer)
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)_elements;

    // save the elements that will wrap around (first 'effective_shift' elements)
    d_memcpy(temp_buffer, elem_ptr, effective_shift * _element_size);

    // shift remaining elements left
    memmove(elem_ptr,
            elem_ptr + (effective_shift * _element_size),
            (_count - effective_shift) * _element_size);

    // place wrapped elements at the end
    d_memcpy(elem_ptr + ((_count - effective_shift) * _element_size),
             temp_buffer,
             effective_shift * _element_size);

    free(temp_buffer);

    return D_SUCCESS;
}

/*
d_array_common_shift_right
  Shift elements right within existing array (lossy).

Parameter(s):
  _elements:     pointer to elements array
  _count:        current number of elements
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_common_shift_right
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    size_t _amount
)
{
    if ( (!_elements)     || 
         (!_element_size) || 
         (_amount == 0) )
    {
        return _amount == 0;
    }

    if (_amount >= _count)
    {
        return D_FAILURE;  // would shift all elements out
    }

    memmove((char*)_elements + (_amount * _element_size),
            _elements,
            (_count - _amount) * _element_size);

    return D_SUCCESS;
}


/*
d_array_common_shift_right_circular
  Circularly shift elements right by the specified amount. Elements that would 
  be shifted out of the right end wrap around to the left end.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements in the array
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift right
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_common_shift_right_circular
(
    void*  _elements,
    size_t _count,
    size_t _element_size,
    size_t _amount
)
{
    size_t effective_shift;
    char* temp_buffer;
    char* elem_ptr;

    if ( (!_elements)     ||
         (!_element_size) ||
         (!_count) )
    {
        return (_count == 0);
    }

    if (_amount == 0)
    {
        return D_SUCCESS;
    }

    // Reduce amount to within array bounds using modulo
    effective_shift = _amount % _count;
    
    if (effective_shift == 0)
    {
        return D_SUCCESS;  // no actual shift needed
    }

    // Allocate temporary buffer for the elements that will wrap around
    temp_buffer = malloc(effective_shift * _element_size);
    if (!temp_buffer)
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)_elements;

    // Save the elements that will wrap around (last 'effective_shift' elements)
    d_memcpy(temp_buffer,
             elem_ptr + ((_count - effective_shift) * _element_size),
             effective_shift * _element_size);

    // Shift remaining elements right
    memmove(elem_ptr + (effective_shift * _element_size),
            elem_ptr,
            (_count - effective_shift) * _element_size);

    // Place wrapped elements at the beginning
    d_memcpy(elem_ptr, temp_buffer, effective_shift * _element_size);

    free(temp_buffer);

    return D_SUCCESS;
}

/*
d_array_common_sort
  Sort elements in the array using provided comparator.

Parameter(s):
  _elements:    pointer to elements array
  _count:       number of elements
  _element_size: the size, in bytes, of each individual element.
  _comparator:  function to compare elements
Return:
  none
*/
D_INLINE void
d_array_common_sort
(
    void*         _elements,
    size_t        _count,
    size_t        _element_size,
    fn_comparator _comparator
)
{
    if ( (!_elements)     || 
         (!_element_size) || 
         (!_comparator)   ||
         (_count <= 1) )
    {
        return;
    }

    qsort(_elements, _count, _element_size, _comparator);

    return;
}

/*
d_array_common_free_elements_arr
  Free container elements safely.

Parameter(s):
  _elements: pointer to the elements to be freed
Return:
  none
*/
D_INLINE void
d_array_common_free_elements_arr
(
    void* _elements
)
{
    if (_elements)
    {
        free(_elements);

        return;
    }

    return;
}

/*
d_array_common_free_elements_deep
  Free container elements safely using a custom free function.

Parameter(s):
  _count:    number of elements to free
  _elements: pointer to elements array to be freed
  _free_fn:  function to free each element
Return:
  none
*/
void
d_array_common_free_elements_deep
(
    size_t  _count, 
    void**  _elements, 
    fn_free _free_fn
)
{
    size_t i;

    if ( (!_count)    ||
         (!_elements) || 
         (!_free_fn) )
    {
        return;
    }

    // Free each element
    for (i = 0; i < _count; i++) 
    {
        if (_elements[i])
        {
            _free_fn(_elements[i]);

            _elements[i] = NULL;  // Optional: set to NULL after freeing
        }
    }

    // free the array itself
    free(_elements);

    return;
}