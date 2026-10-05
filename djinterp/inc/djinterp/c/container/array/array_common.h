/*******************************************************************************
* djinterp [c]                                                    array_common.h
*
* Common utilities and helper functions for array implementations.
*
*
* path:      /inc/djinterp/c/container/array/array_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.13
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ARRAY_ARRAY_COMMON_H
#define DJINTERP_C_CONTAINER_ARRAY_ARRAY_COMMON_H 1

// std
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../container.h"


#ifndef D_ARRAY_DEFAULT_CAPACITY
    // D_ARRAY_DEFAULT_CAPACITY
    //   constant: element count a `d_array` is given when it is created
    // without an explicit capacity. Overridable before this header is
    // included.
    #define D_ARRAY_DEFAULT_CAPACITY 32
#endif  // D_ARRAY_DEFAULT_CAPACITY


// I.    creation and initialization
// I.    creation and initialization
bool  d_array_common_init_sized(void**  _destination,
                                size_t* _count,
                                size_t  _element_size,
                                size_t  _size);
bool  d_array_common_init_from_array(void**      _destination,
                                     size_t*     _count,
                                     size_t      _element_size,
                                     const void* _source,
                                     size_t      _source_count);
bool  d_array_common_init_from_args(void**  _destination,
                                    size_t* _count,
                                    size_t  _element_size,
                                    size_t  _arg_count,
                                    va_list _args);
bool  d_array_common_init_copy(void**      _destination,
                               size_t*     _count,
                               size_t      _element_size,
                               const void* _source,
                               size_t      _source_count);
bool  d_array_common_init_copy_reverse(void**      _destination,
                                       size_t*     _count,
                                       size_t      _element_size,
                                       const void* _source,
                                       size_t      _source_count,
                                       d_index     _start,
                                       d_index     _end);
bool  d_array_common_init_copy_range(void**      _destination,
                                     size_t*     _count,
                                     size_t      _element_size,
                                     const void* _source,
                                     size_t      _source_count,
                                     d_index     _start,
                                     d_index     _end);
bool  d_array_common_init_copy_range_reverse(void**      _destination,
                                             size_t*     _count,
                                             size_t      _element_size,
                                             const void* _source,
                                             size_t      _source_count,
                                             d_index     _start,
                                             d_index     _end);
bool  d_array_common_init_fill(void**      _destination,
                               size_t*     _count,
                               size_t      _element_size,
                               size_t      _size,
                               const void* _value);
bool  d_array_common_init_slice(void**       _destination,
                                size_t*      _count,
                                size_t       _element_size,
                                const void** _source,
                                size_t       _source_count,
                                d_index      _start);
bool  d_array_common_init_slice_reverse(void**       _destination,
                                        size_t*      _count,
                                        size_t       _element_size,
                                        const void** _source,
                                        size_t       _source_count);
bool  d_array_common_init_slice_range(void**       _destination,
                                      size_t*      _count,
                                      size_t       _element_size,
                                      const void** _source,
                                      size_t       _source_count,
                                      d_index      _start,
                                      d_index      _end);
bool  d_array_common_init_slice_range_reverse(void**       _destination,
                                              size_t*      _count,
                                              size_t       _element_size,
                                              const void** _source,
                                              size_t       _source_count,
                                              d_index      _start,
                                              d_index      _end);
void* d_array_common_alloc(size_t _element_size);

// II.   insertion and removal
bool d_array_common_append_element(void**      _elements,
                                   size_t*     _count,
                                   size_t      _element_size,
                                   const void* _value);
bool d_array_common_append_elements(void**      _elements,
                                    size_t*     _count,
                                    size_t      _element_size,
                                    const void* _value,
                                    size_t      _source_count);
bool d_array_common_insert_element(void**      _elements,
                                   size_t*     _count,
                                   size_t      _element_size,
                                   const void* _value,
                                   d_index     _index);
bool d_array_common_insert_elements(void**      _elements,
                                    size_t*     _count,
                                    size_t      _element_size,
                                    const void* _value,
                                    size_t      _source_count,
                                    d_index     _index);
bool d_array_common_prepend_element(void**      _elements,
                                    size_t*     _count,
                                    size_t      _element_size,
                                    const void* _value);
bool d_array_common_prepend_elements(void**      _elements,
                                     size_t*     _count,
                                     size_t      _element_size,
                                     const void* _value,
                                     size_t      _source_count);

// III.  search
bool    d_array_common_contains(void*         _elements,
                                size_t        _count,
                                size_t        _element_size,
                                const void*   _value,
                                fn_comparator _comparator);
ssize_t d_array_common_find(const void*   _elements,
                            size_t        _count,
                            size_t        _element_size,
                            const void*   _value,
                            fn_comparator _comparator);
ssize_t d_array_common_find_closest(const void*   _elements,
                                    size_t        _count,
                                    size_t        _element_size,
                                    const void*   _value,
                                    fn_comparator _comparator);

// IV.   capacity and state queries
size_t d_array_common_calc_capacity(size_t _requested_size);
bool   d_array_common_is_valid_resize_amount(size_t  _count,
                                             ssize_t _amount,
                                             size_t* _result);
bool   d_array_common_is_valid_resize_factor(size_t  _count,
                                             double  _factor,
                                             double* _result,
                                             bool    _round_down);

// V.    mutation and reordering
int     d_array_common_fill(void*       _elements,
                            size_t      _count,
                            size_t      _element_size,
                            const void* _fill_value);
ssize_t d_array_common_resize_amount(void*   _elements,
                                     size_t  _count,
                                     size_t  _element_size,
                                     ssize_t _amount);
ssize_t d_array_common_resize_factor(void*  _elements,
                                     size_t _count,
                                     size_t _element_size,
                                     double _factor);
bool    d_array_common_reverse(void*  _elements,
                               size_t _count,
                               size_t _element_size);
bool    d_array_common_shift_left(void*  _elements,
                                  size_t _count,
                                  size_t _element_size,
                                  size_t _amount);
bool    d_array_common_shift_left_circular(void*  _elements,
                                           size_t _count,
                                           size_t _element_size,
                                           size_t _amount);
bool    d_array_common_shift_right(void*  _elements,
                                   size_t _count,
                                   size_t _element_size,
                                   size_t _amount);
bool    d_array_common_shift_right_circular(void*  _elements,
                                            size_t _count,
                                            size_t _element_size,
                                            size_t _amount);
void    d_array_common_sort(void*         _elements,
                            size_t        _count,
                            size_t        _element_size,
                            fn_comparator _comparator);

// VI.   destruction
void d_array_common_free_elements_arr(void* _elements);
void d_array_common_free_elements_deep(size_t  _count,
                                       void**  _elements,
                                       fn_free _free_fn);

#endif  // DJINTERP_C_CONTAINER_ARRAY_ARRAY_COMMON_H
