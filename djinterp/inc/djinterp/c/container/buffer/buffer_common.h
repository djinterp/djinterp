/*******************************************************************************
* djinterp [c]                                                   buffer_common.h
*
* Common utilities and helper functions for buffer implementations.
*
*   Supports two write modes that can be combined per-buffer:
*
*   RESIZE mode:
*     The buffer grows by reallocating its backing store in place. This is
*     the traditional contiguous-array model.
*
*   APPEND mode:
*     Additional data is attached as a linked list of chunks rather than
*     reallocating. This avoids large copies at the cost of non-contiguous
*     storage. Chunk capacity is determined per-call: the default is
*     exactly the size needed; callers may request a larger capacity
*     (>= size needed). Use `consolidate` to flatten all chunks into a
*     single contiguous allocation.
*
*
* path:      /inc/djinterp/c/container/buffer/buffer_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.29
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_BUFFER_BUFFER_COMMON_H
#define DJINTERP_C_CONTAINER_BUFFER_BUFFER_COMMON_H 1

// std
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../../functional/filter.h"
#include "../container.h"
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// D_BUFFER_DEFAULT_CAPACITY
//   constant: the default capacity, in number of elements, that a new buffer
// has by default.
#ifndef D_BUFFER_DEFAULT_CAPACITY
    #define D_BUFFER_DEFAULT_CAPACITY 256
#endif  // D_BUFFER_DEFAULT_CAPACITY

// D_BUFFER_GROWTH_FACTOR
//   constant: the factor by which buffer capacity is multiplied when automatic
// growth occurs (resize mode).
#ifndef D_BUFFER_GROWTH_FACTOR
    #define D_BUFFER_GROWTH_FACTOR 1.5
#endif  // D_BUFFER_GROWTH_FACTOR


// DBufferWriteMode
//   enum: selects the write strategy when a buffer needs more space.
// D_BUFFER_WRITE_RESIZE - grow the contiguous allocation (realloc).
// D_BUFFER_WRITE_APPEND - attach a new chunk (linked list).
// D_BUFFER_WRITE_DEFAULT - alias for resize mode.
enum DBufferWriteMode
{
    D_BUFFER_WRITE_RESIZE  = 0,
    D_BUFFER_WRITE_APPEND  = 1,
    D_BUFFER_WRITE_DEFAULT = 0
};

// d_buffer_chunk
//   struct: a single overflow chunk used by append-mode buffers. Chunks form a
// singly-linked list appended after the primary allocation is full.
struct d_buffer_chunk
{
    void*                  elements;    // chunk payload
    size_t                 count;       // occupied element count
    size_t                 capacity;    // total element slots
    struct d_buffer_chunk* next;        // next chunk, or NULL
};

// d_buffer_chunk_list
//   struct: head of the overflow chunk chain. Kept as a separate descriptor so
// that buffers which never enter append mode pay no per-instance cost.
struct d_buffer_chunk_list
{
    struct d_buffer_chunk* head;        // first overflow chunk
    struct d_buffer_chunk* tail;        // last overflow chunk (fast append)
    size_t                 chunk_count; // number of overflow chunks
    size_t                 total_count; // total elements across all chunks
};


// I.    initialization
// I.    creation and initialization
// I.    creation and initialization
bool                   d_buffer_common_init_sized(void**  _destination,
                                                  size_t* _count,
                                                  size_t* _capacity,
                                                  size_t  _element_size,
                                                  size_t  _initial_capacity);
bool d_buffer_common_init_from_data(void**      _destination,
                                    size_t*     _count,
                                    size_t*     _capacity,
                                    size_t      _element_size,
                                    const void* _source,
                                    size_t      _source_count,
                                    size_t      _extra_capacity);
bool                   d_buffer_common_init_from_args(void**  _destination,
                                                      size_t* _count,
                                                      size_t* _capacity,
                                                      size_t  _element_size,
                                                      size_t  _arg_count,
                                                      va_list _args);
bool                   d_buffer_common_init_copy(void**      _destination,
                                                 size_t*     _count,
                                                 size_t*     _capacity,
                                                 size_t      _element_size,
                                                 const void* _source,
                                                 size_t      _source_count);
bool                   d_buffer_common_init_fill(void**      _destination,
                                                 size_t*     _count,
                                                 size_t*     _capacity,
                                                 size_t      _element_size,
                                                 size_t      _size,
                                                 const void* _value);
void*                  d_buffer_common_alloc(size_t _element_size,
                                             size_t _capacity);
struct d_buffer_chunk* d_buffer_common_chunk_new(size_t _element_size,
                                                 size_t _capacity);
void                   d_buffer_common_chunk_list_init(
    struct d_buffer_chunk_list* _list);

// II.   element access
void* d_buffer_common_get_element(const void* _elements,
                                  size_t      _count,
                                  size_t      _element_size,
                                  d_index     _index);
bool  d_buffer_common_set_element(void*       _elements,
                                  size_t      _count,
                                  size_t      _element_size,
                                  d_index     _index,
                                  const void* _value);
void* d_buffer_common_peek_front(const void* _elements,
                                 size_t      _count,
                                 size_t      _element_size);
void* d_buffer_common_peek_back(const void* _elements,
                                size_t      _count,
                                size_t      _element_size);
bool  d_buffer_common_append_data(void*       _elements,
                                  size_t*     _count,
                                  size_t      _capacity,
                                  size_t      _element_size,
                                  const void* _data,
                                  size_t      _data_count);
bool  d_buffer_common_prepend_data(void*       _elements,
                                   size_t*     _count,
                                   size_t      _capacity,
                                   size_t      _element_size,
                                   const void* _data,
                                   size_t      _data_count);
bool  d_buffer_common_insert_data(void*       _elements,
                                  size_t*     _count,
                                  size_t      _capacity,
                                  size_t      _element_size,
                                  const void* _data,
                                  size_t      _data_count,
                                  d_index     _index);
bool  d_buffer_common_append_data_chunked(
    struct d_buffer_chunk_list* _list,
    size_t                      _element_size,
    const void*                 _data,
    size_t                      _data_count,
    size_t                      _chunk_capacity);
void* d_buffer_common_get_element_chunked(
    const void*                       _primary_elements,
    size_t                            _primary_count,
    size_t                            _element_size,
    const struct d_buffer_chunk_list* _list,
    d_index                           _index);
bool  d_buffer_common_consume_front(void*   _elements,
                                    size_t* _count,
                                    size_t  _element_size,
                                    size_t  _amount);
bool  d_buffer_common_consume_back(void*   _elements,
                                   size_t* _count,
                                   size_t  _element_size,
                                   size_t  _amount);

// III.  insertion and removal
bool d_buffer_common_append_element(void*       _elements,
                                    size_t*     _count,
                                    size_t      _capacity,
                                    size_t      _element_size,
                                    const void* _value);
bool d_buffer_common_prepend_element(void*       _elements,
                                     size_t*     _count,
                                     size_t      _capacity,
                                     size_t      _element_size,
                                     const void* _value);
bool d_buffer_common_insert_element(void*       _elements,
                                    size_t*     _count,
                                    size_t      _capacity,
                                    size_t      _element_size,
                                    const void* _value,
                                    d_index     _index);
bool d_buffer_common_append_element_chunked(
    struct d_buffer_chunk_list* _list,
    size_t                      _element_size,
    const void*                 _value,
    size_t                      _chunk_capacity);
bool d_buffer_common_remove_element(void*   _elements,
                                    size_t* _count,
                                    size_t  _element_size,
                                    d_index _index);
bool d_buffer_common_remove_range(void*   _elements,
                                  size_t* _count,
                                  size_t  _element_size,
                                  d_index _start,
                                  d_index _end);

// IV.   search
bool    d_buffer_common_contains(const void*   _elements,
                                 size_t        _count,
                                 size_t        _element_size,
                                 const void*   _value,
                                 fn_comparator _comparator);
ssize_t d_buffer_common_find(const void*   _elements,
                             size_t        _count,
                             size_t        _element_size,
                             const void*   _value,
                             fn_comparator _comparator);
ssize_t d_buffer_common_find_last(const void*   _elements,
                                  size_t        _count,
                                  size_t        _element_size,
                                  const void*   _value,
                                  fn_comparator _comparator);

// V.    capacity and state queries
bool   d_buffer_common_ensure_capacity(void**  _elements,
                                       size_t* _capacity,
                                       size_t  _element_size,
                                       size_t  _required_capacity);
size_t d_buffer_common_total_count(
    size_t                            _primary_count,
    const struct d_buffer_chunk_list* _list);
bool   d_buffer_common_is_empty(size_t _count);
bool   d_buffer_common_is_full(size_t _count,
                               size_t _capacity);
size_t d_buffer_common_count_matching(
    const void*                  _elements,
    size_t                       _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain);
bool   d_buffer_common_validate_capacity(size_t _capacity);

// VI.   mutation and reordering
void d_buffer_common_clear(size_t* _count);
bool d_buffer_common_reverse(void*  _elements,
                             size_t _count,
                             size_t _element_size);
void d_buffer_common_sort(void*         _elements,
                          size_t        _count,
                          size_t        _element_size,
                          fn_comparator _comparator);

// VII.  conversion and reporting
bool d_buffer_common_resize_to_fit(void**  _elements,
                                   size_t* _capacity,
                                   size_t  _element_size,
                                   size_t  _count);
bool d_buffer_common_copy_to(const void* _source,
                             size_t      _source_count,
                             size_t      _element_size,
                             void*       _destination,
                             size_t      _destination_capacity,
                             size_t*     _copied_count);
bool d_buffer_common_copy_range_to(const void* _source,
                                   size_t      _source_count,
                                   size_t      _element_size,
                                   d_index     _start,
                                   d_index     _end,
                                   void*       _destination,
                                   size_t      _destination_capacity,
                                   size_t*     _copied_count);

// VIII. operations
size_t d_buffer_common_calc_growth(size_t _current_capacity,
                                   size_t _required_capacity);
bool   d_buffer_common_consolidate(
    void**                      _elements,
    size_t*                     _count,
    size_t*                     _capacity,
    size_t                      _element_size,
    struct d_buffer_chunk_list* _list);
double d_buffer_common_utilization(size_t _count,
                                   size_t _capacity);
bool   d_buffer_common_filter(const void*                  _elements,
                              size_t                       _count,
                              size_t                       _element_size,
                              const struct d_filter_chain* _chain,
                              void**                       _out_elements,
                              size_t*                      _out_count);
bool   d_buffer_common_filter_in_place(
    void*                        _elements,
    size_t*                      _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain);
bool   d_buffer_common_filter_indices(
    const void*                  _elements,
    size_t                       _count,
    size_t                       _element_size,
    const struct d_filter_chain* _chain,
    d_index**                    _out_indices,
    size_t*                      _out_count);
bool   d_buffer_common_filter_chunked(
    const void*                       _primary_elements,
    size_t                            _primary_count,
    size_t                            _element_size,
    const struct d_buffer_chunk_list* _list,
    const struct d_filter_chain*      _chain,
    void**                            _out_elements,
    size_t*                           _out_count);
bool   d_buffer_common_validate_params(size_t _element_size);
bool   d_buffer_common_validate_index(d_index _index,
                                      size_t  _count);

// IX.   destruction
void d_buffer_common_chunk_free(
    struct d_buffer_chunk* _chunk);
void d_buffer_common_chunk_list_free(
    struct d_buffer_chunk_list* _list);
void d_buffer_common_free_data(void* _elements);
void d_buffer_common_free_data_deep(size_t  _count,
                                    void**  _elements,
                                    fn_free _free_fn);

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_CONTAINER_BUFFER_BUFFER_COMMON_H
