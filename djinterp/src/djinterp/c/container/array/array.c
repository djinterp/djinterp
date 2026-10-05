/*******************************************************************************
* djinterp [c]                                                           array.c
*
* TBA
*
*
* path:      /src/djinterp/c/container/array/array.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/array/array.h"

// std


/*
d_array_new
  Allocate and initialize a new `d_array` with specified element size and
capacity.

Parameter(s):
  _element_size: size, in bytes, of each individual element to be stored in the
                 `d_array`.
  _initial_size: initial capacity to allocate for the array.
Return:
  A pointer to the newly created `d_array`, or NULL if allocation failed.
*/
struct d_array*
d_array_new
(
    size_t _element_size,
    size_t _initial_size
)
{
    struct d_array* arr;

    if (!_element_size)
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_sized(&(arr->elements),
                                   &(arr->count),
                                   _element_size,
                                   _initial_size))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_default_size
  Allocate and initialize a new `d_array` with default capacity.

Parameter(s):
  _element_size: size in bytes of each element to be stored in the array.
Return:
  A pointer to the newly created d_array, or NULL if allocation failed.
*/
struct d_array*
d_array_new_default_size
(
    size_t _element_size
)
{
    return (_element_size)
        ? d_array_new(_element_size, D_ARRAY_DEFAULT_CAPACITY)
        : NULL;
}

/*
d_array_new_from_arr
  Allocate and initialize a new `d_array` by copying data from an existing
array.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _source:       pointer to the source array data to copy from.
  _source_count: number of elements in the source array.
Return:
  A pointer to the newly created d_array, or NULL if allocation failed.
*/
struct d_array*
d_array_new_from_arr
(
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    struct d_array* arr;

    if ( (!_element_size) ||
         ( (!_source) &&
           (_source_count > 0) ) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_from_array(&(arr->elements),
                                        &(arr->count),
                                        _element_size,
                                        _source,
                                        _source_count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_from_args
  Allocate and initialize a new d_array from variadic arguments.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _arg_count:    number of variadic arguments to process. ...: variadic
                 arguments containing the element data.
Return:
  A pointer to the newly created d_array, or NULL if allocation failed.
*/
struct d_array*
d_array_new_from_args
(
    size_t _element_size,
    size_t _arg_count,
    ...
)
{
    bool            success;
    va_list         args;
    struct d_array* arr;

    if (!_element_size)
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    if (!arr)
    {
        return NULL;
    }

    va_start(args, _arg_count);

    success = d_array_common_init_from_args(&(arr->elements),
                                            &(arr->count),
                                            _element_size,
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
d_array_new_copy
  Allocate and initialize a new d_array by copying another d_array.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
Return:
  A pointer to the newly created d_array copy, or NULL if allocation failed.
*/
struct d_array*
d_array_new_copy
(
    const struct d_array* _source,
    size_t                _element_size
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy(&(arr->elements),
                                  &(arr->count),
                                  _element_size,
                                  _source->elements,
                                  _source->count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_copy_reverse
  Allocate and initialize a new d_array by copying another d_array in reverse
  order within the specified range.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array copy, or NULL if allocation
  failed.
*/
struct d_array*
d_array_new_copy_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy_reverse(&(arr->elements),
                                          &(arr->count),
                                          _element_size,
                                          _source->elements,
                                          _source->count,
                                          _start,
                                          _end))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_copy_range
  Allocate and initialize a new d_array by copying a range from another
  d_array.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created d_array copy, or NULL if allocation failed.
*/
struct d_array*
d_array_new_copy_range
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocationw was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy_range(&(arr->elements),
                                        &(arr->count),
                                        _element_size,
                                        _source->elements,
                                        _source->count,
                                        _start,
                                        _end))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_copy_range_reverse
  Allocate and initialize a new d_array by copying a range from another
  d_array in reverse order.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array copy, or NULL if allocation
  failed.
*/
struct d_array*
d_array_new_copy_range_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocationw was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy_range_reverse(&(arr->elements),
                                                &(arr->count),
                                                _element_size,
                                                _source->elements,
                                                _source->count,
                                                _start,
                                                _end))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_fill
  Allocate and initialize a new d_array filled with copies of a given value.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _initial_size: initial capacity and number of elements.
  _value:        pointer to the value to fill the array with.
Return:
  A pointer to the newly created filled d_array, or NULL if allocation failed.
*/
struct d_array*
d_array_new_fill
(
    size_t      _element_size,
    size_t      _initial_size,
    const void* _value
)
{
    struct d_array* arr;

    if ( (!_element_size) ||
         ( (!_value) &&
           (_initial_size > 0) ) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocationw was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_fill(&(arr->elements),
                                  &(arr->count),
                                  _element_size,
                                  _initial_size,
                                  _value))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_merge
  Allocate and initialize a new d_array by merging multiple arrays.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _count:        number of arrays to merge. ...: variadic arguments containing
                 pointers to d_array structures.
Return:
  A pointer to the newly created merged d_array, or NULL if allocation failed.
*/
struct d_array*
d_array_new_merge
(
    size_t _element_size,
    size_t _count,
    ...
)
{
    struct d_array* result;
    struct d_array* current;
    va_list         args;
    size_t          total_size;
    size_t          current_offset;

    if ( (!_element_size) ||
         (!_count) )
    {
        return NULL;
    }

    // calculate total size needed
    total_size = 0;

    va_start(args, _count);

    for (size_t i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_array*);

        if (current)
        {
            total_size += current->count;
        }
    }

    va_end(args);

    result = d_array_new(_element_size, total_size);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // merge all arrays
    current_offset = 0;
    va_start(args, _count);

    for (size_t i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_array*);

        if ( (current) &&
             (current->count > 0) )
        {
            d_memcpy((char*)result->elements + (current_offset * _element_size),
                     current->elements,
                     current->count * _element_size);

            current_offset += current->count;
        }
    }

    va_end(args);

    result->count = total_size;

    return result;
}

/*
d_array_new_slice
  Allocate and initialize a new d_array as a slice starting from the specified
index.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the slice (supports negative indexing).
Return:
  A pointer to the newly created d_array slice, or NULL if allocation failed.
*/
struct d_array*
d_array_new_slice
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice(&(arr->elements),
                                   &(arr->count),
                                   _element_size,
                                   (const void**)&_source->elements,
                                   _source->count,
                                   _start))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_slice_reverse
  Allocate and initialize a new d_array as a reversed slice starting from the
  specified index.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the slice (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array slice, or NULL if allocation
  failed.
*/
struct d_array*
d_array_new_slice_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice_reverse(&(arr->elements),
                                           &(arr->count),
                                           _element_size,
                                           (const void**)&_source->elements,
                                           _source->count))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_slice_range
  Allocate and initialize a new d_array as a slice within the specified range.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created d_array slice, or NULL if allocation failed.
*/
struct d_array*
d_array_new_slice_range
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array* arr;

    if ((!_source) ||
        (!_element_size))
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice_range(&(arr->elements),
                                         &(arr->count),
                                         _element_size,
                                         (const void**)&_source->elements,
                                         _source->count,
                                         _start,
                                         _end))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_new_slice_range_reverse
  Allocate and initialize a new d_array as a reversed slice within the
  specified range.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array slice, or NULL if allocation
  failed.
*/
struct d_array*
d_array_new_slice_range_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice_range_reverse(&(arr->elements),
                                                 &(arr->count),
                                                 _element_size,
                                                 (const void**)&_source->elements,
                                                 _source->count,
                                                 _start,
                                                 _end))
    {
        free(arr);

        return NULL;
    }

    return arr;
}

/*
d_array_s_new
  Allocate and initialize a new d_array_s with specified element size and
  capacity.

Parameter(s):
  _element_size: size in bytes of each element to be stored in the array.
  _initial_size: initial capacity to allocate for the array.
Return:
  A pointer to the newly created d_array_s, or NULL if allocation failed.
*/
struct d_array_s*
d_array_s_new
(
    size_t _element_size,
    size_t _initial_size
)
{
    struct d_array_s* arr;

    if (!_element_size)
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_sized(&(arr->elements),
                                   &(arr->count),
                                   _element_size,
                                   _initial_size))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_default_size
  Allocate and initialize a new d_array_s with default capacity.

Parameter(s):
  _element_size: size in bytes of each element to be stored in the array.
Return:
  A pointer to the newly created d_array_s, or NULL if allocation failed.
*/
struct d_array_s*
d_array_s_new_default_size
(
    size_t _element_size
)
{
    return (_element_size)
        ? d_array_s_new(_element_size, D_ARRAY_DEFAULT_CAPACITY)
        : NULL;
}

/*
d_array_s_new_from_arr
  Allocate and initialize a new d_array_s by copying data from an existing
  array.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _source:       pointer to the source array data to copy from.
  _source_count: number of elements in the source array.
Return:
  A pointer to the newly created d_array_s, or NULL if allocation failed.
*/
struct d_array_s*
d_array_s_new_from_arr
(
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
)
{
    struct d_array_s* arr;

    if ( (!_element_size) ||
         ( (!_source) &&
           (_source_count > 0) ) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_from_array(&(arr->elements),
                                        &(arr->count),
                                        _element_size,
                                        _source,
                                        _source_count))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_from_args
  Allocate and initialize a new d_array_s from variadic arguments.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _arg_count:    number of variadic arguments to process. ...: variadic
                 arguments containing the element data.
Return:
  A pointer to the newly created d_array_s, or NULL if allocation failed.
*/
struct d_array_s*
d_array_s_new_from_args
(
    size_t _element_size,
    size_t _arg_count,
    ...
)
{
    bool              success;
    struct d_array_s* arr;
    va_list           args;

    if (!_element_size)
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    va_start(args, _arg_count);

    success = d_array_common_init_from_args(&(arr->elements),
                                            &(arr->count),
                                            _element_size,
                                            _arg_count,
                                            args);

    va_end(args);

    if (!success)
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_copy
  Allocate and initialize a new d_array_s by copying from a d_array.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
Return:
  A pointer to the newly created d_array_s copy, or NULL if allocation failed.
*/
struct d_array_s*
d_array_s_new_copy
(
    const struct d_array* _source,
    size_t                _element_size
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy(&(arr->elements),
                                  &(arr->count),
                                  _element_size,
                                  _source->elements,
                                  _source->count))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_copy_reverse
  Allocate and initialize a new d_array_s by copying another d_array in reverse
  order within the specified range.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array_s copy, or NULL if allocation
  failed.
*/
struct d_array_s*
d_array_s_new_copy_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy_reverse(&(arr->elements),
                                          &(arr->count),
                                          _element_size,
                                          _source->elements,
                                          _source->count,
                                          _start,
                                          _end))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_copy_range
  Allocate and initialize a new `d_array_s` by copying a range from another
`d_array`.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created d_array_s copy, or NULL if allocation failed.
*/
struct d_array_s*
d_array_s_new_copy_range
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy_range(&(arr->elements),
                                        &(arr->count),
                                        _element_size,
                                        _source->elements,
                                        _source->count,
                                        _start,
                                        _end))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_copy_range_reverse
  Allocate and initialize a new d_array_s by copying a range from another
  d_array in reverse order.

Parameter(s):
  _source:       pointer to the d_array to copy from.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array_s copy, or NULL if allocation
  failed.
*/
struct d_array_s*
d_array_s_new_copy_range_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_copy_range_reverse(&(arr->elements),
                                                &(arr->count),
                                                _element_size,
                                                _source->elements,
                                                _source->count,
                                                _start,
                                                _end))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_fill
  Allocate and initialize a new `d_array_s` filled with copies of a given value.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _initial_size: initial capacity and number of elements.
  _value:        pointer to the value to fill the array with.
Return:
  A pointer to the newly created filled d_array_s, or NULL if allocation
failed.
*/
struct d_array_s*
d_array_s_new_fill
(
    size_t      _element_size,
    size_t      _initial_size,
    const void* _value
)
{
    struct d_array_s* arr;

    if ( (!_element_size) ||
         (!_initial_size) ||
         (!_value) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_fill(&(arr->elements),
                                  &(arr->count),
                                  _element_size,
                                  _initial_size,
                                  _value))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_merge
  Allocate and initialize a new d_array_s by merging multiple d_array_s
  structures. All source arrays must have the same element_size as the target.

Parameter(s):
  _element_size: the size, in bytes, of each individual element.
  _count:        number of arrays to merge. ...: variadic arguments containing
                 pointers to d_array_s structures.
Return:
  A pointer to the newly created merged d_array_s, or NULL if allocation
  failed or element sizes don't match.
*/
struct d_array_s*
d_array_s_new_merge
(
    size_t _element_size,
    size_t _count,
    ...
)
{
    struct d_array_s* result;
    struct d_array_s* current;
    va_list           args;
    size_t            total_size;
    size_t            current_offset;

    if ( (!_element_size) ||
         (!_count) )
    {
        return NULL;
    }

    // first pass: validate element sizes and calculate total size
    total_size = 0;
    va_start(args, _count);

    for (size_t i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_array_s*);

        if (current)
        {
            // validate element size match - this is the key validation
            if (current->element_size != _element_size)
            {
                va_end(args);
                return NULL; // reject element size mismatch
            }

            total_size += current->count;
        }
    }

    va_end(args);

    result = d_array_s_new(_element_size, total_size);

    if (!result)
    {
        return NULL;
    }

    // second pass: merge all arrays
    current_offset = 0;
    va_start(args, _count);

    for (size_t i = 0; i < _count; i++)
    {
        current = va_arg(args, struct d_array_s*);

        if ( (current) &&
             (current->count > 0) )
        {
            d_memcpy((char*)result->elements + (current_offset * _element_size),
                     current->elements,
                     current->count * _element_size);

            current_offset += current->count;
        }
    }

    va_end(args);

    result->count = total_size;

    return result;
}

/*
d_array_s_new_slice
  Allocate and initialize a new `d_array_s` as a slice starting from the
  specified index.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the slice (supports negative indexing).
Return:
  A pointer to the newly created d_array_s slice, or NULL if allocation
  failed.
*/
struct d_array_s*
d_array_s_new_slice
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice(&(arr->elements),
                                   &(arr->count),
                                   _element_size,
                                   (const void**)&_source->elements,
                                   _source->count,
                                   _start))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_slice_reverse
  Allocate and initialize a new d_array_s as a reversed slice starting from
the specified index.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the slice (supports negative indexing).
Return:
  A pointer to the newly created reversed d_array_s slice, or NULL if
  allocation failed.
*/
struct d_array_s*
d_array_s_new_slice_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice_reverse(&(arr->elements),
                                           &(arr->count),
                                           _element_size,
                                           (const void**)&_source->elements,
                                           _source->count))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_slice_range
  Allocate and initialize a new d_array_s as a slice within the specified
range.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created d_array_s slice, or NULL if allocation
  failed.
*/
struct d_array_s*
d_array_s_new_slice_range
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice_range(&(arr->elements),
                                         &(arr->count),
                                         _element_size,
                                         (const void**)&_source->elements,
                                         _source->count,
                                         _start,
                                         _end))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_s_new_slice_range_reverse
  Allocate and initialize a new d_array_s as a reversed slice within the
specified range.

Parameter(s):
  _source:       pointer to the source d_array.
  _element_size: the size, in bytes, of each individual element.
  _start:        starting index for the range (supports negative indexing).
  _end:          ending index for the range (supports negative indexing).
Return:
  A pointer to the newly created reversed `d_array_s slice`, or NULL if
  allocation failed.
*/
struct d_array_s*
d_array_s_new_slice_range_reverse
(
    const struct d_array* _source,
    size_t                _element_size,
    d_index               _start,
    d_index               _end
)
{
    struct d_array_s* arr;

    if ( (!_source) ||
         (!_element_size) )
    {
        return NULL;
    }

    arr = d_array_common_alloc(sizeof(struct d_array_s));

    // ensure that memory allocation was successful
    if (!arr)
    {
        return NULL;
    }

    if (!d_array_common_init_slice_range_reverse(&(arr->elements),
                                                 &(arr->count),
                                                 _element_size,
                                                 (const void**)&_source->elements,
                                                 _source->count,
                                                 _start,
                                                 _end))
    {
        free(arr);

        return NULL;
    }

    arr->element_size = _element_size;

    return arr;
}

/*
d_array_append_element
  Append a single element to the end of the array.

Parameter(s):
  _array:        pointer to the d_array to modify.
  _element_size: the size, in bytes, of each individual element.
  _element:      pointer to the element to append.
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully appended, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_append_element
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _element
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             (!_element) )
    ? false
    : d_array_common_append_element(&(_array->elements),
                                    &(_array->count),
                                    _element_size,
                                    _element);
}

/*
d_array_append_elements
  Append multiple elements to the end of the array.

Parameter(s):
  _array:        pointer to the d_array to modify.
  _element_size: the size, in bytes, of each individual element.
  _elements:     pointer to the elements to append.
  _count:        number of elements to append.
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully appended, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_append_elements
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _elements,
    size_t          _count
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             ( (!_elements) &&
               (_count > 0) ) )
    ? false
    : d_array_common_append_elements(&(_array->elements),
                                     &(_array->count),
                                     _element_size,
                                     _elements,
                                     _count);
}

/*
d_array_append_array
  Append all elements from another array.

Parameter(s):
  _array:        pointer to the destination d_array.
  _element_size: the size, in bytes, of each individual element.
  _source:       pointer to the source array elements.
  _count:        number of elements in the source array.
  _index:        index where to insert the source array (unused in append).
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully appended, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_append_array
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _source,
    size_t          _count,
    d_index         _index
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             ( (!_source) &&
               (_count > 0) ) )
    ? false
    // for append, ignore the index parameter and append at the end
    : d_array_append_elements(_array,
                              _element_size,
                              _source,
                              _count);
}

/*
d_array_contains
  Check if the given `d_array` contains a specific value.
  Uses byte-by-byte comparison (memcmp) to find the value.

Parameter(s):
  _array:        pointer to the `d_array` to search.
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to the value to search for.
Return:
  A boolean value corresponding to either:
  - true, if the value was found in the array, or
  - false, if the value was not found or parameters are invalid.
*/
bool
d_array_contains
(
    const struct d_array* _array,
    size_t                _element_size,
    const void*           _value
)
{
    size_t      i;
    const char* elem_ptr;

    if ( (!_array)           ||
         (!_array->elements) ||
         (!_element_size)    ||
         (!_value) )
    {
        return false;
    }

    elem_ptr = (const char*)_array->elements;

    for (i = 0; i < _array->count; i++)
    {
        if (memcmp(elem_ptr + (i * _element_size), _value, _element_size) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_array_fill
  Fill the entire `d_array` with copies of the specified element.

Parameter(s):
  _array:        pointer to the d_array to fill.
  _element_size: the size, in bytes, of each individual element.
  _fill_element: pointer to the element to fill the array with.
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully filled, or
  - false, if the operation failed due to invalid parameters.
*/
bool
d_array_fill
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _fill_element
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             (!_fill_element) )
    ? false
    : d_array_common_fill(_array->elements,
                          _array->count,
                          _element_size,
                          _fill_element);
}

/*
d_array_find
  Find the index of the first occurrence of a specific value.
  Uses byte-by-byte comparison (memcmp) to find the value.

Parameter(s):
  _array:        pointer to the d_array to search.
  _element_size: the size, in bytes, of each individual element.
  _value:        pointer to the value to search for.
Return:
  The index of the first occurrence of the value, or -1 if not found or
  parameters are invalid.
*/
ssize_t
d_array_find
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _value
)
{
    size_t      i;
    const char* elem_ptr;

    if ( (!_array)           ||
         (!_array->elements) ||
         (!_element_size)    ||
         (!_value) )
    {
        return -1;
    }

    elem_ptr = (const char*)_array->elements;

    for (i = 0; i < _array->count; i++)
    {
        if (memcmp(elem_ptr + (i * _element_size), _value, _element_size) == 0)
        {
            return (ssize_t)i;
        }
    }

    return -1;
}

/*
d_array_insert_element
  Insert a single element at the specified index.

Parameter(s):
  _array:        pointer to the d_array to modify.
  _element_size: the size, in bytes, of each individual element.
  _element:      pointer to the element to insert.
  _index:        index where to insert the element (supports negative indexing).
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully inserted, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_insert_element
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _element,
    d_index         _index
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             (!_element) )
    ? false
    : d_array_common_insert_element(&(_array->elements),
                                    &(_array->count),
                                    _element_size,
                                    _element,
                                    _index);
}

/*
d_array_insert_elements
  Insert multiple elements at the specified index.

Parameter(s):
  _array:        pointer to the d_array to modify.
  _element_size: the size, in bytes, of each individual element.
  _elements:     pointer to the elements to insert.
  _count:        number of elements to insert.
  _index:        index where to insert the elements (supports negative
                 indexing).
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully inserted, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_insert_elements
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _elements,
    size_t          _count,
    d_index         _index
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             ( (!_elements) &&
               (_count > 0) ) )
    ? false
    : d_array_common_insert_elements(&(_array->elements),
                                        &(_array->count),
                                        _element_size,
                                        _elements,
                                        _count,
                                        _index);
}

/*
d_array_insert_array
  Insert all elements from another array at the specified index.

Parameter(s):
  _destination:  pointer to the destination d_array.
  _element_size: the size, in bytes, of each individual element.
  _source:       pointer to the source d_array to insert from.
  _index:        index where to insert the source array (supports negative
                 indexing).
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully inserted, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_insert_array
(
    struct d_array*       _destination,
    size_t                _element_size,
    const struct d_array* _source,
    d_index               _index
)
{
    return ( (!_destination) ||
             (!_source)      ||
             (!_element_size) )
    ? false
    : d_array_insert_elements(_destination,
                                _element_size,
                                _source->elements,
                                _source->count,
                                _index);
}

/*
d_array_is_empty
  Check if the array is empty.

Parameter(s):
  _array: pointer to the d_array to check.
Return:
  A boolean value corresponding to either:
  - true, if the array is empty (count is 0), or
  - false, if the array is not empty or the parameter is invalid.
*/
bool
d_array_is_empty
(
    const struct d_array* _array
)
{
    return (!_array)
        ? true  // consider NULL array as empty
        : (_array->count == 0);
}

/*
d_array_prepend_element
  Prepend a single element to the beginning of the array.

Parameter(s):
  _array:        pointer to the d_array to modify.
  _element_size: the size, in bytes, of each individual element.
  _element:      pointer to the element to prepend.
Return:
  A boolean value corresponding to either:
  - true, if the element was successfully prepended, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_prepend_element
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _element
)
{
    return ( (!_array)   ||
             (!_element) ||
             (!_element_size) )
    ? false
    : d_array_common_prepend_element(&_array->elements,
                                     &_array->count,
                                     _element_size,
                                     _element);
}

/*
d_array_prepend_elements
  Prepend multiple elements to the beginning of the array.

Parameter(s):
  _array:        pointer to the d_array to modify.
  _element_size: the size, in bytes, of each individual element.
  _elements:     pointer to the elements to prepend.
  _count:        number of elements to prepend.
Return:
  A boolean value corresponding to either:
  - true, if the elements were successfully prepended, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_prepend_elements
(
    struct d_array* _array,
    size_t          _element_size,
    const void*     _elements,
    size_t          _count
)
{
    return ( (!_array)        ||
             (!_element_size) ||
             ( (!_elements) &&
               (_count > 0) ) )
    ? false
    : d_array_common_prepend_elements(&_array->elements,
                                      &_array->count,
                                      _element_size,
                                      _elements,
                                      _count);
}

/*
d_array_prepend_array
  Prepend all elements from another array to the beginning.

Parameter(s):
  _destination:  pointer to the destination d_array.
  _element_size: the size, in bytes, of each individual element.
  _source:       pointer to the source d_array to prepend from.
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully prepended, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_prepend_array
(
    struct d_array*       _destination,
    size_t                _element_size,
    const struct d_array* _source
)
{
    return ( (!_destination)  ||
             (!_element_size) ||
             (!_source) )
    ? false
    : d_array_prepend_elements(_destination,
                               _element_size,
                               _source->elements,
                               _source->count);
}

/*
d_array_resize_amount
  Resize the array by a specific amount (positive to grow, negative to shrink).

Parameter(s):
  _array:        pointer to the d_array to resize.
  _element_size: the size, in bytes, of each individual element.
  _amount:       amount to resize by (positive to grow, negative to shrink).
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully resized, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_resize_amount
(
    struct d_array* _array,
    size_t          _element_size,
    ssize_t         _amount
)
{
    ssize_t new_count;

    if ( (!_array) ||
         (!_element_size) )
    {
        return false;
    }
    else if (!_amount)
    {
        return true;
    }

    new_count = d_array_common_resize_amount(_array->elements,
                                             _array->count,
                                             _element_size,
                                             _amount);

    if (new_count < 0)
    {
        return false;
    }

    _array->count = (size_t)new_count;

    return true;
}

/*
d_array_resize_factor
  Resize the array by a multiplicative factor.

Parameter(s):
  _array:        pointer to the d_array to resize.
  _element_size: the size, in bytes, of each individual element.
  _factor:       multiplicative factor to resize by.
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully resized, or
  - false, if the operation failed due to invalid parameters or memory
    allocation failure.
*/
bool
d_array_resize_factor
(
    struct d_array* _array,
    size_t          _element_size,
    double          _factor
)
{
    ssize_t new_count;

    if ( (!_array)        ||
         (!_element_size) ||
         (_factor <= 0.0) )
    {
        return false;
    }

    new_count = d_array_common_resize_factor(_array->elements,
                                             _array->count,
                                             _element_size,
                                             _factor);

    if (new_count < 0)
    {
        return false;
    }

    _array->count = (size_t)new_count;

    return true;
}

/*
d_array_reverse
  Reverses the order of elements in the array.

Parameter(s):
  _array:        pointer to the d_array to reverse
  _element_size: size in bytes of each element
Return:
  A boolean value corresponding to either:
  - true, if the array was successfully reversed, or
  - false, if parameters are invalid.
*/
bool
d_array_reverse
(
    struct d_array* _array,
    size_t          _element_size
)
{
    return ( (!_array) ||
             (!_element_size) )
    ? false
    : d_array_common_reverse(_array->elements,
                             _array->count,
                             _element_size);
}

/*
d_array_shift_left
  Shift elements in the d_array to the left (lossy operation).

Parameter(s):
  _array:        pointer to the d_array to shift.
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift left.
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_shift_left
(
    struct d_array* _array,
    size_t          _element_size,
    size_t          _amount
)
{
    return ( (!_array) ||
             (!_element_size) )
    ? false
    : d_array_common_shift_left(_array->elements,
                                _array->count,
                                _element_size,
                                _amount);
}

/*
d_array_shift_left_circular
  Shift elements in the d_array to the left in a circular fashion.

Parameter(s):
  _array:        pointer to the d_array to shift.
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift left.
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_shift_left_circular
(
    struct d_array* _array,
    size_t          _element_size,
    size_t          _amount
)
{
    return ( (!_array) ||
             (!_element_size) )
    ? false
    : d_array_common_shift_left_circular(_array->elements,
                                         _array->count,
                                         _element_size,
                                         _amount);
}

/*
d_array_shift_right
  Shift elements in the d_array to the right (lossy operation).

Parameter(s):
  _array:        pointer to the d_array to shift.
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift right.
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_shift_right
(
    struct d_array* _array,
    size_t          _element_size,
    size_t          _amount
)
{
    return ( (!_array) ||
             (!_element_size) )
    ? false
    : d_array_common_shift_right(_array->elements,
                                 _array->count,
                                 _element_size,
                                 _amount);
}

/*
d_array_shift_right_circular
  Shift elements in the d_array to the right in a circular fashion.

Parameter(s):
  _array:        pointer to the d_array to shift.
  _element_size: the size, in bytes, of each individual element.
  _amount:       number of positions to shift right.
Return:
  A boolean value corresponding to either:
  - true, if shift was successful, or
  - false, if parameters are invalid.
*/
bool
d_array_shift_right_circular
(
    struct d_array* _array,
    size_t          _element_size,
    size_t          _amount
)
{
    return ( (!_array) ||
             (!_element_size) )
    ? false
    : d_array_common_shift_right_circular(_array->elements,
                                          _array->count,
                                          _element_size,
                                          _amount);
}

/*
d_array_slice
  Creates a slice of the array starting at the specified index.

Parameter(s):
  _source:       pointer to the source array elements
  _length:       length of the source array
  _index:        starting index for the slice
  _element_size: size in bytes of each element
Return:
  pointer to the sliced array, or NULL if the operation failed
*/
void*
d_array_slice
(
    void*   _source,
    size_t  _length,
    d_index _index,
    size_t  _element_size
)
{
    void*  result;
    size_t start_index;
    size_t slice_length;
    size_t copy_size;

    if ( (!_source)       ||
         (!_length)       ||
         (!_element_size) ||
         // convert negative indices to positive
         (!d_index_convert_safe(_index, _length, &start_index)) )
    {
        return NULL;
    }

    // calculate slice length (from start_index to end of array)
    slice_length = (_length - start_index);

    if (slice_length == 0)
    {
        return NULL;
    }

    // allocate memory for slice
    copy_size = (slice_length * _element_size);
    result = malloc(copy_size);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // copy the sliced data
    if (!d_memcpy(result,
                 (char*)_source + (start_index * _element_size),
                 copy_size) )
    {
        free(result);

        return NULL;
    }

    return result;
}

/*
d_array_slice_range
  Creates a slice of the array within the specified range.

Parameter(s):
  _source:       pointer to the source array elements
  _length:       length of the source array
  _start:        starting index for the slice
  _end:          ending index for the slice
  _element_size: size in bytes of each element
Return:
  pointer to the sliced array, or NULL if the operation failed
*/
void*
d_array_slice_range
(
    void*   _source,
    size_t  _length,
    d_index _start,
    d_index _end,
    size_t  _element_size
)
{
    void*  result;
    size_t start_index;
    size_t end_index;
    size_t slice_length;
    size_t copy_size;

    if ( (!_source)                                             ||
         (!_length)                                             ||
         (!_element_size)                                       ||
         // convert negative indices to positive
         (!d_index_convert_safe(_start, _length, &start_index)) ||
         (!d_index_convert_safe(_end,   _length, &end_index))   ||
         (start_index >= end_index)                             ||
         (end_index > _length) )
    {
        return NULL;
    }

    // calculate slice length
    slice_length = (end_index - start_index);

    if (slice_length == 0)
    {
        return NULL;
    }

    // allocate memory for slice
    copy_size = (slice_length * _element_size);
    result = malloc(copy_size);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    // copy the sliced data
    if (!d_memcpy(result,
                 (char*)_source + (start_index * _element_size),
                 copy_size) )
    {
        free(result);

        return NULL;
    }

    return result;
}

/*
d_array_sort
  Sorts the elements in the array using the provided comparator function.

Parameter(s):
  _array:        pointer to the d_array to sort
  _element_size: size in bytes of each element
  _comparator:   function to compare two elements
Return:
  none
*/
void
d_array_sort
(
    struct d_array* _array,
    size_t          _element_size,
    fn_comparator   _comparator
)
{
    if ( (!_array)        ||
         (!_element_size) ||
         (!_comparator) )
    {
        return;
    }

    d_array_common_sort(_array->elements,
                        _array->count,
                        _element_size,
                        _comparator);
    return;
}

/*
d_array_free
  Free the memory associated with this d_array.
  Note that this will free all memory held by this d_array, but may not
  free the memory associated with the array's elements themselves if they
  contain pointers; the caller is responsible for avoiding memory leaks
  by freeing such elements first if applicable.

Parameter(s):
  _array: the d_array being freed.
Return:
  none.
*/
void
d_array_free
(
    struct d_array* _array
)
{
    if (_array)
    {
        d_array_common_free_elements_arr(_array->elements);

        free(_array);
    }

    return;
}

/*
d_array_s_free
  Free the memory associated with this d_array_s.
  Note that this will free all memory held by this d_array_s, but may not
  free the memory associated with the array's elements themselves if they
  contain pointers; the caller is responsible for avoiding memory leaks
  by freeing such elements first if applicable.

Parameter(s):
  _array: the d_array_s being freed.
Return:
  none.
*/
void
d_array_s_free
(
    struct d_array_s* _array
)
{
    if (_array)
    {
        d_array_common_free_elements_arr(_array->elements);

        free(_array);
    }

    return;
}
