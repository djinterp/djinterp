/******************************************************************************
* djinterp [container]                                         vector_common.h
*
*   This module provides common building blocks for vector data structures.
* Vectors are dynamic arrays that maintain both a capacity (allocated storage)
* and a count (number of active elements), allowing for efficient growth and
* shrinkage operations.
*   Functions in this module operate on raw pointers and size values, enabling
* reuse across different vector implementations such as `d_vector` and 
* `d_vector_s`.
*
*
* path:      \inc\container\vector\vector_common.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.14
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_VECTOR_COMMON_
#define DJINTERP_C_CONTAINER_VECTOR_COMMON_ 1

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../../djinterp.h"
#include "../../dmemory.h"
#include "../container.h"
#include "../array/array_common.h"


#ifndef D_VECTOR_DEFAULT_CAPACITY
	// D_VECTOR_DEFAULT_CAPACITY
	//   definition: the default size, in number of elements, that a new vector 
	// has by default.
	#define D_VECTOR_DEFAULT_CAPACITY D_ARRAY_DEFAULT_CAPACITY
#endif	// D_VECTOR_DEFAULT_CAPACITY

#ifndef D_VECTOR_GROWTH_FACTOR
// D_VECTOR_GROWTH_FACTOR
//   constant: the multiplicative factor used when a vector's capacity is
// automatically increased. A value of 2.0 means capacity doubles on growth.
#define D_VECTOR_GROWTH_FACTOR 2.0
#endif	// D_VECTOR_GROWTH_FACTOR

#ifndef D_VECTOR_SHRINK_THRESHOLD
	// D_VECTOR_SHRINK_THRESHOLD
	//   constant: the ratio of count to capacity below which a vector may be
	// considered for shrinking. A value of 0.25 means shrink when less than 25%
	// of capacity is in use.
	#define D_VECTOR_SHRINK_THRESHOLD 0.25
#endif	// D_VECTOR_SHRINK_THRESHOLD

#ifndef D_VECTOR_MIN_CAPACITY
	// D_VECTOR_MIN_CAPACITY
	//   constant: the minimum capacity a vector will shrink to. Prevents excessive
	// reallocation for small vectors.
	#define D_VECTOR_MIN_CAPACITY 4
#endif	// D_VECTOR_MIN_CAPACITY


// initialization functions
bool   d_vector_common_init(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, size_t _initial_capacity);
bool   d_vector_common_init_from_array(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, const void* _source, size_t _source_count);
bool   d_vector_common_init_from_args(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, size_t _arg_count, va_list _args);
bool   d_vector_common_init_copy(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, const void* _source, size_t _source_count, size_t _source_capacity);
bool   d_vector_common_init_fill(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, size_t _size, const void* _value);

// capacity management functions
bool   d_vector_common_reserve(void** _elements, size_t _count, size_t* _capacity, size_t _element_size, size_t _new_capacity);
bool   d_vector_common_shrink_to_fit(void** _elements, size_t _count, size_t* _capacity, size_t _element_size);
bool   d_vector_common_ensure_capacity(void** _elements, size_t _count, size_t* _capacity, size_t _element_size, size_t _required);
bool   d_vector_common_grow(void** _elements, size_t _count, size_t* _capacity, size_t _element_size);
bool   d_vector_common_maybe_shrink(void** _elements, size_t _count, size_t* _capacity, size_t _element_size);
size_t d_vector_common_available(size_t _count, size_t _capacity);

// element manipulation functions
bool   d_vector_common_push_back(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, const void* _value);
bool   d_vector_common_push_front(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, const void* _value);
bool   d_vector_common_pop_back(void* _elements, size_t* _count, size_t _element_size, void* _out_value);
bool   d_vector_common_pop_front(void* _elements, size_t* _count, size_t _element_size, void* _out_value);
bool   d_vector_common_insert_element(void** _destination, size_t* _count, size_t* _capacity, size_t _element_size, d_index _index, const void* _value);
bool   d_vector_common_insert_elements(void** _destination, size_t* _count, size_t* _capacity, size_t _element_size, d_index _index, const void* _source, size_t _source_count);
bool   d_vector_common_erase(void* _destination, size_t* _count, size_t _element_size, d_index _index);
bool   d_vector_common_erase_range(void* _destination, size_t* _count, size_t _element_size, d_index _start, d_index  _end);

// append/prepend functions
bool   d_vector_common_append_element(void** _destination, size_t* _count, size_t* _capacity, size_t _element_size, const void* _element);
bool   d_vector_common_append_elements(void** _destination, size_t* _count, size_t* _capacity, size_t _element_size, const void* _source, size_t _source_count);
bool   d_vector_common_prepend_element(void** _destination, size_t* _count, size_t* _capacity, size_t _element_size, const void* _element);
bool   d_vector_common_prepend_elements(void** _destination, size_t* _count, size_t* _capacity, size_t _element_size, const void* _source, size_t _source_count);

// resize functions
bool   d_vector_common_resize(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, size_t _new_count);
bool   d_vector_common_resize_fill(void** _elements, size_t* _count, size_t* _capacity, size_t _element_size, size_t _new_count, const void* _fill_value);

// access functions
void*  d_vector_common_at(const void* _elements, size_t _count, size_t _element_size, d_index _index);
void*  d_vector_common_front(const void* _elements, size_t _count);
void*  d_vector_common_back(const void* _elements, size_t _count, size_t _element_size);
void*  d_vector_common_data(const void* _elements);

// query functions
bool   d_vector_common_is_empty(size_t _count);
bool   d_vector_common_is_full(size_t _count, size_t _capacity);
size_t d_vector_common_size(size_t _count);
size_t d_vector_common_capacity(size_t _capacity);

// utility functions
bool   d_vector_common_swap(void* _elements, size_t _count, size_t _element_size, d_index _index_a, d_index _index_b);
bool   d_vector_common_copy_to(const void* _source, size_t _count, size_t _element_size, void* _destination, size_t _dest_capacity);

// cleanup functions
void   d_vector_common_free_elements(void* _elements);


#endif	// DJINTERP_C_CONTAINER_VECTOR_COMMON_