/******************************************************************************
* djinterp [core]                                                    dstring.c
*
*   Implementation of the d_string safe string type and associated functions.
*
* path:      \src\dstring.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include "../../inc/c/dstring.h"


// internal helper functions

/*
d_string_internal_grow
  Ensures the d_string has at least the required capacity, growing if needed.
*/
D_STATIC bool
d_string_internal_grow
(
    struct d_string* _string,
    size_t           _required
)
{
    size_t new_capacity;
    char*  new_text;

    if (!_string)
    {
        return false;
    }

    // check if we already have enough capacity
    if (_string->capacity >= _required)
    {
        return true;
    }

    // calculate new capacity using growth factor
    new_capacity = _string->capacity;

    if (new_capacity == 0)
    {
        new_capacity = 16;
    }

    while (new_capacity < _required)
    {
        new_capacity *= 2;
    }

    // allocate new buffer
    new_text = malloc(new_capacity);

    // ensure that memory allocation was successful
    if (!new_text)
    {
        return false;
    }

    // copy existing content if present
    if ( (_string->text) && 
         (_string->size > 0) )
    {
        d_memcpy(new_text, _string->text, _string->size + 1);
    }
    else
    {
        new_text[0] = '\0';
    }

    // free old buffer and update
    free(_string->text);
    _string->text     = new_text;
    _string->capacity = new_capacity;

    return true;
}

// creation and destruction functions
/*
d_string_new
  Creates an empty `d_string` with default capacity.

Parameter(s):
  (none)
Return:
  A pointer value corresponding to either:
  - newly allocated `d_string`, if successful, or
  - NULL, if memory allocation failed.
*/
D_INLINE struct d_string*
d_string_new
(
    void
)
{
    return d_string_new_with_capacity(16);
}

/*
d_string_new_with_capacity
  Creates an empty d_string with specified initial capacity.

Parameter(s):
  _capacity: initial capacity in bytes (including space for null terminator).
Return:
  A pointer value corresponding to either:
  - newly allocated d_string, if successful, or
  - NULL, if memory allocation failed.
*/
struct d_string*
d_string_new_with_capacity
(
    size_t _capacity
)
{
    struct d_string* new_string;

    // ensure minimum capacity of 1 for null terminator
    if (_capacity == 0)
    {
        _capacity = 1;
    }

    new_string = malloc(sizeof(struct d_string));

    // ensure that memory allocation was successful
    if (!new_string)
    {
        return NULL;
    }

    new_string->text = malloc(_capacity);
  
    // ensure that memory allocation was successful
    if (!new_string->text)
    {
        free(new_string);

        return NULL;
    }

    new_string->text[0]  = '\0';
    new_string->size     = 0;
    new_string->capacity = _capacity;

    return new_string;
}

/*
d_string_new_from_cstr
  Creates a d_string from a null-terminated C string.

Parameter(s):
  _cstr: null-terminated source string to copy.
Return:
  A pointer value corresponding to either:
  - newly allocated d_string containing copy of _cstr, if successful, or
  - NULL, if _cstr was NULL or memory allocation failed.
*/
struct d_string*
d_string_new_from_cstr
(
    const char* _cstr
)
{
    size_t           len;
    size_t           capacity;
    struct d_string* new_string;

    if (!_cstr)
    {
        return NULL;
    }

    len      = strlen(_cstr);
    capacity = len + 1;

    new_string = d_string_new_with_capacity(capacity);

    // ensure that memory allocation was successful
    if (!new_string)
    {
        return NULL;
    }

    d_memcpy(new_string->text, _cstr, len + 1);
    new_string->size = len;

    return new_string;
}

/*
d_string_new_from_cstr_n
  Creates a d_string from at most n characters of a C string.

Parameter(s):
  _cstr:   source string to copy from.
  _length: maximum number of characters to copy.
Return:
  A pointer value corresponding to either:
  - newly allocated d_string, if successful, or
  - NULL, if _cstr was NULL or memory allocation failed.
*/
struct d_string*
d_string_new_from_cstr_n
(
    const char* _cstr,
    size_t      _length
)
{
    size_t           actual_len;
    size_t           capacity;
    struct d_string* new_string;

    if (!_cstr)
    {
        return NULL;
    }

    actual_len = d_strnlen(_cstr, _length);
    capacity   = actual_len + 1;

    new_string = d_string_new_with_capacity(capacity);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        return NULL;
    }

    d_memcpy(new_string->text, _cstr, actual_len);
    new_string->text[actual_len] = '\0';
    new_string->size             = actual_len;

    return new_string;
}

/*
d_string_new_from_buffer
  Creates a d_string from a buffer of specified length (not necessarily 
null-terminated).

Parameter(s):
  _buffer: source buffer to copy from.
  _length: number of bytes to copy.
Return:
  A pointer value corresponding to either:
  - newly allocated d_string, if successful, or
  - NULL, if _buffer was NULL or memory allocation failed.
*/
struct d_string*
d_string_new_from_buffer
(
    const char* _buffer,
    size_t      _length
)
{
    size_t           capacity;
    struct d_string* new_string;

    if (!_buffer)
    {
        return NULL;
    }

    capacity   = _length + 1;
    new_string = d_string_new_with_capacity(capacity);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        return NULL;
    }

    d_memcpy(new_string->text, _buffer, _length);
    new_string->text[_length] = '\0';
    new_string->size          = _length;

    return new_string;
}

/*
d_string_new_copy
  Creates a deep copy of an existing d_string.

Parameter(s):
  _other: d_string to copy.
Return:
  A pointer value corresponding to either:
  - newly allocated d_string copy, if successful, or
  - NULL, if _other was NULL or memory allocation failed.
*/
D_INLINE struct d_string*
d_string_new_copy
(
    const struct d_string* _other
)
{
    return (_other)
        ? d_string_new_from_buffer(_other->text, _other->size)
        : NULL;
}

/*
d_string_new_fill
  Creates a d_string filled with a repeated character.

Parameter(s):
  _length:    number of times to repeat the character.
  _fill_char: character to fill with.
Return:
  A pointer value corresponding to either:
  - newly allocated d_string, if successful, or
  - NULL, if memory allocation failed.
*/
struct d_string*
d_string_new_fill
(
    size_t _length,
    char   _fill_char
)
{
    struct d_string* new_string;

    new_string = d_string_new_with_capacity(_length + 1);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        return NULL;
    }

    d_memset(new_string->text, _fill_char, _length);
    new_string->text[_length] = '\0';
    new_string->size          = _length;

    return new_string;
}

/*
d_string_new_formatted
  Creates a d_string using printf-style formatting.

Parameter(s):
  _format: printf-style format string.
  ...:     format arguments.
Return:
  A pointer value corresponding to either:
  - newly allocated d_string, if successful, or
  - NULL, if _format was NULL or memory allocation failed.
*/
struct d_string*
d_string_new_formatted
(
    const char* _format, 
    ...
)
{
    va_list          args;
    va_list          args_copy;
    int              len;
    struct d_string* new_string;

    if (!_format)
    {
        return NULL;
    }

    va_start(args, _format);
    va_copy(args_copy, args);

    // determine required length
    len = vsnprintf(NULL, 0, _format, args);
    va_end(args);

    if (len < 0)
    {
        va_end(args_copy);

        return NULL;
    }

    new_string = d_string_new_with_capacity((size_t)len + 1);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        va_end(args_copy);

        return NULL;
    }

    vsnprintf(new_string->text, (size_t)len + 1, _format, args_copy);
    va_end(args_copy);

    new_string->size = (size_t)len;

    return new_string;
}


/******************************************************************************
* Capacity Management Functions
******************************************************************************/
/*
d_string_reserve
  Ensures the `d_string` has at least the specified capacity.

Parameter(s):
  _string:   `d_string` to modify.
  _capacity: minimum capacity to reserve.
Return:
  A boolean value corresponding to either:
  - true, if capacity was reserved successfully, or
  - false, if _string was NULL or allocation failed.
*/
D_INLINE bool
d_string_reserve
(
    struct d_string* _string,
    size_t           _capacity
)
{
    return (_string)
        ? d_string_internal_grow(_string, _capacity)
        : false;
}

/*
d_string_shrink_to_fit
  Reduces capacity to match the current size.

Parameter(s):
  _string: d_string to shrink.
Return:
  A boolean value corresponding to either:
  - true, if shrinking was successful, or
  - false, if _string was NULL or reallocation failed.
*/
bool
d_string_shrink_to_fit
(
    struct d_string* _string
)
{
    size_t new_capacity;
    char*  new_text;

    if (!_string)
    {
        return false;
    }

    new_capacity = _string->size + 1;

    // don't shrink if already at minimum
    if (_string->capacity <= new_capacity)
    {
        return true;
    }

    new_text = malloc(new_capacity);

    // ensure that memory allocation was successful
    if (!new_text)
    {
        return false;
    }

    d_memcpy(new_text, _string->text, new_capacity);
    free(_string->text);

    _string->text     = new_text;
    _string->capacity = new_capacity;

    return true;
}

/*
d_string_capacity
  Returns the current capacity of a d_string.

Parameter(s):
  _string: d_string to query.
Return:
  The capacity of the string, or 0 if _string is NULL.
*/
D_INLINE size_t
d_string_capacity
(
    const struct d_string* _string
)
{
    return (_string)
        ? _string->capacity
        : 0;
}

/*
d_string_resize
  Resizes the `d_string` to the specified size.

Parameter(s):
  _string:   `d_string` to resize.
  _new_size: new size for the `d_string`.
Return:
  A boolean value corresponding to either:
  - true, if resize was successful, or
  - false, if _string was NULL or allocation failed.
*/
bool
d_string_resize
(
    struct d_string* _string,
    size_t           _new_size
)
{
    // must be non-NULL
    // grow if needed
    if ( (!_string) ||
         (!d_string_internal_grow(_string, _new_size + 1)) ) 
    {
        return false;
    }

    // if growing, fill with nulls
    if (_new_size > _string->size)
    {
        d_memset(_string->text + _string->size, '\0', _new_size - _string->size);
    }

    _string->size             = _new_size;
    _string->text[_new_size]  = '\0';

    return true;
}


/******************************************************************************
* Access Functions
******************************************************************************/

/*
d_string_length
  Returns the length of the `d_string` (excluding null terminator).

Parameter(s):
  _string: `d_string` to query.
Return:
  The length of the string, or 0 if _string is NULL.
*/
D_INLINE size_t
d_string_length
(
    const struct d_string* _string
)
{
    return (_string)
        ? _string->size
        : 0;
}

/*
d_string_size
  Alias for `d_string`_length.

Parameter(s):
  _string: `d_string` to query.
Return:
  The size of the `d_string`, or 0 if _string is NULL.
*/
D_INLINE size_t
d_string_size
(
    const struct d_string* _string
)
{
    return d_string_length(_string);
}

/*
d_string_cstr
  Returns a const pointer to the null-terminated string data.

Parameter(s):
  _string: `d_string` to access.
Return:
  A pointer value corresponding to either:
  - const pointer to the string data, or
  - NULL, if _string is NULL.
*/
D_INLINE const char*
d_string_cstr
(
    const struct d_string* _string
)
{
    return (_string)
        ? _string->text
        : NULL;
}

/*
d_string_data
  Returns a mutable pointer to the string data.

Parameter(s):
  _string: `d_string` to access.
Return:
  A pointer value corresponding to either:
  - pointer to the string data, or
  - NULL, if _string is NULL.
*/
D_INLINE char*
d_string_data
(
    struct d_string* _string
)
{
    return (_string)
        ? _string->text
        : NULL;
}

/*
d_string_is_empty
  Checks if the `d_string` is empty.

Parameter(s):
  _string: `d_string` to check.
Return:
  A boolean value corresponding to either:
  - true, if _string is NULL or empty, or
  - false, if _string contains characters.
*/
D_INLINE bool
d_string_is_empty
(
    const struct d_string* _string
)
{
    return ( (!_string) || 
             (_string->size == 0) );
}

/*
d_string_char_at
  Returns the character at the specified index.

Parameter(s):
  _string: `d_string` to access.
  _index:  index of character (negative indices count from end).
Return:
  The character at the index, or '\0' if index is invalid.
*/
D_INLINE char
d_string_char_at
(
    const struct d_string* _string,
    d_index                _index
)
{
    size_t pos;

    return ( (_string) &&
             (d_index_convert_safe(_index, _string->size, &pos)) )
        ? _string->text[pos]
        : '\0';
}

/*
d_string_set_char
  Sets the character at the specified index.

Parameter(s):
  _string: `d_string` to modify.
  _index:  index of character (negative indices count from end).
  _c:      character to set.
Return:
  A boolean value corresponding to either:
  - true, if character was set successfully, or
  - false, if _string was NULL or index was invalid.
*/
bool
d_string_set_char
(
    struct d_string* _string,
    d_index          _index,
    char             _c
)
{
    size_t pos;

    if ( (!_string) ||
         (!d_index_convert_safe(_index, _string->size, &pos)) )
    {
        return false;
    }

    _string->text[pos] = _c;

    return true;
}

/*
d_string_front
  Returns the first character of the string.

Parameter(s):
  _string: `d_string` to access.
Return:
  The first character, or '\0' if string is empty or NULL.
*/
D_INLINE char
d_string_front
(
    const struct d_string* _string
)
{
    return d_string_is_empty(_string)
        ? '\0'
        : _string->text[0];
}

/*
d_string_back
  Returns the last character of the string.

Parameter(s):
  _string: `d_string` to access.
Return:
  The last character, or '\0' if string is empty or NULL.
*/
D_INLINE char
d_string_back
(
    const struct d_string* _string
)
{
    return d_string_is_empty(_string)
        ? '\0'
        : _string->text[_string->size - 1];
}


/******************************************************************************
* Safe Copy Functions
******************************************************************************/

/*
d_string_copy_s
  Safe copy from d_string to d_string.

Parameter(s):
  _destination: destination d_string.
  _source:      source d_string.
Return:
  An integer value corresponding to either:
  - 0, if copy was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_copy_s
(
    struct d_string*       _destination,
    const struct d_string* _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    } 
    else if (!d_string_internal_grow(_destination, _source->size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text, _source->text, _source->size + 1);
    _destination->size = _source->size;

    return 0;
}

/*
d_string_copy_cstr_s
  Safe copy from C string to d_string.

Parameter(s):
  _destination: destination d_string.
  _source:      source C string.
Return:
  An integer value corresponding to either:
  - 0, if copy was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_copy_cstr_s
(
    struct d_string* _destination,
    const char*      _source
)
{
    size_t len;

    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    len = strlen(_source);

    if (!d_string_internal_grow(_destination, len + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text, _source, len + 1);

    _destination->size = len;

    return 0;
}

/*
d_string_ncopy_s
  Safe-bounded copy from `d_string` to `d_string`.

Parameter(s):
  _destination: destination `d_string`.
  _source:      source `d_string`.
  _count:       maximum number of characters to copy.
Return:
  An integer value corresponding to either:
  - 0, if copy was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_ncopy_s
(
    struct d_string*        _destination,
    const struct d_string*  _source,
    size_t                  _count
)
{
    size_t copy_len;

    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    copy_len = (_count < _source->size) ? _count : _source->size;

    if (!d_string_internal_grow(_destination, copy_len + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text, _source->text, copy_len);
    _destination->text[copy_len] = '\0';
    _destination->size           = copy_len;

    return 0;
}

/*
d_string_ncopy_cstr_s
  Safe-bounded copy from C string to `d_string`.

Parameter(s):
  _destination: destination `d_string`.
  _source:      source C string.
  _count:       maximum number of characters to copy.
Return:
  An integer value corresponding to either:
  - 0, if copy was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_ncopy_cstr_s
(
    struct d_string* _destination,
    const char*      _source,
    size_t           _count
)
{
    size_t copy_len;

    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    copy_len = d_strnlen(_source, _count);

    if (!d_string_internal_grow(_destination, copy_len + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text, _source, copy_len);
    _destination->text[copy_len] = '\0';
    _destination->size           = copy_len;

    return 0;
}

/*
d_string_to_buffer_s
  Safe copy from `d_string` to char buffer.

Parameter(s):
  _destination:      destination buffer.
  _destination_size: size of destination buffer.
  _source:           source `d_string`.
Return:
  An integer value corresponding to either:
  - 0, if copy was successful, or
  - EINVAL, if _destination or _source was NULL, or
  - ERANGE, if destination buffer is too small.
*/
int
d_string_to_buffer_s
(
    char* restrict         _destination, 
    size_t                 _destination_size,
    const struct d_string* _source
)
{
    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    if (_destination_size == 0)
    {
        return ERANGE;
    }

    if (_source->size >= _destination_size)
    {
        _destination[0] = '\0';

        return ERANGE;
    }

    d_memcpy(_destination, _source->text, _source->size + 1);

    return 0;
}


/******************************************************************************
* Concatenation Functions
******************************************************************************/

/*
d_string_cat_s
  Safe concatenation of `d_string` to `d_string`.

Parameter(s):
  _destination: destination `d_string`.
  _source:      source `d_string` to append.
Return:
  An integer value corresponding to either:
  - 0, if concatenation was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_cat_s
(
    struct d_string* restrict       _destination,
    const struct d_string* restrict _source
)
{
    size_t new_size;

    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    new_size = (_destination->size + _source->size);

    if (!d_string_internal_grow(_destination, new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size, _source->text, _source->size + 1);
    _destination->size = new_size;

    return 0;
}

/*
d_string_cat_cstr_s
  Safe concatenation of C string to d_string.

Parameter(s):
  _destination: destination d_string.
  _source:      source C string to append.
Return:
  An integer value corresponding to either:
  - 0, if concatenation was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_cat_cstr_s
(
    struct d_string* restrict _destination,
    const char* restrict      _source
)
{
    size_t src_len;
    size_t new_size;

    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    src_len  = strlen(_source);
    new_size = (_destination->size + src_len);

    if (!d_string_internal_grow(_destination, new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size, _source, src_len + 1);
    _destination->size = new_size;

    return 0;
}

/*
d_string_ncat_s
  Safe-bounded concatenation of `d_string` to `d_string`.

Parameter(s):
  _destination: destination `d_string`.
  _source:      source `d_string` to append.
  _count:       maximum number of characters to append.
Return:
  An integer value corresponding to either:
  - 0, if concatenation was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_ncat_s
(
    struct d_string* restrict       _destination,
    const struct d_string* restrict _source,
    size_t                          _count
)
{
    size_t append_len;
    size_t new_size;

    if ( (!_destination) || 
         (!_source) )
    {
        return EINVAL;
    }

    append_len = (_count < _source->size) 
        ? _count
        : _source->size;
    new_size = (_destination->size + append_len);

    if (!d_string_internal_grow(_destination, new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size, _source->text, append_len);
    _destination->size            = new_size;
    _destination->text[new_size]  = '\0';

    return 0;
}

/*
d_string_ncat_cstr_s
  Safe-bounded concatenation of C string to `d_string`.

Parameter(s):
  _destination: destination `d_string`.
  _source:      source C string to append.
  _count:       maximum number of characters to append.
Return:
  An integer value corresponding to either:
  - 0, if concatenation was successful, or
  - EINVAL, if either parameter was NULL.
*/
int
d_string_ncat_cstr_s
(
    struct d_string* restrict _destination,
    const char* restrict      _source,
    size_t                    _count
)
{
    size_t append_len;
    size_t new_size;

    if ( (_destination == NULL) || 
         (_source == NULL) )
    {
        return EINVAL;
    }

    append_len = d_strnlen(_source, _count);
    new_size   = _destination->size + append_len;

    if (!d_string_internal_grow(_destination, new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size, _source, append_len);
    _destination->size            = new_size;
    _destination->text[new_size]  = '\0';

    return 0;
}


/******************************************************************************
* Duplication Functions
******************************************************************************/

/*
d_string_dup
  Duplicate a `d_string`.

Parameter(s):
  _string: `d_string` to duplicate.
Return:
  A pointer value corresponding to either:
  - newly allocated copy, if successful, or
  - NULL, if _string was NULL or allocation failed.
*/
D_INLINE struct d_string*
d_string_dup
(
    const struct d_string* _string
)
{
    return d_string_new_copy(_string);
}

/*
d_string_ndup
  Duplicate at most n characters of a `d_string`.

Parameter(s):
  _string: `d_string` to duplicate.
  _n:       maximum number of characters to copy.
Return:
  A pointer value corresponding to either:
  - newly allocated copy, if successful, or
  - NULL, if _string was NULL or allocation failed.
*/
D_INLINE struct d_string*
d_string_ndup
(
    const struct d_string* _string,
    size_t                 _n
)
{
    return ( (_string) 
        ? d_string_new_from_buffer(_string->text,
                                   ( (_n < _string->size)
                                       ? _n
                                       : _string->size) )
        : NULL );
}

/*
d_string_substr
  Extract substring from `d_string`.

Parameter(s):
  _string: source d_string.
  _start:  starting index (negative counts from end).
  _length: number of characters to extract.
Return:
  A pointer value corresponding to either:
  - newly allocated substring, if successful, or
  - NULL, if _string was NULL or index was invalid.
*/
struct d_string*
d_string_substr
(
    const struct d_string* _string,
    d_index                _start,
    size_t                 _length
)
{
    size_t start_pos;
    size_t actual_len;

    if ( (!_string) ||
         (!d_index_convert_safe(_start, _string->size, &start_pos)) )
    {
        return NULL;
    }

    // clamp length to available characters
    actual_len = _length;

    if (start_pos + actual_len > _string->size)
    {
        actual_len = (_string->size - start_pos);
    }

    return d_string_new_from_buffer(_string->text + start_pos, actual_len);
}


/******************************************************************************
* Comparison Functions
******************************************************************************/

/*
d_string_compare
  Compares two `d_string`s lexicographically using length-aware comparison.

Parameter(s):
  _s1: the first `d_string` to compare.
  _s2: the second `d_string` to compare.
Return:
  A value less than, equal to, or greater than zero if _s1 is found to be
  less than, equal to, or greater than _s2, respectively. Returns 0 if both
  are NULL.
*/
int
d_string_compare
(
    const struct d_string* _s1,
    const struct d_string* _s2
)
{
    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return d_strcmp_n(_s1->text,
                      _s1->size,
                      _s2->text,
                      _s2->size);
}

/*
d_string_compare_cstr
  Compares a `d_string` against a C string lexicographically using length-aware
comparison.

Parameter(s):
  _s1: the `d_string` to compare.
  _s2: the C string to compare against; may be NULL.
Return:
  A value less than, equal to, or greater than zero if _s1 is found to be
  less than, equal to, or greater than _s2, respectively. Returns 0 if both
  are NULL.
*/
D_INLINE int
d_string_compare_cstr
(
    const struct d_string* _s1,
    const char*            _s2
)
{
    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return d_strcmp_n(_s1->text,
                     _s1->size,
                     _s2,
                     strlen(_s2));
}

/*
d_string_ncmp
  Compare at most n characters of two d_strings.

Parameter(s):
  _s1: first d_string.
  _s2: second d_string.
  _n:  maximum characters to compare.
Return:
  An integer value indicating relationship.
*/
int
d_string_ncmp
(
    const struct d_string* _s1,
    const struct d_string* _s2,
    size_t                 _n
)
{
    size_t len1;
    size_t len2;
    size_t min_len;
    int    result;

    if (!_n)
    {
        return 0;
    }

    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    len1 = (_n < _s1->size) 
        ? _n
        : _s1->size;
    len2    = (_n < _s2->size)
        ? _n
        : _s2->size;
    min_len = (len1 < len2) ? len1 : len2;

    result = memcmp(_s1->text, _s2->text, min_len);

    if (result != 0)
    {
        return result;
    }

    if (len1 < len2)
    {
        return -1;
    }

    if (len1 > len2)
    {
        return 1;
    }

    return 0;
}

/*
d_string_ncmp_cstr
  Compare at most n characters of `d_string` to C string.

Parameter(s):
  _s1: `d_string` being compared.
  _s2: C string being compared.
  _n:  maximum characters to compare.
Return:
  An integer value indicating relationship.
*/
D_INLINE int
d_string_ncmp_cstr
(
    const struct d_string* _s1,
    const char*            _s2,
    size_t                 _n
)
{
    if (_n == 0)
    {
        return 0;
    }

    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return strncmp(_s1->text, _s2, _n);
}

/*
d_string_casecmp
  Case-insensitive comparison of two `d_string`s.

Parameter(s):
  _s1: first `d_string` being compared. 
  _s2: second `d_string` being compared.
Return:
  An integer value indicating relationship.
*/
D_INLINE int
d_string_casecmp
(
    const struct d_string* _s1,
    const struct d_string* _s2
)
{
    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return d_strcasecmp(_s1->text, _s2->text);
}

/*
d_string_casecmp_cstr
  Case-insensitive comparison of `d_string` to C string.

Parameter(s):
  _s1: `d_string` being compared.
  _s2: C string being compared.
Return:
  An integer value indicating relationship.
*/
D_INLINE int
d_string_casecmp_cstr
(
    const struct d_string* _s1,
    const char*            _s2
)
{
    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return d_strcasecmp(_s1->text, _s2);
}

/*
d_string_ncasecmp
  Case-insensitive comparison of at most n characters.

Parameter(s):
  _s1: first `d_string` being compared.
  _s2: second `d_string` being compared.
  _n:  maximum characters to compare.
Return:
  An integer value indicating relationship.
*/
D_INLINE int
d_string_ncasecmp
(
    const struct d_string* _s1,
    const struct d_string* _s2,
    size_t                 _n
)
{
    if (_n == 0)
    {
        return 0;
    }

    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return d_strncasecmp(_s1->text, _s2->text, _n);
}

/*
d_string_ncasecmp_cstr
  Case-insensitive comparison of at most n characters from a `d_string` to a 
C string.

Parameter(s):
  _s1: `d_string` being compared.
  _s2: C string being compared.
  _n:  maximum characters to compare.
Return:
  An integer value indicating relationship.
*/
D_INLINE int
d_string_ncasecmp_cstr
(
    const struct d_string* _s1,
    const char*            _s2,
    size_t                 _n
)
{
    if (_n == 0)
    {
        return 0;
    }

    if ( (!_s1) && 
         (!_s2) )
    {
        return 0;
    }

    if (!_s1)
    {
        return -1;
    }

    if (!_s2)
    {
        return 1;
    }

    return d_strncasecmp(_s1->text, _s2, _n);
}

/*
d_string_equals
  Check if two `d_string`s are equal.

Parameter(s):
  _s1: first `d_string` being compared.
  _s2: second `d_string` being compared.
Return:
  A boolean value: true if equal, false otherwise.
*/
D_INLINE bool
d_string_equals
(
    const struct d_string* _s1,
    const struct d_string* _s2
)
{
    return (d_string_compare(_s1, _s2) == 0);
}

/*
d_string_equals_cstr
  Check if d_string equals C string.

Parameter(s):
  _s1: d_string.
  _s2: C string.
Return:
  A boolean value: true if equal, false otherwise.
*/
D_INLINE bool
d_string_equals_cstr
(
    const struct d_string* _s1,
    const char*            _s2
)
{
    return (d_string_compare_cstr(_s1, _s2) == 0);
}

/*
d_string_equals_ignore_case
  Check if two `d_string`s are equal ignoring case.

Parameter(s):
  _s1: first `d_string` being compared.
  _s2: second `d_string` being compared.
Return:
  A boolean value: true if equal ignoring case, false otherwise.
*/
D_INLINE bool
d_string_equals_ignore_case
(
    const struct d_string* _s1,
    const struct d_string* _s2
)
{
    return (d_string_casecmp(_s1, _s2) == 0);
}

/*
d_string_equals_cstr_ignore_case
  Check if d_string equals C string ignoring case.

Parameter(s):
  _s1: first `d_string` being compared.
  _s2: C string being compared.
Return:
  A boolean value: true if equal ignoring case, false otherwise.
*/
D_INLINE bool
d_string_equals_cstr_ignore_case
(
    const struct d_string* _s1,
    const char*            _s2
)
{
    return (d_string_casecmp_cstr(_s1, _s2) == 0);
}

/*
d_string_find_char
  Searches for the first occurrence of a character in a d_string.

Parameter(s):
  _string: the string to search.
  _c:      the character to find.
Return:
  The index of the first occurrence of _c in the string, or D_STRING_NPOS
  if _c was not found or _string is NULL.
*/
d_index
d_string_find_char
(
    const struct d_string* _string,
    char                   _c
)
{
    const char* p;

    if ( (!_string) || 
         (!_string->text) )
    {
        return -1;
    }

    p = strchr(_string->text, _c);

    return (p)
        ? (ssize_t)(p - _string->text)
        : -1;
}

/*
d_string_find_char_from
  Find first occurrence of character starting from index in a given `d_string`.

Parameter(s):
  _string: `d_string` to search.
  _c:      character to find.
  _start:  starting index.
Return:
  Index of character, or -1 if not found.
*/
ssize_t
d_string_find_char_from
(
    const struct d_string* _string,
    char                   _c,
    d_index                _start
)
{
    size_t      start_pos;
    const char* p;

    if ( (!_string)       || 
         (!_string->text) ||
         (!d_index_convert_safe(_start, _string->size, &start_pos)) )
    {
        return -1;
    }

    p = strchr(_string->text + start_pos, _c);

    return (p)
        ? (ssize_t)(p - _string->text)
        : -1;
}

/*
d_string_rfind_char
  Find last occurrence of character in `d_string`.

Parameter(s):
  _string: `d_string` to search.
  _c:      character to find.
Return:
  Index of character, or -1 if not found.
*/
ssize_t
d_string_rfind_char
(
    const struct d_string* _string,
    char                   _c
)
{
    const char* p;

    if ( (!_string) || 
         (!_string->text) )
    {
        return -1;
    }

    p = strrchr(_string->text, _c);

    return (p)
        ? (ssize_t)(p - _string->text)
        : -1;
}

/*
d_string_chr
  Find character in `d_string`, returning pointer.

Parameter(s):
  _string: `d_string` to search.
  _c:      character to find.
Return:
  Pointer to character, or NULL if not found.
*/
D_INLINE char*
d_string_chr
(
    const struct d_string* _string,
    int                    _c
)
{
    return ( (_string) &&
             (_string->text) )
        ? strchr(_string->text, _c)
        : NULL;
}

/*
d_string_rchr
  Find last occurrence of character in a `d_string`, returning pointer.

Parameter(s):
  _string: `d_string` to search.
  _c:      character to find.
Return:
  Pointer to character, or NULL if not found.
*/
D_INLINE char*
d_string_rchr
(
    const struct d_string* _string,
    int                    _c
)
{
    return ( (_string) &&
             (_string->text) )
        ? strrchr(_string->text, _c)
        : NULL;
}

/*
d_string_chrnul
  Find character or return pointer to null terminator.

Parameter(s):
  _string: d_string to search.
  _c:   character to find.
Return:
  Pointer to character or null terminator, NULL only if _string is NULL.
*/
char*
d_string_chrnul
(
    const struct d_string* _string,
    int                    _c
)
{
    return ( (_string) &&
             (_string->text) )
        ? d_strchrnul(_string->text, _c)
        : NULL;
}


/******************************************************************************
* Search Functions - Substring Search
******************************************************************************/

/*
d_string_find
  Find first occurrence of substring in d_string.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   d_string to search for.
Return:
  Index of substring, or -1 if not found.
*/
ssize_t
d_string_find
(
    const struct d_string* _haystack,
    const struct d_string* _needle
)
{
    const char* p;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    if (_needle->size == 0)
    {
        return 0;
    }

    p = strstr(_haystack->text, _needle->text);

    if (p == NULL)
    {
        return -1;
    }

    return (ssize_t)(p - _haystack->text);
}

/*
d_string_find_cstr
  Find first occurrence of C string in d_string.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   C string to search for.
Return:
  Index of substring, or -1 if not found.
*/
ssize_t
d_string_find_cstr
(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    const char* p;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    if (*_needle == '\0')
    {
        return 0;
    }

    p = strstr(_haystack->text, _needle);

    if (p == NULL)
    {
        return -1;
    }

    return (ssize_t)(p - _haystack->text);
}

/*
d_string_find_from
  Find substring starting from index.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   d_string to search for.
  _start:    starting index.
Return:
  Index of substring, or -1 if not found.
*/
ssize_t
d_string_find_from
(
    const struct d_string* _haystack,
    const struct d_string* _needle,
    d_index                _start
)
{
    size_t      start_pos;
    const char* p;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    if (!d_index_convert_safe(_start, _haystack->size, &start_pos))
    {
        return -1;
    }

    p = strstr(_haystack->text + start_pos, _needle->text);

    if (p == NULL)
    {
        return -1;
    }

    return (ssize_t)(p - _haystack->text);
}

/*
d_string_find_cstr_from
  Find C string starting from index.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   C string to search for.
  _start:    starting index.
Return:
  Index of substring, or -1 if not found.
*/
ssize_t
d_string_find_cstr_from
(
    const struct d_string* _haystack,
    const char*            _needle,
    d_index                _start
)
{
    size_t      start_pos;
    const char* p;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    if (!d_index_convert_safe(_start, _haystack->size, &start_pos))
    {
        return -1;
    }

    p = strstr(_haystack->text + start_pos, _needle);

    if (p == NULL)
    {
        return -1;
    }

    return (ssize_t)(p - _haystack->text);
}

/*
d_string_rfind
  Find last occurrence of substring.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   d_string to search for.
Return:
  Index of last occurrence, or -1 if not found.
*/
ssize_t
d_string_rfind
(
    const struct d_string* _haystack,
    const struct d_string* _needle
)
{
    ssize_t     last_pos;
    const char* p;
    const char* search_start;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    if (_needle->size == 0)
    {
        return (ssize_t)_haystack->size;
    }

    if (_needle->size > _haystack->size)
    {
        return -1;
    }

    last_pos     = -1;
    search_start = _haystack->text;

    while ((p = strstr(search_start, _needle->text)) != NULL)
    {
        last_pos     = (ssize_t)(p - _haystack->text);
        search_start = p + 1;
    }

    return last_pos;
}

/*
d_string_rfind_cstr
  Find last occurrence of C string.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   C string to search for.
Return:
  Index of last occurrence, or -1 if not found.
*/
ssize_t
d_string_rfind_cstr
(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    ssize_t     last_pos;
    const char* p;
    const char* search_start;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    if (*_needle == '\0')
    {
        return (ssize_t)_haystack->size;
    }

    last_pos     = -1;
    search_start = _haystack->text;

    while ((p = strstr(search_start, _needle)) != NULL)
    {
        last_pos     = (ssize_t)(p - _haystack->text);
        search_start = p + 1;
    }

    return last_pos;
}

/*
d_string_str
  Find substring, returning pointer (strstr equivalent).

Parameter(s):
  _haystack: d_string to search in.
  _needle:   C string to search for.
Return:
  Pointer to substring, or NULL if not found.
*/
D_INLINE char*
d_string_str
(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    return ( (_haystack) &&
             (_needle) )
        ? strstr(_haystack->text, _needle)
        : NULL;
}

/*
d_string_casefind
  Case-insensitive find substring.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   d_string to search for.
Return:
  Index of substring, or -1 if not found.
*/
ssize_t
d_string_casefind
(
    const struct d_string* _haystack,
    const struct d_string* _needle
)
{
    char* p;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    p = d_strcasestr(_haystack->text, _needle->text);

    if (p == NULL)
    {
        return -1;
    }

    return (ssize_t)(p - _haystack->text);
}

/*
d_string_casefind_cstr
  Case-insensitive find C string.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   C string to search for.
Return:
  Index of substring, or -1 if not found.
*/
ssize_t
d_string_casefind_cstr
(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    char* p;

    if ( (!_haystack) || 
         (!_needle) )
    {
        return -1;
    }

    p = d_strcasestr(_haystack->text, _needle);

    if (p == NULL)
    {
        return -1;
    }

    return (ssize_t)(p - _haystack->text);
}

/*
d_string_casestr
  Case-insensitive substring search returning pointer.

Parameter(s):
  _haystack: d_string to search in.
  _needle:   C string to search for.
Return:
  Pointer to substring, or NULL if not found.
*/
D_INLINE char*
d_string_casestr
(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    return ( (_haystack) &&
             (_needle) )
        ? d_strcasestr(_haystack->text, _needle)
        : NULL;
}


/******************************************************************************
* Search Functions - Containment Checks
******************************************************************************/

/*
d_string_contains
  Check if `d_string` contains substring.

Parameter(s):
  _string: `d_string` to search in.
  _substr: `d_string` to search for.
Return:
  true if _substr is found in _string, false otherwise.
*/
D_INLINE bool
d_string_contains
(
    const struct d_string* _string,
    const struct d_string* _substr
)
{
    return (d_string_find(_string, _substr) >= 0);
}

/*
d_string_contains_cstr
  Check if d_string contains C string.

Parameter(s):
  _string:    d_string to search in.
  _substr: C string to search for.
Return:
  true if _substr is found in _string, false otherwise.
*/
D_INLINE bool
d_string_contains_cstr
(
    const struct d_string* _string,
    const char*            _substr
)
{
    return (d_string_find_cstr(_string, _substr) >= 0);
}

/*
d_string_contains_char
  Check if `d_string` contains character.

Parameter(s):
  _string: `d_string` to search in.
  _c:       character to search for.
Return:
  true if _c is found in _string, false otherwise.
*/
D_INLINE bool
d_string_contains_char
(
    const struct d_string* _string,
    char                   _c
)
{
    return (d_string_find_char(_string, _c) >= 0);
}

/*
d_string_starts_with
  Check if `d_string` starts with prefix.

Parameter(s):
  _string: `d_string` to check.
  _prefix: `d_string` prefix.
Return:
  true if _string starts with _prefix, false otherwise.
*/
D_INLINE bool
d_string_starts_with
(
    const struct d_string* _string,
    const struct d_string* _prefix
)
{
    return ( (_string) &&
             (_prefix) &&
             (_prefix->size <= _string->size) )
        ? (memcmp(_string->text, _prefix->text, _prefix->size) == 0)
        : false;
}

/*
d_string_starts_with_cstr
  Check if `d_string` starts with C string prefix.

Parameter(s):
  _string: `d_string` to check.
  _prefix: C string prefix.
Return:
  true if _string starts with _prefix, false otherwise.
*/
bool
d_string_starts_with_cstr
(
    const struct d_string* _string,
    const char*            _prefix
)
{
    size_t prefix_len;

    if ( (!_string) || 
         (!_prefix) )
    {
        return false;
    }

    prefix_len = strlen(_prefix);

    if (prefix_len > _string->size)
    {
        return false;
    }

    return (memcmp(_string->text, _prefix, prefix_len) == 0);
}

/*
d_string_ends_with
  Check if `d_string` ends with suffix.

Parameter(s):
  _string: `d_string` to check.
  _suffix: `d_string` suffix.
Return:
  true if _string ends with _suffix, false otherwise.
*/
bool
d_string_ends_with
(
    const struct d_string* _string,
    const struct d_string* _suffix
)
{
    size_t offset;

    if ( (_string == NULL) || 
         (_suffix == NULL) )
    {
        return false;
    }

    if (_suffix->size > _string->size)
    {
        return false;
    }

    offset = _string->size - _suffix->size;

    return (memcmp(_string->text + offset, _suffix->text, _suffix->size) == 0);
}

/*
d_string_ends_with_cstr
  Check if `d_string` ends with C string suffix.

Parameter(s):
  _string: `d_string` to check.
  _suffix: C string suffix.
Return:
  true if _string ends with _suffix, false otherwise.
*/
bool
d_string_ends_with_cstr
(
    const struct d_string* _string,
    const char*            _suffix
)
{
    size_t suffix_len;
    size_t offset;

    if ( (!_string) || 
         (!_suffix) )
    {
        return false;
    }

    suffix_len = strlen(_suffix);

    if (suffix_len > _string->size)
    {
        return false;
    }

    offset = (_string->size - suffix_len);

    return (memcmp(_string->text + offset, _suffix, suffix_len) == 0);
}

/*
d_string_spn
  Get length of initial segment containing only characters in accept.

Parameter(s):
  _string: `d_string` to scan.
  _accept: string of accepted characters.
Return:
  Length of initial segment.
*/
D_INLINE size_t
d_string_spn
(
    const struct d_string* _string,
    const char*            _accept
)
{
    return ( (_string) && 
             (_accept) )
        ? strspn(_string->text, _accept)
        : 0;
}

/*
d_string_cspn
  Get length of initial segment not containing characters in reject.

Parameter(s):
  _string:    d_string to scan.
  _reject: string of rejected characters.
Return:
  Length of initial segment.
*/
size_t
d_string_cspn
(
    const struct d_string* _string,
    const char*            _reject
)
{
    if ( (_string == NULL) || 
         (_reject == NULL) )
    {
        return 0;
    }

    return strcspn(_string->text, _reject);
}

/*
d_string_pbrk
  Find first occurrence of any character from accept string.

Parameter(s):
  _string:    d_string to search.
  _accept: string of characters to find.
Return:
  Pointer to first matching character, or NULL if none found.
*/
char*
d_string_pbrk
(
    const struct d_string* _string,
    const char*            _accept
)
{
    if ( (_string == NULL) || 
         (_accept == NULL) )
    {
        return NULL;
    }

    return strpbrk(_string->text, _accept);
}


/******************************************************************************
* Modification Functions - Assignment
******************************************************************************/

/*
d_string_assign
  Assign content from another d_string.

Parameter(s):
  _string:   d_string to modify.
  _other: source d_string.
Return:
  true if successful, false otherwise.
*/
bool
d_string_assign
(
    struct d_string*       _string,
    const struct d_string* _other
)
{
    if ( (!_string) || 
         (!_other) )
    {
        return false;
    }

    return (d_string_copy_s(_string, _other) == 0);
}

/*
d_string_assign_cstr
  Assign content from C string.

Parameter(s):
  _string:  d_string to modify.
  _cstr: source C string.
Return:
  true if successful, false otherwise.
*/
bool
d_string_assign_cstr
(
    struct d_string* _string,
    const char*      _cstr
)
{
    if ( (_string == NULL) || 
         (_cstr == NULL) )
    {
        return false;
    }

    return (d_string_copy_cstr_s(_string, _cstr) == 0);
}

/*
d_string_assign_buffer
  Assign content from buffer.

Parameter(s):
  _string:    d_string to modify.
  _buffer: source buffer.
  _length: number of bytes to copy.
Return:
  true if successful, false otherwise.
*/
bool
d_string_assign_buffer
(
    struct d_string* _string,
    const char*      _buffer,
    size_t           _length
)
{
    if ( (_string == NULL) || 
         (_buffer == NULL) )
    {
        return false;
    }

    if (!d_string_internal_grow(_string, _length + 1))
    {
        return false;
    }

    d_memcpy(_string->text, _buffer, _length);
    _string->text[_length] = '\0';
    _string->size          = _length;

    return true;
}

/*
d_string_assign_char
  Assign repeated character.

Parameter(s):
  _string:   d_string to modify.
  _count: number of times to repeat.
  _c:     character to assign.
Return:
  true if successful, false otherwise.
*/
bool
d_string_assign_char
(
    struct d_string* _string,
    size_t           _count,
    char             _c
)
{
    if (_string == NULL)
    {
        return false;
    }

    if (!d_string_internal_grow(_string, _count + 1))
    {
        return false;
    }

    d_memset(_string->text, _c, _count);
    _string->text[_count] = '\0';
    _string->size         = _count;

    return true;
}


/******************************************************************************
* Modification Functions - Append
******************************************************************************/

/*
d_string_append
  Append another d_string.

Parameter(s):
  _string:   d_string to modify.
  _other: d_string to append.
Return:
  true if successful, false otherwise.
*/
bool
d_string_append
(
    struct d_string*       _string,
    const struct d_string* _other
)
{
    if ( (_string == NULL) || 
         (_other == NULL) )
    {
        return false;
    }

    return (d_string_cat_s(_string, _other) == 0);
}

/*
d_string_append_cstr
  Append C string.

Parameter(s):
  _string:  d_string to modify.
  _cstr: C string to append.
Return:
  true if successful, false otherwise.
*/
bool
d_string_append_cstr
(
    struct d_string* _string,
    const char*      _cstr
)
{
    if ( (_string == NULL) || 
         (_cstr == NULL) )
    {
        return false;
    }

    return (d_string_cat_cstr_s(_string, _cstr) == 0);
}

/*
d_string_append_buffer
  Append buffer of specified length.

Parameter(s):
  _string:    d_string to modify.
  _buffer: buffer to append.
  _length: number of bytes to append.
Return:
  true if successful, false otherwise.
*/
bool
d_string_append_buffer
(
    struct d_string* _string,
    const char*      _buffer,
    size_t           _length
)
{
    size_t new_size;

    if ( (_string == NULL) || 
         (_buffer == NULL) )
    {
        return false;
    }

    new_size = _string->size + _length;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    d_memcpy(_string->text + _string->size, _buffer, _length);
    _string->text[new_size] = '\0';
    _string->size           = new_size;

    return true;
}

/*
d_string_append_char
  Append single character.

Parameter(s):
  _string: d_string to modify.
  _c:   character to append.
Return:
  true if successful, false otherwise.
*/
bool
d_string_append_char
(
    struct d_string* _string,
    char             _c
)
{
    size_t new_size;

    if (_string == NULL)
    {
        return false;
    }

    new_size = _string->size + 1;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    _string->text[_string->size] = _c;
    _string->text[new_size]   = '\0';
    _string->size             = new_size;

    return true;
}

/*
d_string_append_formatted
  Append printf-style formatted text.

Parameter(s):
  _string:    d_string to modify.
  _format: format string.
  ...:     format arguments.
Return:
  true if successful, false otherwise.
*/
bool
d_string_append_formatted
(
    struct d_string* _string,
    const char*      _format,
    ...
)
{
    va_list args;
    va_list args_copy;
    int     len;
    size_t  new_size;

    if ( (_string == NULL) || 
         (_format == NULL) )
    {
        return false;
    }

    va_start(args, _format);
    va_copy(args_copy, args);

    len = vsnprintf(NULL, 0, _format, args);
    va_end(args);

    if (len < 0)
    {
        va_end(args_copy);

        return false;
    }

    new_size = _string->size + (size_t)len;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        va_end(args_copy);

        return false;
    }

    vsnprintf(_string->text + _string->size, (size_t)len + 1, _format, args_copy);
    va_end(args_copy);

    _string->size = new_size;

    return true;
}


/******************************************************************************
* Modification Functions - Prepend
******************************************************************************/

/*
d_string_prepend
  Prepend another d_string.

Parameter(s):
  _string:   d_string to modify.
  _other: d_string to prepend.
Return:
  true if successful, false otherwise.
*/
bool
d_string_prepend
(
    struct d_string*       _string,
    const struct d_string* _other
)
{
    size_t new_size;

    if ( (_string == NULL) || 
         (_other == NULL) )
    {
        return false;
    }

    new_size = _string->size + _other->size;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    // shift existing content
    memmove(_string->text + _other->size, _string->text, _string->size + 1);

    // copy prepend content
    d_memcpy(_string->text, _other->text, _other->size);
    _string->size = new_size;

    return true;
}

/*
d_string_prepend_cstr
  Prepend C string.

Parameter(s):
  _string:  d_string to modify.
  _cstr: C string to prepend.
Return:
  true if successful, false otherwise.
*/
bool
d_string_prepend_cstr
(
    struct d_string* _string,
    const char*      _cstr
)
{
    size_t cstr_len;
    size_t new_size;

    if ( (_string == NULL) || 
         (_cstr == NULL) )
    {
        return false;
    }

    cstr_len = strlen(_cstr);
    new_size = _string->size + cstr_len;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    memmove(_string->text + cstr_len, _string->text, _string->size + 1);
    d_memcpy(_string->text, _cstr, cstr_len);
    _string->size = new_size;

    return true;
}

/*
d_string_prepend_char
  Prepend single character.

Parameter(s):
  _string: d_string to modify.
  _c:   character to prepend.
Return:
  true if successful, false otherwise.
*/
bool
d_string_prepend_char
(
    struct d_string* _string,
    char             _c
)
{
    size_t new_size;

    if (_string == NULL)
    {
        return false;
    }

    new_size = _string->size + 1;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    memmove(_string->text + 1, _string->text, _string->size + 1);
    _string->text[0] = _c;
    _string->size    = new_size;

    return true;
}


/******************************************************************************
* Modification Functions - Insert
******************************************************************************/

/*
d_string_insert
  Insert d_string at index.

Parameter(s):
  _string:   d_string to modify.
  _index: insertion point.
  _other: d_string to insert.
Return:
  true if successful, false otherwise.
*/
bool
d_string_insert
(
    struct d_string*       _string,
    d_index                _index,
    const struct d_string* _other
)
{
    size_t pos;
    size_t new_size;

    if ( (_string == NULL) || 
         (_other == NULL) )
    {
        return false;
    }

    // special case: insert at end
    if (_index == (d_index)_string->size)
    {
        return d_string_append(_string, _other);
    }

    if (!d_index_convert_safe(_index, _string->size, &pos))
    {
        return false;
    }

    new_size = _string->size + _other->size;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    // shift content after insertion point
    memmove(_string->text + pos + _other->size,
            _string->text + pos,
            _string->size - pos + 1);

    // insert new content
    d_memcpy(_string->text + pos, _other->text, _other->size);
    _string->size = new_size;

    return true;
}

/*
d_string_insert_cstr
  Insert C string at index.

Parameter(s):
  _string:   d_string to modify.
  _index: insertion point.
  _cstr:  C string to insert.
Return:
  true if successful, false otherwise.
*/
bool
d_string_insert_cstr
(
    struct d_string* _string,
    d_index          _index,
    const char*      _cstr
)
{
    size_t pos;
    size_t cstr_len;
    size_t new_size;

    if ( (_string == NULL) || 
         (_cstr == NULL) )
    {
        return false;
    }

    // special case: insert at end
    if (_index == (d_index)_string->size)
    {
        return d_string_append_cstr(_string, _cstr);
    }

    if (!d_index_convert_safe(_index, _string->size, &pos))
    {
        return false;
    }

    cstr_len = strlen(_cstr);
    new_size = _string->size + cstr_len;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    memmove(_string->text + pos + cstr_len,
            _string->text + pos,
            _string->size - pos + 1);

    d_memcpy(_string->text + pos, _cstr, cstr_len);
    _string->size = new_size;

    return true;
}

/*
d_string_insert_char
  Insert character at index.

Parameter(s):
  _string:   d_string to modify.
  _index: insertion point.
  _c:     character to insert.
Return:
  true if successful, false otherwise.
*/
bool
d_string_insert_char
(
    struct d_string* _string,
    d_index          _index,
    char             _c
)
{
    size_t pos;
    size_t new_size;

    if (_string == NULL)
    {
        return false;
    }

    // special case: insert at end
    if (_index == (d_index)_string->size)
    {
        return d_string_append_char(_string, _c);
    }

    if (!d_index_convert_safe(_index, _string->size, &pos))
    {
        return false;
    }

    new_size = _string->size + 1;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    memmove(_string->text + pos + 1,
            _string->text + pos,
            _string->size - pos + 1);

    _string->text[pos] = _c;
    _string->size      = new_size;

    return true;
}


/******************************************************************************
* Modification Functions - Erase and Clear
******************************************************************************/

/*
d_string_erase
  Erase characters from string.

Parameter(s):
  _string:   d_string to modify.
  _index: starting index.
  _count: number of characters to erase.
Return:
  true if successful, false otherwise.
*/
bool
d_string_erase
(
    struct d_string* _string,
    d_index          _index,
    size_t           _count
)
{
    size_t pos;
    size_t actual_count;

    if (_string == NULL)
    {
        return false;
    }

    if (!d_index_convert_safe(_index, _string->size, &pos))
    {
        return false;
    }

    // clamp count to available characters
    actual_count = _count;

    if (pos + actual_count > _string->size)
    {
        actual_count = _string->size - pos;
    }

    // shift remaining content
    memmove(_string->text + pos,
            _string->text + pos + actual_count,
            _string->size - pos - actual_count + 1);

    _string->size -= actual_count;

    return true;
}

/*
d_string_erase_char
  Erase single character at index.

Parameter(s):
  _string:   d_string to modify.
  _index: index of character to erase.
Return:
  true if successful, false otherwise.
*/
bool
d_string_erase_char
(
    struct d_string* _string,
    d_index          _index
)
{
    return d_string_erase(_string, _index, 1);
}

/*
d_string_clear
  Clear string contents (set to empty).

Parameter(s):
  _string: d_string to clear.
Return:
  (none)
*/
void
d_string_clear
(
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return;
    }

    if (_string->text != NULL)
    {
        _string->text[0] = '\0';
    }

    _string->size = 0;

    return;
}


/******************************************************************************
* Modification Functions - Replace
******************************************************************************/

/*
d_string_replace
  Replace portion of string with d_string.

Parameter(s):
  _string:         d_string to modify.
  _index:       starting index of replacement.
  _count:       number of characters to replace.
  _replacement: d_string to insert.
Return:
  true if successful, false otherwise.
*/
bool
d_string_replace
(
    struct d_string*       _string,
    d_index                _index,
    size_t                 _count,
    const struct d_string* _replacement
)
{
    size_t pos;
    size_t actual_count;
    size_t new_size;

    if ( (_string == NULL) || 
         (_replacement == NULL) )
    {
        return false;
    }

    if (!d_index_convert_safe(_index, _string->size, &pos))
    {
        return false;
    }

    // clamp count
    actual_count = _count;

    if (pos + actual_count > _string->size)
    {
        actual_count = _string->size - pos;
    }

    new_size = _string->size - actual_count + _replacement->size;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    // shift content after replacement region
    memmove(_string->text + pos + _replacement->size,
            _string->text + pos + actual_count,
            _string->size - pos - actual_count + 1);

    // copy replacement
    d_memcpy(_string->text + pos, _replacement->text, _replacement->size);
    _string->size = new_size;

    return true;
}

/*
d_string_replace_cstr
  Replace portion of string with C string.

Parameter(s):
  _string:         d_string to modify.
  _index:       starting index of replacement.
  _count:       number of characters to replace.
  _replacement: C string to insert.
Return:
  true if successful, false otherwise.
*/
bool
d_string_replace_cstr
(
    struct d_string* _string,
    d_index          _index,
    size_t           _count,
    const char*      _replacement
)
{
    size_t pos;
    size_t actual_count;
    size_t rep_len;
    size_t new_size;

    if ( (_string == NULL) || 
         (_replacement == NULL) )
    {
        return false;
    }

    if (!d_index_convert_safe(_index, _string->size, &pos))
    {
        return false;
    }

    actual_count = _count;

    if (pos + actual_count > _string->size)
    {
        actual_count = _string->size - pos;
    }

    rep_len  = strlen(_replacement);
    new_size = _string->size - actual_count + rep_len;

    if (!d_string_internal_grow(_string, new_size + 1))
    {
        return false;
    }

    memmove(_string->text + pos + rep_len,
            _string->text + pos + actual_count,
            _string->size - pos - actual_count + 1);

    d_memcpy(_string->text + pos, _replacement, rep_len);
    _string->size = new_size;

    return true;
}

/*
d_string_replace_all
  Replace all occurrences of substring.

Parameter(s):
  _string: d_string to modify.
  _old: d_string to find and replace.
  _new: d_string replacement.
Return:
  true if successful, false otherwise.
*/
bool
d_string_replace_all
(
    struct d_string*       _string,
    const struct d_string* _old,
    const struct d_string* _new
)
{
    if ( (_string == NULL) || 
         (_old == NULL) || 
         (_new == NULL) ||
         (_old->size == 0) )
    {
        return false;
    }

    return d_string_replace_all_cstr(_string, _old->text, _new->text);
}

/*
d_string_replace_all_cstr
  Replace all occurrences of C string.

Parameter(s):
  _string: d_string to modify.
  _old: C string to find and replace.
  _new: C string replacement.
Return:
  true if successful, false otherwise.
*/
bool
d_string_replace_all_cstr
(
    struct d_string* _string,
    const char* _old,
    const char* _new
)
{
    size_t           old_len;
    size_t           new_len;
    size_t           count;
    size_t           new_size;
    char* search;
    char* found;
    char* write_ptr;
    char* read_ptr;
    struct d_string* result;

    if ((_string == NULL) ||
        (_old == NULL) ||
        (_new == NULL))
    {
        return false;
    }

    old_len = strlen(_old);

    if (old_len == 0)
    {
        return false;
    }

    new_len = strlen(_new);

    // count occurrences
    count = 0;
    search = _string->text;

    while ((found = strstr(search, _old)) != NULL)
    {
        count++;
        search = found + old_len;
    }

    // if no occurrences, nothing to do
    if (count == 0)
    {
        return true;
    }

    // calculate new size (result length excluding '\0')
    new_size = _string->size + (count * new_len) - (count * old_len);

    // create temporary result (+1 for '\0')
    result = d_string_new_with_capacity(new_size + 1);

    if (result == NULL)
    {
        return false;
    }

    // build result
    read_ptr = _string->text;
    write_ptr = result->text;

    while ((found = strstr(read_ptr, _old)) != NULL)
    {
        size_t before_len = (size_t)(found - read_ptr);

        // copy text before match
        d_memcpy(write_ptr, read_ptr, before_len);
        write_ptr += before_len;

        // copy replacement
        if (new_len > 0)
        {
            d_memcpy(write_ptr, _new, new_len);
            write_ptr += new_len;
        }

        read_ptr = found + old_len;
    }

    // copy remaining text (including the terminating '\0')
    {
        size_t tail_len = strlen(read_ptr);
        d_memcpy(write_ptr, read_ptr, tail_len + 1);
        write_ptr += tail_len;
    }

    // optional sanity: ensure we produced exactly new_size chars
    // (write_ptr now points at the '\0' position)
    // d_assert((size_t)(write_ptr - result->text) == new_size);

    // swap contents
    free(_string->text);
    _string->text = result->text;
    _string->size = new_size;
    _string->capacity = result->capacity;

    // free result struct (but not its text, which we've taken)
    free(result);

    return true;
}


/*
d_string_replace_char
  Replace all occurrences of a character.

Parameter(s):
  _string:      d_string to modify.
  _old_char: character to replace.
  _new_char: replacement character.
Return:
  true if successful, false otherwise.
*/
bool
d_string_replace_char
(
    struct d_string* _string,
    char             _old_char,
    char             _new_char
)
{
    size_t i;

    if (_string == NULL)
    {
        return false;
    }

    for (i = 0; i < _string->size; i++)
    {
        if (_string->text[i] == _old_char)
        {
            _string->text[i] = _new_char;
        }
    }

    return true;
}


/******************************************************************************
* Case Conversion Functions
******************************************************************************/

/*
d_string_to_lower
  Convert string to lowercase in-place.

Parameter(s):
  _string: d_string to convert.
Return:
  true if successful, false if _string is NULL.
*/
bool
d_string_to_lower
(
    struct d_string* _string
)
{
    if ( (_string == NULL) || 
         (_string->text == NULL) )
    {
        return false;
    }

    d_strlwr(_string->text);

    return true;
}

/*
d_string_to_upper
  Convert string to uppercase in-place.

Parameter(s):
  _string: d_string to convert.
Return:
  true if successful, false if _string is NULL.
*/
bool
d_string_to_upper
(
    struct d_string* _string
)
{
    if ( (_string == NULL) || 
         (_string->text == NULL) )
    {
        return false;
    }

    d_strupr(_string->text);

    return true;
}

/*
d_string_lower
  Return new lowercase copy of string.

Parameter(s):
  _string: d_string to copy and convert.
Return:
  New lowercase d_string, or NULL if _string is NULL or allocation fails.
*/
struct d_string*
d_string_lower
(
    const struct d_string* _string
)
{
    struct d_string* result;

    if (_string == NULL)
    {
        return NULL;
    }

    result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_to_lower(result);

    return result;
}

/*
d_string_upper
  Return new uppercase copy of string.

Parameter(s):
  _string: d_string to copy and convert.
Return:
  New uppercase d_string, or NULL if _string is NULL or allocation fails.
*/
struct d_string*
d_string_upper
(
    const struct d_string* _string
)
{
    struct d_string* result;

    if (_string == NULL)
    {
        return NULL;
    }

    result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_to_upper(result);

    return result;
}


/******************************************************************************
* Reversal Functions
******************************************************************************/

/*
d_string_reverse
  Reverse string in-place.

Parameter(s):
  _string: d_string to reverse.
Return:
  true if successful, false if _string is NULL.
*/
bool
d_string_reverse
(
    struct d_string* _string
)
{
    if ( (_string == NULL) || 
         (_string->text == NULL) )
    {
        return false;
    }

    d_strrev(_string->text);

    return true;
}

/*
d_string_reversed
  Return new reversed copy of string.

Parameter(s):
  _string: d_string to copy and reverse.
Return:
  New reversed d_string, or NULL if _string is NULL or allocation fails.
*/
struct d_string*
d_string_reversed
(
    const struct d_string* _string
)
{
    struct d_string* result;

    if (_string == NULL)
    {
        return NULL;
    }

    result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_reverse(result);

    return result;
}


/******************************************************************************
* Trimming Functions
******************************************************************************/

/*
d_string_trim
  Trim whitespace from both ends of string in-place.

Parameter(s):
  _string: d_string to trim.
Return:
  true if successful, false if _string is NULL.
*/
bool
d_string_trim
(
    struct d_string* _string
)
{
    size_t start;
    size_t end;
    size_t new_size;

    if (_string == NULL)
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    // find first non-whitespace
    start = 0;

    while ( (start < _string->size) && 
            (isspace((unsigned char)_string->text[start])) )
    {
        start++;
    }

    // all whitespace
    if (start == _string->size)
    {
        _string->text[0] = '\0';
        _string->size    = 0;

        return true;
    }

    // find last non-whitespace
    end = _string->size - 1;

    while ( (end > start) && 
            (isspace((unsigned char)_string->text[end])) )
    {
        end--;
    }

    new_size = end - start + 1;

    // shift content if needed
    if (start > 0)
    {
        memmove(_string->text, _string->text + start, new_size);
    }

    _string->text[new_size] = '\0';
    _string->size           = new_size;

    return true;
}

/*
d_string_trim_left
  Trim whitespace from left side of string in-place.

Parameter(s):
  _string: d_string to trim.
Return:
  true if successful, false if _string is NULL.
*/
bool
d_string_trim_left
(
    struct d_string* _string
)
{
    size_t start;
    size_t new_size;

    if (_string == NULL)
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    start = 0;

    while ( (start < _string->size) && 
            (isspace((unsigned char)_string->text[start])) )
    {
        start++;
    }

    if (start == _string->size)
    {
        _string->text[0] = '\0';
        _string->size    = 0;

        return true;
    }

    if (start > 0)
    {
        new_size = _string->size - start;
        memmove(_string->text, _string->text + start, new_size + 1);
        _string->size = new_size;
    }

    return true;
}

/*
d_string_trim_right
  Trim whitespace from right side of string in-place.

Parameter(s):
  _string: d_string to trim.
Return:
  true if successful, false if _string is NULL.
*/
bool
d_string_trim_right
(
    struct d_string* _string
)
{
    size_t end;

    if (_string == NULL)
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    end = _string->size;

    while ( (end > 0) && 
            (isspace((unsigned char)_string->text[end - 1])) )
    {
        end--;
    }

    _string->text[end] = '\0';
    _string->size      = end;

    return true;
}

/*
d_string_trim_chars
  Trim specified characters from both ends.

Parameter(s):
  _string:   d_string to trim.
  _chars: characters to trim.
Return:
  true if successful, false if parameters are NULL.
*/
bool
d_string_trim_chars
(
    struct d_string* _string,
    const char*      _chars
)
{
    size_t start;
    size_t end;
    size_t new_size;

    if ( (_string == NULL) || 
         (_chars == NULL) )
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    // find first character not in _chars
    start = 0;

    while ( (start < _string->size) && 
            (strchr(_chars, _string->text[start]) != NULL) )
    {
        start++;
    }

    if (start == _string->size)
    {
        _string->text[0] = '\0';
        _string->size    = 0;

        return true;
    }

    // find last character not in _chars
    end = _string->size - 1;

    while ( (end > start) && 
            (strchr(_chars, _string->text[end]) != NULL) )
    {
        end--;
    }

    new_size = end - start + 1;

    if (start > 0)
    {
        memmove(_string->text, _string->text + start, new_size);
    }

    _string->text[new_size] = '\0';
    _string->size           = new_size;

    return true;
}

/*
d_string_trimmed
  Return new trimmed copy of string.

Parameter(s):
  _string: d_string to copy and trim.
Return:
  New trimmed d_string, or NULL if _string is NULL or allocation fails.
*/
struct d_string*
d_string_trimmed
(
    const struct d_string* _string
)
{
    struct d_string* result;

    if (_string == NULL)
    {
        return NULL;
    }

    result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_trim(result);

    return result;
}

/*
d_string_trimmed_left
  Return new left-trimmed copy of string.

Parameter(s):
  _string: d_string to copy and trim.
Return:
  New trimmed d_string, or NULL if _string is NULL or allocation fails.
*/
struct d_string*
d_string_trimmed_left
(
    const struct d_string* _string
)
{
    struct d_string* result;

    if (_string == NULL)
    {
        return NULL;
    }

    result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_trim_left(result);

    return result;
}

/*
d_string_trimmed_right
  Return new right-trimmed copy of string.

Parameter(s):
  _string: d_string to copy and trim.
Return:
  New trimmed d_string, or NULL if _string is NULL or allocation fails.
*/
struct d_string*
d_string_trimmed_right
(
    const struct d_string* _string
)
{
    struct d_string* result;

    if (_string == NULL)
    {
        return NULL;
    }

    result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_trim_right(result);

    return result;
}


/******************************************************************************
* Tokenization Functions
******************************************************************************/

/*
d_string_tokenize
  Thread-safe string tokenization.

Parameter(s):
  _string:     d_string to tokenize (NULL to continue).
  _delim:   delimiter characters.
  _saveptr: save state pointer.
Return:
  Pointer to next token, or NULL if no more tokens.
*/
char*
d_string_tokenize
(
    struct d_string* _string,
    const char*      _delim,
    char**           _saveptr
)
{
    char* start;

    if ( (_delim == NULL) || 
         (_saveptr == NULL) )
    {
        return NULL;
    }

    if (_string != NULL)
    {
        start = _string->text;
    }
    else
    {
        start = NULL;
    }

    return d_strtok_r(start, _delim, _saveptr);
}

/*
d_string_split
  Split string into array of d_strings.

Parameter(s):
  _string:    d_string to split.
  _delim:  delimiter characters.
  _tokens: output array of d_strings (caller must free with d_string_split_free).
Return:
  Number of tokens, or 0 on error.
*/
size_t
d_string_split
(
    const struct d_string*  _string,
    const char*             _delim,
    struct d_string***      _tokens
)
{
    char*             copy;
    char*             saveptr;
    char*             token;
    size_t            count;
    size_t            capacity;
    struct d_string** result;
    struct d_string** new_result;

    if ( (_string == NULL) || 
         (_delim == NULL) || 
         (_tokens == NULL) )
    {
        return 0;
    }

    *_tokens = NULL;

    // handle empty string: return single empty token
    if (_string->size == 0)
    {
        result = malloc(sizeof(struct d_string*));

        if (result == NULL)
        {
            return 0;
        }

        result[0] = d_string_new();

        if (result[0] == NULL)
        {
            free(result);

            return 0;
        }

        *_tokens = result;

        return 1;
    }

    // make copy for tokenization
    copy = d_strdup(_string->text);

    if (copy == NULL)
    {
        return 0;
    }

    // initial allocation
    capacity = 8;
    result   = malloc(capacity * sizeof(struct d_string*));

    if (result == NULL)
    {
        free(copy);

        return 0;
    }

    count   = 0;
    saveptr = NULL;
    token   = d_strtok_r(copy, _delim, &saveptr);

    while (token != NULL)
    {
        // grow array if needed
        if (count >= capacity)
        {
            capacity  *= 2;
            new_result = (struct d_string**)realloc(result,
                                                    capacity * sizeof(struct d_string*));

            if (new_result == NULL)
            {
                // cleanup on failure
                for (size_t i = 0; i < count; i++)
                {
                    d_string_free(result[i]);
                }

                free(result);
                free(copy);

                return 0;
            }

            result = new_result;
        }

        // create d_string for token
        result[count] = d_string_new_from_cstr(token);

        if (result[count] == NULL)
        {
            for (size_t i = 0; i < count; i++)
            {
                d_string_free(result[i]);
            }

            free(result);
            free(copy);

            return 0;
        }

        count++;
        token = d_strtok_r(NULL, _delim, &saveptr);
    }

    free(copy);

    *_tokens = result;

    return count;
}

/*
d_string_split_free
  Free array of d_strings from d_string_split.

Parameter(s):
  _tokens: array of d_strings.
  _count:  number of tokens in array.
Return:
  (none)
*/
void
d_string_split_free
(
    struct d_string** _tokens,
    size_t            _count
)
{
    size_t i;

    if (_tokens == NULL)
    {
        return;
    }

    for (i = 0; i < _count; i++)
    {
        d_string_free(_tokens[i]);
    }

    free(_tokens);

    return;
}


/******************************************************************************
* Join Functions
******************************************************************************/

/*
d_string_join
  Join array of d_strings with delimiter.

Parameter(s):
  _strings:   array of d_string pointers.
  _count:     number of strings in array.
  _delimiter: delimiter to insert between strings.
Return:
  New d_string with joined content, or NULL on error.
*/
struct d_string*
d_string_join
(
    const struct d_string* const* _strings,
    size_t                        _count,
    const char*                   _delimiter
)
{
    size_t           total_len;
    size_t           delim_len;
    size_t           i;
    struct d_string* result;

    if (_count == 0)
    {
        return d_string_new();
    }

    if ( (_strings == NULL) || 
         (_delimiter == NULL) )
    {
        return NULL;
    }

    delim_len = strlen(_delimiter);

    // calculate total length
    total_len = 0;

    for (i = 0; i < _count; i++)
    {
        if (_strings[i] != NULL)
        {
            total_len += _strings[i]->size;
        }

        if (i < _count - 1)
        {
            total_len += delim_len;
        }
    }

    result = d_string_new_with_capacity(total_len + 1);

    if (result == NULL)
    {
        return NULL;
    }

    // build result
    for (i = 0; i < _count; i++)
    {
        if (_strings[i] != NULL)
        {
            d_string_append(result, _strings[i]);
        }

        if ( (i < _count - 1) && 
             (delim_len > 0) )
        {
            d_string_append_cstr(result, _delimiter);
        }
    }

    return result;
}

/*
d_string_join_cstr
  Join array of C strings with delimiter.

Parameter(s):
  _strings:   array of C string pointers.
  _count:     number of strings in array.
  _delimiter: delimiter to insert between strings.
Return:
  New d_string with joined content, or NULL on error.
*/
struct d_string*
d_string_join_cstr
(
    const char* const* _strings,
    size_t             _count,
    const char*        _delimiter
)
{
    size_t           total_len;
    size_t           delim_len;
    size_t           i;
    struct d_string* result;

    if (_count == 0)
    {
        return d_string_new();
    }

    if ( (_strings == NULL) || 
         (_delimiter == NULL) )
    {
        return NULL;
    }

    delim_len = strlen(_delimiter);

    // calculate total length
    total_len = 0;

    for (i = 0; i < _count; i++)
    {
        if (_strings[i] != NULL)
        {
            total_len += strlen(_strings[i]);
        }

        if (i < _count - 1)
        {
            total_len += delim_len;
        }
    }

    result = d_string_new_with_capacity(total_len + 1);

    if (result == NULL)
    {
        return NULL;
    }

    for (i = 0; i < _count; i++)
    {
        if (_strings[i] != NULL)
        {
            d_string_append_cstr(result, _strings[i]);
        }

        if ( (i < _count - 1) && 
             (delim_len > 0) )
        {
            d_string_append_cstr(result, _delimiter);
        }
    }

    return result;
}

/*
d_string_concat
  Concatenate multiple d_strings into one.

Parameter(s):
  _count: number of d_strings to concatenate.
  ...:    d_string pointers to concatenate.
Return:
  New d_string with concatenated content, or NULL on error.
*/
struct d_string*
d_string_concat
(
    size_t _count, 
    ...
)
{
    va_list                args;
    size_t                 total_len;
    size_t                 i;
    const struct d_string* str;
    struct d_string*       result;

    if (_count == 0)
    {
        return d_string_new();
    }

    // first pass: calculate total length and check for NULL
    va_start(args, _count);
    total_len = 0;

    for (i = 0; i < _count; i++)
    {
        str = va_arg(args, const struct d_string*);

        if (str == NULL)
        {
            va_end(args);

            return NULL;
        }

        total_len += str->size;
    }

    va_end(args);

    result = d_string_new_with_capacity(total_len + 1);

    if (result == NULL)
    {
        return NULL;
    }

    // second pass: concatenate
    va_start(args, _count);

    for (i = 0; i < _count; i++)
    {
        str = va_arg(args, const struct d_string*);

        d_string_append(result, str);
    }

    va_end(args);

    return result;
}


/******************************************************************************
* Validation Functions
******************************************************************************/

/*
d_string_is_valid
  Check if a `d_string` is valid: defined as non-NULL, with a non-NULL buffer,
and a null-terminator at the index corresponding to the parameter's `size`
field.

Parameter(s):
  _string: `d_string` being checked for validity.
Return:
  A boolean value corresponding to either:
  - true, if and only if all characters are ASCII, or
  - false, if one or more characters in `_string` are non-ASCII characters.
*/
D_INLINE bool
d_string_is_valid
(
    const struct d_string* _string
)
{
    return (_string)
        ? ( d_str_is_valid(_string->text, _string->size) &&
            (_string->text[_string->size] == '\0') )
        : false;
}

/*
d_string_is_ascii
  Check if a `d_string` contains only ASCII characters.

Parameter(s):
  _string: valid, non-NULL `d_string` to check.
Return:
  A boolean value corresponding to either:
  - true, if and only if all characters are ASCII, or
  - false, if one or more characters in `_string` are non-ASCII characters.
*/
D_INLINE bool
d_string_is_ascii
(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_ascii(_string->text, _string->size)
        : false;
}

/*
d_string_is_numeric
  Check if a `d_string` contains only numeric characters.

Parameter(s):
  _string: valid, non-NULL `d_string` to check.
Return:
  A boolean value corresponding to either:
  - true, if and only if all characters are numeric, or
  - false, if one or more characters in `_string` are non-numeric characters.
*/
D_INLINE bool
d_string_is_numeric
(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_numeric(_string->text, _string->size)
        : false;
}

/*
d_string_is_alpha
  Check if a `d_string` contains only alphabetical characters.

Parameter(s):
  _string: valid, non-NULL `d_string` to check.
Return:
  A boolean value corresponding to either:
  - true, if and only if all characters are alphabetical, or
  - false, if one or more characters in `_string` are non-alphabetical characters.
*/
D_INLINE bool
d_string_is_alpha
(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_alpha(_string->text, _string->size)
        : false;
}

/*
d_string_is_alnum
  Check if a `d_string` contains only alphanumeric characters.

Parameter(s):
  _string: valid, non-NULL `d_string` to check.
Return:
  A boolean value corresponding to either:
  - true, if and only if all characters are alphanumeric, or
  - false, if one or more characters in `_string` are non-alphanumeric characters.
*/
D_INLINE bool
d_string_is_alnum
(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_alnum(_string->text, _string->size)
        : false;
}

/*
d_string_is_whitespace
  Check if a `d_string` contains only whitespace characters.

Parameter(s):
  _string: valid, non-NULL `d_string` to check.
Return:
  A boolean value corresponding to either:
  - true, if and only if all characters are whitespace characters, or
  - false, if one or more characters in `_string` are non-whitespace characters.
*/
D_INLINE bool
d_string_is_whitespace
(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_whitespace(_string->text, _string->size)
        : false;
}


/******************************************************************************
* Counting Functions
******************************************************************************/

/*
d_string_count_char
  Count occurrences of character in string.

Parameter(s):
  _string: d_string to search.
  _c:   character to count.
Return:
  Number of occurrences.
*/
size_t
d_string_count_char
(
    const struct d_string* _string,
    char                   _c
)
{
    size_t count;
    size_t i;

    if ( (!_string) || 
         (!_string->text) )
    {
        return 0;
    }

    count = 0;

    for (i = 0; i < _string->size; i++)
    {
        if (_string->text[i] == _c)
        {
            count++;
        }
    }

    return count;
}

/*
d_string_count_substr
  Count occurrences of substring in string.

Parameter(s):
  _string:    d_string to search.
  _substr: substring to count.
Return:
  Number of occurrences.
*/
size_t
d_string_count_substr
(
    const struct d_string* _string,
    const char*            _substr
)
{
    size_t      count;
    size_t      substr_len;
    const char* search;
    const char* found;

    if ( (_string == NULL) || 
         (_substr == NULL) )
    {
        return 0;
    }

    substr_len = strlen(_substr);

    if (substr_len == 0)
    {
        return 0;
    }

    count  = 0;
    search = _string->text;

    while ((found = strstr(search, _substr)) != NULL)
    {
        count++;
        search = found + substr_len;
    }

    return count;
}


/******************************************************************************
* Hash Function
******************************************************************************/

/*
d_string_hash
  Calculate hash value for string (djb2 algorithm).

Parameter(s):
  _string: d_string to hash.
Return:
  Hash value.
*/
size_t
d_string_hash
(
    const struct d_string* _string
)
{
    size_t hash;
    size_t i;

    if ( (_string == NULL) || 
         (_string->text == NULL) )
    {
        return 0;
    }

    hash = 5381;

    for (i = 0; i < _string->size; i++)
    {
        hash = ((hash << 5) + hash) + (unsigned char)_string->text[i];
    }

    return hash;
}


/******************************************************************************
* Error String Functions
******************************************************************************/

/*
d_string_error
  Get error description as d_string.

Parameter(s):
  _errnum: error number.
Return:
  New d_string with error description, or NULL on allocation failure.
*/
struct d_string*
d_string_error
(
    int _errnum
)
{
    char buf[256];

    if (d_strerror_r(_errnum, buf, sizeof(buf)) != 0)
    {
        return d_string_new_from_cstr("Unknown error");
    }

    return d_string_new_from_cstr(buf);
}

/*
d_string_error_r
  Thread-safe error string into provided d_string.

Parameter(s):
  _errnum: error number.
  _string:    d_string to store error description.
Return:
  0 on success, error code otherwise.
*/
int
d_string_error_r
(
    int              _errnum,
    struct d_string* _string
)
{
    char buf[256];
    int  result;

    if (_string == NULL)
    {
        return EINVAL;
    }

    result = d_strerror_r(_errnum, buf, sizeof(buf));

    if (result != 0)
    {
        return result;
    }

    if (!d_string_assign_cstr(_string, buf))
    {
        return EINVAL;
    }

    return 0;
}


/******************************************************************************
* Formatted String Functions
******************************************************************************/

/*
d_string_printf
  Create formatted d_string.

Parameter(s):
  _format: printf-style format string.
  ...:     format arguments.
Return:
  New formatted d_string, or NULL on error.
*/
struct d_string*
d_string_printf
(
    const char* _format, 
    ...
)
{
    va_list          args;
    struct d_string* result;

    va_start(args, _format);
    result = d_string_vprintf(_format, args);
    va_end(args);

    return result;
}

/*
d_string_vprintf
  Create formatted d_string with va_list.

Parameter(s):
  _format: printf-style format string.
  _args:   va_list of arguments.
Return:
  New formatted d_string, or NULL on error.
*/
struct d_string*
d_string_vprintf
(
    const char* _format,
    va_list     _args
)
{
    va_list          args_copy;
    int              len;
    struct d_string* result;

    if (_format == NULL)
    {
        return NULL;
    }

    va_copy(args_copy, _args);
    len = vsnprintf(NULL, 0, _format, _args);

    if (len < 0)
    {
        va_end(args_copy);

        return NULL;
    }

    result = d_string_new_with_capacity((size_t)len + 1);

    if (result == NULL)
    {
        va_end(args_copy);

        return NULL;
    }

    vsnprintf(result->text, (size_t)len + 1, _format, args_copy);
    va_end(args_copy);

    result->size = (size_t)len;

    return result;
}

/*
d_string_sprintf
  Format into existing d_string.

Parameter(s):
  _string:    d_string to format into (replaces content).
  _format: printf-style format string.
  ...:     format arguments.
Return:
  Number of characters written, or -1 on error.
*/
int
d_string_sprintf
(
    struct d_string* _string,
    const char*      _format,
    ...
)
{
    va_list args;
    va_list args_copy;
    int     len;

    if ( (_string == NULL) || 
         (_format == NULL) )
    {
        return -1;
    }

    va_start(args, _format);
    va_copy(args_copy, args);

    len = vsnprintf(NULL, 0, _format, args);
    va_end(args);

    if (len < 0)
    {
        va_end(args_copy);

        return -1;
    }

    if (!d_string_internal_grow(_string, (size_t)len + 1))
    {
        va_end(args_copy);

        return -1;
    }

    vsnprintf(_string->text, (size_t)len + 1, _format, args_copy);
    va_end(args_copy);

    _string->size = (size_t)len;

    return len;
}


/*
d_string_free
  Frees a d_string and its contents.

Parameter(s):
  _string: d_string to free.
Return:
  (none)
*/
void
d_string_free
(
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return;
    }

    if (_string->text != NULL)
    {
        free(_string->text);
    }

    free(_string);

    return;
}

/*
d_string_free_contents
  Frees the text buffer of a d_string but not the struct itself.

Parameter(s):
  _string: d_string whose contents to free.
Return:
  (none)
*/
void
d_string_free_contents
(
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return;
    }

    if (_string->text != NULL)
    {
        free(_string->text);
        _string->text = NULL;
    }

    _string->size     = 0;
    _string->capacity = 0;

    return;
}
