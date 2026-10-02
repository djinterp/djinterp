/*******************************************************************************
* djinterp [net]                                                          imap.c
*
* Definitions for the IMAP common module declared in `imap.h`.
*   Everything here is a pure function of its arguments: no allocation, no I/O,
* no global mutable state, and no locale dependence. Parsers return views into
* the caller's input; writers stream through a small stack buffer into a sink,
* so neither side ever sizes or copies a whole string.
*   Only the backend queries consult env_imap.h. The codecs are identical on
* every build, which is what lets every backend share them.
*
*
* path:      /src/djinterp/net/imap/imap.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/imap/imap.h"  // corresponding header
// std
#include <assert.h>   // static_assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // fixed-width integers
#include <string.h>   // memchr, memcmp, memcpy, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // D_STATIC
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_sink_emit
#include "../../../../inc/djinterp/env/net/env_imap.h"    // D_ENV_IMAP_*


// imap.h restates env_imap.h's backend identifiers rather than include the
// detection layer; these make the identity a checked fact, not a comment
static_assert(D_IMAP_BACKEND_NONE == D_ENV_IMAP_BACKEND_NONE,
              "d_imap_backend must mirror D_ENV_IMAP_BACKEND_NONE");
static_assert(D_IMAP_BACKEND_NATIVE == D_ENV_IMAP_BACKEND_NATIVE,
              "d_imap_backend must mirror D_ENV_IMAP_BACKEND_NATIVE");
static_assert(D_IMAP_BACKEND_CURL == D_ENV_IMAP_BACKEND_CURL,
              "d_imap_backend must mirror D_ENV_IMAP_BACKEND_CURL");
static_assert(D_IMAP_BACKEND_LIBETPAN == D_ENV_IMAP_BACKEND_LIBETPAN,
              "d_imap_backend must mirror D_ENV_IMAP_BACKEND_LIBETPAN");
static_assert(D_IMAP_BACKEND_MAILUTILS == D_ENV_IMAP_BACKEND_MAILUTILS,
              "d_imap_backend must mirror D_ENV_IMAP_BACKEND_MAILUTILS");
static_assert(D_IMAP_BACKEND_VMIME == D_ENV_IMAP_BACKEND_VMIME,
              "d_imap_backend must mirror D_ENV_IMAP_BACKEND_VMIME");

// every set type holds one bit per enumerator of its vocabulary
static_assert(D_IMAP_AUTH_COUNT <= 32,
              "mechanism sets are uint32_t: one bit per mechanism");
static_assert(D_IMAP_CAPABILITY_COUNT <= 64,
              "capability sets are uint64_t: one bit per capability");
static_assert(D_IMAP_FLAG_COUNT <= 32,
              "flag sets are uint32_t: one bit per flag");
static_assert(D_IMAP_MAILBOX_ATTRIBUTE_COUNT <= 32,
              "attribute sets are uint32_t: one bit per attribute");
static_assert(D_IMAP_STATUS_ITEM_COUNT <= 32,
              "STATUS item sets are uint32_t: one bit per item");

// the tag and sequence-set code rely on these properties of their constants
static_assert(D_IMAP_TAG_CAPACITY >= D_IMAP_TAG_PREFIX_MAX + 10 + 1,
              "a tag needs its prefix, ten counter digits, and a NUL");
static_assert(D_IMAP_SEQ_STAR == 0,
              "\"*\" must be the one value no sequence number or UID takes");

// internal helpers: ASCII

/*
d_imap_internal_fold
  ASCII-only case folding. IMAP keywords are ASCII, and folding through the C
library's tolower() would let a single-byte Turkish locale turn "I" into a
dotless i, breaking every keyword that contains one.
*/
D_STATIC unsigned char
d_imap_internal_fold(
    unsigned char _c
)
{
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return (unsigned char)(_c + ('a' - 'A'));
    }

    return _c;
}

/*
d_imap_internal_equals_nocase
  Compares a span with a NUL-terminated ASCII keyword, folding both sides.
Lengths are compared first, so a span that merely begins with the keyword does
not match.
*/
D_STATIC bool
d_imap_internal_equals_nocase(
    struct d_pack_text _text,
    const char*        _keyword
)
{
    if (_text.length != strlen(_keyword))
    {
        return false;
    }

    for (size_t i = 0; i < _text.length; ++i)
    {
        unsigned char left  = (unsigned char)_text.data[i];
        unsigned char right = (unsigned char)_keyword[i];

        if (d_imap_internal_fold(left) != d_imap_internal_fold(right))
        {
            return false;
        }
    }

    return true;
}

/*
d_imap_internal_has_prefix_nocase
  Whether a span begins with an ASCII keyword, ignoring case: the prefix test
behind "AUTH=" and "APPENDLIMIT=".
*/
D_STATIC bool
d_imap_internal_has_prefix_nocase(
    struct d_pack_text _text,
    const char*        _keyword
)
{
    size_t             length = strlen(_keyword);
    struct d_pack_text head;

    if (_text.length < length)
    {
        return false;
    }

    head.data   = _text.data;
    head.length = length;

    return d_imap_internal_equals_nocase(head,
                                         _keyword);
}

/*
d_imap_internal_is_digit
  An ASCII decimal digit. isdigit() is avoided for the same locale reason as
tolower().
*/
D_STATIC bool
d_imap_internal_is_digit(
    unsigned char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

/*
d_imap_internal_is_atom_char
  ATOM-CHAR: printable US-ASCII other than the space and the atom-specials
( ) { % * " \ ], per RFC 9051. Octets above 0x7F are excluded; the writers
never put them in an atom, and the lexer admits them on its own terms.
*/
D_STATIC bool
d_imap_internal_is_atom_char(
    unsigned char _c
)
{
    if ( (_c <= 0x20) ||
         (_c >= 0x7F) )
    {
        return false;
    }

    return (strchr("(){%*\"\\]",
                   _c) == NULL);
}

/*
d_imap_internal_is_astring_char
  ASTRING-CHAR: an ATOM-CHAR or "]", which an astring admits even though an
atom does not.
*/
D_STATIC bool
d_imap_internal_is_astring_char(
    unsigned char _c
)
{
    return ( (_c == ']') ||
             (d_imap_internal_is_atom_char(_c)) );
}

/*
d_imap_internal_is_tag_char
  A tag is 1*<any ASTRING-CHAR except "+">; the "+" is reserved because a line
beginning with it is a continuation request.
*/
D_STATIC bool
d_imap_internal_is_tag_char(
    unsigned char _c
)
{
    return ( (_c != '+') &&
             (d_imap_internal_is_astring_char(_c)) );
}

/*
d_imap_internal_is_space
  The one separator IMAP puts between tokens.
*/
D_STATIC bool
d_imap_internal_is_space(
    unsigned char _c
)
{
    return (_c == ' ');
}

// d_imap_internal_octet_test
//   type: a character-class predicate, for the scanning helpers.
typedef bool (*d_imap_internal_octet_test)(unsigned char _c);

// internal helpers: spans and numbers

/*
d_imap_internal_text_is_valid
  A span is usable when it has data or claims none; a NULL pointer with a
nonzero length is the one malformed shape, and every public entry point
rejects it before reading.
*/
D_STATIC bool
d_imap_internal_text_is_valid(
    struct d_pack_text _text
)
{
    return ( (_text.data != NULL) ||
             (_text.length == 0) );
}

/*
d_imap_internal_span
  The view [_start, _end) of a buffer. Callers guarantee the order of the
bounds. A NULL buffer (only ever empty) stays NULL rather than being offset,
since even a zero offset from a null pointer is undefined in C.
*/
D_STATIC struct d_pack_text
d_imap_internal_span(
    const char* _data,
    size_t      _start,
    size_t      _end
)
{
    struct d_pack_text span;

    span.data   = (_data != NULL) ? (_data + _start) : NULL;
    span.length = _end - _start;

    return span;
}

/*
d_imap_internal_parse_decimal
  Parses an all-digit span, refusing any value above `_max` before the
multiplication that would exceed it, so the accumulator never wraps. `_max` is
at least 9 at every call site, so `_max - digit` cannot wrap either.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_decimal(
    struct d_pack_text _text,
    uint64_t           _max,
    uint64_t*          _value
)
{
    uint64_t value = 0;

    if (_text.length == 0)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    for (size_t i = 0; i < _text.length; ++i)
    {
        unsigned char c = (unsigned char)_text.data[i];

        if (!d_imap_internal_is_digit(c))
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        // refuse before multiplying, so the accumulator never wraps
        if (value > (_max - (uint64_t)(c - '0')) / 10u)
        {
            return D_IMAP_ERROR_RANGE;
        }

        value = (value * 10u) + (uint64_t)(c - '0');
    }

    *_value = value;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_format_decimal
  Writes a value's decimal digits into `_digits`, which holds 20 (the most a
uint64_t needs), zero-padded on the left to `_width` (at most 20). Digits come
out least significant first, so they are generated into scratch space and
reversed into place. Returns the digit count.
*/
D_STATIC size_t
d_imap_internal_format_decimal(
    char     _digits[20],
    uint64_t _value,
    size_t   _width
)
{
    char   reversed[20];
    size_t count = 0;

    // generate the digits, least significant first
    do
    {
        reversed[count] = (char)('0' + (int)(_value % 10u));
        _value         /= 10u;
        ++count;
    } while (_value != 0u);

    // pad with zeros to the requested width
    while ( (count < _width) &&
            (count < sizeof(reversed)) )
    {
        reversed[count] = '0';
        ++count;
    }

    // reverse into the caller's array
    for (size_t i = 0; i < count; ++i)
    {
        _digits[i] = reversed[count - 1 - i];
    }

    return count;
}

/*
d_imap_internal_append_decimal
  Appends a value's digits to a line being assembled in a fixed buffer and
advances `*_used`. Each caller sizes its buffer for the widest line it builds.
*/
D_STATIC void
d_imap_internal_append_decimal(
    char*    _buffer,
    size_t*  _used,
    uint64_t _value,
    size_t   _width
)
{
    char   digits[20];
    size_t count = d_imap_internal_format_decimal(digits,
                                                  _value,
                                                  _width);

    memcpy(_buffer + *_used,
           digits,
           count);
    *_used += count;

    return;
}

// internal helpers: output

/*
d_imap_internal_emit
  The one place a sink's refusal becomes D_IMAP_ERROR_SINK. d_sink_emit takes
all of the bytes or none, so there is no partial count to report.
*/
D_STATIC enum d_imap_error
d_imap_internal_emit(
    struct d_pack_sink _sink,
    const void*        _data,
    size_t             _size
)
{
    if (!d_sink_emit(_sink,
                     _data,
                     _size))
    {
        return D_IMAP_ERROR_SINK;
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_emit_after
  Emits only if nothing has failed yet, and otherwise passes the earlier error
through. Chaining writes through it keeps a writer to one check at the end,
and the first failure is the one reported.
*/
D_STATIC enum d_imap_error
d_imap_internal_emit_after(
    enum d_imap_error  _error,
    struct d_pack_sink _sink,
    const void*        _data,
    size_t             _size
)
{
    if (_error != D_IMAP_ERROR_NONE)
    {
        return _error;
    }

    return d_imap_internal_emit(_sink,
                                _data,
                                _size);
}

/*
d_imap_internal_writer
  One buffered write in progress. Output gathers on the stack and reaches the
sink in blocks, so a codec that produces a character at a time does not cost a
sink call per character. The first failure sticks in `error`, and later
operations become no-ops that preserve it.
*/
struct d_imap_internal_writer
{
    struct d_pack_sink sink;
    enum d_imap_error  error;
    size_t             used;
    char               buffer[128];
};

/*
d_imap_internal_writer_init
  Starts a write with an empty buffer and no error.
*/
D_STATIC void
d_imap_internal_writer_init(
    struct d_imap_internal_writer* _writer,
    struct d_pack_sink             _sink
)
{
    _writer->sink  = _sink;
    _writer->error = D_IMAP_ERROR_NONE;
    _writer->used  = 0;

    return;
}

/*
d_imap_internal_writer_flush
  Hands the buffer to the sink and empties it. After a failure the buffer is
discarded unsent, which is safe because the error already dooms the write.
*/
D_STATIC void
d_imap_internal_writer_flush(
    struct d_imap_internal_writer* _writer
)
{
    _writer->error = d_imap_internal_emit_after(_writer->error,
                                                _writer->sink,
                                                _writer->buffer,
                                                _writer->used);
    _writer->used  = 0;

    return;
}

/*
d_imap_internal_writer_put
  Appends one character, flushing first when the buffer is full.
*/
D_STATIC void
d_imap_internal_writer_put(
    struct d_imap_internal_writer* _writer,
    char                           _c
)
{
    if (_writer->used == sizeof(_writer->buffer))
    {
        d_imap_internal_writer_flush(_writer);
    }

    _writer->buffer[_writer->used] = _c;
    ++_writer->used;

    return;
}

/*
d_imap_internal_writer_write
  Appends a run of characters through the same path as single ones, so the
buffer boundary needs no special case.
*/
D_STATIC void
d_imap_internal_writer_write(
    struct d_imap_internal_writer* _writer,
    const char*                    _data,
    size_t                         _length
)
{
    for (size_t i = 0; i < _length; ++i)
    {
        d_imap_internal_writer_put(_writer,
                                   _data[i]);
    }

    return;
}

/*
d_imap_internal_writer_decimal
  Appends a value in decimal, unpadded.
*/
D_STATIC void
d_imap_internal_writer_decimal(
    struct d_imap_internal_writer* _writer,
    uint64_t                       _value
)
{
    char   digits[20];
    size_t count = d_imap_internal_format_decimal(digits,
                                                  _value,
                                                  1);

    d_imap_internal_writer_write(_writer,
                                 digits,
                                 count);

    return;
}

/*
d_imap_internal_writer_finish
  Flushes what remains and reports the write's outcome: the first failure, or
none.
*/
D_STATIC enum d_imap_error
d_imap_internal_writer_finish(
    struct d_imap_internal_writer* _writer
)
{
    d_imap_internal_writer_flush(_writer);

    return _writer->error;
}

// internal helpers: UTF-8

/*
d_imap_internal_utf8_row
  One row of the strict UTF-8 decoding table (RFC 3629 section 4): the lead
octets it covers, the sequence length, and the range the SECOND octet must fall
in. Narrowing that range for the leads E0, ED, F0, and F4 is what excludes
overlong forms, surrogates, and values above U+10FFFF without decoding first;
every later octet is simply 80..BF.
*/
struct d_imap_internal_utf8_row
{
    unsigned char first;   // the lowest lead octet of the row
    unsigned char last;    // the highest lead octet of the row
    unsigned char count;   // octets in the whole sequence
    unsigned char low;     // the lowest permitted second octet
    unsigned char high;    // the highest permitted second octet
};

// d_imap_internal_utf8_rows
//   table: every valid multi-octet lead, in ascending order.
static const struct d_imap_internal_utf8_row d_imap_internal_utf8_rows[] =
{
    { 0xC2, 0xDF, 2, 0x80, 0xBF },
    { 0xE0, 0xE0, 3, 0xA0, 0xBF },  // no overlong three-octet forms
    { 0xE1, 0xEC, 3, 0x80, 0xBF },
    { 0xED, 0xED, 3, 0x80, 0x9F },  // no surrogates
    { 0xEE, 0xEF, 3, 0x80, 0xBF },
    { 0xF0, 0xF0, 4, 0x90, 0xBF },  // no overlong four-octet forms
    { 0xF1, 0xF3, 4, 0x80, 0xBF },
    { 0xF4, 0xF4, 4, 0x80, 0x8F }   // nothing above U+10FFFF
};

/*
d_imap_internal_utf8_next
  Decodes one scalar value from non-empty input, returning the octets it used,
or 0 for an invalid or truncated sequence. The lead's row fixes the length and
the second octet's range; the lead contributes its low (7 - count) bits.
*/
D_STATIC size_t
d_imap_internal_utf8_next(
    const unsigned char* _data,
    size_t               _length,
    uint32_t*            _code_point
)
{
    const size_t                           rows =
        D_ARRAY_STATIC_SIZE(d_imap_internal_utf8_rows);
    const struct d_imap_internal_utf8_row* row  = NULL;
    uint32_t                               value;

    // US-ASCII stands for itself
    if (_data[0] < 0x80)
    {
        *_code_point = _data[0];

        return 1;
    }

    // find the lead's row; an octet in none of them never begins a sequence
    for (size_t i = 0; i < rows; ++i)
    {
        if ( (_data[0] >= d_imap_internal_utf8_rows[i].first) &&
             (_data[0] <= d_imap_internal_utf8_rows[i].last) )
        {
            row = &d_imap_internal_utf8_rows[i];

            break;
        }
    }

    if ( (!row) ||
         (_length < row->count) ||
         (_data[1] < row->low) ||
         (_data[1] > row->high) )
    {
        return 0;
    }

    // accumulate six bits per continuation octet
    value = (uint32_t)(_data[0] & (0x7Fu >> row->count));

    for (size_t i = 1; i < row->count; ++i)
    {
        if ( (_data[i] < 0x80) ||
             (_data[i] > 0xBF) )
        {
            return 0;
        }

        value = (value << 6) | (uint32_t)(_data[i] & 0x3Fu);
    }

    *_code_point = value;

    return row->count;
}

/*
d_imap_internal_utf8_is_valid
  Whether a whole span is strict UTF-8, walking it one sequence at a time.
*/
D_STATIC bool
d_imap_internal_utf8_is_valid(
    struct d_pack_text _text
)
{
    const unsigned char* data     = (const unsigned char*)_text.data;
    size_t               position = 0;

    while (position < _text.length)
    {
        uint32_t code_point;
        size_t   used = d_imap_internal_utf8_next(data + position,
                                                  _text.length - position,
                                                  &code_point);

        if (used == 0)
        {
            return false;
        }

        position += used;
    }

    return true;
}

/*
d_imap_internal_writer_utf8
  Appends one scalar value in UTF-8. Callers pass only scalar values: never a
surrogate, never above U+10FFFF.
*/
D_STATIC void
d_imap_internal_writer_utf8(
    struct d_imap_internal_writer* _writer,
    uint32_t                       _code_point
)
{
    char   octets[4];
    size_t count;

    if (_code_point < 0x80u)
    {
        octets[0] = (char)_code_point;
        count     = 1;
    }
    else if (_code_point < 0x800u)
    {
        octets[0] = (char)(0xC0u | (_code_point >> 6));
        octets[1] = (char)(0x80u | (_code_point & 0x3Fu));
        count     = 2;
    }
    else if (_code_point < 0x10000u)
    {
        octets[0] = (char)(0xE0u | (_code_point >> 12));
        octets[1] = (char)(0x80u | ((_code_point >> 6) & 0x3Fu));
        octets[2] = (char)(0x80u | (_code_point & 0x3Fu));
        count     = 3;
    }
    else
    {
        octets[0] = (char)(0xF0u | (_code_point >> 18));
        octets[1] = (char)(0x80u | ((_code_point >> 12) & 0x3Fu));
        octets[2] = (char)(0x80u | ((_code_point >> 6) & 0x3Fu));
        octets[3] = (char)(0x80u | (_code_point & 0x3Fu));
        count     = 4;
    }

    d_imap_internal_writer_write(_writer,
                                 octets,
                                 count);

    return;
}

// internal helpers: name tables

/*
d_imap_internal_table_name
  Indexes a name table by enumerator value, answering `_fallback` outside it.
Every table is sized by a static assertion against its COUNT constant, so an
in-range value always has an entry.
*/
D_STATIC const char*
d_imap_internal_table_name(
    const char* const* _table,
    size_t             _count,
    int                _value,
    const char*        _fallback
)
{
    if ( (_value < 0) ||
         ((size_t)_value >= _count) )
    {
        return _fallback;
    }

    return _table[_value];
}

/*
d_imap_internal_table_find
  The inverse: a case-insensitive linear search, skipping empty entries, which
stand for values without a wire name. The vocabularies are small enough that a
scan beats any index built for them.
*/
D_STATIC bool
d_imap_internal_table_find(
    const char* const* _table,
    size_t             _count,
    struct d_pack_text _name,
    size_t*            _index
)
{
    for (size_t i = 0; i < _count; ++i)
    {
        if ( (_table[i][0] != '\0') &&
             (d_imap_internal_equals_nocase(_name,
                                            _table[i])) )
        {
            *_index = i;

            return true;
        }
    }

    return false;
}

// d_imap_internal_backend_names
//   table: the backend names, indexed by d_imap_backend; the spellings of
// D_ENV_IMAP_BACKEND_NAME.
static const char* const d_imap_internal_backend_names[] =
{
    "none", "native", "libcurl", "libetpan", "GNU Mailutils", "VMime"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_backend_names) ==
                  D_IMAP_BACKEND_COUNT,
              "one name per backend");

// d_imap_internal_backend_available
//   table: whether this translation unit's environment could build each
// backend, indexed by d_imap_backend. Evaluated as C, so VMime reads 0.
static const bool d_imap_internal_backend_available[] =
{
    false,
    (D_ENV_IMAP_CAN_NATIVE != 0),
    (D_ENV_IMAP_CURL_AVAILABLE != 0),
    (D_ENV_IMAP_LIBETPAN_AVAILABLE != 0),
    (D_ENV_IMAP_MAILUTILS_AVAILABLE != 0),
    (D_ENV_IMAP_VMIME_AVAILABLE != 0)
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_backend_available) ==
                  D_IMAP_BACKEND_COUNT,
              "one availability answer per backend");

/*
d_imap_backend_name
  A table lookup; the table repeats env_imap.h's spellings so that a name
printed from C matches one printed from the preprocessor.
*/
const char*
d_imap_backend_name(
    enum d_imap_backend _backend
)
{
    return d_imap_internal_table_name(d_imap_internal_backend_names,
                                      D_IMAP_BACKEND_COUNT,
                                      (int)_backend,
                                      "unknown");
}

/*
d_imap_backend_is_available
  Reads a table the preprocessor filled when this file was compiled; nothing
is probed at run time.
*/
bool
d_imap_backend_is_available(
    enum d_imap_backend _backend
)
{
    if ((unsigned)_backend >= D_IMAP_BACKEND_COUNT)
    {
        return false;
    }

    return d_imap_internal_backend_available[_backend];
}

/*
d_imap_backend_default
  env_imap.h's choice, converted by the identity the assertions above prove.
*/
enum d_imap_backend
d_imap_backend_default(
    void
)
{
    return (enum d_imap_backend)D_ENV_IMAP_BACKEND;
}

// d_imap_internal_error_names
//   table: each error's enumerator suffix, lower-cased, indexed by
// d_imap_error.
static const char* const d_imap_internal_error_names[] =
{
    "none",
    "invalid_argument", "sink", "capacity", "syntax", "incomplete", "range",
    "encoding", "limit",
    "state", "unsupported", "security", "auth",
    "no", "bad", "bye", "protocol",
    "resolve", "connect", "tls", "timeout", "closed", "io",
    "memory", "internal"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_error_names) ==
                  D_IMAP_ERROR_COUNT,
              "one name per error");

// d_imap_internal_error_messages
//   table: a short sentence for each error, indexed by d_imap_error.
static const char* const d_imap_internal_error_messages[] =
{
    "no error",
    "invalid argument",
    "the output sink refused the data",
    "an output array is too small",
    "malformed IMAP syntax",
    "the input ended before the item was complete",
    "a number or field is out of range",
    "invalid UTF-8 or modified UTF-7",
    "a configured limit was exceeded",
    "the command is not valid in the current state",
    "not supported by this build, backend, or server",
    "refused by the connection security policy",
    "authentication failed",
    "the server rejected the command (NO)",
    "the server reported a protocol error (BAD)",
    "the server closed the session (BYE)",
    "the server's response violated the protocol",
    "host name resolution failed",
    "could not connect to the server",
    "TLS negotiation or certificate verification failed",
    "the operation timed out",
    "the connection closed unexpectedly",
    "transport I/O failed",
    "out of memory",
    "internal error"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_error_messages) ==
                  D_IMAP_ERROR_COUNT,
              "one message per error");

/*
d_imap_error_name
  A table lookup.
*/
const char*
d_imap_error_name(
    enum d_imap_error _error
)
{
    return d_imap_internal_table_name(d_imap_internal_error_names,
                                      D_IMAP_ERROR_COUNT,
                                      (int)_error,
                                      "unknown");
}

/*
d_imap_error_message
  A table lookup.
*/
const char*
d_imap_error_message(
    enum d_imap_error _error
)
{
    return d_imap_internal_table_name(d_imap_internal_error_messages,
                                      D_IMAP_ERROR_COUNT,
                                      (int)_error,
                                      "unknown");
}

/*
d_imap_error_is_server
  The three errors that relay what the server said. PROTOCOL is excluded: the
server did not report it, the client diagnosed it.
*/
bool
d_imap_error_is_server(
    enum d_imap_error _error
)
{
    return ( (_error == D_IMAP_ERROR_NO) ||
             (_error == D_IMAP_ERROR_BAD) ||
             (_error == D_IMAP_ERROR_BYE) );
}

/*
d_imap_error_is_transport
  The transport group is contiguous by construction, so this is a range test.
*/
bool
d_imap_error_is_transport(
    enum d_imap_error _error
)
{
    return ( (_error >= D_IMAP_ERROR_RESOLVE) &&
             (_error <= D_IMAP_ERROR_IO) );
}

// d_imap_internal_state_names
//   table: the state descriptions, indexed by d_imap_state.
static const char* const d_imap_internal_state_names[] =
{
    "disconnected", "not authenticated", "authenticated", "selected", "logout"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_state_names) ==
                  D_IMAP_STATE_COUNT,
              "one name per state");

// d_imap_internal_command_names
//   table: the command keywords, indexed by d_imap_command; the empty first
// entry stands for D_IMAP_COMMAND_UNKNOWN.
static const char* const d_imap_internal_command_names[] =
{
    "",
    "CAPABILITY", "NOOP", "LOGOUT", "ID",
    "STARTTLS", "AUTHENTICATE", "LOGIN",
    "ENABLE", "SELECT", "EXAMINE", "CREATE", "DELETE", "RENAME", "SUBSCRIBE",
    "UNSUBSCRIBE", "LIST", "LSUB", "NAMESPACE", "STATUS", "APPEND", "IDLE",
    "CHECK", "CLOSE", "UNSELECT", "EXPUNGE", "SEARCH", "FETCH", "STORE",
    "COPY", "MOVE", "UID", "SORT", "THREAD"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_command_names) ==
                  D_IMAP_COMMAND_COUNT,
              "one keyword per command");

/*
d_imap_state_name
  A table lookup.
*/
const char*
d_imap_state_name(
    enum d_imap_state _state
)
{
    return d_imap_internal_table_name(d_imap_internal_state_names,
                                      D_IMAP_STATE_COUNT,
                                      (int)_state,
                                      "unknown");
}

/*
d_imap_command_name
  A table lookup; UNKNOWN's entry is already the empty string.
*/
const char*
d_imap_command_name(
    enum d_imap_command _command
)
{
    return d_imap_internal_table_name(d_imap_internal_command_names,
                                      D_IMAP_COMMAND_COUNT,
                                      (int)_command,
                                      "");
}

/*
d_imap_command_from_name
  The table search skips the empty UNKNOWN entry, so an empty name falls
through to UNKNOWN like any other unmatched one.
*/
enum d_imap_command
d_imap_command_from_name(
    struct d_pack_text _name
)
{
    size_t index;

    if (!d_imap_internal_text_is_valid(_name))
    {
        return D_IMAP_COMMAND_UNKNOWN;
    }

    if (d_imap_internal_table_find(d_imap_internal_command_names,
                                   D_IMAP_COMMAND_COUNT,
                                   _name,
                                   &index))
    {
        return (enum d_imap_command)index;
    }

    return D_IMAP_COMMAND_UNKNOWN;
}

/*
d_imap_command_is_valid_in
  The command enumeration is grouped by state, so each group is a range test;
ENABLE is tested before its group because RFC 5161 confines it to the
authenticated state proper.
*/
bool
d_imap_command_is_valid_in(
    enum d_imap_command _command,
    enum d_imap_state   _state
)
{
    // outside the three command states nothing may be sent
    if ( (_state != D_IMAP_STATE_NOT_AUTHENTICATED) &&
         (_state != D_IMAP_STATE_AUTHENTICATED) &&
         (_state != D_IMAP_STATE_SELECTED) )
    {
        return false;
    }

    if ( (_command >= D_IMAP_COMMAND_CAPABILITY) &&
         (_command <= D_IMAP_COMMAND_ID) )
    {
        return true;
    }

    if ( (_command >= D_IMAP_COMMAND_STARTTLS) &&
         (_command <= D_IMAP_COMMAND_LOGIN) )
    {
        return (_state == D_IMAP_STATE_NOT_AUTHENTICATED);
    }

    if (_command == D_IMAP_COMMAND_ENABLE)
    {
        return (_state == D_IMAP_STATE_AUTHENTICATED);
    }

    if ( (_command >= D_IMAP_COMMAND_SELECT) &&
         (_command <= D_IMAP_COMMAND_IDLE) )
    {
        return (_state != D_IMAP_STATE_NOT_AUTHENTICATED);
    }

    if ( (_command >= D_IMAP_COMMAND_CHECK) &&
         (_command <= D_IMAP_COMMAND_THREAD) )
    {
        return (_state == D_IMAP_STATE_SELECTED);
    }

    return false;
}

/*
d_imap_tagger_init
  Every prefix character is checked against the tag class up front, so every
tag the tagger later issues is valid by construction.
*/
enum d_imap_error
d_imap_tagger_init(
    struct d_imap_tagger* _tagger,
    struct d_pack_text    _prefix
)
{
    if ( (!_tagger) ||
         (!d_imap_internal_text_is_valid(_prefix)) ||
         (_prefix.length > D_IMAP_TAG_PREFIX_MAX) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < _prefix.length; ++i)
    {
        if (!d_imap_internal_is_tag_char((unsigned char)_prefix.data[i]))
        {
            return D_IMAP_ERROR_INVALID_ARGUMENT;
        }
    }

    // an empty prefix selects the conventional "A"
    if (_prefix.length == 0)
    {
        _tagger->prefix[0]     = 'A';
        _tagger->prefix_length = 1;
    }
    else
    {
        memcpy(_tagger->prefix,
               _prefix.data,
               _prefix.length);
        _tagger->prefix_length = (uint32_t)_prefix.length;
    }

    _tagger->next = 1;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_tagger_next
  A zeroed counter has issued nothing, so it is read as 1; that keeps a
zero-initialized tagger harmless as well as an initialized one correct. The
prefix length is rechecked because the struct is plain data a caller can
corrupt, and it bounds the copy.
*/
enum d_imap_error
d_imap_tagger_next(
    struct d_imap_tagger* _tagger,
    struct d_imap_tag*    _tag
)
{
    uint32_t counter;
    size_t   used;

    if ( (!_tagger) ||
         (!_tag) ||
         (_tagger->prefix_length > D_IMAP_TAG_PREFIX_MAX) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    counter = (_tagger->next != 0) ? _tagger->next : 1u;
    used    = _tagger->prefix_length;

    // prefix, then the counter zero-padded to four digits
    memcpy(_tag->text,
           _tagger->prefix,
           used);
    d_imap_internal_append_decimal(_tag->text,
                                   &used,
                                   counter,
                                   4);
    _tag->text[used] = '\0';
    _tag->length     = (uint32_t)used;

    // advance, wrapping past the largest counter back to 1
    _tagger->next = (counter == UINT32_MAX) ? 1u : (counter + 1u);

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_tag_text
  A view of the tag's characters, without the NUL. A length at or past the
capacity can only come from corruption, and yields an empty view.
*/
struct d_pack_text
d_imap_tag_text(
    const struct d_imap_tag* _tag
)
{
    struct d_pack_text text = { NULL, 0 };

    if ( (_tag) &&
         (_tag->length < D_IMAP_TAG_CAPACITY) )
    {
        text.data   = _tag->text;
        text.length = _tag->length;
    }

    return text;
}

/*
d_imap_tag_matches
  An octet comparison: the server must echo the tag exactly. No issued tag is
empty, so an empty tag matches nothing.
*/
bool
d_imap_tag_matches(
    const struct d_imap_tag* _tag,
    struct d_pack_text       _candidate
)
{
    struct d_pack_text tag = d_imap_tag_text(_tag);

    if ( (tag.length == 0) ||
         (_candidate.length != tag.length) ||
         (!_candidate.data) )
    {
        return false;
    }

    return (memcmp(tag.data,
                   _candidate.data,
                   tag.length) == 0);
}

// d_imap_internal_security_names
//   table: the security-mode descriptions, indexed by d_imap_security.
static const char* const d_imap_internal_security_names[] =
{
    "tls", "starttls", "opportunistic", "none"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_security_names) ==
                  D_IMAP_SECURITY_COUNT,
              "one name per security mode");

// d_imap_internal_auth_names
//   table: the SASL mechanism names, indexed by d_imap_auth_mechanism; the
// first entry describes the LOGIN command and is never matched by a lookup.
static const char* const d_imap_internal_auth_names[] =
{
    "login-command", "PLAIN", "LOGIN", "CRAM-MD5", "DIGEST-MD5",
    "SCRAM-SHA-1", "SCRAM-SHA-1-PLUS", "SCRAM-SHA-256", "SCRAM-SHA-256-PLUS",
    "XOAUTH2", "OAUTHBEARER", "GSSAPI", "EXTERNAL", "NTLM", "ANONYMOUS"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_auth_names) ==
                  D_IMAP_AUTH_COUNT,
              "one name per mechanism");

/*
d_imap_security_name
  A table lookup.
*/
const char*
d_imap_security_name(
    enum d_imap_security _security
)
{
    return d_imap_internal_table_name(d_imap_internal_security_names,
                                      D_IMAP_SECURITY_COUNT,
                                      (int)_security,
                                      "unknown");
}

/*
d_imap_security_default_port
  Only implicit TLS moves to the IMAPS port; STARTTLS and opportunistic modes
begin in plaintext on the IMAP port, as does no security at all.
*/
uint16_t
d_imap_security_default_port(
    enum d_imap_security _security
)
{
    if (_security == D_IMAP_SECURITY_TLS)
    {
        return D_IMAP_PORT_TLS;
    }

    if ( (_security == D_IMAP_SECURITY_STARTTLS) ||
         (_security == D_IMAP_SECURITY_OPPORTUNISTIC) ||
         (_security == D_IMAP_SECURITY_NONE) )
    {
        return D_IMAP_PORT;
    }

    return 0;
}

/*
d_imap_auth_name
  A table lookup.
*/
const char*
d_imap_auth_name(
    enum d_imap_auth_mechanism _mechanism
)
{
    return d_imap_internal_table_name(d_imap_internal_auth_names,
                                      D_IMAP_AUTH_COUNT,
                                      (int)_mechanism,
                                      "");
}

/*
d_imap_auth_from_name
  Searches the table from its second entry, so the LOGIN command's descriptive
name can never be mistaken for an advertised mechanism.
*/
bool
d_imap_auth_from_name(
    struct d_pack_text          _name,
    enum d_imap_auth_mechanism* _mechanism
)
{
    size_t index;

    if ( (!_mechanism) ||
         (!d_imap_internal_text_is_valid(_name)) )
    {
        return false;
    }

    if (!d_imap_internal_table_find(d_imap_internal_auth_names + 1,
                                    D_IMAP_AUTH_COUNT - 1,
                                    _name,
                                    &index))
    {
        return false;
    }

    *_mechanism = (enum d_imap_auth_mechanism)(index + 1);

    return true;
}

// d_imap_internal_capability_names
//   table: the capability wire names, indexed by d_imap_capability.
static const char* const d_imap_internal_capability_names[] =
{
    "IMAP4rev1", "IMAP4rev2", "STARTTLS", "LOGINDISABLED", "SASL-IR", "IDLE",
    "NAMESPACE", "UIDPLUS", "MOVE", "UNSELECT", "ENABLE", "CONDSTORE",
    "QRESYNC", "LITERAL+", "LITERAL-", "ID", "CHILDREN", "SPECIAL-USE",
    "CREATE-SPECIAL-USE", "LIST-EXTENDED", "LIST-STATUS", "ESEARCH",
    "SEARCHRES", "SORT", "THREAD=ORDEREDSUBJECT", "THREAD=REFERENCES",
    "COMPRESS=DEFLATE", "UTF8=ACCEPT", "UTF8=ONLY", "QUOTA", "ACL", "BINARY",
    "CATENATE", "MULTIAPPEND", "WITHIN", "METADATA", "METADATA-SERVER",
    "NOTIFY", "OBJECTID", "SAVEDATE", "STATUS=SIZE", "APPENDLIMIT", "PREVIEW",
    "UNAUTHENTICATE", "LIST-MYRIGHTS", "REPLACE"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_capability_names) ==
                  D_IMAP_CAPABILITY_COUNT,
              "one wire name per capability");

/*
d_imap_capability_name
  A table lookup.
*/
const char*
d_imap_capability_name(
    enum d_imap_capability _capability
)
{
    return d_imap_internal_table_name(d_imap_internal_capability_names,
                                      D_IMAP_CAPABILITY_COUNT,
                                      (int)_capability,
                                      "");
}

/*
d_imap_capabilities_has
  A bit test, range-checked first because shifting by an out-of-range count
is undefined behavior.
*/
bool
d_imap_capabilities_has(
    struct d_imap_capabilities _caps,
    enum d_imap_capability     _cap
)
{
    if ((unsigned)_cap >= D_IMAP_CAPABILITY_COUNT)
    {
        return false;
    }

    return ((_caps.set & D_IMAP_CAPABILITY_BIT(_cap)) != 0u);
}

/*
d_imap_capabilities_has_auth
  A bit test on the AUTH= set. The LOGIN command has no AUTH= entry, so its
bit is never set; its availability is the absence of LOGINDISABLED.
*/
bool
d_imap_capabilities_has_auth(
    struct d_imap_capabilities _caps,
    enum d_imap_auth_mechanism _mechanism
)
{
    if ((unsigned)_mechanism >= D_IMAP_AUTH_COUNT)
    {
        return false;
    }

    return ((_caps.auth & D_IMAP_AUTH_BIT(_mechanism)) != 0u);
}

/*
d_imap_capability_from_name
  A table search.
*/
bool
d_imap_capability_from_name(
    struct d_pack_text      _name,
    enum d_imap_capability* _capability
)
{
    size_t index;

    if ( (!_capability) ||
         (!d_imap_internal_text_is_valid(_name)) )
    {
        return false;
    }

    if (!d_imap_internal_table_find(d_imap_internal_capability_names,
                                    D_IMAP_CAPABILITY_COUNT,
                                    _name,
                                    &index))
    {
        return false;
    }

    *_capability = (enum d_imap_capability)index;

    return true;
}

/*
d_imap_internal_capability_add
  Files one advertised atom. Two shapes carry a value and are split before the
table search: "AUTH=mechanism" and "APPENDLIMIT=n" (RFC 7889, where a bare
APPENDLIMIT means the limit varies by mailbox). Everything else is a name
looked up whole, and a miss is counted rather than rejected.
*/
D_STATIC enum d_imap_error
d_imap_internal_capability_add(
    struct d_imap_capabilities* _caps,
    struct d_pack_text          _atom
)
{
    enum d_imap_auth_mechanism mechanism;
    enum d_imap_capability     capability;

    if (d_imap_internal_has_prefix_nocase(_atom,
                                          "AUTH="))
    {
        if (d_imap_auth_from_name(d_imap_internal_span(_atom.data,
                                                       5,
                                                       _atom.length),
                                  &mechanism))
        {
            _caps->auth |= D_IMAP_AUTH_BIT(mechanism);
        }
        else
        {
            ++_caps->unknown;
        }

        return D_IMAP_ERROR_NONE;
    }

    if (d_imap_internal_has_prefix_nocase(_atom,
                                          "APPENDLIMIT="))
    {
        _caps->set |= D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_APPENDLIMIT);

        return d_imap_internal_parse_decimal(
                   d_imap_internal_span(_atom.data,
                                        12,
                                        _atom.length),
                   D_IMAP_NUMBER64_MAX,
                   &_caps->append_limit);
    }

    if (d_imap_capability_from_name(_atom,
                                    &capability))
    {
        _caps->set |= D_IMAP_CAPABILITY_BIT(capability);
    }
    else
    {
        ++_caps->unknown;
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_capabilities_parse
  Accumulates into a local set and copies it out only on success, which is
what keeps the caller's set untouched on failure. Any lexer error means an
entry was not an atom, so all of them report as SYNTAX.
*/
enum d_imap_error
d_imap_capabilities_parse(
    struct d_pack_text          _list,
    struct d_imap_capabilities* _caps
)
{
    struct d_imap_capabilities caps = { 0, 0, 0, 0 };
    struct d_imap_lexer        lexer;
    struct d_imap_token        token;
    enum d_imap_error          error;

    if ( (!_caps) ||
         (!d_imap_internal_text_is_valid(_list)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    d_imap_lexer_init(&lexer,
                      _list);

    // every entry until the end of the line is one capability atom
    for (;;)
    {
        if (d_imap_lexer_next(&lexer,
                              0,
                              &token) != D_IMAP_ERROR_NONE)
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        if ( (token.kind == D_IMAP_TOKEN_END) ||
             (token.kind == D_IMAP_TOKEN_CRLF) )
        {
            break;
        }

        if ( (token.kind != D_IMAP_TOKEN_ATOM) &&
             (token.kind != D_IMAP_TOKEN_NUMBER) )
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        error = d_imap_internal_capability_add(&caps,
                                               token.text);

        if (error != D_IMAP_ERROR_NONE)
        {
            return error;
        }
    }

    *_caps = caps;

    return D_IMAP_ERROR_NONE;
}

// d_imap_internal_auth_preference
//   table: every mechanism, strongest first; the order d_imap_auth_select()
// documents.
static const enum d_imap_auth_mechanism d_imap_internal_auth_preference[] =
{
    D_IMAP_AUTH_SCRAM_SHA_256_PLUS, D_IMAP_AUTH_SCRAM_SHA_256,
    D_IMAP_AUTH_SCRAM_SHA_1_PLUS,   D_IMAP_AUTH_SCRAM_SHA_1,
    D_IMAP_AUTH_EXTERNAL,           D_IMAP_AUTH_GSSAPI,
    D_IMAP_AUTH_OAUTHBEARER,        D_IMAP_AUTH_XOAUTH2,
    D_IMAP_AUTH_PLAIN,              D_IMAP_AUTH_LOGIN_COMMAND,
    D_IMAP_AUTH_LOGIN,              D_IMAP_AUTH_CRAM_MD5,
    D_IMAP_AUTH_DIGEST_MD5,         D_IMAP_AUTH_NTLM,
    D_IMAP_AUTH_ANONYMOUS
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_auth_preference) ==
                  D_IMAP_AUTH_COUNT,
              "every mechanism has exactly one rank");

// d_imap_internal_auth_cleartext
//   constant: the mechanisms that reveal the secret itself to
// anyone reading the connection.
static const uint32_t d_imap_internal_auth_cleartext =
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_LOGIN_COMMAND) |
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_PLAIN)         |
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_LOGIN)         |
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_XOAUTH2)       |
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_OAUTHBEARER);

// d_imap_internal_auth_binding
//   constant: the channel-binding mechanisms, which bind to a TLS channel
// and so cannot run without one.
static const uint32_t d_imap_internal_auth_binding =
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_SCRAM_SHA_1_PLUS) |
    D_IMAP_AUTH_BIT(D_IMAP_AUTH_SCRAM_SHA_256_PLUS);

/*
d_imap_internal_auth_has_credentials
  Whether the options hold what a mechanism consumes: nothing for the three
that draw on context (a Kerberos ticket, a client certificate, or nothing at
all); a token for OAUTHBEARER, and a username too for XOAUTH2, whose initial
response names the user; a username and password for every other mechanism.
*/
D_STATIC bool
d_imap_internal_auth_has_credentials(
    const struct d_imap_options* _options,
    enum d_imap_auth_mechanism   _mechanism
)
{
    bool user  = (_options->username.length != 0);
    bool token = (_options->token.length != 0);

    if ( (_mechanism == D_IMAP_AUTH_GSSAPI) ||
         (_mechanism == D_IMAP_AUTH_EXTERNAL) ||
         (_mechanism == D_IMAP_AUTH_ANONYMOUS) )
    {
        return true;
    }

    if (_mechanism == D_IMAP_AUTH_OAUTHBEARER)
    {
        return token;
    }

    if (_mechanism == D_IMAP_AUTH_XOAUTH2)
    {
        return ( (user) &&
                 (token) );
    }

    return ( (user) &&
             (_options->password.length != 0) );
}

/*
d_imap_internal_auth_is_offered
  Whether the server allows a mechanism: an AUTH= entry for SASL, and for the
LOGIN command the absence of LOGINDISABLED, since RFC 3501 has no positive
advertisement for it.
*/
D_STATIC bool
d_imap_internal_auth_is_offered(
    struct d_imap_capabilities _caps,
    enum d_imap_auth_mechanism _mechanism
)
{
    if (_mechanism == D_IMAP_AUTH_LOGIN_COMMAND)
    {
        return !d_imap_capabilities_has(_caps,
                                        D_IMAP_CAPABILITY_LOGINDISABLED);
    }

    return d_imap_capabilities_has_auth(_caps,
                                        _mechanism);
}

/*
d_imap_auth_select
  One pass down the preference table. A candidate that fails only for want of
TLS is remembered, so that exhausting the table can say whether securing the
connection would have helped (SECURITY) or nothing would (UNSUPPORTED).
*/
enum d_imap_error
d_imap_auth_select(
    const struct d_imap_options* _options,
    struct d_imap_capabilities   _caps,
    uint32_t                     _supported,
    bool                         _secure,
    enum d_imap_auth_mechanism*  _mechanism
)
{
    uint32_t candidates;
    uint32_t needs_tls;
    bool     blocked = false;

    if ( (!_options) ||
         (!_mechanism) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    candidates = ( (_options->auth_mechanisms != 0u)
                       ? _options->auth_mechanisms
                       : D_IMAP_AUTH_DEFAULT_SET ) & _supported;
    needs_tls  = d_imap_internal_auth_binding;

    if ((_options->flags & D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH) == 0u)
    {
        needs_tls |= d_imap_internal_auth_cleartext;
    }

    for (size_t i = 0; i < D_IMAP_AUTH_COUNT; ++i)
    {
        enum d_imap_auth_mechanism mechanism =
            d_imap_internal_auth_preference[i];

        if ( ((candidates & D_IMAP_AUTH_BIT(mechanism)) == 0u) ||
             (!d_imap_internal_auth_is_offered(_caps,
                                               mechanism)) ||
             (!d_imap_internal_auth_has_credentials(_options,
                                                    mechanism)) )
        {
            continue;
        }

        if ( (!_secure) &&
             ((needs_tls & D_IMAP_AUTH_BIT(mechanism)) != 0u) )
        {
            blocked = true;

            continue;
        }

        *_mechanism = mechanism;

        return D_IMAP_ERROR_NONE;
    }

    return (blocked) ? D_IMAP_ERROR_SECURITY : D_IMAP_ERROR_UNSUPPORTED;
}

/*
d_imap_options_init
  Assignment from a static zero object rather than memset, so the text spans'
pointers are null pointers by the language's rules, not by assumption about
their representation.
*/
void
d_imap_options_init(
    struct d_imap_options* _options
)
{
    static const struct d_imap_options zero;

    if (_options)
    {
        *_options = zero;
    }

    return;
}

/*
d_imap_internal_host_is_valid
  A host is non-empty text with no space or control character. Octets above
0x7F pass, so an internationalized name can reach a backend that converts it.
*/
D_STATIC bool
d_imap_internal_host_is_valid(
    struct d_pack_text _host
)
{
    if ( (_host.length == 0) ||
         (!_host.data) )
    {
        return false;
    }

    for (size_t i = 0; i < _host.length; ++i)
    {
        unsigned char c = (unsigned char)_host.data[i];

        if ( (c <= 0x20) ||
             (c == 0x7F) )
        {
            return false;
        }
    }

    return true;
}

/*
d_imap_internal_credential_is_valid
  A credential may hold anything but NUL, which SASL PLAIN uses as its field
separator and which no LOGIN argument can carry.
*/
D_STATIC bool
d_imap_internal_credential_is_valid(
    struct d_pack_text _credential
)
{
    if (!d_imap_internal_text_is_valid(_credential))
    {
        return false;
    }

    return ( (_credential.length == 0) ||
             (memchr(_credential.data,
                     '\0',
                     _credential.length) == NULL) );
}

/*
d_imap_options_validate
  Structural checks first, then the one policy check: under SECURITY_NONE a
connection can never become encrypted, so a secret supplied for it could only
travel in the clear. That is refused here, before any connection exists,
rather than discovered at authentication time.
*/
enum d_imap_error
d_imap_options_validate(
    const struct d_imap_options* _options
)
{
    const uint32_t known_mechanisms =
        (uint32_t)((UINT64_C(1) << D_IMAP_AUTH_COUNT) - 1u);
    const uint32_t known_flags = D_IMAP_OPTION_NO_VERIFY_PEER |
                                 D_IMAP_OPTION_NO_VERIFY_HOST |
                                 D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH;

    if (!_options)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (_options->port > 65535u)
    {
        return D_IMAP_ERROR_RANGE;
    }

    if ( (_options->security < 0) ||
         (_options->security >= D_IMAP_SECURITY_COUNT) ||
         (!d_imap_internal_host_is_valid(_options->host)) ||
         ((_options->auth_mechanisms & ~known_mechanisms) != 0u) ||
         ((_options->flags & ~known_flags) != 0u) ||
         (!d_imap_internal_credential_is_valid(_options->username)) ||
         (!d_imap_internal_credential_is_valid(_options->password)) ||
         (!d_imap_internal_credential_is_valid(_options->token)) ||
         (!d_imap_internal_credential_is_valid(_options->authzid)) ||
         ( (_options->password.length != 0) &&
           (_options->username.length == 0) ) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_options->security == D_IMAP_SECURITY_NONE) &&
         ((_options->flags & D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH) == 0u) &&
         ( (_options->password.length != 0) ||
           (_options->token.length != 0) ) )
    {
        return D_IMAP_ERROR_SECURITY;
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_options_port
  An explicit port wins; otherwise the security mode decides. Out-of-range
values yield 0 rather than a truncated port, which would connect somewhere
the caller never asked for.
*/
uint16_t
d_imap_options_port(
    const struct d_imap_options* _options
)
{
    if ( (!_options) ||
         (_options->port > 65535u) ||
         (_options->security < 0) ||
         (_options->security >= D_IMAP_SECURITY_COUNT) )
    {
        return 0;
    }

    if (_options->port != 0u)
    {
        return (uint16_t)_options->port;
    }

    return d_imap_security_default_port(
               (enum d_imap_security)_options->security);
}

// d_imap_internal_flag_names
//   table: the flag names with their backslash or dollar, indexed by
// d_imap_flag.
static const char* const d_imap_internal_flag_names[] =
{
    "\\Seen", "\\Answered", "\\Flagged", "\\Deleted", "\\Draft", "\\Recent",
    "\\*", "$Forwarded", "$MDNSent", "$Junk", "$NotJunk", "$Phishing"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_flag_names) ==
                  D_IMAP_FLAG_COUNT,
              "one name per flag");

// d_imap_internal_mailbox_attribute_names
//   table: the mailbox attribute names, indexed by d_imap_mailbox_attribute.
static const char* const d_imap_internal_mailbox_attribute_names[] =
{
    "\\Noinferiors", "\\Noselect", "\\Marked", "\\Unmarked", "\\HasChildren",
    "\\HasNoChildren", "\\NonExistent", "\\Subscribed", "\\Remote", "\\All",
    "\\Archive", "\\Drafts", "\\Flagged", "\\Junk", "\\Sent", "\\Trash",
    "\\Important"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_mailbox_attribute_names) ==
                  D_IMAP_MAILBOX_ATTRIBUTE_COUNT,
              "one name per mailbox attribute");

// d_imap_internal_status_item_names
//   table: the STATUS item keywords, indexed by d_imap_status_item.
static const char* const d_imap_internal_status_item_names[] =
{
    "MESSAGES", "RECENT", "UIDNEXT", "UIDVALIDITY", "UNSEEN", "DELETED",
    "SIZE", "HIGHESTMODSEQ"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_status_item_names) ==
                  D_IMAP_STATUS_ITEM_COUNT,
              "one keyword per STATUS item");

/*
d_imap_flag_name
  A table lookup.
*/
const char*
d_imap_flag_name(
    enum d_imap_flag _flag
)
{
    return d_imap_internal_table_name(d_imap_internal_flag_names,
                                      D_IMAP_FLAG_COUNT,
                                      (int)_flag,
                                      "");
}

/*
d_imap_mailbox_attribute_name
  A table lookup.
*/
const char*
d_imap_mailbox_attribute_name(
    enum d_imap_mailbox_attribute _attribute
)
{
    return d_imap_internal_table_name(d_imap_internal_mailbox_attribute_names,
                                      D_IMAP_MAILBOX_ATTRIBUTE_COUNT,
                                      (int)_attribute,
                                      "");
}

/*
d_imap_status_item_name
  A table lookup.
*/
const char*
d_imap_status_item_name(
    enum d_imap_status_item _item
)
{
    return d_imap_internal_table_name(d_imap_internal_status_item_names,
                                      D_IMAP_STATUS_ITEM_COUNT,
                                      (int)_item,
                                      "");
}

/*
d_imap_flag_from_name
  A table search.
*/
bool
d_imap_flag_from_name(
    struct d_pack_text _name,
    enum d_imap_flag*  _flag
)
{
    size_t index;

    if ( (!_flag) ||
         (!d_imap_internal_text_is_valid(_name)) ||
         (!d_imap_internal_table_find(d_imap_internal_flag_names,
                                      D_IMAP_FLAG_COUNT,
                                      _name,
                                      &index)) )
    {
        return false;
    }

    *_flag = (enum d_imap_flag)index;

    return true;
}

/*
d_imap_mailbox_attribute_from_name
  A table search.
*/
bool
d_imap_mailbox_attribute_from_name(
    struct d_pack_text             _name,
    enum d_imap_mailbox_attribute* _attribute
)
{
    size_t index;

    if ( (!_attribute) ||
         (!d_imap_internal_text_is_valid(_name)) ||
         (!d_imap_internal_table_find(d_imap_internal_mailbox_attribute_names,
                                      D_IMAP_MAILBOX_ATTRIBUTE_COUNT,
                                      _name,
                                      &index)) )
    {
        return false;
    }

    *_attribute = (enum d_imap_mailbox_attribute)index;

    return true;
}

/*
d_imap_status_item_from_name
  A table search.
*/
bool
d_imap_status_item_from_name(
    struct d_pack_text       _name,
    enum d_imap_status_item* _item
)
{
    size_t index;

    if ( (!_item) ||
         (!d_imap_internal_text_is_valid(_name)) ||
         (!d_imap_internal_table_find(d_imap_internal_status_item_names,
                                      D_IMAP_STATUS_ITEM_COUNT,
                                      _name,
                                      &index)) )
    {
        return false;
    }

    *_item = (enum d_imap_status_item)index;

    return true;
}

/*
d_imap_internal_lex_name_list
  The shared body of the flag and attribute parsers: "(" then names then ")",
each name looked up in `_table` and anything else counted. Flags arrive as
FLAG tokens and keywords as atoms; NIL and all-digit atoms are keywords too,
if odd ones. Any lexer failure inside the list is a malformed list.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_name_list(
    struct d_imap_lexer* _lexer,
    const char* const*   _table,
    size_t               _count,
    uint32_t*            _set,
    uint32_t*            _unknown
)
{
    struct d_imap_token token;
    size_t              index;

    if ( (d_imap_lexer_next(_lexer,
                            0,
                            &token) != D_IMAP_ERROR_NONE) ||
         (token.kind != D_IMAP_TOKEN_LPAREN) )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    for (;;)
    {
        if (d_imap_lexer_next(_lexer,
                              0,
                              &token) != D_IMAP_ERROR_NONE)
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        if (token.kind == D_IMAP_TOKEN_RPAREN)
        {
            return D_IMAP_ERROR_NONE;
        }

        if ( (token.kind != D_IMAP_TOKEN_FLAG) &&
             (token.kind != D_IMAP_TOKEN_ATOM) &&
             (token.kind != D_IMAP_TOKEN_NUMBER) &&
             (token.kind != D_IMAP_TOKEN_NIL) )
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        if (d_imap_internal_table_find(_table,
                                       _count,
                                       token.text,
                                       &index))
        {
            *_set |= (uint32_t)1 << index;
        }
        else
        {
            ++*_unknown;
        }
    }
}

/*
d_imap_internal_expect_line_end
  Succeeds when nothing but an optional line break remains.
*/
D_STATIC enum d_imap_error
d_imap_internal_expect_line_end(
    struct d_imap_lexer* _lexer
)
{
    struct d_imap_token token;

    if ( (d_imap_lexer_next(_lexer,
                            0,
                            &token) != D_IMAP_ERROR_NONE) ||
         ( (token.kind != D_IMAP_TOKEN_END) &&
           (token.kind != D_IMAP_TOKEN_CRLF) ) )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_parse_name_list
  A whole-input name list: the list, then nothing but a line end. Results go
to locals first, so the caller's outputs are written only on success.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_name_list(
    struct d_pack_text _list,
    const char* const* _table,
    size_t             _count,
    uint32_t*          _set,
    uint32_t*          _unknown
)
{
    struct d_imap_lexer lexer;
    uint32_t            set     = 0;
    uint32_t            unknown = 0;
    enum d_imap_error   error;

    if ( (!_set) ||
         (!d_imap_internal_text_is_valid(_list)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    d_imap_lexer_init(&lexer,
                      _list);

    error = d_imap_internal_lex_name_list(&lexer,
                                          _table,
                                          _count,
                                          &set,
                                          &unknown);

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_expect_line_end(&lexer);
    }

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    *_set = set;

    if (_unknown)
    {
        *_unknown = unknown;
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_write_name_list
  Writes "(" + the names of the set bits in table order + ")", separated by
single spaces. The caller has already refused bits outside the table.
*/
D_STATIC enum d_imap_error
d_imap_internal_write_name_list(
    struct d_pack_sink _sink,
    const char* const* _table,
    size_t             _count,
    uint32_t           _set
)
{
    struct d_imap_internal_writer writer;
    bool                          first = true;

    d_imap_internal_writer_init(&writer,
                                _sink);
    d_imap_internal_writer_put(&writer,
                               '(');

    for (size_t i = 0; i < _count; ++i)
    {
        if ((_set & ((uint32_t)1 << i)) == 0u)
        {
            continue;
        }

        if (!first)
        {
            d_imap_internal_writer_put(&writer,
                                       ' ');
        }

        d_imap_internal_writer_write(&writer,
                                     _table[i],
                                     strlen(_table[i]));
        first = false;
    }

    d_imap_internal_writer_put(&writer,
                               ')');

    return d_imap_internal_writer_finish(&writer);
}

/*
d_imap_flags_parse
  The shared name-list parser over the flag table.
*/
enum d_imap_error
d_imap_flags_parse(
    struct d_pack_text _list,
    uint32_t*          _set,
    uint32_t*          _unknown
)
{
    return d_imap_internal_parse_name_list(_list,
                                           d_imap_internal_flag_names,
                                           D_IMAP_FLAG_COUNT,
                                           _set,
                                           _unknown);
}

/*
d_imap_flags_write
  Bits outside the vocabulary are refused rather than skipped: silently
dropping a flag from a STORE would change what the command does.
*/
enum d_imap_error
d_imap_flags_write(
    struct d_pack_sink _sink,
    uint32_t           _set
)
{
    const uint32_t known = (uint32_t)((UINT64_C(1) << D_IMAP_FLAG_COUNT) - 1u);

    if ( (!_sink.write) ||
         ((_set & ~known) != 0u) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    return d_imap_internal_write_name_list(_sink,
                                           d_imap_internal_flag_names,
                                           D_IMAP_FLAG_COUNT,
                                           _set);
}

/*
d_imap_mailbox_attributes_parse
  The shared name-list parser over the attribute table.
*/
enum d_imap_error
d_imap_mailbox_attributes_parse(
    struct d_pack_text _list,
    uint32_t*          _set,
    uint32_t*          _unknown
)
{
    return d_imap_internal_parse_name_list(
               _list,
               d_imap_internal_mailbox_attribute_names,
               D_IMAP_MAILBOX_ATTRIBUTE_COUNT,
               _set,
               _unknown);
}

/*
d_imap_status_items_write
  STATUS requires at least one item, so the empty set is refused along with
unknown bits; the grammar has no empty item list.
*/
enum d_imap_error
d_imap_status_items_write(
    struct d_pack_sink _sink,
    uint32_t           _items
)
{
    const uint32_t known =
        (uint32_t)((UINT64_C(1) << D_IMAP_STATUS_ITEM_COUNT) - 1u);

    if ( (!_sink.write) ||
         (_items == 0u) ||
         ((_items & ~known) != 0u) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    return d_imap_internal_write_name_list(_sink,
                                           d_imap_internal_status_item_names,
                                           D_IMAP_STATUS_ITEM_COUNT,
                                           _items);
}

/*
d_imap_internal_skip_value
  Consumes one value of any shape: a single token, or a parenthesized list
however deeply nested. It is what lets the STATUS parser step over an item
it does not know without knowing what the item's value looks like.
*/
D_STATIC enum d_imap_error
d_imap_internal_skip_value(
    struct d_imap_lexer* _lexer
)
{
    struct d_imap_token token;
    size_t              depth = 0;
    enum d_imap_error   error;

    do
    {
        error = d_imap_lexer_next(_lexer,
                                  0,
                                  &token);

        if (error != D_IMAP_ERROR_NONE)
        {
            return error;
        }

        if (token.kind == D_IMAP_TOKEN_END)
        {
            return D_IMAP_ERROR_INCOMPLETE;
        }

        if ( (token.kind == D_IMAP_TOKEN_CRLF) ||
             ( (token.kind == D_IMAP_TOKEN_RPAREN) &&
               (depth == 0) ) )
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        if (token.kind == D_IMAP_TOKEN_LPAREN)
        {
            ++depth;
        }
        else if (token.kind == D_IMAP_TOKEN_RPAREN)
        {
            --depth;
        }
    } while (depth > 0);

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_status_store
  Files one known item's value after checking that it fits: 32 bits for the
counts and UIDs, 63 for SIZE and HIGHESTMODSEQ.
*/
D_STATIC enum d_imap_error
d_imap_internal_status_store(
    struct d_imap_status_data* _data,
    enum d_imap_status_item    _item,
    uint64_t                   _value
)
{
    uint32_t* narrow = NULL;

    switch (_item)
    {
        case D_IMAP_STATUS_MESSAGES:    { narrow = &_data->messages;    break; }
        case D_IMAP_STATUS_RECENT:      { narrow = &_data->recent;      break; }
        case D_IMAP_STATUS_UIDNEXT:     { narrow = &_data->uidnext;     break; }
        case D_IMAP_STATUS_UIDVALIDITY: { narrow = &_data->uidvalidity; break; }
        case D_IMAP_STATUS_UNSEEN:      { narrow = &_data->unseen;      break; }
        case D_IMAP_STATUS_DELETED:     { narrow = &_data->deleted;     break; }
        default:                        { break; }
    }

    if (_value > ((narrow) ? D_IMAP_NUMBER_MAX : D_IMAP_NUMBER64_MAX))
    {
        return D_IMAP_ERROR_RANGE;
    }

    if (narrow)
    {
        *narrow = (uint32_t)_value;
    }
    else if (_item == D_IMAP_STATUS_SIZE)
    {
        _data->size = _value;
    }
    else
    {
        _data->highestmodseq = _value;
    }

    _data->present |= D_IMAP_STATUS_BIT(_item);

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_status_pair
  Reads one "name value" pair after its name: a known item must carry a
number, and an unknown one's value is skipped whole.
*/
D_STATIC enum d_imap_error
d_imap_internal_status_pair(
    struct d_imap_lexer*       _lexer,
    struct d_pack_text         _name,
    struct d_imap_status_data* _data
)
{
    struct d_imap_token     token;
    enum d_imap_status_item item;
    enum d_imap_error       error;

    if (!d_imap_status_item_from_name(_name,
                                      &item))
    {
        return d_imap_internal_skip_value(_lexer);
    }

    error = d_imap_lexer_next(_lexer,
                              0,
                              &token);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    if (token.kind == D_IMAP_TOKEN_END)
    {
        return D_IMAP_ERROR_INCOMPLETE;
    }

    if (token.kind != D_IMAP_TOKEN_NUMBER)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    return d_imap_internal_status_store(_data,
                                        item,
                                        token.number);
}

/*
d_imap_internal_status_pairs
  The pairs of a STATUS list, after its "(" and through its ")". Running out
of input before the ")" is INCOMPLETE rather than SYNTAX, since more of the
line may yet arrive.
*/
D_STATIC enum d_imap_error
d_imap_internal_status_pairs(
    struct d_imap_lexer*       _lexer,
    struct d_imap_status_data* _data
)
{
    struct d_imap_token token;
    enum d_imap_error   error = D_IMAP_ERROR_NONE;

    // each pair is a name atom and its value, until ")"
    while (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_lexer_next(_lexer,
                                  0,
                                  &token);

        if ( (error != D_IMAP_ERROR_NONE) ||
             (token.kind == D_IMAP_TOKEN_RPAREN) )
        {
            break;
        }

        if (token.kind == D_IMAP_TOKEN_END)
        {
            error = D_IMAP_ERROR_INCOMPLETE;
        }
        else if (token.kind != D_IMAP_TOKEN_ATOM)
        {
            error = D_IMAP_ERROR_SYNTAX;
        }
        else
        {
            error = d_imap_internal_status_pair(_lexer,
                                                token.text,
                                                _data);
        }
    }

    return error;
}

/*
d_imap_status_data_parse
  "(" then the pairs then nothing but a line end, parsed into a local that is
copied out only on success.
*/
enum d_imap_error
d_imap_status_data_parse(
    struct d_pack_text         _list,
    struct d_imap_status_data* _data
)
{
    static const struct d_imap_status_data empty;
    struct d_imap_status_data              data = empty;
    struct d_imap_lexer                    lexer;
    struct d_imap_token                    token;
    enum d_imap_error                      error;

    if ( (!_data) ||
         (!d_imap_internal_text_is_valid(_list)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    d_imap_lexer_init(&lexer,
                      _list);
    error = d_imap_lexer_next(&lexer,
                              0,
                              &token);

    if ( (error == D_IMAP_ERROR_NONE) &&
         (token.kind != D_IMAP_TOKEN_LPAREN) )
    {
        error = (token.kind == D_IMAP_TOKEN_END) ? D_IMAP_ERROR_INCOMPLETE
                                                 : D_IMAP_ERROR_SYNTAX;
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_status_pairs(&lexer,
                                             &data);
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_expect_line_end(&lexer);
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        *_data = data;
    }

    return error;
}

/*
d_imap_internal_string_class
  What one pass over a string learns: whether every octet may appear in an
atom, whether no octet forbids a quoted string, and whether the string holds
a NUL, which no form can carry.
*/
struct d_imap_internal_string_class
{
    bool atom;
    bool quotable;
    bool has_nul;
};

/*
d_imap_internal_classify
  Classifies a string for the writers. Control characters send a string to a
literal even though the grammar would quote them (see imap.h section 9.2).
Octets above 0x7F are quotable only as valid UTF-8 under D_IMAP_STRING_UTF8.
"NIL" and the empty string are never atoms.
*/
D_STATIC struct d_imap_internal_string_class
d_imap_internal_classify(
    struct d_pack_text _value,
    uint32_t           _flags
)
{
    struct d_imap_internal_string_class result = { true, true, false };
    bool eight_bit = false;
    bool wildcards = ((_flags & D_IMAP_STRING_LIST_WILDCARDS) != 0u);

    for (size_t i = 0; i < _value.length; ++i)
    {
        unsigned char c = (unsigned char)_value.data[i];

        if (c >= 0x80)
        {
            eight_bit   = true;
            result.atom = false;
        }
        else if ( (c < 0x20) ||
                  (c == 0x7F) )
        {
            result.has_nul  = (result.has_nul || (c == 0));
            result.atom     = false;
            result.quotable = false;
        }
        else if ( (!d_imap_internal_is_astring_char(c)) &&
                  ( (!wildcards) ||
                    ( (c != '%') &&
                      (c != '*') ) ) )
        {
            result.atom = false;
        }
    }

    if ( (eight_bit) &&
         ( ((_flags & D_IMAP_STRING_UTF8) == 0u) ||
           (!d_imap_internal_utf8_is_valid(_value)) ) )
    {
        result.quotable = false;
    }

    if ( (_value.length == 0) ||
         (d_imap_internal_equals_nocase(_value,
                                        "NIL")) )
    {
        result.atom = false;
    }

    return result;
}

/*
d_imap_internal_literal_form
  Non-synchronizing when LITERAL+ allows any size, or LITERAL- allows this
one; synchronizing otherwise.
*/
D_STATIC enum d_imap_string_form
d_imap_internal_literal_form(
    uint64_t _size,
    uint32_t _flags
)
{
    if ( ((_flags & D_IMAP_STRING_LITERAL_PLUS) != 0u) ||
         ( ((_flags & D_IMAP_STRING_LITERAL_MINUS) != 0u) &&
           (_size <= D_IMAP_LITERAL_MINUS_MAX) ) )
    {
        return D_IMAP_STRING_LITERAL_NONSYNC;
    }

    return D_IMAP_STRING_LITERAL;
}

/*
d_imap_internal_write_quoted
  Writes '"', the escaped value, and '"'. Runs between specials go out whole;
each special is preceded by a backslash and then leads the next run.
*/
D_STATIC enum d_imap_error
d_imap_internal_write_quoted(
    struct d_pack_sink _sink,
    struct d_pack_text _value
)
{
    enum d_imap_error error = d_imap_internal_emit(_sink,
                                                   "\"",
                                                   1);
    size_t            start = 0;

    for (size_t i = 0; i < _value.length; ++i)
    {
        if ( (_value.data[i] == '"') ||
             (_value.data[i] == '\\') )
        {
            error = d_imap_internal_emit_after(error,
                                               _sink,
                                               _value.data + start,
                                               i - start);
            error = d_imap_internal_emit_after(error,
                                               _sink,
                                               "\\",
                                               1);
            start = i;
        }
    }

    error = d_imap_internal_emit_after(error,
                                       _sink,
                                       _value.data + start,
                                       _value.length - start);

    return d_imap_internal_emit_after(error,
                                      _sink,
                                      "\"",
                                      1);
}

/*
d_imap_internal_write_literal_prefix
  Builds "{n}" or "{n+}" and the CRLF in one buffer: one brace, twenty digits,
the plus, the other brace, and two octets of line break.
*/
D_STATIC enum d_imap_error
d_imap_internal_write_literal_prefix(
    struct d_pack_sink      _sink,
    uint64_t                _size,
    enum d_imap_string_form _form
)
{
    char   line[25];
    size_t used = 0;

    line[used++] = '{';
    d_imap_internal_append_decimal(line,
                                   &used,
                                   _size,
                                   1);

    if (_form == D_IMAP_STRING_LITERAL_NONSYNC)
    {
        line[used++] = '+';
    }

    line[used++] = '}';
    line[used++] = '\r';
    line[used++] = '\n';

    return d_imap_internal_emit(_sink,
                                line,
                                used);
}

/*
d_imap_astring_form
  The classification, reduced to a form: atom, else quoted, else a literal
whose synchronization the flags decide.
*/
enum d_imap_error
d_imap_astring_form(
    struct d_pack_text       _value,
    uint32_t                 _flags,
    enum d_imap_string_form* _form
)
{
    struct d_imap_internal_string_class kind;

    if ( (!_form) ||
         (!d_imap_internal_text_is_valid(_value)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    kind = d_imap_internal_classify(_value,
                                    _flags);

    if (kind.has_nul)
    {
        return D_IMAP_ERROR_ENCODING;
    }

    if (kind.atom)
    {
        *_form = D_IMAP_STRING_ATOM;
    }
    else if (kind.quotable)
    {
        *_form = D_IMAP_STRING_QUOTED;
    }
    else
    {
        *_form = d_imap_internal_literal_form(_value.length,
                                              _flags);
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_astring_write
  The form is decided before any octet is written, which is what lets a
literal be refused for want of `_form` without leaving half a command behind.
*/
enum d_imap_error
d_imap_astring_write(
    struct d_pack_sink       _sink,
    struct d_pack_text       _value,
    uint32_t                 _flags,
    enum d_imap_string_form* _form
)
{
    enum d_imap_string_form form;
    enum d_imap_error       error;

    if (!_sink.write)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    error = d_imap_astring_form(_value,
                                _flags,
                                &form);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    if ( (!_form) &&
         (form != D_IMAP_STRING_ATOM) &&
         (form != D_IMAP_STRING_QUOTED) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (_form)
    {
        *_form = form;
    }

    if (form == D_IMAP_STRING_ATOM)
    {
        return d_imap_internal_emit(_sink,
                                    _value.data,
                                    _value.length);
    }

    if (form == D_IMAP_STRING_QUOTED)
    {
        return d_imap_internal_write_quoted(_sink,
                                            _value);
    }

    return d_imap_internal_write_literal_prefix(_sink,
                                                _value.length,
                                                form);
}

/*
d_imap_quoted_write
  Quotes regardless of whether an atom would do, for arguments the grammar
requires quoted, such as SEARCH text under some servers' parsers.
*/
enum d_imap_error
d_imap_quoted_write(
    struct d_pack_sink _sink,
    struct d_pack_text _value,
    uint32_t           _flags
)
{
    if ( (!_sink.write) ||
         (!d_imap_internal_text_is_valid(_value)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (!d_imap_internal_classify(_value,
                                  _flags).quotable)
    {
        return D_IMAP_ERROR_ENCODING;
    }

    return d_imap_internal_write_quoted(_sink,
                                        _value);
}

/*
d_imap_literal_prefix_write
  The number64 bound is checked here because it is the grammar's, and a
larger count could never be announced validly.
*/
enum d_imap_error
d_imap_literal_prefix_write(
    struct d_pack_sink       _sink,
    uint64_t                 _size,
    uint32_t                 _flags,
    enum d_imap_string_form* _form
)
{
    enum d_imap_string_form form;

    if (!_sink.write)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (_size > D_IMAP_NUMBER64_MAX)
    {
        return D_IMAP_ERROR_RANGE;
    }

    form = d_imap_internal_literal_form(_size,
                                        _flags);

    if (_form)
    {
        *_form = form;
    }

    return d_imap_internal_write_literal_prefix(_sink,
                                                _size,
                                                form);
}

/*
d_imap_quoted_unescape
  Emits the runs between escapes whole. An escape's second character is kept
as the first of the next run, which drops exactly the backslash.
*/
enum d_imap_error
d_imap_quoted_unescape(
    struct d_pack_sink _sink,
    struct d_pack_text _content
)
{
    enum d_imap_error error = D_IMAP_ERROR_NONE;
    size_t            start = 0;
    size_t            i     = 0;

    if ( (!_sink.write) ||
         (!d_imap_internal_text_is_valid(_content)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    while (i < _content.length)
    {
        if (_content.data[i] != '\\')
        {
            ++i;

            continue;
        }

        // only the two quoted-specials may follow a backslash
        if ( (i + 1 >= _content.length) ||
             ( (_content.data[i + 1] != '"') &&
               (_content.data[i + 1] != '\\') ) )
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        error = d_imap_internal_emit_after(error,
                                           _sink,
                                           _content.data + start,
                                           i - start);
        start = i + 1;
        i    += 2;
    }

    return d_imap_internal_emit_after(error,
                                      _sink,
                                      _content.data + start,
                                      _content.length - start);
}

// d_imap_internal_base64
//   table: the modified BASE64 alphabet of RFC 3501 section 5.1.3: standard
// BASE64 with "," in place of "/", and no padding.
static const char d_imap_internal_base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,";

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_base64) == 64 + 1,
              "the alphabet has 64 characters and a terminator");

/*
d_imap_internal_base64_value
  A character's sextet, or -1 for one outside the modified alphabet.
*/
D_STATIC int
d_imap_internal_base64_value(
    unsigned char _c
)
{
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return _c - 'A';
    }

    if ( (_c >= 'a') &&
         (_c <= 'z') )
    {
        return _c - 'a' + 26;
    }

    if ( (_c >= '0') &&
         (_c <= '9') )
    {
        return _c - '0' + 52;
    }

    if (_c == '+')
    {
        return 62;
    }

    return (_c == ',') ? 63 : -1;
}

/*
d_imap_internal_mutf7_encoder
  An encoding in progress: whether a BASE64 run is open, and the bits not yet
written out as a character. At most 16 + 5 bits are ever pending, so 32 bits
of storage never overflow.
*/
struct d_imap_internal_mutf7_encoder
{
    struct d_imap_internal_writer writer;
    uint32_t                      bits;
    uint32_t                      count;
    bool                          shifted;
    bool                          escape;
};

/*
d_imap_internal_mutf7_unit
  Adds one UTF-16 code unit to the open run, opening it with "&" if needed,
and writes every whole sextet now available.
*/
D_STATIC void
d_imap_internal_mutf7_unit(
    struct d_imap_internal_mutf7_encoder* _encoder,
    uint32_t                              _unit
)
{
    if (!_encoder->shifted)
    {
        d_imap_internal_writer_put(&_encoder->writer,
                                   '&');
        _encoder->shifted = true;
    }

    _encoder->bits   = (_encoder->bits << 16) | _unit;
    _encoder->count += 16;

    // write every whole sextet, most significant first
    while (_encoder->count >= 6)
    {
        uint32_t sextet;

        _encoder->count -= 6;
        sextet           = (_encoder->bits >> _encoder->count) & 0x3Fu;
        d_imap_internal_writer_put(&_encoder->writer,
                                   d_imap_internal_base64[sextet]);
    }

    // keep only the bits still owed, so the next shift loses none
    _encoder->bits &= (1u << _encoder->count) - 1u;

    return;
}

/*
d_imap_internal_mutf7_close
  Ends an open run: the leftover bits, zero-padded to a sextet, then "-". The
decoder insists on that zero padding, so the encoder must produce it.
*/
D_STATIC void
d_imap_internal_mutf7_close(
    struct d_imap_internal_mutf7_encoder* _encoder
)
{
    if (!_encoder->shifted)
    {
        return;
    }

    if (_encoder->count > 0)
    {
        uint32_t sextet = (_encoder->bits << (6 - _encoder->count)) & 0x3Fu;

        d_imap_internal_writer_put(&_encoder->writer,
                                   d_imap_internal_base64[sextet]);
    }

    d_imap_internal_writer_put(&_encoder->writer,
                               '-');
    _encoder->bits    = 0;
    _encoder->count   = 0;
    _encoder->shifted = false;

    return;
}

/*
d_imap_internal_mutf7_direct
  Writes a printable character as itself: "&" as "&-", and, when the output
is headed into a quoted string, '"' and '\' behind a backslash. Those two can
only appear here, never inside a BASE64 run, which is why escaping at this one
point is enough to make the whole output quotable.
*/
D_STATIC void
d_imap_internal_mutf7_direct(
    struct d_imap_internal_mutf7_encoder* _encoder,
    char                                  _c
)
{
    d_imap_internal_mutf7_close(_encoder);

    if ( (_encoder->escape) &&
         ( (_c == '"') ||
           (_c == '\\') ) )
    {
        d_imap_internal_writer_put(&_encoder->writer,
                                   '\\');
    }

    d_imap_internal_writer_put(&_encoder->writer,
                               _c);

    if (_c == '&')
    {
        d_imap_internal_writer_put(&_encoder->writer,
                                   '-');
    }

    return;
}

/*
d_imap_internal_mutf7_encode
  The encoder proper, over input already known to be valid UTF-8 without NUL.
Printable US-ASCII stands for itself; everything else joins a BASE64 run as
UTF-16, a supplementary character as its surrogate pair.
*/
D_STATIC enum d_imap_error
d_imap_internal_mutf7_encode(
    struct d_pack_sink _sink,
    struct d_pack_text _utf8,
    bool               _escape
)
{
    struct d_imap_internal_mutf7_encoder encoder;
    const unsigned char* data     = (const unsigned char*)_utf8.data;
    size_t               position = 0;

    d_imap_internal_writer_init(&encoder.writer,
                                _sink);
    encoder.bits    = 0;
    encoder.count   = 0;
    encoder.shifted = false;
    encoder.escape  = _escape;

    while (position < _utf8.length)
    {
        uint32_t code_point = 0;

        position += d_imap_internal_utf8_next(data + position,
                                              _utf8.length - position,
                                              &code_point);

        if ( (code_point >= 0x20u) &&
             (code_point <= 0x7Eu) )
        {
            d_imap_internal_mutf7_direct(&encoder,
                                         (char)code_point);
        }
        else if (code_point < 0x10000u)
        {
            d_imap_internal_mutf7_unit(&encoder,
                                       code_point);
        }
        else
        {
            uint32_t offset = code_point - 0x10000u;

            d_imap_internal_mutf7_unit(&encoder,
                                       0xD800u | (offset >> 10));
            d_imap_internal_mutf7_unit(&encoder,
                                       0xDC00u | (offset & 0x3FFu));
        }
    }

    d_imap_internal_mutf7_close(&encoder);

    return d_imap_internal_writer_finish(&encoder.writer);
}

/*
d_imap_internal_name_is_encodable
  Mailbox input must be valid UTF-8 without NUL. Checked in full before the
encoder writes anything, so bad input never leaves a partial name behind.
*/
D_STATIC bool
d_imap_internal_name_is_encodable(
    struct d_pack_text _name
)
{
    if ( (_name.length != 0) &&
         (memchr(_name.data,
                 '\0',
                 _name.length) != NULL) )
    {
        return false;
    }

    return d_imap_internal_utf8_is_valid(_name);
}

/*
d_imap_mutf7_encode
  Validation, then the encoder without escaping.
*/
enum d_imap_error
d_imap_mutf7_encode(
    struct d_pack_sink _sink,
    struct d_pack_text _utf8
)
{
    if ( (!_sink.write) ||
         (!d_imap_internal_text_is_valid(_utf8)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (!d_imap_internal_name_is_encodable(_utf8))
    {
        return D_IMAP_ERROR_ENCODING;
    }

    return d_imap_internal_mutf7_encode(_sink,
                                        _utf8,
                                        false);
}

/*
d_imap_internal_mutf7_accept
  Takes one decoded UTF-16 unit. A high surrogate waits in `*_high` for its
partner; any other unit must not be the partner of nothing, and must not be a
character that was required to represent itself: printable US-ASCII, or NUL,
which no mailbox name may hold.
*/
D_STATIC enum d_imap_error
d_imap_internal_mutf7_accept(
    struct d_imap_internal_writer* _writer,
    uint32_t                       _unit,
    uint32_t*                      _high
)
{
    if ( (_unit >= 0xD800u) &&
         (_unit <= 0xDBFFu) )
    {
        if (*_high != 0u)
        {
            return D_IMAP_ERROR_ENCODING;
        }

        *_high = _unit;

        return D_IMAP_ERROR_NONE;
    }

    if ( (_unit >= 0xDC00u) &&
         (_unit <= 0xDFFFu) )
    {
        if (*_high == 0u)
        {
            return D_IMAP_ERROR_ENCODING;
        }

        d_imap_internal_writer_utf8(_writer,
                                    0x10000u +
                                    ((*_high - 0xD800u) << 10) +
                                    (_unit - 0xDC00u));
        *_high = 0;

        return D_IMAP_ERROR_NONE;
    }

    if ( (*_high != 0u) ||
         (_unit == 0u) ||
         ( (_unit >= 0x20u) &&
           (_unit <= 0x7Eu) ) )
    {
        return D_IMAP_ERROR_ENCODING;
    }

    d_imap_internal_writer_utf8(_writer,
                                _unit);

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_mutf7_run
  Decodes the BASE64 run whose "&" is at `*_position`, through its closing
"-". Every 16 bits make a unit. The run must yield at least one unit, end with
no surrogate pending, and leave fewer than six padding bits, all zero; those
are the conditions under which exactly one encoding exists for a name, which
is what makes names comparable after decoding.
*/
D_STATIC enum d_imap_error
d_imap_internal_mutf7_run(
    struct d_imap_internal_writer* _writer,
    struct d_pack_text             _text,
    size_t*                        _position
)
{
    size_t            i     = *_position + 1;
    uint32_t          bits  = 0;
    uint32_t          count = 0;
    uint32_t          high  = 0;
    bool              any   = false;
    enum d_imap_error error;

    for (; (i < _text.length) && (_text.data[i] != '-'); ++i)
    {
        int value = d_imap_internal_base64_value((unsigned char)_text.data[i]);

        if (value < 0)
        {
            return D_IMAP_ERROR_ENCODING;
        }

        bits   = (bits << 6) | (uint32_t)value;
        count += 6;

        if (count >= 16)
        {
            count -= 16;
            error  = d_imap_internal_mutf7_accept(_writer,
                                                  (bits >> count) & 0xFFFFu,
                                                  &high);
            bits  &= (1u << count) - 1u;
            any    = true;

            if (error != D_IMAP_ERROR_NONE)
            {
                return error;
            }
        }
    }

    if ( (i >= _text.length) ||
         (!any) ||
         (high != 0u) ||
         (count >= 6) ||
         (bits != 0u) )
    {
        return D_IMAP_ERROR_ENCODING;
    }

    *_position = i + 1;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_mutf7_decode
  The strict decoder. `after_run` catches a null shift: a run that closes and
is immediately reopened, which RFC 3501 forbids because the two runs should
have been one. With `_escaped`, the input is a quoted string's raw content;
its escapes can only stand for '"' and '\', both printable, so they are
resolved on the direct path and a backslash inside a run is simply invalid.
*/
D_STATIC enum d_imap_error
d_imap_internal_mutf7_decode(
    struct d_pack_sink _sink,
    struct d_pack_text _text,
    bool               _escaped
)
{
    struct d_imap_internal_writer writer;
    size_t                        i         = 0;
    bool                          after_run = false;
    enum d_imap_error             error     = D_IMAP_ERROR_NONE;

    d_imap_internal_writer_init(&writer,
                                _sink);

    while ( (i < _text.length) &&
            (error == D_IMAP_ERROR_NONE) )
    {
        unsigned char c    = (unsigned char)_text.data[i];
        bool          amp  = (c == '&');
        bool          lone = ( (amp) &&
                               (i + 1 < _text.length) &&
                               (_text.data[i + 1] == '-') );

        if ( (amp) &&
             (!lone) )
        {
            error     = (after_run) ? D_IMAP_ERROR_ENCODING
                                    : d_imap_internal_mutf7_run(&writer,
                                                                _text,
                                                                &i);
            after_run = true;

            continue;
        }

        if ( (_escaped) &&
             (c == '\\') )
        {
            ++i;
            c = (i < _text.length) ? (unsigned char)_text.data[i] : 0;
            error = ( (c == '"') || (c == '\\') ) ? D_IMAP_ERROR_NONE
                                                  : D_IMAP_ERROR_SYNTAX;
        }
        else if ( (c < 0x20) ||
                  (c > 0x7E) )
        {
            error = D_IMAP_ERROR_ENCODING;
        }

        d_imap_internal_writer_put(&writer,
                                   (char)c);
        i        += (lone) ? 2u : 1u;
        after_run = false;
    }

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    return d_imap_internal_writer_finish(&writer);
}

/*
d_imap_mutf7_decode
  The strict decoder over unescaped input.
*/
enum d_imap_error
d_imap_mutf7_decode(
    struct d_pack_sink _sink,
    struct d_pack_text _mutf7
)
{
    if ( (!_sink.write) ||
         (!d_imap_internal_text_is_valid(_mutf7)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    return d_imap_internal_mutf7_decode(_sink,
                                        _mutf7,
                                        false);
}

/*
d_imap_internal_name_needs_quotes
  Whether a name's modified UTF-7 form must be quoted. That form's direct
characters are exactly the name's printable US-ASCII characters, and its
BASE64 runs use only atom-safe characters ("&", "-", letters, digits, "+",
","), so the question is answerable from the UTF-8 name without encoding it.
*/
D_STATIC bool
d_imap_internal_name_needs_quotes(
    struct d_pack_text _name,
    uint32_t           _flags
)
{
    bool wildcards = ((_flags & D_IMAP_STRING_LIST_WILDCARDS) != 0u);

    if ( (_name.length == 0) ||
         (d_imap_internal_equals_nocase(_name,
                                        "NIL")) )
    {
        return true;
    }

    for (size_t i = 0; i < _name.length; ++i)
    {
        unsigned char c = (unsigned char)_name.data[i];

        if ( (c >= 0x20) &&
             (c <= 0x7E) &&
             (!d_imap_internal_is_astring_char(c)) &&
             ( (!wildcards) ||
               ( (c != '%') &&
                 (c != '*') ) ) )
        {
            return true;
        }
    }

    return false;
}

/*
d_imap_mailbox_write
  Under UTF8=ACCEPT a name is an ordinary astring. Otherwise it is encoded
straight into the sink, inside quotes when needed, with the encoder escaping
the only two characters that quoting requires; no intermediate copy exists.
*/
enum d_imap_error
d_imap_mailbox_write(
    struct d_pack_sink       _sink,
    struct d_pack_text       _name,
    uint32_t                 _flags,
    enum d_imap_string_form* _form
)
{
    bool              quoted;
    enum d_imap_error error = D_IMAP_ERROR_NONE;

    if ( (!_sink.write) ||
         (!d_imap_internal_text_is_valid(_name)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if ((_flags & D_IMAP_STRING_UTF8) != 0u)
    {
        return d_imap_astring_write(_sink,
                                    _name,
                                    _flags,
                                    _form);
    }

    if (!d_imap_internal_name_is_encodable(_name))
    {
        return D_IMAP_ERROR_ENCODING;
    }

    quoted = d_imap_internal_name_needs_quotes(_name,
                                               _flags);

    if (_form)
    {
        *_form = (quoted) ? D_IMAP_STRING_QUOTED : D_IMAP_STRING_ATOM;
    }

    if (quoted)
    {
        error = d_imap_internal_emit(_sink,
                                     "\"",
                                     1);
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_mutf7_encode(_sink,
                                             _name,
                                             quoted);
    }

    if (!quoted)
    {
        return error;
    }

    return d_imap_internal_emit_after(error,
                                      _sink,
                                      "\"",
                                      1);
}

/*
d_imap_mailbox_is_inbox
  RFC 3501 section 5.1: INBOX is case-insensitive; every other name compares
exactly.
*/
bool
d_imap_mailbox_is_inbox(
    struct d_pack_text _name
)
{
    if (!d_imap_internal_text_is_valid(_name))
    {
        return false;
    }

    return d_imap_internal_equals_nocase(_name,
                                         "INBOX");
}

/*
d_imap_internal_writer_seq_number
  Writes one sequence number, or "*" for D_IMAP_SEQ_STAR.
*/
D_STATIC void
d_imap_internal_writer_seq_number(
    struct d_imap_internal_writer* _writer,
    uint32_t                       _value
)
{
    if (_value == D_IMAP_SEQ_STAR)
    {
        d_imap_internal_writer_put(_writer,
                                   '*');
    }
    else
    {
        d_imap_internal_writer_decimal(_writer,
                                       _value);
    }

    return;
}

/*
d_imap_seqset_write
  Ranges go out as given, neither sorted nor merged: IMAP accepts any order,
and the caller's may be deliberate.
*/
enum d_imap_error
d_imap_seqset_write(
    struct d_pack_sink             _sink,
    const struct d_imap_seq_range* _ranges,
    size_t                         _count
)
{
    struct d_imap_internal_writer writer;

    if ( (!_sink.write) ||
         (!_ranges) ||
         (_count == 0) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    d_imap_internal_writer_init(&writer,
                                _sink);

    for (size_t i = 0; i < _count; ++i)
    {
        if (i > 0)
        {
            d_imap_internal_writer_put(&writer,
                                       ',');
        }

        d_imap_internal_writer_seq_number(&writer,
                                          _ranges[i].first);

        // a range whose ends agree is a single number
        if (_ranges[i].last != _ranges[i].first)
        {
            d_imap_internal_writer_put(&writer,
                                       ':');
            d_imap_internal_writer_seq_number(&writer,
                                              _ranges[i].last);
        }
    }

    return d_imap_internal_writer_finish(&writer);
}

/*
d_imap_internal_seq_number
  Reads "*" or a nz-number at `*_position` and advances past it. Leading
zeros are tolerated; the value 0 is not, since it names no message.
*/
D_STATIC enum d_imap_error
d_imap_internal_seq_number(
    struct d_pack_text _text,
    size_t*            _position,
    uint32_t*          _value
)
{
    size_t            end   = *_position;
    uint64_t          value = 0;
    enum d_imap_error error;

    if ( (end < _text.length) &&
         (_text.data[end] == '*') )
    {
        *_value    = D_IMAP_SEQ_STAR;
        *_position = end + 1;

        return D_IMAP_ERROR_NONE;
    }

    while ( (end < _text.length) &&
            (d_imap_internal_is_digit((unsigned char)_text.data[end])) )
    {
        ++end;
    }

    error = d_imap_internal_parse_decimal(d_imap_internal_span(_text.data,
                                                               *_position,
                                                               end),
                                          D_IMAP_NUMBER_MAX,
                                          &value);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    if (value == 0u)
    {
        return D_IMAP_ERROR_RANGE;
    }

    *_value    = (uint32_t)value;
    *_position = end;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_seq_range
  Reads one element: a number, or two joined by ":".
*/
D_STATIC enum d_imap_error
d_imap_internal_seq_range(
    struct d_pack_text       _text,
    size_t*                  _position,
    struct d_imap_seq_range* _range
)
{
    enum d_imap_error error = d_imap_internal_seq_number(_text,
                                                         _position,
                                                         &_range->first);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    _range->last = _range->first;

    if ( (*_position < _text.length) &&
         (_text.data[*_position] == ':') )
    {
        ++*_position;
        error = d_imap_internal_seq_number(_text,
                                           _position,
                                           &_range->last);
    }

    return error;
}

/*
d_imap_seqset_parse
  One left-to-right pass. Elements beyond the capacity are still parsed and
counted, so a measuring call and a filling call agree on the count and on
whether the text is valid at all.
*/
enum d_imap_error
d_imap_seqset_parse(
    struct d_pack_text       _text,
    struct d_imap_seq_range* _ranges,
    size_t                   _capacity,
    size_t*                  _count
)
{
    size_t position = 0;
    size_t count    = 0;

    if ( (!_count) ||
         (!d_imap_internal_text_is_valid(_text)) ||
         ( (!_ranges) &&
           (_capacity != 0) ) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    // elements separated by commas, to the end of the text
    for (;;)
    {
        struct d_imap_seq_range range;
        enum d_imap_error       error = d_imap_internal_seq_range(_text,
                                                                  &position,
                                                                  &range);

        if (error != D_IMAP_ERROR_NONE)
        {
            return error;
        }

        if (count < _capacity)
        {
            _ranges[count] = range;
        }

        ++count;

        if (position >= _text.length)
        {
            break;
        }

        if (_text.data[position] != ',')
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        ++position;
    }

    *_count = count;

    return (count > _capacity) ? D_IMAP_ERROR_CAPACITY : D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_seq_resolve
  A range end with "*" resolved to the largest number in use.
*/
D_STATIC uint32_t
d_imap_internal_seq_resolve(
    uint32_t _value,
    uint32_t _largest
)
{
    return (_value == D_IMAP_SEQ_STAR) ? _largest : _value;
}

/*
d_imap_seqset_contains
  Each range is resolved and then ordered, so "4:2" behaves as "2:4", which
RFC 9051 section 9 requires. An empty mailbox resolves "*" to 0, which no
value matches.
*/
bool
d_imap_seqset_contains(
    const struct d_imap_seq_range* _ranges,
    size_t                         _count,
    uint32_t                       _value,
    uint32_t                       _largest
)
{
    if ( (!_ranges) ||
         (_value == 0u) )
    {
        return false;
    }

    for (size_t i = 0; i < _count; ++i)
    {
        uint32_t first = d_imap_internal_seq_resolve(_ranges[i].first,
                                                     _largest);
        uint32_t last  = d_imap_internal_seq_resolve(_ranges[i].last,
                                                     _largest);
        uint32_t low   = (first < last) ? first : last;
        uint32_t high  = (first < last) ? last : first;

        if ( (_value >= low) &&
             (_value <= high) )
        {
            return true;
        }
    }

    return false;
}

// d_imap_internal_months
//   table: the English month abbreviations IMAP dates use, January first.
static const char* const d_imap_internal_months[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_months) == 12,
              "one abbreviation per month");

// d_imap_internal_unix_min, d_imap_internal_unix_max
//   constants: the Unix times of 0000-01-01T00:00:00 and 9999-12-31T23:59:59,
// the span a four-digit year can express.
static const int64_t d_imap_internal_unix_min = INT64_C(-62167219200);
static const int64_t d_imap_internal_unix_max = INT64_C(253402300799);

/*
d_imap_internal_month_length
  Days in a month of the proleptic Gregorian calendar; `_month` is 1..12.
*/
D_STATIC int32_t
d_imap_internal_month_length(
    int32_t _year,
    int32_t _month
)
{
    static const int32_t lengths[12] =
    {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    bool leap = ( ((_year % 4) == 0) &&
                  ( ((_year % 100) != 0) ||
                    ((_year % 400) == 0) ) );

    if ( (_month == 2) &&
         (leap) )
    {
        return 29;
    }

    return lengths[_month - 1];
}

/*
d_imap_internal_date_is_valid
  A four-digit year, a month, and a day that month has. The month is checked
before it indexes the length table.
*/
D_STATIC bool
d_imap_internal_date_is_valid(
    struct d_imap_datetime _datetime
)
{
    return ( (_datetime.year >= 0) &&
             (_datetime.year <= 9999) &&
             (_datetime.month >= 1) &&
             (_datetime.month <= 12) &&
             (_datetime.day >= 1) &&
             (_datetime.day <= d_imap_internal_month_length(_datetime.year,
                                                            _datetime.month)) );
}

/*
d_imap_internal_datetime_is_valid
  A valid date, a time of day admitting a leap second, and a zone within a
day of UTC.
*/
D_STATIC bool
d_imap_internal_datetime_is_valid(
    struct d_imap_datetime _datetime
)
{
    return ( (d_imap_internal_date_is_valid(_datetime)) &&
             (_datetime.hour >= 0) &&
             (_datetime.hour <= 23) &&
             (_datetime.minute >= 0) &&
             (_datetime.minute <= 59) &&
             (_datetime.second >= 0) &&
             (_datetime.second <= 60) &&
             (_datetime.zone >= -1439) &&
             (_datetime.zone <= 1439) );
}

/*
d_imap_internal_days_from_civil
  Days from 1970-01-01 to a proleptic Gregorian date, by Howard Hinnant's
era decomposition: counting the year from March puts the leap day last, so
every 400-year era has the same shape and no table of month offsets is
needed. Exact for every year this module admits.
*/
D_STATIC int64_t
d_imap_internal_days_from_civil(
    int64_t _year,
    int64_t _month,
    int64_t _day
)
{
    int64_t year  = (_month <= 2) ? (_year - 1) : _year;
    int64_t era   = ((year >= 0) ? year : (year - 399)) / 400;
    int64_t yoe   = year - (era * 400);
    int64_t march = (_month > 2) ? (_month - 3) : (_month + 9);
    int64_t doy   = (((153 * march) + 2) / 5) + _day - 1;
    int64_t doe   = (yoe * 365) + (yoe / 4) - (yoe / 100) + doy;

    return (era * 146097) + doe - 719468;
}

/*
d_imap_internal_civil_from_days
  The inverse, by the same decomposition. Writes the date fields only.
*/
D_STATIC void
d_imap_internal_civil_from_days(
    int64_t                 _days,
    struct d_imap_datetime* _datetime
)
{
    int64_t z     = _days + 719468;
    int64_t era   = ((z >= 0) ? z : (z - 146096)) / 146097;
    int64_t doe   = z - (era * 146097);
    int64_t yoe   = (doe - (doe / 1460) + (doe / 36524) - (doe / 146096)) / 365;
    int64_t doy   = doe - ((365 * yoe) + (yoe / 4) - (yoe / 100));
    int64_t march = ((5 * doy) + 2) / 153;
    int64_t month = (march < 10) ? (march + 3) : (march - 9);

    _datetime->year  = (int32_t)(yoe + (era * 400) + ((month <= 2) ? 1 : 0));
    _datetime->month = (int32_t)month;
    _datetime->day   = (int32_t)(doy - (((153 * march) + 2) / 5) + 1);

    return;
}

/*
d_imap_datetime_from_unix
  Shifts to local time, then splits days from seconds with floor division,
which C's truncating `/` only provides for non-negative values. The UTC value
is bounded first, loosely, so that adding the zone cannot overflow; the local
value is then bounded exactly.
*/
enum d_imap_error
d_imap_datetime_from_unix(
    int64_t                 _seconds,
    int32_t                 _zone,
    struct d_imap_datetime* _datetime
)
{
    struct d_imap_datetime result;
    int64_t                local;
    int64_t                days;
    int64_t                second_of_day;

    if (!_datetime)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_zone < -1439) ||
         (_zone > 1439) ||
         (_seconds < d_imap_internal_unix_min - 86400) ||
         (_seconds > d_imap_internal_unix_max + 86400) )
    {
        return D_IMAP_ERROR_RANGE;
    }

    local = _seconds + ((int64_t)_zone * 60);

    if ( (local < d_imap_internal_unix_min) ||
         (local > d_imap_internal_unix_max) )
    {
        return D_IMAP_ERROR_RANGE;
    }

    days          = local / 86400;
    second_of_day = local % 86400;

    if (second_of_day < 0)
    {
        --days;
        second_of_day += 86400;
    }

    d_imap_internal_civil_from_days(days,
                                    &result);
    result.hour   = (int32_t)(second_of_day / 3600);
    result.minute = (int32_t)((second_of_day / 60) % 60);
    result.second = (int32_t)(second_of_day % 60);
    result.zone   = _zone;
    *_datetime    = result;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_datetime_to_unix
  Days since the epoch, plus the time of day, less the zone. A leap second is
simply one more second, landing on the next minute's first.
*/
enum d_imap_error
d_imap_datetime_to_unix(
    struct d_imap_datetime _datetime,
    int64_t*               _seconds
)
{
    int64_t days;

    if (!_seconds)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (!d_imap_internal_datetime_is_valid(_datetime))
    {
        return D_IMAP_ERROR_RANGE;
    }

    days = d_imap_internal_days_from_civil(_datetime.year,
                                           _datetime.month,
                                           _datetime.day);

    *_seconds = (days * 86400) +
                ((int64_t)_datetime.hour * 3600) +
                ((int64_t)_datetime.minute * 60) +
                _datetime.second -
                ((int64_t)_datetime.zone * 60);

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_append_date
  Appends "d-Mon-yyyy" to a line under assembly: the day space-padded to two
characters when `_padded` (date-time's date-day-fixed), bare otherwise (a
SEARCH date). The date has been validated, so the month indexes safely.
*/
D_STATIC void
d_imap_internal_append_date(
    char*                  _line,
    size_t*                _used,
    struct d_imap_datetime _datetime,
    bool                   _padded
)
{
    if ( (_padded) &&
         (_datetime.day < 10) )
    {
        _line[(*_used)++] = ' ';
    }

    d_imap_internal_append_decimal(_line,
                                   _used,
                                   (uint64_t)_datetime.day,
                                   1);
    _line[(*_used)++] = '-';
    memcpy(_line + *_used,
           d_imap_internal_months[_datetime.month - 1],
           3);
    *_used           += 3;
    _line[(*_used)++] = '-';
    d_imap_internal_append_decimal(_line,
                                   _used,
                                   (uint64_t)_datetime.year,
                                   4);

    return;
}

/*
d_imap_internal_append_time
  Appends "hh:mm:ss +zzzz", the zone as a sign and four digits of hours and
minutes; UTC is written "+0000".
*/
D_STATIC void
d_imap_internal_append_time(
    char*                  _line,
    size_t*                _used,
    struct d_imap_datetime _datetime
)
{
    int32_t zone = (_datetime.zone < 0) ? -_datetime.zone : _datetime.zone;
    int32_t hhmm = ((zone / 60) * 100) + (zone % 60);

    d_imap_internal_append_decimal(_line,
                                   _used,
                                   (uint64_t)_datetime.hour,
                                   2);
    _line[(*_used)++] = ':';
    d_imap_internal_append_decimal(_line,
                                   _used,
                                   (uint64_t)_datetime.minute,
                                   2);
    _line[(*_used)++] = ':';
    d_imap_internal_append_decimal(_line,
                                   _used,
                                   (uint64_t)_datetime.second,
                                   2);
    _line[(*_used)++] = ' ';
    _line[(*_used)++] = (_datetime.zone < 0) ? '-' : '+';
    d_imap_internal_append_decimal(_line,
                                   _used,
                                   (uint64_t)hhmm,
                                   4);

    return;
}

/*
d_imap_date_write
  The date assembled in one buffer and emitted in one call: at most two day
digits, three dashes and letters, and four year digits.
*/
enum d_imap_error
d_imap_date_write(
    struct d_pack_sink     _sink,
    struct d_imap_datetime _datetime
)
{
    char   line[16];
    size_t used = 0;

    if (!_sink.write)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (!d_imap_internal_date_is_valid(_datetime))
    {
        return D_IMAP_ERROR_RANGE;
    }

    d_imap_internal_append_date(line,
                                &used,
                                _datetime,
                                false);

    return d_imap_internal_emit(_sink,
                                line,
                                used);
}

/*
d_imap_datetime_write
  The quoted date-time assembled in one buffer: 28 characters with quotes.
*/
enum d_imap_error
d_imap_datetime_write(
    struct d_pack_sink     _sink,
    struct d_imap_datetime _datetime
)
{
    char   line[32];
    size_t used = 0;

    if (!_sink.write)
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (!d_imap_internal_datetime_is_valid(_datetime))
    {
        return D_IMAP_ERROR_RANGE;
    }

    line[used++] = '"';
    d_imap_internal_append_date(line,
                                &used,
                                _datetime,
                                true);
    line[used++] = ' ';
    d_imap_internal_append_time(line,
                                &used,
                                _datetime);
    line[used++] = '"';

    return d_imap_internal_emit(_sink,
                                line,
                                used);
}

/*
d_imap_internal_cursor
  A read position in text, for the character-level date grammar.
*/
struct d_imap_internal_cursor
{
    struct d_pack_text text;
    size_t             position;
};

/*
d_imap_internal_cursor_peek
  The next octet, or -1 at the end.
*/
D_STATIC int
d_imap_internal_cursor_peek(
    const struct d_imap_internal_cursor* _cursor
)
{
    if (_cursor->position >= _cursor->text.length)
    {
        return -1;
    }

    return (unsigned char)_cursor->text.data[_cursor->position];
}

/*
d_imap_internal_cursor_take
  Consumes the next octet if it is `_c`.
*/
D_STATIC bool
d_imap_internal_cursor_take(
    struct d_imap_internal_cursor* _cursor,
    char                           _c
)
{
    if (d_imap_internal_cursor_peek(_cursor) != (unsigned char)_c)
    {
        return false;
    }

    ++_cursor->position;

    return true;
}

/*
d_imap_internal_cursor_number
  Consumes between `_min` and `_max` digits (at most four, so the value fits)
and reports whether at least `_min` were there.
*/
D_STATIC bool
d_imap_internal_cursor_number(
    struct d_imap_internal_cursor* _cursor,
    size_t                         _min,
    size_t                         _max,
    int32_t*                       _value
)
{
    int32_t value = 0;
    size_t  count = 0;
    int     c     = d_imap_internal_cursor_peek(_cursor);

    while ( (count < _max) &&
            (c >= '0') &&
            (c <= '9') )
    {
        value = (value * 10) + (c - '0');
        ++_cursor->position;
        ++count;
        c = d_imap_internal_cursor_peek(_cursor);
    }

    *_value = value;

    return (count >= _min);
}

/*
d_imap_internal_cursor_month
  Consumes a three-letter month abbreviation in any case.
*/
D_STATIC bool
d_imap_internal_cursor_month(
    struct d_imap_internal_cursor* _cursor,
    int32_t*                       _month
)
{
    struct d_pack_text word;

    if (_cursor->text.length - _cursor->position < 3)
    {
        return false;
    }

    word = d_imap_internal_span(_cursor->text.data,
                                _cursor->position,
                                _cursor->position + 3);

    for (int32_t i = 0; i < 12; ++i)
    {
        if (d_imap_internal_equals_nocase(word,
                                          d_imap_internal_months[i]))
        {
            *_month            = i + 1;
            _cursor->position += 3;

            return true;
        }
    }

    return false;
}

/*
d_imap_internal_cursor_date
  "d-Mon-yyyy" with a one- or two-digit day; with `_padded`, one leading
space may pad a one-digit day, as date-time writes it.
*/
D_STATIC bool
d_imap_internal_cursor_date(
    struct d_imap_internal_cursor* _cursor,
    struct d_imap_datetime*        _datetime,
    bool                           _padded
)
{
    if (_padded)
    {
        (void)d_imap_internal_cursor_take(_cursor,
                                          ' ');
    }

    return ( (d_imap_internal_cursor_number(_cursor,
                                            1,
                                            2,
                                            &_datetime->day)) &&
             (d_imap_internal_cursor_take(_cursor,
                                          '-')) &&
             (d_imap_internal_cursor_month(_cursor,
                                           &_datetime->month)) &&
             (d_imap_internal_cursor_take(_cursor,
                                          '-')) &&
             (d_imap_internal_cursor_number(_cursor,
                                            4,
                                            4,
                                            &_datetime->year)) );
}

/*
d_imap_internal_cursor_time
  "hh:mm:ss", two digits each.
*/
D_STATIC bool
d_imap_internal_cursor_time(
    struct d_imap_internal_cursor* _cursor,
    struct d_imap_datetime*        _datetime
)
{
    return ( (d_imap_internal_cursor_number(_cursor,
                                            2,
                                            2,
                                            &_datetime->hour)) &&
             (d_imap_internal_cursor_take(_cursor,
                                          ':')) &&
             (d_imap_internal_cursor_number(_cursor,
                                            2,
                                            2,
                                            &_datetime->minute)) &&
             (d_imap_internal_cursor_take(_cursor,
                                          ':')) &&
             (d_imap_internal_cursor_number(_cursor,
                                            2,
                                            2,
                                            &_datetime->second)) );
}

/*
d_imap_internal_cursor_zone
  "+hhmm" or "-hhmm" into minutes east of UTC. Syntax failures are SYNTAX;
minutes of 60 or more are RANGE, since the text was well formed.
*/
D_STATIC enum d_imap_error
d_imap_internal_cursor_zone(
    struct d_imap_internal_cursor* _cursor,
    int32_t*                       _zone
)
{
    int32_t sign = 1;
    int32_t hhmm = 0;

    if (d_imap_internal_cursor_take(_cursor,
                                    '-'))
    {
        sign = -1;
    }
    else if (!d_imap_internal_cursor_take(_cursor,
                                          '+'))
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    if (!d_imap_internal_cursor_number(_cursor,
                                       4,
                                       4,
                                       &hhmm))
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    if ((hhmm % 100) >= 60)
    {
        return D_IMAP_ERROR_RANGE;
    }

    *_zone = sign * (((hhmm / 100) * 60) + (hhmm % 100));

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_date_parse
  An optional opening quote obliges a closing one, and nothing may follow.
*/
enum d_imap_error
d_imap_date_parse(
    struct d_pack_text      _text,
    struct d_imap_datetime* _datetime
)
{
    struct d_imap_internal_cursor cursor;
    struct d_imap_datetime        result = { 0, 0, 0, 0, 0, 0, 0 };
    bool                          quoted;

    if ( (!_datetime) ||
         (!d_imap_internal_text_is_valid(_text)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    cursor.text     = _text;
    cursor.position = 0;
    quoted          = d_imap_internal_cursor_take(&cursor,
                                                  '"');

    if ( (!d_imap_internal_cursor_date(&cursor,
                                       &result,
                                       false)) ||
         ( (quoted) &&
           (!d_imap_internal_cursor_take(&cursor,
                                         '"')) ) ||
         (cursor.position != _text.length) )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    if (!d_imap_internal_date_is_valid(result))
    {
        return D_IMAP_ERROR_RANGE;
    }

    *_datetime = result;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_datetime_parse
  Date, space, time, space, zone; quoted or not. A zone whose minutes reach
60 is RANGE rather than SYNTAX, since its shape is right.
*/
enum d_imap_error
d_imap_datetime_parse(
    struct d_pack_text      _text,
    struct d_imap_datetime* _datetime
)
{
    struct d_imap_internal_cursor cursor;
    struct d_imap_datetime        result = { 0, 0, 0, 0, 0, 0, 0 };
    enum d_imap_error             error  = D_IMAP_ERROR_SYNTAX;
    bool                          quoted;

    if ( (!_datetime) ||
         (!d_imap_internal_text_is_valid(_text)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    cursor.text     = _text;
    cursor.position = 0;
    quoted          = d_imap_internal_cursor_take(&cursor,
                                                  '"');

    if ( (d_imap_internal_cursor_date(&cursor,
                                      &result,
                                      true)) &&
         (d_imap_internal_cursor_take(&cursor,
                                      ' ')) &&
         (d_imap_internal_cursor_time(&cursor,
                                      &result)) &&
         (d_imap_internal_cursor_take(&cursor,
                                      ' ')) )
    {
        error = d_imap_internal_cursor_zone(&cursor,
                                            &result.zone);
    }

    if ( (error == D_IMAP_ERROR_NONE) &&
         ( ( (quoted) &&
             (!d_imap_internal_cursor_take(&cursor,
                                           '"')) ) ||
           (cursor.position != _text.length) ) )
    {
        error = D_IMAP_ERROR_SYNTAX;
    }

    if ( (error == D_IMAP_ERROR_NONE) &&
         (!d_imap_internal_datetime_is_valid(result)) )
    {
        error = D_IMAP_ERROR_RANGE;
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        *_datetime = result;
    }

    return error;
}

// d_imap_internal_condition_names
//   table: the condition keywords, indexed by d_imap_condition; the empty
// first entry stands for NONE.
static const char* const d_imap_internal_condition_names[] =
{
    "", "OK", "NO", "BAD", "PREAUTH", "BYE"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_condition_names) ==
                  D_IMAP_CONDITION_COUNT,
              "one keyword per condition");

// d_imap_internal_code_names
//   table: the response-code atoms, indexed by d_imap_code; NONE and OTHER
// have no single wire name and are empty.
static const char* const d_imap_internal_code_names[] =
{
    "", "",
    "ALERT", "BADCHARSET", "CAPABILITY", "PARSE", "PERMANENTFLAGS",
    "READ-ONLY", "READ-WRITE", "TRYCREATE", "UIDNEXT", "UIDVALIDITY",
    "UNSEEN", "HASCHILDREN",
    "APPENDUID", "COPYUID", "UIDNOTSTICKY",
    "UNAVAILABLE", "AUTHENTICATIONFAILED", "AUTHORIZATIONFAILED", "EXPIRED",
    "PRIVACYREQUIRED", "CONTACTADMIN", "NOPERM", "INUSE", "EXPUNGEISSUED",
    "CORRUPTION", "SERVERBUG", "CLIENTBUG", "CANNOT", "LIMIT", "OVERQUOTA",
    "ALREADYEXISTS", "NONEXISTENT",
    "HIGHESTMODSEQ", "NOMODSEQ", "MODIFIED", "CLOSED",
    "UNKNOWN-CTE", "BADURL", "TOOBIG", "COMPRESSIONACTIVE", "NOTSAVED",
    "USEATTR", "MAILBOXID"
};

static_assert(D_ARRAY_STATIC_SIZE(d_imap_internal_code_names) ==
                  D_IMAP_CODE_COUNT,
              "one atom per response code");

/*
d_imap_condition_name
  A table lookup; NONE's entry is already empty.
*/
const char*
d_imap_condition_name(
    enum d_imap_condition _condition
)
{
    return d_imap_internal_table_name(d_imap_internal_condition_names,
                                      D_IMAP_CONDITION_COUNT,
                                      (int)_condition,
                                      "");
}

/*
d_imap_condition_from_name
  A table search that skips the empty NONE entry, so an empty or unknown word
falls back to NONE.
*/
enum d_imap_condition
d_imap_condition_from_name(
    struct d_pack_text _name
)
{
    size_t index;

    if ( (!d_imap_internal_text_is_valid(_name)) ||
         (!d_imap_internal_table_find(d_imap_internal_condition_names,
                                      D_IMAP_CONDITION_COUNT,
                                      _name,
                                      &index)) )
    {
        return D_IMAP_CONDITION_NONE;
    }

    return (enum d_imap_condition)index;
}

/*
d_imap_code_name
  A table lookup.
*/
const char*
d_imap_code_name(
    enum d_imap_code _code
)
{
    return d_imap_internal_table_name(d_imap_internal_code_names,
                                      D_IMAP_CODE_COUNT,
                                      (int)_code,
                                      "");
}

/*
d_imap_code_from_name
  Empty means there was no code; anything the table lacks is still a code,
just not one this vocabulary names.
*/
enum d_imap_code
d_imap_code_from_name(
    struct d_pack_text _name
)
{
    size_t index;

    if ( (!d_imap_internal_text_is_valid(_name)) ||
         (_name.length == 0) )
    {
        return D_IMAP_CODE_NONE;
    }

    if (d_imap_internal_table_find(d_imap_internal_code_names,
                                   D_IMAP_CODE_COUNT,
                                   _name,
                                   &index))
    {
        return (enum d_imap_code)index;
    }

    return D_IMAP_CODE_OTHER;
}

/*
d_imap_error_from_condition
  The three refusals map to their own codes; OK and PREAUTH are successes,
and NONE is no status at all.
*/
enum d_imap_error
d_imap_error_from_condition(
    enum d_imap_condition _condition
)
{
    switch (_condition)
    {
        case D_IMAP_CONDITION_NO:  { return D_IMAP_ERROR_NO; }
        case D_IMAP_CONDITION_BAD: { return D_IMAP_ERROR_BAD; }
        case D_IMAP_CONDITION_BYE: { return D_IMAP_ERROR_BYE; }
        default:                   { break; }
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_scan
  The end of the run of characters `_accept` admits, starting at `_start` and
stopping at `_end`. Keywords, code names, and tags are all such runs.
*/
D_STATIC size_t
d_imap_internal_scan(
    struct d_pack_text         _text,
    size_t                     _start,
    size_t                     _end,
    d_imap_internal_octet_test _accept
)
{
    size_t i = _start;

    while ( (i < _end) &&
            (_accept((unsigned char)_text.data[i])) )
    {
        ++i;
    }

    return i;
}

/*
d_imap_internal_line_end
  The offset of the first CR or LF, or the length. A status response's text
ends there; untagged data does not, since literals follow line breaks.
*/
D_STATIC size_t
d_imap_internal_line_end(
    struct d_pack_text _line
)
{
    for (size_t i = 0; i < _line.length; ++i)
    {
        if ( (_line.data[i] == '\r') ||
             (_line.data[i] == '\n') )
        {
            return i;
        }
    }

    return _line.length;
}

/*
d_imap_internal_rest
  Everything from `_start` to the end of the input, less one final line
break, CRLF or bare LF. A `_start` past that end yields an empty view.
*/
D_STATIC struct d_pack_text
d_imap_internal_rest(
    struct d_pack_text _text,
    size_t             _start
)
{
    size_t end = _text.length;

    if ( (end > _start) &&
         (_text.data[end - 1] == '\n') )
    {
        --end;

        if ( (end > _start) &&
             (_text.data[end - 1] == '\r') )
        {
            --end;
        }
    }

    return d_imap_internal_span(_text.data,
                                (_start < end) ? _start : end,
                                end);
}

/*
d_imap_internal_parse_code
  "[" atom [SP argument] "]" at `*_position`, which is on the "[". The
argument runs to the first "]": the grammar keeps "]" out of every code's
argument, so no bracket counting is needed.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_code(
    struct d_pack_text      _line,
    size_t*                 _position,
    size_t                  _end,
    struct d_imap_response* _response
)
{
    size_t start = *_position + 1;
    size_t i     = d_imap_internal_scan(_line,
                                        start,
                                        _end,
                                        d_imap_internal_is_atom_char);

    if (i == start)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    _response->code_name = d_imap_internal_span(_line.data,
                                                start,
                                                i);
    _response->code      =
        (int32_t)d_imap_code_from_name(_response->code_name);

    // an optional argument, up to the closing bracket
    if ( (i < _end) &&
         (_line.data[i] == ' ') )
    {
        start = ++i;

        while ( (i < _end) &&
                (_line.data[i] != ']') )
        {
            ++i;
        }

        _response->code_argument = d_imap_internal_span(_line.data,
                                                        start,
                                                        i);
    }

    if ( (i >= _end) ||
         (_line.data[i] != ']') )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    *_position = i + 1;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_parse_resp_text
  resp-text after a condition or a "+": an optional space, an optional
bracketed code, another optional space, and text to the end of the line.
Continuation text is resp-text or base64, and base64 never begins with "[",
so one parser serves both.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_resp_text(
    struct d_pack_text      _line,
    size_t                  _start,
    size_t                  _end,
    struct d_imap_response* _response
)
{
    size_t            i     = _start;
    enum d_imap_error error = D_IMAP_ERROR_NONE;

    if ( (i < _end) &&
         (_line.data[i] == ' ') )
    {
        ++i;
    }

    if ( (i < _end) &&
         (_line.data[i] == '[') )
    {
        error = d_imap_internal_parse_code(_line,
                                           &i,
                                           _end,
                                           _response);

        if ( (i < _end) &&
             (_line.data[i] == ' ') )
        {
            ++i;
        }
    }

    _response->text = d_imap_internal_span(_line.data,
                                           i,
                                           _end);

    return error;
}

/*
d_imap_internal_data_after
  The data following a keyword: its one separating space skipped, then
everything to the end of the input, literals included, less a final line
break.
*/
D_STATIC struct d_pack_text
d_imap_internal_data_after(
    struct d_pack_text _line,
    size_t             _keyword_end
)
{
    size_t start = _keyword_end;

    if ( (start < _line.length) &&
         (_line.data[start] == ' ') )
    {
        ++start;
    }

    return d_imap_internal_rest(_line,
                                start);
}

/*
d_imap_internal_parse_number_prefix
  The "n " of "* n KEYWORD", if present at `*_position`: a number that fits
32 bits, then exactly one space. Without digits there is nothing to read.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_number_prefix(
    struct d_pack_text      _line,
    size_t                  _end,
    size_t*                 _position,
    struct d_imap_response* _response
)
{
    size_t   start = *_position;
    size_t   i     = d_imap_internal_scan(_line,
                                          start,
                                          _end,
                                          d_imap_internal_is_digit);
    uint64_t number;

    if (i == start)
    {
        return D_IMAP_ERROR_NONE;
    }

    if (d_imap_internal_parse_decimal(d_imap_internal_span(_line.data,
                                                           start,
                                                           i),
                                      D_IMAP_NUMBER_MAX,
                                      &number) != D_IMAP_ERROR_NONE)
    {
        return D_IMAP_ERROR_RANGE;
    }

    if ( (i >= _end) ||
         (_line.data[i] != ' ') )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    _response->has_number = 1;
    _response->number     = (uint32_t)number;
    *_position            = i + 1;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_parse_untagged
  After "* ": an optional number, then a keyword. Unnumbered, the keyword may
be a condition, making this a status response; otherwise it names the data
that follows.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_untagged(
    struct d_pack_text      _line,
    size_t                  _end,
    struct d_imap_response* _response
)
{
    size_t                i = 2;
    size_t                keyword_end;
    enum d_imap_condition condition;
    enum d_imap_error     error;

    _response->kind = D_IMAP_RESPONSE_UNTAGGED;

    if ( (_end < 2) ||
         (_line.data[1] != ' ') )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    error = d_imap_internal_parse_number_prefix(_line,
                                                _end,
                                                &i,
                                                _response);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    keyword_end = d_imap_internal_scan(_line,
                                       i,
                                       _end,
                                       d_imap_internal_is_atom_char);

    if (keyword_end == i)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    _response->keyword = d_imap_internal_span(_line.data,
                                              i,
                                              keyword_end);
    condition          = (_response->has_number)
                             ? D_IMAP_CONDITION_NONE
                             : d_imap_condition_from_name(_response->keyword);

    if (condition != D_IMAP_CONDITION_NONE)
    {
        _response->condition = (int32_t)condition;

        return d_imap_internal_parse_resp_text(_line,
                                               keyword_end,
                                               _end,
                                               _response);
    }

    _response->rest = d_imap_internal_data_after(_line,
                                                 keyword_end);

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_parse_tagged
  tag SP condition resp-text. A tagged response completes a command, so only
OK, NO, and BAD may follow the tag.
*/
D_STATIC enum d_imap_error
d_imap_internal_parse_tagged(
    struct d_pack_text      _line,
    size_t                  _end,
    struct d_imap_response* _response
)
{
    size_t                tag_end = d_imap_internal_scan(
                                        _line,
                                        0,
                                        _end,
                                        d_imap_internal_is_tag_char);
    size_t                keyword_end;
    enum d_imap_condition condition;

    _response->kind = D_IMAP_RESPONSE_TAGGED;

    if ( (tag_end == 0) ||
         (tag_end >= _end) ||
         (_line.data[tag_end] != ' ') )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    keyword_end        = d_imap_internal_scan(_line,
                                              tag_end + 1,
                                              _end,
                                              d_imap_internal_is_atom_char);
    _response->tag     = d_imap_internal_span(_line.data,
                                              0,
                                              tag_end);
    _response->keyword = d_imap_internal_span(_line.data,
                                              tag_end + 1,
                                              keyword_end);
    condition          = d_imap_condition_from_name(_response->keyword);

    if ( (condition != D_IMAP_CONDITION_OK) &&
         (condition != D_IMAP_CONDITION_NO) &&
         (condition != D_IMAP_CONDITION_BAD) )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    _response->condition = (int32_t)condition;

    return d_imap_internal_parse_resp_text(_line,
                                           keyword_end,
                                           _end,
                                           _response);
}

/*
d_imap_response_parse
  Dispatches on the first octet: "+" continuation, "*" untagged, anything else
a tag. The parse fills a local copy, so the caller's struct is written only
on success.
*/
enum d_imap_error
d_imap_response_parse(
    struct d_pack_text      _line,
    struct d_imap_response* _response
)
{
    static const struct d_imap_response empty;
    struct d_imap_response              result = empty;
    size_t                              end;
    enum d_imap_error                   error;

    if ( (!_response) ||
         (!d_imap_internal_text_is_valid(_line)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    end = d_imap_internal_line_end(_line);

    if (end == 0)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    if (_line.data[0] == '+')
    {
        result.kind = D_IMAP_RESPONSE_CONTINUATION;
        error       = d_imap_internal_parse_resp_text(_line,
                                                      1,
                                                      end,
                                                      &result);
    }
    else if (_line.data[0] == '*')
    {
        error = d_imap_internal_parse_untagged(_line,
                                               end,
                                               &result);
    }
    else
    {
        error = d_imap_internal_parse_tagged(_line,
                                             end,
                                             &result);
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        *_response = result;
    }

    return error;
}

/*
d_imap_lexer_init
  A NULL pointer with a length is treated as empty input rather than trusted,
so later reads can never follow it.
*/
void
d_imap_lexer_init(
    struct d_imap_lexer* _lexer,
    struct d_pack_text   _input
)
{
    if (_lexer)
    {
        _lexer->data     = _input.data;
        _lexer->length   = (_input.data != NULL) ? _input.length : 0;
        _lexer->position = 0;
    }

    return;
}

/*
d_imap_internal_input
  The lexer's whole input as a span, for the shared scanning helpers.
*/
D_STATIC struct d_pack_text
d_imap_internal_input(
    const struct d_imap_lexer* _lexer
)
{
    struct d_pack_text input;

    input.data   = _lexer->data;
    input.length = _lexer->length;

    return input;
}

/*
d_imap_internal_lex_finish
  Fills in a completed token and moves the lexer past it; every successful
sub-lexer ends here, and no failing one does.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_finish(
    struct d_imap_lexer*   _lexer,
    struct d_imap_token*   _token,
    enum d_imap_token_kind _kind,
    size_t                 _start,
    size_t                 _end
)
{
    _token->kind     = (int32_t)_kind;
    _token->text     = d_imap_internal_span(_lexer->data,
                                            _start,
                                            _end);
    _lexer->position = _end;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_lex_crlf
  CR must be followed by LF; a bare LF is accepted as a line break alone.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_crlf(
    struct d_imap_lexer* _lexer,
    size_t               _start,
    struct d_imap_token* _token
)
{
    size_t end = _start + 1;

    if (_lexer->data[_start] == '\r')
    {
        if (end >= _lexer->length)
        {
            return D_IMAP_ERROR_INCOMPLETE;
        }

        if (_lexer->data[end] != '\n')
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        ++end;
    }

    return d_imap_internal_lex_finish(_lexer,
                                      _token,
                                      D_IMAP_TOKEN_CRLF,
                                      _start,
                                      end);
}

/*
d_imap_internal_lex_quoted
  A quoted string runs to the first unescaped '"'. The lexer only delimits,
so any character may follow a backslash here, except a line break, which no
quoted string can hold; d_imap_quoted_unescape() judges the escapes. Running
out of input is INCOMPLETE, since the rest of the line may yet arrive.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_quoted(
    struct d_imap_lexer* _lexer,
    size_t               _start,
    struct d_imap_token* _token
)
{
    size_t i = _start + 1;

    while (i < _lexer->length)
    {
        char c = _lexer->data[i];

        if (c == '"')
        {
            (void)d_imap_internal_lex_finish(_lexer,
                                             _token,
                                             D_IMAP_TOKEN_QUOTED,
                                             _start + 1,
                                             i);
            _lexer->position = i + 1;

            return D_IMAP_ERROR_NONE;
        }

        if (c == '\\')
        {
            _token->flags |= D_IMAP_TOKEN_ESCAPED;
            ++i;
            c = (i < _lexer->length) ? _lexer->data[i] : '\0';
        }

        if ( (c == '\r') ||
             (c == '\n') )
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        ++i;
    }

    return D_IMAP_ERROR_INCOMPLETE;
}

/*
d_imap_internal_lex_expect
  Consumes the octet `_c` at `*_position`: INCOMPLETE if the input ends
first, SYNTAX if another octet is there.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_expect(
    const struct d_imap_lexer* _lexer,
    size_t*                    _position,
    char                       _c
)
{
    if (*_position >= _lexer->length)
    {
        return D_IMAP_ERROR_INCOMPLETE;
    }

    if (_lexer->data[*_position] != _c)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    ++*_position;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_lex_literal_head
  From the "{" at `*_position`: the count, an optional "+", "}", and the line
break, leaving `*_position` on the first octet. The count is bounded by
number64 as it is read, and a missing count is SYNTAX.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_literal_head(
    const struct d_imap_lexer* _lexer,
    size_t*                    _position,
    uint64_t*                  _count,
    uint32_t*                  _flags
)
{
    struct d_pack_text input = d_imap_internal_input(_lexer);
    size_t             start = *_position + 1;
    size_t             i     = d_imap_internal_scan(input,
                                                    start,
                                                    _lexer->length,
                                                    d_imap_internal_is_digit);
    enum d_imap_error  error;

    if (i >= _lexer->length)
    {
        return D_IMAP_ERROR_INCOMPLETE;
    }

    error = d_imap_internal_parse_decimal(d_imap_internal_span(_lexer->data,
                                                               start,
                                                               i),
                                          D_IMAP_NUMBER64_MAX,
                                          _count);

    if ( (error == D_IMAP_ERROR_NONE) &&
         (_lexer->data[i] == '+') )
    {
        *_flags |= D_IMAP_TOKEN_NONSYNC;
        ++i;
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_lex_expect(_lexer,
                                           &i,
                                           '}');
    }

    // the line break's CR is optional; its LF is not
    if ( (error == D_IMAP_ERROR_NONE) &&
         (i < _lexer->length) &&
         (_lexer->data[i] == '\r') )
    {
        ++i;
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_lex_expect(_lexer,
                                           &i,
                                           '\n');
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        *_position = i;
    }

    return error;
}

/*
d_imap_internal_lex_literal
  A literal or, after "~", a literal8. When the octets are not all present
the token still carries the declared count and the octets so far, which is
how a streaming reader learns how much more to fetch; the position does not
move until the literal is whole.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_literal(
    struct d_imap_lexer* _lexer,
    size_t               _start,
    struct d_imap_token* _token
)
{
    size_t            body  = _start;
    uint64_t          count = 0;
    size_t            available;
    enum d_imap_error error;

    if (_lexer->data[_start] == '~')
    {
        _token->flags |= D_IMAP_TOKEN_BINARY;
        ++body;
    }

    error = d_imap_internal_lex_literal_head(_lexer,
                                             &body,
                                             &count,
                                             &_token->flags);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    available      = _lexer->length - body;
    _token->kind   = D_IMAP_TOKEN_LITERAL;
    _token->number = count;
    _token->text   = d_imap_internal_span(_lexer->data,
                                          body,
                                          body + ( (count < available)
                                                       ? (size_t)count
                                                       : available ));

    if (available < count)
    {
        return D_IMAP_ERROR_INCOMPLETE;
    }

    _lexer->position = body + (size_t)count;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_lex_flag
  "\*", or a backslash and an atom: a system flag or a mailbox attribute.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_flag(
    struct d_imap_lexer* _lexer,
    size_t               _start,
    struct d_imap_token* _token
)
{
    size_t end;

    if (_start + 1 >= _lexer->length)
    {
        return D_IMAP_ERROR_INCOMPLETE;
    }

    end = (_lexer->data[_start + 1] == '*')
              ? (_start + 2)
              : d_imap_internal_scan(d_imap_internal_input(_lexer),
                                     _start + 1,
                                     _lexer->length,
                                     d_imap_internal_is_atom_char);

    if (end == _start + 1)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    return d_imap_internal_lex_finish(_lexer,
                                      _token,
                                      D_IMAP_TOKEN_FLAG,
                                      _start,
                                      end);
}

/*
d_imap_internal_lex_atom_char
  What an atom may hold while lexing: brackets only in astring context, and
octets above 0x7F always, because some servers send UTF-8 unquoted.
*/
D_STATIC bool
d_imap_internal_lex_atom_char(
    unsigned char _c,
    uint32_t      _flags
)
{
    if ( (_c == '[') ||
         (_c == ']') )
    {
        return ((_flags & D_IMAP_LEX_ASTRING) != 0u);
    }

    return ( (_c >= 0x80) ||
             (d_imap_internal_is_atom_char(_c)) );
}

/*
d_imap_internal_lex_atom
  The longest run of atom characters, then classified: all digits and within
64 bits is a NUMBER, "NIL" in any case is NIL, and anything else an ATOM.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_atom(
    struct d_imap_lexer* _lexer,
    size_t               _start,
    uint32_t             _flags,
    struct d_imap_token* _token
)
{
    size_t                 end  = _start;
    enum d_imap_token_kind kind = D_IMAP_TOKEN_ATOM;
    struct d_pack_text     text;

    while ( (end < _lexer->length) &&
            (d_imap_internal_lex_atom_char((unsigned char)_lexer->data[end],
                                           _flags)) )
    {
        ++end;
    }

    if (end == _start)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    text = d_imap_internal_span(_lexer->data,
                                _start,
                                end);

    if (d_imap_internal_parse_decimal(text,
                                      UINT64_MAX,
                                      &_token->number) == D_IMAP_ERROR_NONE)
    {
        kind = D_IMAP_TOKEN_NUMBER;
    }
    else if (d_imap_internal_equals_nocase(text,
                                           "NIL"))
    {
        kind = D_IMAP_TOKEN_NIL;
    }

    return d_imap_internal_lex_finish(_lexer,
                                      _token,
                                      kind,
                                      _start,
                                      end);
}

/*
d_imap_internal_punctuation
  The token a single punctuation octet makes, or END for an octet that
starts something longer.
*/
D_STATIC enum d_imap_token_kind
d_imap_internal_punctuation(
    unsigned char _c,
    uint32_t      _flags
)
{
    // brackets are punctuation only outside astring context
    if ( (_c == '[') ||
         (_c == ']') )
    {
        if ((_flags & D_IMAP_LEX_ASTRING) != 0u)
        {
            return D_IMAP_TOKEN_END;
        }

        return (_c == '[') ? D_IMAP_TOKEN_LBRACKET : D_IMAP_TOKEN_RBRACKET;
    }

    switch (_c)
    {
        case '(': { return D_IMAP_TOKEN_LPAREN; }
        case ')': { return D_IMAP_TOKEN_RPAREN; }
        case '*': { return D_IMAP_TOKEN_STAR; }
        default:  { break; }
    }

    return D_IMAP_TOKEN_END;
}

/*
d_imap_internal_lex_dispatch
  Chooses the sub-lexer for a token that is not single punctuation, by its
first octet: a line break, a quoted string, a flag, a literal or literal8,
and failing all of those an atom.
*/
D_STATIC enum d_imap_error
d_imap_internal_lex_dispatch(
    struct d_imap_lexer* _lexer,
    size_t               _start,
    uint32_t             _flags,
    struct d_imap_token* _token
)
{
    unsigned char c = (unsigned char)_lexer->data[_start];

    if ( (c == '\r') ||
         (c == '\n') )
    {
        return d_imap_internal_lex_crlf(_lexer,
                                        _start,
                                        _token);
    }

    if (c == '"')
    {
        return d_imap_internal_lex_quoted(_lexer,
                                          _start,
                                          _token);
    }

    if (c == '\\')
    {
        return d_imap_internal_lex_flag(_lexer,
                                        _start,
                                        _token);
    }

    if ( (c == '{') ||
         ( (c == '~') &&
           (_start + 1 < _lexer->length) &&
           (_lexer->data[_start + 1] == '{') ) )
    {
        return d_imap_internal_lex_literal(_lexer,
                                           _start,
                                           _token);
    }

    return d_imap_internal_lex_atom(_lexer,
                                    _start,
                                    _flags,
                                    _token);
}

/*
d_imap_lexer_next
  Skips spaces, then reads the end of input, a punctuation octet, or whatever
d_imap_internal_lex_dispatch() finds. Every sub-lexer moves the position only
on success, so after INCOMPLETE the same lexer can simply be asked again once
more input is appended to its buffer.
*/
enum d_imap_error
d_imap_lexer_next(
    struct d_imap_lexer* _lexer,
    uint32_t             _flags,
    struct d_imap_token* _token
)
{
    size_t                 start;
    enum d_imap_token_kind kind;

    if ( (!_lexer) ||
         (!_token) ||
         ( (!_lexer->data) &&
           (_lexer->length != 0) ) ||
         (_lexer->position > _lexer->length) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    start = d_imap_internal_scan(d_imap_internal_input(_lexer),
                                 _lexer->position,
                                 _lexer->length,
                                 d_imap_internal_is_space);

    _token->number = 0;
    _token->flags  = 0;

    if (start >= _lexer->length)
    {
        return d_imap_internal_lex_finish(_lexer,
                                          _token,
                                          D_IMAP_TOKEN_END,
                                          start,
                                          start);
    }

    kind = d_imap_internal_punctuation((unsigned char)_lexer->data[start],
                                       _flags);

    if (kind != D_IMAP_TOKEN_END)
    {
        return d_imap_internal_lex_finish(_lexer,
                                          _token,
                                          kind,
                                          start,
                                          start + 1);
    }

    return d_imap_internal_lex_dispatch(_lexer,
                                        start,
                                        _flags,
                                        _token);
}

/*
d_imap_lexer_rest
  The unread input as a view.
*/
struct d_pack_text
d_imap_lexer_rest(
    const struct d_imap_lexer* _lexer
)
{
    struct d_pack_text rest = { NULL, 0 };

    if ( (_lexer) &&
         (_lexer->data) &&
         (_lexer->position <= _lexer->length) )
    {
        rest = d_imap_internal_span(_lexer->data,
                                    _lexer->position,
                                    _lexer->length);
    }

    return rest;
}

/*
d_imap_string_decode
  NIL writes nothing, so that an nstring and an empty string share a code
path; the token's kind still tells them apart. A quoted string is unescaped
only when the lexer saw a backslash in it.
*/
enum d_imap_error
d_imap_string_decode(
    struct d_pack_sink         _sink,
    const struct d_imap_token* _token
)
{
    if ( (!_sink.write) ||
         (!_token) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    if (_token->kind == D_IMAP_TOKEN_NIL)
    {
        return D_IMAP_ERROR_NONE;
    }

    if ( (_token->kind == D_IMAP_TOKEN_QUOTED) &&
         ((_token->flags & D_IMAP_TOKEN_ESCAPED) != 0u) )
    {
        return d_imap_quoted_unescape(_sink,
                                      _token->text);
    }

    if ( (_token->kind != D_IMAP_TOKEN_ATOM) &&
         (_token->kind != D_IMAP_TOKEN_NUMBER) &&
         (_token->kind != D_IMAP_TOKEN_QUOTED) &&
         (_token->kind != D_IMAP_TOKEN_LITERAL) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    return d_imap_internal_emit(_sink,
                                _token->text.data,
                                _token->text.length);
}

/*
d_imap_mailbox_decode
  Under UTF8=ACCEPT the name is validated and then written as any string
would be; the escapes a quoted form may hold are ASCII and cannot affect the
validation. Otherwise the modified UTF-7 decoder reads the token's raw text,
resolving escapes itself.
*/
enum d_imap_error
d_imap_mailbox_decode(
    struct d_pack_sink         _sink,
    const struct d_imap_token* _token,
    uint32_t                   _flags
)
{
    bool escaped;

    if ( (!_sink.write) ||
         (!_token) ||
         ( (_token->kind != D_IMAP_TOKEN_ATOM) &&
           (_token->kind != D_IMAP_TOKEN_NUMBER) &&
           (_token->kind != D_IMAP_TOKEN_QUOTED) &&
           (_token->kind != D_IMAP_TOKEN_LITERAL) ) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    escaped = ( (_token->kind == D_IMAP_TOKEN_QUOTED) &&
                ((_token->flags & D_IMAP_TOKEN_ESCAPED) != 0u) );

    if ((_flags & D_IMAP_STRING_UTF8) == 0u)
    {
        return d_imap_internal_mutf7_decode(_sink,
                                            _token->text,
                                            escaped);
    }

    if (!d_imap_internal_name_is_encodable(_token->text))
    {
        return D_IMAP_ERROR_ENCODING;
    }

    return d_imap_string_decode(_sink,
                                _token);
}

/*
d_imap_internal_list_delimiter
  NIL, for a flat namespace, or a quoted single character, which may itself
be an escaped '"' or '\'.
*/
D_STATIC enum d_imap_error
d_imap_internal_list_delimiter(
    struct d_imap_lexer* _lexer,
    int32_t*             _delimiter
)
{
    struct d_imap_token token;
    const char*         text;

    if (d_imap_lexer_next(_lexer,
                          0,
                          &token) != D_IMAP_ERROR_NONE)
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    if (token.kind == D_IMAP_TOKEN_NIL)
    {
        *_delimiter = -1;

        return D_IMAP_ERROR_NONE;
    }

    text = token.text.data;

    if ( (token.kind == D_IMAP_TOKEN_QUOTED) &&
         (token.text.length == 1) &&
         (text[0] != '\\') )
    {
        *_delimiter = (unsigned char)text[0];

        return D_IMAP_ERROR_NONE;
    }

    if ( (token.kind == D_IMAP_TOKEN_QUOTED) &&
         (token.text.length == 2) &&
         (text[0] == '\\') &&
         ( (text[1] == '"') ||
           (text[1] == '\\') ) )
    {
        *_delimiter = (unsigned char)text[1];

        return D_IMAP_ERROR_NONE;
    }

    return D_IMAP_ERROR_SYNTAX;
}

/*
d_imap_internal_list_name
  The name is an astring, so it is lexed in astring context, where
"[Gmail]/Sent" stays one atom. A literal name that has not fully arrived is
INCOMPLETE; every other failure is a malformed listing.
*/
D_STATIC enum d_imap_error
d_imap_internal_list_name(
    struct d_imap_lexer* _lexer,
    struct d_imap_token* _name
)
{
    enum d_imap_error error = d_imap_lexer_next(_lexer,
                                                D_IMAP_LEX_ASTRING,
                                                _name);

    if (error == D_IMAP_ERROR_INCOMPLETE)
    {
        return error;
    }

    if ( (error != D_IMAP_ERROR_NONE) ||
         ( (_name->kind != D_IMAP_TOKEN_ATOM) &&
           (_name->kind != D_IMAP_TOKEN_NUMBER) &&
           (_name->kind != D_IMAP_TOKEN_QUOTED) &&
           (_name->kind != D_IMAP_TOKEN_LITERAL) ) )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_list_parse
  Attributes, delimiter, name, in that order, into a local copy. Whatever
follows the name is LIST-EXTENDED data, kept as text for the caller.
*/
enum d_imap_error
d_imap_list_parse(
    struct d_pack_text        _data,
    struct d_imap_list_entry* _entry
)
{
    static const struct d_imap_list_entry empty;
    struct d_imap_list_entry              entry = empty;
    struct d_imap_lexer                   lexer;
    struct d_pack_text                    rest;
    enum d_imap_error                     error;

    if ( (!_entry) ||
         (!d_imap_internal_text_is_valid(_data)) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    d_imap_lexer_init(&lexer,
                      _data);
    error = d_imap_internal_lex_name_list(
                &lexer,
                d_imap_internal_mailbox_attribute_names,
                D_IMAP_MAILBOX_ATTRIBUTE_COUNT,
                &entry.attributes,
                &entry.unknown);

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_list_delimiter(&lexer,
                                               &entry.delimiter);
    }

    if (error == D_IMAP_ERROR_NONE)
    {
        error = d_imap_internal_list_name(&lexer,
                                          &entry.name);
    }

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    rest           = d_imap_lexer_rest(&lexer);
    entry.extended = d_imap_internal_rest(rest,
                                          d_imap_internal_scan(
                                              rest,
                                              0,
                                              rest.length,
                                              d_imap_internal_is_space));
    *_entry        = entry;

    return D_IMAP_ERROR_NONE;
}

/*
d_imap_internal_search_modseq
  The "(MODSEQ n)" CONDSTORE appends to SEARCH results, after its "(" has been
read; it must end the response.
*/
D_STATIC enum d_imap_error
d_imap_internal_search_modseq(
    struct d_imap_lexer* _lexer,
    uint64_t*            _modseq
)
{
    struct d_imap_token keyword;
    struct d_imap_token value;
    struct d_imap_token close;

    if ( (d_imap_lexer_next(_lexer,
                            0,
                            &keyword) != D_IMAP_ERROR_NONE) ||
         (keyword.kind != D_IMAP_TOKEN_ATOM) ||
         (!d_imap_internal_equals_nocase(keyword.text,
                                         "MODSEQ")) ||
         (d_imap_lexer_next(_lexer,
                            0,
                            &value) != D_IMAP_ERROR_NONE) ||
         (value.kind != D_IMAP_TOKEN_NUMBER) ||
         (d_imap_lexer_next(_lexer,
                            0,
                            &close) != D_IMAP_ERROR_NONE) ||
         (close.kind != D_IMAP_TOKEN_RPAREN) )
    {
        return D_IMAP_ERROR_SYNTAX;
    }

    if (value.number > D_IMAP_NUMBER64_MAX)
    {
        return D_IMAP_ERROR_RANGE;
    }

    *_modseq = value.number;

    return d_imap_internal_expect_line_end(_lexer);
}

/*
d_imap_internal_search_numbers
  The body of a SEARCH response: numbers until the line ends, or until a "("
opens the CONDSTORE suffix. Numbers past the capacity are still validated and
counted, so a measuring pass sizes the filling pass exactly.
*/
D_STATIC enum d_imap_error
d_imap_internal_search_numbers(
    struct d_imap_lexer* _lexer,
    uint32_t*            _ids,
    size_t               _capacity,
    size_t*              _count,
    uint64_t*            _modseq
)
{
    struct d_imap_token token;
    enum d_imap_error   error = D_IMAP_ERROR_NONE;

    while (error == D_IMAP_ERROR_NONE)
    {
        if (d_imap_lexer_next(_lexer,
                              0,
                              &token) != D_IMAP_ERROR_NONE)
        {
            return D_IMAP_ERROR_SYNTAX;
        }

        if ( (token.kind == D_IMAP_TOKEN_END) ||
             (token.kind == D_IMAP_TOKEN_CRLF) )
        {
            break;
        }

        if (token.kind == D_IMAP_TOKEN_LPAREN)
        {
            return d_imap_internal_search_modseq(_lexer,
                                                 _modseq);
        }

        if (token.kind != D_IMAP_TOKEN_NUMBER)
        {
            error = D_IMAP_ERROR_SYNTAX;
        }
        else if ( (token.number == 0u) ||
                  (token.number > D_IMAP_NUMBER_MAX) )
        {
            error = D_IMAP_ERROR_RANGE;
        }
        else if (*_count < _capacity)
        {
            _ids[*_count] = (uint32_t)token.number;
        }

        ++*_count;
    }

    return error;
}

/*
d_imap_search_parse
  The numbers and the optional MODSEQ are gathered into locals, and the
caller's outputs are written only once the whole response has parsed.
*/
enum d_imap_error
d_imap_search_parse(
    struct d_pack_text _data,
    uint32_t*          _ids,
    size_t             _capacity,
    size_t*            _count,
    uint64_t*          _modseq
)
{
    struct d_imap_lexer lexer;
    size_t              count  = 0;
    uint64_t            modseq = 0;
    enum d_imap_error   error;

    if ( (!_count) ||
         (!d_imap_internal_text_is_valid(_data)) ||
         ( (!_ids) &&
           (_capacity != 0) ) )
    {
        return D_IMAP_ERROR_INVALID_ARGUMENT;
    }

    d_imap_lexer_init(&lexer,
                      _data);
    error = d_imap_internal_search_numbers(&lexer,
                                           _ids,
                                           _capacity,
                                           &count,
                                           &modseq);

    if (error != D_IMAP_ERROR_NONE)
    {
        return error;
    }

    *_count = count;

    if (_modseq)
    {
        *_modseq = modseq;
    }

    return (count > _capacity) ? D_IMAP_ERROR_CAPACITY : D_IMAP_ERROR_NONE;
}
