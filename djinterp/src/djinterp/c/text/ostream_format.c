/*******************************************************************************
* djinterp [c]                                                  ostream_format.c
*
*   The locale-independent double, the double in a caller's locale, and
* printf, onto a stream.
*
*
* path:      /src/djinterp/c/text/ostream_format.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.09
*******************************************************************************/
// enable POSIX.1-2008 (newlocale, uselocale, freelocale) under a strict C
// compiler; a feature-test macro must precede every include, so it sits ahead
// of the corresponding header, as in c/modules/net/tcp/tcp.c
#if ( (!defined(_WIN32)) &&                                                    \
      (!defined(_WIN64)) )
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif  // _POSIX_C_SOURCE
#endif

#include "../../../../inc/djinterp/c/text/ostream_format.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, EILSEQ, ENOENT, ENOMEM, ERANGE
#include <float.h>   // DBL_MAX
#include <locale.h>  // localeconv, struct lconv; newlocale, uselocale,
                     // freelocale or _create_locale, _free_locale
#include <stdarg.h>  // va_list, va_copy, va_start, va_end
#include <stdio.h>   // snprintf, vsnprintf, _snprintf_s_l
#include <stdlib.h>  // malloc, free, _TRUNCATE
#include <string.h>  // memmove, strcmp, strlen, strstr
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // reserve
#include "../../../../inc/djinterp/config/c/text/cfg_ostream_format.h"  // knob
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


// D_INTERNAL_OSTREAM_FORMAT_LOCALE
//   resolved: the locale mechanism in effect -- a D_CFG_OSTREAM_FORMAT_LOCALE_*
// selector, never AUTO. POSIX is keyed on what <locale.h> declared here
// (LC_NUMERIC_MASK comes with newlocale); WINDOWS on the Microsoft CRT.
#if (D_CFG_NORM(D_CFG_OSTREAM_FORMAT_LOCALE) !=                               \
     D_CFG_OSTREAM_FORMAT_LOCALE_AUTO)
    #define D_INTERNAL_OSTREAM_FORMAT_LOCALE                                   \
        D_CFG_NORM(D_CFG_OSTREAM_FORMAT_LOCALE)
#elif defined(_MSC_VER)
    #define D_INTERNAL_OSTREAM_FORMAT_LOCALE                                   \
        D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS
#elif defined(LC_NUMERIC_MASK)
    #define D_INTERNAL_OSTREAM_FORMAT_LOCALE                                   \
        D_CFG_OSTREAM_FORMAT_LOCALE_POSIX
#else
    #define D_INTERNAL_OSTREAM_FORMAT_LOCALE                                   \
        D_CFG_OSTREAM_FORMAT_LOCALE_REWRITE
#endif

#if ( (D_INTERNAL_OSTREAM_FORMAT_LOCALE ==                                     \
       D_CFG_OSTREAM_FORMAT_LOCALE_POSIX) &&                                   \
      (!defined(LC_NUMERIC_MASK)) )
    #error "D_CFG_OSTREAM_FORMAT_LOCALE is POSIX; <locale.h> has no newlocale"
#endif

#if ( (D_INTERNAL_OSTREAM_FORMAT_LOCALE ==                                     \
       D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS) &&                                 \
      (!defined(_MSC_VER)) )
    #error "D_CFG_OSTREAM_FORMAT_LOCALE is WINDOWS, the Microsoft CRT's"
#endif

// D_INTERNAL_FORMAT_OBJECTS
//   resolved: 1 where the mechanism makes locale objects.
#if (D_INTERNAL_OSTREAM_FORMAT_LOCALE == D_CFG_OSTREAM_FORMAT_LOCALE_REWRITE)
    #define D_INTERNAL_FORMAT_OBJECTS  0
#else
    #define D_INTERNAL_FORMAT_OBJECTS  1
#endif

// D_INTERNAL_FORMAT_SHARED
//   resolved: 1 where the C locale object can be made once and published
// without a lock: the compilers' atomic builtins. Elsewhere it is made for
// each call that needs it.
#if ( (D_INTERNAL_FORMAT_OBJECTS == 1) &&                                      \
      ( (defined(__GNUC__)) ||                                                 \
        (defined(__clang__)) ) )
    #define D_INTERNAL_FORMAT_SHARED  1
#elif ( (D_INTERNAL_FORMAT_OBJECTS == 1) &&                                    \
        (defined(_MSC_VER)) )
    #include <intrin.h>  // _InterlockedCompareExchangePointer
    #define D_INTERNAL_FORMAT_SHARED  1
#else
    #define D_INTERNAL_FORMAT_SHARED  0
#endif

// D_INTERNAL_FORMAT_ENOTSUP
//   constant: the errno for a named locale the build cannot make.
#if defined(ENOTSUP)
    #define D_INTERNAL_FORMAT_ENOTSUP  ENOTSUP
#else
    #define D_INTERNAL_FORMAT_ENOTSUP  EINVAL
#endif


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

#if (D_INTERNAL_FORMAT_OBJECTS == 1)

/**
 * @brief Makes the platform's numeric locale object for a name.
 *
 * @param[in] _name  the locale's name.
 * @return the object, or NULL with errno set.
 */
static void*
d_internal_format_locale_make(
    const char* _name
)
{
#if (D_INTERNAL_OSTREAM_FORMAT_LOCALE == D_CFG_OSTREAM_FORMAT_LOCALE_POSIX)
    const locale_t made = newlocale(LC_NUMERIC_MASK, _name, (locale_t)0);

    return (made == (locale_t)0) ? NULL : (void*)made;
#else
    return (void*)_create_locale(LC_NUMERIC, _name);
#endif
}

/**
 * @brief Releases a locale object d_internal_format_locale_make made.
 *
 * @param[in] _handle  the object; not NULL.
 */
static void
d_internal_format_locale_free(
    void* _handle
)
{
#if (D_INTERNAL_OSTREAM_FORMAT_LOCALE == D_CFG_OSTREAM_FORMAT_LOCALE_POSIX)
    freelocale((locale_t)_handle);
#else
    _free_locale((_locale_t)_handle);
#endif

    return;
}

#endif  // D_INTERNAL_FORMAT_OBJECTS

/**
 * @brief Formats a double, in the program's locale or in a locale object.
 *
 * @param[out] _out        the buffer.
 * @param[in]  _capacity   its size.
 * @param[in]  _value      the value; finite.
 * @param[in]  _style      'g', 'f' or 'e'.
 * @param[in]  _precision  the precision.
 * @param[in]  _handle     a locale object, or NULL for the program's locale.
 * @return what snprintf returned; negative when the locale could not be
 *         used, with errno set.
 *
 * A literal format per style, so the compiler checks each. Under POSIX the
 * object is this thread's locale for the one call and the thread's own is
 * put back; under the Microsoft CRT the object goes to _snprintf_s_l.
 */
static int
d_internal_format_double(
    char*  _out,
    size_t _capacity,
    double _value,
    char   _style,
    int    _precision,
    void*  _handle
)
{
#if (D_INTERNAL_OSTREAM_FORMAT_LOCALE == D_CFG_OSTREAM_FORMAT_LOCALE_POSIX)
    locale_t previous = (locale_t)0;

    if (_handle != NULL)
    {
        previous = uselocale((locale_t)_handle);

        if (previous == (locale_t)0)
        {
            return -1;
        }
    }
#elif (D_INTERNAL_OSTREAM_FORMAT_LOCALE == D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS)
    if (_handle != NULL)
    {
        const _locale_t loc = (_locale_t)_handle;

        switch (_style)
        {
            case 'f':
            {
                return _snprintf_s_l(_out, _capacity, _TRUNCATE, "%.*f",
                                     loc, _precision, _value);
            }
            case 'e':
            {
                return _snprintf_s_l(_out, _capacity, _TRUNCATE, "%.*e",
                                     loc, _precision, _value);
            }
            default:
            {
                return _snprintf_s_l(_out, _capacity, _TRUNCATE, "%.*g",
                                     loc, _precision, _value);
            }
        }
    }
#else
    (void)_handle;
#endif

    int length;

    switch (_style)
    {
        case 'f':
        {
            length = snprintf(_out, _capacity, "%.*f", _precision, _value);

            break;
        }
        case 'e':
        {
            length = snprintf(_out, _capacity, "%.*e", _precision, _value);

            break;
        }
        default:
        {
            length = snprintf(_out, _capacity, "%.*g", _precision, _value);

            break;
        }
    }

#if (D_INTERNAL_OSTREAM_FORMAT_LOCALE == D_CFG_OSTREAM_FORMAT_LOCALE_POSIX)
    if (_handle != NULL)
    {
        (void)uselocale(previous);
    }
#endif

    return length;
}

#if (D_INTERNAL_FORMAT_SHARED == 1)

// d_internal_format_c_object
//   the C locale object every C-form double shares: made by the first call
// that needs it, published with a compare-and-swap, never released.
static void* d_internal_format_c_object = NULL;

/**
 * @brief The shared C locale object, made and published on first use.
 *
 * @return the object, or NULL when it could not be made.
 *
 * Two threads may both make one; the one whose compare-and-swap loses
 * releases its own and takes the winner's, so exactly one is kept.
 */
static void*
d_internal_format_c_shared(void)
{
    void* expected = NULL;

#if ( (defined(__GNUC__)) ||                                                   \
      (defined(__clang__)) )
    void* const seen = __atomic_load_n(&d_internal_format_c_object,
                                       __ATOMIC_ACQUIRE);
#else
    void* const seen = _InterlockedCompareExchangePointer(
                           &d_internal_format_c_object, NULL, NULL);
#endif

    if (seen != NULL)
    {
        return seen;
    }

    void* const made = d_internal_format_locale_make("C");

    if (made == NULL)
    {
        return NULL;
    }

#if ( (defined(__GNUC__)) ||                                                   \
      (defined(__clang__)) )
    if (__atomic_compare_exchange_n(&d_internal_format_c_object,
                                    &expected,
                                    made,
                                    0,
                                    __ATOMIC_ACQ_REL,
                                    __ATOMIC_ACQUIRE))
    {
        return made;
    }
#else
    expected = _InterlockedCompareExchangePointer(&d_internal_format_c_object,
                                                  made,
                                                  NULL);

    if (expected == NULL)
    {
        return made;
    }
#endif

    d_internal_format_locale_free(made);

    return expected;
}

#endif  // D_INTERNAL_FORMAT_SHARED

/**
 * @brief Formats a double in the C form.
 *
 * @param[out] _out        the buffer.
 * @param[in]  _capacity   its size.
 * @param[in]  _value      the value; finite.
 * @param[in]  _style      'g', 'f' or 'e'.
 * @param[in]  _precision  the precision.
 * @return the length, as snprintf's; negative on failure.
 *
 * Through the C locale object where there is one. Where there is none, or
 * it cannot be made, in the program's locale with its decimal point
 * rewritten: the C form still, though not proof against setlocale.
 */
static int
d_internal_format_c_form(
    char*  _out,
    size_t _capacity,
    double _value,
    char   _style,
    int    _precision
)
{
#if (D_INTERNAL_FORMAT_OBJECTS == 1)
    #if (D_INTERNAL_FORMAT_SHARED == 1)
    void* const handle = d_internal_format_c_shared();
    #else
    void* const handle = d_internal_format_locale_make("C");
    #endif

    if (handle != NULL)
    {
        const int length = d_internal_format_double(_out, _capacity, _value,
                                                    _style, _precision,
                                                    handle);

    #if (D_INTERNAL_FORMAT_SHARED == 0)
        d_internal_format_locale_free(handle);
    #endif

        return length;
    }
#endif

    const int length = d_internal_format_double(_out, _capacity, _value,
                                                _style, _precision, NULL);

    if ( (length < 0) ||
         ((size_t)length >= _capacity) )
    {
        return length;
    }

    return (int)d_internal_format_c_point(_out, (size_t)length);
}

/*
d_ostream_write_double_l
  Not-a-number and the infinities are spelled here, because C libraries spell
them differently; they are told apart by comparison rather than by isnan and
isinf, whose MinGW macros narrow a double to float on one of their paths. A
finite value is formatted on the stack: snprintf's cost dwarfs the copy, and
the stack holds the longest one.
*/
int32_t
d_ostream_write_double_l(
    struct dj_ostream*              _os,
    double                          _value,
    char                            _style,
    int32_t                         _precision,
    const struct dj_ostream_locale* _locale
)
{
    // the readiness check
    if (d_ostream_write_slow(_os, NULL, 0u) == 0)
    {
        return 0;
    }

    if ( ( (_style != 'g') &&
           (_style != 'f') &&
           (_style != 'e') ) ||
         ( (_locale != NULL) &&
           (_locale->is_c == 0) &&
           (_locale->handle == NULL) ) )
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
    const int length    = ( (_locale == NULL) ||
                            (_locale->is_c != 0) )
                              ? d_internal_format_c_form(text,
                                                         sizeof(text),
                                                         _value,
                                                         _style,
                                                         precision)
                              : d_internal_format_double(text,
                                                         sizeof(text),
                                                         _value,
                                                         _style,
                                                         precision,
                                                         _locale->handle);

    if (length < 0)
    {
        return d_ostream_fail(_os,
                              D_OSTREAM_FAILED,
                              (errno != 0) ? (int32_t)errno : EINVAL);
    }

    if ((size_t)length >= sizeof(text))
    {
        return d_ostream_fail(_os, D_OSTREAM_FAILED, ERANGE);
    }

    return d_ostream_write(_os, text, (size_t)length);
}

/*
d_ostream_write_double
  The C form: d_ostream_write_double_l with no locale.
*/
int32_t
d_ostream_write_double(
    struct dj_ostream* _os,
    double             _value,
    char               _style,
    int32_t            _precision
)
{
    return d_ostream_write_double_l(_os, _value, _style, _precision, NULL);
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

/*
d_ostream_locale_init
  "C" and "POSIX" need no object under any mechanism: the C form is how a
double is written anyway.
*/
int32_t
d_ostream_locale_init(
    struct dj_ostream_locale* _locale,
    const char*               _name
)
{
    if (_locale == NULL)
    {
        errno = EINVAL;

        return 0;
    }

    _locale->handle = NULL;
    _locale->is_c   = 0;

    if (_name == NULL)
    {
        errno = EINVAL;

        return 0;
    }

    if ( (strcmp(_name, "C") == 0) ||
         (strcmp(_name, "POSIX") == 0) )
    {
        _locale->is_c = 1;

        return 1;
    }

#if (D_INTERNAL_FORMAT_OBJECTS == 1)
    errno = 0;

    void* const made = d_internal_format_locale_make(_name);

    if (made == NULL)
    {
        // a locale the platform lacks, whatever it called it
        if (errno != ENOMEM)
        {
            errno = ENOENT;
        }

        return 0;
    }

    _locale->handle = made;

    return 1;
#else
    errno = D_INTERNAL_FORMAT_ENOTSUP;

    return 0;
#endif
}

/*
d_ostream_locale_close
  Leaves the locale as a failed init does, so a second close is harmless.
*/
void
d_ostream_locale_close(
    struct dj_ostream_locale* _locale
)
{
    if (_locale == NULL)
    {
        return;
    }

#if (D_INTERNAL_FORMAT_OBJECTS == 1)
    if (_locale->handle != NULL)
    {
        d_internal_format_locale_free(_locale->handle);
    }
#endif

    _locale->handle = NULL;
    _locale->is_c   = 0;

    return;
}

/*
d_ostream_locale_named
  What the build resolved, for a caller deciding whether to offer a choice.
*/
int32_t
d_ostream_locale_named(void)
{
    return (D_INTERNAL_FORMAT_OBJECTS == 1) ? 1 : 0;
}
