/*******************************************************************************
* djinterp [c]                                                     text_lineup.c
*
*   String arrays, and tokens laid out in aligned, wrapped columns.
*
*   d_text_lineup measures once -- each token's length, the slot width, the
* first line's padding and room -- then writes the first line and each line
* after it with the same three steps: the tokens that fit (one, at least),
* each padded to its slot, then the line's end. The 2025.12.17 original wrote
* the first line and the others with two copies of that loop; they are one
* helper here, and the output is the same.
*
*
* path:      /src/djinterp/c/text/text_lineup.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.17
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/text_lineup.h"  // corresponding
                                                          // header
// std
#include <limits.h>  // INT_MAX
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free
#include <string.h>  // memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"    // d_ckd_add, d_ckd_mul
#include "../../../../inc/djinterp/c/string_fn.h"  // d_strdup, d_strcpy_s


//==============================================================================
// 1.  STRING ARRAYS
//==============================================================================

/*
d_string_array_new
  A capacity of 0 allocates no element array until the first append.
*/
struct dj_string_array*
d_string_array_new(
    size_t _capacity
)
{
    struct dj_string_array* const array = malloc(sizeof(*array));

    if (!array)
    {
        return NULL;
    }

    array->count    = 0u;
    array->capacity = _capacity;
    array->elements = NULL;

    if (_capacity > 0u)
    {
        array->elements = (char**)calloc(_capacity, sizeof(char*));

        if (!array->elements)
        {
            free(array);

            return NULL;
        }
    }

    return array;
}

/*
d_string_array_free
  Each string, then the element array, then the array.
*/
void
d_string_array_free(
    struct dj_string_array* _array
)
{
    if (!_array)
    {
        return;
    }

    if (_array->elements)
    {
        for (size_t i = 0u; i < _array->count; ++i)
        {
            free(_array->elements[i]);
        }

        free(_array->elements);
    }

    free(_array);

    return;
}

/*
d_string_array_reserve
  Doubles from 8, or from the current capacity, until _min_capacity fits;
past half of SIZE_MAX it takes _min_capacity itself. The bytes are checked
with d_ckd_mul before realloc, and the new slots are zeroed.
*/
bool
d_string_array_reserve(
    struct dj_string_array* _array,
    size_t                  _min_capacity
)
{
    size_t capacity;
    size_t bytes;
    char** grown;

    if (!_array)
    {
        return false;
    }

    if (_min_capacity <= _array->capacity)
    {
        return true;
    }

    capacity = (_array->capacity != 0u) ? _array->capacity : 8u;

    while (capacity < _min_capacity)
    {
        if (d_ckd_mul(&capacity, capacity, (size_t)2u))
        {
            capacity = _min_capacity;

            break;
        }
    }

    if (d_ckd_mul(&bytes, capacity, sizeof(char*)))
    {
        return false;
    }

    grown = (char**)realloc(_array->elements, bytes);

    if (!grown)
    {
        return false;
    }

    memset(grown + _array->capacity,
           0,
           (capacity - _array->capacity) * sizeof(char*));

    _array->elements = grown;
    _array->capacity = capacity;

    return true;
}

/*
d_string_array_append
  The copy is made after the room, so a failed copy leaves the array as it
was, only larger.
*/
bool
d_string_array_append(
    struct dj_string_array* _array,
    const char*             _string
)
{
    char* copy;

    if ( (!_array) ||
         (!_string) )
    {
        return false;
    }

    if (!d_string_array_reserve(_array, _array->count + 1u))
    {
        return false;
    }

    copy = d_strdup(_string);

    if (!copy)
    {
        return false;
    }

    _array->elements[_array->count] = copy;
    ++_array->count;

    return true;
}

/*
d_internal_digits_u
  The decimal digits in _value.
*/
static size_t
d_internal_digits_u(
    unsigned long _value
)
{
    size_t digits = 1u;

    while (_value >= 10u)
    {
        _value /= 10u;
        ++digits;
    }

    return digits;
}

/*
d_internal_series_entry
  Writes one series entry -- _prefix, the number, _suffix -- into a new
string of _size bytes, the widest any entry needs. The number is formatted
in place, where the padding has room; the original went through a 64-byte
array, which cut a padding past 63 short.
*/
static char*
d_internal_series_entry(
    long        _value,
    const char* _prefix,
    const char* _suffix,
    size_t      _zero_padding,
    size_t      _size
)
{
    char* const  entry  = malloc(_size);
    const size_t before = _prefix ? strlen(_prefix) : 0u;
    const int    width  = (_zero_padding > (size_t)INT_MAX)
                              ? INT_MAX
                              : (int)_zero_padding;
    int          length;

    if (!entry)
    {
        return NULL;
    }

    memcpy(entry, _prefix ? _prefix : "", before);

    length = ( (_zero_padding > 0u) && (_value >= 0) )
                 ? snprintf(entry + before, _size - before, "%0*ld", width,
                            _value)
                 : snprintf(entry + before, _size - before, "%ld", _value);

    if ( (length < 0) ||
         ((size_t)length >= (_size - before)) )
    {
        free(entry);

        return NULL;
    }

    if (_suffix)
    {
        (void)d_strcpy_s(entry + before + (size_t)length,
                         _size - before - (size_t)length,
                         _suffix);
    }

    return entry;
}

/*
d_string_array_generate_series_capacity
  The count is computed exactly, so the loop makes that many entries. The
running value is a long, so the step past the last entry cannot overflow an
int (the original's could, at INT_MAX). Every entry has room for the widest
number either end can give, a sign where either is negative, the padding,
and the prefix and suffix, summed with d_ckd_add.
*/
struct dj_string_array*
d_string_array_generate_series_capacity(
    int         _start,
    int         _end,
    int         _step,
    const char* _prefix,
    const char* _suffix,
    size_t      _zero_padding,
    size_t      _additional_capacity
)
{
    struct dj_string_array* array;
    size_t                  count;
    size_t                  capacity;
    size_t                  size;
    size_t                  digits;
    long                    value;
    const long              span  = (long)_end - (long)_start;
    const unsigned long     a     = (_start < 0) ? (0ul - (unsigned long)_start)
                                                 : (unsigned long)_start;
    const unsigned long     b     = (_end < 0) ? (0ul - (unsigned long)_end)
                                               : (unsigned long)_end;

    if ( (_step == 0) ||
         ( (_step > 0) && (_start > _end) ) ||
         ( (_step < 0) && (_start < _end) ) )
    {
        return NULL;
    }

    count = (size_t)(span / (long)_step) + 1u;

    if (d_ckd_add(&capacity, count, _additional_capacity))
    {
        return NULL;
    }

    digits = d_internal_digits_u((a > b) ? a : b);
    digits = (_zero_padding > digits) ? _zero_padding : digits;
    digits += ( (_start < 0) || (_end < 0) ) ? 1u : 0u;

    if ( d_ckd_add(&size, digits, (size_t)1u) ||
         d_ckd_add(&size, size, _prefix ? strlen(_prefix) : 0u) ||
         d_ckd_add(&size, size, _suffix ? strlen(_suffix) : 0u) )
    {
        return NULL;
    }

    array = d_string_array_new(capacity);

    if (!array)
    {
        return NULL;
    }

    value = (long)_start;

    for (size_t i = 0u; i < count; ++i)
    {
        array->elements[i] = d_internal_series_entry(value, _prefix, _suffix,
                                                     _zero_padding, size);

        if (!array->elements[i])
        {
            d_string_array_free(array);

            return NULL;
        }

        array->count = i + 1u;
        value       += (long)_step;
    }

    return array;
}

/*
d_string_array_generate_series
  The series with no extra room.
*/
struct dj_string_array*
d_string_array_generate_series(
    int         _start,
    int         _end,
    int         _step,
    const char* _prefix,
    const char* _suffix,
    size_t      _zero_padding
)
{
    return d_string_array_generate_series_capacity(_start, _end, _step,
                                                   _prefix, _suffix,
                                                   _zero_padding, 0u);
}


//==============================================================================
// 2.  LINEUP
//==============================================================================

// dj_internal_lineup
//   struct: one lineup's measures, shared by the helpers that write it.
struct dj_internal_lineup
{
    const struct dj_text_lineup_template* tmpl;
    struct dj_text_buffer*                out;
    const size_t*                         lengths;           // per token
    size_t                                count;             // tokens
    size_t                                slot;              // token width
    size_t                                separator_length;
    size_t                                line_suffix_length;
    size_t                                line_width;
    bool                                  same_length;       // no padding
    bool                                  outdent_suffix;
};

/*
d_internal_lineup_put
  Appends a string; NULL appends nothing.
*/
static bool
d_internal_lineup_put(
    struct dj_text_buffer* _out,
    const char*            _string
)
{
    return (_string == NULL) || d_text_buffer_append_string(_out, _string);
}

/*
d_internal_lineup_spaces
  Appends _count spaces; 0 appends nothing (the buffer refuses a count of 0).
*/
static bool
d_internal_lineup_spaces(
    struct dj_text_buffer* _out,
    size_t                 _count
)
{
    return (_count == 0u) || d_text_buffer_append_chars(_out, ' ', _count);
}

/*
d_internal_lineup_token
  Writes one token padded to its slot: right-aligned by default, or left or
centred by flag. With PAD_AFTER_SEPARATOR, a left or centred token's
separator goes before the padding, and *_separator is cleared to say so.
*/
static bool
d_internal_lineup_token(
    const struct dj_internal_lineup* _l,
    size_t                           _index,
    bool*                            _separator
)
{
    const char* const token = _l->tmpl->tokens->elements[_index];
    const uint32_t    flags = _l->tmpl->flags;
    const size_t      pad   = _l->same_length
                                  ? 0u
                                  : (_l->slot - _l->lengths[_index]);
    const bool        early = D_TEXT_LINEUP_PADS_AFTER_SEPARATOR(flags) &&
                              *_separator;
    size_t            before;

    if (pad == 0u)
    {
        return d_internal_lineup_put(_l->out, token);
    }

    if ( (!D_TEXT_LINEUP_ALIGNS_TOKEN_LEFT(flags)) &&
         (!D_TEXT_LINEUP_ALIGNS_TOKEN_CENTER(flags)) )
    {
        return d_internal_lineup_spaces(_l->out, pad) &&
               d_internal_lineup_put(_l->out, token);
    }

    before = D_TEXT_LINEUP_ALIGNS_TOKEN_LEFT(flags) ? 0u : (pad / 2u);

    if ( (!d_internal_lineup_spaces(_l->out, before)) ||
         (!d_internal_lineup_put(_l->out, token)) )
    {
        return false;
    }

    if (early)
    {
        *_separator = false;

        if (!d_internal_lineup_put(_l->out, _l->tmpl->separator))
        {
            return false;
        }
    }

    return d_internal_lineup_spaces(_l->out, pad - before);
}

/*
d_internal_lineup_row
  Writes the tokens from *_index that fit in _width, at least one, each with
the separator that follows it; advances *_index past them.
*/
static bool
d_internal_lineup_row(
    const struct dj_internal_lineup* _l,
    size_t*                          _index,
    size_t                           _width
)
{
    size_t used = 0u;

    while (*_index < _l->count)
    {
        const bool   last   = (*_index == (_l->count - 1u));
        const bool   trails = (!last) || _l->tmpl->separator_after;
        const size_t space  = _l->slot +
                              (trails ? _l->separator_length : 0u);
        bool         separator;

        if ( (used > 0u) &&
             ((used + space) > _width) )
        {
            break;
        }

        separator = (_l->tmpl->separator != NULL) && trails;

        if (!d_internal_lineup_token(_l, *_index, &separator))
        {
            return false;
        }

        if ( separator &&
             (!d_internal_lineup_put(_l->out, _l->tmpl->separator)) )
        {
            return false;
        }

        used += space;
        ++*_index;
    }

    return true;
}

/*
d_internal_lineup_align_suffix
  Pads the line begun at _line_start so a line suffix ends at the line's
width, where there is room.
*/
static bool
d_internal_lineup_align_suffix(
    const struct dj_internal_lineup* _l,
    size_t                           _line_start
)
{
    const size_t length = d_text_buffer_length(_l->out) - _line_start;

    if ((length + _l->line_suffix_length) >= _l->line_width)
    {
        return true;
    }

    return d_internal_lineup_spaces(_l->out,
                                    _l->line_width - length -
                                    _l->line_suffix_length);
}

/*
d_internal_lineup_line_end
  Ends a line. The last takes the suffix, then the line suffix where
LINE_SUFFIX_AFTER_SUFFIX asks; any other takes the line suffix and a
newline. A right-aligned line suffix is padded out on the last line, and on
the others where the suffix is outdented.
*/
static bool
d_internal_lineup_line_end(
    const struct dj_internal_lineup* _l,
    size_t                           _line_start,
    bool                             _last,
    const char*                      _suffix
)
{
    const uint32_t flags = _l->tmpl->flags;
    const bool     has   = (_l->tmpl->line_suffix != NULL) &&
                           (_l->line_suffix_length > 0u);
    const bool     right = D_TEXT_LINEUP_RIGHT_ALIGNS_LINE_SUFFIX(flags);

    if (_last)
    {
        if (!d_internal_lineup_put(_l->out, _suffix))
        {
            return false;
        }

        if ( (!has) ||
             (!D_TEXT_LINEUP_LINE_SUFFIX_AFTER_SUFFIX(flags)) )
        {
            return true;
        }

        return ( (!right) ||
                 d_internal_lineup_align_suffix(_l, _line_start) ) &&
               d_internal_lineup_put(_l->out, _l->tmpl->line_suffix);
    }

    if ( has &&
         ( ( right &&
             _l->outdent_suffix &&
             (!d_internal_lineup_align_suffix(_l, _line_start)) ) ||
           (!d_internal_lineup_put(_l->out, _l->tmpl->line_suffix)) ) )
    {
        return false;
    }

    return d_internal_lineup_put(_l->out, "\n");
}

/*
d_internal_lineup_unwrapped
  A line width of 0: the prefix, the tokens and their separators, and the
suffix, on one line with no padding.
*/
static bool
d_internal_lineup_unwrapped(
    const struct dj_text_lineup_template* _template,
    struct dj_text_buffer*                _out,
    const char*                           _prefix,
    const char*                           _suffix
)
{
    const size_t count = _template->tokens->count;

    if ( (!d_internal_lineup_put(_out, _prefix)) ||
         ( _template->separator_before &&
           (!d_internal_lineup_put(_out, _template->separator)) ) )
    {
        return false;
    }

    for (size_t i = 0u; i < count; ++i)
    {
        if (!d_internal_lineup_put(_out, _template->tokens->elements[i]))
        {
            return false;
        }

        if ( ( (i < (count - 1u)) || _template->separator_after ) &&
             (!d_internal_lineup_put(_out, _template->separator)) )
        {
            return false;
        }
    }

    return d_internal_lineup_put(_out, _suffix);
}

/*
d_internal_lineup_measure
  Each token's length, the slot (the widest, or the minimum width, which
also ends "all the same length"), and the room on a line after the first.
*/
static void
d_internal_lineup_measure(
    struct dj_internal_lineup* _l,
    size_t*                    _lengths,
    size_t                     _line_prefix_length,
    size_t*                    _content_width
)
{
    _l->same_length = true;
    _l->slot        = strlen(_l->tmpl->tokens->elements[0]);
    _lengths[0]     = _l->slot;

    for (size_t i = 1u; i < _l->count; ++i)
    {
        _lengths[i] = strlen(_l->tmpl->tokens->elements[i]);

        if (_lengths[i] != _l->slot)
        {
            _l->same_length = false;
            _l->slot        = (_lengths[i] > _l->slot) ? _lengths[i]
                                                       : _l->slot;
        }
    }

    if (_l->tmpl->min_token_width > _l->slot)
    {
        _l->slot        = _l->tmpl->min_token_width;
        _l->same_length = false;
    }

    *_content_width = _l->line_width;
    *_content_width = (_line_prefix_length < *_content_width)
                          ? (*_content_width - _line_prefix_length) : 0u;
    *_content_width = (_l->line_suffix_length < *_content_width)
                          ? (*_content_width - _l->line_suffix_length) : 0u;

    return;
}

/*
d_internal_lineup_less
  _width less _amount, or 0 where _amount is not less than it.
*/
static size_t
d_internal_lineup_less(
    size_t _width,
    size_t _amount
)
{
    return (_amount < _width) ? (_width - _amount) : 0u;
}

/*
d_internal_lineup_first_line
  Writes the first line's opening -- the line prefix where it is indented,
the prefix, the padding that lines its slots up with the lines below, and a
leading separator -- then its tokens and its end. The room left for tokens
follows the original's arithmetic exactly, including a line prefix as wide
as the line, which it leaves in the room.
*/
static bool
d_internal_lineup_first_line(
    const struct dj_internal_lineup* _l,
    size_t*                          _index,
    const char*                      _prefix,
    const char*                      _suffix,
    bool                             _indent_prefix,
    size_t                           _line_prefix_length
)
{
    const size_t slot_size     = _l->slot + _l->separator_length;
    const size_t prefix_length = _prefix ? strlen(_prefix) : 0u;
    const size_t line_start    = d_text_buffer_length(_l->out);
    size_t       padding       = 0u;
    size_t       room          = _l->line_width;

    if ( (slot_size != 0u) && _indent_prefix )
    {
        padding = (slot_size - (prefix_length % slot_size)) % slot_size;
    }
    else if (slot_size != 0u)
    {
        padding = ( (_line_prefix_length % slot_size) + slot_size -
                    (prefix_length % slot_size) ) % slot_size;
    }

    if ( _indent_prefix &&
         (_line_prefix_length < room) )
    {
        room -= _line_prefix_length;
    }

    room = d_internal_lineup_less(room, prefix_length);
    room = d_internal_lineup_less(room, padding);
    room = d_internal_lineup_less(room, _l->line_suffix_length);

    if ( ( _indent_prefix &&
           (!d_internal_lineup_put(_l->out, _l->tmpl->line_prefix)) ) ||
         (!d_internal_lineup_put(_l->out, _prefix)) ||
         (!d_internal_lineup_spaces(_l->out, padding)) )
    {
        return false;
    }

    if ( _l->tmpl->separator_before &&
         (_l->tmpl->separator != NULL) )
    {
        if (!d_internal_lineup_put(_l->out, _l->tmpl->separator))
        {
            return false;
        }

        room = (room >= _l->separator_length)
                   ? (room - _l->separator_length) : 0u;
    }

    return d_internal_lineup_row(_l, _index, room) &&
           d_internal_lineup_line_end(_l, line_start, *_index >= _l->count,
                                      _suffix);
}

/*
d_text_lineup
  Measures, then writes the first line and each line after it; on any
failure the buffer is cut back to where it began.
*/
bool
d_text_lineup(
    const struct dj_text_lineup_template* _template,
    struct dj_text_buffer*                _text_buffer,
    size_t                                _line_width,
    const char*                           _prefix,
    const char*                           _suffix,
    bool                                  _indent_prefix,
    bool                                  _outdent_suffix
)
{
    struct dj_internal_lineup l;
    size_t*                   lengths;
    size_t                    bytes;
    size_t                    content_width;
    size_t                    index = 0u;
    size_t                    start;
    size_t                    line_prefix_length;
    bool                      ok;

    if ( (!_template) ||
         (!_template->tokens) ||
         (_template->tokens->count == 0u) ||
         (!_text_buffer) )
    {
        return false;
    }

    start = d_text_buffer_length(_text_buffer);

    if (_line_width == 0u)
    {
        ok = d_internal_lineup_unwrapped(_template, _text_buffer, _prefix,
                                         _suffix);

        if (!ok)
        {
            (void)d_text_buffer_consume_back(
                _text_buffer, d_text_buffer_length(_text_buffer) - start);
        }

        return ok;
    }

    if ( ( _prefix &&
           (strlen(_prefix) > _line_width) &&
           (!D_TEXT_LINEUP_ALLOWS_OVERSIZE_PREFIX(_template->flags)) ) ||
         ( _suffix &&
           (strlen(_suffix) > _line_width) &&
           (!D_TEXT_LINEUP_ALLOWS_OVERSIZE_SUFFIX(_template->flags)) ) ||
         d_ckd_mul(&bytes, _template->tokens->count, sizeof(size_t)) )
    {
        return false;
    }

    lengths = (size_t*)malloc(bytes);

    if (!lengths)
    {
        return false;
    }

    line_prefix_length   = _template->line_prefix
                               ? strlen(_template->line_prefix) : 0u;
    l.tmpl               = _template;
    l.out                = _text_buffer;
    l.lengths            = lengths;
    l.count              = _template->tokens->count;
    l.separator_length   = _template->separator
                               ? strlen(_template->separator) : 0u;
    l.line_suffix_length = _template->line_suffix
                               ? strlen(_template->line_suffix) : 0u;
    l.line_width         = _line_width;
    l.outdent_suffix     = _outdent_suffix;

    d_internal_lineup_measure(&l, lengths, line_prefix_length,
                              &content_width);

    ok = d_internal_lineup_first_line(&l, &index, _prefix, _suffix,
                                      _indent_prefix, line_prefix_length);

    while ( ok &&
            (index < l.count) )
    {
        const size_t line_start = d_text_buffer_length(_text_buffer);

        ok = d_internal_lineup_put(_text_buffer, _template->line_prefix) &&
             d_internal_lineup_row(&l, &index, content_width) &&
             d_internal_lineup_line_end(&l, line_start, index >= l.count,
                                        _suffix);
    }

    free(lengths);

    if (!ok)
    {
        (void)d_text_buffer_consume_back(
            _text_buffer, d_text_buffer_length(_text_buffer) - start);
    }

    return ok;
}
