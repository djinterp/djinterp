/*******************************************************************************
* djinterp [c]                                                          vector.h
*
*   A `d_vector` is a dynamically-resizable vector. This module provides a
* struct-based wrapper around the vector_common functions for convenient use.
*
*
* path:      /inc/djinterp/c/container/vector/vector.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_VECTOR_VECTOR_H
#define DJINTERP_C_CONTAINER_VECTOR_VECTOR_H 1

// std
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../dmacro.h"  // D_ARRAY_COUNT_T
#include "../../memory/dmemory.h"
#include "../container.h"
#include "./vector_common.h"


#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_VECTOR_INIT
//   macro: brace initializer for a stack-allocated `d_vector`. Backs the
// vector with a compound literal of `element_type` and sets `capacity` equal
// to `count`, so the vector starts full and cannot grow without first being
// given heap storage.
#define D_VECTOR_INIT(element_type, ...)                 \
    {                                                    \
        .elements     = (element_type[]){ __VA_ARGS__ }, \
        .element_size = sizeof(element_type),            \
        .capacity     = D_ARRAY_COUNT_T(element_type,    \
                                        __VA_ARGS__),    \
        .count        = D_ARRAY_COUNT_T(element_type,    \
                                        __VA_ARGS__)     \
    }

// D_VECTOR_INIT_CAPACITY
//   macro: as D_VECTOR_INIT, but takes `initial_capacity` explicitly. The
// caller is responsible for the literal being large enough; a capacity above
// the number of arguments does not itself reserve storage.
#define D_VECTOR_INIT_CAPACITY(element_type,             \
                               initial_capacity,         \
                               ...)                      \
    {                                                    \
        .elements     = (element_type[]){ __VA_ARGS__ }, \
        .element_size = sizeof(element_type),            \
        .capacity     = initial_capacity,                \
        .count        = D_ARRAY_COUNT_T(element_type,    \
                                        __VA_ARGS__)     \
    }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS


// d_vector
//   struct: a resizable array that stores its own element stride, so
// operations need not be told the element size. `capacity` is the allocated
// element count and `count` the number in use.
struct d_vector
{
    void*  elements;
    size_t element_size;
    size_t capacity;
    size_t count;
};


// constructor functions
// I.    creation and initialization
// I.    creation and initialization
struct d_vector* d_vector_new(size_t _element_size,
                              size_t _initial_capacity);
struct d_vector* d_vector_new_default(size_t _element_size);
struct d_vector* d_vector_new_from_array(size_t      _element_size,
                                         const void* _source,
                                         size_t      _count);
struct d_vector* d_vector_new_from_args(size_t _element_size,
                                        size_t _arg_count,
                                        ...);
struct d_vector* d_vector_new_copy(const struct d_vector* _other);
struct d_vector* d_vector_new_fill(size_t      _element_size,
                                   size_t      _count,
                                   const void* _value);

// II.   element access
bool  d_vector_push_back(struct d_vector* _vector,
                         const void*      _value);
bool  d_vector_push_front(struct d_vector* _vector,
                          const void*      _value);
bool  d_vector_pop_back(struct d_vector* _vector,
                        void*            _out_value);
bool  d_vector_pop_front(struct d_vector* _vector,
                         void*            _out_value);
void* d_vector_at(const struct d_vector* _vector,
                  d_index                _index);
void* d_vector_front(const struct d_vector* _vector);
void* d_vector_back(const struct d_vector* _vector);
void* d_vector_data(const struct d_vector* _vector);
bool  d_vector_get(const struct d_vector* _vector,
                   d_index                _index,
                   void*                  _out_value);
bool  d_vector_set(struct d_vector* _vector,
                   d_index          _index,
                   const void*      _value);

// III.  insertion and removal
bool d_vector_insert_element(struct d_vector* _vector,
                             d_index          _index,
                             const void*      _value);
bool d_vector_insert_elements(struct d_vector* _vector,
                              d_index          _index,
                              const void*      _source,
                              size_t           _count);
bool d_vector_erase(struct d_vector* _vector,
                    d_index          _index);
bool d_vector_erase_range(struct d_vector* _vector,
                          d_index          _start,
                          d_index          _end);
bool d_vector_append_element(struct d_vector* _vector,
                             const void*      _element);
bool d_vector_append_elements(struct d_vector* _vector,
                              const void*      _source,
                              size_t           _count);
bool d_vector_append_vector(struct d_vector*       _destination,
                            const struct d_vector* _source);
bool d_vector_prepend_element(struct d_vector* _vector,
                              const void*      _element);
bool d_vector_prepend_elements(struct d_vector* _vector,
                               const void*      _source,
                               size_t           _count);
bool d_vector_prepend_vector(struct d_vector*       _destination,
                             const struct d_vector* _source);

// IV.   search
ssize_t d_vector_find(const struct d_vector* _vector,
                      const void*            _value,
                      fn_comparator          _comparator);
ssize_t d_vector_find_last(const struct d_vector* _vector,
                           const void*            _value,
                           fn_comparator          _comparator);
bool    d_vector_contains(const struct d_vector* _vector,
                          const void*            _value,
                          fn_comparator          _comparator);
size_t  d_vector_count_value(const struct d_vector* _vector,
                             const void*            _value,
                             fn_comparator          _comparator);

// V.    capacity and state queries
bool   d_vector_ensure_capacity(struct d_vector* _vector,
                                size_t           _required);
bool   d_vector_is_empty(const struct d_vector* _vector);
bool   d_vector_is_full(const struct d_vector* _vector);
size_t d_vector_size(const struct d_vector* _vector);
size_t d_vector_capacity(const struct d_vector* _vector);
size_t d_vector_element_size(const struct d_vector* _vector);

// VI.   mutation and reordering
bool d_vector_reserve(struct d_vector* _vector,
                      size_t           _new_capacity);
bool d_vector_grow(struct d_vector* _vector);
bool d_vector_maybe_shrink(struct d_vector* _vector);
void d_vector_clear(struct d_vector* _vector);
bool d_vector_resize(struct d_vector* _vector,
                     size_t           _new_count);
bool d_vector_resize_fill(struct d_vector* _vector,
                          size_t           _new_count,
                          const void*      _fill_value);
bool d_vector_swap(struct d_vector* _vector,
                   d_index          _index_a,
                   d_index          _index_b);
bool d_vector_reverse(struct d_vector* _vector);
void d_vector_sort(struct d_vector* _vector,
                   fn_comparator    _comparator);

// VII.  conversion and reporting
bool d_vector_shrink_to_fit(struct d_vector* _vector);
bool d_vector_copy_to(const struct d_vector* _vector,
                      void*                  _destination,
                      size_t                 _dest_capacity);

// VIII. operations
size_t d_vector_available(const struct d_vector* _vector);

// IX.   destruction
void d_vector_free(struct d_vector* _vector);
void d_vector_free_deep(struct d_vector* _vector,
                        fn_free          _free_fn);

#endif  // DJINTERP_C_CONTAINER_VECTOR_VECTOR_H
