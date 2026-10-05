/*******************************************************************************
* djinterp [c]                                              circular_array_mtx.h
*
*   A thread-safe circular array (ring buffer) that wraps `d_circular_array`
* with mutex-based synchronization. All operations acquire a lock before
* delegating to the underlying circular array, ensuring safe concurrent
* access from multiple threads.
*   This structure is ideal for producer-consumer queues, bounded logging
* buffers, and any multithreaded scenario requiring a fixed-capacity ring
* buffer.
*
*
* path:      /inc/djinterp/c/sync/container/array/circular_array_mtx.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.08
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_SYNC_CONTAINER_ARRAY_CIRCULAR_ARRAY_MTX_H
#define DJINTERP_C_SYNC_CONTAINER_ARRAY_CIRCULAR_ARRAY_MTX_H 1

#include <stdarg.h>
#include <stdlib.h>
#include "../../../djinterp.h"
#include "../../../memory/dmemory.h"
#include "../../dmutex.h"
#include "../../../container/container.h"
#include "../../../container/array/array_common.h"
#include "../../../container/array/circular_array.h"


// d_circular_array_mtx
//   struct: a thread-safe wrapper around `d_circular_array`. All operations
// acquire the internal mutex before delegating to the underlying buffer.
struct d_circular_array_mtx
{
    struct d_circular_array* buffer;
    d_mutex_t                mutex;
};


// =============================================================================
// constructor functions
// =============================================================================
struct d_circular_array_mtx* d_circular_array_mtx_new(size_t _capacity, size_t _element_size);
struct d_circular_array_mtx* d_circular_array_mtx_new_default_capacity(size_t _element_size);
struct d_circular_array_mtx* d_circular_array_mtx_new_from_arr(size_t _capacity, size_t _element_size, const void* _source, size_t _source_count);
struct d_circular_array_mtx* d_circular_array_mtx_new_from_args(size_t _capacity, size_t _element_size, size_t _arg_count, ...);
struct d_circular_array_mtx* d_circular_array_mtx_new_copy(const struct d_circular_array_mtx* _other);
struct d_circular_array_mtx* d_circular_array_mtx_new_copy_resized(const struct d_circular_array_mtx* _other, size_t _new_capacity);
struct d_circular_array_mtx* d_circular_array_mtx_new_fill(size_t _capacity, size_t _element_size, const void* _fill_value);

// =============================================================================
// element access functions
// =============================================================================
bool   d_circular_array_mtx_get(struct d_circular_array_mtx* _mtx, d_index _index, void* _out_value);
bool   d_circular_array_mtx_set(struct d_circular_array_mtx* _mtx, d_index _index, const void* _value);
bool   d_circular_array_mtx_front(struct d_circular_array_mtx* _mtx, void* _out_value);
bool   d_circular_array_mtx_back(struct d_circular_array_mtx* _mtx, void* _out_value);
bool   d_circular_array_mtx_peek(struct d_circular_array_mtx* _mtx, void* _out_value);
bool   d_circular_array_mtx_peek_back(struct d_circular_array_mtx* _mtx, void* _out_value);

// =============================================================================
// modification functions - push/pop operations
// =============================================================================
bool   d_circular_array_mtx_push(struct d_circular_array_mtx* _mtx, const void* _element);
bool   d_circular_array_mtx_push_front(struct d_circular_array_mtx* _mtx, const void* _element);
bool   d_circular_array_mtx_push_back(struct d_circular_array_mtx* _mtx, const void* _element);
bool   d_circular_array_mtx_push_all(struct d_circular_array_mtx* _mtx, const void* _elements, size_t _count);
bool   d_circular_array_mtx_push_all_front(struct d_circular_array_mtx* _mtx, const void* _elements, size_t _count);
bool   d_circular_array_mtx_pop_to(struct d_circular_array_mtx* _mtx, void* _out_value);
bool   d_circular_array_mtx_pop_front_to(struct d_circular_array_mtx* _mtx, void* _out_value);
bool   d_circular_array_mtx_pop_back_to(struct d_circular_array_mtx* _mtx, void* _out_value);

// =============================================================================
// modification functions - overwriting operations
// =============================================================================
bool   d_circular_array_mtx_push_overwrite(struct d_circular_array_mtx* _mtx, const void* _element);
bool   d_circular_array_mtx_push_front_overwrite(struct d_circular_array_mtx* _mtx, const void* _element);
bool   d_circular_array_mtx_push_all_overwrite(struct d_circular_array_mtx* _mtx, const void* _elements, size_t _count);

// =============================================================================
// modification functions - bulk operations
// =============================================================================
bool   d_circular_array_mtx_clear(struct d_circular_array_mtx* _mtx);
bool   d_circular_array_mtx_fill(struct d_circular_array_mtx* _mtx, const void* _fill_value);
bool   d_circular_array_mtx_rotate_left(struct d_circular_array_mtx* _mtx, size_t _amount);
bool   d_circular_array_mtx_rotate_right(struct d_circular_array_mtx* _mtx, size_t _amount);
bool   d_circular_array_mtx_reverse(struct d_circular_array_mtx* _mtx);
bool   d_circular_array_mtx_swap(struct d_circular_array_mtx* _mtx, d_index _index_a, d_index _index_b);

// =============================================================================
// query functions
// =============================================================================
bool   d_circular_array_mtx_is_empty(struct d_circular_array_mtx* _mtx);
bool   d_circular_array_mtx_is_full(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_count(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_capacity(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_available_space(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_element_size(struct d_circular_array_mtx* _mtx);

// =============================================================================
// search functions
// =============================================================================
bool    d_circular_array_mtx_contains(struct d_circular_array_mtx* _mtx, const void* _value, fn_comparator _comparator);
ssize_t d_circular_array_mtx_find(struct d_circular_array_mtx* _mtx, const void* _value, fn_comparator _comparator);
ssize_t d_circular_array_mtx_find_last(struct d_circular_array_mtx* _mtx, const void* _value, fn_comparator _comparator);
size_t  d_circular_array_mtx_count_value(struct d_circular_array_mtx* _mtx, const void* _value, fn_comparator _comparator);

// =============================================================================
// conversion functions
// =============================================================================
void*   d_circular_array_mtx_to_linear_array(struct d_circular_array_mtx* _mtx);
bool    d_circular_array_mtx_copy_to(struct d_circular_array_mtx* _mtx, void* _destination, size_t _dest_capacity);

// =============================================================================
// iteration helpers
// =============================================================================
void   d_circular_array_mtx_foreach(struct d_circular_array_mtx* _mtx, fn_apply _apply_fn);
void   d_circular_array_mtx_foreach_reverse(struct d_circular_array_mtx* _mtx, fn_apply _apply_fn);

// =============================================================================
// utility functions
// =============================================================================
void   d_circular_array_mtx_sort(struct d_circular_array_mtx* _mtx, fn_comparator _comparator);
bool   d_circular_array_mtx_linearize(struct d_circular_array_mtx* _mtx);

// =============================================================================
// locking functions
// =============================================================================
int    d_circular_array_mtx_lock(struct d_circular_array_mtx* _mtx);
int    d_circular_array_mtx_trylock(struct d_circular_array_mtx* _mtx);
int    d_circular_array_mtx_unlock(struct d_circular_array_mtx* _mtx);

// =============================================================================
// memory management
// =============================================================================
void   d_circular_array_mtx_free(struct d_circular_array_mtx* _mtx);
void   d_circular_array_mtx_free_deep(struct d_circular_array_mtx* _mtx, fn_free _free_fn);


#endif  // DJINTERP_CONTAINER_ARRAY_CIRCULAR_MT_
