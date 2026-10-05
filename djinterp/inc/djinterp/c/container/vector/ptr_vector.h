/*******************************************************************************
* djinterp [c]                                                      ptr_vector.h
*
*   A dynamic array (vector) optimized for storing pointers. This
* implementation maintains both capacity and count, allowing for efficient
* growth operations without requiring element size to be stored (since all
* elements are pointers).
*   For vectors of arbitrary element types, see `vector.h`.
*
*
* path:      /inc/djinterp/c/container/vector/ptr_vector.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_VECTOR_PTR_VECTOR_H
#define DJINTERP_C_CONTAINER_VECTOR_PTR_VECTOR_H 1

// std
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../dmacro.h"  // D_VARG_COUNT, D_FOR_EACH_DATA_COMMA
#include "../../memory/dmemory.h"
#include "../container.h"
#include "./vector_common.h"


#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_PTR_VECTOR_INIT
//   macro: brace initializer for a stack-allocated `d_ptr_vector`. Sets both
// `count` and `capacity` to the number of variadic arguments, so the vector
// starts full; growing it requires storage the macro did not allocate.
//   Usage: struct d_ptr_vector vec = D_PTR_VECTOR_INIT(Item, {"a",1},
// {"b",2});
#define D_PTR_VECTOR_INIT(element_type, ...)                              \
    {                                                                     \
        .count = D_VARG_COUNT(__VA_ARGS__),                               \
        .elements = (void*[]){ D_FOR_EACH_DATA_COMMA(D_INTERNAL_PTR_ELEM, \
                                                     element_type,        \
                                                     __VA_ARGS__) },      \
        .capacity = D_VARG_COUNT(__VA_ARGS__)                             \
    }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// d_ptr_vector
//   struct: a dynamic array optimized for storing pointers. Does not require
// element_size since all elements are sizeof(void*).
struct d_ptr_vector
{
    size_t count;
    void** elements;
    size_t capacity;
};


// constructor functions
// I.    creation and initialization
// I.    creation and initialization
struct d_ptr_vector* d_ptr_vector_new(size_t _initial_capacity);
struct d_ptr_vector* d_ptr_vector_new_default(void);
struct d_ptr_vector* d_ptr_vector_new_from_array(const void** _source,
                                                 size_t       _count);
struct d_ptr_vector* d_ptr_vector_new_from_args(size_t _arg_count,
                                                ...);
struct d_ptr_vector* d_ptr_vector_new_copy(const struct d_ptr_vector* _other);
struct d_ptr_vector* d_ptr_vector_new_fill(size_t _count,
                                           void*  _value);
struct d_ptr_vector* d_ptr_vector_new_merge(size_t _count,
                                            ...);

// II.   element access
bool   d_ptr_vector_push_back(struct d_ptr_vector* _ptr_vector,
                              void*                _value);
bool   d_ptr_vector_push_front(struct d_ptr_vector* _ptr_vector,
                               void*                _value);
void*  d_ptr_vector_pop_back(struct d_ptr_vector* _ptr_vector);
void*  d_ptr_vector_pop_front(struct d_ptr_vector* _ptr_vector);
void*  d_ptr_vector_at(const struct d_ptr_vector* _ptr_vector,
                       d_index                    _index);
void*  d_ptr_vector_front(const struct d_ptr_vector* _ptr_vector);
void*  d_ptr_vector_back(const struct d_ptr_vector* _ptr_vector);
void** d_ptr_vector_data(const struct d_ptr_vector* _ptr_vector);
void*  d_ptr_vector_get(const struct d_ptr_vector* _ptr_vector,
                        d_index                    _index);
bool   d_ptr_vector_set(struct d_ptr_vector* _ptr_vector,
                        d_index              _index,
                        void*                _value);

// III.  insertion and removal
bool  d_ptr_vector_insert(struct d_ptr_vector* _ptr_vector,
                          d_index              _index,
                          void*                _value);
bool  d_ptr_vector_insert_range(struct d_ptr_vector* _ptr_vector,
                                d_index              _index,
                                const void**         _source,
                                size_t               _count);
bool  d_ptr_vector_erase(struct d_ptr_vector* _ptr_vector,
                         d_index              _index);
bool  d_ptr_vector_erase_range(struct d_ptr_vector* _ptr_vector,
                               d_index              _start,
                               d_index              _end);
void* d_ptr_vector_remove(struct d_ptr_vector* _ptr_vector,
                          d_index              _index);
bool  d_ptr_vector_append(struct d_ptr_vector* _ptr_vector,
                          const void**         _source,
                          size_t               _count);
bool  d_ptr_vector_append_vector(struct d_ptr_vector*       _destination,
                                 const struct d_ptr_vector* _source);
bool  d_ptr_vector_prepend(struct d_ptr_vector* _ptr_vector,
                           const void**         _source,
                           size_t               _count);
bool  d_ptr_vector_prepend_vector(struct d_ptr_vector*       _destination,
                                  const struct d_ptr_vector* _source);

// IV.   search
ssize_t d_ptr_vector_find(const struct d_ptr_vector* _ptr_vector,
                          const void*                _value,
                          fn_comparator              _comparator);
ssize_t d_ptr_vector_find_last(const struct d_ptr_vector* _ptr_vector,
                               const void*                _value,
                               fn_comparator              _comparator);
ssize_t d_ptr_vector_find_ptr(const struct d_ptr_vector* _ptr_vector,
                              const void*                _ptr);
bool    d_ptr_vector_contains(const struct d_ptr_vector* _ptr_vector,
                              const void*                _value,
                              fn_comparator              _comparator);
bool    d_ptr_vector_contains_ptr(const struct d_ptr_vector* _ptr_vector,
                                  const void*                _ptr);
size_t  d_ptr_vector_count_value(const struct d_ptr_vector* _ptr_vector,
                                 const void*                _value,
                                 fn_comparator              _comparator);

// V.    capacity and state queries
bool   d_ptr_vector_ensure_capacity(struct d_ptr_vector* _ptr_vector,
                                    size_t               _required);
bool   d_ptr_vector_is_empty(const struct d_ptr_vector* _ptr_vector);
bool   d_ptr_vector_is_full(const struct d_ptr_vector* _ptr_vector);
size_t d_ptr_vector_size(const struct d_ptr_vector* _ptr_vector);
size_t d_ptr_vector_capacity(const struct d_ptr_vector* _ptr_vector);

// VI.   iteration
void d_ptr_vector_foreach(struct d_ptr_vector* _ptr_vector,
                          fn_apply             _apply_fn);

// VII.  mutation and reordering
bool d_ptr_vector_reserve(struct d_ptr_vector* _ptr_vector,
                          size_t               _new_capacity);
void d_ptr_vector_clear(struct d_ptr_vector* _ptr_vector);
bool d_ptr_vector_resize(struct d_ptr_vector* _ptr_vector,
                         size_t               _new_count);
bool d_ptr_vector_resize_fill(struct d_ptr_vector* _ptr_vector,
                              size_t               _new_count,
                              void*                _fill_value);
bool d_ptr_vector_swap(struct d_ptr_vector* _ptr_vector,
                       d_index              _index_a,
                       d_index              _index_b);
bool d_ptr_vector_reverse(struct d_ptr_vector* _ptr_vector);
void d_ptr_vector_sort(struct d_ptr_vector* _ptr_vector,
                       fn_comparator        _comparator);
void d_ptr_vector_clear_deep(struct d_ptr_vector* _ptr_vector,
                             fn_free              _free_fn);

// VIII. conversion and reporting
bool d_ptr_vector_shrink_to_fit(struct d_ptr_vector* _ptr_vector);
bool d_ptr_vector_copy_to(const struct d_ptr_vector* _ptr_vector,
                          void**                     _destination,
                          size_t                     _dest_capacity);

// IX.   operations
size_t d_ptr_vector_available(const struct d_ptr_vector* _ptr_vector);

// X.    destruction
void d_ptr_vector_free(struct d_ptr_vector* _ptr_vector);
void d_ptr_vector_free_deep(struct d_ptr_vector* _ptr_vector,
                            fn_free              _free_fn);

#endif  // DJINTERP_C_CONTAINER_VECTOR_PTR_VECTOR_H
