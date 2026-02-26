/******************************************************************************
* djinterp [container]                                             ptr_array.h
*
* A `d_array` is a dynamically-resizable array.
* Does not allow NULL-elements.
*
*
* 
* path:      \inc\container\array\array.h      
* links:     TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.29
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_PTR_ARRAY_
#define DJINTERP_C_CONTAINER_PTR_ARRAY_ 1

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../../djinterp.h"
#include "../../dmemory.h"
#include "../container.h"
#include "./array_common.h"


#ifndef D_PTR_ARRAY_DEFAULT_CAPACITY
	// D_PTR_ARRAY_DEFAULT_CAPACITY
	//   definition: the default size, in number of elements, that a new
	// `d_ptr_array` has by default.
	#define D_PTR_ARRAY_DEFAULT_CAPACITY 32
#endif	// D_PTR_ARRAY_DEFAULT_CAPACITY

// D_PTR_ARRAY_INIT - initialize d_ptr_array with struct initializers
//   Usage: struct d_ptr_array arr = D_PTR_ARRAY_INIT(Item, {"a",1}, {"b",2});
#define D_PTR_ARRAY_INIT(element_type, ...)                                \
    { 										                               \
        .count    = D_VARG_COUNT(__VA_ARGS__),                             \
        .elements = (void*[])				                               \
        {                                                                  \
			D_FOR_EACH_DATA_COMMA(D_INTERNAL_PTR_ELEM,                     \
                                  element_type,                            \
                                  __VA_ARGS__)                             \
        }                                                                  \
    }



// d_ptr_array
//   struct: 
struct d_ptr_array
{
	size_t count;
	void** elements;
};

// 
struct d_ptr_array* d_ptr_array_new(size_t _initial_size);
struct d_ptr_array* d_ptr_array_new_default_size(void);
struct d_ptr_array* d_ptr_array_new_from_arr(const void** _source, size_t _count);
struct d_ptr_array* d_ptr_array_new_from_args(size_t _arg_count , ...);
struct d_ptr_array* d_ptr_array_new_copy(const struct d_ptr_array* _other);
struct d_ptr_array* d_ptr_array_new_merge(size_t _count, ...);
struct d_ptr_array* d_ptr_array_new_slice(const struct d_ptr_array* _ptr_array, d_index _start);
struct d_ptr_array* d_ptr_array_new_slice_range(const struct d_ptr_array* _ptr_array, d_index _start, d_index _end);

bool    d_ptr_array_append_element(struct d_ptr_array* _ptr_array, const void* _element);
bool    d_ptr_array_append_elements(struct d_ptr_array* _ptr_array, const void** _elements, size_t _count);
bool    d_ptr_array_append_array(struct d_ptr_array* _ptr_array, const struct d_ptr_array* _source);
bool    d_ptr_array_contains(const struct d_ptr_array* _ptr_array, const void* _value);
bool    d_ptr_array_fill(struct d_ptr_array* _ptr_array, const void* _fill_element);
ssize_t d_ptr_array_find(struct d_ptr_array* _ptr_array, const void* _value);
bool    d_ptr_array_insert_element(struct d_ptr_array* _ptr_array, const void* _element, d_index _index);
bool    d_ptr_array_insert_elements(struct d_ptr_array* _ptr_array, const void** _elements, size_t _count, d_index _index);
bool    d_ptr_array_insert_array(struct d_ptr_array* _destination, const struct d_ptr_array* _source, d_index _index);
bool    d_ptr_array_is_empty(const struct d_ptr_array* _ptr_array);
bool    d_ptr_array_prepend_element(struct d_ptr_array* _ptr_array, const void* _element);
bool    d_ptr_array_prepend_elements(struct d_ptr_array* _ptr_array, const void** _elements, size_t _count);
bool    d_ptr_array_prepend_array(struct d_ptr_array* _destination, const struct d_ptr_array* _source);
bool    d_ptr_array_reverse(struct d_ptr_array* _ptr_array);
bool    d_ptr_array_resize_amount(struct d_ptr_array* _ptr_array, ssize_t _amount);
bool    d_ptr_array_resize_factor(struct d_ptr_array* _ptr_array, double _factor);
void    d_ptr_array_sort(struct d_ptr_array* _ptr_array, fn_comparator _comparator);

void    d_ptr_array_free(struct d_ptr_array* _ptr_array);
void    d_ptr_array_deep_free(struct d_ptr_array* _ptr_array, fn_free _free_fn);


#endif	// DJINTERP_C_CONTAINER_PTR_ARRAY_