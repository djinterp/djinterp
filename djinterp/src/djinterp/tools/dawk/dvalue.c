/******************************************************************************
* djinterp [dawk]                                                     dvalue.c
*
*   Definitions for the non-inline declarations in dvalue.h.
*     One scanner serves both numeric questions. d_awk_looks_numeric asks
* whether it consumed the whole string, which decides STRNUM; d_awk_text_to_
* number takes whatever prefix it consumed, which is what awk's arithmetic
* does with "3abc". Writing it by hand rather than leaning on strtod is
* deliberate: strtod accepts hexadecimal, infinity and NaN spellings, none of
* which POSIX awk treats as numeric.
*
*
* path:      /src/djinterp/tools/dawk/dvalue.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dvalue.h"  // corresponding header
// std
#include <math.h>    // isnan, isinf, floor
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, strtod
#include <string.h>  // memcmp, memcpy, memset


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Scalars
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_value
//   struct: one cell.  `tag` decides comparison; `has_text` and `has_number`
//   are cache validity flags and say nothing about the tag, so a NUMBER may
//   carry a cached rendering and a STRING a cached numeric value.
struct d_awk_value
{
    enum d_awk_value_tag tag;
    double               number;
    char*                text;
    size_t               length;
    size_t               capacity;
    bool                 has_number;
    bool                 has_text;
    void*                object;
    d_awk_fn_extern_text resolve;
    void*                user;
    struct d_awk_array*  array;
};

// 1.2    Arrays
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_entry
//   struct: one array element.  A deleted entry keeps its slot with a NULL
//   key, so an index handed out as an iteration cursor stays valid.
struct d_internal_entry
{
    char*               key;
    size_t              length;
    size_t              hash;
    size_t              next;
    struct d_awk_value* value;
};

// 1.2.2
// d_awk_array
//   struct: chained hash over an insertion-ordered entry vector.  Iteration
//   walks the vector, so a cursor is an index and deletion during a walk is
//   safe.
struct d_awk_array
{
    struct d_internal_entry* entries;
    size_t                   entry_count;
    size_t                   entry_capacity;
    size_t*                  buckets;
    size_t                   bucket_count;
    size_t                   live;
};

// 1.2.3
// D_INTERNAL_NONE
//   constant: sentinel for an absent bucket head or chain link.
#define D_INTERNAL_NONE ((size_t)-1)


//==============================================================================
// 2.  NUMERIC SCANNING
//==============================================================================


/*
d_internal_scan
  Consumes the longest prefix of a byte string having awk's numeric shape.
NOTE:
  The accepted shape is optional blanks, an optional sign, then either digits
  with an optional fraction or a fraction alone, then an optional exponent.
  Hexadecimal, `inf` and `nan` are rejected on purpose: strtod accepts all
  three and POSIX awk treats none of them as numeric.

Parameter(s):
  _text:         the bytes to scan.
  _length:       the number of bytes available.
  _out_number:   receives the value; zero when nothing was consumed.
  _out_consumed: receives the number of bytes consumed, blanks included.
Return:
  A boolean value corresponding to either:
  - true, if a number was consumed, or
  - false, otherwise.
*/
static bool
d_internal_scan(
    const char* _text,
    size_t      _length,
    double*     _out_number,
    size_t*     _out_consumed
)
{
    *_out_number   = 0.0;
    *_out_consumed = 0;

    // parameter validation first
    if ((!_text) || (_length == 0))
    {
        return false;
    }

    size_t at = 0;

    // leading blanks belong to the prefix but do not make it numeric
    while ((at < _length) && ((_text[at] == ' ') || (_text[at] == '\t')))
    {
        at++;
    }

    const size_t body = at;

    // an optional sign precedes the digits
    if ((at < _length) && ((_text[at] == '+') || (_text[at] == '-')))
    {
        at++;
    }

    size_t digits = 0;

    // the integer part
    while ((at < _length) && (_text[at] >= '0') && (_text[at] <= '9'))
    {
        at++;
        digits++;
    }

    // an optional fraction, which may carry the only digits present
    if ((at < _length) && (_text[at] == '.'))
    {
        at++;

        while ((at < _length) && (_text[at] >= '0') && (_text[at] <= '9'))
        {
            at++;
            digits++;
        }
    }

    // a mantissa with no digits at all is not a number
    if (digits == 0)
    {
        return false;
    }

    const size_t mantissa = at;

    // an exponent counts only when it carries at least one digit
    if ((at < _length) && ((_text[at] == 'e') || (_text[at] == 'E')))
    {
        size_t probe = at + 1u;

        // the exponent may itself be signed
        if ((probe < _length) && ((_text[probe] == '+') ||
                                  (_text[probe] == '-')))
        {
            probe++;
        }

        size_t exponent = 0;

        while ((probe < _length) && (_text[probe] >= '0') &&
               (_text[probe] <= '9'))
        {
            probe++;
            exponent++;
        }

        // an `e` with no digits after it is not part of the number
        if (exponent > 0)
        {
            at = probe;
        }
        else
        {
            at = mantissa;
        }
    }

    char   buffer[512];
    size_t span = at - body;

    // an absurdly long literal is clamped rather than rejected
    if (span >= sizeof(buffer))
    {
        span = sizeof(buffer) - 1u;
    }

    memcpy(buffer, &_text[body], span);
    buffer[span] = '\0';

    *_out_number   = strtod(buffer, NULL);
    *_out_consumed = at;

    return true;
}


/*
d_awk_looks_numeric
  Reports whether a whole byte string has awk's numeric shape.

Parameter(s):
  _text:       the bytes to test.
  _length:     the number of bytes.
  _out_number: receives the value when the test passes; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the whole string is a numeric string, or
  - false, otherwise.
*/
bool
d_awk_looks_numeric(
    const char* _text,
    size_t      _length,
    double*     _out_number
)
{
    double number   = 0.0;
    size_t consumed = 0;

    // the shape must be present before the extent can matter
    if (!d_internal_scan(_text, _length, &number, &consumed))
    {
        return false;
    }

    // trailing blanks are permitted, anything else is not
    while ((consumed < _length) && ((_text[consumed] == ' ') ||
                                    (_text[consumed] == '\t')))
    {
        consumed++;
    }

    // the string is numeric only when the scan reached its end
    if (consumed != _length)
    {
        return false;
    }

    // report the value when the caller asked for it
    if (_out_number)
    {
        *_out_number = number;
    }

    return true;
}


/*
d_awk_text_to_number
  Converts a byte string to a number using its longest numeric prefix.

Parameter(s):
  _text:   the bytes to convert.
  _length: the number of bytes.
Return:
  The value of the prefix, or zero when there is none.
*/
double
d_awk_text_to_number(
    const char* _text,
    size_t      _length
)
{
    double number   = 0.0;
    size_t consumed = 0;

    (void)d_internal_scan(_text, _length, &number, &consumed);

    return number;
}


/*
d_awk_number_to_text
  Renders a number into a caller-supplied buffer.
NOTE:
  POSIX requires an integral value to convert as an integer whatever CONVFMT
  says, so the format applies only to values with a fractional part.

Parameter(s):
  _number:   the value to render.
  _format:   the format to use for non-integral values; NULL selects CONVFMT.
  _buffer:   the destination.
  _capacity: the size of the destination.
Return:
  The number of bytes written, excluding the terminator.
*/
size_t
d_awk_number_to_text(
    double      _number,
    const char* _format,
    char*       _buffer,
    size_t      _capacity
)
{
    // parameter validation first
    if ((!_buffer) || (_capacity == 0))
    {
        return 0;
    }

    int written = 0;

    // the exceptional values have fixed spellings rather than a format
    if (isnan(_number))
    {
        written = snprintf(_buffer, _capacity, "nan");
    }
    else if (isinf(_number))
    {
        written = snprintf(_buffer,
                           _capacity,
                           (_number < 0.0) ? "-inf" : "inf");
    }
    else if ( (_number == floor(_number))              &&
              (_number >= -9223372036854775296.0)      &&
              (_number <= 9223372036854775296.0) )
    {
        // POSIX renders an integral value as if by %d, and that holds past
        // 2^53 where a double can no longer represent every integer: 1e17 is
        // still integral and prints in full.  The bound is the largest double
        // that converts to long long without undefined behaviour, not the
        // largest exactly representable integer.
        written = snprintf(_buffer, _capacity, "%lld", (long long)_number);
    }
    else
    {
        written = snprintf(_buffer,
                           _capacity,
                           (_format ? _format : D_AWK_CONVFMT_DEFAULT),
                           _number);
    }

    // a truncated or failed rendering reports what actually fits
    if (written < 0)
    {
        _buffer[0] = '\0';
        return 0;
    }

    return ((size_t)written < _capacity) ? (size_t)written : (_capacity - 1u);
}


//==============================================================================
// 3.  SCALARS
//==============================================================================


/*
d_internal_store_text
  Replaces a value's cached text, growing the buffer when needed.

Parameter(s):
  _value:  the value to modify.
  _text:   the bytes to store.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the text was stored, or
  - false, if the buffer could not be grown.
*/
static bool
d_internal_store_text(
    struct d_awk_value* _value,
    const char*         _text,
    size_t              _length
)
{
    // grow the buffer when the text and its terminator do not fit
    if (_value->capacity < (_length + 1u))
    {
        char* grown = realloc(_value->text, _length + 1u);

        // report the failure rather than truncating
        if (!grown)
        {
            return false;
        }

        _value->text     = grown;
        _value->capacity = _length + 1u;
    }

    // a zero-length assignment still needs a terminated buffer
    if (_length > 0)
    {
        memcpy(_value->text, _text, _length);
    }

    _value->text[_length] = '\0';
    _value->length        = _length;
    _value->has_text      = true;

    return true;
}


/*
d_awk_value_new
  Allocates an uninitialised value.

Parameter(s):
  none.
Return:
  The new value, or NULL on failure.
*/
struct d_awk_value*
d_awk_value_new(void)
{
    return calloc(1, sizeof(struct d_awk_value));
}


/*
d_awk_value_free
  Releases a value and its cached text.
NOTE:
  An array reached through a value is not released here, since assigning an
  array to a value does not transfer ownership of it.

Parameter(s):
  _value: the value to release; may be NULL.
Return:
  none.
*/
void
d_awk_value_free(
    struct d_awk_value* _value
)
{
    if (_value)
    {
        free(_value->text);
        free(_value);
    }

    return;
}


/*
d_awk_value_set_uninit
  Resets a value to the uninitialised state.

Parameter(s):
  _value: the value to reset.
Return:
  none.
*/
void
d_awk_value_set_uninit(
    struct d_awk_value* _value
)
{
    if (_value)
    {
        _value->tag        = D_AWK_VAL_UNINIT;
        _value->number     = 0.0;
        _value->length     = 0;
        _value->has_number = false;
        _value->has_text   = false;
        _value->object     = NULL;
        _value->resolve    = NULL;
        _value->array      = NULL;
    }

    return;
}


/*
d_awk_value_set_number
  Assigns a numeric value.

Parameter(s):
  _value:  the value to assign to.
  _number: the number to assign.
Return:
  none.
*/
void
d_awk_value_set_number(
    struct d_awk_value* _value,
    double              _number
)
{
    if (_value)
    {
        d_awk_value_set_uninit(_value);
        _value->tag        = D_AWK_VAL_NUMBER;
        _value->number     = _number;
        _value->has_number = true;
    }

    return;
}


/*
d_awk_value_set_string
  Assigns text that is never treated as numeric.

Parameter(s):
  _value:  the value to assign to.
  _text:   the bytes to assign.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the assignment succeeded, or
  - false, otherwise.
*/
bool
d_awk_value_set_string(
    struct d_awk_value* _value,
    const char*         _text,
    size_t              _length
)
{
    // parameter validation first
    if ((!_value) || ((!_text) && (_length > 0)))
    {
        return false;
    }

    d_awk_value_set_uninit(_value);

    // report a failure to hold the text rather than leaving a partial value
    if (!d_internal_store_text(_value, _text, _length))
    {
        return false;
    }

    _value->tag = D_AWK_VAL_STRING;

    return true;
}


/*
d_awk_value_set_input
  Assigns input-derived text, applying the numeric string test.
CAUTION:
  Use this only for text that came from outside the program: a field, a
  getline result, an ARGV or ENVIRON element, a split() result, FILENAME.  A
  string constant assigned through here would compare numerically, which is
  precisely the bug this pair of entry points exists to prevent.

Parameter(s):
  _value:  the value to assign to.
  _text:   the bytes to assign.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the assignment succeeded, or
  - false, otherwise.
*/
bool
d_awk_value_set_input(
    struct d_awk_value* _value,
    const char*         _text,
    size_t              _length
)
{
    // the text is stored first, whatever the test then decides
    if (!d_awk_value_set_string(_value, _text, _length))
    {
        return false;
    }

    double number = 0.0;

    // text shaped like a number compares as one
    if (d_awk_looks_numeric(_text, _length, &number))
    {
        _value->tag        = D_AWK_VAL_STRNUM;
        _value->number     = number;
        _value->has_number = true;
    }

    return true;
}


/*
d_awk_value_set_extern
  Assigns an opaque host object together with the callback yielding its text.

Parameter(s):
  _value:  the value to assign to.
  _object: the host object.
  _text:   the callback yielding the object's text.
  _user:   opaque pointer passed back to the callback.
Return:
  none.
*/
void
d_awk_value_set_extern(
    struct d_awk_value*  _value,
    void*                _object,
    d_awk_fn_extern_text _text,
    void*                _user
)
{
    if (_value)
    {
        d_awk_value_set_uninit(_value);
        _value->tag     = D_AWK_VAL_EXTERN;
        _value->object  = _object;
        _value->resolve = _text;
        _value->user    = _user;
    }

    return;
}


/*
d_awk_value_set_array
  Makes a value refer to an array.

Parameter(s):
  _value: the value to assign to.
  _array: the array to refer to; not owned by the value.
Return:
  A boolean value corresponding to either:
  - true, if the assignment succeeded, or
  - false, otherwise.
*/
bool
d_awk_value_set_array(
    struct d_awk_value* _value,
    struct d_awk_array* _array
)
{
    // parameter validation first
    if ((!_value) || (!_array))
    {
        return false;
    }

    d_awk_value_set_uninit(_value);
    _value->tag   = D_AWK_VAL_ARRAY;
    _value->array = _array;

    return true;
}


/*
d_awk_value_copy
  Copies one value onto another.
NOTE:
  An extern value is copied by reference, since the host owns the object.

Parameter(s):
  _target: the value to overwrite.
  _source: the value to copy.
Return:
  A boolean value corresponding to either:
  - true, if the copy succeeded, or
  - false, otherwise.
*/
bool
d_awk_value_copy(
    struct d_awk_value*       _target,
    const struct d_awk_value* _source
)
{
    // parameter validation first
    if ((!_target) || (!_source))
    {
        return false;
    }

    // a self-copy would free the buffer it is about to read
    if (_target == _source)
    {
        return true;
    }

    const enum d_awk_value_tag tag = _source->tag;

    d_awk_value_set_uninit(_target);

    // the cached text travels with the value when there is one
    if (_source->has_text)
    {
        // report a failure to hold the text rather than leaving a partial copy
        if (!d_internal_store_text(_target, _source->text, _source->length))
        {
            return false;
        }
    }

    _target->tag        = tag;
    _target->number     = _source->number;
    _target->has_number = _source->has_number;
    _target->object     = _source->object;
    _target->resolve    = _source->resolve;
    _target->user       = _source->user;
    _target->array      = _source->array;

    return true;
}


/*
d_awk_value_tag_of
  Returns the tag of a value.

Parameter(s):
  _value: the value to inspect.
Return:
  The tag, or D_AWK_VAL_UNINIT when the value is NULL.
*/
enum d_awk_value_tag
d_awk_value_tag_of(
    const struct d_awk_value* _value
)
{
    return _value ? _value->tag : D_AWK_VAL_UNINIT;
}


/*
d_awk_value_array_of
  Returns the array a value refers to.

Parameter(s):
  _value: the value to inspect.
Return:
  The array, or NULL when the value does not refer to one.
*/
struct d_awk_array*
d_awk_value_array_of(
    const struct d_awk_value* _value
)
{
    // parameter validation first
    if ((!_value) || (_value->tag != D_AWK_VAL_ARRAY))
    {
        return NULL;
    }

    return _value->array;
}


/*
d_internal_effective_tag
  Resolves an extern value to the tag it behaves as.
NOTE:
  An extern value is treated as though it came from input, so a node whose
  text is "10" compares numerically against 10.  That is what makes a tree
  walk behave like a flat record walk for the same expression.

Parameter(s):
  _value: the value to resolve.
Return:
  The tag governing conversion and comparison.
*/
static enum d_awk_value_tag
d_internal_effective_tag(
    struct d_awk_value* _value
)
{
    // only an extern value needs resolving
    if (_value->tag != D_AWK_VAL_EXTERN)
    {
        return _value->tag;
    }

    size_t      length = 0;
    const char* text   = _value->resolve
                       ? _value->resolve(_value->object,
                                         _value->user,
                                         &length)
                       : NULL;

    // an unresolvable extern behaves as the empty string
    if (!text)
    {
        return D_AWK_VAL_STRING;
    }

    return d_awk_looks_numeric(text, length, NULL)
         ? D_AWK_VAL_STRNUM
         : D_AWK_VAL_STRING;
}


/*
d_awk_value_text
  Returns the text of a value, rendering and caching it when necessary.
CAUTION:
  The returned buffer belongs to the value and is invalidated by the next
  assignment to it.

Parameter(s):
  _value:      the value to render.
  _format:     the format for a non-integral number; NULL selects CONVFMT.
  _out_length: receives the length; may be NULL.
Return:
  The text, never NULL.
*/
const char*
d_awk_value_text(
    struct d_awk_value* _value,
    const char*         _format,
    size_t*             _out_length
)
{
    static const char empty[] = "";

    // parameter validation first
    if (!_value)
    {
        // report the empty length when the caller asked for it
        if (_out_length)
        {
            *_out_length = 0;
        }

        return empty;
    }

    // an extern value defers to its host for text
    if (_value->tag == D_AWK_VAL_EXTERN)
    {
        size_t      length = 0;
        const char* text   = _value->resolve
                           ? _value->resolve(_value->object,
                                             _value->user,
                                             &length)
                           : NULL;

        // report the resolved text when the host supplied any
        if (_out_length)
        {
            *_out_length = text ? length : 0;
        }

        return text ? text : empty;
    }

    // a number is rendered on first use and then cached
    if ((!_value->has_text) && (_value->tag == D_AWK_VAL_NUMBER))
    {
        char         buffer[64];
        const size_t written = d_awk_number_to_text(_value->number,
                                                    _format,
                                                    buffer,
                                                    sizeof(buffer));

        // report the empty string when the rendering could not be held
        if (!d_internal_store_text(_value, buffer, written))
        {
            // report the empty length when the caller asked for it
            if (_out_length)
            {
                *_out_length = 0;
            }

            return empty;
        }
    }

    // an uninitialised value is the empty string
    if (!_value->has_text)
    {
        // report the empty length when the caller asked for it
        if (_out_length)
        {
            *_out_length = 0;
        }

        return empty;
    }

    // report the length when the caller asked for it
    if (_out_length)
    {
        *_out_length = _value->length;
    }

    return _value->text;
}


/*
d_awk_value_number
  Returns the numeric value of a value.

Parameter(s):
  _value: the value to convert.
Return:
  The number, which is zero for text carrying no numeric prefix.
*/
double
d_awk_value_number(
    struct d_awk_value* _value
)
{
    // parameter validation first
    if (!_value)
    {
        return 0.0;
    }

    // a cached number needs no reconversion
    if (_value->has_number)
    {
        return _value->number;
    }

    size_t            length = 0;
    const char* const text   = d_awk_value_text(_value, NULL, &length);

    _value->number = d_awk_text_to_number(text, length);

    // an extern value is not cached, since its text belongs to the host
    if (_value->tag != D_AWK_VAL_EXTERN)
    {
        _value->has_number = true;
    }

    return _value->number;
}


/*
d_awk_value_is_true
  Reports the truth of a value in a boolean context.
NOTE:
  A numeric value is true when non-zero and a textual one when non-empty, so
  the field "0" read from input is false while the string constant "0" is
  true.  The tag decides, exactly as it does for comparison.

Parameter(s):
  _value: the value to test.
Return:
  A boolean value corresponding to either:
  - true, if the value is true in awk's sense, or
  - false, otherwise.
*/
bool
d_awk_value_is_true(
    struct d_awk_value* _value
)
{
    // parameter validation first
    if (!_value)
    {
        return false;
    }

    const enum d_awk_value_tag tag = d_internal_effective_tag(_value);

    // an uninitialised value is false in both senses
    if (tag == D_AWK_VAL_UNINIT)
    {
        return false;
    }

    // a numeric tag is judged by its value
    if ((tag == D_AWK_VAL_NUMBER) || (tag == D_AWK_VAL_STRNUM))
    {
        return (d_awk_value_number(_value) != 0.0);
    }

    // an array in a boolean context is a program error, reported as false
    if (tag == D_AWK_VAL_ARRAY)
    {
        return false;
    }

    size_t length = 0;

    (void)d_awk_value_text(_value, NULL, &length);

    return (length > 0);
}


/*
d_awk_value_compare
  Compares two values under awk's rules.
NOTE:
  The comparison is numeric when both operands are numeric, a numeric string,
  or uninitialised, and textual otherwise.  So a field holding "10" equals the
  number 10, while the string constant "10" does not equal 10.5 - 0.5 by text
  even though it does by value.

Parameter(s):
  _left:   the left operand.
  _right:  the right operand.
  _format: the format used to render a number for a textual comparison.
Return:
  A negative value, zero, or a positive value as the left operand orders
  before, equal to, or after the right.
*/
int
d_awk_value_compare(
    struct d_awk_value* _left,
    struct d_awk_value* _right,
    const char*         _format
)
{
    // parameter validation first
    if ((!_left) || (!_right))
    {
        return (_left == _right) ? 0 : ((_left) ? 1 : -1);
    }

    const enum d_awk_value_tag left  = d_internal_effective_tag(_left);
    const enum d_awk_value_tag right = d_internal_effective_tag(_right);

    const bool left_numeric  = ( (left == D_AWK_VAL_NUMBER)  ||
                                 (left == D_AWK_VAL_STRNUM)  ||
                                 (left == D_AWK_VAL_UNINIT) );
    const bool right_numeric = ( (right == D_AWK_VAL_NUMBER) ||
                                 (right == D_AWK_VAL_STRNUM) ||
                                 (right == D_AWK_VAL_UNINIT) );

    // both sides numeric means a numeric comparison
    if ((left_numeric) && (right_numeric))
    {
        const double a = d_awk_value_number(_left);
        const double b = d_awk_value_number(_right);

        // an unordered pair compares equal rather than trapping
        if ((a != a) || (b != b))
        {
            return 0;
        }

        return (a < b) ? -1 : ((a > b) ? 1 : 0);
    }

    size_t            left_length  = 0;
    size_t            right_length = 0;
    const char* const left_text    = d_awk_value_text(_left,
                                                      _format,
                                                      &left_length);
    const char* const right_text   = d_awk_value_text(_right,
                                                      _format,
                                                      &right_length);

    const size_t shared = (left_length < right_length)
                        ? left_length
                        : right_length;
    const int    order  = (shared > 0)
                        ? memcmp(left_text, right_text, shared)
                        : 0;

    // a difference within the shared prefix settles the order
    if (order != 0)
    {
        return (order < 0) ? -1 : 1;
    }

    // otherwise the shorter string orders first
    if (left_length != right_length)
    {
        return (left_length < right_length) ? -1 : 1;
    }

    return 0;
}


//==============================================================================
// 4.  ASSOCIATIVE ARRAYS
//==============================================================================


/*
d_internal_hash
  Computes the FNV-1a hash of a key.

Parameter(s):
  _key:    the key bytes.
  _length: the number of bytes.
Return:
  The hash.
*/
static size_t
d_internal_hash(
    const char* _key,
    size_t      _length
)
{
    size_t hash = (size_t)1469598103934665603ULL;

    for (size_t at = 0; at < _length; ++at)
    {
        hash ^= (size_t)(unsigned char)_key[at];
        hash *= (size_t)1099511628211ULL;
    }

    return hash;
}


/*
d_internal_rehash
  Rebuilds the bucket index at a new size.

Parameter(s):
  _array: the array to rebuild.
  _count: the new bucket count; must be a power of two.
Return:
  A boolean value corresponding to either:
  - true, if the index was rebuilt, or
  - false, otherwise.
*/
static bool
d_internal_rehash(
    struct d_awk_array* _array,
    size_t              _count
)
{
    size_t* buckets = malloc(_count * sizeof(*buckets));

    // report the failure rather than leaving a half-built index
    if (!buckets)
    {
        return false;
    }

    // an empty bucket holds the absent sentinel
    for (size_t at = 0; at < _count; ++at)
    {
        buckets[at] = D_INTERNAL_NONE;
    }

    free(_array->buckets);
    _array->buckets      = buckets;
    _array->bucket_count = _count;

    // relink every live entry into its new bucket
    for (size_t at = 0; at < _array->entry_count; ++at)
    {
        struct d_internal_entry* entry = &_array->entries[at];

        // a deleted entry keeps its slot but leaves the index
        if (!entry->key)
        {
            continue;
        }

        const size_t bucket = entry->hash & (_count - 1u);

        entry->next      = buckets[bucket];
        buckets[bucket]  = at;
    }

    return true;
}


/*
d_awk_array_new
  Allocates an empty array.

Parameter(s):
  none.
Return:
  The new array, or NULL on failure.
*/
struct d_awk_array*
d_awk_array_new(void)
{
    struct d_awk_array* array = calloc(1, sizeof(*array));

    // abandon the allocation when the handle could not be held
    if (!array)
    {
        return NULL;
    }

    // report the failure rather than returning an array with no index
    if (!d_internal_rehash(array, 16u))
    {
        free(array);
        return NULL;
    }

    return array;
}


/*
d_awk_array_clear
  Removes every element, keeping the array itself.

Parameter(s):
  _array: the array to empty.
Return:
  none.
*/
void
d_awk_array_clear(
    struct d_awk_array* _array
)
{
    if (_array)
    {
        // each entry owns its key and its value
        for (size_t at = 0; at < _array->entry_count; ++at)
        {
            free(_array->entries[at].key);
            d_awk_value_free(_array->entries[at].value);
        }

        _array->entry_count = 0;
        _array->live        = 0;

        // an empty bucket holds the absent sentinel
        for (size_t at = 0; at < _array->bucket_count; ++at)
        {
            _array->buckets[at] = D_INTERNAL_NONE;
        }
    }

    return;
}


/*
d_awk_array_free
  Releases an array and everything in it.

Parameter(s):
  _array: the array to release; may be NULL.
Return:
  none.
*/
void
d_awk_array_free(
    struct d_awk_array* _array
)
{
    if (_array)
    {
        d_awk_array_clear(_array);
        free(_array->entries);
        free(_array->buckets);
        free(_array);
    }

    return;
}


/*
d_awk_array_count
  Returns the number of live elements.

Parameter(s):
  _array: the array to measure.
Return:
  The element count, or zero when the array is NULL.
*/
size_t
d_awk_array_count(
    const struct d_awk_array* _array
)
{
    return _array ? _array->live : 0;
}


/*
d_awk_array_find
  Locates an element without creating it.
CAUTION:
  This is the `in` operator.  Referencing a subscript in any other context
  creates the element, which is what d_awk_array_lookup does; using the wrong
  one silently changes what a program observes.

Parameter(s):
  _array:  the array to search.
  _key:    the key bytes.
  _length: the number of bytes.
Return:
  The element, or NULL when it is absent.
*/
struct d_awk_value*
d_awk_array_find(
    const struct d_awk_array* _array,
    const char*               _key,
    size_t                    _length
)
{
    // parameter validation first
    if ((!_array) || ((!_key) && (_length > 0)))
    {
        return NULL;
    }

    const size_t hash   = d_internal_hash(_key, _length);
    size_t       cursor = _array->buckets[hash & (_array->bucket_count - 1u)];

    // walk the chain until the key is found or it ends
    while (cursor != D_INTERNAL_NONE)
    {
        const struct d_internal_entry* entry = &_array->entries[cursor];

        if ( (entry->key)                                        &&
             (entry->hash == hash)                               &&
             (entry->length == _length)                          &&
             ((_length == 0) ||
              (memcmp(entry->key, _key, _length) == 0)) )
        {
            return entry->value;
        }

        cursor = entry->next;
    }

    return NULL;
}


/*
d_awk_array_lookup
  Locates an element, creating it uninitialised when absent.

Parameter(s):
  _array:  the array to search.
  _key:    the key bytes.
  _length: the number of bytes.
Return:
  The element, or NULL when it could not be created.
*/
struct d_awk_value*
d_awk_array_lookup(
    struct d_awk_array* _array,
    const char*         _key,
    size_t              _length
)
{
    struct d_awk_value* existing = d_awk_array_find(_array, _key, _length);

    // an existing element is returned as it stands
    if (existing)
    {
        return existing;
    }

    // parameter validation first
    if ((!_array) || ((!_key) && (_length > 0)))
    {
        return NULL;
    }

    // grow the entry vector when it is full
    if (_array->entry_count == _array->entry_capacity)
    {
        const size_t capacity = (_array->entry_capacity == 0)
                              ? 8u
                              : (_array->entry_capacity * 2u);

        struct d_internal_entry* grown =
            realloc(_array->entries, capacity * sizeof(*grown));

        // report the failure rather than creating a partial element
        if (!grown)
        {
            return NULL;
        }

        _array->entries        = grown;
        _array->entry_capacity = capacity;
    }

    char* key = malloc(_length + 1u);

    // report the failure rather than creating a keyless element
    if (!key)
    {
        return NULL;
    }

    // a zero-length key still needs a terminated buffer
    if (_length > 0)
    {
        memcpy(key, _key, _length);
    }

    key[_length] = '\0';

    struct d_awk_value* value = d_awk_value_new();

    // report the failure rather than creating a valueless element
    if (!value)
    {
        free(key);
        return NULL;
    }

    const size_t hash   = d_internal_hash(_key, _length);
    const size_t bucket = hash & (_array->bucket_count - 1u);
    const size_t index  = _array->entry_count++;

    _array->entries[index].key     = key;
    _array->entries[index].length  = _length;
    _array->entries[index].hash    = hash;
    _array->entries[index].value   = value;
    _array->entries[index].next    = _array->buckets[bucket];
    _array->buckets[bucket]        = index;
    _array->live++;

    // keep the chains short by doubling once the index is three-quarters full
    if ((_array->live * 4u) > (_array->bucket_count * 3u))
    {
        (void)d_internal_rehash(_array, _array->bucket_count * 2u);
    }

    return value;
}


/*
d_awk_array_delete
  Removes one element.

Parameter(s):
  _array:  the array to modify.
  _key:    the key bytes.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if an element was removed, or
  - false, if none was present.
*/
bool
d_awk_array_delete(
    struct d_awk_array* _array,
    const char*         _key,
    size_t              _length
)
{
    // parameter validation first
    if ((!_array) || ((!_key) && (_length > 0)))
    {
        return false;
    }

    const size_t hash    = d_internal_hash(_key, _length);
    const size_t bucket  = hash & (_array->bucket_count - 1u);
    size_t       cursor  = _array->buckets[bucket];
    size_t       trailer = D_INTERNAL_NONE;

    // walk the chain, remembering the predecessor so it can be relinked
    while (cursor != D_INTERNAL_NONE)
    {
        struct d_internal_entry* entry = &_array->entries[cursor];

        if ( (entry->key)                                        &&
             (entry->hash == hash)                               &&
             (entry->length == _length)                          &&
             ((_length == 0) ||
              (memcmp(entry->key, _key, _length) == 0)) )
        {
            // unlink from the head or from the predecessor
            if (trailer == D_INTERNAL_NONE)
            {
                _array->buckets[bucket] = entry->next;
            }
            else
            {
                _array->entries[trailer].next = entry->next;
            }

            free(entry->key);
            d_awk_value_free(entry->value);

            entry->key   = NULL;
            entry->value = NULL;
            entry->next  = D_INTERNAL_NONE;
            _array->live--;

            return true;
        }

        trailer = cursor;
        cursor  = entry->next;
    }

    return false;
}


/*
d_awk_array_next
  Advances an iteration over the live elements.

Parameter(s):
  _array:      the array to walk.
  _cursor:     the walk position; start at zero.
  _out_key:    receives the key; may be NULL.
  _out_length: receives the key length; may be NULL.
  _out_value:  receives the element; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if an element was produced, or
  - false, at the end of the walk.
*/
bool
d_awk_array_next(
    const struct d_awk_array* _array,
    size_t*                   _cursor,
    const char**              _out_key,
    size_t*                   _out_length,
    struct d_awk_value**      _out_value
)
{
    // parameter validation first
    if ((!_array) || (!_cursor))
    {
        return false;
    }

    // skip the slots left behind by deleted elements
    while (*_cursor < _array->entry_count)
    {
        const struct d_internal_entry* entry = &_array->entries[*_cursor];

        (*_cursor)++;

        // a deleted entry keeps its slot but yields nothing
        if (!entry->key)
        {
            continue;
        }

        // report each part the caller asked for
        if (_out_key)
        {
            *_out_key = entry->key;
        }

        if (_out_length)
        {
            *_out_length = entry->length;
        }

        if (_out_value)
        {
            *_out_value = entry->value;
        }

        return true;
    }

    return false;
}


/*
d_awk_subscript_join
  Joins the parts of a multi-dimensional subscript with SUBSEP.

Parameter(s):
  _buffer:   the destination.
  _capacity: the size of the destination.
  _parts:    the part strings.
  _lengths:  the length of each part.
  _count:    the number of parts.
  _subsep:   the separator; NULL selects the POSIX default.
Return:
  The number of bytes the joined key needs, excluding the terminator.  A
  result at or above `_capacity` means the buffer was too small and the
  contents are truncated.
*/
size_t
d_awk_subscript_join(
    char*              _buffer,
    size_t             _capacity,
    const char* const* _parts,
    const size_t*      _lengths,
    size_t             _count,
    const char*        _subsep
)
{
    // parameter validation first
    if ((!_parts) || (!_lengths) || (_count == 0))
    {
        // terminate the buffer even when there is nothing to join
        if ((_buffer) && (_capacity > 0))
        {
            _buffer[0] = '\0';
        }

        return 0;
    }

    const char* const separator = _subsep ? _subsep : D_AWK_SUBSEP_DEFAULT;
    const size_t      span      = strlen(separator);
    size_t            needed    = 0;

    // copy each part, preceded by a separator after the first
    for (size_t part = 0; part < _count; ++part)
    {
        // the separator joins parts rather than leading them
        if (part > 0)
        {
            for (size_t at = 0; at < span; ++at)
            {
                // write only while the destination has room
                if ((_buffer) && ((needed + 1u) < _capacity))
                {
                    _buffer[needed] = separator[at];
                }

                needed++;
            }
        }

        for (size_t at = 0; at < _lengths[part]; ++at)
        {
            // write only while the destination has room
            if ((_buffer) && ((needed + 1u) < _capacity))
            {
                _buffer[needed] = _parts[part][at];
            }

            needed++;
        }
    }

    // terminate whatever fitted
    if ((_buffer) && (_capacity > 0))
    {
        _buffer[(needed < _capacity) ? needed : (_capacity - 1u)] = '\0';
    }

    return needed;
}
