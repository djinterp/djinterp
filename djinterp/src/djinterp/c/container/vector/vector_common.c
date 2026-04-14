#include "../../../../inc/c/container/vector/vector_common.h"


// =============================================================================
// initialization functions
// =============================================================================

/*
d_vector_common_init
  Initializes a vector with the specified initial capacity.

Parameter(s):
  _elements:         pointer to elements pointer to be initialized
  _count:            pointer to count variable to be set
  _capacity:         pointer to capacity variable to be set
  _element_size:     size in bytes of each element
  _initial_capacity: initial capacity to allocate
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_vector_common_init
(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _initial_capacity
)
{
    if ( (!_elements) ||
         (!_count)    ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    if (_initial_capacity == 0)
    {
        *(_elements) = NULL;
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_SUCCESS;
    }

    *(_elements) = calloc(_initial_capacity, _element_size);

    // ensure that memory allocation was successful
    if (!*(_elements))
    {
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_FAILURE;
    }

    *(_count)    = 0;
    *(_capacity) = _initial_capacity;

    return D_SUCCESS;
}

/*
d_vector_common_init_from_array
  Initializes a vector by copying elements from an existing array.

Parameter(s):
  _destination:  pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _capacity:     pointer to capacity variable to be set
  _element_size: size in bytes of each element
  _source:       pointer to source array to copy from
  _source_count: number of elements in the source array
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_vector_common_init_from_array
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    size_t alloc_capacity;

    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    if ( (!_source) || 
         (_source_count == 0) )
    {
        *(_destination) = NULL;
        *(_count)       = 0;
        *(_capacity)    = 0;

        return D_SUCCESS;
    }

    // allocate with some extra capacity for future growth
    alloc_capacity = d_array_common_calc_capacity(_source_count);

    *(_destination) = malloc(alloc_capacity * _element_size);

    // ensure that memory allocation was successful
    if (!*(_destination))
    {
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_FAILURE;
    }

    if (!d_memcpy(*(_destination),
                  _source,
                  (_source_count * _element_size)) )
    {

    }

    *(_count)    = _source_count;
    *(_capacity) = alloc_capacity;

    return D_SUCCESS;
}

/*
d_vector_common_init_from_args
  Initializes a vector from variadic arguments.

Parameter(s):
  _elements:     pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _capacity:     pointer to capacity variable to be set
  _element_size: size in bytes of each element
  _arg_count:    number of variadic arguments
  _args:         va_list containing the variadic arguments
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_vector_common_init_from_args
(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _arg_count,
    va_list _args
)
{
    size_t alloc_capacity;
    size_t i;
    char*  elem_ptr;

    if ( (!_elements) ||
         (!_count)    ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // if arg_count is zero
    if (!_arg_count)
    {
        *(_elements) = NULL;
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_SUCCESS;
    }

    // allocate with some extra capacity for future growth
    alloc_capacity = d_array_common_calc_capacity(_arg_count);

    *(_elements) = malloc(alloc_capacity * _element_size);

    // ensure that memory allocation was successful
    if (!*(_elements))
    {
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_FAILURE;
    }

    elem_ptr = (char*)*(_elements);

    for (i = 0; i < _arg_count; i++)
    {
        // handle different element sizes appropriately
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

    *(_count)    = _arg_count;
    *(_capacity) = alloc_capacity;

    return D_SUCCESS;
}

/*
d_vector_common_init_copy
  Initializes a vector as a copy of another vector's data.

Parameter(s):
  _elements:        pointer to elements pointer to be initialized
  _count:           pointer to count variable to be set
  _capacity:        pointer to capacity variable to be set
  _element_size:    size in bytes of each element
  _source:          pointer to source elements to copy
  _source_count:    number of elements in the source
  _source_capacity: capacity of the source (preserved in copy)
Return:
  A boolean value corresponding to either:
  - true, if copy initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_vector_common_init_copy
(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count,
    size_t      _source_capacity
)
{
    size_t alloc_capacity;

    if ( (!_elements) ||
         (!_count)    ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    if ( (!_source) ||
         (_source_count == 0) )
    {
        *(_elements) = NULL;
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_SUCCESS;
    }

    // use source capacity if provided and valid, otherwise calculate
    alloc_capacity = (_source_capacity >= _source_count)
                         ? _source_capacity
                         : d_array_common_calc_capacity(_source_count);

    *(_elements) = malloc(alloc_capacity * _element_size);

    // ensure that memory allocation was successful
    if (!*(_elements))
    {
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_FAILURE;
    }

    d_memcpy(*(_elements), _source, _source_count * _element_size);

    *(_count)    = _source_count;
    *(_capacity) = alloc_capacity;

    return D_SUCCESS;
}

/*
d_vector_common_init_fill
  Initializes a vector filled with a specified value.

Parameter(s):
  _elements:     pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _capacity:     pointer to capacity variable to be set
  _element_size: size in bytes of each element
  _size:         number of elements to create
  _value:        pointer to value to fill with
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed or parameters are invalid.
*/
bool
d_vector_common_init_fill
(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    size_t      _size,
    const void* _value
)
{
    size_t alloc_capacity;
    size_t i;
    char*  elem_ptr;

    if ( (!_elements) ||
         (!_count)    ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    if (_size == 0)
    {
        *(_elements) = NULL;
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_SUCCESS;
    }

    if (!_value)
    {
        return D_FAILURE;
    }

    // allocate with some extra capacity for future growth
    alloc_capacity = d_array_common_calc_capacity(_size);

    // ensure that memory allocation was successful
    *(_elements) = malloc(alloc_capacity * _element_size);

    if (!*(_elements))
    {
        *(_count)    = 0;
        *(_capacity) = 0;

        return D_FAILURE;
    }

    elem_ptr = (char*)*(_elements);

    for (i = 0; i < _size; i++)
    {
        d_memcpy(elem_ptr + (i * _element_size),
                 _value, 
                 _element_size);
    }

    *(_count)    = _size;
    *(_capacity) = alloc_capacity;

    return D_SUCCESS;
}


// =============================================================================
// capacity management functions
// =============================================================================

/*
d_vector_common_reserve
  Reserves storage capacity for at least the specified number of elements.
Reallocation only occurs if _new_capacity exceeds the current capacity.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        current number of elements
  _capacity:     pointer to capacity variable (updated if reallocated)
  _element_size: size in bytes of each element
  _new_capacity: minimum capacity to reserve
Return:
  A boolean value corresponding to either:
  - true, if reservation was successful (or already sufficient), or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_reserve
(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _new_capacity
)
{
    void* new_elements;

    if ( (!_elements) ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // already have sufficient capacity
    if (_new_capacity <= *(_capacity))
    {
        return D_SUCCESS;
    }

    new_elements = realloc(*(_elements), _new_capacity * _element_size);

    // ensure that memory reallocation was susccessful
    if (!new_elements)
    {
        return D_FAILURE;
    }

    *(_elements) = new_elements;
    *(_capacity) = _new_capacity;

    return D_SUCCESS;
}

/*
d_vector_common_shrink_to_fit
  Reduces capacity to match the current count, freeing unused memory.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        current number of elements
  _capacity:     pointer to capacity variable (updated if reallocated)
  _element_size: size in bytes of each element
Return:
  A boolean value corresponding to either:
  - true, if shrink was successful (or no change needed), or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_shrink_to_fit
(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size
)
{
    void* new_elements;

    if ( (!_elements) ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // already at minimum size
    if (*(_capacity) == _count)
    {
        return D_SUCCESS;
    }

    // handle empty vector case
    if (_count == 0)
    {
        if (*(_elements))
        {
            free(*(_elements));

            *(_elements) = NULL;
        }

        *(_capacity) = 0;

        return D_SUCCESS;
    }

    new_elements = realloc(*(_elements), _count * _element_size);

    // ensure that memory reallocation was susccessful
    if (!new_elements)
    {
        return D_FAILURE;
    }

    *(_elements) = new_elements;
    *(_capacity) = _count;

    return D_SUCCESS;
}

/*
d_vector_common_ensure_capacity
  Ensures the vector has at least the required capacity, growing if necessary.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        current number of elements
  _capacity:     pointer to capacity variable (updated if reallocated)
  _element_size: size in bytes of each element
  _required:     minimum capacity required
Return:
  A boolean value corresponding to either:
  - true, if sufficient capacity exists or was allocated, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_ensure_capacity
(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _required
)
{
    size_t new_capacity;

    if ( (!_elements)         ||
         (!_capacity)         ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // already have sufficient capacity
    if (_required <= *(_capacity))
    {
        return D_SUCCESS;
    }

    // calculate new capacity using growth factor
    new_capacity = *(_capacity);

    if (new_capacity == 0)
    {
        new_capacity = D_VECTOR_DEFAULT_CAPACITY;
    }

    while (new_capacity < _required)
    {
        // check for overflow before multiplying
        if (new_capacity > SIZE_MAX / D_VECTOR_GROWTH_FACTOR)
        {
            new_capacity = _required;

            break;
        }

        new_capacity = (size_t)(new_capacity * D_VECTOR_GROWTH_FACTOR);
    }

    return d_vector_common_reserve(_elements,
                                   _count,
                                   _capacity,
                                   _element_size,
                                   new_capacity);
}

/*
d_vector_common_grow
  Increases the vector's capacity by the growth factor.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        current number of elements
  _capacity:     pointer to capacity variable (updated if reallocated)
  _element_size: size in bytes of each element
Return:
  A boolean value corresponding to either:
  - true, if growth was successful, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_grow
(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size
)
{
    size_t new_capacity;

    if ( (!_elements) ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    if (*(_capacity) == 0)
    {
        new_capacity = D_VECTOR_DEFAULT_CAPACITY;
    }
    else
    {
        // check for overflow
        if (*(_capacity) > SIZE_MAX / D_VECTOR_GROWTH_FACTOR)
        {
            return D_FAILURE;
        }

        new_capacity = (size_t)(*(_capacity) * D_VECTOR_GROWTH_FACTOR);
    }

    return d_vector_common_reserve(_elements,
                                   _count,
                                   _capacity,
                                   _element_size,
                                   new_capacity);
}

/*
d_vector_common_maybe_shrink
  Shrinks the vector's capacity if usage falls below the shrink threshold.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        current number of elements
  _capacity:     pointer to capacity variable (updated if reallocated)
  _element_size: size in bytes of each element
Return:
  A boolean value corresponding to either:
  - true, if no shrink needed or shrink was successful, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_maybe_shrink
(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size
)
{
    size_t new_capacity;
    void*  new_elements;
    double usage_ratio;

    if ( (!_elements) ||
         (!_capacity) ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // don't shrink if capacity is already at or below minimum
    if (*(_capacity) <= D_VECTOR_MIN_CAPACITY)
    {
        return D_SUCCESS;
    }

    // calculate usage ratio
    usage_ratio = (double)_count / (double)*(_capacity);

    // don't shrink if usage is above threshold
    if (usage_ratio >= D_VECTOR_SHRINK_THRESHOLD)
    {
        return D_SUCCESS;
    }

    // calculate new capacity (half of current, but not below minimum or count)
    new_capacity = *(_capacity) / 2;

    if (new_capacity < D_VECTOR_MIN_CAPACITY)
    {
        new_capacity = D_VECTOR_MIN_CAPACITY;
    }

    if (new_capacity < _count)
    {
        new_capacity = _count;
    }

    // no actual change needed
    if (new_capacity >= *(_capacity))
    {
        return D_SUCCESS;
    }

    new_elements = realloc(*(_elements), new_capacity * _element_size);

    // ensure that memory reallocation was susccessful
    if (!new_elements)
    {
        // shrinking failed, but vector is still valid
        return D_SUCCESS;
    }

    *(_elements) = new_elements;
    *(_capacity) = new_capacity;

    return D_SUCCESS;
}

/*
d_vector_common_available
  Returns the number of elements that can be added without reallocation.

Parameter(s):
  _count:    current number of elements
  _capacity: current capacity
Return:
  The number of available slots (capacity - count).
*/
D_INLINE size_t
d_vector_common_available
(
    size_t _count,
    size_t _capacity
)
{
    return (_capacity > _count)
        ? (_capacity - _count)
        : 0;
}


// =============================================================================
// element manipulation functions
// =============================================================================

/*
d_vector_common_push_back
  Appends an element to the end of the vector, growing capacity if needed.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (incremented on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _value:        pointer to value to append
Return:
  A boolean value corresponding to either:
  - true, if element was successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_push_back
(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _value
)
{
    if ( (!_elements)     ||
         (!_count)        ||
         (!_capacity)     ||
         (!_element_size) ||
         (!_value)        ||
         (!d_vector_common_ensure_capacity(_elements,
                                           *(_count),
                                           _capacity,
                                           _element_size,
                                           *(_count) + 1)) )
    {
        return D_FAILURE;
    }

    d_memcpy((char*)*(_elements)+(*(_count)*_element_size),
             _value,
             _element_size);

    (*_count)++;

    return D_SUCCESS;
}

/*
d_vector_common_push_front
  Prepends an element to the beginning of the vector, growing capacity if 
needed.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (incremented on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _value:        pointer to value to prepend
Return:
  A boolean value corresponding to either:
  - true, if element was successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_push_front
(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _value
)
{
    char* elem_ptr;

    if ( (!_elements)         ||
         (!_count)            ||
         (!_capacity)         ||
         (!_element_size) ||
         (!_value) )
    {
        return D_FAILURE;
    }

    // ensure we have space for one more element
    if (!d_vector_common_ensure_capacity(_elements,
                                         *(_count),
                                         _capacity,
                                         _element_size,
                                         *(_count) + 1))
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)*(_elements);

    // shift all existing elements right by one position
    if (*(_count) > 0)
    {
        memmove(elem_ptr + _element_size, elem_ptr, *(_count) * _element_size);
    }

    // copy the new element to the front
    d_memcpy(elem_ptr, _value, _element_size);

    (*_count)++;

    return D_SUCCESS;
}

/*
d_vector_common_pop_back
  Removes and optionally returns the last element of the vector.

Parameter(s):
  _elements:     pointer to elements array
  _count:        pointer to count variable (decremented on success)
  _element_size: size in bytes of each element
  _out_value:    optional pointer to receive the removed element (may be NULL)
Return:
  A boolean value corresponding to either:
  - true, if element was successfully removed, or
  - false, if vector is empty or parameters are invalid.
*/
bool
d_vector_common_pop_back
(
    void*  _elements,
    size_t* _count,
    size_t  _element_size,
    void*   _out_value
)
{
    char* elem_ptr;

    if ( (!_elements)         ||
         (!_count)            ||
         (!_element_size) ||
         (*(_count) == 0) )
    {
        return D_FAILURE;
    }

    // copy the last element to output if requested
    if (_out_value)
    {
        elem_ptr = (char*)_elements + ((*(_count) - 1) * _element_size);
        d_memcpy(_out_value, elem_ptr, _element_size);
    }

    (*_count)--;

    return D_SUCCESS;
}

/*
d_vector_common_pop_front
  Removes and optionally returns the first element of the vector.

Parameter(s):
  _elements:     pointer to elements array
  _count:        pointer to count variable (decremented on success)
  _element_size: size in bytes of each element
  _out_value:    optional pointer to receive the removed element (may be NULL)
Return:
  A boolean value corresponding to either:
  - true, if element was successfully removed, or
  - false, if vector is empty or parameters are invalid.
*/
bool
d_vector_common_pop_front
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    void*   _out_value
)
{
    char* elem_ptr;

    if ( (!_elements)         ||
         (!_count)            ||
         (!_element_size) ||
         (*(_count) == 0) )
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)_elements;

    // copy the first element to output if requested
    if (_out_value)
    {
        d_memcpy(_out_value, elem_ptr, _element_size);
    }

    // shift all remaining elements left by one position
    if (*(_count) > 1)
    {
        memmove(elem_ptr, elem_ptr + _element_size, (*(_count) - 1) * _element_size);
    }

    (*_count)--;

    return D_SUCCESS;
}

/*
d_vector_common_insert
  Inserts an element at the specified index, growing capacity if needed.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (incremented on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _index:        index where to insert (supports negative indexing)
  _value:        pointer to value to insert
Return:
  A boolean value corresponding to either:
  - true, if element was successfully inserted, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_insert_element
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    d_index     _index,
    const void* _value
)
{
    size_t insert_idx;
    char*  elem_ptr;

    if ( (!_destination)  ||
         (!_count)        ||
         (!_capacity)     ||
         (!_element_size) ||
         (!_value)        ||
         // convert negative index to positive
         (!d_index_convert_safe(_index,
                               (*(_count) + 1),
                               &(insert_idx))) ||
         (!d_vector_common_ensure_capacity(_destination,
                                           *(_count),
                                           _capacity,
                                           _element_size,
                                           *(_count) + 1)) )
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)*(_destination);

    // shift elements to the right to make space
    if (insert_idx < *(_count))
    {
        memmove(elem_ptr + ((insert_idx + 1) * _element_size),
                elem_ptr + (insert_idx * _element_size),
                (*(_count) - insert_idx) * _element_size);
    }

    // insert the new element
    if (!d_memcpy(elem_ptr + (insert_idx * _element_size),
                  _value,
                  _element_size))
    {
        return D_FAILURE;
    }

    (*_count)++;

    return D_SUCCESS;
}

/*
d_vector_common_insert_elements
  Inserts multiple elements at the specified index, growing capacity if needed.

Parameter(s):
  _destination:  pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _index:        index where to insert (supports negative indexing)
  _source:       pointer to source array to insert
  _source_count: number of elements to insert
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully inserted, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_insert_elements
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    d_index     _index,
    const void* _source,
    size_t      _source_count
)
{
    size_t insert_idx;
    char*  elem_ptr;

    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // inserting zero elements is a no-op success
    if ( (_source_count == 0) ||
         (!_source) )
    {
        return (_source_count == 0);
    }

    // convert negative index to positive, ensure index is valid
    // also ensure that there is sufficient space
    if ( (!d_index_convert_safe(_index, *(_count) + 1, &insert_idx)) ||
         (!d_vector_common_ensure_capacity(_destination,
                                           *(_count),
                                           _capacity,
                                           _element_size,
                                           *(_count) + _source_count)) )
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)*(_destination);

    // shift elements to the right to make space
    if (insert_idx < *(_count))
    {
        memmove(elem_ptr + ((insert_idx + _source_count) * _element_size),
                elem_ptr + (insert_idx * _element_size),
                (*(_count) - insert_idx) * _element_size);
    }

    // insert the new elements
    if (!d_memcpy(elem_ptr + (insert_idx * _element_size),
                  _source,
                  _source_count * _element_size) )
    {
        return D_FAILURE;
    }

    *(_count) += _source_count;

    return D_SUCCESS;
}

/*
d_vector_common_erase
  Removes an element at the specified index.

Parameter(s):
  _elements:     pointer to elements array
  _count:        pointer to count variable (decremented on success)
  _element_size: size in bytes of each element
  _index:        index of element to remove (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if element was successfully removed, or
  - false, if index is invalid or parameters are invalid.
*/
bool
d_vector_common_erase
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    d_index _index
)
{
    size_t erase_idx;
    char*  elem_ptr;

    if ( (!_elements)         ||
         (!_count)            ||
         (!_element_size) ||
         (*(_count) == 0) )
    {
        return D_FAILURE;
    }

    // convert negative index to positive
    if (!d_index_convert_safe(_index, *(_count), &erase_idx))
    {
        return D_FAILURE;
    }

    elem_ptr = (char*)_elements;

    // shift elements left to fill the gap
    if (erase_idx < *(_count) - 1)
    {
        memmove(elem_ptr + (erase_idx * _element_size),
                elem_ptr + ((erase_idx + 1) * _element_size),
                (*(_count) - erase_idx - 1) * _element_size);
    }

    (*_count)--;

    return D_SUCCESS;
}

/*
d_vector_common_erase_range
  Removes elements in the specified index range [_start, _end].

Parameter(s):
  _elements:     pointer to elements array
  _count:        pointer to count variable (updated on success)
  _element_size: size in bytes of each element
  _start:        starting index (supports negative indexing)
  _end:          ending index, inclusive (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully removed, or
  - false, if indices are invalid or parameters are invalid.
*/
bool
d_vector_common_erase_range
(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    d_index _start,
    d_index _end
)
{
    size_t start_idx;
    size_t end_idx;
    size_t erase_count;
    char*  elem_ptr;

    if ( (!_elements)         ||
         (!_count)            ||
         (!_element_size) ||
         (*(_count) == 0) )
    {
        return D_FAILURE;
    }

    // convert negative indices to positive
    if ( (!d_index_convert_safe(_start, *(_count), &start_idx)) ||
         (!d_index_convert_safe(_end, *(_count), &end_idx)) )
    {
        return D_FAILURE;
    }

    // validate range
    if (start_idx > end_idx)
    {
        return D_FAILURE;
    }

    erase_count = end_idx - start_idx + 1;
    elem_ptr    = (char*)_elements;

    // shift elements left to fill the gap
    if (end_idx < *(_count) - 1)
    {
        memmove(elem_ptr + (start_idx * _element_size),
                elem_ptr + ((end_idx + 1) * _element_size),
                (*(_count) - end_idx - 1) * _element_size);
    }

    *(_count) -= erase_count;

    return D_SUCCESS;
}


// =============================================================================
// append/extend functions
// =============================================================================

/*
d_vector_common_append_element
  Appends a single element to the end of the array of elements provided.

Parameter(s):
  _element:      pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _source:       pointer to source array to append
Return:
  A boolean value corresponding to either:
  - D_SUCCESS, if elements were successfully appended, or
  - D_FAILURE, if parameters are invalid, or if memory reallocation failed.
*/
bool
d_vector_common_append_element
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _element
)
{
    // all parameters must be non-NULL or non-zero EXCEPT `_element`, and there
    // must be sufficient space
    if ( (!_destination)  ||
         (!_count)        ||
         (!_capacity)     ||
         (!_element_size) ||
         (!d_vector_common_ensure_capacity(_destination,
                                           *(_count),
                                           _capacity,
                                           _element_size,
                                           (*(_count) + 1))) )
    {
        return D_FAILURE;
    }

    // ensure that memory copying was successful
    if (!d_memcpy((char*)*(_destination) + (*(_count) * _element_size),
                  _element, 
                  _element_size))
    {
        return D_FAILURE;
    }

    *(_count) += 1;

    return D_SUCCESS;
}

/*
d_vector_common_append_elements
  Appends a one or more elements to the end of the `d_vector` provided.

Parameter(s):
  _destination:  pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _source:       pointer to source array to append
  _source_count: number of elements to append
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully appended, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_append_elements
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    // all parameters must be non-NULL or non-zero
    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // appending zero elements is a no-op success
    if ( (!_source) || 
         (!_source_count) )
    {
        return (_source_count == 0);
    }

    // ensure there is space for the new elements
    if (!d_vector_common_ensure_capacity(_destination,
                                         *(_count),
                                         _capacity,
                                         _element_size,
                                         (*(_count) + _source_count)) )
    {
        return D_FAILURE;
    }

    // ensure that memory copying was successful
    if (!d_memcpy((char*)*(_destination) + (*(_count) * _element_size),
                  _source, 
                  (_source_count * _element_size)))
    {
        return D_FAILURE;
    }

    // increment count
    *(_count) += _source_count;

    return D_SUCCESS;
}

/*
d_vector_common_prepend_element
  Prepends a single element to the beginning of the array of elements provided.

Parameter(s):
  _element:      pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _source:       pointer to source array to append
Return:
  A boolean value corresponding to either:
  - D_SUCCESS, if elements were successfully appended, or
  - D_FAILURE, if parameters are invalid, or if memory reallocation failed.
*/
D_INLINE bool
d_vector_common_prepend_element
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _element
)
{
    return d_vector_common_insert_element(_destination,
                                          _count,
                                          _capacity,
                                          _element_size,
                                          0,
                                          _element);
}

/*
d_vector_common_prepend_elements
  Prepends multiple elements to the beginning of the vector.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _source:       pointer to source array to prepend
  _source_count: number of elements to prepend
Return:
  A boolean value corresponding to either:
  - true, if elements were successfully prepended, or
  - false, if reallocation failed or parameters are invalid.
*/
D_INLINE bool
d_vector_common_prepend_elements
(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    return d_vector_common_insert_elements(_elements,
                                        _count,
                                        _capacity,
                                        _element_size,
                                        0,
                                        _source,
                                        _source_count);
}


// =============================================================================
// resize functions
// =============================================================================

/*
d_vector_common_resize
  Resizes the vector to contain exactly _new_count elements. If growing, new
elements are zero-initialized.

Parameter(s):
  _elements:     pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _new_count:    new number of elements
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_resize
(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _new_count
)
{
    char* elem_ptr;

    if ( (!_elements)         ||
         (!_count)            ||
         (!_capacity)         ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // shrinking: just update count
    if (_new_count <= *(_count))
    {
        *(_count) = _new_count;

        return D_SUCCESS;
    }

    // growing: ensure capacity and zero-initialize new elements
    if (!d_vector_common_ensure_capacity(_elements,
                                         *(_count),
                                         _capacity,
                                         _element_size,
                                         _new_count))
    {
        return D_FAILURE;
    }

    // zero-initialize the new elements
    elem_ptr = (char*)*(_elements) + (*(_count) * _element_size);
    d_memset(elem_ptr, 0, (_new_count - *(_count)) * _element_size);

    *(_count) = _new_count;

    return D_SUCCESS;
}

/*
d_vector_common_resize_fill
  Resizes the vector to contain exactly _new_count elements. If growing, new
elements are initialized with the specified fill value.

Parameter(s):
  _destination:  pointer to elements pointer (may be reallocated)
  _count:        pointer to count variable (updated on success)
  _capacity:     pointer to capacity variable (may be updated)
  _element_size: size in bytes of each element
  _new_count:    new number of elements
  _fill_value:   pointer to value to fill new elements with
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if reallocation failed or parameters are invalid.
*/
bool
d_vector_common_resize_fill
(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    size_t      _new_count,
    const void* _fill_value
)
{
    size_t i;
    char*  elem_ptr;

    if ( (!_destination) ||
         (!_count)       ||
         (!_capacity)    ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // shrinking: just update count
    if (_new_count <= *(_count))
    {
        *(_count) = _new_count;

        return D_SUCCESS;
    }

    // fill value required when growing
    // growing: ensure capacity and fill new elements
    if ( (!_fill_value) ||
         (!d_vector_common_ensure_capacity(_destination,
                                           *(_count),
                                           _capacity,
                                           _element_size,
                                           _new_count)) )
    {
        return D_FAILURE;
    }

    // fill the new elements with the specified value
    elem_ptr = (char*)*(_destination);

    for (i = *(_count); i < _new_count; i++)
    {
        if (!d_memcpy(elem_ptr + (i * _element_size),
                      _fill_value,
                      _element_size))
        {
            return D_FAILURE;
        }
    }

    *(_count) = _new_count;

    return D_SUCCESS;
}

// =============================================================================
// access functions
// =============================================================================

/*
d_vector_common_at
  Returns a pointer to the element at the specified index.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: size in bytes of each element
  _index:        index of element to access (supports negative indexing)
Return:
  Pointer to the element, or NULL if index is out of bounds.
*/
void*
d_vector_common_at
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    d_index     _index
)
{
    size_t actual_idx;

    if ( (!_elements)         ||
         (!_element_size) ||
         (_count == 0) )
    {
        return NULL;
    }

    // convert negative index to positive
    if (!d_index_convert_safe(_index, _count, &actual_idx))
    {
        return NULL;
    }

    return (char*)_elements + (actual_idx * _element_size);
}

/*
d_vector_common_front
  Returns a pointer to the first element of the vector.

Parameter(s):
  _elements: pointer to elements array
  _count:    number of elements
Return:
  Pointer to the first element, or NULL if vector is empty.
*/
void*
d_vector_common_front
(
    const void* _elements,
    size_t      _count
)
{
    if ( (!_elements) ||
         (_count == 0) )
    {
        return NULL;
    }

    return (void*)_elements;
}

/*
d_vector_common_back
  Returns a pointer to the last element of the vector.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: size in bytes of each element
Return:
  Pointer to the last element, or NULL if vector is empty.
*/
void*
d_vector_common_back
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
)
{
    if ( (!_elements)         ||
         (_count == 0)        ||
         (!_element_size) )
    {
        return NULL;
    }

    return (char*)_elements + ((_count - 1) * _element_size);
}

/*
d_vector_common_data
  Returns a pointer to the underlying array data.

Parameter(s):
  _elements: pointer to elements array
Return:
  Pointer to the underlying array, or NULL if elements is NULL.
*/
void*
d_vector_common_data
(
    const void* _elements
)
{
    return (void*)_elements;
}


// =============================================================================
// query functions
// =============================================================================

/*
d_vector_common_is_empty
  Checks if the vector contains no elements.

Parameter(s):
  _count: number of elements
Return:
  A boolean value corresponding to either:
  - true, if the vector is empty (count == 0), or
  - false, if the vector contains elements.
*/
bool
d_vector_common_is_empty
(
    size_t _count
)
{
    return (_count == 0);
}

/*
d_vector_common_is_full
  Checks if the vector's count equals its capacity.

Parameter(s):
  _count:    number of elements
  _capacity: current capacity
Return:
  A boolean value corresponding to either:
  - true, if count equals capacity, or
  - false, if there is available capacity.
*/
bool
d_vector_common_is_full
(
    size_t _count,
    size_t _capacity
)
{
    return (_count >= _capacity);
}

/*
d_vector_common_size
  Returns the number of elements in the vector.

Parameter(s):
  _count: number of elements
Return:
  The number of elements.
*/
size_t
d_vector_common_size
(
    size_t _count
)
{
    return _count;
}

/*
d_vector_common_capacity
  Returns the current capacity of the vector.

Parameter(s):
  _capacity: current capacity
Return:
  The current capacity.
*/
size_t
d_vector_common_capacity
(
    size_t _capacity
)
{
    return _capacity;
}


// =============================================================================
// utility functions
// =============================================================================

/*
d_vector_common_swap
  Swaps two elements in the vector.

Parameter(s):
  _elements:     pointer to elements array
  _count:        number of elements
  _element_size: size in bytes of each element
  _index_a:      index of first element (supports negative indexing)
  _index_b:      index of second element (supports negative indexing)
Return:
  A boolean value corresponding to either:
  - true, if swap was successful, or
  - false, if indices are invalid or parameters are invalid.
*/
bool
d_vector_common_swap
(
    void*   _elements,
    size_t  _count,
    size_t  _element_size,
    d_index _index_a,
    d_index _index_b
)
{
    size_t idx_a;
    size_t idx_b;
    char*  elem_a;
    char*  elem_b;
    char*  temp;

    if ( (!_elements)         ||
         (!_element_size) ||
         (_count == 0) )
    {
        return D_FAILURE;
    }

    // convert negative indices to positive
    if ( (!d_index_convert_safe(_index_a, _count, &idx_a)) ||
         (!d_index_convert_safe(_index_b, _count, &idx_b)) )
    {
        return D_FAILURE;
    }

    // same index: no swap needed
    if (idx_a == idx_b)
    {
        return D_SUCCESS;
    }

    temp = malloc(_element_size);

    if (!temp)
    {
        return D_FAILURE;
    }

    elem_a = (char*)_elements + (idx_a * _element_size);
    elem_b = (char*)_elements + (idx_b * _element_size);

    d_memcpy(temp, elem_a, _element_size);
    d_memcpy(elem_a, elem_b, _element_size);
    d_memcpy(elem_b, temp, _element_size);

    free(temp);

    return D_SUCCESS;
}

/*
d_vector_common_copy_to
  Copies the vector's elements to a destination buffer.

Parameter(s):
  _source:        pointer to source elements
  _count:         number of elements to copy
  _element_size:  size in bytes of each element
  _destination:   pointer to destination buffer
  _dest_capacity: capacity of destination buffer (in elements)
Return:
  A boolean value corresponding to either:
  - true, if copy was successful, or
  - false, if destination is too small or parameters are invalid.
*/
bool
d_vector_common_copy_to
(
    const void* _source,
    size_t      _count,
    size_t      _element_size,
    void*       _destination,
    size_t      _dest_capacity
)
{
    if ( (!_destination)      ||
         (!_element_size) )
    {
        return D_FAILURE;
    }

    // nothing to copy
    if ( (_count == 0) ||
         (!_source) )
    {
        return D_SUCCESS;
    }

    // destination too small
    if (_dest_capacity < _count)
    {
        return D_FAILURE;
    }

    d_memcpy(_destination, _source, _count * _element_size);

    return D_SUCCESS;
}


// =============================================================================
// cleanup functions
// =============================================================================

/*
d_vector_common_free_elements
  Frees the elements array.

Parameter(s):
  _elements: pointer to elements array to free
Return:
  none
*/
void
d_vector_common_free_elements
(
    void* _elements
)
{
    if (_elements)
    {
        free(_elements);
    }

    return;
}