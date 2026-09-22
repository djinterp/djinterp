/*******************************************************************************
* djinterp [c]                                                         dstring.c
*
* Definitions for the declarations in `dstring.h`.
*   Implements the `d_string` safe string type and its operations, building on
* the raw-buffer primitives in `string_fn.h`.
*
* path:      /src/djinterp/c/dstring.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.09.22
*******************************************************************************/
#include "../../../inc/djinterp/c/dstring.h"  // corresponding header
// std
#include <ctype.h>                              // isspace
#include <errno.h>                              // EINVAL, ERANGE
#include <stdarg.h>                             // va_list, va_start, va_copy
#include <stdbool.h>                            // bool
#include <stddef.h>                             // size_t, NULL
#include <stdio.h>                              // vsnprintf
#include <stdlib.h>                             // malloc, realloc, free
#include <string.h>                             // strlen, strstr, memmove, ...
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"   // framework root
#include "../../../inc/djinterp/c/dmemory.h"    // d_memcpy, d_memset
#include "../../../inc/djinterp/c/string_fn.h"  // d_str* primitives


// internal helpers
/*
d_string_internal_grow
  File-local, so its contract lives here: ensures `_string` can hold
`_required` bytes, terminator included, and returns false only for a NULL
string or a failed allocation, in which case the string is untouched. Capacity
doubles from its current value (or from 16 when it is 0) until the request
fits, which keeps a run of appends amortized O(1). The old buffer is freed
only after the new one is filled.
*/
D_STATIC bool
d_string_internal_grow(
    struct d_string* _string,
    size_t           _required
)
{
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
    size_t new_capacity = _string->capacity;

    if (new_capacity == 0)
    {
        new_capacity = 16;
    }

    while (new_capacity < _required)
    {
        new_capacity *= 2;
    }

    // allocate new buffer
    char* new_text = malloc(new_capacity);

    // ensure that memory allocation was successful
    if (!new_text)
    {
        return false;
    }

    // copy existing content if present
    if ( (_string->text) &&
         (_string->size > 0) )
    {
        d_memcpy(new_text,
                 _string->text,
                 _string->size + 1);
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

// lifecycle: creation
/*
d_string_new
  Delegates to d_string_new_with_capacity() with a default capacity of 16.
*/
struct d_string*
d_string_new(
    void
)
{
    return d_string_new_with_capacity(16);
}

/*
d_string_new_with_capacity
  The struct is freed again if the text allocation fails, so a failed call
leaks nothing; the terminator is written at index 0 so the empty string is
valid at once.
*/
struct d_string*
d_string_new_with_capacity(
    size_t _capacity
)
{
    // ensure minimum capacity of 1 for null terminator
    if (_capacity == 0)
    {
        _capacity = 1;
    }

    struct d_string* new_string = malloc(sizeof(struct d_string));

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
  Measures `_cstr` once and copies the terminator along with the text, so the
buffer is filled exactly to its capacity.
*/
struct d_string*
d_string_new_from_cstr(
    const char* _cstr
)
{
    if (!_cstr)
    {
        return NULL;
    }

    const size_t len      = strlen(_cstr);
    const size_t capacity = len + 1;

    struct d_string* new_string = d_string_new_with_capacity(capacity);

    // ensure that memory allocation was successful
    if (!new_string)
    {
        return NULL;
    }

    d_memcpy(new_string->text,
             _cstr,
             len + 1);
    new_string->size = len;

    return new_string;
}

/*
d_string_new_from_cstr_n
  d_strnlen() bounds the scan at `_length`, so `_cstr` need not be terminated
within that range; the terminator is written explicitly after the copy.
*/
struct d_string*
d_string_new_from_cstr_n(
    const char* _cstr,
    size_t      _length
)
{
    if (!_cstr)
    {
        return NULL;
    }

    const size_t actual_len = d_strnlen(_cstr,
                                        _length);
    const size_t capacity   = actual_len + 1;

    struct d_string* new_string = d_string_new_with_capacity(capacity);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        return NULL;
    }

    d_memcpy(new_string->text,
             _cstr,
             actual_len);
    new_string->text[actual_len] = '\0';
    new_string->size             = actual_len;

    return new_string;
}

/*
d_string_new_from_buffer
  Copies exactly `_length` bytes, embedded NULs included, and terminates the
copy itself because the source need not be terminated.
*/
struct d_string*
d_string_new_from_buffer(
    const char* _buffer,
    size_t      _length
)
{
    if (!_buffer)
    {
        return NULL;
    }

    const size_t     capacity   = _length + 1;
    struct d_string* new_string = d_string_new_with_capacity(capacity);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        return NULL;
    }

    d_memcpy(new_string->text,
             _buffer,
             _length);
    new_string->text[_length] = '\0';
    new_string->size          = _length;

    return new_string;
}

/*
d_string_new_copy
  Delegates to d_string_new_from_buffer() with the source's size, so embedded
NULs survive and the copy's capacity fits its content rather than the source's
capacity.
*/
struct d_string*
d_string_new_copy(
    const struct d_string* _other
)
{
    return (_other)
        ? d_string_new_from_buffer(_other->text,
                                   _other->size)
        : NULL;
}

/*
d_string_new_fill
  Fills with d_memset() and writes the terminator separately.
*/
struct d_string*
d_string_new_fill(
    size_t _length,
    char   _fill_char
)
{
    struct d_string* new_string = d_string_new_with_capacity(_length + 1);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        return NULL;
    }

    d_memset(new_string->text,
             _fill_char,
             _length);
    new_string->text[_length] = '\0';
    new_string->size          = _length;

    return new_string;
}

/*
d_string_new_formatted
  Formats twice: a vsnprintf() dry run measures the output, then a va_copy()
of the arguments writes it into a buffer of exactly that size. Every early
return ends both lists.
*/
struct d_string*
d_string_new_formatted(
    const char* _format,
    ...
)
{
    if (!_format)
    {
        return NULL;
    }

    va_list args;
    va_list args_copy;

    va_start(args,
             _format);
    va_copy(args_copy,
            args);

    // determine required length
    const int len = vsnprintf(NULL,
                              0,
                              _format,
                              args);
    va_end(args);

    if (len < 0)
    {
        va_end(args_copy);

        return NULL;
    }

    struct d_string* new_string = d_string_new_with_capacity((size_t)len + 1);

    // ensure that new `dstring` was created successfully
    if (!new_string)
    {
        va_end(args_copy);

        return NULL;
    }

    vsnprintf(new_string->text,
              (size_t)len + 1,
              _format,
              args_copy);
    va_end(args_copy);

    new_string->size = (size_t)len;

    return new_string;
}

// lifecycle: capacity management
/*
d_string_reserve
  Delegates to d_string_internal_grow(), which never shrinks.
*/
bool
d_string_reserve(
    struct d_string* _string,
    size_t           _capacity
)
{
    return (_string)
        ? d_string_internal_grow(_string,
                                 _capacity)
        : false;
}

/*
d_string_shrink_to_fit
  Copies into a fresh buffer rather than calling realloc(), so an allocation
failure leaves the original buffer in place.
*/
bool
d_string_shrink_to_fit(
    struct d_string* _string
)
{
    if (!_string)
    {
        return false;
    }

    const size_t new_capacity = _string->size + 1;

    // don't shrink if already at minimum
    if (_string->capacity <= new_capacity)
    {
        return true;
    }

    char* new_text = malloc(new_capacity);

    // ensure that memory allocation was successful
    if (!new_text)
    {
        return false;
    }

    d_memcpy(new_text,
             _string->text,
             new_capacity);
    free(_string->text);

    _string->text     = new_text;
    _string->capacity = new_capacity;

    return true;
}

/*
d_string_capacity
  Reads the field directly.
*/
size_t
d_string_capacity(
    const struct d_string* _string
)
{
    return (_string)
        ? _string->capacity
        : 0;
}

/*
d_string_resize
  Growth goes through d_string_internal_grow() first, so a failed allocation
leaves the string unchanged; the terminator is rewritten at the new size in
every case, which is what truncates when shrinking.
*/
bool
d_string_resize(
    struct d_string* _string,
    size_t           _new_size
)
{
    // must be non-NULL, and grown first if the new size needs room
    if ( (!_string) ||
         (!d_string_internal_grow(_string,
                                  _new_size + 1)) )
    {
        return false;
    }

    // if growing, fill with nulls
    if (_new_size > _string->size)
    {
        d_memset(_string->text + _string->size,
                 '\0',
                 _new_size - _string->size);
    }

    _string->size            = _new_size;
    _string->text[_new_size] = '\0';

    return true;
}

// access
/*
d_string_length
  Reads the size field directly.
*/
size_t
d_string_length(
    const struct d_string* _string
)
{
    return (_string)
        ? _string->size
        : 0;
}

/*
d_string_size
  Delegates to d_string_length().
*/
size_t
d_string_size(
    const struct d_string* _string
)
{
    return d_string_length(_string);
}

/*
d_string_cstr
  Returns the internal buffer; no copy is made.
*/
const char*
d_string_cstr(
    const struct d_string* _string
)
{
    return (_string)
        ? _string->text
        : NULL;
}

/*
d_string_data
  Returns the internal buffer; no copy is made.
*/
char*
d_string_data(
    struct d_string* _string
)
{
    return (_string)
        ? _string->text
        : NULL;
}

/*
d_string_is_empty
  Treats NULL as empty, so callers can test an optional string in one call.
*/
bool
d_string_is_empty(
    const struct d_string* _string
)
{
    return ( (!_string) ||
             (_string->size == 0) );
}

/*
d_string_char_at
  d_index_convert_safe() both validates the index and resolves a negative one,
so `pos` is only read after the conversion has succeeded.
*/
char
d_string_char_at(
    const struct d_string* _string,
    d_index                _index
)
{
    size_t pos = 0;

    return ( (_string) &&
             (d_index_convert_safe(_index,
                                   _string->size,
                                   &pos)) )
        ? _string->text[pos]
        : '\0';
}

/*
d_string_set_char
  d_index_convert_safe() accepts only indices below the size, so the
terminator can never be overwritten.
*/
bool
d_string_set_char(
    struct d_string* _string,
    d_index          _index,
    char             _c
)
{
    size_t pos = 0;

    if ( (!_string) ||
         (!d_index_convert_safe(_index,
                                _string->size,
                                &pos)) )
    {
        return false;
    }

    _string->text[pos] = _c;

    return true;
}

/*
d_string_front
  d_string_is_empty() covers both NULL and empty, so index 0 is only read when
it holds a character.
*/
char
d_string_front(
    const struct d_string* _string
)
{
    return d_string_is_empty(_string)
        ? '\0'
        : _string->text[0];
}

/*
d_string_back
  d_string_is_empty() covers both NULL and empty, so `size - 1` cannot
underflow.
*/
char
d_string_back(
    const struct d_string* _string
)
{
    return d_string_is_empty(_string)
        ? '\0'
        : _string->text[_string->size - 1];
}

// copying: safe copy
/*
d_string_copy_s
  Grows the destination before touching its text, so a failed copy leaves it
unchanged; the terminator is copied along with the text.
*/
int
d_string_copy_s(
    struct d_string*       _destination,
    const struct d_string* _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }
    else if (!d_string_internal_grow(_destination,
                                     _source->size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text,
             _source->text,
             _source->size + 1);
    _destination->size = _source->size;

    return 0;
}

/*
d_string_copy_cstr_s
  Measures the source once, grows, then copies text and terminator together.
*/
int
d_string_copy_cstr_s(
    struct d_string* _destination,
    const char*      _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }

    const size_t len = strlen(_source);

    if (!d_string_internal_grow(_destination,
                                len + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text,
             _source,
             len + 1);

    _destination->size = len;

    return 0;
}

/*
d_string_ncopy_s
  Clamps `_count` to the source's size before growing, and terminates
explicitly because the copied range may stop short of the source's terminator.
*/
int
d_string_ncopy_s(
    struct d_string*        _destination,
    const struct d_string*  _source,
    size_t                  _count
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }

    const size_t copy_len = (_count < _source->size) ? _count : _source->size;

    if (!d_string_internal_grow(_destination,
                                copy_len + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text,
             _source->text,
             copy_len);
    _destination->text[copy_len] = '\0';
    _destination->size           = copy_len;

    return 0;
}

/*
d_string_ncopy_cstr_s
  d_strnlen() clamps the length without scanning past `_count` bytes; the
terminator is written explicitly.
*/
int
d_string_ncopy_cstr_s(
    struct d_string* _destination,
    const char*      _source,
    size_t           _count
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }

    const size_t copy_len = d_strnlen(_source,
                                      _count);

    if (!d_string_internal_grow(_destination,
                                copy_len + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text,
             _source,
             copy_len);
    _destination->text[copy_len] = '\0';
    _destination->size           = copy_len;

    return 0;
}

/*
d_string_to_buffer_s
  A zero-size destination is rejected before anything is written. Otherwise a
destination that is too small is cleared to an empty string, so a caller that
ignores ERANGE still holds a terminated buffer.
*/
int
d_string_to_buffer_s(
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

    d_memcpy(_destination,
             _source->text,
             _source->size + 1);

    return 0;
}

// copying: safe concatenation
/*
d_string_cat_s
  Grows the destination, then copies the source's text and terminator in one
d_memcpy(). That copy is why the parameters are restrict: appending a string
to itself would overlap by the terminator.
*/
int
d_string_cat_s(
    struct d_string* restrict       _destination,
    const struct d_string* restrict _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }

    const size_t new_size = (_destination->size + _source->size);

    if (!d_string_internal_grow(_destination,
                                new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size,
             _source->text,
             _source->size + 1);
    _destination->size = new_size;

    return 0;
}

/*
d_string_cat_cstr_s
  Measures the source once, grows, then copies text and terminator together.
*/
int
d_string_cat_cstr_s(
    struct d_string* restrict _destination,
    const char* restrict      _source
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }

    const size_t src_len  = strlen(_source);
    const size_t new_size = (_destination->size + src_len);

    if (!d_string_internal_grow(_destination,
                                new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size,
             _source,
             src_len + 1);
    _destination->size = new_size;

    return 0;
}

/*
d_string_ncat_s
  Clamps `_count` to the source's size, grows, copies, and terminates
explicitly because the copied range may end before the source's terminator.
*/
int
d_string_ncat_s(
    struct d_string* restrict       _destination,
    const struct d_string* restrict _source,
    size_t                          _count
)
{
    if ( (!_destination) ||
         (!_source) )
    {
        return EINVAL;
    }

    const size_t append_len = (_count < _source->size)
        ? _count
        : _source->size;
    const size_t new_size   = (_destination->size + append_len);

    if (!d_string_internal_grow(_destination,
                                new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size,
             _source->text,
             append_len);
    _destination->size           = new_size;
    _destination->text[new_size] = '\0';

    return 0;
}

/*
d_string_ncat_cstr_s
  d_strnlen() clamps the length without scanning past `_count` bytes; the
terminator is written explicitly after the copy.
*/
int
d_string_ncat_cstr_s(
    struct d_string* restrict _destination,
    const char* restrict      _source,
    size_t                    _count
)
{
    if ( (_destination == NULL) ||
         (_source == NULL) )
    {
        return EINVAL;
    }

    const size_t append_len = d_strnlen(_source,
                                        _count);
    const size_t new_size   = _destination->size + append_len;

    if (!d_string_internal_grow(_destination,
                                new_size + 1))
    {
        return ERANGE;
    }

    d_memcpy(_destination->text + _destination->size,
             _source,
             append_len);
    _destination->size           = new_size;
    _destination->text[new_size] = '\0';

    return 0;
}

// copying: duplication
/*
d_string_dup
  Delegates to d_string_new_copy().
*/
struct d_string*
d_string_dup(
    const struct d_string* _string
)
{
    return d_string_new_copy(_string);
}

/*
d_string_ndup
  Clamps `_n` to the size and delegates to d_string_new_from_buffer().
*/
struct d_string*
d_string_ndup(
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
  Resolves `_start` with d_index_convert_safe(), clamps the length to the
characters that remain, and delegates to d_string_new_from_buffer().
*/
struct d_string*
d_string_substr(
    const struct d_string* _string,
    d_index                _start,
    size_t                 _length
)
{
    size_t start_pos = 0;

    if ( (!_string) ||
         (!d_index_convert_safe(_start,
                                _string->size,
                                &start_pos)) )
    {
        return NULL;
    }

    // clamp length to available characters
    size_t actual_len = _length;

    if (start_pos + actual_len > _string->size)
    {
        actual_len = (_string->size - start_pos);
    }

    return d_string_new_from_buffer(_string->text + start_pos,
                                    actual_len);
}

// comparison
/*
d_string_compare
  Settles the NULL cases first, then defers to d_strcmp_n(), which compares
lengths as well as content, so embedded NULs take part.
*/
int
d_string_compare(
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
  Settles the NULL cases first, measures the C string, and defers to
d_strcmp_n().
*/
int
d_string_compare_cstr(
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
  Clamps each side to `_n`, compares the common prefix with memcmp(), and
breaks a tie on length, so embedded NULs take part.
*/
int
d_string_ncmp(
    const struct d_string* _s1,
    const struct d_string* _s2,
    size_t                 _n
)
{
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

    const size_t len1    = (_n < _s1->size)
        ? _n
        : _s1->size;
    const size_t len2    = (_n < _s2->size)
        ? _n
        : _s2->size;
    const size_t min_len = (len1 < len2) ? len1 : len2;

    const int result = memcmp(_s1->text,
                              _s2->text,
                              min_len);

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
  Settles the NULL cases, then defers to strncmp(), which stops at the first
NUL.
*/
int
d_string_ncmp_cstr(
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

    return strncmp(_s1->text,
                   _s2,
                   _n);
}

/*
d_string_casecmp
  Settles the NULL cases, then defers to d_strcasecmp().
*/
int
d_string_casecmp(
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

    return d_strcasecmp(_s1->text,
                        _s2->text);
}

/*
d_string_casecmp_cstr
  Settles the NULL cases, then defers to d_strcasecmp().
*/
int
d_string_casecmp_cstr(
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

    return d_strcasecmp(_s1->text,
                        _s2);
}

/*
d_string_ncasecmp
  Settles the NULL cases, then defers to d_strncasecmp().
*/
int
d_string_ncasecmp(
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

    return d_strncasecmp(_s1->text,
                         _s2->text,
                         _n);
}

/*
d_string_ncasecmp_cstr
  Settles the NULL cases, then defers to d_strncasecmp().
*/
int
d_string_ncasecmp_cstr(
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

    return d_strncasecmp(_s1->text,
                         _s2,
                         _n);
}

/*
d_string_equals
  Delegates to d_string_compare().
*/
bool
d_string_equals(
    const struct d_string* _s1,
    const struct d_string* _s2
)
{
    return (d_string_compare(_s1,
                             _s2) == 0);
}

/*
d_string_equals_cstr
  Delegates to d_string_compare_cstr().
*/
bool
d_string_equals_cstr(
    const struct d_string* _s1,
    const char*            _s2
)
{
    return (d_string_compare_cstr(_s1,
                                  _s2) == 0);
}

/*
d_string_equals_ignore_case
  Delegates to d_string_casecmp().
*/
bool
d_string_equals_ignore_case(
    const struct d_string* _s1,
    const struct d_string* _s2
)
{
    return (d_string_casecmp(_s1,
                             _s2) == 0);
}

/*
d_string_equals_cstr_ignore_case
  Delegates to d_string_casecmp_cstr().
*/
bool
d_string_equals_cstr_ignore_case(
    const struct d_string* _s1,
    const char*            _s2
)
{
    return (d_string_casecmp_cstr(_s1,
                                  _s2) == 0);
}

// search: character search
/*
d_string_find_char
  Searches with strchr() and converts the hit to an index. Searching for '\0'
therefore finds the terminator rather than failing.
*/
d_index
d_string_find_char(
    const struct d_string* _string,
    char                   _c
)
{
    if ( (!_string) ||
         (!_string->text) )
    {
        return -1;
    }

    const char* p = strchr(_string->text,
                           _c);

    return (p)
        ? (d_index)(p - _string->text)
        : -1;
}

/*
d_string_find_char_from
  Resolves `_start` with d_index_convert_safe() and searches from there with
strchr(); the index is measured from the start of the string, not from
`_start`.
*/
d_index
d_string_find_char_from(
    const struct d_string* _string,
    char                   _c,
    d_index                _start
)
{
    size_t start_pos = 0;

    if ( (!_string)       ||
         (!_string->text) ||
         (!d_index_convert_safe(_start,
                                _string->size,
                                &start_pos)) )
    {
        return -1;
    }

    const char* p = strchr(_string->text + start_pos,
                           _c);

    return (p)
        ? (d_index)(p - _string->text)
        : -1;
}

/*
d_string_rfind_char
  Searches with strrchr() and converts the hit to an index.
*/
d_index
d_string_rfind_char(
    const struct d_string* _string,
    char                   _c
)
{
    if ( (!_string) ||
         (!_string->text) )
    {
        return -1;
    }

    const char* p = strrchr(_string->text,
                            _c);

    return (p)
        ? (d_index)(p - _string->text)
        : -1;
}

/*
d_string_chr
  Wraps strchr().
*/
char*
d_string_chr(
    const struct d_string* _string,
    int                    _c
)
{
    return ( (_string) &&
             (_string->text) )
        ? strchr(_string->text,
                 _c)
        : NULL;
}

/*
d_string_rchr
  Wraps strrchr().
*/
char*
d_string_rchr(
    const struct d_string* _string,
    int                    _c
)
{
    return ( (_string) &&
             (_string->text) )
        ? strrchr(_string->text,
                  _c)
        : NULL;
}

/*
d_string_chrnul
  Wraps d_strchrnul().
*/
char*
d_string_chrnul(
    const struct d_string* _string,
    int                    _c
)
{
    return ( (_string) &&
             (_string->text) )
        ? d_strchrnul(_string->text,
                      _c)
        : NULL;
}

// search: substring search
/*
d_string_find
  An empty needle matches at index 0 without searching; otherwise strstr()
does the work, so matching stops at the first NUL of either string.
*/
d_index
d_string_find(
    const struct d_string* _haystack,
    const struct d_string* _needle
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    if (_needle->size == 0)
    {
        return 0;
    }

    const char* p = strstr(_haystack->text,
                           _needle->text);

    if (p == NULL)
    {
        return -1;
    }

    return (d_index)(p - _haystack->text);
}

/*
d_string_find_cstr
  An empty needle matches at index 0 without searching; otherwise strstr()
does the work.
*/
d_index
d_string_find_cstr(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    if (*_needle == '\0')
    {
        return 0;
    }

    const char* p = strstr(_haystack->text,
                           _needle);

    if (p == NULL)
    {
        return -1;
    }

    return (d_index)(p - _haystack->text);
}

/*
d_string_find_from
  Resolves `_start` with d_index_convert_safe() and searches from there with
strstr(); the index is measured from the start of the string.
*/
d_index
d_string_find_from(
    const struct d_string* _haystack,
    const struct d_string* _needle,
    d_index                _start
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    size_t start_pos = 0;

    if (!d_index_convert_safe(_start,
                              _haystack->size,
                              &start_pos))
    {
        return -1;
    }

    const char* p = strstr(_haystack->text + start_pos,
                           _needle->text);

    if (p == NULL)
    {
        return -1;
    }

    return (d_index)(p - _haystack->text);
}

/*
d_string_find_cstr_from
  Resolves `_start` with d_index_convert_safe() and searches from there with
strstr(); the index is measured from the start of the string.
*/
d_index
d_string_find_cstr_from(
    const struct d_string* _haystack,
    const char*            _needle,
    d_index                _start
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    size_t start_pos = 0;

    if (!d_index_convert_safe(_start,
                              _haystack->size,
                              &start_pos))
    {
        return -1;
    }

    const char* p = strstr(_haystack->text + start_pos,
                           _needle);

    if (p == NULL)
    {
        return -1;
    }

    return (d_index)(p - _haystack->text);
}

/*
d_string_rfind
  Restarts strstr() one character past each hit and keeps the last one, so
overlapping occurrences are found. An empty needle short-circuits to the
length.
*/
d_index
d_string_rfind(
    const struct d_string* _haystack,
    const struct d_string* _needle
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    if (_needle->size == 0)
    {
        return (d_index)_haystack->size;
    }

    if (_needle->size > _haystack->size)
    {
        return -1;
    }

    d_index     last_pos     = -1;
    const char* search_start = _haystack->text;
    const char* p            = NULL;

    while ((p = strstr(search_start,
                       _needle->text)) != NULL)
    {
        last_pos     = (d_index)(p - _haystack->text);
        search_start = p + 1;
    }

    return last_pos;
}

/*
d_string_rfind_cstr
  Restarts strstr() one character past each hit and keeps the last one, so
overlapping occurrences are found. An empty needle short-circuits to the
length.
*/
d_index
d_string_rfind_cstr(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    if (*_needle == '\0')
    {
        return (d_index)_haystack->size;
    }

    d_index     last_pos     = -1;
    const char* search_start = _haystack->text;
    const char* p            = NULL;

    while ((p = strstr(search_start,
                       _needle)) != NULL)
    {
        last_pos     = (d_index)(p - _haystack->text);
        search_start = p + 1;
    }

    return last_pos;
}

/*
d_string_str
  Wraps strstr().
*/
char*
d_string_str(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    return ( (_haystack) &&
             (_needle) )
        ? strstr(_haystack->text,
                 _needle)
        : NULL;
}

// search: case-insensitive search
/*
d_string_casefind
  Wraps d_strcasestr() and converts the hit to an index.
*/
d_index
d_string_casefind(
    const struct d_string* _haystack,
    const struct d_string* _needle
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    char* p = d_strcasestr(_haystack->text,
                           _needle->text);

    if (p == NULL)
    {
        return -1;
    }

    return (d_index)(p - _haystack->text);
}

/*
d_string_casefind_cstr
  Wraps d_strcasestr() and converts the hit to an index.
*/
d_index
d_string_casefind_cstr(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    if ( (!_haystack) ||
         (!_needle) )
    {
        return -1;
    }

    char* p = d_strcasestr(_haystack->text,
                           _needle);

    if (p == NULL)
    {
        return -1;
    }

    return (d_index)(p - _haystack->text);
}

/*
d_string_casestr
  Wraps d_strcasestr().
*/
char*
d_string_casestr(
    const struct d_string* _haystack,
    const char*            _needle
)
{
    return ( (_haystack) &&
             (_needle) )
        ? d_strcasestr(_haystack->text,
                       _needle)
        : NULL;
}

// search: containment and spans
/*
d_string_contains
  Delegates to d_string_find().
*/
bool
d_string_contains(
    const struct d_string* _string,
    const struct d_string* _substr
)
{
    return (d_string_find(_string,
                          _substr) >= 0);
}

/*
d_string_contains_cstr
  Delegates to d_string_find_cstr().
*/
bool
d_string_contains_cstr(
    const struct d_string* _string,
    const char*            _substr
)
{
    return (d_string_find_cstr(_string,
                               _substr) >= 0);
}

/*
d_string_contains_char
  Delegates to d_string_find_char().
*/
bool
d_string_contains_char(
    const struct d_string* _string,
    char                   _c
)
{
    return (d_string_find_char(_string,
                               _c) >= 0);
}

/*
d_string_starts_with
  Checks the prefix fits before comparing, then compares with memcmp() over
the prefix's full size, so embedded NULs take part.
*/
bool
d_string_starts_with(
    const struct d_string* _string,
    const struct d_string* _prefix
)
{
    return ( (_string) &&
             (_prefix) &&
             (_prefix->size <= _string->size) )
        ? (memcmp(_string->text,
                  _prefix->text,
                  _prefix->size) == 0)
        : false;
}

/*
d_string_starts_with_cstr
  Measures the prefix, rejects one longer than the string, and compares with
memcmp().
*/
bool
d_string_starts_with_cstr(
    const struct d_string* _string,
    const char*            _prefix
)
{
    if ( (!_string) ||
         (!_prefix) )
    {
        return false;
    }

    const size_t prefix_len = strlen(_prefix);

    if (prefix_len > _string->size)
    {
        return false;
    }

    return (memcmp(_string->text,
                   _prefix,
                   prefix_len) == 0);
}

/*
d_string_ends_with
  Rejects a suffix longer than the string first, which keeps the offset from
underflowing, then compares the tail with memcmp().
*/
bool
d_string_ends_with(
    const struct d_string* _string,
    const struct d_string* _suffix
)
{
    if ( (_string == NULL) ||
         (_suffix == NULL) )
    {
        return false;
    }

    if (_suffix->size > _string->size)
    {
        return false;
    }

    const size_t offset = _string->size - _suffix->size;

    return (memcmp(_string->text + offset,
                   _suffix->text,
                   _suffix->size) == 0);
}

/*
d_string_ends_with_cstr
  Measures the suffix and rejects one longer than the string, which keeps the
offset from underflowing, then compares the tail with memcmp().
*/
bool
d_string_ends_with_cstr(
    const struct d_string* _string,
    const char*            _suffix
)
{
    if ( (!_string) ||
         (!_suffix) )
    {
        return false;
    }

    const size_t suffix_len = strlen(_suffix);

    if (suffix_len > _string->size)
    {
        return false;
    }

    const size_t offset = (_string->size - suffix_len);

    return (memcmp(_string->text + offset,
                   _suffix,
                   suffix_len) == 0);
}

/*
d_string_spn
  Wraps strspn().
*/
size_t
d_string_spn(
    const struct d_string* _string,
    const char*            _accept
)
{
    return ( (_string) &&
             (_accept) )
        ? strspn(_string->text,
                 _accept)
        : 0;
}

/*
d_string_cspn
  Wraps strcspn().
*/
size_t
d_string_cspn(
    const struct d_string* _string,
    const char*            _reject
)
{
    if ( (_string == NULL) ||
         (_reject == NULL) )
    {
        return 0;
    }

    return strcspn(_string->text,
                   _reject);
}

/*
d_string_pbrk
  Wraps strpbrk().
*/
char*
d_string_pbrk(
    const struct d_string* _string,
    const char*            _accept
)
{
    if ( (_string == NULL) ||
         (_accept == NULL) )
    {
        return NULL;
    }

    return strpbrk(_string->text,
                   _accept);
}

// modification: assignment
/*
d_string_assign
  Delegates to d_string_copy_s() and folds its error code into a bool.
*/
bool
d_string_assign(
    struct d_string*       _string,
    const struct d_string* _other
)
{
    if ( (!_string) ||
         (!_other) )
    {
        return false;
    }

    return (d_string_copy_s(_string,
                            _other) == 0);
}

/*
d_string_assign_cstr
  Delegates to d_string_copy_cstr_s() and folds its error code into a bool.
*/
bool
d_string_assign_cstr(
    struct d_string* _string,
    const char*      _cstr
)
{
    if ( (_string == NULL) ||
         (_cstr == NULL) )
    {
        return false;
    }

    return (d_string_copy_cstr_s(_string,
                                 _cstr) == 0);
}

/*
d_string_assign_buffer
  Grows first and copies only on success, so a failed assignment leaves the
string unchanged; the terminator is written explicitly because `_buffer` need
not have one.
*/
bool
d_string_assign_buffer(
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

    if (!d_string_internal_grow(_string,
                                _length + 1))
    {
        return false;
    }

    d_memcpy(_string->text,
             _buffer,
             _length);
    _string->text[_length] = '\0';
    _string->size          = _length;

    return true;
}

/*
d_string_assign_char
  Grows first, fills with d_memset(), and writes the terminator separately.
*/
bool
d_string_assign_char(
    struct d_string* _string,
    size_t           _count,
    char             _c
)
{
    if (_string == NULL)
    {
        return false;
    }

    if (!d_string_internal_grow(_string,
                                _count + 1))
    {
        return false;
    }

    d_memset(_string->text,
             _c,
             _count);
    _string->text[_count] = '\0';
    _string->size         = _count;

    return true;
}

// modification: append
/*
d_string_append
  Delegates to d_string_cat_s() and folds its error code into a bool.
*/
bool
d_string_append(
    struct d_string*       _string,
    const struct d_string* _other
)
{
    if ( (_string == NULL) ||
         (_other == NULL) )
    {
        return false;
    }

    return (d_string_cat_s(_string,
                           _other) == 0);
}

/*
d_string_append_cstr
  Delegates to d_string_cat_cstr_s() and folds its error code into a bool.
*/
bool
d_string_append_cstr(
    struct d_string* _string,
    const char*      _cstr
)
{
    if ( (_string == NULL) ||
         (_cstr == NULL) )
    {
        return false;
    }

    return (d_string_cat_cstr_s(_string,
                                _cstr) == 0);
}

/*
d_string_append_buffer
  Grows first, copies exactly `_length` bytes, and terminates explicitly
because `_buffer` need not have a terminator.
*/
bool
d_string_append_buffer(
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

    const size_t new_size = _string->size + _length;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    d_memcpy(_string->text + _string->size,
             _buffer,
             _length);
    _string->text[new_size] = '\0';
    _string->size           = new_size;

    return true;
}

/*
d_string_append_char
  Grows for the character and the terminator, then writes both.
*/
bool
d_string_append_char(
    struct d_string* _string,
    char             _c
)
{
    if (_string == NULL)
    {
        return false;
    }

    const size_t new_size = _string->size + 1;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    _string->text[_string->size] = _c;
    _string->text[new_size]      = '\0';
    _string->size                = new_size;

    return true;
}

/*
d_string_append_formatted
  Formats twice: a vsnprintf() dry run measures the output, then a va_copy()
of the arguments writes it straight after the existing text, terminator
included. Every early return ends both lists.
*/
bool
d_string_append_formatted(
    struct d_string* _string,
    const char*      _format,
    ...
)
{
    if ( (_string == NULL) ||
         (_format == NULL) )
    {
        return false;
    }

    va_list args;
    va_list args_copy;

    va_start(args,
             _format);
    va_copy(args_copy,
            args);

    const int len = vsnprintf(NULL,
                              0,
                              _format,
                              args);
    va_end(args);

    if (len < 0)
    {
        va_end(args_copy);

        return false;
    }

    const size_t new_size = _string->size + (size_t)len;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        va_end(args_copy);

        return false;
    }

    vsnprintf(_string->text + _string->size,
              (size_t)len + 1,
              _format,
              args_copy);
    va_end(args_copy);

    _string->size = new_size;

    return true;
}

// modification: prepend
/*
d_string_prepend
  Grows, shifts the existing text and terminator right with memmove(), then
copies the new text into the gap.
*/
bool
d_string_prepend(
    struct d_string*       _string,
    const struct d_string* _other
)
{
    if ( (_string == NULL) ||
         (_other == NULL) )
    {
        return false;
    }

    const size_t new_size = _string->size + _other->size;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    // shift existing content
    memmove(_string->text + _other->size,
            _string->text,
            _string->size + 1);

    // copy prepend content
    d_memcpy(_string->text,
             _other->text,
             _other->size);
    _string->size = new_size;

    return true;
}

/*
d_string_prepend_cstr
  Grows, shifts the existing text and terminator right with memmove(), then
copies the C string into the gap.
*/
bool
d_string_prepend_cstr(
    struct d_string* _string,
    const char*      _cstr
)
{
    if ( (_string == NULL) ||
         (_cstr == NULL) )
    {
        return false;
    }

    const size_t cstr_len = strlen(_cstr);
    const size_t new_size = _string->size + cstr_len;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    memmove(_string->text + cstr_len,
            _string->text,
            _string->size + 1);
    d_memcpy(_string->text,
             _cstr,
             cstr_len);
    _string->size = new_size;

    return true;
}

/*
d_string_prepend_char
  Grows, shifts the existing text and terminator right by one, then writes the
character at index 0.
*/
bool
d_string_prepend_char(
    struct d_string* _string,
    char             _c
)
{
    if (_string == NULL)
    {
        return false;
    }

    const size_t new_size = _string->size + 1;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    memmove(_string->text + 1,
            _string->text,
            _string->size + 1);
    _string->text[0] = _c;
    _string->size    = new_size;

    return true;
}

// modification: insert
/*
d_string_insert
  An index equal to the length is accepted as an append, since
d_index_convert_safe() would reject it. Otherwise the tail from the insertion
point, terminator included, is shifted with memmove() and the new text copied
into the gap.
*/
bool
d_string_insert(
    struct d_string*       _string,
    d_index                _index,
    const struct d_string* _other
)
{
    if ( (_string == NULL) ||
         (_other == NULL) )
    {
        return false;
    }

    // special case: insert at end
    if (_index == (d_index)_string->size)
    {
        return d_string_append(_string,
                               _other);
    }

    size_t pos = 0;

    if (!d_index_convert_safe(_index,
                              _string->size,
                              &pos))
    {
        return false;
    }

    const size_t new_size = _string->size + _other->size;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    // shift content after insertion point
    memmove(_string->text + pos + _other->size,
            _string->text + pos,
            _string->size - pos + 1);

    // insert new content
    d_memcpy(_string->text + pos,
             _other->text,
             _other->size);
    _string->size = new_size;

    return true;
}

/*
d_string_insert_cstr
  An index equal to the length is accepted as an append, since
d_index_convert_safe() would reject it. Otherwise the tail from the insertion
point, terminator included, is shifted with memmove() and the C string copied
into the gap.
*/
bool
d_string_insert_cstr(
    struct d_string* _string,
    d_index          _index,
    const char*      _cstr
)
{
    if ( (_string == NULL) ||
         (_cstr == NULL) )
    {
        return false;
    }

    // special case: insert at end
    if (_index == (d_index)_string->size)
    {
        return d_string_append_cstr(_string,
                                    _cstr);
    }

    size_t pos = 0;

    if (!d_index_convert_safe(_index,
                              _string->size,
                              &pos))
    {
        return false;
    }

    const size_t cstr_len = strlen(_cstr);
    const size_t new_size = _string->size + cstr_len;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    memmove(_string->text + pos + cstr_len,
            _string->text + pos,
            _string->size - pos + 1);

    d_memcpy(_string->text + pos,
             _cstr,
             cstr_len);
    _string->size = new_size;

    return true;
}

/*
d_string_insert_char
  An index equal to the length is accepted as an append, since
d_index_convert_safe() would reject it. Otherwise the tail from the insertion
point, terminator included, is shifted right by one before the character is
written.
*/
bool
d_string_insert_char(
    struct d_string* _string,
    d_index          _index,
    char             _c
)
{
    if (_string == NULL)
    {
        return false;
    }

    // special case: insert at end
    if (_index == (d_index)_string->size)
    {
        return d_string_append_char(_string,
                                    _c);
    }

    size_t pos = 0;

    if (!d_index_convert_safe(_index,
                              _string->size,
                              &pos))
    {
        return false;
    }

    const size_t new_size = _string->size + 1;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
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

// modification: erase and clear
/*
d_string_erase
  Clamps the count to the characters that remain, then closes the gap with a
memmove() that carries the terminator along.
*/
bool
d_string_erase(
    struct d_string* _string,
    d_index          _index,
    size_t           _count
)
{
    if (_string == NULL)
    {
        return false;
    }

    size_t pos = 0;

    if (!d_index_convert_safe(_index,
                              _string->size,
                              &pos))
    {
        return false;
    }

    // clamp count to available characters
    size_t actual_count = _count;

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
  Delegates to d_string_erase() with a count of 1.
*/
bool
d_string_erase_char(
    struct d_string* _string,
    d_index          _index
)
{
    return d_string_erase(_string,
                          _index,
                          1);
}

/*
d_string_clear
  Keeps the buffer and writes the terminator at index 0; a string whose buffer
was freed by d_string_free_contents() only has its size reset.
*/
void
d_string_clear(
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

// modification: replace
/*
d_string_replace
  Clamps the count, grows to the final size, shifts the tail after the
replaced range with memmove() (terminator included), then copies the
replacement into place. One shift serves both growing and shrinking
replacements.
*/
bool
d_string_replace(
    struct d_string*       _string,
    d_index                _index,
    size_t                 _count,
    const struct d_string* _replacement
)
{
    if ( (_string == NULL) ||
         (_replacement == NULL) )
    {
        return false;
    }

    size_t pos = 0;

    if (!d_index_convert_safe(_index,
                              _string->size,
                              &pos))
    {
        return false;
    }

    // clamp count
    size_t actual_count = _count;

    if (pos + actual_count > _string->size)
    {
        actual_count = _string->size - pos;
    }

    const size_t new_size = _string->size - actual_count + _replacement->size;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    // shift content after replacement region
    memmove(_string->text + pos + _replacement->size,
            _string->text + pos + actual_count,
            _string->size - pos - actual_count + 1);

    // copy replacement
    d_memcpy(_string->text + pos,
             _replacement->text,
             _replacement->size);
    _string->size = new_size;

    return true;
}

/*
d_string_replace_cstr
  Clamps the count, grows to the final size, shifts the tail after the
replaced range with memmove() (terminator included), then copies the
replacement into place. One shift serves both growing and shrinking
replacements.
*/
bool
d_string_replace_cstr(
    struct d_string* _string,
    d_index          _index,
    size_t           _count,
    const char*      _replacement
)
{
    if ( (_string == NULL) ||
         (_replacement == NULL) )
    {
        return false;
    }

    size_t pos = 0;

    if (!d_index_convert_safe(_index,
                              _string->size,
                              &pos))
    {
        return false;
    }

    size_t actual_count = _count;

    if (pos + actual_count > _string->size)
    {
        actual_count = _string->size - pos;
    }

    const size_t rep_len  = strlen(_replacement);
    const size_t new_size = _string->size - actual_count + rep_len;

    if (!d_string_internal_grow(_string,
                                new_size + 1))
    {
        return false;
    }

    memmove(_string->text + pos + rep_len,
            _string->text + pos + actual_count,
            _string->size - pos - actual_count + 1);

    d_memcpy(_string->text + pos,
             _replacement,
             rep_len);
    _string->size = new_size;

    return true;
}

/*
d_string_replace_all
  Rejects an empty `_old` here, then delegates to d_string_replace_all_cstr()
with the two texts.
*/
bool
d_string_replace_all(
    struct d_string*       _string,
    const struct d_string* _old,
    const struct d_string* _new
)
{
    if ( (_string == NULL)  ||
         (_old == NULL)     ||
         (_new == NULL)     ||
         (_old->size == 0) )
    {
        return false;
    }

    return d_string_replace_all_cstr(_string,
                                     _old->text,
                                     _new->text);
}

/*
d_string_replace_all_cstr
  Two passes: the first counts matches to size the result exactly, the second
copies the text between matches and the replacement into a temporary string.
The temporary's buffer is then adopted and only its struct freed, so the
result is never copied a second time. Matches are found left to right and do
not overlap.
*/
bool
d_string_replace_all_cstr(
    struct d_string* _string,
    const char*      _old,
    const char*      _new
)
{
    if ( (_string == NULL) ||
         (_old == NULL)    ||
         (_new == NULL) )
    {
        return false;
    }

    const size_t old_len = strlen(_old);

    if (old_len == 0)
    {
        return false;
    }

    const size_t new_len = strlen(_new);

    // count occurrences
    size_t count  = 0;
    char*  search = _string->text;
    char*  found  = NULL;

    while ((found = strstr(search,
                           _old)) != NULL)
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
    const size_t new_size = _string->size + (count * new_len) -
                            (count * old_len);

    // create temporary result (+1 for '\0')
    struct d_string* result = d_string_new_with_capacity(new_size + 1);

    if (result == NULL)
    {
        return false;
    }

    // build result
    char* read_ptr  = _string->text;
    char* write_ptr = result->text;

    while ((found = strstr(read_ptr,
                           _old)) != NULL)
    {
        const size_t before_len = (size_t)(found - read_ptr);

        // copy text before match
        d_memcpy(write_ptr,
                 read_ptr,
                 before_len);
        write_ptr += before_len;

        // copy replacement
        if (new_len > 0)
        {
            d_memcpy(write_ptr,
                     _new,
                     new_len);
            write_ptr += new_len;
        }

        read_ptr = found + old_len;
    }

    // copy remaining text (including the terminating '\0')
    {
        const size_t tail_len = strlen(read_ptr);

        d_memcpy(write_ptr,
                 read_ptr,
                 tail_len + 1);
        write_ptr += tail_len;
    }

    // optional sanity: ensure we produced exactly new_size chars
    // (write_ptr now points at the '\0' position)
    // d_assert((size_t)(write_ptr - result->text) == new_size);

    // swap contents
    free(_string->text);
    _string->text     = result->text;
    _string->size     = new_size;
    _string->capacity = result->capacity;

    // free result struct (but not its text, which we've taken)
    free(result);

    return true;
}

/*
d_string_replace_char
  Walks the text once, rewriting matches in place; the size never changes.
*/
bool
d_string_replace_char(
    struct d_string* _string,
    char             _old_char,
    char             _new_char
)
{
    if (_string == NULL)
    {
        return false;
    }

    for (size_t i = 0; i < _string->size; i++)
    {
        if (_string->text[i] == _old_char)
        {
            _string->text[i] = _new_char;
        }
    }

    return true;
}

// transformation: case conversion
/*
d_string_to_lower
  Delegates to d_strlwr() on the buffer in place.
*/
bool
d_string_to_lower(
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
  Delegates to d_strupr() on the buffer in place.
*/
bool
d_string_to_upper(
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
  Copies with d_string_new_copy() and converts the copy with
d_string_to_lower().
*/
struct d_string*
d_string_lower(
    const struct d_string* _string
)
{
    if (_string == NULL)
    {
        return NULL;
    }

    struct d_string* result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_to_lower(result);

    return result;
}

/*
d_string_upper
  Copies with d_string_new_copy() and converts the copy with
d_string_to_upper().
*/
struct d_string*
d_string_upper(
    const struct d_string* _string
)
{
    if (_string == NULL)
    {
        return NULL;
    }

    struct d_string* result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_to_upper(result);

    return result;
}

// transformation: reversal
/*
d_string_reverse
  Delegates to d_strrev() on the buffer in place.
*/
bool
d_string_reverse(
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
  Copies with d_string_new_copy() and reverses the copy with
d_string_reverse().
*/
struct d_string*
d_string_reversed(
    const struct d_string* _string
)
{
    if (_string == NULL)
    {
        return NULL;
    }

    struct d_string* result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_reverse(result);

    return result;
}

// transformation: trimming
/*
d_string_trim
  Scans inward from both ends; the backward scan stops at `start`, so no
character is examined twice. The surviving run is moved to the front with one
memmove(), and only when leading whitespace was found.
*/
bool
d_string_trim(
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    // find first non-whitespace
    size_t start = 0;

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
    size_t end = _string->size - 1;

    while ( (end > start) &&
            (isspace((unsigned char)_string->text[end])) )
    {
        end--;
    }

    const size_t new_size = end - start + 1;

    // shift content if needed
    if (start > 0)
    {
        memmove(_string->text,
                _string->text + start,
                new_size);
    }

    _string->text[new_size] = '\0';
    _string->size           = new_size;

    return true;
}

/*
d_string_trim_left
  Scans forward past the whitespace, then moves the rest of the text,
terminator included, to the front with one memmove().
*/
bool
d_string_trim_left(
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    size_t start = 0;

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
        const size_t new_size = _string->size - start;
        memmove(_string->text,
                _string->text + start,
                new_size + 1);
        _string->size = new_size;
    }

    return true;
}

/*
d_string_trim_right
  Scans backward past the whitespace and writes the terminator there; nothing
is moved.
*/
bool
d_string_trim_right(
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return false;
    }

    if (_string->size == 0)
    {
        return true;
    }

    size_t end = _string->size;

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
  Same shape as d_string_trim(), testing membership with strchr() on `_chars`
in place of isspace().
*/
bool
d_string_trim_chars(
    struct d_string* _string,
    const char*      _chars
)
{
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
    size_t start = 0;

    while ( (start < _string->size) &&
            (strchr(_chars,
                    _string->text[start]) != NULL) )
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
    size_t end = _string->size - 1;

    while ( (end > start) &&
            (strchr(_chars,
                    _string->text[end]) != NULL) )
    {
        end--;
    }

    const size_t new_size = end - start + 1;

    if (start > 0)
    {
        memmove(_string->text,
                _string->text + start,
                new_size);
    }

    _string->text[new_size] = '\0';
    _string->size           = new_size;

    return true;
}

/*
d_string_trimmed
  Copies with d_string_new_copy() and trims the copy with d_string_trim().
*/
struct d_string*
d_string_trimmed(
    const struct d_string* _string
)
{
    if (_string == NULL)
    {
        return NULL;
    }

    struct d_string* result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_trim(result);

    return result;
}

/*
d_string_trimmed_left
  Copies with d_string_new_copy() and trims the copy with
d_string_trim_left().
*/
struct d_string*
d_string_trimmed_left(
    const struct d_string* _string
)
{
    if (_string == NULL)
    {
        return NULL;
    }

    struct d_string* result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_trim_left(result);

    return result;
}

/*
d_string_trimmed_right
  Copies with d_string_new_copy() and trims the copy with
d_string_trim_right().
*/
struct d_string*
d_string_trimmed_right(
    const struct d_string* _string
)
{
    if (_string == NULL)
    {
        return NULL;
    }

    struct d_string* result = d_string_new_copy(_string);

    if (result == NULL)
    {
        return NULL;
    }

    d_string_trim_right(result);

    return result;
}

// transformation: tokenization
/*
d_string_tokenize
  Passes the buffer (or NULL, to continue) straight to d_strtok_r(), which
writes NULs into it; the size field is not updated.
*/
char*
d_string_tokenize(
    struct d_string* _string,
    const char*      _delim,
    char**           _saveptr
)
{
    if ( (_delim == NULL) ||
         (_saveptr == NULL) )
    {
        return NULL;
    }

    char* start = (_string != NULL)
        ? _string->text
        : NULL;

    return d_strtok_r(start,
                      _delim,
                      _saveptr);
}

/*
d_string_split
  Tokenizes a d_strdup() copy so `_string` is never modified, growing the
result array by doubling. On any allocation failure every token made so far is
freed, leaving nothing for the caller to release.
*/
size_t
d_string_split(
    const struct d_string*  _string,
    const char*             _delim,
    struct d_string***      _tokens
)
{
    if ( (_string == NULL) ||
         (_delim == NULL)  ||
         (_tokens == NULL) )
    {
        return 0;
    }

    *_tokens = NULL;

    // handle empty string: return single empty token
    if (_string->size == 0)
    {
        struct d_string** result = malloc(sizeof(struct d_string*));

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
    char* copy = d_strdup(_string->text);

    if (copy == NULL)
    {
        return 0;
    }

    // initial allocation
    size_t            capacity = 8;
    struct d_string** result   = malloc(capacity * sizeof(struct d_string*));

    if (result == NULL)
    {
        free(copy);

        return 0;
    }

    size_t count   = 0;
    char*  saveptr = NULL;
    char*  token   = d_strtok_r(copy,
                                _delim,
                                &saveptr);

    while (token != NULL)
    {
        // grow array if needed
        if (count >= capacity)
        {
            capacity *= 2;

            const size_t      new_bytes  = capacity * sizeof(struct d_string*);
            struct d_string** new_result = realloc(result,
                                                   new_bytes);

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
        token = d_strtok_r(NULL,
                           _delim,
                           &saveptr);
    }

    free(copy);

    *_tokens = result;

    return count;
}

/*
d_string_split_free
  Frees each token, then the array.
*/
void
d_string_split_free(
    struct d_string** _tokens,
    size_t            _count
)
{
    if (_tokens == NULL)
    {
        return;
    }

    for (size_t i = 0; i < _count; i++)
    {
        d_string_free(_tokens[i]);
    }

    free(_tokens);

    return;
}

// transformation: joining
/*
d_string_join
  Two passes: the first sums the lengths so the result is allocated once at
its final size, which is why the appends in the second pass cannot fail.
*/
struct d_string*
d_string_join(
    const struct d_string* const* _strings,
    size_t                        _count,
    const char*                   _delimiter
)
{
    if (_count == 0)
    {
        return d_string_new();
    }

    if ( (_strings == NULL) ||
         (_delimiter == NULL) )
    {
        return NULL;
    }

    const size_t delim_len = strlen(_delimiter);

    // calculate total length
    size_t total_len = 0;

    for (size_t i = 0; i < _count; i++)
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

    struct d_string* result = d_string_new_with_capacity(total_len + 1);

    if (result == NULL)
    {
        return NULL;
    }

    // build result
    for (size_t i = 0; i < _count; i++)
    {
        if (_strings[i] != NULL)
        {
            d_string_append(result,
                            _strings[i]);
        }

        if ( (i < _count - 1) &&
             (delim_len > 0) )
        {
            d_string_append_cstr(result,
                                 _delimiter);
        }
    }

    return result;
}

/*
d_string_join_cstr
  Two passes: the first sums the lengths so the result is allocated once at
its final size, which is why the appends in the second pass cannot fail.
*/
struct d_string*
d_string_join_cstr(
    const char* const* _strings,
    size_t             _count,
    const char*        _delimiter
)
{
    if (_count == 0)
    {
        return d_string_new();
    }

    if ( (_strings == NULL) ||
         (_delimiter == NULL) )
    {
        return NULL;
    }

    const size_t delim_len = strlen(_delimiter);

    // calculate total length
    size_t total_len = 0;

    for (size_t i = 0; i < _count; i++)
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

    struct d_string* result = d_string_new_with_capacity(total_len + 1);

    if (result == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < _count; i++)
    {
        if (_strings[i] != NULL)
        {
            d_string_append_cstr(result,
                                 _strings[i]);
        }

        if ( (i < _count - 1) &&
             (delim_len > 0) )
        {
            d_string_append_cstr(result,
                                 _delimiter);
        }
    }

    return result;
}

/*
d_string_concat
  Walks the arguments twice, restarting the list with va_start(): the first
pass sums the sizes and rejects NULLs before anything is allocated, so the
appends in the second pass cannot fail.
*/
struct d_string*
d_string_concat(
    size_t _count,
    ...
)
{
    if (_count == 0)
    {
        return d_string_new();
    }

    // first pass: calculate total length and check for NULL
    va_list args;
    size_t  total_len = 0;

    va_start(args,
             _count);

    for (size_t i = 0; i < _count; i++)
    {
        const struct d_string* str = va_arg(args,
                                            const struct d_string*);

        if (str == NULL)
        {
            va_end(args);

            return NULL;
        }

        total_len += str->size;
    }

    va_end(args);

    struct d_string* result = d_string_new_with_capacity(total_len + 1);

    if (result == NULL)
    {
        return NULL;
    }

    // second pass: concatenate
    va_start(args,
             _count);

    for (size_t i = 0; i < _count; i++)
    {
        const struct d_string* str = va_arg(args,
                                            const struct d_string*);

        d_string_append(result,
                        str);
    }

    va_end(args);

    return result;
}

// utilities: validation
/*
d_string_is_valid
  d_str_is_valid() checks the buffer and rejects NULs before `size`; the
terminator is checked separately at `size` itself.
*/
bool
d_string_is_valid(
    const struct d_string* _string
)
{
    return (_string)
        ? ( (d_str_is_valid(_string->text,
                            _string->size)) &&
            (_string->text[_string->size] == '\0') )
        : false;
}

/*
d_string_is_ascii
  Delegates to d_str_is_ascii() over the stored size.
*/
bool
d_string_is_ascii(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_ascii(_string->text,
                         _string->size)
        : false;
}

/*
d_string_is_numeric
  Delegates to d_str_is_numeric() over the stored size.
*/
bool
d_string_is_numeric(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_numeric(_string->text,
                           _string->size)
        : false;
}

/*
d_string_is_alpha
  Delegates to d_str_is_alpha() over the stored size.
*/
bool
d_string_is_alpha(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_alpha(_string->text,
                         _string->size)
        : false;
}

/*
d_string_is_alnum
  Delegates to d_str_is_alnum() over the stored size.
*/
bool
d_string_is_alnum(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_alnum(_string->text,
                         _string->size)
        : false;
}

/*
d_string_is_whitespace
  Delegates to d_str_is_whitespace() over the stored size.
*/
bool
d_string_is_whitespace(
    const struct d_string* _string
)
{
    return (_string)
        ? d_str_is_whitespace(_string->text,
                              _string->size)
        : false;
}

// utilities: counting and hashing
/*
d_string_count_char
  Walks the stored size rather than stopping at a NUL, so embedded NULs can be
counted.
*/
size_t
d_string_count_char(
    const struct d_string* _string,
    char                   _c
)
{
    if ( (!_string) ||
         (!_string->text) )
    {
        return 0;
    }

    size_t count = 0;

    for (size_t i = 0; i < _string->size; i++)
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
  Restarts strstr() just past each match, so occurrences are counted without
overlap.
*/
size_t
d_string_count_substr(
    const struct d_string* _string,
    const char*            _substr
)
{
    if ( (_string == NULL) ||
         (_substr == NULL) )
    {
        return 0;
    }

    const size_t substr_len = strlen(_substr);

    if (substr_len == 0)
    {
        return 0;
    }

    size_t      count  = 0;
    const char* search = _string->text;
    const char* found  = NULL;

    while ((found = strstr(search,
                           _substr)) != NULL)
    {
        count++;
        search = found + substr_len;
    }

    return count;
}

/*
d_string_hash
  djb2 (hash * 33 + c from a seed of 5381) over the stored size, reading
characters as unsigned char so the result is the same whatever the signedness
of char.
*/
size_t
d_string_hash(
    const struct d_string* _string
)
{
    if ( (_string == NULL) ||
         (_string->text == NULL) )
    {
        return 0;
    }

    size_t hash = 5381;

    for (size_t i = 0; i < _string->size; i++)
    {
        hash = ((hash << 5) + hash) + (unsigned char)_string->text[i];
    }

    return hash;
}

// utilities: error strings
/*
d_string_error
  Formats into a 256-byte stack buffer with d_strerror_r(), falling back to a
fixed "Unknown error" when that fails.
*/
struct d_string*
d_string_error(
    int _errnum
)
{
    char buf[256] = {0};

    if (d_strerror_r(_errnum,
                     buf,
                     sizeof(buf)) != 0)
    {
        return d_string_new_from_cstr("Unknown error");
    }

    return d_string_new_from_cstr(buf);
}

/*
d_string_error_r
  Formats into a 256-byte stack buffer with d_strerror_r(), then assigns it;
an error from d_strerror_r() is passed through unchanged.
*/
int
d_string_error_r(
    int              _errnum,
    struct d_string* _string
)
{
    if (_string == NULL)
    {
        return EINVAL;
    }

    char      buf[256] = {0};
    const int result   = d_strerror_r(_errnum,
                                      buf,
                                      sizeof(buf));

    if (result != 0)
    {
        return result;
    }

    if (!d_string_assign_cstr(_string,
                              buf))
    {
        return EINVAL;
    }

    return 0;
}

// utilities: formatted strings
/*
d_string_printf
  Collects the arguments and delegates to d_string_vprintf().
*/
struct d_string*
d_string_printf(
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    struct d_string* result = d_string_vprintf(_format,
                                               args);
    va_end(args);

    return result;
}

/*
d_string_vprintf
  Formats twice: a dry run over `_args` measures the output, then a va_copy()
made beforehand writes it into a buffer of exactly that size.
*/
struct d_string*
d_string_vprintf(
    const char* _format,
    va_list     _args
)
{
    if (_format == NULL)
    {
        return NULL;
    }

    va_list args_copy;

    va_copy(args_copy,
            _args);

    const int len = vsnprintf(NULL,
                              0,
                              _format,
                              _args);

    if (len < 0)
    {
        va_end(args_copy);

        return NULL;
    }

    struct d_string* result = d_string_new_with_capacity((size_t)len + 1);

    if (result == NULL)
    {
        va_end(args_copy);

        return NULL;
    }

    vsnprintf(result->text,
              (size_t)len + 1,
              _format,
              args_copy);
    va_end(args_copy);

    result->size = (size_t)len;

    return result;
}

/*
d_string_sprintf
  Formats twice: a vsnprintf() dry run measures the output, then a va_copy()
of the arguments writes it over the existing text from index 0. The buffer is
only grown, never shrunk.
*/
int
d_string_sprintf(
    struct d_string* _string,
    const char*      _format,
    ...
)
{
    if ( (_string == NULL) ||
         (_format == NULL) )
    {
        return -1;
    }

    va_list args;
    va_list args_copy;

    va_start(args,
             _format);
    va_copy(args_copy,
            args);

    const int len = vsnprintf(NULL,
                              0,
                              _format,
                              args);
    va_end(args);

    if (len < 0)
    {
        va_end(args_copy);

        return -1;
    }

    if (!d_string_internal_grow(_string,
                                (size_t)len + 1))
    {
        va_end(args_copy);

        return -1;
    }

    vsnprintf(_string->text,
              (size_t)len + 1,
              _format,
              args_copy);
    va_end(args_copy);

    _string->size = (size_t)len;

    return len;
}

// lifecycle: destruction
/*
d_string_free
  Frees the text before the struct that points to it.
*/
void
d_string_free(
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
  Frees the text and resets the fields, leaving an empty string with no buffer
that the growing functions can reuse.
*/
void
d_string_free_contents(
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
