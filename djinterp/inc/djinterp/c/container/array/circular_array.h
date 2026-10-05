/*******************************************************************************
* djinterp [c]                                                  circular_array.h
*
*   A circular array (ring buffer) is a fixed-capacity array data structure
* that wraps around when elements are added or removed. It supports efficient
* FIFO operations with O(1) push and pop at both ends.
*   This structure is ideal for streaming data, queues, and bounded buffers
* where the capacity is known ahead of time.
*
*
* path:      /inc/djinterp/c/container/array/circular_array.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ARRAY_CIRCULAR_ARRAY_H
#define DJINTERP_C_CONTAINER_ARRAY_CIRCULAR_ARRAY_H 1

// std
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../container.h"
#include "./array_common.h"


#ifndef D_CIRCULAR_ARRAY_DEFAULT_CAPACITY
    // D_CIRCULAR_ARRAY_DEFAULT_CAPACITY
    //   constant: the default capacity, in number of elements, that a new
    // `d_circular_array` has by default.
    #define D_CIRCULAR_ARRAY_DEFAULT_CAPACITY 32
#endif  // D_CIRCULAR_ARRAY_DEFAULT_CAPACITY


#ifndef D_CIRCULAR_ARRAY_COUNT_T
    #if D_ENV_PP_HAS_VARIADIC_MACROS
    // D_CIRCULAR_ARRAY_COUNT_T
    //   macro: computes the number of variadic arguments of a given type.
    #define D_CIRCULAR_ARRAY_COUNT_T(element_type, ...) \
        (sizeof((element_type[]){ __VA_ARGS__ }) / sizeof(element_type))
    #endif  // D_ENV_PP_HAS_VARIADIC_MACROS
#endif  // D_CIRCULAR_ARRAY_COUNT_T


#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_CIRCULAR_ARRAY_INIT
//   macro: macro-based initializer; assigns a `d_circular_array` with the
// elements specified. Example: struct d_circular_array arr =
// D_CIRCULAR_ARRAY_INIT(int, 8, 1, 2, 3, 4, 5); Note: first parameter after
// type is capacity
#define D_CIRCULAR_ARRAY_INIT(element_type, capacity, ...)                   \
    {                                                                        \
        .count        = D_CIRCULAR_ARRAY_COUNT_T(element_type, __VA_ARGS__), \
        .elements     = (element_type[]){ __VA_ARGS__ },                     \
        .element_size = sizeof(element_type),                                \
        .capacity     = (capacity),                                          \
        .head         = 0,                                                   \
        .tail         = D_CIRCULAR_ARRAY_COUNT_T(element_type, __VA_ARGS__)  \
    }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS


// d_circular_array
//   struct: a circular buffer data structure with fixed capacity. Supports
// wrap-around element access and efficient FIFO/LIFO operations.
struct d_circular_array
{
    size_t count;
    void*  elements;
    size_t element_size;
    size_t capacity;
    size_t head;
    size_t tail;
};


// =============================================================================
// constructor functions
// =============================================================================
// I.    creation and initialization
// I.    creation and initialization
struct d_circular_array* d_circular_array_new(size_t _capacity,
                                              size_t _element_size);
struct d_circular_array* d_circular_array_new_default_capacity(
    size_t _element_size);
struct d_circular_array* d_circular_array_new_from_arr(
    size_t      _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count);
struct d_circular_array* d_circular_array_new_from_args(size_t _capacity,
                                                        size_t _element_size,
                                                        size_t _arg_count,
                                                        ...);
struct d_circular_array* d_circular_array_new_copy(
    const struct d_circular_array* _other);
struct d_circular_array* d_circular_array_new_copy_resized(
    const struct d_circular_array* _other,
    size_t                         _new_capacity);
struct d_circular_array* d_circular_array_new_fill(size_t      _capacity,
                                                   size_t      _element_size,
                                                   const void* _fill_value);

// II.   element access
void* d_circular_array_get(const struct d_circular_array* _circular_array,
                           d_index                        _index);
bool  d_circular_array_set(struct d_circular_array* _circular_array,
                           d_index                  _index,
                           const void*              _value);
void* d_circular_array_front(const struct d_circular_array* _circular_array);
void* d_circular_array_back(const struct d_circular_array* _circular_array);
void* d_circular_array_peek(const struct d_circular_array* _circular_array);
void* d_circular_array_peek_back(
    const struct d_circular_array* _circular_array);
bool  d_circular_array_push_front(struct d_circular_array* _circular_array,
                                  const void*              _element);
bool  d_circular_array_push_back(struct d_circular_array* _circular_array,
                                 const void*              _element);
bool  d_circular_array_push_all_front(struct d_circular_array* _circular_array,
                                      const void*              _elements,
                                      size_t                   _count);
void* d_circular_array_pop_front(struct d_circular_array* _circular_array);
void* d_circular_array_pop_back(struct d_circular_array* _circular_array);
bool  d_circular_array_pop_front_to(struct d_circular_array* _circular_array,
                                    void*                    _out_value);
bool  d_circular_array_pop_back_to(struct d_circular_array* _circular_array,
                                   void*                    _out_value);
bool  d_circular_array_push_front_overwrite(
     struct d_circular_array* _circular_array,
     const void*              _element);

// III.  insertion and removal
bool  d_circular_array_push(struct d_circular_array* _circular_array,
                            const void*              _element);
bool  d_circular_array_push_all(struct d_circular_array* _circular_array,
                                const void*              _elements,
                                size_t                   _count);
void* d_circular_array_pop(struct d_circular_array* _circular_array);
bool  d_circular_array_pop_to(struct d_circular_array* _circular_array,
                              void*                    _out_value);
bool  d_circular_array_push_overwrite(struct d_circular_array* _circular_array,
                                      const void*              _element);
bool  d_circular_array_push_all_overwrite(
     struct d_circular_array* _circular_array,
     const void*              _elements,
     size_t                   _count);

// IV.   search
bool    d_circular_array_contains(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator);
ssize_t d_circular_array_find(const struct d_circular_array* _circular_array,
                              const void*                    _value,
                              fn_comparator                  _comparator);
ssize_t d_circular_array_find_last(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator);
size_t  d_circular_array_count_value(
    const struct d_circular_array* _circular_array,
    const void*                    _value,
    fn_comparator                  _comparator);

// V.    capacity and state queries
bool   d_circular_array_is_empty(
    const struct d_circular_array* _circular_array);
bool   d_circular_array_is_full(const struct d_circular_array* _circular_array);
size_t d_circular_array_count(const struct d_circular_array* _circular_array);
size_t d_circular_array_capacity(
    const struct d_circular_array* _circular_array);
size_t d_circular_array_available_space(
    const struct d_circular_array* _circular_array);
size_t d_circular_array_element_size(
    const struct d_circular_array* _circular_array);

// VI.   iteration
void d_circular_array_foreach(struct d_circular_array* _circular_array,
                              fn_apply                 _apply_fn);
void d_circular_array_foreach_reverse(struct d_circular_array* _circular_array,
                                      fn_apply                 _apply_fn);

// VII.  mutation and reordering
bool d_circular_array_clear(struct d_circular_array* _circular_array);
bool d_circular_array_fill(struct d_circular_array* _circular_array,
                           const void*              _fill_value);
bool d_circular_array_rotate_left(struct d_circular_array* _circular_array,
                                  size_t                   _amount);
bool d_circular_array_rotate_right(struct d_circular_array* _circular_array,
                                   size_t                   _amount);
bool d_circular_array_reverse(struct d_circular_array* _circular_array);
bool d_circular_array_swap(struct d_circular_array* _circular_array,
                           d_index                  _index_a,
                           d_index                  _index_b);
void d_circular_array_sort(struct d_circular_array* _circular_array,
                           fn_comparator            _comparator);
bool d_circular_array_linearize(struct d_circular_array* _circular_array);

// VIII. conversion and reporting
void* d_circular_array_to_linear_array(
    const struct d_circular_array* _circular_array);
bool  d_circular_array_copy_to(const struct d_circular_array* _circular_array,
                               void*                          _destination,
                               size_t                         _dest_capacity);

// IX.   destruction
void d_circular_array_free(struct d_circular_array* _circular_array);
void d_circular_array_free_deep(struct d_circular_array* _circular_array,
                                fn_free                  _free_fn);

#endif  // DJINTERP_C_CONTAINER_ARRAY_CIRCULAR_ARRAY_H
