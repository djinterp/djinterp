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
* path:      /inc/djinterp/c/container/array/circular_array_mtx.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.08
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TYPES
      -----
      1.    d_circular_array_mtx

II.   CONSTRUCTION
      ------------
      1.    Capacity and element-size constructors
      2.    Copying constructors
      3.    Fill constructor

III.  ELEMENT ACCESS
      --------------
      1.    Indexed access          (get / set)
      2.    End access              (front / back / peek / peek_back)

IV.   MODIFICATION
      ------------
      1.    Push and pop
      2.    Overwriting pushes      (advance over the oldest element)
      3.    Bulk operations         (clear / fill / rotate / reverse / swap)

V.    QUERIES
      -------
      1.    State                   (is_empty / is_full)
      2.    Sizes                   (count / capacity / available / element)

VI.   SEARCH
      ------
      1.    contains / find / find_last / count_value

VII.  CONVERSION
      ----------
      1.    to_linear_array / copy_to

VIII. ITERATION
      ---------
      1.    foreach / foreach_reverse

IX.   UTILITIES
      ---------
      1.    sort / linearize

X.    LOCKING
      -------
      1.    Manual lock / trylock / unlock

XI.   MEMORY MANAGEMENT
      -----------------
      1.    free / free_deep
*/

#ifndef DJINTERP_C_CONTAINER_ARRAY_CIRCULAR_ARRAY_MTX_H
#define DJINTERP_C_CONTAINER_ARRAY_CIRCULAR_ARRAY_MTX_H 1

// std
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../../sync/dmutex.h"
#include "../container.h"
#include "array_common.h"
#include "circular_array.h"

D_EXTERN_C_BEGIN


// I.    Types

// d_circular_array_mtx
//   type: a thread-safe wrapper around `d_circular_array`. All operations
// acquire the internal mutex before delegating to the underlying buffer.
//   The mutex is held for the whole of each call, so a caller that needs
// two operations to be atomic together must take the lock itself with
// d_circular_array_mtx_lock.
struct d_circular_array_mtx
{
    struct d_circular_array* buffer;
    d_mutex_t                mutex;
};


// II.   Construction
struct d_circular_array_mtx* d_circular_array_mtx_new(
    size_t _capacity,
    size_t _element_size);
struct d_circular_array_mtx* d_circular_array_mtx_new_default_capacity(
    size_t _element_size);
struct d_circular_array_mtx* d_circular_array_mtx_new_from_arr(
    size_t      _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count);
struct d_circular_array_mtx* d_circular_array_mtx_new_from_args(
    size_t _capacity,
    size_t _element_size,
    size_t _arg_count,
    ...);
struct d_circular_array_mtx* d_circular_array_mtx_new_copy(
    const struct d_circular_array_mtx* _other);
struct d_circular_array_mtx* d_circular_array_mtx_new_copy_resized(
    const struct d_circular_array_mtx* _other,
    size_t                             _new_capacity);
struct d_circular_array_mtx* d_circular_array_mtx_new_fill(
    size_t      _capacity,
    size_t      _element_size,
    const void* _fill_value);
// III.  Element access
bool d_circular_array_mtx_get(
    struct d_circular_array_mtx* _mtx,
    d_index                      _index,
    void*                        _out_value);
bool d_circular_array_mtx_set(
    struct d_circular_array_mtx* _mtx,
    d_index                      _index,
    const void*                  _value);
bool d_circular_array_mtx_front(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
bool d_circular_array_mtx_back(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
bool d_circular_array_mtx_peek(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
bool d_circular_array_mtx_peek_back(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
// IV.1  Push and pop
bool d_circular_array_mtx_push(
    struct d_circular_array_mtx* _mtx,
    const void*                  _element);
bool d_circular_array_mtx_push_front(
    struct d_circular_array_mtx* _mtx,
    const void*                  _element);
bool d_circular_array_mtx_push_back(
    struct d_circular_array_mtx* _mtx,
    const void*                  _element);
bool d_circular_array_mtx_push_all(
    struct d_circular_array_mtx* _mtx,
    const void*                  _elements,
    size_t                       _count);
bool d_circular_array_mtx_push_all_front(
    struct d_circular_array_mtx* _mtx,
    const void*                  _elements,
    size_t                       _count);
bool d_circular_array_mtx_pop_to(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
bool d_circular_array_mtx_pop_front_to(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
bool d_circular_array_mtx_pop_back_to(
    struct d_circular_array_mtx* _mtx,
    void*                        _out_value);
// IV.2  Overwriting pushes
bool d_circular_array_mtx_push_overwrite(
    struct d_circular_array_mtx* _mtx,
    const void*                  _element);
bool d_circular_array_mtx_push_front_overwrite(
    struct d_circular_array_mtx* _mtx,
    const void*                  _element);
bool d_circular_array_mtx_push_all_overwrite(
    struct d_circular_array_mtx* _mtx,
    const void*                  _elements,
    size_t                       _count);
// IV.3  Bulk operations
bool d_circular_array_mtx_clear(struct d_circular_array_mtx* _mtx);
bool d_circular_array_mtx_fill(
    struct d_circular_array_mtx* _mtx,
    const void*                  _fill_value);
bool d_circular_array_mtx_rotate_left(
    struct d_circular_array_mtx* _mtx,
    size_t                       _amount);
bool d_circular_array_mtx_rotate_right(
    struct d_circular_array_mtx* _mtx,
    size_t                       _amount);
bool d_circular_array_mtx_reverse(struct d_circular_array_mtx* _mtx);
bool d_circular_array_mtx_swap(
    struct d_circular_array_mtx* _mtx,
    d_index                      _index_a,
    d_index                      _index_b);
// V.    Queries
bool d_circular_array_mtx_is_empty(struct d_circular_array_mtx* _mtx);
bool d_circular_array_mtx_is_full(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_count(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_capacity(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_available_space(struct d_circular_array_mtx* _mtx);
size_t d_circular_array_mtx_element_size(struct d_circular_array_mtx* _mtx);
// VI.   Search
bool d_circular_array_mtx_contains(
    struct d_circular_array_mtx* _mtx,
    const void*                  _value,
    fn_comparator                _comparator);
ssize_t d_circular_array_mtx_find(
    struct d_circular_array_mtx* _mtx,
    const void*                  _value,
    fn_comparator                _comparator);
ssize_t d_circular_array_mtx_find_last(
    struct d_circular_array_mtx* _mtx,
    const void*                  _value,
    fn_comparator                _comparator);
size_t d_circular_array_mtx_count_value(
    struct d_circular_array_mtx* _mtx,
    const void*                  _value,
    fn_comparator                _comparator);
// VII.  Conversion
void* d_circular_array_mtx_to_linear_array(struct d_circular_array_mtx* _mtx);
bool d_circular_array_mtx_copy_to(
    struct d_circular_array_mtx* _mtx,
    void*                        _destination,
    size_t                       _dest_capacity);
// VIII. Iteration
void d_circular_array_mtx_foreach(
    struct d_circular_array_mtx* _mtx,
    fn_apply                     _apply_fn);
void d_circular_array_mtx_foreach_reverse(
    struct d_circular_array_mtx* _mtx,
    fn_apply                     _apply_fn);
// IX.   Utilities
void d_circular_array_mtx_sort(
    struct d_circular_array_mtx* _mtx,
    fn_comparator                _comparator);
bool d_circular_array_mtx_linearize(struct d_circular_array_mtx* _mtx);
// X.    Locking
int d_circular_array_mtx_lock(struct d_circular_array_mtx* _mtx);
int d_circular_array_mtx_trylock(struct d_circular_array_mtx* _mtx);
int d_circular_array_mtx_unlock(struct d_circular_array_mtx* _mtx);
// XI.   Memory management
void d_circular_array_mtx_free(struct d_circular_array_mtx* _mtx);
void d_circular_array_mtx_free_deep(
    struct d_circular_array_mtx* _mtx,
    fn_free                      _free_fn);
D_EXTERN_C_END

#endif  // DJINTERP_C_CONTAINER_ARRAY_CIRCULAR_ARRAY_MTX_H
