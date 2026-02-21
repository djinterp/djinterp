/******************************************************************************
* djinterp [container]                                          array_common.h
*
* Common utilities and helper functions for array implementations.
* 
*
* path:      \inc\container\array\array_common.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.13
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ARRAY_COMMON_
#define DJINTERP_C_CONTAINER_ARRAY_COMMON_ 1

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include "..\..\djinterp.h"
#include "..\..\dmemory.h"
#include "..\container.h"


#ifndef D_ARRAY_DEFAULT_CAPACITY
	// D_PTR_ARRAY_DEFAULT_CAPACITY
	//   definition: the default size, in number of elements, that a new
	// `d_array` has by default.
	#define D_ARRAY_DEFAULT_CAPACITY 32
#endif	// D_ARRAY_DEFAULT_CAPACITY


bool    d_array_common_init_sized(void** _destination, size_t* _count, size_t _element_size, size_t _size);
bool    d_array_common_init_from_array(void** _destination, size_t* _count, size_t _element_size, const void* _source, size_t _source_count);
bool    d_array_common_init_from_args(void** _destination, size_t* _count, size_t _element_size, size_t _arg_count, va_list _args);
bool    d_array_common_init_copy(void** _destination, size_t* _count, size_t _element_size, const void* _source, size_t _source_count);
bool    d_array_common_init_copy_reverse(void** _destination, size_t* _count, size_t _element_size, const void* _source, size_t _source_count, d_index _start, d_index _end);
bool    d_array_common_init_copy_range(void** _destination, size_t* _count, size_t _element_size, const void* _source, size_t _source_count, d_index _start, d_index _end);
bool    d_array_common_init_copy_range_reverse(void** _destination, size_t* _count, size_t _element_size, const void* _source, size_t _source_count, d_index _start, d_index _end);
bool    d_array_common_init_fill(void** _destination, size_t* _count, size_t _element_size, size_t _size, const void* _value);
bool    d_array_common_init_slice(void** _destination, size_t* _count, size_t _element_size, const void** _source, size_t _source_count, d_index _start);
bool    d_array_common_init_slice_reverse(void** _destination, size_t* _count, size_t _element_size, const void** _source, size_t _source_count);
bool    d_array_common_init_slice_range(void** _destination, size_t* _count, size_t _element_size, const void** _source, size_t _source_count, d_index _start, d_index _end);
bool    d_array_common_init_slice_range_reverse(void** _destination, size_t* _count, size_t _element_size, const void** _source, size_t _source_count, d_index _start, d_index _end);

void*   d_array_common_alloc(size_t _element_size);
bool    d_array_common_append_element(void** _elements, size_t* _count, size_t _element_size, const void* _value);
bool    d_array_common_append_elements(void** _elements, size_t* _count, size_t _element_size, const void* _value, size_t _source_count);
size_t  d_array_common_calc_capacity(size_t _requested_size);
bool    d_array_common_contains(void* _elements, size_t _count, size_t _element_size, const void* _value, fn_comparator _comparator);
int     d_array_common_fill(void* _elements, size_t _count, size_t _element_size, const void* _fill_value);
ssize_t d_array_common_find(const void* _elements, size_t _count, size_t _element_size, const void* _value, fn_comparator _comparator);
ssize_t d_array_common_find_closest(const void* _elements, size_t _count, size_t _element_size, const void* _value, fn_comparator _comparator);
bool    d_array_common_insert_element(void** _elements, size_t* _count, size_t _element_size, const void* _value, d_index _index);
bool    d_array_common_insert_elements(void** _elements, size_t* _count, size_t _element_size, const void* _value, size_t _source_count, d_index _index);
bool    d_array_common_is_valid_resize_amount(size_t _count, ssize_t _amount, size_t* _result);
bool    d_array_common_is_valid_resize_factor(size_t _count, double _factor, double* _result, bool _round_down);
bool    d_array_common_prepend_element(void** _elements, size_t* _count, size_t _element_size, const void* _value);
bool    d_array_common_prepend_elements(void** _elements, size_t* _count, size_t _element_size, const void* _value, size_t _source_count);
ssize_t d_array_common_resize_amount(void* _elements, size_t _count, size_t _element_size, ssize_t _amount);
ssize_t d_array_common_resize_factor(void* _elements, size_t _count, size_t _element_size, double _factor);
bool    d_array_common_reverse(void* _elements, size_t _count, size_t _element_size);
bool    d_array_common_shift_left(void* _elements, size_t _count, size_t _element_size, size_t _amount);
bool    d_array_common_shift_left_circular(void* _elements, size_t _count, size_t _element_size, size_t _amount);
bool    d_array_common_shift_right(void* _elements, size_t _count, size_t _element_size, size_t _amount);
bool    d_array_common_shift_right_circular(void* _elements, size_t _count, size_t _element_size, size_t _amount);
void    d_array_common_sort(void* _elements, size_t _count, size_t _element_size, fn_comparator _comparator);

void    d_array_common_free_elements_arr(void* _elements);
void    d_array_common_free_elements_deep(size_t _count, void** _elements, fn_free _free_fn);


#endif	// DJINTERP_C_CONTAINER_ARRAY_COMMON_