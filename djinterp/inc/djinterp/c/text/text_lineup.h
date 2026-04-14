/******************************************************************************
* djinterp [text]                                                text_lineup.h
*
*   Text lineup and formatting utilities for generating aligned, wrapped text
* output. Provides tools for formatting token sequences into columns with
* configurable alignment, prefixes, suffixes, and line wrapping.
*
* Contents:
*   - d_text_buffer:  dynamically growing text buffer with append/prepend
*   - d_string_array: dynamic array of strings with series generation
*   - d_text_lineup:  core formatting function for aligned token output
*
*
* path:      \inc\text\text_lineup.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.17
******************************************************************************/

#ifndef DJINTERP_TEXT_LINEUP_
#define DJINTERP_TEXT_LINEUP_ 1

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../djinterp.h"
#include "../../string_fn.h"


// D_DEFAULT_TEXT_BUFFER_SIZE
//   constant: default size for text buffer allocation.
#define D_DEFAULT_TEXT_BUFFER_SIZE 256

// d_text_buffer
//   struct: holds a dynamically growing text buffer with position tracking.
struct d_text_buffer
{
    size_t pos;
    char*  buffer;
    size_t capacity;
};

// text buffer operations
struct d_text_buffer* d_text_buffer_new(size_t _capacity);

bool                  d_text_buffer_ensure_capacity(struct d_text_buffer* _text_buffer, size_t _capacity);
size_t                d_text_buffer_append(struct d_text_buffer* _text_buffer, const char* _text);
size_t                d_text_buffer_append_s(struct d_text_buffer* _text_buffer, const char* _text, size_t _text_length);
size_t                d_text_buffer_prepend(struct d_text_buffer* _text_buffer, const char* _text);
size_t                d_text_buffer_write_over(struct d_text_buffer* _text_buffer, d_index _start, d_index _end);

void                  d_text_buffer_free(struct d_text_buffer* _text_buffer);


// d_string_array
//   struct: holds an array of strings with count + capacity.
struct d_string_array
{
    size_t count;
    size_t capacity;
    char** elements;
};

// string array operations
struct d_string_array* d_string_array_new(size_t _capacity);

bool                   d_string_array_reserve(struct d_string_array* _array, size_t _min_capacity);
bool                   d_string_array_append(struct d_string_array* _array, const char* _str);
struct d_string_array* d_string_array_generate_series(int _start, int _end, int _step, const char* _prefix, const char* _suffix, size_t _zero_padding);
struct d_string_array* d_string_array_generate_series_capacity(int _start, int _end, int _step, const char* _prefix, const char* _suffix, size_t _zero_padding, size_t _additional_capacity);

void                   d_string_array_free(struct d_string_array* _array);


// D_BIT_MASK_UINT32
//   macro: determines if a value has a bit enabled.
#define D_BIT_MASK_UINT32(value, flag) \
    ( (uint32_t)((value) & (flag)) )

// DBehaviorFlag
//   enum: flags controlling text lineup behavior.
enum DBehaviorFlag
{
    D_BEHAVIOR_FLAG_UNKNOWN                    = 0x0000,
    D_BEHAVIOR_FLAG_LINE_PREFIX_ALL_LINES      = 0x0001,
    D_BEHAVIOR_FLAG_LINE_SUFFIX_ALL_LINES      = 0x0002,
    D_BEHAVIOR_FLAG_ALLOW_OVERSIZE_PREFIX      = 0x0004,
    D_BEHAVIOR_FLAG_ALLOW_OVERSIZE_SUFFIX      = 0x0008,
    D_BEHAVIOR_FLAG_ALIGN_SINGLE_ROW           = 0x0010,
    D_BEHAVIOR_FLAG_LINE_SUFFIX_ON_SINGLE_LINE = 0x0020,
    D_BEHAVIOR_FLAG_LINE_SUFFIX_AFTER_SUFFIX   = 0x0040,
    D_BEHAVIOR_FLAG_RIGHT_ALIGN_LINE_SUFFIX    = 0x0080,
    D_BEHAVIOR_FLAG_TOKEN_ALIGN_LEFT           = 0x0100,
    D_BEHAVIOR_FLAG_TOKEN_ALIGN_CENTER         = 0x0200,
    D_BEHAVIOR_FLAG_PAD_AFTER_SEPARATOR        = 0x0400,
    D_BEHAVIOR_FLAG_ALIGN_FIRST_ROW_TO_SLOTS   = 0x0800
};

// flag query macros
#define D_TEXT_TEMPLATE_PREFIX_ALL_LINES(v)        D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_LINE_PREFIX_ALL_LINES)
#define D_TEXT_TEMPLATE_SUFFIX_ALL_LINES(v)        D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_LINE_SUFFIX_ALL_LINES)
#define D_TEXT_TEMPLATE_ALLOWS_OVERSIZE_PREFIX(v)  D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_ALLOW_OVERSIZE_PREFIX)
#define D_TEXT_TEMPLATE_ALLOWS_OVERSIZE_SUFFIX(v)  D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_ALLOW_OVERSIZE_SUFFIX)
#define D_TEXT_LINE_UP_ALIGNS_SINGLE_ROW(v)        D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_ALIGN_SINGLE_ROW)
#define D_TEXT_LINE_UP_SUFFIX_ON_SINGLE_LINE(v)    D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_LINE_SUFFIX_ON_SINGLE_LINE)
#define D_TEXT_LINE_UP_LINE_SUFFIX_AFTER_SUFFIX(v) D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_LINE_SUFFIX_AFTER_SUFFIX)
#define D_TEXT_LINE_UP_RIGHT_ALIGN_LINE_SUFFIX(v)  D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_RIGHT_ALIGN_LINE_SUFFIX)
#define D_TEXT_LINE_UP_TOKEN_ALIGN_LEFT(v)         D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_TOKEN_ALIGN_LEFT)
#define D_TEXT_LINE_UP_TOKEN_ALIGN_CENTER(v)       D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_TOKEN_ALIGN_CENTER)
#define D_TEXT_LINE_UP_PAD_AFTER_SEPARATOR(v)      D_BIT_MASK_UINT32(v, D_BEHAVIOR_FLAG_PAD_AFTER_SEPARATOR)


// d_text_template
//   struct: template configuration for text lineup operations.
struct d_text_template
{
    struct d_string_array* tokens;
    char*                  separator;
    bool                   separator_before;
    bool                   separator_after;
    char*                  line_prefix;
    char*                  line_suffix;
    size_t                 min_token_width;
    uint32_t               flags;
};

// text lineup
bool d_text_lineup(struct d_text_template* _template,
                   struct d_text_buffer*   _text_buffer,
                   size_t                  _line_width,
                   const char*             _prefix,
                   const char*             _suffix,
                   bool                    _indent_prefix,
                   bool                    _outdent_suffix);


#endif  // DJINTERP_TEXT_LINEUP_
