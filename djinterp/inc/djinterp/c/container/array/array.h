/*******************************************************************************
* djinterp [c]                                                           array.h
*
* djinterp array container module
*   A safe fixed-size array container for arrays that will be resized
* infrequently or not at all.
*
*   Includes:
*   - macro initializers for stack-allocated literals
*   - heap-backed constructors:
*     - args, buffers, slices, ranges and merges
*   - manipulation functions:
*     - append, insert, prepend
*     - resizing (amount or factor)
*     - shifting (left, right, circular)
*     - reverse
*     - sort
*
* See also:
*   - array_sorted.h   - for a sorted array container
*   - circular_array.h - for a circular array container
*   - ptr_array.h      - for an array container containing only pointers
*
*  For the dynamic array equivalents of the above containers, please consider:
*
*
* path:      /inc/djinterp/c/container/array/array.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================

      macros
      1.    D_ARRAY_INIT
      2.    D_ARRAY_S_INIT

      types
      1.    struct d_array
      2.    struct d_array_s

      creation functions
      i.    d_array
            1.    d_array_new
            2.    d_array_new_default_size
            3.    d_array_new_from_arr
            4.    d_array_new_from_args
            5.    d_array_new_copy
            6.    d_array_new_copy_reverse
            7.    d_array_new_copy_range
            8.    d_array_new_copy_range_reverse
            9.    d_array_new_fill
            10.   d_array_new_merge
            11.   d_array_new_slice
            12.   d_array_new_slice_reverse
            13.   d_array_new_slice_range
            14.   d_array_new_slice_range_reverse
      ii.   d_array_s
            1.    d_array_s_new
            2.    d_array_s_new_default_size
            3.    d_array_s_new_from_arr
            4.    d_array_s_new_from_args
            5.    d_array_s_new_copy
            6.    d_array_s_new_copy_reverse
            7.    d_array_s_new_copy_range
            8.    d_array_s_new_copy_range_reverse
            9.    d_array_s_new_fill
            10.   d_array_s_new_merge
            11.   d_array_s_new_slice
            12.   d_array_s_new_slice_reverse
            13.   d_array_s_new_slice_range
            14.   d_array_s_new_slice_range_reverse

      manipulation functions
            1.    d_array_append_element
            2.    d_array_append_elements
            3.    d_array_append_array
            4.    d_array_contains
            5.    d_array_fill
            6.    d_array_find
            7.    d_array_insert_element
            8.    d_array_insert_elements
            9.    d_array_insert_array
            10.   d_array_is_empty
            11.   d_array_prepend_element
            12.   d_array_prepend_elements
            13.   d_array_prepend_array
            14.   d_array_resize_amount
            15.   d_array_resize_factor
            16.   d_array_reverse
            17.   d_array_shift_left
            18.   d_array_shift_left_circular
            19.   d_array_shift_right
            20.   d_array_shift_right_circular
            21.   d_array_slice
            22.   d_array_slice_range
            23.   d_array_sort

      destruction
            1.    d_array_free
            2.    d_array_s_free
*/

#ifndef DJINTERP_C_CONTAINER_ARRAY_ARRAY_H
#define DJINTERP_C_CONTAINER_ARRAY_ARRAY_H 1

// std
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../djinterp.h"
#include "../../dmacro.h"  // D_ARRAY_COUNT_T
#include "../../memory/dmemory.h"
#include "../container.h"
#include "./array_common.h"


#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_ARRAY_INIT
//   macro: brace initializer for a stack-allocated `d_array`. Sets `count`
// from the number of variadic arguments and points `elements` at a compound
// literal of `element_type`; the literal has automatic storage, so the array
// must not outlive the enclosing scope.
//   Example: struct d_array arr = D_ARRAY_INIT(int, 1, 2, 3, 4, 5);
#define D_ARRAY_INIT(element_type, ...)             \
    {                                               \
        .count    = D_ARRAY_COUNT_T(element_type,   \
                                    __VA_ARGS__),   \
        .elements = (element_type[]){ __VA_ARGS__ } \
    }

// D_ARRAY_S_INIT
//   macro: brace initializer for a stack-allocated `d_array_s`. As
// D_ARRAY_INIT, but also records `sizeof(element_type)` in `element_size`, so
// the resulting array carries its own stride and need not be told it again.
//   Example: struct d_array_s arr = D_ARRAY_S_INIT(int, 1, 2, 3, 4, 5);
#define D_ARRAY_S_INIT(element_type, ...)                \
    {                                                    \
        .count        = D_ARRAY_COUNT_T(element_type,    \
                                        __VA_ARGS__),    \
        .elements     = (element_type[]){ __VA_ARGS__ }, \
        .element_size = sizeof(element_type)             \
    }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// d_array
//   struct: a fixed-size array that carries its own length. The element stride
// is NOT stored, so every operation takes `_element_size` from the caller; use
// `d_array_s` when the stride should travel with the array.
struct d_array
{
    size_t count;       // number of elements currently held
    void*  elements;    // contiguous element storage, or NULL when empty
};
// d_array_s
//   struct: a `d_array` that also stores its element stride, so the size need
// not be supplied at each call. Layout matches `d_array` in its first two
// members, so an `_s` array may be read through a `d_array*`.
struct d_array_s
{
    size_t count;           // number of elements currently held
    void*  elements;        // contiguous element storage, or NULL when empty
    size_t element_size;    // stride in bytes of a single element
};


// `d_array` creation functions
struct d_array* d_array_new(size_t _element_size,
                            size_t _initial_size);
struct d_array* d_array_new_default_size(size_t _element_size);
struct d_array* d_array_new_from_arr(size_t      _element_size,
                                     const void* _source,
                                     size_t      _source_count);
struct d_array* d_array_new_from_args(size_t _element_size,
                                      size_t _arg_count,
                                      ...);
struct d_array* d_array_new_copy(const struct d_array* _other,
                                 size_t                _element_size);
struct d_array* d_array_new_copy_reverse(const struct d_array* _other,
                                         size_t                _element_size,
                                         d_index               _start,
                                         d_index               _end);
struct d_array* d_array_new_copy_range(const struct d_array* _other,
                                       size_t                _element_size,
                                       d_index               _start,
                                       d_index               _end);
struct d_array* d_array_new_copy_range_reverse(
    const struct d_array* _other,
    size_t                _element_size,
    d_index               _start,
    d_index               _end);
struct d_array* d_array_new_fill(size_t      _element_size,
                                 size_t      _initial_size,
                                 const void* _value);
struct d_array* d_array_new_merge(size_t _element_size,
                                  size_t _count,
                                  ...);
struct d_array* d_array_new_slice(const struct d_array* _other,
                                  size_t                _element_size,
                                  d_index               _start);
struct d_array* d_array_new_slice_reverse(const struct d_array* _other,
                                          size_t                _element_size,
                                          d_index               _start);
struct d_array* d_array_new_slice_range(const struct d_array* _other,
                                        size_t                _element_size,
                                        d_index               _start,
                                        d_index               _end);
struct d_array* d_array_new_slice_range_reverse(
    const struct d_array* _other,
    size_t                _element_size,
    d_index               _start,
    d_index               _end);

// `d_array_s` creation functions
struct d_array_s* d_array_s_new(size_t _element_size,
                                size_t _initial_size);
struct d_array_s* d_array_s_new_default_size(size_t _element_size);
struct d_array_s* d_array_s_new_from_arr(size_t      _element_size,
                                         const void* _source,
                                         size_t      _source_count);
struct d_array_s* d_array_s_new_from_args(size_t _element_size,
                                          size_t _arg_count,
                                          ...);
struct d_array_s* d_array_s_new_copy(const struct d_array* _other,
                                     size_t                _element_size);
struct d_array_s* d_array_s_new_copy_reverse(
    const struct d_array* _other,
    size_t                _element_size,
    d_index               _start,
    d_index               _end);
struct d_array_s* d_array_s_new_copy_range(const struct d_array* _other,
                                           size_t                _element_size,
                                           d_index               _start,
                                           d_index               _end);
struct d_array_s* d_array_s_new_copy_range_reverse(
    const struct d_array* _other,
    size_t                _element_size,
    d_index               _start,
    d_index               _end);
struct d_array_s* d_array_s_new_fill(size_t      _element_size,
                                     size_t      _initial_size,
                                     const void* _value);
struct d_array_s* d_array_s_new_merge(size_t _element_size,
                                      size_t _count,
                                      ...);
struct d_array_s* d_array_s_new_slice(const struct d_array* _other,
                                      size_t                _element_size,
                                      d_index               _start);
struct d_array_s* d_array_s_new_slice_reverse(
    const struct d_array* _other,
    size_t                _element_size,
    d_index               _start);
struct d_array_s* d_array_s_new_slice_range(const struct d_array* _other,
                                            size_t                _element_size,
                                            d_index               _start,
                                            d_index               _end);
struct d_array_s* d_array_s_new_slice_range_reverse(
    const struct d_array* _other,
    size_t                _element_size,
    d_index               _start,
    d_index               _end);

// `d_array` manipulation functions
bool    d_array_append_element(struct d_array* _array,
                               size_t          _element_size,
                               const void*     _element);
bool    d_array_append_elements(struct d_array* _array,
                                size_t          _element_size,
                                const void*     _elements,
                                size_t          _count);
bool    d_array_append_array(struct d_array* _array,
                             size_t          _element_size,
                             const void*     _elements,
                             size_t          _count,
                             d_index         _index);
bool    d_array_contains(const struct d_array* _array,
                         size_t                _element_size,
                         const void*           _value);
bool    d_array_fill(struct d_array* _array,
                     size_t          _element_size,
                     const void*     _fill_element);
ssize_t d_array_find(struct d_array* _array,
                     size_t          _element_size,
                     const void*     _value);
bool    d_array_insert_element(struct d_array* _array,
                               size_t          _element_size,
                               const void*     _element,
                               d_index         _index);
bool    d_array_insert_elements(struct d_array* _array,
                                size_t          _element_size,
                                const void*     _elements,
                                size_t          _count,
                                d_index         _index);
bool    d_array_insert_array(struct d_array*       _destination,
                             size_t                _element_size,
                             const struct d_array* _source,
                             d_index               _index);
bool    d_array_is_empty(const struct d_array* _array);
bool    d_array_prepend_element(struct d_array* _array,
                                size_t          _element_size,
                                const void*     _element);
bool    d_array_prepend_elements(struct d_array* _array,
                                 size_t          _element_size,
                                 const void*     _elements,
                                 size_t          _count);
bool    d_array_prepend_array(struct d_array*       _destination,
                              size_t                _element_size,
                              const struct d_array* _source);
bool    d_array_resize_amount(struct d_array* _array,
                              size_t          _element_size,
                              ssize_t         _amount);
bool    d_array_resize_factor(struct d_array* _array,
                              size_t          _element_size,
                              double          _factor);
bool    d_array_reverse(struct d_array* _array,
                        size_t          _element_size);
bool    d_array_shift_left(struct d_array* _array,
                           size_t          _element_size,
                           size_t          _amount);
bool    d_array_shift_left_circular(struct d_array* _array,
                                    size_t          _element_size,
                                    size_t          _amount);
bool    d_array_shift_right(struct d_array* _array,
                            size_t          _element_size,
                            size_t          _amount);
bool    d_array_shift_right_circular(struct d_array* _array,
                                     size_t          _element_size,
                                     size_t          _amount);
void*   d_array_slice(void*  _source,
                      size_t _length,
                      d_index index,
                      size_t _element_size);
void*   d_array_slice_range(void*   _source,
                            size_t  _length,
                            d_index _start,
                            d_index _end,
                            size_t  _element_size);
void    d_array_sort(struct d_array* _array,
                     size_t          _element_size,
                     fn_comparator   _comparator);

// memory management
void d_array_free(struct d_array* _array);
void d_array_s_free(struct d_array_s* _array);


#endif  // DJINTERP_C_CONTAINER_ARRAY_ARRAY_H
