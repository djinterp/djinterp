/*******************************************************************************
* djinterp [c]                                                     text_buffer.h
*
* Text buffer implementation optimized for string operations with automatic
* null-termination and capacity management for temporary text processing.
*
*   Supports two write modes:
*
*   RESIZE mode (default):
*     The backing char array is reallocated in place via the growth
*     factor. Best for buffers whose final size is roughly known or that
*     are modified in a read-modify-write pattern.
*
*   APPEND mode:
*     Additional text is stored in linked overflow chunks instead of
*     reallocating. Ideal for streaming or incremental construction where
*     the final size is unknown. Use `consolidate` to flatten all chunks
*     into one contiguous, null-terminated buffer.
*
*   Both modes may be active on the same buffer simultaneously.
*
*
* path:      /inc/djinterp/c/container/buffer/text_buffer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.11.13
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_BUFFER_TEXT_BUFFER_H
#define DJINTERP_C_CONTAINER_BUFFER_TEXT_BUFFER_H 1

// std
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../../dstring.h"
#include "../../functional/filter.h"
#include "../container.h"
#include "./buffer_common.h"
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64: its
                                                 // floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if RE_STD_HAS_INT64


// dj_text_buffer
//   struct: a capacity-aware text buffer optimized for string operations with
// automatic null-termination management. Optionally supports overflow chunks
// via a dj_buffer_chunk_list for append-mode writes.
struct dj_text_buffer
{
    size_t                     count;    // byte length (excl. null)
    size_t                     capacity; // allocated bytes (incl. null)
    char*                      data;     // primary contiguous store
    struct dj_buffer_chunk_list chunks;   // overflow chunks (append mode)
};


// I.    creation
// I.    creation and initialization
// I.    creation and initialization
struct dj_text_buffer* d_text_buffer_new(
    size_t _initial_capacity
);
struct dj_text_buffer* d_text_buffer_new_default_capacity(void);
struct dj_text_buffer* d_text_buffer_new_from_string(
    const char* _string
);
struct dj_text_buffer* d_text_buffer_new_from_string_n(
    const char* _string,
    size_t      _length
);
struct dj_text_buffer* d_text_buffer_new_from_strings(
    size_t _count,
    ...
);
struct dj_text_buffer* d_text_buffer_new_from_buffer(
    const char* _buffer,
    size_t      _length
);
struct dj_text_buffer* d_text_buffer_new_copy(
    const struct dj_text_buffer* _other
);
struct dj_text_buffer* d_text_buffer_new_copy_range(
    const struct dj_text_buffer* _other,
    dj_index                     _start,
    dj_index                     _end
);
struct dj_text_buffer* d_text_buffer_new_fill(
    size_t _length,
    char   _fill_char
);
struct dj_text_buffer* d_text_buffer_new_formatted(
    const char* _format,
    ...
);
ssize_t d_text_buffer_find_char_from(
    const struct dj_text_buffer* _buffer,
    char                         _character,
    dj_index                     _start
);
ssize_t d_text_buffer_find_string_from(
    const struct dj_text_buffer* _buffer,
    const char*                  _string,
    dj_index                     _start
);

// II.   element access
bool d_text_buffer_set_string(
    struct dj_text_buffer* _buffer,
    const char*            _string
);
bool d_text_buffer_set_buffer(
    struct dj_text_buffer* _buffer,
    const char*            _data,
    size_t                 _length
);
bool d_text_buffer_set_formatted(
    struct dj_text_buffer* _buffer,
    const char*            _format,
    ...
);
bool d_text_buffer_consume_front(
    struct dj_text_buffer* _buffer,
    size_t                 _amount
);
bool d_text_buffer_consume_back(
    struct dj_text_buffer* _buffer,
    size_t                 _amount
);
char d_text_buffer_get_char(
    const struct dj_text_buffer* _buffer,
    dj_index                     _index
);
bool d_text_buffer_set_char(
    struct dj_text_buffer* _buffer,
    dj_index               _index,
    char                   _character
);
char* d_text_buffer_get_string(
    const struct dj_text_buffer* _buffer
);
char* d_text_buffer_get_range_string(
    const struct dj_text_buffer* _buffer,
    dj_index                     _start,
    dj_index                     _end
);
bool d_text_buffer_trim_front(
    struct dj_text_buffer* _buffer
);
bool d_text_buffer_trim_back(
    struct dj_text_buffer* _buffer
);

// III.  insertion and removal
bool d_text_buffer_append_string(
    struct dj_text_buffer* _buffer,
    const char*            _string
);
bool d_text_buffer_append_string_n(
    struct dj_text_buffer* _buffer,
    const char*            _string,
    size_t                 _length
);
bool d_text_buffer_append_buffer(
    struct dj_text_buffer* _buffer,
    const char*            _data,
    size_t                 _length
);
bool d_text_buffer_append_char(
    struct dj_text_buffer* _buffer,
    char                   _character
);
bool d_text_buffer_append_chars(
    struct dj_text_buffer* _buffer,
    char                   _character,
    size_t                 _count
);
bool d_text_buffer_append_formatted(
    struct dj_text_buffer* _buffer,
    const char*            _format,
    ...
);
bool d_text_buffer_append_buffer_obj(
    struct dj_text_buffer*       _destination,
    const struct dj_text_buffer* _source
);
bool d_text_buffer_prepend_string(
    struct dj_text_buffer* _buffer,
    const char*            _string
);
bool d_text_buffer_prepend_buffer(
    struct dj_text_buffer* _buffer,
    const char*            _data,
    size_t                 _length
);
bool d_text_buffer_prepend_char(
    struct dj_text_buffer* _buffer,
    char                   _character
);
bool d_text_buffer_insert_string(
    struct dj_text_buffer* _buffer,
    dj_index               _index,
    const char*            _string
);
bool d_text_buffer_insert_buffer(
    struct dj_text_buffer* _buffer,
    dj_index               _index,
    const char*            _data,
    size_t                 _length
);
bool d_text_buffer_insert_char(
    struct dj_text_buffer* _buffer,
    dj_index               _index,
    char                   _character
);
bool d_text_buffer_append_string_chunked(
    struct dj_text_buffer* _buffer,
    const char*            _string,
    size_t                 _chunk_capacity
);
bool d_text_buffer_append_buffer_chunked(
    struct dj_text_buffer* _buffer,
    const char*            _data,
    size_t                 _length,
    size_t                 _chunk_capacity
);
bool d_text_buffer_append_char_chunked(
    struct dj_text_buffer* _buffer,
    char                   _character,
    size_t                 _chunk_capacity
);
bool d_text_buffer_append_formatted_chunked(
    struct dj_text_buffer* _buffer,
    size_t                 _chunk_capacity,
    const char*            _format,
    ...
);
bool d_text_buffer_remove_char(
    struct dj_text_buffer* _buffer,
    dj_index               _index
);
bool d_text_buffer_remove_range(
    struct dj_text_buffer* _buffer,
    dj_index               _start,
    dj_index               _end
);

// IV.   search
ssize_t d_text_buffer_find_char(
    const struct dj_text_buffer* _buffer,
    char                         _character
);
ssize_t d_text_buffer_find_string(
    const struct dj_text_buffer* _buffer,
    const char*                  _string
);
ssize_t d_text_buffer_find_last_char(
    const struct dj_text_buffer* _buffer,
    char                         _character
);
ssize_t d_text_buffer_find_last_string(
    const struct dj_text_buffer* _buffer,
    const char*                  _string
);
bool d_text_buffer_contains_char(
    const struct dj_text_buffer* _buffer,
    char                         _character
);
bool d_text_buffer_contains_string(
    const struct dj_text_buffer* _buffer,
    const char*                  _string
);

// V.    capacity and state queries
bool d_text_buffer_ensure_capacity(
    struct dj_text_buffer* _buffer,
    size_t                 _required_capacity
);
size_t d_text_buffer_total_length(
    const struct dj_text_buffer* _buffer
);
bool d_text_buffer_has_chunks(
    const struct dj_text_buffer* _buffer
);
size_t d_text_buffer_count_char(
    const struct dj_text_buffer* _buffer,
    char                         _character
);
size_t d_text_buffer_count_string(
    const struct dj_text_buffer* _buffer,
    const char*                  _string
);
size_t d_text_buffer_count_matching(
    const struct dj_text_buffer*  _buffer,
    const struct dj_filter_chain* _chain
);
bool d_text_buffer_is_empty(
    const struct dj_text_buffer* _buffer
);
size_t d_text_buffer_length(
    const struct dj_text_buffer* _buffer
);
size_t d_text_buffer_capacity(
    const struct dj_text_buffer* _buffer
);

// VI.   mutation and reordering
bool d_text_buffer_reserve(
    struct dj_text_buffer* _buffer,
    size_t                 _additional_capacity
);
bool d_text_buffer_reverse(
    struct dj_text_buffer* _buffer
);
void d_text_buffer_clear(
    struct dj_text_buffer* _buffer
);

// VII.  conversion and reporting
bool d_text_buffer_resize_to_fit(
    struct dj_text_buffer* _buffer
);
bool d_text_buffer_to_upper(
    struct dj_text_buffer* _buffer
);
bool d_text_buffer_to_lower(
    struct dj_text_buffer* _buffer
);
char* d_text_buffer_to_cstring(
    const struct dj_text_buffer* _buffer
);
bool d_text_buffer_copy_to_buffer(
    const struct dj_text_buffer* _source,
    char*                        _destination,
    size_t                       _destination_size
);
size_t d_text_buffer_copy_to_buffer_n(
    const struct dj_text_buffer* _source,
    char*                        _destination,
    size_t                       _destination_size,
    size_t                       _max_chars
);
struct dj_string* d_text_buffer_to_d_string(
    const struct dj_text_buffer* _buffer
);

// VIII. operations
bool d_text_buffer_consolidate(
    struct dj_text_buffer* _buffer
);
bool d_text_buffer_replace_char(
    struct dj_text_buffer* _buffer,
    char                   _old_char,
    char                   _new_char
);
bool d_text_buffer_replace_string(
    struct dj_text_buffer* _buffer,
    const char*            _old_string,
    const char*            _new_string
);
bool d_text_buffer_replace_range(
    struct dj_text_buffer* _buffer,
    dj_index               _start,
    dj_index               _end,
    const char*            _replacement
);
bool d_text_buffer_starts_with(
    const struct dj_text_buffer* _buffer,
    const char*                  _prefix
);
bool d_text_buffer_ends_with(
    const struct dj_text_buffer* _buffer,
    const char*                  _suffix
);
int d_text_buffer_compare(
    const struct dj_text_buffer* _buffer1,
    const struct dj_text_buffer* _buffer2
);
int d_text_buffer_compare_string(
    const struct dj_text_buffer* _buffer,
    const char*                  _string
);
int d_text_buffer_compare_n(
    const struct dj_text_buffer* _buffer1,
    const struct dj_text_buffer* _buffer2,
    size_t                       _n
);
bool d_text_buffer_equals(
    const struct dj_text_buffer* _buffer1,
    const struct dj_text_buffer* _buffer2
);
bool d_text_buffer_equals_string(
    const struct dj_text_buffer* _buffer,
    const char*                  _string
);
bool d_text_buffer_trim_whitespace(
    struct dj_text_buffer* _buffer
);
bool d_text_buffer_trim_chars(
    struct dj_text_buffer* _buffer,
    const char*            _chars
);
bool d_text_buffer_pad_left(
    struct dj_text_buffer* _buffer,
    size_t                 _width,
    char                   _pad_char
);
bool d_text_buffer_pad_right(
    struct dj_text_buffer* _buffer,
    size_t                 _width,
    char                   _pad_char
);
struct dj_text_buffer* d_text_buffer_filter(
    const struct dj_text_buffer*  _buffer,
    const struct dj_filter_chain* _chain
);
bool d_text_buffer_filter_in_place(
    struct dj_text_buffer*        _buffer,
    const struct dj_filter_chain* _chain
);
bool d_text_buffer_filter_indices(
    const struct dj_text_buffer*  _buffer,
    const struct dj_filter_chain* _chain,
    dj_index**                    _out_indices,
    size_t*                       _out_count
);
struct dj_text_buffer* d_text_buffer_filter_chunked(
    const struct dj_text_buffer*  _buffer,
    const struct dj_filter_chain* _chain
);
double d_text_buffer_utilization(
    const struct dj_text_buffer* _buffer
);
size_t d_text_buffer_hash(
    const struct dj_text_buffer* _buffer
);

// IX.   destruction
void d_text_buffer_free(
    struct dj_text_buffer* _buffer
);

#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_CONTAINER_BUFFER_TEXT_BUFFER_H
