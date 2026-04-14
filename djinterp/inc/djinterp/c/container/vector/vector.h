/******************************************************************************
* djinterp [container]                                                vector.h
*
*   A `d_vector` is a dynamically-resizable vector. This module provides a
* struct-based wrapper around the vector_common functions for convenient use.
*
*
* path:      \inc\container\vector\vector.h   
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.30
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_VECTOR_
#define DJINTERP_C_CONTAINER_VECTOR_ 1

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../../djinterp.h"
#include "../../dmemory.h"
#include "../container.h"
#include "./vector_common.h"


// D_VECTOR_INIT
//   macro: 
#define D_VECTOR_INIT(element_type, ...)                              \
    {                                                                 \
        .elements     = (element_type[]){ __VA_ARGS__ },              \
        .element_size = sizeof(element_type),                         \
        .capacity     = D_ARRAY_COUNT_T(element_type,                 \
                                        __VA_ARGS__),                 \
        .count        = D_ARRAY_COUNT_T(element_type,                 \
                                        __VA_ARGS__)                  \
    }

// D_VECTOR_INIT_CAPACITY
//   macro: 
#define D_VECTOR_INIT_CAPACITY(element_type,                          \
                               initial_capacity,                      \
                               ...)                                   \
    {                                                                 \
        .elements     = (element_type[]){ __VA_ARGS__ },              \
        .element_size = sizeof(element_type),                         \
        .capacity     = initial_capacity,                             \
        .count        = D_ARRAY_COUNT_T(element_type,                 \
                                        __VA_ARGS__)                  \
    }


// d_vector
//   struct: a dynamically-resizable vector
struct d_vector
{
	void*  elements;
	size_t element_size;
	size_t capacity;
	size_t count;
};


// constructor functions
struct d_vector* d_vector_new(size_t _element_size, size_t _initial_capacity);
struct d_vector* d_vector_new_default(size_t _element_size);
struct d_vector* d_vector_new_from_array(size_t _element_size, const void* _source, size_t _count);
struct d_vector* d_vector_new_from_args(size_t _element_size, size_t _arg_count, ...);
struct d_vector* d_vector_new_copy(const struct d_vector* _other);
struct d_vector* d_vector_new_fill(size_t _element_size, size_t _count, const void* _value);

// capacity management functions
bool   d_vector_reserve(struct d_vector* _vector, size_t _new_capacity);
bool   d_vector_shrink_to_fit(struct d_vector* _vector);
bool   d_vector_ensure_capacity(struct d_vector* _vector, size_t _required);
bool   d_vector_grow(struct d_vector* _vector);
bool   d_vector_maybe_shrink(struct d_vector* _vector);
size_t d_vector_available(const struct d_vector* _vector);

// element manipulation functions
bool   d_vector_push_back(struct d_vector* _vector, const void* _value);
bool   d_vector_push_front(struct d_vector* _vector, const void* _value);
bool   d_vector_pop_back(struct d_vector* _vector, void* _out_value);
bool   d_vector_pop_front(struct d_vector* _vector, void* _out_value);
bool   d_vector_insert_element(struct d_vector* _vector, d_index _index, const void* _value);
bool   d_vector_insert_elements(struct d_vector* _vector, d_index _index, const void* _source, size_t _count);
bool   d_vector_erase(struct d_vector* _vector, d_index _index);
bool   d_vector_erase_range(struct d_vector* _vector, d_index _start, d_index _end);
void   d_vector_clear(struct d_vector* _vector);

// append/prepend functions
bool   d_vector_append_element(struct d_vector* _vector, const void* _element);
bool   d_vector_append_elements(struct d_vector* _vector, const void* _source, size_t _count);
bool   d_vector_append_vector(struct d_vector* _destination, const struct d_vector* _source);
bool   d_vector_prepend_element(struct d_vector* _vector, const void* _element);
bool   d_vector_prepend_elements(struct d_vector* _vector, const void* _source, size_t _count);
bool   d_vector_prepend_vector(struct d_vector* _destination, const struct d_vector* _source);

// resize functions
bool   d_vector_resize(struct d_vector* _vector, size_t _new_count);
bool   d_vector_resize_fill(struct d_vector* _vector, size_t _new_count, const void* _fill_value);

// access functions
void*  d_vector_at(const struct d_vector* _vector, d_index _index);
void*  d_vector_front(const struct d_vector* _vector);
void*  d_vector_back(const struct d_vector* _vector);
void*  d_vector_data(const struct d_vector* _vector);
bool   d_vector_get(const struct d_vector* _vector, d_index _index, void* _out_value);
bool   d_vector_set(struct d_vector* _vector, d_index _index, const void* _value);

// query functions
bool   d_vector_is_empty(const struct d_vector* _vector);
bool   d_vector_is_full(const struct d_vector* _vector);
size_t d_vector_size(const struct d_vector* _vector);
size_t d_vector_capacity(const struct d_vector* _vector);
size_t d_vector_element_size(const struct d_vector* _vector);

// search functions
ssize_t d_vector_find(const struct d_vector* _vector, const void* _value, fn_comparator _comparator);
ssize_t d_vector_find_last(const struct d_vector* _vector, const void* _value, fn_comparator _comparator);
bool    d_vector_contains(const struct d_vector* _vector, const void* _value, fn_comparator _comparator);
size_t  d_vector_count_value(const struct d_vector* _vector, const void* _value, fn_comparator _comparator);

// utility functions
bool    d_vector_swap(struct d_vector* _vector, d_index _index_a, d_index _index_b);
bool    d_vector_reverse(struct d_vector* _vector);
void    d_vector_sort(struct d_vector* _vector, fn_comparator _comparator);
bool    d_vector_copy_to(const struct d_vector* _vector, void* _destination, size_t _dest_capacity);

// destructor functions
void    d_vector_free(struct d_vector* _vector);
void    d_vector_free_deep(struct d_vector* _vector, fn_free _free_fn);


#endif	// DJINTERP_C_CONTAINER_VECTOR_
