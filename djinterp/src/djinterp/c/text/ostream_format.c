/*******************************************************************************
* djinterp [c]                                                  ostream_format.c
*
*   The locale-independent double and printf, onto a stream.
*
*
* path:      /src/djinterp/c/text/ostream_format.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/ostream_format.h"
// std
#include <errno.h>   // errno, EINVAL, EILSEQ, ENOMEM, ERANGE
#include <float.h>   // DBL_MAX
#include <locale.h>  // localeconv, struct lconv
#include <stdarg.h>  // va_list, va_copy, va_start, va_end
#include <stdio.h>   // snprintf, vsnprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memmove, strlen, strstr
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"
                                                // d_ostream_reserve
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


// D_INTERNAL_FORMAT_PRECISION_MAX
//   constant: the largest precision a double is formatted with.
#define D_INTERNAL_FORMAT_PRECISION_MAX  64

// D_INTERNAL_FORMAT_DOUBLE_MAX
//   constant: the buffer a double is formatted into. The longest is %.64f of
// DBL_MAX: a sign, 309 digits, a point and 64 decimals, 375 bytes.
#define D_INTERNAL_FORMAT_DOUBLE_MAX     512

// D_INTERNAL_FORMAT_STACK
//   constant: what printf tries when the window has no room, before it
// allocates.
#define D_INTERNAL_FORMAT_STACK          256

// D_INTERNAL_FORMAT_WINDOW_MIN
//   constant: the least room in the window worth formatting into directly; a
// smaller window is skipped for the stack buffer rather than measured twice.
#define D_INTERNAL_FORMAT_WINDOW_MIN     32


/**
 * @brief Rewrites the locale's decimal point to '.'.
 *
 * @param[in,out] _text    NUL-terminated formatted text.
 * @param[in]     _length  its length.
 * @return the new length.
 *
 * printf's %f, %e and %g take nothing else from the locale -- no grouping
 * without the ' flag, and the digits are always ASCII -- so this one
 * substitution gives the C locale's text. The point can be more than one byte
 * (Pashto's is two), so the text after it moves up.
 */
static size_t
d_internal_format_c_point(
    char*  _text,
    size_t _length
)
{
    const struct lconv* const conv  = localeconv();
    const char* const         point = ( (conv != NULL) &&
                                        (conv->decimal_point != NULL) &&
                                        (conv->decimal_point[0] != '\0') )
                                          ? conv->decimal_point
                                          : ".";
    const size_t              width = strlen(point);

    if ( (width == 1u) &&
         (point[0] == '.') )
    {
        return _length;
    }

    char* const found = strstr(_text, point);

    if (found == NULL)
    {
        return _length;
    }

    *found = '.';
    memmove(found + 1, found + width, strlen(found + width) + 1u);

    return _length - (width - 1u);
}

/**
 * @brief snprintf with a literal format per style, so the compiler checks
 *        each.
 *
 * @param[out] _out        the buffer.
 * @param[in]  _capacity   its size.
 * @param[in]  _value      the value; finite.
 * @param[in]  _style      'g', 'f' or 'e'.
 * @param[in]  _precision  the precision.
 * @return what snprintf returned.
 */
static int
d_internal_format_double(
    char*  _out,
    size_t _capacity,
    double _value,
    char   _style,
    int    _precision
)
{
    switch (_style)
    {
        case 'f':
        {
            return snprintf(_out, _capacity, "%.*f", _precision, _value);
        }
        case 'e':
        {
            return snprintf(_out, _capacity, "%.*e", _precision, _value);
        }
        default:
        {
            return snprintf(_out, _capacity, "%.*g", _precision, _value);
        }
    }
}

/*
d_ostream_write_double
  Not-a-number and the infinities are spelled here, because C libraries spell
them differently; they are told apart by comparison rather than by isnan and
isinf, whose MinGW macros narrow a double to float on one of their paths. A
finite value is formatted on the stack: snprintf's cost dwarfs the copy, and
the stack holds the longest one.
*/
int32_t
d_ostream_write_double(
    struct dj_ostream* _os,
    double             _value,
    char               _style,
    int32_t            _precision
)
{
    // the readiness check
    if (d_ostream_write_slow(_os, NULL, 0u) == 0)
    {
        return 0;
    }

    if ( (_style != 'g') &&
         (_style != 'f') &&
         (_style != 'e') )
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
    }

    // not-a-number is the one value unequal to itself
    if (_value != _value)
    {
        return d_ostream_write(_os, "nan", 3u);
    }

    if ( (_value > DBL_MAX) ||
         (_value < -DBL_MAX) )
    {
        return (_value < 0.0) ? d_ostream_write(_os, "-inf", 4u)
                              : d_ostream_write(_os, "inf", 3u);
    }

    char      text[D_INTERNAL_FORMAT_DOUBLE_MAX];
    const int precision = (_precision < 0)
                              ? 6
                              : ( (_precision > D_INTERNAL_FORMAT_PRECISION_MAX)
                                      ? D_INTERNAL_FORMAT_PRECISION_MAX
                                      : (int)_precision );
    const int length    = d_internal_format_double(text,
                                                   sizeof(text),
                                                   _value,
                                                   _style,
                                                   precision);

    if ( (length < 0) ||
         ((size_t)length >= sizeof(text)) )
    {
        return d_ostream_fail(_os, D_OSTREAM_FAILED, ERANGE);
    }

    return d_ostream_write(_os,
                           text,
                           d_internal_format_c_point(text, (size_t)length));
}

/**
 * @brief Formats the whole of a printf's output into room the stream gives,
 *        or into the heap when it gives none.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _format  the format.
 * @param[in]     _args    the values; used once.
 * @param[in]     _length  the length the first pass measured.
 * @return as d_ostream_write().
 *
 * The second pass of an output too large for the first: room for the length
 * and its terminator, then the commit of the length alone, or, from a
 * transport too small to give that room, an allocation exactly as large.
 */
static int32_t
d_internal_format_long_vprintf(
    struct dj_ostream* _os,
    const char*        _format,
    va_list            _args,
    size_t             _length
)
{
    size_t size = 0u;

    // the length and its terminator; a length vsnprintf measured always fits
    if (d_ckd_add(&size, _length, 1u))
    {
        return d_ostream_fail(_os, D_OSTREAM_FAILED, ENOMEM);
    }

    char* const room = d_ostream_reserve(_os, size);

    if (room != NULL)
    {
        (void)vsnprintf(room, size, _format, _args);

        return d_ostream_commit(_os, _length);
    }

    char* const heap = (char*)malloc(size);

    if (heap == NULL)
    {
        return d_ostream_fail(_os, D_OSTREAM_FAILED, ENOMEM);
    }

    (void)vsnprintf(heap, size, _format, _args);

    const int32_t result = d_ostream_write(_os, heap, _length);

    free(heap);

    return result;
}

/*
d_ostream_vprintf
  The first pass formats through a copy of the arguments into the window when
it has room worth trying -- so most calls are formatted in place and only
committed -- or into a stack buffer. Text that did not fit was measured by
that pass, and the second pass formats it, from the original arguments, into
room exactly that large. A window too small to try is never written to, so
nothing past its end is touched; vsnprintf writes its terminator inside the
room it was given, past what is committed.
*/
int32_t
d_ostream_vprintf(
    struct dj_ostream* _os,
    const char*        _format,
    va_list            _args
)
{
    // the readiness check
    if (d_ostream_write_slow(_os, NULL, 0u) == 0)
    {
        return 0;
    }

    if (_format == NULL)
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
    }

    const size_t room     = (size_t)(_os->end - _os->cur);
    const int    in_place = (room >= D_INTERNAL_FORMAT_WINDOW_MIN) ? 1 : 0;
    char         stack[D_INTERNAL_FORMAT_STACK];
    char* const  target   = (in_place != 0) ? (char*)_os->cur : stack;
    const size_t capacity = (in_place != 0) ? room : sizeof(stack);
    va_list      copy;

    va_copy(copy, _args);

    const int length = vsnprintf(target, capacity, _format, copy);

    va_end(copy);

    if (length < 0)
    {
        return d_ostream_fail(_os,
                              D_OSTREAM_FAILED,
                              (errno != 0) ? (int32_t)errno : EILSEQ);
    }

    // it fitted: commit it in place, or write it from the stack
    if ((size_t)length < capacity)
    {
        return (in_place != 0)
                   ? d_ostream_commit(_os, (size_t)length)
                   : d_ostream_write(_os, stack, (size_t)length);
    }

    return d_internal_format_long_vprintf(_os,
                                          _format,
                                          _args,
                                          (size_t)length);
}

/*
d_ostream_printf
  Collects the variadic arguments and defers to d_ostream_vprintf.
*/
int32_t
d_ostream_printf(
    struct dj_ostream* _os,
    const char*        _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int32_t result = d_ostream_vprintf(_os, _format, args);

    va_end(args);

    return result;
}
