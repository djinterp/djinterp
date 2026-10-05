/*******************************************************************************
* djinterp [c]                                                     text_buffer.c
*
* TBA
*
*
* path:      /src/djinterp/c/container/buffer/text_buffer.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/c/container/buffer/text_buffer.h"


// ----------------------------------------------------------------------------
// Creation
// ----------------------------------------------------------------------------

/*
d_text_buffer_new
  Creates a new text buffer with the specified initial capacity.

Parameter(s):
  _initial_capacity: the initial capacity in characters.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new
(
    size_t _initial_capacity
)
{
    struct d_text_buffer* buffer;

    buffer = malloc(sizeof(struct d_text_buffer));

    if (!buffer)
    {
        return NULL;
    }

    buffer->count    = 0;
    buffer->capacity = _initial_capacity;

    d_buffer_common_chunk_list_init(&buffer->chunks);

    if (_initial_capacity > 0)
    {
        buffer->data = calloc(_initial_capacity, sizeof(char));
        if (!buffer->data)
        {
            free(buffer);
            return NULL;
        }
    }
    else
    {
        buffer->data = NULL;
    }

    return buffer;
}

/*
d_text_buffer_new_default_capacity
  Creates a new text buffer using the default initial capacity.

Parameter(s):
  none.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_default_capacity
(
    void
)
{
    return d_text_buffer_new(D_BUFFER_DEFAULT_CAPACITY);
}

/*
d_text_buffer_new_from_string
  Creates a new text buffer initialized from a null-terminated string.

Parameter(s):
  _string: the null-terminated string to use; must not be NULL.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_from_string
(
    const char* _string
)
{
    struct d_text_buffer* buffer;
    size_t                len;

    if (!_string)
    {
        return d_text_buffer_new_default_capacity();
    }

    len    = strlen(_string);
    buffer = d_text_buffer_new(len + 1);
    if (!buffer)
    {
        return NULL;
    }

    d_memcpy(buffer->data, _string, len + 1);  // includes null
    buffer->count = len;
    return buffer;
}

/*
d_text_buffer_new_from_string_n
  Creates a new text buffer initialized from the first N characters of a string.

Parameter(s):
  _string: the null-terminated string to use; must not be NULL.
  _length: the number of characters to use from the input.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_from_string_n
(
    const char* _string,
    size_t      _length
)
{
    if ( (!_string) ||
         (_length == 0) )
    {
        return d_text_buffer_new_default_capacity();
    }

    return d_text_buffer_new_from_buffer(_string, _length);
}

/*
d_text_buffer_new_from_strings
  Creates a new text buffer by concatenating a list of strings.

Parameter(s):
  _count: the number of variadic string arguments that follow. ...: additional
          variadic arguments.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_from_strings
(
    size_t _count,
    ...
)
{
    struct d_text_buffer* buffer;
    va_list               args;
    size_t                total_length;
    const char*           cur;

    char* dst;
    if (_count == 0)
    {
        return d_text_buffer_new_default_capacity();
    }

    // first pass: compute total length
    total_length = 0;
    va_start(args, _count);
    for (size_t i = 0; i < _count; ++i)
    {
        cur = va_arg(args, const char*);
        if (cur)
        {
            total_length += strlen(cur);
        }
    }
    va_end(args);

    buffer = d_text_buffer_new(total_length + 1);
    if (!buffer)
    {
        return NULL;
    }

    // second pass: copy via pointer walk (avoids repeated strlen + strcat)
    dst = buffer->data;
    va_start(args, _count);
    for (size_t i = 0; i < _count; ++i)
    {
        cur = va_arg(args, const char*);
        if (cur)
        {
            size_t len = strlen(cur);
            d_memcpy(dst, cur, len);
            dst += len;
        }
    }
    va_end(args);

    *dst = '\0';
    buffer->count = total_length;
    return buffer;
}

/*
d_text_buffer_new_from_buffer
  Performs an operation on a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _length: the number of characters to use from the input.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_from_buffer
(
    const char* _buffer,
    size_t      _length
)
{
    struct d_text_buffer* text_buffer;

    if ( (!_buffer) ||
         (_length == 0) )
    {
        return d_text_buffer_new_default_capacity();
    }

    text_buffer = d_text_buffer_new(_length + 1);
    if (!text_buffer)
    {
        return NULL;
    }

    d_memcpy(text_buffer->data, _buffer, _length);
    text_buffer->data[_length] = '\0';
    text_buffer->count = _length;
    return text_buffer;
}

/*
d_text_buffer_new_copy
  Creates a new text buffer by copying another text buffer.

Parameter(s):
  _other: the source text buffer to copy from; must not be NULL.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_copy
(
    const struct d_text_buffer* _other
)
{
    struct d_text_buffer* buffer;

    if (!_other)
    {
        return NULL;
    }

    buffer = d_text_buffer_new(_other->capacity);
    if (!buffer)
    {
        return NULL;
    }

    if ( (_other->data) &&
         (_other->count > 0) )
    {
        d_memcpy(buffer->data, _other->data, _other->count);
        buffer->data[_other->count] = '\0';
        buffer->count = _other->count;
    }

    return buffer;
}

/*
d_text_buffer_new_copy_range
  Creates a new text buffer by copying a range from another text buffer.

Parameter(s):
  _other: the source text buffer to copy from; must not be NULL.
  _start: the start index.
  _end:   the end index.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_copy_range
(
    const struct d_text_buffer* _other,
    d_index                     _start,
    d_index                     _end
)
{
    struct d_text_buffer* buffer;
    size_t                start_pos;
    size_t                end_pos;
    size_t                range_length;

    if ( (!_other) ||
         (!_other->data) )
    {
        return NULL;
    }

    start_pos = D_NEG_IDX(_start, _other->count);
    end_pos   = D_NEG_IDX(_end, _other->count);

    if (start_pos >= _other->count || end_pos > _other->count ||
        start_pos >= end_pos)
        return NULL;

    range_length = end_pos - start_pos;
    buffer = d_text_buffer_new(range_length + 1);
    if (!buffer)
    {
        return NULL;
    }

    d_memcpy(buffer->data, _other->data + start_pos, range_length);
    buffer->data[range_length] = '\0';
    buffer->count = range_length;
    return buffer;
}

/*
d_text_buffer_new_fill
  Creates a new text buffer filled with a repeated character.

Parameter(s):
  _length:    the number of characters to use from the input.
  _fill_char: parameter.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_fill
(
    size_t _length,
    char   _fill_char
)
{
    struct d_text_buffer* buffer;

    if (_length == 0)
    {
        return d_text_buffer_new_default_capacity();
    }

    buffer = d_text_buffer_new(_length + 1);
    if (!buffer)
    {
        return NULL;
    }

    d_memset(buffer->data, _fill_char, _length);
    buffer->data[_length] = '\0';
    buffer->count = _length;
    return buffer;
}

/*
d_text_buffer_new_formatted
  Creates a new text buffer initialized from formatted text.

Parameter(s):
  _format: printf-style format string; must not be NULL. ...: additional
           variadic arguments.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_new_formatted
(
    const char* _format,
    ...
)
{
    struct d_text_buffer* buffer;
    va_list               args;
    va_list               args_copy;
    int                   required_size;

    if (!_format)
    {
        return NULL;
    }

    va_start(args, _format);
    va_copy(args_copy, args);
    required_size = vsnprintf(NULL, 0, _format, args_copy);
    va_end(args_copy);

    if (required_size < 0)
    {
        va_end(args);
        return NULL;
    }

    buffer = d_text_buffer_new((size_t)required_size + 1);
    if (!buffer)
    {
        va_end(args);
        return NULL;
    }

    vsnprintf(buffer->data, buffer->capacity, _format, args);
    buffer->count = (size_t)required_size;
    va_end(args);
    return buffer;
}

// ----------------------------------------------------------------------------
// Capacity management
// ----------------------------------------------------------------------------

/*
d_text_buffer_ensure_capacity
  Ensures the buffer has at least the specified capacity.

Parameter(s):
  _buffer:            the text buffer to operate on; must not be NULL.
  _required_capacity: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_ensure_capacity
(
    struct d_text_buffer* _buffer,
    size_t                _required_capacity
)
{
    char*  new_data;
    size_t new_capacity;

    if (!_buffer)
    {
        return D_FAILURE;
    }

    if (_buffer->capacity >= _required_capacity)
    {
        return D_SUCCESS;
    }

    // grow by doubling (fast for text workloads)
    new_capacity = _buffer->capacity;
    if (new_capacity == 0)
    {
        new_capacity = D_BUFFER_DEFAULT_CAPACITY;
    }

    while (new_capacity < _required_capacity)
    {
        new_capacity *= 2;
    }

    new_data = realloc(_buffer->data, new_capacity);
    if (!new_data)
    {
        return D_FAILURE;
    }

    _buffer->data     = new_data;
    _buffer->capacity = new_capacity;
    return D_SUCCESS;
}

/*
d_text_buffer_resize_to_fit
  Resizes the internal storage to fit the current contents.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_resize_to_fit
(
    struct d_text_buffer* _buffer
)
{
    char*  new_data;
    size_t new_capacity;

    if (!_buffer)
    {
        return D_FAILURE;
    }

    if (_buffer->count == 0)
    {
        if (_buffer->data)
        {
            free(_buffer->data);
            _buffer->data     = NULL;
            _buffer->capacity = 0;
        }
        return D_SUCCESS;
    }

    new_capacity = _buffer->count + 1;
    if (new_capacity == _buffer->capacity)
    {
        return D_SUCCESS;
    }

    new_data = realloc(_buffer->data, new_capacity);
    if (!new_data)
    {
        return D_FAILURE;
    }

    _buffer->data     = new_data;
    _buffer->capacity = new_capacity;
    return D_SUCCESS;
}

/*
d_text_buffer_reserve
  Ensures the buffer has at least the requested additional capacity.

Parameter(s):
  _buffer:              the text buffer to operate on; must not be NULL.
  _additional_capacity: the additional capacity to reserve in characters.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_reserve
(
    struct d_text_buffer* _buffer,
    size_t                _additional_capacity
)
{
    if (!_buffer)
    {
        return D_FAILURE;
    }
    else if (_additional_capacity == 0)
    {
        return D_SUCCESS;
    }

    return d_text_buffer_ensure_capacity(_buffer,
                                         _buffer->capacity +
                                         _additional_capacity);
}

// ----------------------------------------------------------------------------
// String operations (resize mode)
// ----------------------------------------------------------------------------

/*
d_text_buffer_append_string
  Appends data to the end of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_string
(
    struct d_text_buffer* _buffer,
    const char*           _string
)
{
    size_t len;

    if ( (!_buffer) ||
         (!_string) )
    {
        return D_FAILURE;
    }

    len = strlen(_string);

    if (len == 0)
    {
        return D_SUCCESS;
    }

    if (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + len + 1))
    {
        return D_FAILURE;
    }

    d_memcpy(_buffer->data + _buffer->count, _string, len);
    _buffer->count += len;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_append_string_n
  Appends data to the end of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
  _length: the number of characters to use from the input.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_string_n
(
    struct d_text_buffer* _buffer,
    const char*           _string,
    size_t                _length
)
{
    if ( (!_buffer)     ||
         (!_string)     ||
         (_length == 0) ||
         (!d_text_buffer_ensure_capacity(_buffer,
                                         _buffer->count + _length + 1)) )
    {
        return D_FAILURE;
    }

    d_memcpy(_buffer->data + _buffer->count, _string, _length);

    _buffer->count += _length;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_append_buffer
  Appends data to the end of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _data:   parameter.
  _length: the number of characters to use from the input.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_buffer
(
    struct d_text_buffer* _buffer,
    const char*           _data,
    size_t                _length
)
{
    if ( (!_buffer)     ||
         (!_data)       ||
         (_length == 0) ||
         (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + _length + 1)) )
    {
        return D_FAILURE;
    }

    d_memcpy(_buffer->data + _buffer->count, _data, _length);

    _buffer->count += _length;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_append_char
  Appends data to the end of a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _character: the character to operate on.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_char
(
    struct d_text_buffer* _buffer,
    char                  _character
)
{
    if ( (!_buffer) ||
         (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + 2)) )
    {
        return D_FAILURE;
    }

    _buffer->data[_buffer->count++] = _character;
    _buffer->data[_buffer->count]   = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_append_chars
  Appends data to the end of a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _character: the character to operate on.
  _count:     the number of variadic string arguments that follow.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_chars
(
    struct d_text_buffer* _buffer,
    char                  _character,
    size_t                _count
)
{
    if ( (!_buffer)    ||
         (_count == 0) ||
         (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + _count + 1)) )
    {
        return D_FAILURE;
    }

    d_memset(_buffer->data + _buffer->count, _character, _count);

    _buffer->count += _count;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_append_formatted
  Appends data to the end of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _format: printf-style format string. ...: additional variadic arguments.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_formatted
(
    struct d_text_buffer* _buffer,
    const char*           _format,
    ...
)
{
    va_list args;
    va_list args_copy;
    int     required_size;

    if ( (!_buffer) ||
         (!_format) )
    {
        return D_FAILURE;
    }

    va_start(args, _format);
    va_copy(args_copy, args);
    required_size = vsnprintf(NULL, 0, _format, args_copy);
    va_end(args_copy);

    if (required_size < 0)
    {
        va_end(args);

        return D_FAILURE;
    }

    if (!d_text_buffer_ensure_capacity(_buffer,
            _buffer->count + (size_t)required_size + 1))
    {
        va_end(args);

        return D_FAILURE;
    }

    vsnprintf(_buffer->data + _buffer->count,
              _buffer->capacity - _buffer->count,
              _format, args);
    _buffer->count += (size_t)required_size;
    va_end(args);

    return D_SUCCESS;
}

/*
d_text_buffer_append_buffer_obj
  Appends data to the end of a text buffer.

Parameter(s):
  _destination: the destination buffer to write into; must not be NULL.
  _source:      the source text buffer to copy from.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_buffer_obj
(
    struct d_text_buffer*       _destination,
    const struct d_text_buffer* _source
)
{
    if ( (!_destination) ||
         (!_source)      ||
         (!_source->data) )
    {
        return D_FAILURE;
    }

    return d_text_buffer_append_buffer(_destination,
                                       _source->data,
                                       _source->count);
}

/*
d_text_buffer_prepend_string
  Prepends data to the beginning of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_prepend_string
(
    struct d_text_buffer* _buffer,
    const char*           _string
)
{
    size_t len;

    if ( (!_buffer) ||
         (!_string) )
    {
        return D_FAILURE;
    }

    len = strlen(_string);
    if (len == 0)
    {
        return D_SUCCESS;
    }

    if (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + len + 1))
    {
        return D_FAILURE;
    }

    if (_buffer->count > 0)
    {
        memmove(_buffer->data + len, _buffer->data, _buffer->count);
    }

    d_memcpy(_buffer->data, _string, len);
    _buffer->count += len;
    _buffer->data[_buffer->count] = '\0';
    return D_SUCCESS;
}

/*
d_text_buffer_prepend_buffer
  Prepends data to the beginning of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _data:   parameter.
  _length: the number of characters to use from the input.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_prepend_buffer
(
    struct d_text_buffer* _buffer,
    const char*           _data,
    size_t                _length
)
{
    if ( (!_buffer)     ||
         (!_data)       ||
         (_length == 0) ||
         (!d_text_buffer_ensure_capacity(_buffer,
                                         _buffer->count + _length + 1)) )
    {
        return D_FAILURE;
    }

    if (_buffer->count > 0)
    {
        memmove(_buffer->data + _length, _buffer->data, _buffer->count);
    }

    d_memcpy(_buffer->data, _data, _length);

    _buffer->count += _length;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_prepend_char
  Prepends data to the beginning of a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _character: the character to operate on.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_prepend_char
(
    struct d_text_buffer* _buffer,
    char                  _character
)
{
    if ( (!_buffer) ||
         (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + 2)) )
    {
        return D_FAILURE;
    }

    if (_buffer->count > 0)
    {
        memmove(_buffer->data + 1, _buffer->data, _buffer->count);
    }

    _buffer->data[0] = _character;
    _buffer->count++;
    _buffer->data[_buffer->count] = '\0';
    return D_SUCCESS;
}

/*
d_text_buffer_insert_string
  Inserts data into a text buffer at a given index.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _index:  the index into the buffer.
  _string: the null-terminated string to use.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_insert_string
(
    struct d_text_buffer* _buffer,
    d_index               _index,
    const char*           _string
)
{
    size_t insert_pos;
    size_t len;

    if ( (!_buffer) ||
         (!_string) )
    {
        return D_FAILURE;
    }

    insert_pos = D_NEG_IDX(_index, _buffer->count);
    if (insert_pos > _buffer->count)
    {
        return D_FAILURE;
    }

    len = strlen(_string);
    if (len == 0)
    {
        return D_SUCCESS;
    }

    if (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + len + 1))
    {
        return D_FAILURE;
    }

    if (insert_pos < _buffer->count)
    {
        memmove(_buffer->data + insert_pos + len,
                _buffer->data + insert_pos,
                _buffer->count - insert_pos);
    }

    d_memcpy(_buffer->data + insert_pos, _string, len);
    _buffer->count += len;
    _buffer->data[_buffer->count] = '\0';
    return D_SUCCESS;
}

/*
d_text_buffer_insert_buffer
  Inserts data into a text buffer at a given index.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _index:  the index into the buffer.
  _data:   parameter.
  _length: the number of characters to use from the input.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_insert_buffer
(
    struct d_text_buffer* _buffer,
    d_index               _index,
    const char*           _data,
    size_t                _length
)
{
    size_t insert_pos;

    if ( (!_buffer) ||
         (!_data) ||
         (_length == 0) )
    {
        return D_FAILURE;
    }

    insert_pos = D_NEG_IDX(_index, _buffer->count);
    if (insert_pos > _buffer->count)
    {
        return D_FAILURE;
    }

    if (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + _length + 1))
    {
        return D_FAILURE;
    }

    if (insert_pos < _buffer->count)
    {
        memmove(_buffer->data + insert_pos + _length,
                _buffer->data + insert_pos,
                _buffer->count - insert_pos);
    }

    d_memcpy(_buffer->data + insert_pos, _data, _length);
    _buffer->count += _length;
    _buffer->data[_buffer->count] = '\0';
    return D_SUCCESS;
}

/*
d_text_buffer_insert_char
  Inserts data into a text buffer at a given index.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _index:     the index into the buffer.
  _character: the character to operate on.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_insert_char
(
    struct d_text_buffer* _buffer,
    d_index               _index,
    char                  _character
)
{
    size_t insert_pos;

    if (!_buffer)
    {
        return D_FAILURE;
    }

    insert_pos = D_NEG_IDX(_index, _buffer->count);
    if (insert_pos > _buffer->count)
    {
        return D_FAILURE;
    }

    if (!d_text_buffer_ensure_capacity(_buffer, _buffer->count + 2))
    {
        return D_FAILURE;
    }

    if (insert_pos < _buffer->count)
    {
        memmove(_buffer->data + insert_pos + 1,
                _buffer->data + insert_pos,
                _buffer->count - insert_pos);
    }

    _buffer->data[insert_pos] = _character;
    _buffer->count++;
    _buffer->data[_buffer->count] = '\0';
    return D_SUCCESS;
}

// ----------------------------------------------------------------------------
// String operations (append mode)
// ----------------------------------------------------------------------------

/*
d_text_buffer_append_string_chunked
  Appends data using chunked storage without immediate consolidation.

Parameter(s):
  _buffer:         the text buffer to operate on; must not be NULL.
  _string:         the null-terminated string to use.
  _chunk_capacity: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_string_chunked
(
    struct d_text_buffer* _buffer,
    const char*           _string,
    size_t                _chunk_capacity
)
{
    size_t len;

    if ( (!_buffer) ||
         (!_string) )
    {
        return D_FAILURE;
    }

    len = strlen(_string);
    if (len == 0)
    {
        return D_SUCCESS;
    }

    return d_buffer_common_append_data_chunked(&_buffer->chunks,
                                               sizeof(char),
                                               _string,
                                               len,
                                               _chunk_capacity);
}

/*
d_text_buffer_append_buffer_chunked
  Appends data using chunked storage without immediate consolidation.

Parameter(s):
  _buffer:         the text buffer to operate on; must not be NULL.
  _data:           parameter.
  _length:         the number of characters to use from the input.
  _chunk_capacity: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_buffer_chunked
(
    struct d_text_buffer* _buffer,
    const char*           _data,
    size_t                _length,
    size_t                _chunk_capacity
)
{
    if ( (!_buffer) ||
         (!_data) ||
         (_length == 0) )
    {
        return D_FAILURE;
    }

    return d_buffer_common_append_data_chunked(&_buffer->chunks,
                                               sizeof(char),
                                               _data,
                                               _length,
                                               _chunk_capacity);
}

/*
d_text_buffer_append_char_chunked
  Appends data using chunked storage without immediate consolidation.

Parameter(s):
  _buffer:         the text buffer to operate on; must not be NULL.
  _character:      the character to operate on.
  _chunk_capacity: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_char_chunked
(
    struct d_text_buffer* _buffer,
    char                  _character,
    size_t                _chunk_capacity
)
{
    return (_buffer)
        ? d_buffer_common_append_element_chunked(&_buffer->chunks,
                                                 sizeof(char),
                                                 &_character,
                                                 _chunk_capacity)
        : D_FAILURE;
}

/*
d_text_buffer_append_formatted_chunked
  Appends data using chunked storage without immediate consolidation.

Parameter(s):
  _buffer:         the text buffer to operate on; must not be NULL.
  _chunk_capacity: parameter.
  _format:         printf-style format string. ...: additional variadic
                   arguments.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_append_formatted_chunked
(
    struct d_text_buffer* _buffer,
    size_t                _chunk_capacity,
    const char*           _format,
    ...
)
{
    va_list args;
    va_list args_copy;
    int     required_size;
    char*   tmp;
    bool    result;

    if ( (!_buffer) ||
         (!_format) )
    {
        return D_FAILURE;
    }

    va_start(args, _format);
    va_copy(args_copy, args);
    required_size = vsnprintf(NULL, 0, _format, args_copy);
    va_end(args_copy);

    if (required_size < 0)
    {
        va_end(args);
        return D_FAILURE;
    }

    tmp = malloc((size_t)required_size + 1);
    if (!tmp)
    {
        va_end(args);

        return D_FAILURE;
    }

    vsnprintf(tmp, (size_t)required_size + 1, _format, args);
    va_end(args);

    result = d_buffer_common_append_data_chunked(&_buffer->chunks,
                                                 sizeof(char),
                                                 tmp,
                                                 (size_t)required_size,
                                                 _chunk_capacity);
    free(tmp);

    return result;
}

/*
d_text_buffer_consolidate
  Consolidates any chunked data into a single contiguous buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_consolidate
(
    struct d_text_buffer* _buffer
)
{
    size_t total;
    char* dst;
    struct d_buffer_chunk* cur;

    if (!_buffer)
    {
        return D_FAILURE;
    }

    if (_buffer->chunks.chunk_count == 0)
    {
        return D_SUCCESS;
    }

    total = _buffer->count + _buffer->chunks.total_count;

    // ensure space for all data + null terminator
    if (!d_text_buffer_ensure_capacity(_buffer, total + 1))
    {
        return D_FAILURE;
    }

    // copy chunk data after the primary content
    dst = _buffer->data + _buffer->count;
    cur = _buffer->chunks.head;

    while (cur)
    {
        if (cur->count > 0)
        {
            d_memcpy(dst, cur->elements, cur->count);
            dst += cur->count;
        }
        cur = cur->next;
    }

    _buffer->count = total;
    _buffer->data[_buffer->count] = '\0';

    d_buffer_common_chunk_list_free(&_buffer->chunks);

    return D_SUCCESS;
}

/*
d_text_buffer_total_length
  Returns the total length including any chunked data.

Parameter(s):
  _buffer: the text buffer to operate on; may be NULL.
Return:
  The number of characters in the text buffer.
*/
size_t
d_text_buffer_total_length
(
    const struct d_text_buffer* _buffer
)
{
    return (_buffer)
        ? (_buffer->count + _buffer->chunks.total_count)
        : 0;
}

/*
d_text_buffer_has_chunks
  Returns whether the buffer currently has chunked overflow data.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_has_chunks
(
    const struct d_text_buffer* _buffer
)
{
    return (_buffer)
        ? _buffer->chunks.chunk_count > 0
        : false;
}

// ----------------------------------------------------------------------------
// Modification operations
// ----------------------------------------------------------------------------

/*
d_text_buffer_set_string
  Replaces the buffer contents with a null-terminated string.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_set_string
(
    struct d_text_buffer* _buffer,
    const char*           _string
)
{
    size_t len;

    if ( (!_buffer) ||
         (!_string) )
    {
        return D_FAILURE;
    }

    len = strlen(_string);

    if (!d_text_buffer_ensure_capacity(_buffer, len + 1))
    {
        return D_FAILURE;
    }

    d_memcpy(_buffer->data, _string, len + 1);
    _buffer->count = len;

    return D_SUCCESS;
}

/*
d_text_buffer_set_buffer
  Replaces the buffer contents with another text buffer's contents.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _data:   parameter.
  _length: the number of characters to use from the input.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_set_buffer
(
    struct d_text_buffer* _buffer,
    const char*           _data,
    size_t                _length
)
{
    if ( (!_buffer)     ||
         (!_data)       ||
         (_length == 0) ||
         (!d_text_buffer_ensure_capacity(_buffer, _length + 1)) )
    {
        return D_FAILURE;
    }

    d_memcpy(_buffer->data, _data, _length);
    _buffer->data[_length] = '\0';
    _buffer->count = _length;

    return D_SUCCESS;
}

/*
d_text_buffer_set_formatted
  Replaces the buffer contents with formatted text.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _format: printf-style format string. ...: additional variadic arguments.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_set_formatted
(
    struct d_text_buffer* _buffer,
    const char*           _format,
    ...
)
{
    va_list args;
    va_list args_copy;
    int     required_size;

    if ( (!_buffer) ||
         (!_format) )
    {
        return D_FAILURE;
    }

    va_start(args, _format);
    va_copy(args_copy, args);

    required_size = vsnprintf(NULL, 0, _format, args_copy);

    va_end(args_copy);

    if (required_size < 0)
    {
        va_end(args);
        return D_FAILURE;
    }

    if (!d_text_buffer_ensure_capacity(_buffer, (size_t)required_size + 1))
    {
        va_end(args);
        return D_FAILURE;
    }

    vsnprintf(_buffer->data, _buffer->capacity, _format, args);
    _buffer->count = (size_t)required_size;
    va_end(args);
    return D_SUCCESS;
}

/*
d_text_buffer_replace_char
  Replaces characters or substrings in a text buffer.

Parameter(s):
  _buffer:   the text buffer to operate on; must not be NULL.
  _old_char: parameter.
  _new_char: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_replace_char
(
    struct d_text_buffer* _buffer,
    char                  _old_char,
    char                  _new_char
)
{
    char*       p;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }

    // tight loop: single branch per byte
    p = _buffer->data;
    const char* end = p + _buffer->count;

    while (p < end)
    {
        if (*p == _old_char)
        {
            *p = _new_char;
        }

        ++p;
    }

    return D_SUCCESS;
}

/*
d_text_buffer_replace_string
  Replaces characters or substrings in a text buffer.

Parameter(s):
  _buffer:     the text buffer to operate on; must not be NULL.
  _old_string: parameter.
  _new_string: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_replace_string
(
    struct d_text_buffer* _buffer,
    const char*           _old_string,
    const char*           _new_string
)
{
    char*  temp;
    char*  rp;
    char*  wp;
    char*  fp;
    size_t old_len;
    size_t new_len;
    size_t occurrences;
    size_t new_size;
    size_t tail;

    if ( (!_buffer)     ||
         (!_old_string) ||
         (!_new_string) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }

    old_len = strlen(_old_string);

    if (old_len == 0)
    {
        return D_SUCCESS;
    }

    new_len = strlen(_new_string);

    // count occurrences
    occurrences = 0;
    rp = _buffer->data;

    while ( (fp = strstr(rp, _old_string)) != NULL)
    {
        ++occurrences;
        rp = fp + old_len;
    }

    if (occurrences == 0)
    {
        return D_SUCCESS;
    }

    new_size = _buffer->count +
               (occurrences * new_len) - (occurrences * old_len);

    temp = malloc(new_size + 1);

    // ensure that memory allocation was successful
    if (!temp)
    {
        return D_FAILURE;
    }

    rp = _buffer->data;
    wp = temp;

    while ((fp = strstr(rp, _old_string)) != NULL)
    {
        size_t span = (size_t)(fp - rp);
        d_memcpy(wp, rp, span);
        wp += span;
        d_memcpy(wp, _new_string, new_len);
        wp += new_len;
        rp = fp + old_len;
    }

    // copy tail (including null)
    tail = (size_t)((_buffer->data + _buffer->count) - rp);
    d_memcpy(wp, rp, tail);
    wp[tail] = '\0';

    if (!d_text_buffer_ensure_capacity(_buffer, new_size + 1))
    {
        free(temp);
        return D_FAILURE;
    }

    d_memcpy(_buffer->data, temp, new_size + 1);

    _buffer->count = new_size;
    free(temp);

    return D_SUCCESS;
}

/*
d_text_buffer_replace_range
  Replaces characters or substrings in a text buffer.

Parameter(s):
  _buffer:      the text buffer to operate on; must not be NULL.
  _start:       the start index.
  _end:         the end index.
  _replacement: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_replace_range
(
    struct d_text_buffer* _buffer,
    d_index               _start,
    d_index               _end,
    const char*           _replacement
)
{
    size_t start_pos;
    size_t end_pos;
    size_t range_length;
    size_t rep_len;
    size_t new_size;

    if ( (!_buffer) ||
         (!_replacement) )
    {
        return D_FAILURE;
    }

    start_pos = D_NEG_IDX(_start, _buffer->count);
    end_pos   = D_NEG_IDX(_end, _buffer->count);

    if ( (start_pos > _buffer->count) ||
         (end_pos > _buffer->count)   ||
         (start_pos > end_pos) )
    {
        return D_FAILURE;
    }

    range_length = end_pos - start_pos;
    rep_len  = strlen(_replacement);
    new_size = _buffer->count - range_length + rep_len;

    if (!d_text_buffer_ensure_capacity(_buffer, new_size + 1))
    {
        return D_FAILURE;
    }

    if (end_pos < _buffer->count)
    {
        memmove(_buffer->data + start_pos + rep_len,
                _buffer->data + end_pos,
                _buffer->count - end_pos);
    }

    d_memcpy(_buffer->data + start_pos, _replacement, rep_len);
    _buffer->count = new_size;
    _buffer->data[_buffer->count] = '\0';
    return D_SUCCESS;
}

/*
d_text_buffer_remove_char
  Removes characters or ranges from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _index:  the index into the buffer.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_remove_char
(
    struct d_text_buffer* _buffer,
    d_index               _index
)
{
    size_t pos;

    if ( (!_buffer) ||
         (!_buffer->data) ||
         (_buffer->count == 0) )
    {
        return D_FAILURE;
    }

    pos = D_NEG_IDX(_index, _buffer->count);

    if (pos >= _buffer->count)
    {
        return D_FAILURE;
    }

    if (pos < _buffer->count - 1)
    {
        memmove(_buffer->data + pos,
                _buffer->data + pos + 1,
                _buffer->count - pos - 1);
    }

    _buffer->count--;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_remove_range
  Removes characters or ranges from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _start:  the start index.
  _end:    the end index.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_remove_range
(
    struct d_text_buffer* _buffer,
    d_index               _start,
    d_index               _end
)
{
    size_t start_pos;
    size_t end_pos;
    size_t range_length;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }

    start_pos = D_NEG_IDX(_start, _buffer->count);
    end_pos   = D_NEG_IDX(_end, _buffer->count);

    if ( (start_pos > _buffer->count) ||
         (end_pos > _buffer->count)   ||
         (start_pos >= end_pos) )
    {
        return D_FAILURE;
    }

    range_length = end_pos - start_pos;

    if (end_pos < _buffer->count)
    {
        memmove(_buffer->data + start_pos,
                _buffer->data + end_pos,
                _buffer->count - end_pos);
    }

    _buffer->count -= range_length;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_consume_front
  Consumes characters from the front or back of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _amount: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_consume_front
(
    struct d_text_buffer* _buffer,
    size_t                _amount
)
{
    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }
    else if (_amount == 0)
    {
        return D_SUCCESS;
    }
    else if (_amount >= _buffer->count)
    {
        _buffer->count   = 0;
        _buffer->data[0] = '\0';

        return D_SUCCESS;
    }

    memmove(_buffer->data,
            _buffer->data + _amount,
            _buffer->count - _amount);
    _buffer->count -= _amount;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_consume_back
  Consumes characters from the front or back of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _amount: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_consume_back
(
    struct d_text_buffer* _buffer,
    size_t                _amount
)
{
    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }
    else if (_amount == 0)
    {
        return D_SUCCESS;
    }
    else if (_amount >= _buffer->count)
    {
        _buffer->count   = 0;
        _buffer->data[0] = '\0';
        return D_SUCCESS;
    }

    _buffer->count -= _amount;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

// ----------------------------------------------------------------------------
// Access operations
// ----------------------------------------------------------------------------

/*
d_text_buffer_get_char
  Gets a character from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _index:  the index into the buffer.
Return:
  The character at the requested index, or '\0' on failure.
*/
char
d_text_buffer_get_char
(
    const struct d_text_buffer* _buffer,
    d_index                     _index
)
{
    size_t pos;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (_buffer->count == 0) )
    {
        return '\0';
    }

    pos = D_NEG_IDX(_index, _buffer->count);

    if (pos >= _buffer->count)
    {
        return '\0';
    }

    return _buffer->data[pos];
}

/*
d_text_buffer_set_char
  Sets a character in a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _index:     the index into the buffer.
  _character: the character to operate on.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_set_char
(
    struct d_text_buffer* _buffer,
    d_index               _index,
    char                  _character
)
{
    size_t pos;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (_buffer->count == 0) )
    {
        return D_FAILURE;
    }

    pos = D_NEG_IDX(_index, _buffer->count);

    if (pos >= _buffer->count)
    {
        return D_FAILURE;
    }

    _buffer->data[pos] = _character;

    return D_SUCCESS;
}

/*
d_text_buffer_get_string
  Returns a pointer to the internal null-terminated string.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A pointer to the internal null-terminated string, or NULL on failure.
*/
char*
d_text_buffer_get_string
(
    const struct d_text_buffer* _buffer
)
{
    return ( (_buffer) &&
             (_buffer->data) )
        ? _buffer->data
        : NULL;
}

/*
d_text_buffer_get_range_string
  Creates a newly allocated string containing a range of characters.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _start:  the start index.
  _end:    the end index.
Return:
  A newly allocated null-terminated string for the requested range, or NULL on
  failure.
*/
char*
d_text_buffer_get_range_string
(
    const struct d_text_buffer* _buffer,
    d_index                     _start,
    d_index                     _end
)
{
    size_t start_pos;
    size_t end_pos;
    size_t range_length;
    char*  result;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return NULL;
    }

    start_pos = D_NEG_IDX(_start, _buffer->count);
    end_pos   = D_NEG_IDX(_end, _buffer->count);

    if ( (start_pos > _buffer->count) ||
         (end_pos > _buffer->count) ||
         (start_pos >= end_pos) )
    {
        return NULL;
    }

    range_length = end_pos - start_pos;
    result = malloc(range_length + 1);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    d_memcpy(result, _buffer->data + start_pos, range_length);
    result[range_length] = '\0';

    return result;
}

// ----------------------------------------------------------------------------
// Search operations
// ----------------------------------------------------------------------------

/*
d_text_buffer_find_char
  Finds a character or substring in a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _character: the character to operate on.
Return:
  The zero-based index of the match, or -1 if not found.
*/
ssize_t
d_text_buffer_find_char
(
    const struct d_text_buffer* _buffer,
    char                        _character
)
{
    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return -1;
    }

    // use memchr for speed
    const char* p = (const char*)memchr(_buffer->data,
                                        (unsigned char)_character,
                                        _buffer->count);

    if (!p)
    {
        return -1;
    }

    return (ssize_t)(p - _buffer->data);
}

/*
d_text_buffer_find_char_from
  Finds a character or substring in a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _character: the character to operate on.
  _start:     the start index.
Return:
  The zero-based index of the match, or -1 if not found.
*/
ssize_t
d_text_buffer_find_char_from
(
    const struct d_text_buffer* _buffer,
    char                        _character,
    d_index                     _start
)
{
    size_t start_pos;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return -1;
    }

    start_pos = D_NEG_IDX(_start, _buffer->count);

    if (start_pos >= _buffer->count)
    {
        return -1;
    }

    const char* p = (const char*)memchr(_buffer->data + start_pos,
                                        (unsigned char)_character,
                                        _buffer->count - start_pos);

    if (!p)
    {
        return -1;
    }

    return (ssize_t)(p - _buffer->data);
}

/*
d_text_buffer_find_string
  Finds a character or substring in a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
Return:
  The zero-based index of the match, or -1 if not found.
*/
ssize_t
d_text_buffer_find_string
(
    const struct d_text_buffer* _buffer,
    const char*                 _string
)
{
    char* fp;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_string) )
    {
        return -1;
    }

    fp = strstr(_buffer->data, _string);

    if (!fp)
    {
        return -1;
    }

    return (ssize_t)(fp - _buffer->data);
}

/*
d_text_buffer_find_string_from
  Finds a character or substring in a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
  _start:  the start index.
Return:
  The zero-based index of the match, or -1 if not found.
*/
ssize_t
d_text_buffer_find_string_from
(
    const struct d_text_buffer* _buffer,
    const char*                 _string,
    d_index                     _start
)
{
    size_t start_pos;
    char*  fp;

    if ( (!_buffer) ||
         (!_buffer->data) ||
         (!_string) )
    {
        return -1;
    }

    start_pos = D_NEG_IDX(_start, _buffer->count);

    if (start_pos >= _buffer->count)
    {
        return -1;
    }

    fp = strstr(_buffer->data + start_pos, _string);

    if (!fp)
    {
        return -1;
    }

    return (ssize_t)(fp - _buffer->data);
}

/*
d_text_buffer_find_last_char
  Finds a character or substring in a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; must not be NULL.
  _character: the character to operate on.
Return:
  The zero-based index of the match, or -1 if not found.
*/
ssize_t
d_text_buffer_find_last_char
(
    const struct d_text_buffer* _buffer,
    char                        _character
)
{
    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (_buffer->count == 0) )
    {
        return -1;
    }

    // scan backwards
    const char* p = _buffer->data + _buffer->count;

    while (p-- > _buffer->data)
    {
        if (*p == _character)
        {
            return (ssize_t)(p - _buffer->data);
        }
    }

    return -1;
}

/*
d_text_buffer_find_last_string
  Finds a character or substring in a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use.
Return:
  The zero-based index of the match, or -1 if not found.
*/
ssize_t
d_text_buffer_find_last_string
(
    const struct d_text_buffer* _buffer,
    const char*                 _string
)
{
    char*  fp;
    char*  last_found;
    size_t slen;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_string) )
    {
        return -1;
    }

    slen = strlen(_string);

    if (slen > _buffer->count)
    {
        return -1;
    }

    last_found = NULL;
    fp = _buffer->data;

    while ((fp = strstr(fp, _string)) != NULL)
    {
        last_found = fp;
        ++fp;
    }

    if (!last_found)
    {
        return -1;
    }

    return (ssize_t)(last_found - _buffer->data);
}

/*
d_text_buffer_contains_char
  Checks whether a text buffer contains a character or substring.

Parameter(s):
  _buffer:    the text buffer to operate on.
  _character: the character to operate on.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_contains_char
(
    const struct d_text_buffer* _buffer,
    char                        _character
)
{
    return d_text_buffer_find_char(_buffer, _character) != -1;
}

/*
d_text_buffer_contains_string
  Checks whether a text buffer contains a character or substring.

Parameter(s):
  _buffer: the text buffer to operate on.
  _string: the null-terminated string to use.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_contains_string
(
    const struct d_text_buffer* _buffer,
    const char*                 _string
)
{
    return d_text_buffer_find_string(_buffer, _string) != -1;
}

/*
d_text_buffer_starts_with
  Checks whether a text buffer starts with a given string.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _prefix: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_starts_with
(
    const struct d_text_buffer* _buffer,
    const char*                 _prefix
)
{
    size_t plen;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_prefix) )
    {
        return false;
    }

    plen = strlen(_prefix);

    if (plen > _buffer->count)
    {
        return false;
    }

    return memcmp(_buffer->data, _prefix, plen) == 0;
}

/*
d_text_buffer_ends_with
  Checks whether a text buffer ends with a given string.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _suffix: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_ends_with
(
    const struct d_text_buffer* _buffer,
    const char*                 _suffix
)
{
    size_t slen;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_suffix) )
    {
        return false;
    }

    slen = strlen(_suffix);

    if (slen > _buffer->count)
    {
        return false;
    }

    return memcmp(_buffer->data + _buffer->count - slen, _suffix, slen) == 0;
}

/*
d_text_buffer_count_char
  Counts occurrences of a character or substring in a text buffer.

Parameter(s):
  _buffer:    the text buffer to operate on; may be NULL.
  _character: the character to operate on.
Return:
  A size value result.
*/
size_t
d_text_buffer_count_char
(
    const struct d_text_buffer* _buffer,
    char                        _character
)
{
    size_t      count;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return 0;
    }

    count = 0;
    const char* p   = _buffer->data;
    const char* end = p + _buffer->count;

    while (p < end)
    {
        p = (const char*)memchr(p,
                                (unsigned char)_character,
                                (size_t)(end - p));
        if (!p)
        {
            break;
        }

        ++count;
        ++p;
    }

    return count;
}

/*
d_text_buffer_count_string
  Counts occurrences of a character or substring in a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; may be NULL.
  _string: the null-terminated string to use.
Return:
  A size value result.
*/
size_t
d_text_buffer_count_string
(
    const struct d_text_buffer* _buffer,
    const char*                 _string
)
{
    size_t slen;
    size_t count;
    char*  p;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_string) )
    {
        return 0;
    }

    slen = strlen(_string);

    if ( (slen == 0) ||
         (slen > _buffer->count) )
    {
        return 0;
    }

    count = 0;
    p = _buffer->data;

    while ((p = strstr(p, _string)) != NULL)
    {
        ++count;
        p += slen;
    }

    return count;
}

// ----------------------------------------------------------------------------
// Comparison operations
// ----------------------------------------------------------------------------

/*
d_text_buffer_compare
  Compares two buffers or a buffer and a string.

Parameter(s):
  _buffer1: parameter; must not be NULL.
  _buffer2: parameter; must not be NULL.
Return:
  A comparison result: negative, zero, or positive.
*/
int
d_text_buffer_compare
(
    const struct d_text_buffer* _buffer1,
    const struct d_text_buffer* _buffer2
)
{
    if ( (!_buffer1) &&
         (!_buffer2) )
    {
        return  0;
    }

    if (!_buffer1)
    {
        return -1;
    }

    if (!_buffer2)
    {
        return  1;
    }

    if ( (!_buffer1->data) &&
         (!_buffer2->data) )
    {
        return 0;
    }

    if (!_buffer1->data)
    {
        return -1;
    }

    if (!_buffer2->data)
    {
        return  1;
    }

    return strcmp(_buffer1->data, _buffer2->data);
}

/*
d_text_buffer_compare_string
  Compares two buffers or a buffer and a string.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _string: the null-terminated string to use; must not be NULL.
Return:
  A comparison result: negative, zero, or positive.
*/
int
d_text_buffer_compare_string
(
    const struct d_text_buffer* _buffer,
    const char*                 _string
)
{
    if ( (!_buffer) &&
         (!_string) )
    {
        return  0;
    }

    if (!_buffer)
    {
        return -1;
    }

    if (!_string)
    {
        return  1;
    }

    if (!_buffer->data)
    {
        return -1;
    }

    return strcmp(_buffer->data, _string);
}

/*
d_text_buffer_compare_n
  Compares two buffers or a buffer and a string.

Parameter(s):
  _buffer1: parameter; must not be NULL.
  _buffer2: parameter; must not be NULL.
  _n:       parameter.
Return:
  A comparison result: negative, zero, or positive.
*/
int
d_text_buffer_compare_n
(
    const struct d_text_buffer* _buffer1,
    const struct d_text_buffer* _buffer2,
    size_t                      _n
)
{
    if ( (!_buffer1) &&
         (!_buffer2) )
    {
        return  0;
    }

    if (!_buffer1)
    {
        return -1;
    }

    if (!_buffer2)
    {
        return  1;
    }

    if ( (!_buffer1->data) &&
         (!_buffer2->data) )
    {
        return 0;
    }

    if (!_buffer1->data)
    {
        return -1;
    }

    if (!_buffer2->data)
    {
        return  1;
    }

    return strncmp(_buffer1->data, _buffer2->data, _n);
}

/*
d_text_buffer_equals
  Checks whether two buffers or a buffer and a string are equal.

Parameter(s):
  _buffer1: parameter.
  _buffer2: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_equals
(
    const struct d_text_buffer* _buffer1,
    const struct d_text_buffer* _buffer2
)
{
    // fast path: length mismatch
    if ( (_buffer1) &&
         (_buffer2) &&
         (_buffer1->count != _buffer2->count) )
    {
        return false;
    }

    return (d_text_buffer_compare(_buffer1, _buffer2) == 0);
}

/*
d_text_buffer_equals_string
  Checks whether two buffers or a buffer and a string are equal.

Parameter(s):
  _buffer: the text buffer to operate on.
  _string: the null-terminated string to use.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_equals_string
(
    const struct d_text_buffer* _buffer,
    const char*                 _string
)
{
    return (d_text_buffer_compare_string(_buffer, _string) == 0);
}

// ----------------------------------------------------------------------------
// Text processing
// ----------------------------------------------------------------------------

/*
d_text_buffer_trim_whitespace
  Trims characters from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_trim_whitespace
(
    struct d_text_buffer* _buffer
)
{
    size_t start;
    size_t end;
    size_t new_length;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }

    if (_buffer->count == 0)
    {
        return D_SUCCESS;
    }

    start = 0;

    while ( (start < _buffer->count) &&
            (isspace((unsigned char)_buffer->data[start])) )
    {
        ++start;
    }

    if (start == _buffer->count)
    {
        _buffer->count   = 0;
        _buffer->data[0] = '\0';

        return D_SUCCESS;
    }

    end = _buffer->count - 1;

    while ( (end > start) &&
            (isspace((unsigned char)_buffer->data[end])) )
    {
        --end;
    }

    new_length = end - start + 1;

    if (start > 0)
    {
        memmove(_buffer->data, _buffer->data + start, new_length);
    }

    _buffer->count = new_length;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_trim_front
  Trims characters from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_trim_front
(
    struct d_text_buffer* _buffer
)
{
    size_t start;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }
    else if (_buffer->count == 0)
    {
        return D_SUCCESS;
    }

    start = 0;

    while ( (start < _buffer->count) &&
            (isspace((unsigned char)_buffer->data[start])) )
    {
        ++start;
    }

    if (start == 0)
    {
        return D_SUCCESS;
    }

    if (start == _buffer->count)
    {
        _buffer->count   = 0;
        _buffer->data[0] = '\0';

        return D_SUCCESS;
    }

    memmove(_buffer->data, _buffer->data + start, _buffer->count - start);
    _buffer->count -= start;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_trim_back
  Trims characters from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_trim_back
(
    struct d_text_buffer* _buffer
)
{
    size_t end;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }
    else if (_buffer->count == 0)
    {
        return D_SUCCESS;
    }

    end = _buffer->count;

    while ( (end > 0) &&
            (isspace((unsigned char)_buffer->data[end - 1])) )
    {
        --end;
    }

    _buffer->count = end;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_trim_chars
  Trims characters from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _chars:  the set of characters to match.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_trim_chars
(
    struct d_text_buffer* _buffer,
    const char*           _chars
)
{
    size_t start;
    size_t end;
    size_t new_length;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_chars) )
    {
        return D_FAILURE;
    }
    else if (_buffer->count == 0)
    {
        return D_SUCCESS;
    }

    start = 0;
    while ( (start < _buffer->count) &&
            (strchr(_chars, _buffer->data[start]) != NULL) )
    {
        ++start;
    }

    if (start == _buffer->count)
    {
        _buffer->count   = 0;
        _buffer->data[0] = '\0';

        return D_SUCCESS;
    }

    end = _buffer->count - 1;

    while ( (end > start) &&
            (strchr(_chars, _buffer->data[end]) != NULL) )
    {
        --end;
    }

    new_length = end - start + 1;

    if (start > 0)
    {
        memmove(_buffer->data, _buffer->data + start, new_length);
    }

    _buffer->count = new_length;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_to_upper
  Converts buffer contents to uppercase.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_to_upper
(
    struct d_text_buffer* _buffer
)
{
    char*       p;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }

    p = _buffer->data;
    const char* end = (p + _buffer->count);

    while (p < end)
    {
        *p = (char)toupper((unsigned char)*p);
        ++p;
    }

    return D_SUCCESS;
}

/*
d_text_buffer_to_lower
  Converts buffer contents to lowercase.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_to_lower
(
    struct d_text_buffer* _buffer
)
{
    char*       p;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }

    p = _buffer->data;
    const char* end = (p + _buffer->count);

    while (p < end)
    {
        *p = (char)tolower((unsigned char)*p);
        ++p;
    }

    return D_SUCCESS;
}

/*
d_text_buffer_reverse
  Reverses buffer contents in-place.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_reverse
(
    struct d_text_buffer* _buffer
)
{
    char* lo;
    char* hi;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return D_FAILURE;
    }
    else if (_buffer->count <= 1)
    {
        return D_SUCCESS;
    }

    lo = _buffer->data;
    hi = _buffer->data + _buffer->count - 1;

    while (lo < hi)
    {
        char t = *lo;

        *lo++ = *hi;
        *hi-- = t;
    }

    return D_SUCCESS;
}

/*
d_text_buffer_pad_left
  Pads a text buffer with a character.

Parameter(s):
  _buffer:   the text buffer to operate on; must not be NULL.
  _width:    parameter.
  _pad_char: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_pad_left
(
    struct d_text_buffer* _buffer,
    size_t                _width,
    char                  _pad_char
)
{
    size_t pad;
    if (!_buffer)
    {
        return D_FAILURE;
    }
    else if (_buffer->count >= _width)
    {
        return D_SUCCESS;
    }

    pad = _width - _buffer->count;

    if (!d_text_buffer_ensure_capacity(_buffer, _width + 1))
    {
        return D_FAILURE;
    }

    memmove(_buffer->data + pad, _buffer->data, _buffer->count);
    d_memset(_buffer->data, _pad_char, pad);
    _buffer->count = _width;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_pad_right
  Pads a text buffer with a character.

Parameter(s):
  _buffer:   the text buffer to operate on; must not be NULL.
  _width:    parameter.
  _pad_char: parameter.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_pad_right
(
    struct d_text_buffer* _buffer,
    size_t                _width,
    char                  _pad_char
)
{
    size_t pad;

    if (!_buffer)
    {
        return D_FAILURE;
    }
    else if (_buffer->count >= _width)
    {
        return D_SUCCESS;
    }

    pad = (_width - _buffer->count);

    if (!d_text_buffer_ensure_capacity(_buffer, _width + 1))
    {
        return D_FAILURE;
    }

    d_memset(_buffer->data + _buffer->count, _pad_char, pad);
    _buffer->count = _width;
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

// ----------------------------------------------------------------------------
// Filter
// ----------------------------------------------------------------------------

/*
d_text_buffer_filter
  Filters characters according to a filter chain.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _chain:  the filter chain each character is tested against.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_filter
(
    const struct d_text_buffer*  _buffer,
    const struct d_filter_chain* _chain
)
{
    void*  out_data  = NULL;
    size_t out_count = 0;

    struct d_text_buffer* result;

    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_chain)        ||
         (!d_buffer_common_filter(_buffer->data,
                                  _buffer->count,
                                  sizeof(char),
                                  _chain,
                                  &out_data,
                                  &out_count)) )
    {
        return NULL;
    }

    // wrap in a text_buffer
    result = d_text_buffer_new(out_count + 1);

    if (!result)
    {
        free(out_data);

        return NULL;
    }

    if ( (out_count > 0) &&
         (out_data) )
    {
        d_memcpy(result->data, out_data, out_count);
        result->count = out_count;
        result->data[result->count] = '\0';
    }

    free(out_data);

    return result;
}

/*
d_text_buffer_filter_in_place
  Filters characters in place according to a filter chain.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _chain:  the filter chain each character is tested against.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_filter_in_place
(
    struct d_text_buffer*        _buffer,
    const struct d_filter_chain* _chain
)
{
    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (!_chain)        ||
         (!d_buffer_common_filter_in_place(_buffer->data,
                                         &_buffer->count,
                                         sizeof(char),
                                         _chain)) )
    {
        return D_FAILURE;
    }

    // re-null-terminate
    _buffer->data[_buffer->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_filter_indices
  Collects the indices of the characters that satisfy a filter chain.

Parameter(s):
  _buffer:      the text buffer to operate on; must not be NULL.
  _chain:       the filter chain each character is tested against.
  _out_indices: receives the allocated array of matching indices.
  _out_count:   receives the number of matching indices.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_filter_indices
(
    const struct d_text_buffer*  _buffer,
    const struct d_filter_chain* _chain,
    d_index**                    _out_indices,
    size_t*                      _out_count
)
{
    return ( (_buffer)       &&
             (_buffer->data) &&
             (_chain) )
        ? d_buffer_common_filter_indices(_buffer->data,
                                         _buffer->count,
                                         sizeof(char),
                                         _chain,
                                         _out_indices,
                                         _out_count)
        : D_FAILURE;
}

/*
d_text_buffer_count_matching
  Counts the characters in a text buffer that satisfy a filter chain.

Parameter(s):
  _buffer: the text buffer to operate on; may be NULL.
  _chain:  the filter chain each character is tested against.
Return:
  A size value result.
*/
size_t
d_text_buffer_count_matching
(
    const struct d_text_buffer*  _buffer,
    const struct d_filter_chain* _chain
)
{
    return ( (_buffer)       &&
             (_buffer->data) &&
             (_chain) )
        ? d_buffer_common_count_matching(_buffer->data,
                                         _buffer->count,
                                         sizeof(char),
                                         _chain)
        : 0;
}

/*
d_text_buffer_filter_chunked
  Filters characters, including those in overflow chunks, according to a
filter chain.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
  _chain:  the filter chain each character is tested against.
Return:
  A newly allocated text buffer, or NULL on failure.
*/
struct d_text_buffer*
d_text_buffer_filter_chunked
(
    const struct d_text_buffer*  _buffer,
    const struct d_filter_chain* _chain
)
{
    void*  out_data  = NULL;
    size_t out_count = 0;

    struct d_text_buffer* result;

    if ( (!_buffer) ||
         (!_chain)  ||
         (!d_buffer_common_filter_chunked(_buffer->data,
                                        _buffer->count,
                                        sizeof(char),
                                        &_buffer->chunks,
                                        _chain,
                                        &out_data,
                                        &out_count)) )
    {
        return NULL;
    }

    result = d_text_buffer_new(out_count + 1);

    if (!result)
    {
        free(out_data);

        return NULL;
    }

    if ( (out_count > 0) &&
         (out_data) )
    {
        d_memcpy(result->data, out_data, out_count);
        result->count = out_count;
        result->data[result->count] = '\0';
    }

    free(out_data);

    return result;
}

// ----------------------------------------------------------------------------
// Utility
// ----------------------------------------------------------------------------

/*
d_text_buffer_clear
  Clears the contents of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on.
Return:
  none.
*/
void
d_text_buffer_clear
(
    struct d_text_buffer* _buffer
)
{
    if ( (_buffer) &&
         (_buffer->data) )
    {
        _buffer->count   = 0;
        _buffer->data[0] = '\0';
    }

    return;
}

/*
d_text_buffer_is_empty
  Returns whether the text buffer is empty.

Parameter(s):
  _buffer: the text buffer to operate on; may be NULL.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_is_empty
(
    const struct d_text_buffer* _buffer
)
{
    if ( (!_buffer)       ||
         (!_buffer->data) ||
         (_buffer->count == 0) )
    {
        return true;
    }

    return false;
}

/*
d_text_buffer_length
  Returns the number of characters in the text buffer.

Parameter(s):
  _buffer: the text buffer to operate on.
Return:
  The number of characters in the text buffer.
*/
size_t
d_text_buffer_length
(
    const struct d_text_buffer* _buffer
)
{
    return _buffer
        ? _buffer->count
        : 0;
}

/*
d_text_buffer_capacity
  Returns the capacity of the text buffer in characters.

Parameter(s):
  _buffer: the text buffer to operate on.
Return:
  The capacity of the text buffer in characters.
*/
size_t
d_text_buffer_capacity
(
    const struct d_text_buffer* _buffer
)
{
    return _buffer
        ? _buffer->capacity
        : 0;
}

/*
d_text_buffer_utilization
  Returns the utilization ratio (count divided by capacity).

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  The utilization ratio (count divided by capacity).
*/
double
d_text_buffer_utilization
(
    const struct d_text_buffer* _buffer
)
{
    return ( (_buffer) &&
             (_buffer->capacity != 0) )
        ? ( (double)_buffer->count / (double)_buffer->capacity)
        : 0.0;
}

/*
d_text_buffer_hash
  Computes a hash of a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; may be NULL.
Return:
  A hash value for the buffer contents.
*/
size_t
d_text_buffer_hash
(
    const struct d_text_buffer* _buffer
)
{
    size_t hash;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return 0;
    }

    // FNV-1a
    hash = 14695981039346656037ULL;
    const unsigned char* p   = (const unsigned char*)_buffer->data;
    const unsigned char* end = (p + _buffer->count);

    while (p < end)
    {
        hash ^= (size_t)*p++;
        hash *= 1099511628211ULL;
    }

    return hash;
}

// ----------------------------------------------------------------------------
// Conversion
// ----------------------------------------------------------------------------

/*
d_text_buffer_to_cstring
  Creates a newly allocated null-terminated string from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A newly allocated null-terminated string, or NULL on failure.
*/
char*
d_text_buffer_to_cstring
(
    const struct d_text_buffer* _buffer
)
{
    char* result;

    if ( (!_buffer) ||
         (!_buffer->data) )
    {
        return NULL;
    }

    result = malloc(_buffer->count + 1);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    d_memcpy(result, _buffer->data, _buffer->count);

    result[_buffer->count] = '\0';

    return result;
}

/*
d_text_buffer_copy_to_buffer
  Copies buffer contents into a destination character buffer.

Parameter(s):
  _source:           the source text buffer to copy from; must not be NULL.
  _destination:      the destination buffer to write into.
  _destination_size: the size of the destination buffer in bytes.
Return:
  A boolean value indicating success.
*/
bool
d_text_buffer_copy_to_buffer
(
    const struct d_text_buffer* _source,
    char*                       _destination,
    size_t                      _destination_size
)
{
    if ( (!_source)               ||
         (!_source->data)         ||
         (!_destination)          ||
         (_destination_size == 0) ||
         (_source->count + 1 > _destination_size) )
    {
        return D_FAILURE;
    }

    d_memcpy(_destination, _source->data, _source->count);
    _destination[_source->count] = '\0';

    return D_SUCCESS;
}

/*
d_text_buffer_copy_to_buffer_n
  Copies buffer contents into a destination character buffer.

Parameter(s):
  _source:           the source text buffer to copy from; may be NULL.
  _destination:      the destination buffer to write into.
  _destination_size: the size of the destination buffer in bytes.
  _max_chars:        the maximum number of characters to copy.
Return:
  The number of characters copied.
*/
size_t
d_text_buffer_copy_to_buffer_n
(
    const struct d_text_buffer* _source,
    char*                       _destination,
    size_t                      _destination_size,
    size_t                      _max_chars
)
{
    size_t copy_count;

    if ( (!_source)       ||
         (!_source->data) ||
         (!_destination)  ||
         (_destination_size == 0) )
    {
        return 0;
    }

    copy_count = _source->count;

    if (copy_count > _max_chars)
    {
        copy_count = _max_chars;
    }
    else if (copy_count >= _destination_size)
    {
        copy_count = _destination_size - 1;
    }

    d_memcpy(_destination, _source->data, copy_count);
    _destination[copy_count] = '\0';

    return copy_count;
}

/*
d_text_buffer_to_d_string
  Creates a newly allocated d_string from a text buffer.

Parameter(s):
  _buffer: the text buffer to operate on; must not be NULL.
Return:
  A newly allocated d_string, or NULL on failure.
*/
struct d_string*
d_text_buffer_to_d_string
(
    const struct d_text_buffer* _buffer
)
{
    return ( (_buffer) &&
             (_buffer->data) )
        ? d_string_new_from_buffer(_buffer->data, _buffer->count)
        : NULL;
}

// ----------------------------------------------------------------------------
// Memory management
// ----------------------------------------------------------------------------

/*
d_text_buffer_free
  Frees a text buffer and all associated memory.

Parameter(s):
  _buffer: the text buffer to operate on; may be NULL.
Return:
  none.
*/
void
d_text_buffer_free
(
    struct d_text_buffer* _buffer
)
{
    if (!_buffer)
    {
        return;
    }

    // free overflow chunks first
    d_buffer_common_chunk_list_free(&_buffer->chunks);

    if (_buffer->data)
    {
        free(_buffer->data);
    }

    free(_buffer);

    return;
}
