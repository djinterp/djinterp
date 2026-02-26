/******************************************************************************
* djinterp [container]                                         circular_array.h
*
*   A circular array (ring buffer) is a fixed-capacity array data structure
* that wraps around when elements are added or removed. It supports efficient
* FIFO operations with O(1) push and pop at both ends.
*   This structure is ideal for streaming data, queues, and bounded buffers
* where the capacity is known ahead of time.
*
*
* path:      \inc\container\array\circular_array.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.05.08
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARRAY_CIRCULAR_
#define DJINTERP_CONTAINER_ARRAY_CIRCULAR_ 1

#include <stdarg.h>
#include <stdlib.h>
#include "../../djinterp.h"
#include "../../dmemory.h"
#include "../../container/container.h"
#include "./array_common.h"


#ifndef D_CIRCULAR_ARRAY_DEFAULT_CAPACITY
    // D_CIRCULAR_ARRAY_DEFAULT_CAPACITY
    //   constant: the default capacity, in number of elements, that a new
    // `d_circular_array` has by default.
    #define D_CIRCULAR_ARRAY_DEFAULT_CAPACITY 32
#endif  // D_CIRCULAR_ARRAY_DEFAULT_CAPACITY


#ifndef D_CIRCULAR_ARRAY_COUNT_T
    // D_CIRCULAR_ARRAY_COUNT_T
    //   macro: computes the number of variadic arguments of a given type.
    #define D_CIRCULAR_ARRAY_COUNT_T(element_type, ...)                       \
        (sizeof((element_type[]){ __VA_ARGS__ }) / sizeof(element_type))
#endif  // D_CIRCULAR_ARRAY_COUNT_T


// D_CIRCULAR_ARRAY_INIT
//   macro: macro-based initializer; assigns a `d_circular_array` with the
// elements specified.
// Example:
//   struct d_circular_array arr = D_CIRCULAR_ARRAY_INIT(int, 8, 1, 2, 3, 4, 5);
// Note: first parameter after type is capacity
#define D_CIRCULAR_ARRAY_INIT(element_type, capacity, ...)                    \
    {                                                                         \
        .count        = D_CIRCULAR_ARRAY_COUNT_T(element_type, __VA_ARGS__),  \
        .elements     = (element_type[]){ __VA_ARGS__ },                      \
        .element_size = sizeof(element_type),                                 \
        .capacity     = (capacity),                                           \
        .head         = 0,                                                    \
        .tail         = D_CIRCULAR_ARRAY_COUNT_T(element_type, __VA_ARGS__)   \
    }


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
struct d_circular_array* d_circular_array_new(size_t _capacity, size_t _element_size);
struct d_circular_array* d_circular_array_new_default_capacity(size_t _element_size);
struct d_circular_array* d_circular_array_new_from_arr(size_t _capacity, size_t _element_size, const void* _source, size_t _source_count);
struct d_circular_array* d_circular_array_new_from_args(size_t _capacity, size_t _element_size, size_t _arg_count, ...);
struct d_circular_array* d_circular_array_new_copy(const struct d_circular_array* _other);
struct d_circular_array* d_circular_array_new_copy_resized(const struct d_circular_array* _other, size_t _new_capacity);
struct d_circular_array* d_circular_array_new_fill(size_t _capacity, size_t _element_size, const void* _fill_value);

// =============================================================================
// element access functions
// =============================================================================
void*  d_circular_array_get(const struct d_circular_array* _circular_array, d_index _index);
bool   d_circular_array_set(struct d_circular_array* _circular_array, d_index _index, const void* _value);
void*  d_circular_array_front(const struct d_circular_array* _circular_array);
void*  d_circular_array_back(const struct d_circular_array* _circular_array);
void*  d_circular_array_peek(const struct d_circular_array* _circular_array);
void*  d_circular_array_peek_back(const struct d_circular_array* _circular_array);

// =============================================================================
// modification functions - push/pop operations
// =============================================================================
bool   d_circular_array_push(struct d_circular_array* _circular_array, const void* _element);
bool   d_circular_array_push_front(struct d_circular_array* _circular_array, const void* _element);
bool   d_circular_array_push_back(struct d_circular_array* _circular_array, const void* _element);
bool   d_circular_array_push_all(struct d_circular_array* _circular_array, const void* _elements, size_t _count);
bool   d_circular_array_push_all_front(struct d_circular_array* _circular_array, const void* _elements, size_t _count);
void*  d_circular_array_pop(struct d_circular_array* _circular_array);
void*  d_circular_array_pop_front(struct d_circular_array* _circular_array);
void*  d_circular_array_pop_back(struct d_circular_array* _circular_array);
bool   d_circular_array_pop_to(struct d_circular_array* _circular_array, void* _out_value);
bool   d_circular_array_pop_front_to(struct d_circular_array* _circular_array, void* _out_value);
bool   d_circular_array_pop_back_to(struct d_circular_array* _circular_array, void* _out_value);

// =============================================================================
// modification functions - overwriting operations
// =============================================================================
bool   d_circular_array_push_overwrite(struct d_circular_array* _circular_array, const void* _element);
bool   d_circular_array_push_front_overwrite(struct d_circular_array* _circular_array, const void* _element);
bool   d_circular_array_push_all_overwrite(struct d_circular_array* _circular_array, const void* _elements, size_t _count);

// =============================================================================
// modification functions - bulk operations
// =============================================================================
bool   d_circular_array_clear(struct d_circular_array* _circular_array);
bool   d_circular_array_fill(struct d_circular_array* _circular_array, const void* _fill_value);
bool   d_circular_array_rotate_left(struct d_circular_array* _circular_array, size_t _amount);
bool   d_circular_array_rotate_right(struct d_circular_array* _circular_array, size_t _amount);
bool   d_circular_array_reverse(struct d_circular_array* _circular_array);
bool   d_circular_array_swap(struct d_circular_array* _circular_array, d_index _index_a, d_index _index_b);

// =============================================================================
// query functions
// =============================================================================
bool   d_circular_array_is_empty(const struct d_circular_array* _circular_array);
bool   d_circular_array_is_full(const struct d_circular_array* _circular_array);
size_t d_circular_array_count(const struct d_circular_array* _circular_array);
size_t d_circular_array_capacity(const struct d_circular_array* _circular_array);
size_t d_circular_array_available_space(const struct d_circular_array* _circular_array);
size_t d_circular_array_element_size(const struct d_circular_array* _circular_array);

// =============================================================================
// search functions
// =============================================================================
bool    d_circular_array_contains(const struct d_circular_array* _circular_array, const void* _value, fn_comparator _comparator);
ssize_t d_circular_array_find(const struct d_circular_array* _circular_array, const void* _value, fn_comparator _comparator);
ssize_t d_circular_array_find_last(const struct d_circular_array* _circular_array, const void* _value, fn_comparator _comparator);
size_t  d_circular_array_count_value(const struct d_circular_array* _circular_array, const void* _value, fn_comparator _comparator);

// =============================================================================
// conversion functions
// =============================================================================
void*   d_circular_array_to_linear_array(const struct d_circular_array* _circular_array);
bool    d_circular_array_copy_to(const struct d_circular_array* _circular_array, void* _destination, size_t _dest_capacity);

// =============================================================================
// iteration helpers
// =============================================================================
void   d_circular_array_foreach(struct d_circular_array* _circular_array, fn_apply _apply_fn);
void   d_circular_array_foreach_reverse(struct d_circular_array* _circular_array, fn_apply _apply_fn);

// =============================================================================
// utility functions
// =============================================================================
void   d_circular_array_sort(struct d_circular_array* _circular_array, fn_comparator _comparator);
bool   d_circular_array_linearize(struct d_circular_array* _circular_array);

// =============================================================================
// memory management
// =============================================================================
void   d_circular_array_free(struct d_circular_array* _circular_array);
void   d_circular_array_free_deep(struct d_circular_array* _circular_array, fn_free _free_fn);


#endif  // DJINTERP_CONTAINER_ARRAY_CIRCULAR_
