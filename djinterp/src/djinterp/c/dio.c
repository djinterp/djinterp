/*******************************************************************************
* djinterp [c]                                                             dio.c
*
* Definitions for the declarations in `dio.h`.
*   The printf and gets wrappers defer to the C library, choosing the Annex K
* or MSVC secure variant where D_ENV_C_HAS_SCANF_S is set and a standard
* fallback elsewhere. The checked scanf family is the framework's own and the
* same everywhere: its engine, first below, hands each directive of a format
* to the plain library function and holds %c, %s and %[ to their buffer
* sizes (decision 38 of the register, 2026.10.04).
*
*
* path:      /src/djinterp/c/dio.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.19
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../inc/djinterp/c/dio.h"  // corresponding header
// std
#include <ctype.h>                          // isdigit, isspace
#include <limits.h>                         // INT_MAX
#include <stdarg.h>                         // va_list, va_start, va_copy
#include <stddef.h>                         // ptrdiff_t, size_t, wchar_t
#include <stdio.h>                          // vsscanf, vfscanf, fgets, ...
#include <stdlib.h>                         // malloc, free
#include <string.h>                         // memcpy, memset, strchr
#include <wchar.h>                          // mbrtowc, mbstate_t
// re_std
#include "../../../inc/re_std/cstdint/dstdint.h"  // intmax_t, uintmax_t


// the checked scanf family's engine
//   The framework's own reading of the C11 Annex K convention (K.3.5.3) for
// d_sscanf_s, d_vsscanf_s and d_fscanf_s, the same on every platform, MSVC's
// included. The format is split into its directives and each one goes, alone
// and followed by a %n, to the C library's plain vsscanf or vfscanf: the
// conversions keep the library's own syntax and locale, while the engine
// walks the arguments, holds %c, %s and %[ to their buffer sizes, and counts
// what each directive consumed.

// d_dio_scan_step
//   enum: what one directive came to.
enum d_dio_scan_step
{
    D_DIO_SCAN_OK,        // matched; the next directive follows
    D_DIO_SCAN_INPUT,     // input failure: end of input, or an encoding error
    D_DIO_SCAN_MATCH,     // matching failure, a too-small buffer included
    D_DIO_SCAN_VIOLATION  // runtime-constraint violation: the call returns EOF
};

// d_dio_scan_length
//   enum: a conversion specification's length modifier.
enum d_dio_scan_length
{
    D_DIO_SCAN_LEN_NONE,
    D_DIO_SCAN_LEN_HH,
    D_DIO_SCAN_LEN_H,
    D_DIO_SCAN_LEN_L,
    D_DIO_SCAN_LEN_LL,
    D_DIO_SCAN_LEN_J,
    D_DIO_SCAN_LEN_Z,
    D_DIO_SCAN_LEN_T,
    D_DIO_SCAN_LEN_BIG_L  // L: long double
};

// d_dio_scan_source
//   struct: what one call reads, a string or a stream, and how many characters
// the call has consumed from it so far, which is what %n stores.
struct d_dio_scan_source
{
    const char* text;      // the string, or NULL for a stream
    size_t      pos;       // the string's first unread character
    FILE*       stream;    // the stream, or NULL for a string
    size_t      consumed;  // characters this call has consumed
};

// d_dio_scan_spec
//   struct: one parsed conversion specification.
struct d_dio_scan_spec
{
    int                    suppress;  // '*' was given
    size_t                 width;     // the field width, 0 where none was given
    enum d_dio_scan_length length;    // the length modifier
    const char*            mod;       // the modifier's text in the format
    size_t                 mod_n;     // its length: 0, 1 or 2
    char                   conv;      // the conversion character
    const char*            set;       // for '[', the scanset after the '['
    size_t                 set_n;     // its length, through the closing ']'
};

// d_dio_scan_fmt
//   struct: a one-directive format, in a local buffer where it fits and on
// the heap where a long scanset or literal does not.
struct d_dio_scan_fmt
{
    char  local[96];
    char* heap;
};

/*
d_dio_scan_call
  The one place the C library is called. The format is one directive and a
trailing %n; the variadic arguments are the directive's target, if it has
one, then the %n counter. Passing the target through a variadic call takes it
to the library's own va_arg with its own type.
*/
static int
d_dio_scan_call(
    const struct d_dio_scan_source* _src,
    const char*                     _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const int result = (_src->stream)
                     ? vfscanf(_src->stream,
                               _format,
                               args)
                     : vsscanf(_src->text + _src->pos,
                               _format,
                               args);

    va_end(args);

    return result;
}

/*
d_dio_scan_result
  Turns a library call's return value and %n count into a step, and moves the
source past what a matched directive consumed. %n runs only once everything
before it in the directive has matched, so a count still at -1 means it did
not; the library's EOF then says whether input ran out or failed to match.
*/
static enum d_dio_scan_step
d_dio_scan_result(
    struct d_dio_scan_source* _src,
    int                       _returned,
    int                       _count
)
{
    // %n ran: the directive matched
    if (_count >= 0)
    {
        if (!_src->stream)
        {
            _src->pos += (size_t)_count;
        }

        _src->consumed += (size_t)_count;

        return D_DIO_SCAN_OK;
    }

    return (_returned == EOF) ? D_DIO_SCAN_INPUT
                              : D_DIO_SCAN_MATCH;
}

/*
d_dio_scan_peek
  The source's next character, left unread, or EOF at its end. A stream takes
the character back with ungetc, which C guarantees for one character.
*/
static int
d_dio_scan_peek(
    const struct d_dio_scan_source* _src
)
{
    if (!_src->stream)
    {
        const unsigned char c = (unsigned char)_src->text[_src->pos];

        return (c == '\0') ? EOF : (int)c;
    }

    const int c = getc(_src->stream);

    // a peek: the stream gets the character back
    if (c != EOF)
    {
        (void)ungetc(c,
                     _src->stream);
    }

    return c;
}

/*
d_dio_scan_parse_length
  Reads the length modifier at _at into _spec, and returns what follows it.
*/
static const char*
d_dio_scan_parse_length(
    const char*             _at,
    struct d_dio_scan_spec* _spec
)
{
    const char* p = _at;

    _spec->mod    = p;
    _spec->length = D_DIO_SCAN_LEN_NONE;

    switch (*p)
    {
        case 'h':
            _spec->length = (p[1] == 'h') ? D_DIO_SCAN_LEN_HH
                                          : D_DIO_SCAN_LEN_H;
            break;
        case 'l':
            _spec->length = (p[1] == 'l') ? D_DIO_SCAN_LEN_LL
                                          : D_DIO_SCAN_LEN_L;
            break;
        case 'j':
            _spec->length = D_DIO_SCAN_LEN_J;
            break;
        case 'z':
            _spec->length = D_DIO_SCAN_LEN_Z;
            break;
        case 't':
            _spec->length = D_DIO_SCAN_LEN_T;
            break;
        case 'L':
            _spec->length = D_DIO_SCAN_LEN_BIG_L;
            break;
        default:
            break;
    }

    _spec->mod_n = ( (_spec->length == D_DIO_SCAN_LEN_HH) ||
                     (_spec->length == D_DIO_SCAN_LEN_LL) )
                 ? 2
                 : (_spec->length == D_DIO_SCAN_LEN_NONE) ? 0 : 1;

    return p + _spec->mod_n;
}

/*
d_dio_scan_takes_length
  Whether conversion _conv accepts length modifier _length (C11 7.21.6.2p11):
the integer modifiers go with d, i, o, u, x, X and n; l also with the
floating conversions and with c, s and [; L only with the floating ones; p
and % take none.
*/
static int
d_dio_scan_takes_length(
    char                   _conv,
    enum d_dio_scan_length _length
)
{
    const int integer  = (strchr("diouxXn", _conv) != NULL);
    const int floating = (strchr("aAeEfFgG", _conv) != NULL);

    switch (_length)
    {
        case D_DIO_SCAN_LEN_NONE:
            return 1;
        case D_DIO_SCAN_LEN_L:
            return ( (integer)  ||
                     (floating) ||
                     (strchr("cs[", _conv) != NULL) );
        case D_DIO_SCAN_LEN_BIG_L:
            return floating;
        default:
            return integer;
    }
}

/*
d_dio_scan_parse_conversion
  Reads the conversion character at _at, and a scanset after '['; checks it
against what came before it; returns what follows the specification, or NULL
for one the standard leaves undefined. A ']' right after the '[' or '[^' is a
member of the set, not its end.
*/
static const char*
d_dio_scan_parse_conversion(
    const char*             _at,
    struct d_dio_scan_spec* _spec
)
{
    const char* p = _at;

    // no conversion character, or one the standard does not define
    if ( (*p == '\0') ||
         (strchr("diouxXaAeEfFgGcsp[n%", *p) == NULL) )
    {
        return NULL;
    }

    _spec->conv = *p++;

    // %% is exactly two characters; %n takes neither '*' nor a width
    if ( ( (_spec->conv == '%') &&
           ( (_spec->suppress) || (_spec->width) || (_spec->mod_n) ) ) ||
         ( (_spec->conv == 'n') &&
           ( (_spec->suppress) || (_spec->width) ) )                   ||
         (!d_dio_scan_takes_length(_spec->conv, _spec->length)) )
    {
        return NULL;
    }

    // the scanset runs to the first ']' after its first member
    if (_spec->conv == '[')
    {
        const char* end = p;

        if (*end == '^')
        {
            end++;
        }

        if (*end == ']')
        {
            end++;
        }

        end = strchr(end, ']');

        // an unclosed scanset
        if (!end)
        {
            return NULL;
        }

        _spec->set   = p;
        _spec->set_n = (size_t)(end - p) + 1;
        p            = end + 1;
    }

    return p;
}

/*
d_dio_scan_parse
  Parses the specification whose '%' is at _at into _spec. Returns what
follows it, or NULL where the standard leaves the specification undefined or
the engine does not take it: a width of 0, or past INT_MAX, which no library
reads, besides the cases d_dio_scan_parse_conversion refuses.
*/
static const char*
d_dio_scan_parse(
    const char*             _at,
    struct d_dio_scan_spec* _spec
)
{
    const char* p = _at + 1;

    memset(_spec,
           0,
           sizeof(*_spec));

    // '*': assignment suppression
    if (*p == '*')
    {
        _spec->suppress = 1;
        p++;
    }

    // the field width: a nonzero decimal integer
    while (isdigit((unsigned char)*p))
    {
        // a digit more would pass INT_MAX
        if (_spec->width > ((size_t)INT_MAX - 9) / 10)
        {
            return NULL;
        }

        _spec->width = (_spec->width * 10) + (size_t)(*p - '0');
        p++;
    }

    // "%0d": the width was all zeros
    if ( (p > _at + 1 + (size_t)_spec->suppress) &&
         (_spec->width == 0) )
    {
        return NULL;
    }

    p = d_dio_scan_parse_length(p,
                                _spec);

    return d_dio_scan_parse_conversion(p,
                                       _spec);
}

/*
d_dio_scan_fmt_buffer
  A buffer of _n bytes for a one-directive format: _out's own where it fits,
else the heap's, which d_dio_scan_fmt_free releases. NULL if that fails.
*/
static char*
d_dio_scan_fmt_buffer(
    struct d_dio_scan_fmt* _out,
    size_t                 _n
)
{
    _out->heap = NULL;

    if (_n <= sizeof(_out->local))
    {
        return _out->local;
    }

    _out->heap = (char*)malloc(_n);

    return _out->heap;
}

/*
d_dio_scan_fmt_free
  Releases the heap buffer d_dio_scan_fmt_buffer took, if it took one.
*/
static void
d_dio_scan_fmt_free(
    struct d_dio_scan_fmt* _out
)
{
    free(_out->heap);
    _out->heap = NULL;

    return;
}

/*
d_dio_scan_format
  Writes the one-directive format for _spec: "%", '*' when _suppress, _width
when it is not 0, the length modifier, the conversion and its scanset, then
"%n". The width is written in decimal by hand: it is at most INT_MAX, and the
engine stays clear of the printf family it sits beside. Returns the format,
or NULL if a long scanset needed the heap and the heap said no.
*/
static const char*
d_dio_scan_format(
    struct d_dio_scan_fmt*        _out,
    const struct d_dio_scan_spec* _spec,
    int                           _suppress,
    size_t                        _width
)
{
    char   digits[24];
    size_t n_digits = 0;

    // the width's digits, last first
    for (size_t w = _width; w != 0; w /= 10)
    {
        digits[n_digits++] = (char)('0' + (int)(w % 10));
    }

    char* f = d_dio_scan_fmt_buffer(_out,
                                    1 + 1 + n_digits + _spec->mod_n + 1 +
                                    _spec->set_n + 2 + 1);

    // the heap refused a long scanset
    if (!f)
    {
        return NULL;
    }

    char* p = f;

    *p++ = '%';

    if (_suppress)
    {
        *p++ = '*';
    }

    while (n_digits != 0)
    {
        *p++ = digits[--n_digits];
    }

    memcpy(p, _spec->mod, _spec->mod_n);
    p += _spec->mod_n;
    *p++ = _spec->conv;
    memcpy(p, _spec->set ? _spec->set : "", _spec->set_n);
    p += _spec->set_n;
    memcpy(p, "%n", 3);

    return f;
}

/*
d_dio_scan_run
  Runs a directive that stores nothing but its %n: white space, a literal,
%%, or a suppressed conversion.
*/
static enum d_dio_scan_step
d_dio_scan_run(
    struct d_dio_scan_source* _src,
    const char*               _format
)
{
    int count = -1;

    const int returned = d_dio_scan_call(_src,
                                         _format,
                                         &count);

    return d_dio_scan_result(_src,
                             returned,
                             count);
}

// D_INTERNAL_DIO_SCAN_TARGET
//   macro: inside d_dio_scan_value, takes the next argument as a pointer to
// `type`, refuses a null one, and runs the directive into it. A function per
// target type would be twenty copies of these lines; the type has to reach
// va_arg and the library call as itself, so a macro it is, and it ends with
// the function.
#define D_INTERNAL_DIO_SCAN_TARGET(type)                                       \
    {                                                                          \
        type* target = va_arg(*_args, type*);                                  \
                                                                               \
        if (!target)                                                           \
        {                                                                      \
            return D_DIO_SCAN_VIOLATION;                                       \
        }                                                                      \
                                                                               \
        int       count    = -1;                                               \
        const int returned = d_dio_scan_call(_src, _format, target, &count);   \
                                                                               \
        return d_dio_scan_result(_src, returned, count);                       \
    }

/*
d_dio_scan_value
  Runs a numeric or %p conversion into the next argument, taken with the type
its conversion and length modifier name (C11 7.21.6.2p11-12). C has no name
for the signed type of size_t or the unsigned type of ptrdiff_t; ptrdiff_t
and size_t stand in for them, as each is the other's counterpart on every
ABI the framework targets.
*/
static enum d_dio_scan_step
d_dio_scan_value(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    const char*                   _format,
    va_list*                      _args
)
{
    // signed integers
    if ( (_spec->conv == 'd') ||
         (_spec->conv == 'i') )
    {
        switch (_spec->length)
        {
            case D_DIO_SCAN_LEN_HH: D_INTERNAL_DIO_SCAN_TARGET(signed char)
            case D_DIO_SCAN_LEN_H:  D_INTERNAL_DIO_SCAN_TARGET(short)
            case D_DIO_SCAN_LEN_L:  D_INTERNAL_DIO_SCAN_TARGET(long)
            case D_DIO_SCAN_LEN_LL: D_INTERNAL_DIO_SCAN_TARGET(long long)
            case D_DIO_SCAN_LEN_J:  D_INTERNAL_DIO_SCAN_TARGET(intmax_t)
            case D_DIO_SCAN_LEN_Z:  D_INTERNAL_DIO_SCAN_TARGET(ptrdiff_t)
            case D_DIO_SCAN_LEN_T:  D_INTERNAL_DIO_SCAN_TARGET(ptrdiff_t)
            default:                D_INTERNAL_DIO_SCAN_TARGET(int)
        }
    }

    // unsigned integers
    if (strchr("ouxX", _spec->conv) != NULL)
    {
        switch (_spec->length)
        {
            case D_DIO_SCAN_LEN_HH: D_INTERNAL_DIO_SCAN_TARGET(unsigned char)
            case D_DIO_SCAN_LEN_H:  D_INTERNAL_DIO_SCAN_TARGET(unsigned short)
            case D_DIO_SCAN_LEN_L:  D_INTERNAL_DIO_SCAN_TARGET(unsigned long)
            case D_DIO_SCAN_LEN_LL:
                D_INTERNAL_DIO_SCAN_TARGET(unsigned long long)
            case D_DIO_SCAN_LEN_J:  D_INTERNAL_DIO_SCAN_TARGET(uintmax_t)
            case D_DIO_SCAN_LEN_Z:  D_INTERNAL_DIO_SCAN_TARGET(size_t)
            case D_DIO_SCAN_LEN_T:  D_INTERNAL_DIO_SCAN_TARGET(size_t)
            default:                D_INTERNAL_DIO_SCAN_TARGET(unsigned int)
        }
    }

    // pointers
    if (_spec->conv == 'p')
    {
        D_INTERNAL_DIO_SCAN_TARGET(void*)
    }

    // floating: a, A, e, E, f, F, g, G
    switch (_spec->length)
    {
        case D_DIO_SCAN_LEN_L:     D_INTERNAL_DIO_SCAN_TARGET(double)
        case D_DIO_SCAN_LEN_BIG_L: D_INTERNAL_DIO_SCAN_TARGET(long double)
        default:                   D_INTERNAL_DIO_SCAN_TARGET(float)
    }
}

#undef D_INTERNAL_DIO_SCAN_TARGET

// D_INTERNAL_DIO_SCAN_COUNT
//   macro: inside d_dio_scan_count, stores the characters consumed so far
// through the next argument, a pointer to `type`; the same reason as
// D_INTERNAL_DIO_SCAN_TARGET, and it ends with the function too.
#define D_INTERNAL_DIO_SCAN_COUNT(type)                                        \
    {                                                                          \
        type* target = va_arg(*_args, type*);                                  \
                                                                               \
        if (!target)                                                           \
        {                                                                      \
            return D_DIO_SCAN_VIOLATION;                                       \
        }                                                                      \
                                                                               \
        *target = (type)_src->consumed;                                        \
                                                                               \
        return D_DIO_SCAN_OK;                                                  \
    }

/*
d_dio_scan_count
  %n: stores the characters this call has consumed, through the next
argument. It reads no input and assigns no item.
*/
static enum d_dio_scan_step
d_dio_scan_count(
    const struct d_dio_scan_source* _src,
    const struct d_dio_scan_spec*   _spec,
    va_list*                        _args
)
{
    switch (_spec->length)
    {
        case D_DIO_SCAN_LEN_HH: D_INTERNAL_DIO_SCAN_COUNT(signed char)
        case D_DIO_SCAN_LEN_H:  D_INTERNAL_DIO_SCAN_COUNT(short)
        case D_DIO_SCAN_LEN_L:  D_INTERNAL_DIO_SCAN_COUNT(long)
        case D_DIO_SCAN_LEN_LL: D_INTERNAL_DIO_SCAN_COUNT(long long)
        case D_DIO_SCAN_LEN_J:  D_INTERNAL_DIO_SCAN_COUNT(intmax_t)
        case D_DIO_SCAN_LEN_Z:  D_INTERNAL_DIO_SCAN_COUNT(ptrdiff_t)
        case D_DIO_SCAN_LEN_T:  D_INTERNAL_DIO_SCAN_COUNT(ptrdiff_t)
        default:                D_INTERNAL_DIO_SCAN_COUNT(int)
    }
}

#undef D_INTERNAL_DIO_SCAN_COUNT

/*
d_dio_scan_wide_count
  The wide characters the _n bytes at _text convert to, as the library's own
conversion counted them; (size_t)-1 if they do not convert, which cannot
happen to a field the library has just converted.
*/
static size_t
d_dio_scan_wide_count(
    const char* _text,
    size_t      _n
)
{
    mbstate_t state;
    size_t    chars = 0;

    memset(&state,
           0,
           sizeof(state));

    for (size_t i = 0; i < _n; chars++)
    {
        const size_t step = mbrtowc(NULL,
                                    _text + i,
                                    _n - i,
                                    &state);

        // not a whole, valid character
        if ( (step == (size_t)-1) ||
             (step == (size_t)-2) )
        {
            return (size_t)-1;
        }

        i += (step == 0) ? 1 : step;
    }

    return chars;
}

/*
d_dio_scan_skip_space
  Consumes white space up to the next other character or the end of input. A
white-space directive cannot fail (C11 7.21.6.2p5): where the library reports
the end of input instead of running the %n, nothing was there to consume, and
whatever comes next meets the end itself.
*/
static void
d_dio_scan_skip_space(
    struct d_dio_scan_source* _src
)
{
    (void)d_dio_scan_run(_src,
                         " %n");

    return;
}

/*
d_dio_scan_in_set
  Whether the next character belongs to _spec's scanset, asked of the library
with a suppressed one-character %[ so its ranges and locale decide. On a
string nothing is consumed; on a stream a matching character is, which only
ever happens on the way to a matching failure. Returns 1, 0, or EOF at the
end of input.
*/
static int
d_dio_scan_in_set(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec
)
{
    struct d_dio_scan_fmt fmt;
    const char*           format = d_dio_scan_format(&fmt, _spec, 1, 1);

    // only a scanset longer than the local buffer, with the heap refusing
    if (!format)
    {
        return 0;
    }

    int       count    = -1;
    const int returned = d_dio_scan_call(_src,
                                         format,
                                         &count);

    d_dio_scan_fmt_free(&fmt);

    // on a stream the character is gone; account for it
    if ( (count > 0) &&
         (_src->stream) )
    {
        _src->consumed += (size_t)count;
    }

    if (count > 0)
    {
        return 1;
    }

    return (returned == EOF) ? EOF : 0;
}

/*
d_dio_scan_target_text
  Runs a %c, %s or %[ directive into _target, a char* or a wchar_t* as the
length modifier says, with width _width (0: none).
*/
static enum d_dio_scan_step
d_dio_scan_target_text(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    void*                         _target,
    size_t                        _width
)
{
    struct d_dio_scan_fmt fmt;
    const char*           format = d_dio_scan_format(&fmt, _spec, 0, _width);

    // the heap refused a long scanset
    if (!format)
    {
        return D_DIO_SCAN_VIOLATION;
    }

    int       count    = -1;
    const int returned = (_spec->length == D_DIO_SCAN_LEN_L)
                       ? d_dio_scan_call(_src,
                                         format,
                                         (wchar_t*)_target,
                                         &count)
                       : d_dio_scan_call(_src,
                                         format,
                                         (char*)_target,
                                         &count);

    d_dio_scan_fmt_free(&fmt);

    return d_dio_scan_result(_src,
                             returned,
                             count);
}

/*
d_dio_scan_too_small
  The answer for a %c, %s or %[ whose buffer cannot hold its field: a matching
failure, or an input failure if the input has already run out, as it would
have for a buffer of any size. A string's buffer is left holding the empty
string where it has room for one, so it is never left unterminated.
*/
static enum d_dio_scan_step
d_dio_scan_too_small(
    const struct d_dio_scan_source* _src,
    const struct d_dio_scan_spec*   _spec,
    void*                           _target,
    size_t                          _size
)
{
    // a string conversion: terminate what is there
    if ( (_spec->conv != 'c') &&
         (_size != 0) )
    {
        if (_spec->length == D_DIO_SCAN_LEN_L)
        {
            ((wchar_t*)_target)[0] = L'\0';
        }
        else
        {
            ((char*)_target)[0] = '\0';
        }
    }

    return (d_dio_scan_peek(_src) == EOF) ? D_DIO_SCAN_INPUT
                                          : D_DIO_SCAN_MATCH;
}

/*
d_dio_scan_text_measured
  A wide %lc, %ls or %l[ on a string, which can be read twice: the field is
measured first with the conversion suppressed, its bytes counted back into
wide characters, and only a field that fits is converted, with the width the
caller gave. Exact, whatever the library counts its widths in.
*/
static enum d_dio_scan_step
d_dio_scan_text_measured(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    void*                         _target,
    size_t                        _size
)
{
    struct d_dio_scan_fmt fmt;
    const char*           format = d_dio_scan_format(&fmt, _spec, 1,
                                                     _spec->width);

    // the heap refused a long scanset
    if (!format)
    {
        return D_DIO_SCAN_VIOLATION;
    }

    int       count    = -1;
    const int returned = d_dio_scan_call(_src,
                                         format,
                                         &count);

    d_dio_scan_fmt_free(&fmt);

    // the field itself fails: as the plain conversion would, except that
    // %lc, which matches any character, fails only at the end of input or
    // on an encoding error, and both are input failures (C11 7.21.6.2p10)
    if (count < 0)
    {
        return ( (returned == EOF) ||
                 (_spec->conv == 'c') ) ? D_DIO_SCAN_INPUT
                                        : D_DIO_SCAN_MATCH;
    }

    const size_t chars  = d_dio_scan_wide_count(_src->text + _src->pos,
                                                (size_t)count);
    const size_t needed = (_spec->conv == 'c') ? chars : chars + 1;

    // an unconvertible field, or one the buffer cannot hold
    if ( (chars == (size_t)-1) ||
         (_size < needed) )
    {
        return d_dio_scan_too_small(_src, _spec, _target, _size);
    }

    return d_dio_scan_target_text(_src,
                                  _spec,
                                  _target,
                                  _spec->width);
}

/*
d_dio_scan_char_bounded
  %c and %lc, which need as many elements as their width (1 where none is
given); a buffer that has them cannot overrun.
*/
static enum d_dio_scan_step
d_dio_scan_char_bounded(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    void*                         _target,
    size_t                        _size
)
{
    const size_t width  = (_spec->width) ? _spec->width : 1;
    const size_t before = _src->consumed;

    // too small for the field
    if (_size < width)
    {
        return d_dio_scan_too_small(_src, _spec, _target, _size);
    }

    const enum d_dio_scan_step step =
        d_dio_scan_target_text(_src, _spec, _target, _spec->width);

    // %c matches any character, so it fails only at the end of input,
    // on a read error or, for %lc, on an encoding error: input failures
    // all (C11 7.21.6.2p10), whatever the library called them
    if (step == D_DIO_SCAN_MATCH)
    {
        return D_DIO_SCAN_INPUT;
    }

    // %c takes exactly its width (p12), but glibc's stops short at the
    // end of input and reports success; a narrow field's count is its
    // length, so the short one is the input failure it is
    return ( (step == D_DIO_SCAN_OK)              &&
             (_spec->length != D_DIO_SCAN_LEN_L) &&
             (_src->consumed - before < width) )
         ? D_DIO_SCAN_INPUT
         : step;
}

/*
d_dio_scan_text_bounded
  %c, %s and %[ wherever the field cannot be read twice: narrow ones always,
wide ones on a stream. %c goes to d_dio_scan_char_bounded. A string
conversion is read with the width capped at the buffer's size less one, so it
cannot overrun; if the cap, not the caller's width, ended the field and the
field goes on, the field was too long, and that is a matching failure, not a
truncation. Exact for narrow conversions; for a wide one on a stream, exact
where the library counts widths in characters, as glibc does.
*/
static enum d_dio_scan_step
d_dio_scan_text_bounded(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    void*                         _target,
    size_t                        _size
)
{
    if (_spec->conv == 'c')
    {
        return d_dio_scan_char_bounded(_src,
                                       _spec,
                                       _target,
                                       _size);
    }

    // %s skips white space before its field; skip it here, so a peek sees
    // the field's first character
    if (_spec->conv == 's')
    {
        d_dio_scan_skip_space(_src);
    }

    // no room for a character and the terminator
    if (_size < 2)
    {
        return d_dio_scan_too_small(_src, _spec, _target, _size);
    }

    const size_t cap    = (_size - 1 < (size_t)INT_MAX) ? _size - 1
                                                         : (size_t)INT_MAX;
    const int    capped = ( (_spec->width == 0) ||
                            (_spec->width > cap) );
    const enum d_dio_scan_step step =
        d_dio_scan_target_text(_src,
                               _spec,
                               _target,
                               capped ? cap : _spec->width);

    // a field the cap did not end, or one that did not match at all
    if ( (step != D_DIO_SCAN_OK) ||
         (!capped) )
    {
        return step;
    }

    // the cap ended it: does the field go on?
    const int next = (_spec->conv == 's') ? d_dio_scan_peek(_src)
                                          : d_dio_scan_in_set(_src, _spec);
    const int more = (_spec->conv == 's')
                   ? ( (next != EOF) && (!isspace(next)) )
                   : (next == 1);

    return more ? d_dio_scan_too_small(_src, _spec, _target, _size)
                : D_DIO_SCAN_OK;
}

/*
d_dio_scan_text
  %c, %s or %[: each takes its buffer, then the buffer's size in elements, as
a size_t (Annex K's rsize_t). A suppressed one takes neither.
*/
static enum d_dio_scan_step
d_dio_scan_text(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    va_list*                      _args
)
{
    // suppressed: no buffer, no size, nothing to hold to a size
    if (_spec->suppress)
    {
        struct d_dio_scan_fmt fmt;
        const char*           format = d_dio_scan_format(&fmt, _spec, 1,
                                                         _spec->width);

        // the heap refused a long scanset
        if (!format)
        {
            return D_DIO_SCAN_VIOLATION;
        }

        const enum d_dio_scan_step step = d_dio_scan_run(_src,
                                                         format);

        d_dio_scan_fmt_free(&fmt);

        return step;
    }

    void* target = (_spec->length == D_DIO_SCAN_LEN_L)
                 ? (void*)va_arg(*_args, wchar_t*)
                 : (void*)va_arg(*_args, char*);
    const size_t size = va_arg(*_args, size_t);

    // a null buffer: a runtime-constraint violation (K.3.5.3.2p2)
    if (!target)
    {
        return D_DIO_SCAN_VIOLATION;
    }

    return ( (_spec->length == D_DIO_SCAN_LEN_L) &&
             (!_src->stream) )
         ? d_dio_scan_text_measured(_src, _spec, target, size)
         : d_dio_scan_text_bounded(_src, _spec, target, size);
}

/*
d_dio_scan_literal
  Runs a run of ordinary characters, which must match the input exactly.
*/
static enum d_dio_scan_step
d_dio_scan_literal(
    struct d_dio_scan_source* _src,
    const char*               _text,
    size_t                    _n
)
{
    struct d_dio_scan_fmt fmt;
    char*                 format = d_dio_scan_fmt_buffer(&fmt, _n + 3);

    // the heap refused a long literal
    if (!format)
    {
        return D_DIO_SCAN_VIOLATION;
    }

    memcpy(format, _text, _n);
    memcpy(format + _n, "%n", 3);

    const enum d_dio_scan_step step = d_dio_scan_run(_src,
                                                     format);

    d_dio_scan_fmt_free(&fmt);

    return step;
}

/*
d_dio_scan_conversion
  Runs one conversion specification; counts it as assigned where it stores an
item, and as converted where it consumed input, which is what decides between
EOF and a count when input later runs out.
*/
static enum d_dio_scan_step
d_dio_scan_conversion(
    struct d_dio_scan_source*     _src,
    const struct d_dio_scan_spec* _spec,
    va_list*                      _args,
    int*                          _assigned,
    int*                          _converted
)
{
    if (_spec->conv == '%')
    {
        return d_dio_scan_run(_src,
                              "%%%n");
    }

    if (_spec->conv == 'n')
    {
        return d_dio_scan_count(_src,
                                _spec,
                                _args);
    }

    enum d_dio_scan_step step;

    if (strchr("cs[", _spec->conv) != NULL)
    {
        step = d_dio_scan_text(_src, _spec, _args);
    }
    else
    {
        struct d_dio_scan_fmt fmt;
        const char*           format = d_dio_scan_format(&fmt, _spec,
                                                         _spec->suppress,
                                                         _spec->width);

        step = (_spec->suppress) ? d_dio_scan_run(_src, format)
                                 : d_dio_scan_value(_src, _spec, format,
                                                    _args);
        d_dio_scan_fmt_free(&fmt);
    }

    // a conversion that matched
    if (step == D_DIO_SCAN_OK)
    {
        *_converted = 1;
        *_assigned += (_spec->suppress) ? 0 : 1;
    }

    return step;
}

/*
d_dio_scan_directive
  Runs the directive at *_f, white space, a run of ordinary characters, or a
conversion specification, and moves *_f past it.
*/
static enum d_dio_scan_step
d_dio_scan_directive(
    struct d_dio_scan_source* _src,
    const char**              _f,
    va_list*                  _args,
    int*                      _assigned,
    int*                      _converted
)
{
    const char* f = *_f;

    // white space: any amount in the format matches any amount of input
    if (isspace((unsigned char)*f))
    {
        while (isspace((unsigned char)*f))
        {
            f++;
        }

        *_f = f;
        d_dio_scan_skip_space(_src);

        return D_DIO_SCAN_OK;
    }

    // ordinary characters, up to the next white space or specification
    if (*f != '%')
    {
        while ( (*f != '\0') &&
                (*f != '%')  &&
                (!isspace((unsigned char)*f)) )
        {
            f++;
        }

        const enum d_dio_scan_step step = d_dio_scan_literal(_src,
                                                             *_f,
                                                             (size_t)(f - *_f));
        *_f = f;

        return step;
    }

    struct d_dio_scan_spec spec;
    const char*            next = d_dio_scan_parse(f,
                                                   &spec);

    // a specification the standard leaves undefined: refused as a constraint
    if (!next)
    {
        return D_DIO_SCAN_VIOLATION;
    }

    *_f = next;

    return d_dio_scan_conversion(_src,
                                 &spec,
                                 _args,
                                 _assigned,
                                 _converted);
}

/*
d_dio_scan_engine
  Runs the format a directive at a time. The result is EOF on a
runtime-constraint violation, and on an input failure before any conversion
consumed input (C11 7.21.6.2p16, K.3.5.3.2p4); otherwise it is the number of
items assigned. A suppressed conversion counts as a conversion, as the
standard has it: "5" against "%*d%d" returns 0 here, where glibc's own sscanf
returns EOF.
*/
static int
d_dio_scan_engine(
    struct d_dio_scan_source* _src,
    const char*               _format,
    va_list                   _argptr
)
{
    va_list args;
    int     assigned  = 0;
    int     converted = 0;
    int     result    = 0;
    int     done      = 0;

    // a local va_list: the helpers take its address, which C allows (7.16p3)
    va_copy(args,
            _argptr);

    for (const char* f = _format; (*f != '\0') && (!done); )
    {
        switch (d_dio_scan_directive(_src, &f, &args, &assigned, &converted))
        {
            case D_DIO_SCAN_VIOLATION:
                result = EOF;
                done   = 1;
                break;
            case D_DIO_SCAN_INPUT:
                result = (converted) ? assigned : EOF;
                done   = 1;
                break;
            case D_DIO_SCAN_MATCH:
                result = assigned;
                done   = 1;
                break;
            default:
                break;
        }
    }

    va_end(args);

    return (done) ? result : assigned;
}


// formatted input
/*
d_sscanf
  Collects the variadic arguments and defers to d_vsscanf.
*/
int
d_sscanf(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);
    const int result = d_vsscanf(_buffer,
                                 _format,
                                 args);
    va_end(args);

    return result;
}

/*
d_sscanf_s
  Collects the variadic arguments and defers to d_vsscanf_s.
*/
int
d_sscanf_s(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);
    const int result = d_vsscanf_s(_buffer,
                                   _format,
                                   args);
    va_end(args);

    return result;
}

/*
d_vsscanf
  Delegates to vsscanf.
*/
int
d_vsscanf(
    const char* _buffer,
    const char* _format,
    va_list     _argptr
)
{
    return vsscanf(_buffer,
                   _format,
                   _argptr);
}

/*
d_vsscanf_s
  The engine over a string. A null string or format is a runtime-constraint
violation (K.3.5.3.14p2): EOF, before anything is read.
*/
int
d_vsscanf_s(
    const char* _buffer,
    const char* _format,
    va_list     _argptr
)
{
    // runtime constraints: neither may be null
    if ( (!_buffer) ||
         (!_format) )
    {
        return EOF;
    }

    struct d_dio_scan_source src = { _buffer, 0, NULL, 0 };

    return d_dio_scan_engine(&src,
                             _format,
                             _argptr);
}

/*
d_fscanf
  Collects the variadic arguments and calls vfscanf.
*/
int
d_fscanf(
    FILE*       _stream,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const int result = vfscanf(_stream,
                               _format,
                               args);

    va_end(args);

    return result;
}

/*
d_fscanf_s
  The engine over a stream; a null stream or format is refused as at
d_vsscanf_s.
*/
int
d_fscanf_s(
    FILE*       _stream,
    const char* _format,
    ...
)
{
    // runtime constraints: neither may be null
    if ( (!_stream) ||
         (!_format) )
    {
        return EOF;
    }

    va_list args;

    va_start(args,
             _format);

    struct d_dio_scan_source src    = { NULL, 0, _stream, 0 };
    const int                result = d_dio_scan_engine(&src,
                                                        _format,
                                                        args);

    va_end(args);

    return result;
}

// formatted output
/*
d_sprintf_s
  Collects the variadic arguments and defers to d_vsprintf_s.
*/
int
d_sprintf_s(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const int result = d_vsprintf_s(_buffer,
                                    _size,
                                    _format,
                                    args);

    va_end(args);

    return result;
}

/*
d_vsprintf_s
  vsprintf_s where D_ENV_C_HAS_SCANF_S is set; otherwise vsnprintf, which
truncates where vsprintf_s would report the overflow.
*/
int
d_vsprintf_s(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    va_list     _argptr
)
{
#if D_ENV_C_HAS_SCANF_S

    return vsprintf_s(_buffer,
                      _size,
                      _format,
                      _argptr);
#else

    // fallback: vsnprintf, which bounds the write to _size
    return vsnprintf(_buffer,
                     _size,
                     _format,
                     _argptr);
#endif
}

/*
d_snprintf
  Collects the variadic arguments and defers to d_vsnprintf.
*/
int
d_snprintf(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);
    const int result = d_vsnprintf(_buffer,
                                   _size,
                                   _format,
                                   args);
    va_end(args);

    return result;
}

/*
d_vsnprintf
  vsnprintf, except under MSVC before 2015, which has only _vsnprintf: that
one returns -1 and leaves the buffer unterminated when the output does not
fit.
*/
int
d_vsnprintf(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    va_list     _argptr
)
{
#if defined(D_ENV_COMPILER_MSVC) && (D_ENV_COMPILER_MAJOR < 14)

    // MSVC before 2015 has only _vsnprintf
    return _vsnprintf(_buffer,
                      _size,
                      _format,
                      _argptr);
#else

    return vsnprintf(_buffer,
                     _size,
                     _format,
                     _argptr);
#endif
}

// character and string I/O
#if D_ENV_C_HAS_SCANF_S

/*
d_gets_s
  gets_s, where D_ENV_C_HAS_SCANF_S is set.
*/
char*
d_gets_s(
    char*  _buffer,
    size_t _size
)
{
    // parameter validation
    if ( (!_buffer) ||
         (_size == 0) )
    {
        return NULL;
    }

    char* result = gets_s(_buffer,
                          _size);

    return result;
}

#else

/*
d_gets_s
  Fallback: fgets from stdin, with the trailing newline stripped. Unlike
gets_s, a line longer than the buffer is not an error: its first part is
returned and the rest stays in stdin. _size is narrowed to int for fgets.
*/
char*
d_gets_s(
    char*  _buffer,
    size_t _size
)
{
    // parameter validation
    if ( (!_buffer) ||
         (_size == 0) )
    {
        return NULL;
    }

    // safe fallback using fgets
    char* result = fgets(_buffer,
                         (int)_size,
                         stdin);

    if (result)
    {
        const size_t len = strlen(_buffer);

        // remove trailing newline if present, similar to gets behavior
        if ( (len > 0) &&
             (_buffer[len - 1] == '\n') )
        {
            _buffer[len - 1] = '\0';
        }
    }

    return result;
}

#endif

/*
d_fputs
  Delegates to fputs.
*/
int
d_fputs(
    const char* _str,
    FILE*       _stream
)
{
    return fputs(_str,
                 _stream);
}

/*
d_fgets
  Delegates to fgets.
*/
char*
d_fgets(
    char* _str,
    int   _num,
    FILE* _stream
)
{
    return fgets(_str,
                 _num,
                 _stream);
}

// error handling
/*
d_perror
  Delegates to perror.
*/
void
d_perror(
    const char* _s
)
{
    perror(_s);

    return;
}

/*
d_feof
  Delegates to feof.
*/
int
d_feof(
    FILE* _stream
)
{
    return feof(_stream);
}

/*
d_ferror
  Delegates to ferror.
*/
int
d_ferror(
    FILE* _stream
)
{
    return ferror(_stream);
}

/*
d_clearerr
  Delegates to clearerr.
*/
void
d_clearerr(
    FILE* _stream
)
{
    clearerr(_stream);

    return;
}
