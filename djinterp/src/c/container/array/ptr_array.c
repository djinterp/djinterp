#include "../../../../inc/c/container/array/ptr_array.h"


// =============================================================================
// constructor functions
// =============================================================================

/*
d_ptr_array_new
  Allocate and initialize a new `d_ptr_array` with specified initial capacity.

Parameter(s):
  _initial_size: initial capacity to allocate for the array.
Return:
  A pointer to the newly created `d_ptr_array`, or NULL if allocation failed.
*/
struct d_ptr_array*
d_ptr_array_new
(
    size_t _initial_size
)
{
    struct d_ptr_array* arr;

    arr = d_array_common_alloc(sizeof(struct d_ptr_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_sized(arr->elements,
                                   &(arr->count),
                                   sizeof(void*),
                                   _initial_size))
    {
        free(arr);

        return NULL;
    }

    return arr;
}


/*
d_ptr_array_new_default_size
  Allocate and initialize a new `d_ptr_array` with default capacity.

Parameter(s):
  none
Return:
  A pointer to the newly created `d_ptr_array`, or NULL if allocation failed.
*/
D_INLINE struct d_ptr_array*
d_ptr_array_new_default_size
(
    void
)
{
    return d_ptr_array_new(D_PTR_ARRAY_DEFAULT_CAPACITY);
}


/*
d_ptr_array_new_from_arr
  Allocate and initialize a new `d_ptr_array` by copying pointers from an 
existing array.

Parameter(s):
  _source: pointer to the source array of pointers to copy from.
  _count:  number of pointers in the source array.
Return:
  A pointer to the newly created `d_ptr_array`, or NULL if allocation failed.
*/
struct d_ptr_array*
d_ptr_array_new_from_arr
(
    const void** _source,
    size_t       _count
)
{
    struct d_ptr_array* arr;

    if ( (!_source) &&
         (_count > 0) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_ptr_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_from_array(&(arr->elements),
                                        &(arr->count),
                                        sizeof(void*),
                                        _source,
                                        _count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}


/*
d_ptr_array_new_from_args
  Allocate and initialize a new `d_ptr_array` from variadic pointer arguments.

Parameter(s):
  _arg_count: number of variadic pointer arguments to process.
  ...:        variadic arguments containing pointers.
Return:
  A pointer to the newly created `d_ptr_array`, or NULL if allocation failed.
*/
struct d_ptr_array*
d_ptr_array_new_from_args
(
    size_t _arg_count,
    ...
)
{
    bool                success;
    va_list             args;
    struct d_ptr_array* arr;

    arr = d_array_common_alloc(sizeof(struct d_ptr_array));

    if (!arr)
    {
        return NULL;
    }

    va_start(args, _arg_count);

    success = d_array_common_init_from_args(&(arr->elements),
                                            &(arr->count),
                                            sizeof(void*),
                                            _arg_count,
                                            args);
    va_end(args);

    if (!success)
    {
        free(arr);

        return NULL;
    }

    return arr;
}


/*
d_ptr_array_new_copy
  Allocate and initialize a new `d_ptr_array` by copying another `d_ptr_array`.
  This is a shallow copy - the pointers are copied, not the pointed-to data.

Parameter(s):
  _other: pointer to the `d_ptr_array` to copy from.
Return:
  A pointer to the newly created `d_ptr_array` copy, or NULL if allocation 
  failed.
*/
struct d_ptr_array*
d_ptr_array_new_copy
(
    const struct d_ptr_array* _other
)
{
    struct d_ptr_array* arr;

    if (!_other)
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_ptr_array));

    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy(&(arr->elements),
                                  &(arr->count),
                                  sizeof(void*),
                                  _other->elements,
                                  _other->count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}


/*
d_ptr_array_new_merge
  Allocate and initialize a new `d_ptr_array` by merging multiple pointer 
arrays.

Parameter(s):
  _count: number of arrays to merge.
  ...:    variadic arguments containing pointers to `d_ptr_array` structures.
Return:
  A pointer to the newly created merged `d_ptr_array`, or NULL if allocation 
  failed.
*/
struct d_ptr_array*
d_ptr_array_new_merge
(
    size_t _count,
    ...
)
{
    struct d_ptr_array* result;
    struct d_ptr_array* current;
    va_list             args;
    size_t              total_size;
    size_t              current_offset;
    size_t              i;

    if (_count == 0)
    {
        return d_ptr_array_new_default_size();
    }

    // first pass: calculate total size needed
    total_size = 0;

    va_start(args, _count);

    for (i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_ptr_array*);

        if (current)
        {
            total_size += current->count;
        }
    }

    va_end(args);

    result = d_ptr_array_new(total_size);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // second pass: copy all elements
    current_offset = 0;

    va_start(args, _count);

    for (i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_ptr_array*);

        if ( (current) &&
             (current->count > 0) )
        {
            d_memcpy((char*)result->elements + (current_offset * sizeof(void*)),
                     current->elements,
                     current->count * sizeof(void*));

            current_offset += current->count;
        }
    }

    va_end(args);

    result->count = total_size;

    return result;
}


/*
d_ptr_array_new_slice
  Allocate and initialize a new `d_ptr_array` as a slice starting from the 
specified index.

Parameter(s):
  _ptr_array: pointer to the source `d_ptr_array`.
  _start:     starting index for the slice (supports negative indexing).
Return:
  A pointer to the newly created `d_ptr_array` slice, or NULL if allocation 
  failed.
*/
struct d_ptr_array*
d_ptr_array_new_slice
(
    const struct d_ptr_array* _ptr_array,
    d_index                   _start
)
{
    struct d_ptr_array* arr;
    size_t              start_idx;
    size_t              slice_count;

    if (!_ptr_array)
    {
        return NULL;
    }

    // convert negative index to positive
    if (!d_index_convert_safe(_start, _ptr_array->count, &start_idx))
    {
        return NULL;
    }

    // calculate slice length
    slice_count = (_ptr_array->count - start_idx);

    arr = d_array_common_alloc(sizeof(struct d_ptr_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_from_array(&(arr->elements),
                                        &(arr->count),
                                        sizeof(void*),
                                        (const void*)(_ptr_array->elements + start_idx),
                                        slice_count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}


/*
d_ptr_array_new_slice_range
  Allocate and initialize a new `d_ptr_array` as a slice within the specified 
range.

Parameter(s):
  _ptr_array: pointer to the source `d_ptr_array`.
  _start:     starting index for the range (supports negative indexing).
  _end:       ending index for the range, inclusive (supports negative 
              indexing).
Return:
  A pointer to the newly created `d_ptr_array` slice, or NULL if allocation 
  failed.
*/
struct d_ptr_array*
d_ptr_array_new_slice_range
(
    const struct d_ptr_array* _ptr_array,
    d_index                   _start,
    d_index                   _end
)
{
    struct d_ptr_array* arr;
    size_t              start_idx;
    size_t              end_idx;
    size_t              slice_count;

    if (!_ptr_array)
    {
        return NULL;
    }

    // convert negative indices to positive
    if ( (!d_index_convert_safe(_start, _ptr_array->count, &start_idx)) ||
         (!d_index_convert_safe(_end, _ptr_array->count, &end_idx)) )
    {
        return NULL;
    }

    // validate range (end must be >= start)
    if (end_idx < start_idx)
    {
        return NULL;
    }

    // calculate slice length (inclusive of end)
    slice_count = (end_idx - start_idx + 1);

    arr = d_array_common_alloc(sizeof(struct d_ptr_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_from_array(&(arr->elements),
                                        &(arr->count),
                                        sizeof(void*),
                                        (const void*)(_ptr_array->elements + start_idx),
                                        slice_count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}


// =============================================================================
// element manipulation functions
// =============================================================================

/*
d_ptr_array_append_element
  Append a single pointer to the end of the array.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to modify.
  _element:   pointer to append.
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully appended, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_append_element
(
    struct d_ptr_array* _ptr_array,
    const void*         _element
)
{
    return (!_ptr_array)
        ? false
        : d_array_common_append_element((void**)&(_ptr_array->elements),
                                        &(_ptr_array->count),
                                        sizeof(void*),
                                        &_element);
}


/*
d_ptr_array_append_elements
  Append multiple pointers to the end of the array.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to modify.
  _elements:  pointer to the array of pointers to append.
  _count:     number of pointers to append.
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully appended, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_append_elements
(
    struct d_ptr_array* _ptr_array,
    const void**        _elements,
    size_t              _count
)
{
    return ( (!_ptr_array) ||
             ( (!_elements) &&
               (_count > 0) ) )
        ? false
        : d_array_common_append_elements((void**)&(_ptr_array->elements),
                                         &(_ptr_array->count),
                                         sizeof(void*),
                                         _elements,
                                         _count);
}


/*
d_ptr_array_append_array
  Append all pointers from another `d_ptr_array`.

Parameter(s):
  _ptr_array: pointer to the destination `d_ptr_array`.
  _source:    pointer to the source `d_ptr_array` to append from.
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully appended, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_append_array
(
    struct d_ptr_array*       _ptr_array,
    const struct d_ptr_array* _source
)
{
    return ( (!_ptr_array) ||
             (!_source) )
        ? false
        : d_ptr_array_append_elements(_ptr_array,
                                      (const void**)_source->elements,
                                      _source->count);
}


/*
d_ptr_array_contains
  Check if the `d_ptr_array` contains a specific pointer (by identity).

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to search.
  _value:     pointer value to search for.
Return:
  A boolean value corresponding to either:
  - true, if the pointer was found in the array, or
  - false, if the pointer was not found or parameters are invalid.
*/
bool
d_ptr_array_contains
(
    const struct d_ptr_array* _ptr_array,
    const void*               _value
)
{
    size_t i;

    if ( (!_ptr_array) ||
         (!_ptr_array->elements) )
    {
        return false;
    }

    for (i = 0; i < _ptr_array->count; i++)
    {
        if (_ptr_array->elements[i] == _value)
        {
            return true;
        }
    }

    return false;
}


/*
d_ptr_array_fill
  Fill the entire `d_ptr_array` with the specified pointer value.

Parameter(s):
  _ptr_array:    pointer to the `d_ptr_array` to fill.
  _fill_element: pointer value to fill the array with.
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully filled, or
  - false, if the operation failed.
*/
bool
d_ptr_array_fill
(
    struct d_ptr_array* _ptr_array,
    const void*         _fill_element
)
{
    size_t i;

    if (!_ptr_array)
    {
        return D_FAILURE;
    }

    for (i = 0; i < _ptr_array->count; i++)
    {
        _ptr_array->elements[i] = (void*)_fill_element;
    }

    return D_SUCCESS;
}


/*
d_ptr_array_find
  Find the index of the first occurrence of a pointer (by identity).

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to search.
  _value:     pointer value to search for.
Return:
  The index of the first occurrence, or -1 if not found or parameters are 
  invalid.
*/
ssize_t
d_ptr_array_find
(
    struct d_ptr_array* _ptr_array,
    const void*         _value
)
{
    size_t i;

    if ( (!_ptr_array) ||
         (!_ptr_array->elements) )
    {
        return -1;
    }

    for (i = 0; i < _ptr_array->count; i++)
    {
        if (_ptr_array->elements[i] == _value)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}


/*
d_ptr_array_insert_element
  Insert a single pointer at the specified index.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to modify.
  _element:   pointer to insert.
  _index:     index where to insert (supports negative indexing).
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully inserted, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_insert_element
(
    struct d_ptr_array* _ptr_array,
    const void*         _element,
    d_index             _index
)
{
    return (!_ptr_array)
        ? false
        : d_array_common_insert_element(&(_ptr_array->elements),
                                        &(_ptr_array->count),
                                        sizeof(void*),
                                        &_element,
                                        _index);
}


/*
d_ptr_array_insert_elements
  Insert multiple pointers at the specified index.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to modify.
  _elements:  pointer to the array of pointers to insert.
  _count:     number of pointers to insert.
  _index:     index where to insert (supports negative indexing).
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully inserted, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_insert_elements
(
    struct d_ptr_array* _ptr_array,
    const void**        _elements,
    size_t              _count,
    d_index             _index
)
{
    return ( (!_ptr_array) ||
             ( (!_elements) &&
               (_count > 0) ) )
        ? false
        : d_array_common_insert_elements(&(_ptr_array->elements),
                                         &(_ptr_array->count),
                                         sizeof(void*),
                                         _elements,
                                         _count,
                                         _index);
}


/*
d_ptr_array_insert_array
  Insert all pointers from another `d_ptr_array` at the specified index.

Parameter(s):
  _destination: pointer to the destination `d_ptr_array`.
  _source:      pointer to the source `d_ptr_array` to insert from.
  _index:       index where to insert (supports negative indexing).
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully inserted, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_insert_array
(
    struct d_ptr_array*       _destination,
    const struct d_ptr_array* _source,
    d_index                   _index
)
{
    return ( (!_destination) ||
             (!_source) )
        ? false
        : d_ptr_array_insert_elements(_destination,
                                      (const void**)_source->elements,
                                      _source->count,
                                      _index);
}


/*
d_ptr_array_is_empty
  Check if the `d_ptr_array` is empty.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to check.
Return:
  A boolean value corresponding to either:
  - true, if the array is empty or NULL, or
  - false, if the array contains elements.
*/
D_INLINE bool
d_ptr_array_is_empty
(
    const struct d_ptr_array* _ptr_array
)
{
    return ( (!_ptr_array) ||
             (_ptr_array->count == 0) );
}


/*
d_ptr_array_prepend_element
  Prepend a single pointer to the beginning of the array.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to modify.
  _element:   pointer to prepend.
Return:
  A boolean value corresponding to either:
  - true, if the pointer was successfully prepended, or
  - false, if the operation failed.
*/
bool
d_ptr_array_prepend_element
(
    struct d_ptr_array* _ptr_array,
    const void*         _element
)
{
    void** new_elements;
    size_t new_count;
    size_t i;

    if (!_ptr_array)
    {
        return D_FAILURE;
    }

    // calculate new count
    new_count = (_ptr_array->count + 1);

    // reallocate the array with space for one more element
    new_elements = (void**)realloc(_ptr_array->elements,
                                   new_count * sizeof(void*));

    if (!new_elements)
    {
        return D_FAILURE;
    }

    // shift existing elements to the right
    for (i = _ptr_array->count; i > 0; i--)
    {
        new_elements[i] = new_elements[i - 1];
    }

    // insert the new element at the beginning
    new_elements[0] = (void*)_element;

    _ptr_array->elements = new_elements;
    _ptr_array->count    = new_count;

    return D_SUCCESS;
}


/*
d_ptr_array_prepend_elements
  Prepend multiple pointers to the beginning of the array.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to modify.
  _elements:  pointer to the array of pointers to prepend.
  _count:     number of pointers to prepend.
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully prepended, or
  - false, if the operation failed.
*/
bool
d_ptr_array_prepend_elements
(
    struct d_ptr_array* _ptr_array,
    const void**        _elements,
    size_t              _count
)
{
    void** new_elements;
    size_t new_count;
    size_t i;

    if (!_ptr_array)
    {
        return D_FAILURE;
    }

    // prepending zero elements is a no-op success
    if (_count == 0)
    {
        return D_SUCCESS;
    }

    if (!_elements)
    {
        return D_FAILURE;
    }

    // calculate new count
    new_count = (_ptr_array->count + _count);

    // reallocate the array
    new_elements = (void**)realloc(_ptr_array->elements,
                                   new_count * sizeof(void*));

    if (!new_elements)
    {
        return D_FAILURE;
    }

    // shift existing elements to the right by _count positions
    for (i = _ptr_array->count; i > 0; i--)
    {
        new_elements[i + _count - 1] = new_elements[i - 1];
    }

    // copy the new elements to the beginning
    for (i = 0; i < _count; i++)
    {
        new_elements[i] = (void*)_elements[i];
    }

    _ptr_array->elements = new_elements;
    _ptr_array->count    = new_count;

    return D_SUCCESS;
}


/*
d_ptr_array_prepend_array
  Prepend all pointers from another `d_ptr_array`.

Parameter(s):
  _destination: pointer to the destination `d_ptr_array`.
  _source:      pointer to the source `d_ptr_array` to prepend from.
Return:
  A boolean value corresponding to either:
  - true, if the pointers were successfully prepended, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_prepend_array
(
    struct d_ptr_array*       _destination,
    const struct d_ptr_array* _source
)
{
    return ( (!_destination) ||
             (!_source) )
        ? false
        : d_ptr_array_prepend_elements(_destination,
                                       (const void**)_source->elements,
                                       _source->count);
}


/*
d_ptr_array_reverse
  Reverse the order of pointers in the array.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to reverse.
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully reversed, or
  - false, if the operation failed.
*/
D_INLINE bool
d_ptr_array_reverse
(
    struct d_ptr_array* _ptr_array
)
{
    return (!_ptr_array)
        ? false
        : d_array_common_reverse(_ptr_array->elements,
                                 _ptr_array->count,
                                 sizeof(void*));
}


/*
d_ptr_array_resize_amount
  Resize the array by a specified amount.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to resize.
  _amount:    amount to change the size by (positive to grow, negative to 
              shrink).
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully resized, or
  - false, if the operation failed.
*/
bool
d_ptr_array_resize_amount
(
    struct d_ptr_array* _ptr_array,
    ssize_t             _amount
)
{
    size_t new_count;
    void*  new_elements;

    if (!_ptr_array)
    {
        return D_FAILURE;
    }

    // calculate new count
    if (!d_array_common_is_valid_resize_amount(_ptr_array->count,
                                               _amount,
                                               &new_count))
    {
        return D_FAILURE;
    }

    // if no change, return success
    if (new_count == _ptr_array->count)
    {
        return D_SUCCESS;
    }

    // reallocate
    new_elements = realloc(_ptr_array->elements, new_count * sizeof(void*));

    if ( (!new_elements) &&
         (new_count > 0) )
    {
        return D_FAILURE;
    }

    _ptr_array->elements = (void**)new_elements;
    _ptr_array->count    = new_count;

    return D_SUCCESS;
}


/*
d_ptr_array_resize_factor
  Resize the array by a multiplication factor.

Parameter(s):
  _ptr_array: pointer to the `d_ptr_array` to resize.
  _factor:    multiplication factor (> 1.0 to grow, < 1.0 to shrink).
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully resized, or
  - false, if the operation failed.
*/
bool
d_ptr_array_resize_factor
(
    struct d_ptr_array* _ptr_array,
    double              _factor
)
{
    double new_count_d;
    size_t new_count;
    void*  new_elements;

    if ( (!_ptr_array) ||
         (_factor < 0.0) )
    {
        return D_FAILURE;
    }

    // calculate new count
    if (!d_array_common_is_valid_resize_factor(_ptr_array->count,
                                               _factor,
                                               &new_count_d,
                                               true))
    {
        return D_FAILURE;
    }

    new_count = (size_t)new_count_d;

    // if no change, return success
    if (new_count == _ptr_array->count)
    {
        return D_SUCCESS;
    }

    // reallocate
    new_elements = realloc(_ptr_array->elements, new_count * sizeof(void*));

    if ( (!new_elements) &&
         (new_count > 0) )
    {
        return D_FAILURE;
    }

    _ptr_array->elements = (void**)new_elements;
    _ptr_array->count    = new_count;

    return D_SUCCESS;
}


/*
d_ptr_array_sort
  Sort the pointers in the array using the provided comparator function.
  The comparator receives pointers to the array elements (i.e., void**).

Parameter(s):
  _ptr_array:   pointer to the `d_ptr_array` to sort.
  _comparator:  function to compare two elements.
Return:
  none
*/
D_INLINE void
d_ptr_array_sort
(
    struct d_ptr_array* _ptr_array,
    fn_comparator       _comparator
)
{
    if ( (!_ptr_array) ||
         (!_comparator) )
    {
        return;
    }

    d_array_common_sort(_ptr_array->elements,
                        _ptr_array->count,
                        sizeof(void*),
                        _comparator);

    return;
}


// =============================================================================
// destructor functions
// =============================================================================

/*
d_ptr_array_free
  Free the memory associated with this `d_ptr_array`.
  Note: This performs a shallow free - the pointed-to objects are NOT freed.
  The caller is responsible for freeing those if necessary.

Parameter(s):
  _ptr_array: the `d_ptr_array` being freed.
Return:
  none
*/
void
d_ptr_array_free
(
    struct d_ptr_array* _ptr_array
)
{
    if (_ptr_array)
    {
        d_array_common_free_elements_arr(_ptr_array->elements);

        free(_ptr_array);
    }

    return;
}


/*
d_ptr_array_deep_free
  Free the memory associated with this `d_ptr_array` and all pointed-to 
objects.
  Uses the provided free function to free each element.

Parameter(s):
  _ptr_array: the `d_ptr_array` being freed.
  _free_fn:   function to use for freeing each pointed-to object.
Return:
  none
*/
void
d_ptr_array_deep_free
(
    struct d_ptr_array* _ptr_array,
    fn_free             _free_fn
)
{
    if ( (_ptr_array) &&
         (_free_fn) )
    {
        d_array_common_free_elements_deep(_ptr_array->count,
                                          _ptr_array->elements,
                                          _free_fn);

        free(_ptr_array);
    }

    return;
}