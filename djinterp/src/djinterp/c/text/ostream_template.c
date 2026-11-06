/*******************************************************************************
* djinterp [c]                                                ostream_template.c
*
*   Templates onto a stream (ostream_template.h): the value formatter, the
* conversions for JSON, CSV, Markdown and XML, the compiler and the program
* that runs a compiled template.
*
*   A FIELD WITHOUT A WIDTH IS WRITTEN ONCE. Only a width needs the length of
* what will be written before it is written, and only then is it measured:
* numbers are formatted into a local buffer first anyway, and text is counted
* by the same loop that converts it, run once without a stream.
*
*   NUMBERS NEED NO 64-BIT TYPE. A magnitude is divided as four 16-bit limbs
* in 32-bit arithmetic, so this unit gives the same digits on every target.
*
*
* path:      /src/djinterp/c/text/ostream_template.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/ostream_template.h"
// std
#include <errno.h>   // EINVAL
#include <limits.h>  // LONG_MAX, LONG_MIN
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memcmp, memcpy, strchr, strcspn, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // d_ostream_*
#include "../../../../inc/djinterp/c/text/ostream_format.h"   // doubles
#include "../../../../inc/djinterp/config/c/text/cfg_ostream_template.h"
                                                              // depth
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t, uint32_t,
                                                       // uint16_t, uint8_t


// D_INTERNAL_TEMPLATE_NUMBER_BUFFER
//   constant: the bytes an integer is formatted into: 64 binary digits, a
// sign and a two-character prefix, with room to spare.
#define D_INTERNAL_TEMPLATE_NUMBER_BUFFER  80

// D_INTERNAL_TEMPLATE_REAL_BUFFER
//   constant: the bytes a double is formatted into: DBL_MAX as %f has 309
// digits, and d_ostream_write_double keeps a precision to 64.
#define D_INTERNAL_TEMPLATE_REAL_BUFFER    400

// D_INTERNAL_TEMPLATE_NONE
//   constant: an open block with no {:} yet.
#define D_INTERNAL_TEMPLATE_NONE           0xFFFFFFFFu

// SPACES
//   constant: a run of spaces, so the usual padding is a memcpy.
static const char SPACES[] =
    "                                                                ";

// DIGITS_LOWER / DIGITS_UPPER
//   constant: the digits of every base up to 16.
static const char DIGITS_LOWER[17] = "0123456789abcdef";
static const char DIGITS_UPPER[17] = "0123456789ABCDEF";

// STATUS_NAMES
//   constant: the template statuses' names, by value.
static const char* const STATUS_NAMES[D_OSTREAM_TEMPLATE_STATUS_COUNT] =
{
    "D_OSTREAM_TEMPLATE_OK",
    "D_OSTREAM_TEMPLATE_INVALID",
    "D_OSTREAM_TEMPLATE_SYNTAX",
    "D_OSTREAM_TEMPLATE_UNKNOWN_FIELD",
    "D_OSTREAM_TEMPLATE_BAD_SPEC",
    "D_OSTREAM_TEMPLATE_BAD_CONVERSION",
    "D_OSTREAM_TEMPLATE_NESTING",
    "D_OSTREAM_TEMPLATE_CAPACITY"
};

/**
 * @brief Where converted text goes: onto a stream, or into a count.
 *
 * Converting and measuring are one loop, so the two cannot disagree on how
 * long a field is; with no stream, the loop counts code points instead.
 */
struct dj_internal_template_emitter
{
    struct dj_ostream* os;     // the stream, or null to count
    size_t             count;  // code points, when counting
};

/**
 * @brief An open block, while a template compiles.
 */
struct dj_internal_template_block
{
    uint32_t    op;       // its IF or UNLESS
    uint32_t    jump;     // its {:}'s JUMP, or D_INTERNAL_TEMPLATE_NONE
    const char* name;     // the field it tests, for {/name}
    size_t      size;     // the name's length
    size_t      open_at;  // its '{', for an unclosed block's error
};

/**
 * @brief A template being compiled.
 */
struct dj_internal_template_compiler
{
    const char*                       source;
    struct dj_ostream_template_op*    ops;
    size_t                            capacity;
    uint32_t                          count;     // operations needed so far
    dj_ostream_template_lookup_fn     lookup;
    void*                             context;
    struct dj_ostream_template_error* error;
    struct dj_internal_template_block blocks[D_CFG_OSTREAM_TEMPLATE_DEPTH];
    uint32_t                          depth;
};


//==============================================================================
// VALUES
//==============================================================================

/*
d_ostream_value_to_long
  The magnitude fits an unsigned long when its high half is zero or unsigned
long is wider than 32 bits; the second shift of 16 is then defined either way.
*/
long
d_ostream_value_to_long(
    const struct dj_ostream_value* _value
)
{
    // only integers and booleans have a number to give
    if (_value == NULL)
    {
        return 0L;
    }

    if (_value->kind == D_OSTREAM_VALUE_BOOLEAN)
    {
        return (_value->low != 0u) ? 1L : 0L;
    }

    if (_value->kind != D_OSTREAM_VALUE_INTEGER)
    {
        return 0L;
    }

    const unsigned long limit = (_value->negative != 0)
                                    ? ((unsigned long)LONG_MAX + 1ul)
                                    : (unsigned long)LONG_MAX;

    // a high half that unsigned long cannot hold is past either limit
    if ( (_value->high != 0u) &&
         (sizeof(unsigned long) <= 4u) )
    {
        return (_value->negative != 0) ? LONG_MIN : LONG_MAX;
    }

    const unsigned long magnitude =
        ((((unsigned long)_value->high) << 16) << 16) |
        (unsigned long)_value->low;

    // clamp, then negate in a way LONG_MIN's magnitude survives
    if (magnitude >= limit)
    {
        return (_value->negative != 0) ? LONG_MIN : LONG_MAX;
    }

    return (_value->negative != 0) ? -(long)magnitude
                                   : (long)magnitude;
}


//==============================================================================
// SPECIFICATIONS
//==============================================================================

/*
d_ostream_spec_init
  Every member, so a caller's uninitialised struct becomes the default.
*/
void
d_ostream_spec_init(
    struct dj_ostream_spec* _spec
)
{
    if (_spec != NULL)
    {
        _spec->width     = 0u;
        _spec->precision = (uint16_t)D_OSTREAM_SPEC_NO_PRECISION;
        _spec->fill      = ' ';
        _spec->align     = (uint8_t)D_OSTREAM_ALIGN_DEFAULT;
        _spec->convert   = (uint8_t)D_OSTREAM_CONVERT_RAW;
        _spec->sign      = (uint8_t)D_OSTREAM_SIGN_MINUS;
        _spec->type      = 0u;
        _spec->flags     = 0u;
    }

    return;
}

/**
 * @brief The alignment an align character names.
 *
 * @param[in] _c  the character.
 * @return the enum dj_ostream_align, or -1 for any other character.
 */
static int32_t
d_internal_template_align_of(
    char _c
)
{
    switch (_c)
    {
        case '<':
            return D_OSTREAM_ALIGN_LEFT;
        case '>':
            return D_OSTREAM_ALIGN_RIGHT;
        case '^':
            return D_OSTREAM_ALIGN_CENTER;
        default:
            return -1;
    }
}

/**
 * @brief Whether a character may begin a field's name.
 *
 * @param[in] _c  the character.
 * @return 1 or 0.
 */
static int32_t
d_internal_template_is_name_start(
    char _c
)
{
    return ( ((_c >= 'a') && (_c <= 'z')) ||
             ((_c >= 'A') && (_c <= 'Z')) ||
             (_c == '_') ) ? 1 : 0;
}

/**
 * @brief Whether a character may continue a field's name.
 *
 * @param[in] _c  the character.
 * @return 1 or 0.
 */
static int32_t
d_internal_template_is_name_char(
    char _c
)
{
    return ( (d_internal_template_is_name_start(_c) != 0) ||
             ((_c >= '0') && (_c <= '9')) ) ? 1 : 0;
}

/**
 * @brief The length of the name at the start of some text.
 *
 * @param[in] _text  the text.
 * @param[in] _size  its length.
 * @return the name's length; 0 when the text does not begin with one.
 */
static size_t
d_internal_template_name_length(
    const char* _text,
    size_t      _size
)
{
    size_t length = 0u;

    // a letter or underscore, then letters, digits and underscores
    if ( (_size == 0u) ||
         (d_internal_template_is_name_start(_text[0]) == 0) )
    {
        return 0u;
    }

    while ( (length < _size) &&
            (d_internal_template_is_name_char(_text[length]) != 0) )
    {
        ++length;
    }

    return length;
}

/**
 * @brief The conversion a name after '!' names.
 *
 * @param[in] _name  the name.
 * @param[in] _size  its length.
 * @return the enum dj_ostream_convert, or -1 for a name that is none.
 */
static int32_t
d_internal_template_convert_of(
    const char* _name,
    size_t      _size
)
{
    static const struct
    {
        const char* name;
        size_t      size;
        int32_t     convert;
    } CONVERSIONS[] =
    {
        { "raw",  3u, D_OSTREAM_CONVERT_RAW      },
        { "json", 4u, D_OSTREAM_CONVERT_JSON     },
        { "csv",  3u, D_OSTREAM_CONVERT_CSV      },
        { "md",   2u, D_OSTREAM_CONVERT_MARKDOWN },
        { "xml",  3u, D_OSTREAM_CONVERT_XML      }
    };
    // a handful of names: a linear search is the fastest there is
    for (size_t i = 0u; i < sizeof(CONVERSIONS) / sizeof(CONVERSIONS[0]); ++i)
    {
        if ( (CONVERSIONS[i].size == _size) &&
             (memcmp(CONVERSIONS[i].name, _name, _size) == 0) )
        {
            return CONVERSIONS[i].convert;
        }
    }

    return -1;
}

/**
 * @brief Reads a width or a precision: digits, or -- where fields are
 *        allowed -- a field's name in braces.
 *
 * @param[in]     _text     the specification.
 * @param[in]     _size     its length.
 * @param[in,out] _at       where to read; moved past what was read.
 * @param[out]    _number   the number read; untouched for a field.
 * @param[out]    _field    the field's index, or NO_FIELD for a number;
 *                          null when fields are not allowed.
 * @param[in]     _compiler the compiler, for its lookup; null with _field.
 * @return D_OSTREAM_TEMPLATE_OK, or why not; nothing read is OK, with
 *         *_at unmoved.
 */
static int32_t
d_internal_template_read_count(
    const char*                           _text,
    size_t                                _size,
    size_t*                               _at,
    uint16_t*                             _number,
    uint16_t*                             _field,
    struct dj_internal_template_compiler* _compiler
)
{
    size_t   at    = *_at;
    uint32_t value = 0u;

    // a field in braces: {name}
    if ( (at < _size) &&
         (_text[at] == '{') )
    {
        const size_t length = d_internal_template_name_length(_text + at + 1u,
                                                             _size - at - 1u);

        if ( (_field == NULL)            ||
             (_compiler == NULL)         ||
             (length == 0u)              ||
             (at + 1u + length >= _size) ||
             (_text[at + 1u + length] != '}') )
        {
            return D_OSTREAM_TEMPLATE_BAD_SPEC;
        }

        if (_compiler->lookup == NULL)
        {
            return D_OSTREAM_TEMPLATE_UNKNOWN_FIELD;
        }

        const int32_t index = _compiler->lookup(_compiler->context,
                                                _text + at + 1u,
                                                length);

        if (index < 0)
        {
            return D_OSTREAM_TEMPLATE_UNKNOWN_FIELD;
        }

        if ((uint32_t)index >= D_OSTREAM_TEMPLATE_NO_FIELD)
        {
            return D_OSTREAM_TEMPLATE_INVALID;
        }

        *_field = (uint16_t)index;
        *_at    = at + length + 2u;

        return D_OSTREAM_TEMPLATE_OK;
    }

    // digits, up to the largest width allowed
    while ( (at < _size) &&
            (_text[at] >= '0') &&
            (_text[at] <= '9') )
    {
        value = (value * 10u) + (uint32_t)(_text[at] - '0');

        if (value > D_OSTREAM_SPEC_MAX_WIDTH)
        {
            return D_OSTREAM_TEMPLATE_BAD_SPEC;
        }

        ++at;
    }

    if (at != *_at)
    {
        *_number = (uint16_t)value;
        *_at     = at;
    }

    return D_OSTREAM_TEMPLATE_OK;
}

/**
 * @brief Reads a specification's [[fill]align][sign][#][0].
 *
 * @param[in]     _text  the specification, without its ':'.
 * @param[in]     _size  its length.
 * @param[in,out] _spec  its fill, align, sign and flags are set as read.
 * @param[out]    _at    where reading stopped; 0 for a bad fill.
 * @return D_OSTREAM_TEMPLATE_OK, or D_OSTREAM_TEMPLATE_BAD_SPEC for a fill
 *         that is a brace or not printable ASCII.
 *
 * A fill is any ASCII character but a brace or a control, so a fill and an
 * align are told apart by the second character. '-' is the default sign, so
 * it is read and changes nothing.
 */
static int32_t
d_internal_template_read_flags(
    const char*             _text,
    size_t                  _size,
    struct dj_ostream_spec* _spec,
    size_t*                 _at
)
{
    size_t at = 0u;

    // [[fill]align]
    if ( (_size >= 2u) &&
         (d_internal_template_align_of(_text[1]) >= 0) )
    {
        const unsigned char fill = (unsigned char)_text[0];

        if ( (fill < 0x20u) ||
             (fill >= 0x7Fu) ||
             (fill == '{')   ||
             (fill == '}') )
        {
            *_at = 0u;

            return D_OSTREAM_TEMPLATE_BAD_SPEC;
        }

        _spec->fill  = (char)fill;
        _spec->align = (uint8_t)d_internal_template_align_of(_text[1]);
        at = 2u;
    }
    else if ( (_size >= 1u) &&
              (d_internal_template_align_of(_text[0]) >= 0) )
    {
        _spec->align = (uint8_t)d_internal_template_align_of(_text[0]);
        at = 1u;
    }

    // [sign]
    if (at < _size)
    {
        if (_text[at] == '+')
        {
            _spec->sign = (uint8_t)D_OSTREAM_SIGN_PLUS;
            ++at;
        }
        else if (_text[at] == ' ')
        {
            _spec->sign = (uint8_t)D_OSTREAM_SIGN_SPACE;
            ++at;
        }
        else if (_text[at] == '-')
        {
            ++at;
        }
    }

    // [#][0]
    if ( (at < _size) &&
         (_text[at] == '#') )
    {
        _spec->flags = (uint8_t)(_spec->flags | D_OSTREAM_SPEC_ALTERNATE);
        ++at;
    }

    if ( (at < _size) &&
         (_text[at] == '0') )
    {
        _spec->flags = (uint8_t)(_spec->flags | D_OSTREAM_SPEC_ZERO);
        ++at;
    }

    *_at = at;

    return D_OSTREAM_TEMPLATE_OK;
}

/**
 * @brief Reads the part of a specification after ':'.
 *
 * @param[in]     _text      the specification, without its ':'.
 * @param[in]     _size      its length.
 * @param[out]    _spec      filled in.
 * @param[out]    _width     the width's field, or NO_FIELD; null when fields
 *                           are not allowed.
 * @param[out]    _precision the precision's field, or NO_FIELD; as _width.
 * @param[in]     _compiler  the compiler, for its lookup; null with them.
 * @param[out]    _stop      where reading stopped, for an error's offset.
 * @return D_OSTREAM_TEMPLATE_OK, or why not.
 *
 * The grammar is std::format's: [[fill]align][sign][#][0][width]
 * [.precision][type]; d_internal_template_read_flags reads up to the width.
 */
static int32_t
d_internal_template_read_spec(
    const char*                           _text,
    size_t                                _size,
    struct dj_ostream_spec*               _spec,
    uint16_t*                             _width,
    uint16_t*                             _precision,
    struct dj_internal_template_compiler* _compiler,
    size_t*                               _stop
)
{
    size_t  at     = 0u;
    int32_t status = d_internal_template_read_flags(_text,
                                                    _size,
                                                    _spec,
                                                    &at);

    // [width][.precision]
    if (status == D_OSTREAM_TEMPLATE_OK)
    {
        status = d_internal_template_read_count(_text,
                                                _size,
                                                &at,
                                                &_spec->width,
                                                _width,
                                                _compiler);
    }

    if ( (status == D_OSTREAM_TEMPLATE_OK) &&
         (at < _size) &&
         (_text[at] == '.') )
    {
        const size_t dot = ++at;

        status = d_internal_template_read_count(_text,
                                                _size,
                                                &at,
                                                &_spec->precision,
                                                _precision,
                                                _compiler);

        // a '.' with nothing after it is not a precision
        if ( (status == D_OSTREAM_TEMPLATE_OK) &&
             (at == dot) )
        {
            status = D_OSTREAM_TEMPLATE_BAD_SPEC;
        }
    }

    // [type], and then nothing
    if ( (status == D_OSTREAM_TEMPLATE_OK) &&
         (at < _size) &&
         (_text[at] != '\0') &&
         (strchr("dxXobefgs", _text[at]) != NULL) )
    {
        _spec->type = (uint8_t)_text[at];
        ++at;
    }

    if ( (status == D_OSTREAM_TEMPLATE_OK) &&
         (at != _size) )
    {
        status = D_OSTREAM_TEMPLATE_BAD_SPEC;
    }

    *_stop = at;

    return status;
}

/*
d_ostream_spec_parse
  The template's own readers, with fields refused, over a copy of the
default so a failure leaves _spec at it.
*/
int32_t
d_ostream_spec_parse(
    struct dj_ostream_spec* _spec,
    const char*             _text,
    size_t                  _size
)
{
    struct dj_ostream_spec spec;
    size_t                 at = 0u;
    size_t                 stop = 0u;

    if ( (_spec == NULL) ||
         ( (_text == NULL) &&
           (_size != 0u) ) )
    {
        return D_OSTREAM_TEMPLATE_INVALID;
    }

    d_ostream_spec_init(_spec);
    d_ostream_spec_init(&spec);

    // !conv, a name up to ':' or the end
    if ( (_size > 0u) &&
         (_text[0] == '!') )
    {
        const size_t  length  = d_internal_template_name_length(_text + 1u,
                                                               _size - 1u);
        const int32_t convert = d_internal_template_convert_of(_text + 1u,
                                                              length);

        if (convert < 0)
        {
            return D_OSTREAM_TEMPLATE_BAD_CONVERSION;
        }

        spec.convert = (uint8_t)convert;
        at = 1u + length;
    }

    // :spec, the rest
    if (at < _size)
    {
        if (_text[at] != ':')
        {
            return D_OSTREAM_TEMPLATE_BAD_SPEC;
        }

        const int32_t status = d_internal_template_read_spec(_text + at + 1u,
                                                            _size - at - 1u,
                                                            &spec,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            &stop);

        if (status != D_OSTREAM_TEMPLATE_OK)
        {
            return status;
        }
    }

    *_spec = spec;

    return D_OSTREAM_TEMPLATE_OK;
}


//==============================================================================
// CONVERSIONS
//==============================================================================

/**
 * @brief Writes or counts a run of bytes that need no conversion.
 *
 * @param[in,out] _emitter  where they go.
 * @param[in]     _text     the bytes.
 * @param[in]     _size     how many.
 */
static void
d_internal_template_emit(
    struct dj_internal_template_emitter* _emitter,
    const char*                          _text,
    size_t                               _size
)
{
    // a stream takes the run at once; a count wants its code points
    if (_emitter->os != NULL)
    {
        (void)d_ostream_write(_emitter->os, _text, _size);

        return;
    }

    for (size_t i = 0u; i < _size; ++i)
    {
        if ((((unsigned char)_text[i]) & 0xC0u) != 0x80u)
        {
            ++_emitter->count;
        }
    }

    return;
}

/**
 * @brief What one byte becomes under a conversion.
 *
 * @param[in]  _convert  the conversion.
 * @param[in]  _c        the byte.
 * @param[out] _buffer   room for a JSON escape of six characters.
 * @param[out] _size     the replacement's length.
 * @return the replacement, or null when the byte stays as it is.
 */
static const char*
d_internal_template_replacement(
    int32_t       _convert,
    unsigned char _c,
    char          _buffer[8],
    size_t*       _size
)
{
    const char* text = NULL;

    switch (_convert)
    {
        case D_OSTREAM_CONVERT_JSON:
            switch (_c)
            {
                case '"':  text = "\\\""; break;
                case '\\': text = "\\\\"; break;
                case '\b': text = "\\b";  break;
                case '\f': text = "\\f";  break;
                case '\n': text = "\\n";  break;
                case '\r': text = "\\r";  break;
                case '\t': text = "\\t";  break;
                default:
                    // every other control is \u00XX
                    if (_c < 0x20u)
                    {
                        _buffer[0] = '\\';
                        _buffer[1] = 'u';
                        _buffer[2] = '0';
                        _buffer[3] = '0';
                        _buffer[4] = DIGITS_LOWER[_c >> 4];
                        _buffer[5] = DIGITS_LOWER[_c & 0x0Fu];
                        *_size     = 6u;

                        return _buffer;
                    }
                    break;
            }
            break;
        case D_OSTREAM_CONVERT_CSV:
            // inside the quotes a field is wrapped in, a quote is doubled
            if (_c == '"')
            {
                text = "\"\"";
            }
            break;
        case D_OSTREAM_CONVERT_MARKDOWN:
            switch (_c)
            {
                case '\\': text = "\\\\"; break;
                case '`':  text = "\\`";  break;
                case '*':  text = "\\*";  break;
                case '_':  text = "\\_";  break;
                case '[':  text = "\\[";  break;
                case ']':  text = "\\]";  break;
                case '<':  text = "\\<";  break;
                case '>':  text = "\\>";  break;
                case '|':  text = "\\|";  break;
                case '~':  text = "\\~";  break;
                case '\n':
                case '\r': text = " ";    break;
                default:                  break;
            }
            break;
        case D_OSTREAM_CONVERT_XML:
            switch (_c)
            {
                case '&':  text = "&amp;";  break;
                case '<':  text = "&lt;";   break;
                case '>':  text = "&gt;";   break;
                case '"':  text = "&quot;"; break;
                case '\'': text = "&#39;";  break;
                default:
                    // XML 1.0 has no other controls: U+FFFD stands in
                    if ( (_c < 0x20u)  &&
                         (_c != '\t') &&
                         (_c != '\n') &&
                         (_c != '\r') )
                    {
                        text = "\xEF\xBF\xBD";
                    }
                    break;
            }
            break;
        default:
            break;
    }

    if (text != NULL)
    {
        *_size = strlen(text);
    }

    return text;
}

/**
 * @brief Whether a CSV field must be quoted: it holds a separator, a quote
 *        or a line break, or begins or ends with a space.
 *
 * @param[in] _text  the field.
 * @param[in] _size  its length.
 * @return 1 or 0.
 */
static int32_t
d_internal_template_csv_quoted(
    const char* _text,
    size_t      _size
)
{
    if ( (_size > 0u) &&
         ( (_text[0] == ' ')          ||
           (_text[0] == '\t')         ||
           (_text[_size - 1u] == ' ') ||
           (_text[_size - 1u] == '\t') ) )
    {
        return 1;
    }

    for (size_t i = 0u; i < _size; ++i)
    {
        if ( (_text[i] == ',')  ||
             (_text[i] == '"')  ||
             (_text[i] == '\n') ||
             (_text[i] == '\r') )
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Writes or counts text under a conversion: runs that stay as they
 *        are go at once, and each byte that changes goes as its replacement.
 *
 * @param[in,out] _emitter  where it goes.
 * @param[in]     _convert  the conversion.
 * @param[in]     _text     the text.
 * @param[in]     _size     its length.
 */
static void
d_internal_template_convert_text(
    struct dj_internal_template_emitter* _emitter,
    int32_t                              _convert,
    const char*                          _text,
    size_t                               _size
)
{
    const int32_t quoted = ( (_convert == D_OSTREAM_CONVERT_JSON) ||
                             ( (_convert == D_OSTREAM_CONVERT_CSV) &&
                               (d_internal_template_csv_quoted(_text, _size)
                                != 0) ) ) ? 1 : 0;
    char          buffer[8];
    size_t        run = 0u;

    // raw text is one run
    if (_convert == D_OSTREAM_CONVERT_RAW)
    {
        d_internal_template_emit(_emitter, _text, _size);

        return;
    }

    if (quoted != 0)
    {
        d_internal_template_emit(_emitter, "\"", 1u);
    }

    for (size_t i = 0u; i < _size; ++i)
    {
        size_t            length      = 0u;
        const char* const replacement =
            d_internal_template_replacement(_convert,
                                            (unsigned char)_text[i],
                                            buffer,
                                            &length);

        // the run so far, then the byte's replacement
        if (replacement != NULL)
        {
            d_internal_template_emit(_emitter, _text + run, i - run);
            d_internal_template_emit(_emitter, replacement, length);
            run = i + 1u;
        }
    }

    d_internal_template_emit(_emitter, _text + run, _size - run);

    if (quoted != 0)
    {
        d_internal_template_emit(_emitter, "\"", 1u);
    }

    return;
}

/**
 * @brief The bytes of the first _count code points of some text.
 *
 * @param[in] _text   the text.
 * @param[in] _size   its length.
 * @param[in] _count  the code points to keep.
 * @return their length in bytes.
 */
static size_t
d_internal_template_prefix(
    const char* _text,
    size_t      _size,
    size_t      _count
)
{
    size_t kept = 0u;

    // stop at the lead byte of the code point one past the last kept
    for (size_t i = 0u; i < _size; ++i)
    {
        if ((((unsigned char)_text[i]) & 0xC0u) != 0x80u)
        {
            if (kept == _count)
            {
                return i;
            }

            ++kept;
        }
    }

    return _size;
}


//==============================================================================
// WRITING ONE VALUE
//==============================================================================

/**
 * @brief Writes padding around a body of a known width.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _spec   the specification: fill and width.
 * @param[in]     _align  the alignment in effect, never DEFAULT.
 * @param[in]     _width  the body's width in code points.
 * @param[in]     _left   1 for the padding before the body, 0 after.
 */
static void
d_internal_template_pad(
    struct dj_ostream*            _os,
    const struct dj_ostream_spec* _spec,
    int32_t                       _align,
    size_t                        _width,
    int32_t                       _left
)
{
    const size_t pad = (_spec->width > _width)
                           ? ((size_t)_spec->width - _width)
                           : 0u;
    size_t       count;

    // left: all of it before; right: all of it after; center: split
    if (pad == 0u)
    {
        return;
    }

    switch (_align)
    {
        case D_OSTREAM_ALIGN_RIGHT:
            count = (_left != 0) ? pad : 0u;
            break;
        case D_OSTREAM_ALIGN_CENTER:
            count = (_left != 0) ? (pad / 2u) : (pad - (pad / 2u));
            break;
        default:
            count = (_left != 0) ? 0u : pad;
            break;
    }

    // spaces from the table; any other fill, or a long run, by fill
    if ( (_spec->fill == ' ') &&
         (count < sizeof(SPACES)) )
    {
        (void)d_ostream_write(_os, SPACES, count);
    }
    else if (count > 0u)
    {
        (void)d_ostream_fill(_os, _spec->fill, count);
    }

    return;
}

/**
 * @brief Writes an ASCII body -- a number's or a word's -- with its padding.
 *
 * @param[in,out] _os       the stream.
 * @param[in]     _spec     the specification.
 * @param[in]     _numeric  whether the body is a number: aligned right by
 *                          default, and zero-padded on request.
 * @param[in]     _body     the body.
 * @param[in]     _size     its length.
 * @param[in]     _lead     the bytes of sign and prefix a zero pad goes after.
 */
static void
d_internal_template_write_body(
    struct dj_ostream*            _os,
    const struct dj_ostream_spec* _spec,
    int32_t                       _numeric,
    const char*                   _body,
    size_t                        _size,
    size_t                        _lead
)
{
    int32_t align = (int32_t)_spec->align;

    // zeros go between the sign and the digits, and only with no alignment
    if ( (_numeric != 0)                                 &&
         ((_spec->flags & D_OSTREAM_SPEC_ZERO) != 0u)    &&
         (align == D_OSTREAM_ALIGN_DEFAULT)              &&
         ((size_t)_spec->width > _size) )
    {
        (void)d_ostream_write(_os, _body, _lead);
        (void)d_ostream_fill(_os, '0', (size_t)_spec->width - _size);
        (void)d_ostream_write(_os, _body + _lead, _size - _lead);

        return;
    }

    if (align == D_OSTREAM_ALIGN_DEFAULT)
    {
        align = (_numeric != 0) ? D_OSTREAM_ALIGN_RIGHT
                                : D_OSTREAM_ALIGN_LEFT;
    }

    d_internal_template_pad(_os, _spec, align, _size, 1);
    (void)d_ostream_write(_os, _body, _size);
    d_internal_template_pad(_os, _spec, align, _size, 0);

    return;
}

/**
 * @brief Formats a magnitude's digits, backwards from the end of a buffer.
 *
 * @param[in] _end    one past where the last digit goes.
 * @param[in] _high   the magnitude's high half.
 * @param[in] _low    its low half.
 * @param[in] _base   2, 8, 10 or 16.
 * @param[in] _upper  1 for upper-case hexadecimal.
 * @return the digits' count; at least 1.
 *
 * The magnitude is four 16-bit limbs. Each step divides all four by the base,
 * most significant first, carrying each remainder into the next limb, so no
 * intermediate is wider than 32 bits; the last remainder is the next digit.
 */
static size_t
d_internal_template_digits(
    char*    _end,
    uint32_t _high,
    uint32_t _low,
    uint32_t _base,
    int32_t  _upper
)
{
    const char* const digits = (_upper != 0) ? DIGITS_UPPER : DIGITS_LOWER;
    uint32_t          limbs[4];
    size_t            count = 0u;

    // a magnitude that fits 32 bits, the usual one, divides directly
    if (_high == 0u)
    {
        uint32_t value = _low;

        do
        {
            *--_end = digits[value % _base];
            value  /= _base;
            ++count;
        } while (value != 0u);

        return count;
    }

    limbs[0] = _high >> 16;
    limbs[1] = _high & 0xFFFFu;
    limbs[2] = _low >> 16;
    limbs[3] = _low & 0xFFFFu;

    // one digit per pass, until every limb is zero
    do
    {
        uint32_t remainder = 0u;

        for (size_t i = 0u; i < 4u; ++i)
        {
            const uint32_t current = (remainder << 16) | limbs[i];

            limbs[i]  = current / _base;
            remainder = current % _base;
        }

        *--_end = digits[remainder];
        ++count;
    } while ((limbs[0] | limbs[1] | limbs[2] | limbs[3]) != 0u);

    return count;
}

/**
 * @brief Writes an integer: sign, prefix and digits, padded.
 *
 * @param[in,out] _os        the stream.
 * @param[in]     _value     the value: INTEGER, or BOOLEAN as 0 or 1.
 * @param[in]     _spec      the specification.
 */
static void
d_internal_template_write_integer(
    struct dj_ostream*             _os,
    const struct dj_ostream_value* _value,
    const struct dj_ostream_spec*  _spec
)
{
    char           buffer[D_INTERNAL_TEMPLATE_NUMBER_BUFFER];
    char* const    end     = buffer + sizeof(buffer);
    const int32_t  json    = (_spec->convert == D_OSTREAM_CONVERT_JSON);
    const int32_t  type    = (json != 0) ? 'd' : (int32_t)_spec->type;
    uint32_t       base    = 10u;
    const char*    prefix  = "";
    struct dj_ostream_spec spec = *_spec;

    // the base, and the prefix '#' asks for
    switch (type)
    {
        case 'x':
        case 'X':
            base   = 16u;
            prefix = (type == 'x') ? "0x" : "0X";
            break;
        case 'o':
            base   = 8u;
            prefix = "0o";
            break;
        case 'b':
            base   = 2u;
            prefix = "0b";
            break;
        default:
            break;
    }

    if ((_spec->flags & D_OSTREAM_SPEC_ALTERNATE) == 0u)
    {
        prefix = "";
    }

    const size_t count = d_internal_template_digits(
                             end,
                             (_value->kind == D_OSTREAM_VALUE_INTEGER)
                                 ? _value->high
                                 : 0u,
                             _value->low,
                             base,
                             (type == 'X') ? 1 : 0);
    char*        start = end - count;

    // a JSON number has no '+', no prefix and no leading zeros
    if (json != 0)
    {
        prefix     = "";
        spec.flags = 0u;
        spec.sign  = (uint8_t)D_OSTREAM_SIGN_MINUS;
    }

    const size_t prefix_size = strlen(prefix);

    start -= prefix_size;
    memcpy(start, prefix, prefix_size);

    if ( (_value->kind == D_OSTREAM_VALUE_INTEGER) &&
         (_value->negative != 0) )
    {
        *--start = '-';
    }
    else if (spec.sign == D_OSTREAM_SIGN_PLUS)
    {
        *--start = '+';
    }
    else if (spec.sign == D_OSTREAM_SIGN_SPACE)
    {
        *--start = ' ';
    }

    d_internal_template_write_body(_os,
                                   &spec,
                                   1,
                                   start,
                                   (size_t)(end - start),
                                   (size_t)(end - start) - count);

    return;
}

/**
 * @brief Writes a double: through d_ostream_write_double into a buffer, so
 *        its length is known before the padding.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @param[in]     _spec   the specification.
 */
static void
d_internal_template_write_real(
    struct dj_ostream*            _os,
    double                        _value,
    const struct dj_ostream_spec* _spec
)
{
    char                    buffer[D_INTERNAL_TEMPLATE_REAL_BUFFER];
    struct dj_ostream       local;
    struct dj_ostream_fixed fixed;
    struct dj_ostream_spec  spec  = *_spec;
    const int32_t           json  = (_spec->convert == D_OSTREAM_CONVERT_JSON);
    char                    style = 'g';
    int32_t                 precision = -1;
    size_t                  lead  = 0u;

    // printf's style, and the precision: JSON's default round-trips
    if ( (_spec->type == 'f') ||
         (_spec->type == 'e') )
    {
        style = (char)_spec->type;
    }

    if (_spec->precision != D_OSTREAM_SPEC_NO_PRECISION)
    {
        precision = (int32_t)_spec->precision;
    }
    else if (json != 0)
    {
        precision = 17;
    }

    // one byte kept in front for a '+' or ' ' sign
    (void)d_ostream_init_fixed(&local, &fixed, buffer + 1, sizeof(buffer) - 1u,
                               0);
    (void)d_ostream_write_double(&local, _value, style, precision);
    (void)d_ostream_close(&local);

    size_t      size = d_ostream_fixed_length(&fixed);
    const char* body = buffer + 1;

    if ( (d_ostream_fixed_truncated(&fixed) != 0) ||
         (size == 0u) )
    {
        (void)d_ostream_fail(_os, D_OSTREAM_FAILED, 0);

        return;
    }

    // nan and the infinities: null in JSON, and never zero-padded
    const int32_t finite = ( (body[(body[0] == '-') ? 1 : 0] >= '0') &&
                             (body[(body[0] == '-') ? 1 : 0] <= '9') );

    if (finite == 0)
    {
        if (json != 0)
        {
            spec.flags = 0u;
            d_internal_template_write_body(_os, &spec, 0, "null", 4u, 0u);

            return;
        }

        spec.flags = (uint8_t)(spec.flags & ~D_OSTREAM_SPEC_ZERO);
    }

    if (body[0] == '-')
    {
        lead = 1u;
    }
    else if ( (json == 0) &&
              (spec.sign != D_OSTREAM_SIGN_MINUS) )
    {
        buffer[0] = (spec.sign == D_OSTREAM_SIGN_PLUS) ? '+' : ' ';
        body      = buffer;
        ++size;
        lead      = 1u;
    }

    if (json != 0)
    {
        spec.flags = 0u;
    }

    d_internal_template_write_body(_os, &spec, 1, body, size, lead);

    return;
}

/**
 * @brief Writes text: cut to its precision, converted, padded.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _text   the text.
 * @param[in]     _size   its length.
 * @param[in]     _spec   the specification.
 *
 * Raw text, the usual case, is written once and its width counted in place;
 * a conversion's width is counted by running the conversion without a
 * stream, so the two cannot disagree.
 */
static void
d_internal_template_write_text(
    struct dj_ostream*            _os,
    const char*                   _text,
    size_t                        _size,
    const struct dj_ostream_spec* _spec
)
{
    struct dj_internal_template_emitter emitter;
    const int32_t                       align = (_spec->align ==
                                                 D_OSTREAM_ALIGN_DEFAULT)
                                                   ? D_OSTREAM_ALIGN_LEFT
                                                   : (int32_t)_spec->align;
    size_t                             size  = _size;
    size_t                             width = 0u;

    // precision cuts the text itself, before it is converted
    if (_spec->precision != D_OSTREAM_SPEC_NO_PRECISION)
    {
        size = d_internal_template_prefix(_text, _size, _spec->precision);
    }

    // raw text with no width, the commonest field of all, is one write
    if ( (_spec->width == 0u) &&
         (_spec->convert == D_OSTREAM_CONVERT_RAW) )
    {
        (void)d_ostream_write(_os, _text, size);

        return;
    }

    emitter.os    = _os;
    emitter.count = 0u;

    // no width: one pass, straight onto the stream
    if (_spec->width == 0u)
    {
        d_internal_template_convert_text(&emitter,
                                         (int32_t)_spec->convert,
                                         _text,
                                         size);

        return;
    }

    if (_spec->convert == D_OSTREAM_CONVERT_RAW)
    {
        for (size_t i = 0u; i < size; ++i)
        {
            width += ((((unsigned char)_text[i]) & 0xC0u) != 0x80u) ? 1u : 0u;
        }
    }
    else
    {
        struct dj_internal_template_emitter counter;

        counter.os    = NULL;
        counter.count = 0u;
        d_internal_template_convert_text(&counter,
                                         (int32_t)_spec->convert,
                                         _text,
                                         size);
        width = counter.count;
    }

    d_internal_template_pad(_os, _spec, align, width, 1);
    d_internal_template_convert_text(&emitter,
                                     (int32_t)_spec->convert,
                                     _text,
                                     size);
    d_internal_template_pad(_os, _spec, align, width, 0);

    return;
}

/**
 * @brief Writes one value as a specification says, both given.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @param[in]     _spec   the specification.
 *
 * One branch per kind. A boolean with an integer type letter is the integer
 * 0 or 1; without one it is a word, which JSON spells as JSON does.
 */
static void
d_internal_template_write(
    struct dj_ostream*             _os,
    const struct dj_ostream_value* _value,
    const struct dj_ostream_spec*  _spec
)
{
    const int32_t json = (_spec->convert == D_OSTREAM_CONVERT_JSON);

    switch (_value->kind)
    {
        case D_OSTREAM_VALUE_TEXT:
            d_internal_template_write_text(_os,
                                           _value->text,
                                           _value->size,
                                           _spec);
            break;
        case D_OSTREAM_VALUE_INTEGER:
            d_internal_template_write_integer(_os, _value, _spec);
            break;
        case D_OSTREAM_VALUE_REAL:
            d_internal_template_write_real(_os, _value->real, _spec);
            break;
        case D_OSTREAM_VALUE_BOOLEAN:
            // a number when the type asks for one, else a word
            if ( (json == 0)          &&
                 (_spec->type != 0u)  &&
                 (_spec->type != 's') &&
                 (strchr("dxXob", (int)_spec->type) != NULL) )
            {
                d_internal_template_write_integer(_os, _value, _spec);
            }
            else
            {
                d_internal_template_write_body(_os,
                                               _spec,
                                               0,
                                               (_value->low != 0u) ? "true"
                                                                   : "false",
                                               (_value->low != 0u) ? 4u : 5u,
                                               0u);
            }
            break;
        case D_OSTREAM_VALUE_NONE:
            d_internal_template_write_body(_os,
                                           _spec,
                                           0,
                                           (json != 0) ? "null" : "",
                                           (json != 0) ? 4u : 0u,
                                           0u);
            break;
        default:
            (void)d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
            break;
    }

    return;
}

/*
d_ostream_write_value
  The defaults stand in for what was not given; the writing is the template's
own.
*/
int32_t
d_ostream_write_value(
    struct dj_ostream*             _os,
    const struct dj_ostream_value* _value,
    const struct dj_ostream_spec*  _spec
)
{
    struct dj_ostream_spec        defaults;
    const struct dj_ostream_value none = d_ostream_value_none();

    if (_os == NULL)
    {
        return 0;
    }

    d_ostream_spec_init(&defaults);
    d_internal_template_write(_os,
                              (_value != NULL) ? _value : &none,
                              (_spec != NULL) ? _spec : &defaults);

    return d_ostream_is_ok(_os);
}


//==============================================================================
// COMPILING
//==============================================================================

/**
 * @brief Records a compile error, once: the first is the one reported.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _status    why.
 * @param[in]     _offset    where.
 * @return _status, so a caller can return it.
 */
static int32_t
d_internal_template_error(
    struct dj_internal_template_compiler* _compiler,
    int32_t                               _status,
    size_t                                _offset
)
{
    if ( (_compiler->error != NULL) &&
         (_compiler->error->status == D_OSTREAM_TEMPLATE_OK) )
    {
        _compiler->error->status = _status;
        _compiler->error->offset = _offset;
    }

    return _status;
}

/**
 * @brief Appends an operation, or only counts it when there is no room, so
 *        a template too large still reports the room it needs.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _op        the operation.
 * @return its index.
 */
static uint32_t
d_internal_template_append(
    struct dj_internal_template_compiler* _compiler,
    const struct dj_ostream_template_op*  _op
)
{
    const uint32_t index = _compiler->count;

    if ((size_t)index < _compiler->capacity)
    {
        _compiler->ops[index] = *_op;
    }

    ++_compiler->count;

    return index;
}

/**
 * @brief Sets a jump's target, if the jump was kept.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _index     the jump's index.
 * @param[in]     _target    where it goes.
 */
static void
d_internal_template_patch(
    struct dj_internal_template_compiler* _compiler,
    uint32_t                              _index,
    uint32_t                              _target
)
{
    if ((size_t)_index < _compiler->capacity)
    {
        _compiler->ops[_index].extent = _target;
    }

    return;
}

/**
 * @brief A blank operation of a code.
 *
 * @param[in] _code  the code.
 * @return the operation.
 */
static struct dj_ostream_template_op
d_internal_template_op(
    int32_t _code
)
{
    struct dj_ostream_template_op op;

    op.operand         = 0u;
    op.extent          = 0u;
    d_ostream_spec_init(&op.spec);
    op.width_field     = (uint16_t)D_OSTREAM_TEMPLATE_NO_FIELD;
    op.precision_field = (uint16_t)D_OSTREAM_TEMPLATE_NO_FIELD;
    op.code            = (uint8_t)_code;
    op.reserved        = 0u;

    return op;
}

/**
 * @brief Appends a literal for a range of the source, unless it is empty.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _start     its first byte.
 * @param[in]     _end       one past its last.
 */
static void
d_internal_template_literal(
    struct dj_internal_template_compiler* _compiler,
    size_t                                _start,
    size_t                                _end
)
{
    if (_end > _start)
    {
        struct dj_ostream_template_op op =
            d_internal_template_op(D_OSTREAM_TEMPLATE_LITERAL);

        op.operand = (uint32_t)_start;
        op.extent  = (uint32_t)(_end - _start);
        (void)d_internal_template_append(_compiler, &op);
    }

    return;
}

/**
 * @brief A field's index, through the lookup.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _name      the name.
 * @param[in]     _size      its length.
 * @param[out]    _index     the index.
 * @return D_OSTREAM_TEMPLATE_OK, or why not, recorded at the name.
 */
static int32_t
d_internal_template_find(
    struct dj_internal_template_compiler* _compiler,
    const char*                           _name,
    size_t                                _size,
    uint32_t*                             _index
)
{
    const size_t offset = (size_t)(_name - _compiler->source);

    if (_compiler->lookup == NULL)
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_UNKNOWN_FIELD,
                                         offset);
    }

    const int32_t index = _compiler->lookup(_compiler->context, _name, _size);

    if (index < 0)
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_UNKNOWN_FIELD,
                                         offset);
    }

    if ((uint32_t)index >= D_OSTREAM_TEMPLATE_NO_FIELD)
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_INVALID,
                                         offset);
    }

    *_index = (uint32_t)index;

    return D_OSTREAM_TEMPLATE_OK;
}

/**
 * @brief Opens a block: {?name} or {^name}.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _kind      '?' or '^'.
 * @param[in]     _name      the field's name.
 * @param[in]     _length    its length; 0 is a syntax error.
 * @param[in]     _open_at   the tag's '{'.
 * @return D_OSTREAM_TEMPLATE_OK, or why not.
 *
 * The IF or UNLESS goes in with no target; the block's {:} or {/} gives it
 * one. The compiler's stack keeps the block's name for {/name} to match.
 */
static int32_t
d_internal_template_block_open(
    struct dj_internal_template_compiler* _compiler,
    char                                  _kind,
    const char*                           _name,
    size_t                                _length,
    size_t                                _open_at
)
{
    struct dj_ostream_template_op op =
        d_internal_template_op( (_kind == '?') ? D_OSTREAM_TEMPLATE_IF
                                               : D_OSTREAM_TEMPLATE_UNLESS );

    if ( (_length == 0u) ||
         (_compiler->depth >= (uint32_t)D_CFG_OSTREAM_TEMPLATE_DEPTH) )
    {
        return d_internal_template_error(_compiler,
                                         (_length == 0u)
                                             ? D_OSTREAM_TEMPLATE_SYNTAX
                                             : D_OSTREAM_TEMPLATE_NESTING,
                                         _open_at);
    }

    if (d_internal_template_find(_compiler,
                                 _name,
                                 _length,
                                 &op.operand) != D_OSTREAM_TEMPLATE_OK)
    {
        return D_OSTREAM_TEMPLATE_UNKNOWN_FIELD;
    }

    struct dj_internal_template_block* const block =
        &_compiler->blocks[_compiler->depth++];

    block->op      = d_internal_template_append(_compiler, &op);
    block->jump    = D_INTERNAL_TEMPLATE_NONE;
    block->name    = _name;
    block->size    = _length;
    block->open_at = _open_at;

    return D_OSTREAM_TEMPLATE_OK;
}

/**
 * @brief Compiles a block's tag: {?name}, {^name}, {:} or {/name}.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _tag       the tag's text, between its braces.
 * @param[in]     _size      its length.
 * @param[in]     _open_at   its '{'.
 * @return D_OSTREAM_TEMPLATE_OK, or why not.
 *
 * An IF or UNLESS is appended with no target; {:} appends a JUMP and points
 * the IF past it; {/} points whichever came last -- the JUMP, or the IF when
 * there was no {:} -- past everything so far. Every jump is forward.
 */
static int32_t
d_internal_template_block(
    struct dj_internal_template_compiler* _compiler,
    const char*                           _tag,
    size_t                                _size,
    size_t                                _open_at
)
{
    const char   kind   = _tag[0];
    const size_t length = d_internal_template_name_length(_tag + 1,
                                                          _size - 1u);

    // the name, where there is one, is the whole rest of the tag
    if ( ( (kind == '?') ||
           (kind == '^') ||
           (kind == '/') ) &&
         (length != _size - 1u) )
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_SYNTAX,
                                         _open_at);
    }

    if ( (kind == '?') ||
         (kind == '^') )
    {
        return d_internal_template_block_open(_compiler,
                                              kind,
                                              _tag + 1,
                                              length,
                                              _open_at);
    }

    // {:} and {/...} need an open block
    if (_compiler->depth == 0u)
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_NESTING,
                                         _open_at);
    }

    struct dj_internal_template_block* const block =
        &_compiler->blocks[_compiler->depth - 1u];

    if (kind == ':')
    {
        struct dj_ostream_template_op op =
            d_internal_template_op(D_OSTREAM_TEMPLATE_JUMP);

        if ( (_size != 1u) ||
             (block->jump != D_INTERNAL_TEMPLATE_NONE) )
        {
            return d_internal_template_error(_compiler,
                                             (_size != 1u)
                                                 ? D_OSTREAM_TEMPLATE_SYNTAX
                                                 : D_OSTREAM_TEMPLATE_NESTING,
                                             _open_at);
        }

        block->jump = d_internal_template_append(_compiler, &op);
        d_internal_template_patch(_compiler, block->op, _compiler->count);

        return D_OSTREAM_TEMPLATE_OK;
    }

    // {/} or {/name}, and a name must be the block's own
    if ( (length != 0u) &&
         ( (length != block->size) ||
           (memcmp(_tag + 1, block->name, length) != 0) ) )
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_NESTING,
                                         _open_at);
    }

    d_internal_template_patch(_compiler,
                              (block->jump != D_INTERNAL_TEMPLATE_NONE)
                                  ? block->jump
                                  : block->op,
                              _compiler->count);
    --_compiler->depth;

    return D_OSTREAM_TEMPLATE_OK;
}

/**
 * @brief Compiles a field's tag: name, then !conv, then :spec.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _tag       the tag's text, between its braces.
 * @param[in]     _size      its length.
 * @param[in]     _open_at   its '{'.
 * @return D_OSTREAM_TEMPLATE_OK, or why not.
 */
static int32_t
d_internal_template_field(
    struct dj_internal_template_compiler* _compiler,
    const char*                           _tag,
    size_t                                _size,
    size_t                                _open_at
)
{
    struct dj_ostream_template_op op =
        d_internal_template_op(D_OSTREAM_TEMPLATE_FIELD);
    const size_t length = d_internal_template_name_length(_tag, _size);
    size_t       at     = length;

    if (length == 0u)
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_SYNTAX,
                                         _open_at);
    }

    if (d_internal_template_find(_compiler,
                                 _tag,
                                 length,
                                 &op.operand) != D_OSTREAM_TEMPLATE_OK)
    {
        return D_OSTREAM_TEMPLATE_UNKNOWN_FIELD;
    }

    // !conv
    if ( (at < _size) &&
         (_tag[at] == '!') )
    {
        const size_t  name    = d_internal_template_name_length(_tag + at + 1u,
                                                               _size - at - 1u);
        const int32_t convert = d_internal_template_convert_of(_tag + at + 1u,
                                                              name);

        if (convert < 0)
        {
            return d_internal_template_error(
                       _compiler,
                       D_OSTREAM_TEMPLATE_BAD_CONVERSION,
                       (size_t)(_tag - _compiler->source) + at);
        }

        op.spec.convert = (uint8_t)convert;
        at += 1u + name;
    }

    // :spec, which is the rest
    if (at < _size)
    {
        size_t stop = 0u;

        if (_tag[at] != ':')
        {
            return d_internal_template_error(
                       _compiler,
                       D_OSTREAM_TEMPLATE_SYNTAX,
                       (size_t)(_tag - _compiler->source) + at);
        }

        const int32_t status = d_internal_template_read_spec(
                                   _tag + at + 1u,
                                   _size - at - 1u,
                                   &op.spec,
                                   &op.width_field,
                                   &op.precision_field,
                                   _compiler,
                                   &stop);

        if (status != D_OSTREAM_TEMPLATE_OK)
        {
            return d_internal_template_error(
                       _compiler,
                       status,
                       (size_t)(_tag - _compiler->source) + at + 1u + stop);
        }
    }

    (void)d_internal_template_append(_compiler, &op);

    return D_OSTREAM_TEMPLATE_OK;
}

/**
 * @brief Compiles the tag whose '{' is at _open_at.
 *
 * @param[in,out] _compiler  the compiler.
 * @param[in]     _open_at   the tag's '{'.
 * @param[out]    _next      one past its '}'.
 * @return D_OSTREAM_TEMPLATE_OK, or why not.
 *
 * A tag ends at the '}' that balances its '{'. Only a field's width and
 * precision put braces inside one, and only a level deep; the readers check
 * that, so this only has to find the end.
 */
static int32_t
d_internal_template_tag(
    struct dj_internal_template_compiler* _compiler,
    size_t                                _open_at,
    size_t*                               _next
)
{
    const char* const source = _compiler->source;
    size_t            close  = _open_at + 1u;
    uint32_t          depth  = 1u;

    while (source[close] != '\0')
    {
        if (source[close] == '{')
        {
            ++depth;
        }
        else if ( (source[close] == '}') &&
                  (--depth == 0u) )
        {
            break;
        }

        ++close;
    }

    if (source[close] == '\0')
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_SYNTAX,
                                         _open_at);
    }

    const char* const tag  = source + _open_at + 1u;
    const size_t      size = close - _open_at - 1u;

    *_next = close + 1u;

    // an empty tag is nothing at all
    if (size == 0u)
    {
        return d_internal_template_error(_compiler,
                                         D_OSTREAM_TEMPLATE_SYNTAX,
                                         _open_at);
    }

    if ( (tag[0] == '?') ||
         (tag[0] == '^') ||
         (tag[0] == ':') ||
         (tag[0] == '/') )
    {
        return d_internal_template_block(_compiler, tag, size, _open_at);
    }

    return d_internal_template_field(_compiler, tag, size, _open_at);
}

/**
 * @brief Compiles the whole source: the literals between tags, and each tag.
 *
 * @param[in,out] _compiler  the compiler, set up over a source.
 * @return D_OSTREAM_TEMPLATE_OK, or the first error, recorded.
 *
 * One pass. Text runs between tags become literals over the source itself;
 * "{{" and "}}" end a run one byte into the pair, so the brace they stand for
 * is the run's last byte and the next run starts after the pair. A block
 * still open at the end is an error at its own '{'.
 */
static int32_t
d_internal_template_scan(
    struct dj_internal_template_compiler* _compiler
)
{
    const char* const source = _compiler->source;
    size_t            at     = 0u;
    size_t            start  = 0u;
    int32_t           status = D_OSTREAM_TEMPLATE_OK;

    // text up to the next brace, then the brace
    while ( (status == D_OSTREAM_TEMPLATE_OK) &&
            (source[at] != '\0') )
    {
        at += strcspn(source + at, "{}");

        if (source[at] == '\0')
        {
            break;
        }

        if (source[at + 1u] == source[at])
        {
            d_internal_template_literal(_compiler, start, at + 1u);
            at   += 2u;
            start = at;
        }
        else if (source[at] == '}')
        {
            status = d_internal_template_error(_compiler,
                                               D_OSTREAM_TEMPLATE_SYNTAX,
                                               at);
        }
        else
        {
            size_t next = at;

            d_internal_template_literal(_compiler, start, at);
            status = d_internal_template_tag(_compiler, at, &next);
            at     = next;
            start  = at;
        }
    }

    if (status == D_OSTREAM_TEMPLATE_OK)
    {
        d_internal_template_literal(_compiler, start, at);

        if (_compiler->depth != 0u)
        {
            status = d_internal_template_error(
                         _compiler,
                         D_OSTREAM_TEMPLATE_NESTING,
                         _compiler->blocks[_compiler->depth - 1u].open_at);
        }
    }

    return status;
}

/*
d_ostream_template_compile
  Checks its arguments, sets the compiler up, and scans. The operations are
counted past the capacity, so a template that does not fit still reports
what it needs.
*/
int32_t
d_ostream_template_compile(
    struct dj_ostream_template*       _template,
    struct dj_ostream_template_op*    _ops,
    size_t                            _capacity,
    const char*                       _source,
    dj_ostream_template_lookup_fn     _lookup,
    void*                             _context,
    struct dj_ostream_template_error* _error
)
{
    struct dj_internal_template_compiler compiler;

    if (_error != NULL)
    {
        _error->offset = 0u;
        _error->needed = 0u;
        _error->status = D_OSTREAM_TEMPLATE_OK;
    }

    if ( (_template == NULL) ||
         ( (_ops == NULL) &&
           (_capacity != 0u) ) )
    {
        if (_error != NULL)
        {
            _error->status = D_OSTREAM_TEMPLATE_INVALID;
        }

        return D_OSTREAM_TEMPLATE_INVALID;
    }

    _template->source   = _source;
    _template->ops      = _ops;
    _template->count    = 0u;
    _template->reserved = 0u;

    // nothing to compile is an empty template
    if (_source == NULL)
    {
        return D_OSTREAM_TEMPLATE_OK;
    }

    // offsets and lengths are 32 bits
    if (strlen(_source) >= (size_t)0xFFFFFFFFu)
    {
        if (_error != NULL)
        {
            _error->status = D_OSTREAM_TEMPLATE_INVALID;
        }

        return D_OSTREAM_TEMPLATE_INVALID;
    }

    compiler.source   = _source;
    compiler.ops      = _ops;
    compiler.capacity = _capacity;
    compiler.count    = 0u;
    compiler.lookup   = _lookup;
    compiler.context  = _context;
    compiler.error    = _error;
    compiler.depth    = 0u;

    int32_t status = d_internal_template_scan(&compiler);

    if ( (status == D_OSTREAM_TEMPLATE_OK) &&
         ((size_t)compiler.count > _capacity) )
    {
        status = d_internal_template_error(&compiler,
                                           D_OSTREAM_TEMPLATE_CAPACITY,
                                           0u);
    }

    if (_error != NULL)
    {
        _error->needed = compiler.count;
    }

    if (status == D_OSTREAM_TEMPLATE_OK)
    {
        _template->count = compiler.count;
    }

    return status;
}


//==============================================================================
// RUNNING
//==============================================================================

/**
 * @brief Gives a field's value as a width or a precision.
 *
 * @param[in]  _resolve  the resolver.
 * @param[in]  _context  its context.
 * @param[in]  _field    the field.
 * @param[out] _count    the value, clamped to [0, D_OSTREAM_SPEC_MAX_WIDTH].
 * @return 1, or 0 when the resolver stopped.
 */
static int32_t
d_internal_template_count(
    dj_ostream_template_resolve_fn _resolve,
    void*                          _context,
    uint32_t                       _field,
    uint16_t*                      _count
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    if (_resolve(_context, _field, &value) == 0)
    {
        return 0;
    }

    const long count = d_ostream_value_to_long(&value);

    *_count = (count <= 0L)
                  ? 0u
                  : ( (count > (long)D_OSTREAM_SPEC_MAX_WIDTH)
                          ? (uint16_t)D_OSTREAM_SPEC_MAX_WIDTH
                          : (uint16_t)count );

    return 1;
}

/**
 * @brief Runs one FIELD operation.
 *
 * @param[in,out] _os       the stream.
 * @param[in]     _op       the operation.
 * @param[in]     _resolve  the resolver.
 * @param[in]     _context  its context.
 * @return 1, or 0 when the resolver stopped.
 */
static int32_t
d_internal_template_run_field(
    struct dj_ostream*                   _os,
    const struct dj_ostream_template_op* _op,
    dj_ostream_template_resolve_fn       _resolve,
    void*                                _context
)
{
    struct dj_ostream_value value;

    // a resolver that writes nothing leaves NONE, the only member read then
    value.kind = D_OSTREAM_VALUE_NONE;

    if (_resolve(_context, _op->operand, &value) == 0)
    {
        return 0;
    }

    // the usual field's specification is the operation's own, unchanged
    if ( (_op->width_field == D_OSTREAM_TEMPLATE_NO_FIELD) &&
         (_op->precision_field == D_OSTREAM_TEMPLATE_NO_FIELD) )
    {
        d_internal_template_write(_os, &value, &_op->spec);

        return 1;
    }

    // the fields a width and a precision name are read each time
    struct dj_ostream_spec spec = _op->spec;

    if ( ( (_op->width_field != D_OSTREAM_TEMPLATE_NO_FIELD) &&
           (d_internal_template_count(_resolve,
                                      _context,
                                      _op->width_field,
                                      &spec.width) == 0) )      ||
         ( (_op->precision_field != D_OSTREAM_TEMPLATE_NO_FIELD) &&
           (d_internal_template_count(_resolve,
                                      _context,
                                      _op->precision_field,
                                      &spec.precision) == 0) ) )
    {
        return 0;
    }

    d_internal_template_write(_os, &value, &spec);

    return 1;
}

/*
d_ostream_template_render
  A loop over the operations. Every jump a compiler makes is forward, and one
that is not -- from operations built some other way -- stops the template
rather than loop. A field is read once per operation that names it, so a
resolver may compute.
*/
int32_t
d_ostream_template_render(
    struct dj_ostream*                _os,
    const struct dj_ostream_template* _template,
    dj_ostream_template_resolve_fn    _resolve,
    void*                             _context
)
{
    uint32_t pc = 0u;

    if (_os == NULL)
    {
        return 0;
    }

    if ( (_template == NULL) ||
         ( (_template->count != 0u) &&
           (_template->ops == NULL) ) )
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
    }

    while (pc < _template->count)
    {
        const struct dj_ostream_template_op* const op = &_template->ops[pc];
        struct dj_ostream_value                    value;
        uint32_t                                   next = pc + 1u;

        switch (op->code)
        {
            case D_OSTREAM_TEMPLATE_LITERAL:
                (void)d_ostream_write(_os,
                                      _template->source + op->operand,
                                      (size_t)op->extent);
                break;
            case D_OSTREAM_TEMPLATE_FIELD:
                if ( (_resolve == NULL) ||
                     (d_internal_template_run_field(_os,
                                                    op,
                                                    _resolve,
                                                    _context) == 0) )
                {
                    return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
                }
                break;
            case D_OSTREAM_TEMPLATE_IF:
            case D_OSTREAM_TEMPLATE_UNLESS:
                value = d_ostream_value_none();

                if ( (_resolve == NULL) ||
                     (_resolve(_context, op->operand, &value) == 0) )
                {
                    return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
                }

                // IF skips when false, UNLESS when true
                if ( (d_ostream_value_is_true(&value) != 0) !=
                     (op->code == D_OSTREAM_TEMPLATE_IF) )
                {
                    next = op->extent;
                }
                break;
            case D_OSTREAM_TEMPLATE_JUMP:
                next = op->extent;
                break;
            default:
                return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
        }

        if (next <= pc)
        {
            return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
        }

        pc = next;
    }

    return d_ostream_is_ok(_os);
}

/*
d_ostream_template_status_name
  A table by value; anything outside it is "unknown".
*/
const char*
d_ostream_template_status_name(
    int32_t _status
)
{
    if ( (_status < 0) ||
         (_status >= D_OSTREAM_TEMPLATE_STATUS_COUNT) )
    {
        return "unknown";
    }

    return STATUS_NAMES[_status];
}
