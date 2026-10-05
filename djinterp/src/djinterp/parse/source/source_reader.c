/*******************************************************************************
* djinterp [parse]                                               source_reader.c
*
* Definitions for the non-inline declarations in source_reader.h.
*   Three rewritings happen here and nowhere else: a trigraph becomes the
* character it names, a backslash-newline disappears, and every line ending
* becomes a single newline.  A consumer above this file sees none of the
* three and so needs no cases for them.
*   Normalizing line endings in the reader rather than in the scanner is
* what keeps `\r\n` out of every literal and comment loop.  A carriage return
* is one logical newline two bytes wide; the physical offset stays exact
* because the width is returned rather than assumed.
*
*
* path:      /src/djinterp/parse/source/source_reader.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/djinterp/parse/source/source_reader.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memcmp, strlen
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"               // uint32_t,
                                                                  // uint8_t


//==============================================================================
// 1.  PHASE 1 AND 2
//==============================================================================
// Trigraph translation, line-ending normalization, and splice removal, applied
// in that order because a trigraph may spell the backslash of a splice.


/**
 * @brief Maps the third character of a trigraph to the character it denotes.
 *
 * @param[in] _third  the character following `??`.
 * @return the denoted character, or -1 when the pair does not begin a trigraph.
 */
static int
d_internal_trigraph(
    int _third
)
{
    switch (_third)
    {
        case '=':  return '#';
        case '(':  return '[';
        case '/':  return '\\';
        case ')':  return ']';
        case '\'': return '^';
        case '<':  return '{';
        case '!':  return '|';
        case '>':  return '}';
        case '-':  return '~';
        default:   return -1;
    }
}


/**
 * @brief Reads one character at a physical offset, translating a trigraph and
 *        folding a line ending to a single newline.
 *
 * Splices are not considered here.
 *
 * @param[in]  _source     the source to read from.
 * @param[in]  _options    D_READER_OPT_* bits.
 * @param[in]  _offset     the physical byte offset to read at.
 * @param[out] _out_width  receives the number of physical bytes consumed.
 * @return the character read, or D_SOURCE_END past the end.
 */
static int
d_internal_raw_at(
    const struct d_source* _source,
    unsigned               _options,
    size_t                 _offset,
    size_t*                _out_width
)
{
    *_out_width = 0;

    // nothing remains
    if (_offset >= _source->length)
    {
        return D_SOURCE_END;
    }

    const unsigned char byte = (unsigned char)_source->text[_offset];

    // a trigraph stands for one character and is three bytes wide
    if ( (byte == '?') &&
         ((_options & D_READER_OPT_TRIGRAPH) != 0u) &&
         ((_offset + 2u) < _source->length) &&
         (_source->text[_offset + 1u] == '?') )
    {
        const int denoted =
            d_internal_trigraph((unsigned char)_source->text[_offset + 2u]);

        // only a recognized third character forms a trigraph
        if (denoted >= 0)
        {
            *_out_width = 3u;
            return denoted;
        }
    }

    // every line ending reads as one newline
    if (byte == '\r')
    {
        const bool paired = (((_offset + 1u) < _source->length) &&
                             (_source->text[_offset + 1u] == '\n'));

        *_out_width = paired ? 2u : 1u;
        return '\n';
    }

    *_out_width = 1u;
    return (int)byte;
}


/**
 * @brief Reports whether a splice begins at an offset and, if so, where it
 *        ends.
 *
 * @note A splice may be spelled with a trigraph backslash, so the test reads
 *       through d_internal_raw_at rather than comparing a byte.
 *
 * @param[in]  _source    the source to examine.
 * @param[in]  _options   D_READER_OPT_* bits.
 * @param[in]  _offset    the physical offset to test.
 * @param[out] _out_next  receives the offset just past the splice.
 * @return `true` if a splice begins at the offset, `false` otherwise.
 */
static bool
d_internal_splice_at(
    const struct d_source* _source,
    unsigned               _options,
    size_t                 _offset,
    size_t*                _out_next
)
{
    size_t    width = 0;
    const int first = d_internal_raw_at(_source, _options, _offset, &width);

    // a splice begins with a backslash and nothing else
    if (first != '\\')
    {
        return false;
    }

    size_t at = _offset + width;

    // some compilers tolerate blanks between the backslash and the newline
    if ((_options & D_READER_OPT_SPLICE_SPACE) != 0u)
    {
        while ( (at < _source->length) &&
                ((_source->text[at] == ' ') || (_source->text[at] == '\t')) )
        {
            ++at;
        }
    }

    size_t    end_width = 0;
    const int ending    = d_internal_raw_at(_source, _options, at, &end_width);

    // a backslash not followed by a line ending is an ordinary backslash
    if (ending != '\n')
    {
        return false;
    }

    *_out_next = at + end_width;
    return true;
}


/**
 * @brief Reports whether a byte is a UTF-8 continuation byte.
 *
 * @param[in] _byte  the byte to test.
 * @return `true` for 0x80 through 0xBF, `false` otherwise.
 */
static bool
d_internal_is_continuation(
    unsigned char _byte
)
{
    return ((_byte & 0xC0u) == 0x80u);
}


/**
 * @brief Classifies a UTF-8 lead byte.
 *
 * @note Table 3-7 as a lead byte's length plus the range it narrows the
 *       second byte to: E0 excludes the overlong three-byte forms, ED the
 *       surrogates, F0 the overlong four-byte forms and F4 everything past
 *       U+10FFFF.  C0, C1 and F5 through FF can begin nothing.
 *
 * @param[in]  _lead       the lead byte, 0x80 or above.
 * @param[out] _out_value  receives the lead's payload bits.
 * @param[out] _out_low    receives the least second byte allowed.
 * @param[out] _out_high   receives the greatest.
 * @return the sequence length, or zero when the byte begins nothing.
 */
static size_t
d_internal_utf8_lead(
    unsigned char  _lead,
    uint32_t*      _out_value,
    unsigned char* _out_low,
    unsigned char* _out_high
)
{
    *_out_low  = 0x80u;
    *_out_high = 0xBFu;

    if ( (_lead >= 0xC2u) &&
         (_lead <= 0xDFu) )
    {
        *_out_value = (uint32_t)(_lead & 0x1Fu);

        return 2u;
    }

    if ( (_lead >= 0xE0u) &&
         (_lead <= 0xEFu) )
    {
        *_out_value = (uint32_t)(_lead & 0x0Fu);
        *_out_low   = (_lead == 0xE0u) ? 0xA0u : 0x80u;
        *_out_high  = (_lead == 0xEDu) ? 0x9Fu : 0xBFu;

        return 3u;
    }

    if ( (_lead >= 0xF0u) &&
         (_lead <= 0xF4u) )
    {
        *_out_value = (uint32_t)(_lead & 0x07u);
        *_out_low   = (_lead == 0xF0u) ? 0x90u : 0x80u;
        *_out_high  = (_lead == 0xF4u) ? 0x8Fu : 0xBFu;

        return 4u;
    }

    return 0u;
}


/*
d_source_utf8_decode
  Checking the second byte's range from the lead, rather than the decoded
value afterwards, rejects exactly the same inputs and needs no second
comparison per class.
*/
size_t
d_source_utf8_decode(
    const char* _text,
    size_t      _length,
    uint32_t*   _out_code_point
)
{
    // parameter validation first
    if ( (!_text)           ||
         (_length == 0u)    ||
         (!_out_code_point) )
    {
        return 0u;
    }

    const unsigned char* const bytes = (const unsigned char*)_text;
    uint32_t                   value = bytes[0];
    unsigned char              low   = 0u;
    unsigned char              high  = 0u;

    // ASCII is its own code point
    if (bytes[0] < 0x80u)
    {
        *_out_code_point = value;

        return 1u;
    }

    const size_t need = d_internal_utf8_lead(bytes[0], &value, &low, &high);

    // an impossible lead, or a truncated or out-of-range second byte
    if ( (need == 0u)       ||
         (_length < need)   ||
         (bytes[1] < low)   ||
         (bytes[1] > high) )
    {
        return 0u;
    }

    // every byte after the lead carries six bits
    for (size_t at = 1u; at < need; ++at)
    {
        if (!d_internal_is_continuation(bytes[at]))
        {
            return 0u;
        }

        value = (value << 6) | (uint32_t)(bytes[at] & 0x3Fu);
    }

    *_out_code_point = value;

    return need;
}


/**
 * @brief Moves the line and column past one character just consumed, and
 *        validates UTF-8 when the reader was asked to.
 *
 * @note Both advancing entry points come here, so the logical and the
 *       physical reader count lines, columns and encoding identically.  A lead
 *       byte is validated with its whole sequence at once, from the physical
 *       bytes; its continuation bytes are then expected, and one that arrives
 *       unexpected -- or a sequence broken by a splice -- is ill-formed.
 *
 * @param[in,out] _reader     the reader, already moved past the character.
 * @param[in]     _character  the character consumed.
 * @param[in]     _at         the physical offset it was read from.
 */
static void
d_internal_advance(
    struct d_reader* _reader,
    int              _character,
    size_t           _at
)
{
    // a newline opens the next line and resets any sequence in progress
    if (_character == '\n')
    {
        _reader->line   += 1u;
        _reader->column  = 1u;
        _reader->pending = 0u;

        return;
    }

    // below 0x80, or with no validation asked for, a byte is a column
    if ( (_character < 0x80) ||
         ((_reader->options & D_READER_OPT_UTF8) == 0u) )
    {
        _reader->column  += 1u;
        _reader->pending  = 0u;

        return;
    }

    // an expected continuation byte belongs to the column its lead opened
    if (_reader->pending > 0u)
    {
        _reader->pending = (uint8_t)(_reader->pending - 1u);

        return;
    }

    uint32_t     code_point = 0u;
    const size_t width      =
        d_source_utf8_decode(_reader->source->text + _at,
                             _reader->source->length - _at,
                             &code_point);

    _reader->column += 1u;

    // a lead byte that begins no well-formed sequence, or a stray
    // continuation byte, is recorded once until the consumer clears it
    if ( (width < 2u) &&
         (!_reader->malformed) )
    {
        _reader->malformed        = true;
        _reader->malformed_offset = _at;
        _reader->malformed_line   = _reader->line;
        _reader->malformed_column = _reader->column - 1u;
    }

    _reader->pending = (width < 2u) ? 0u : (uint8_t)(width - 1u);

    return;
}


/**
 * @brief Reports whether the byte at an offset is its own character: in the
 *        source, and able to begin no splice, trigraph or line ending.
 *
 * @note Only `\`, `?` and a carriage return can start phase 1 or 2 work, so
 *       every other byte is read as itself with width one.  This is the
 *       reader's fast path; the answer never differs from the full test's.
 *
 * @param[in] _source  the source.
 * @param[in] _offset  the physical offset.
 * @return `true` if the byte needs no phase 1 or 2 processing.
 */
static bool
d_internal_plain_at(
    const struct d_source* _source,
    size_t                 _offset
)
{
    if (_offset >= _source->length)
    {
        return false;
    }

    const char byte = _source->text[_offset];

    return ( (byte != '\\') &&
             (byte != '?')  &&
             (byte != '\r') );
}


//==============================================================================
// 2.  SOURCE
//==============================================================================


/*
d_source_init
  Binds a source to a borrowed byte range.
*/
void
d_source_init(
    struct d_source* _out_source,
    const char*      _text,
    size_t           _length,
    const char*      _name
)
{
    // parameter validation first
    if (!_out_source)
    {
        return;
    }

    _out_source->text   = _text;
    _out_source->length = _text ? _length : 0u;
    _out_source->name   = _name;
    _out_source->id     = 0u;

    return;
}


/*
d_source_skip_bom
  Advances past a UTF-8 byte-order mark when one is present, so that offset
zero is the first byte of content.
*/
bool
d_source_skip_bom(
    struct d_source* _source
)
{
    // parameter validation first
    if ((!_source) || (!_source->text) || (_source->length < 3u))
    {
        return false;
    }

    // reject anything that is not the UTF-8 mark
    if (memcmp(_source->text, "\xEF\xBB\xBF", 3u) != 0)
    {
        return false;
    }

    _source->text   += 3;
    _source->length -= 3u;

    return true;
}


/*
d_source_line_count
  Counts the physical lines in a source.  A trailing line ending does not open
a further line.
*/
size_t
d_source_line_count(
    const struct d_source* _source
)
{
    // parameter validation first
    if ((!_source) || (!_source->text) || (_source->length == 0u))
    {
        return 0u;
    }

    size_t count = 1u;

    for (size_t at = 0u; at < _source->length; ++at)
    {
        // count a line at each ending, but not one past the last
        if (_source->text[at] == '\n')
        {
            if ((at + 1u) < _source->length)
            {
                ++count;
            }
        }
    }

    return count;
}


/*
d_source_line_at
  Locates one physical line, excluding its line ending.
  This is the entry point a verifier uses.  It performs no lexing and the
module needs no scanner linked to serve it.
*/
bool
d_source_line_at(
    const struct d_source* _source,
    uint32_t               _line,
    const char**           _out_text,
    size_t*                _out_length
)
{
    // parameter validation first
    if ( (!_source)        ||
         (!_source->text)  ||
         (!_out_text)      ||
         (!_out_length)    ||
         (_line == 0u) )
    {
        return false;
    }

    uint32_t current = 1u;
    size_t   start   = 0u;

    // walk to the requested line
    while ((current < _line) && (start < _source->length))
    {
        if (_source->text[start] == '\n')
        {
            ++current;
        }

        ++start;
    }

    // the source ended before the line began
    if ((current != _line) || (start > _source->length))
    {
        return false;
    }

    size_t end = start;

    while ((end < _source->length) && (_source->text[end] != '\n'))
    {
        ++end;
    }

    size_t length = end - start;

    // a carriage return belongs to the ending, not to the line
    if ((length > 0u) && (_source->text[start + length - 1u] == '\r'))
    {
        --length;
    }

    *_out_text   = _source->text + start;
    *_out_length = length;

    return true;
}


/*
d_source_line_of
  Reports the one-based physical line containing an offset.
*/
uint32_t
d_source_line_of(
    const struct d_source* _source,
    size_t                 _offset
)
{
    // parameter validation first
    if ((!_source) || (!_source->text) || (_offset > _source->length))
    {
        return 0u;
    }

    uint32_t line = 1u;

    for (size_t at = 0u; at < _offset; ++at)
    {
        if (_source->text[at] == '\n')
        {
            ++line;
        }
    }

    return line;
}


//==============================================================================
// 3.  READER
//==============================================================================


/*
d_reader_init
  Positions a reader at the start of a source.
*/
void
d_reader_init(
    struct d_reader*       _out_reader,
    const struct d_source* _source,
    unsigned               _options
)
{
    // parameter validation first
    if (!_out_reader)
    {
        return;
    }

    _out_reader->source  = _source;
    _out_reader->offset  = 0u;
    _out_reader->line    = 1u;
    _out_reader->column  = 1u;
    _out_reader->options = _options;
    _out_reader->spliced = false;

    _out_reader->malformed        = false;
    _out_reader->pending          = 0u;
    _out_reader->malformed_offset = 0u;
    _out_reader->malformed_line   = 0u;
    _out_reader->malformed_column = 0u;

    return;
}


/*
d_reader_peek
  Reads the logical character `_ahead` positions forward without moving.
  Positions are counted in logical characters, so a splice lying between two
of them is not itself a position.
*/
int
d_reader_peek(
    const struct d_reader* _reader,
    size_t                 _ahead
)
{
    // parameter validation first
    if ((!_reader) || (!_reader->source))
    {
        return D_SOURCE_END;
    }

    size_t at   = _reader->offset;
    size_t next = 0u;

    for (;;)
    {
        // an ordinary byte is its own character and nothing precedes it
        if (d_internal_plain_at(_reader->source, at))
        {
            if (_ahead == 0u)
            {
                return (int)(unsigned char)_reader->source->text[at];
            }

            ++at;
            --_ahead;
            continue;
        }

        // step over every splice before reading
        while (d_internal_splice_at(_reader->source, _reader->options,
                                    at, &next))
        {
            at = next;
        }

        size_t    width     = 0u;
        const int character = d_internal_raw_at(_reader->source,
                                                _reader->options, at, &width);

        // the requested position has been reached
        if (_ahead == 0u)
        {
            return character;
        }

        // the source ended before the position did
        if (character == D_SOURCE_END)
        {
            return D_SOURCE_END;
        }

        at += width;
        --_ahead;
    }
}


/*
d_reader_peek_physical
  Reads one byte at a physical offset from the position, with no phase 1 or 2
applied.  A raw string literal's contents are physical text and are read here.
*/
int
d_reader_peek_physical(
    const struct d_reader* _reader,
    size_t                 _ahead
)
{
    // parameter validation first
    if ((!_reader) || (!_reader->source) || (!_reader->source->text))
    {
        return D_SOURCE_END;
    }

    const size_t at = _reader->offset + _ahead;

    // the offset lies past the end
    if (at >= _reader->source->length)
    {
        return D_SOURCE_END;
    }

    return (int)(unsigned char)_reader->source->text[at];
}


/*
d_reader_next_physical
  Reads one byte at the position and moves past it, with no phase 1 or 2
applied, maintaining the physical line and column.
  A raw string literal's contents are physical text, so a scanner reading one
must advance through this rather than through d_reader_next.  Mixing a
physical peek with a logical advance would consume a splice the peek had
just reported as an ordinary backslash.
*/
int
d_reader_next_physical(
    struct d_reader* _reader
)
{
    // parameter validation first
    if ((!_reader) || (!_reader->source) || (!_reader->source->text))
    {
        return D_SOURCE_END;
    }

    // nothing remains to read
    if (_reader->offset >= _reader->source->length)
    {
        return D_SOURCE_END;
    }

    const size_t at   = _reader->offset;
    const int    byte = (int)(unsigned char)_reader->source->text[at];

    _reader->offset += 1u;

    d_internal_advance(_reader,
                       byte,
                       at);

    return byte;
}


/*
d_reader_next
  Reads the logical character at the position and moves past it, maintaining
the physical line and column.

  `spliced` is not cleared here.  It accumulates until the consumer clears it,
because the question a consumer asks is whether a splice lay anywhere inside
the run it just read, not whether one lay before the last character of it.
*/
int
d_reader_next(
    struct d_reader* _reader
)
{
    // parameter validation first
    if ((!_reader) || (!_reader->source))
    {
        return D_SOURCE_END;
    }

    size_t next = 0u;

    // an ordinary byte is read as itself, with no splice to cross
    if (d_internal_plain_at(_reader->source, _reader->offset))
    {
        const size_t at   = _reader->offset;
        const int    byte = (int)(unsigned char)_reader->source->text[at];

        _reader->offset += 1u;

        d_internal_advance(_reader,
                           byte,
                           at);

        return byte;
    }

    // a splice is crossed silently, but its newline still moves the line
    while (d_internal_splice_at(_reader->source, _reader->options,
                                _reader->offset, &next))
    {
        _reader->offset  = next;
        _reader->line   += 1u;
        _reader->column  = 1u;
        _reader->spliced = true;
        _reader->pending = 0u;
    }

    size_t    width     = 0u;
    const int character = d_internal_raw_at(_reader->source, _reader->options,
                                            _reader->offset, &width);

    // nothing remains to read
    if (character == D_SOURCE_END)
    {
        return D_SOURCE_END;
    }

    const size_t at = _reader->offset;

    _reader->offset += width;

    d_internal_advance(_reader,
                       character,
                       at);

    return character;
}


/*
d_reader_match
  Consumes a run of logical characters when they are the ones given.
*/
bool
d_reader_match(
    struct d_reader* _reader,
    const char*      _text
)
{
    // parameter validation first
    if ((!_reader) || (!_text) || (!*_text))
    {
        return false;
    }

    const size_t length = strlen(_text);

    // compare before consuming anything
    for (size_t at = 0u; at < length; ++at)
    {
        if (d_reader_peek(_reader, at) != (int)(unsigned char)_text[at])
        {
            return false;
        }
    }

    for (size_t at = 0u; at < length; ++at)
    {
        (void)d_reader_next(_reader);
    }

    return true;
}


/*
d_reader_seek
  Moves the reader to a physical offset, recomputing the line and column.
*/
void
d_reader_seek(
    struct d_reader* _reader,
    size_t           _offset
)
{
    // parameter validation first
    if ((!_reader) || (!_reader->source))
    {
        return;
    }

    const size_t target = (_offset > _reader->source->length)
                        ? _reader->source->length
                        : _offset;

    _reader->offset  = target;
    _reader->line    = d_source_line_of(_reader->source, target);
    _reader->spliced = false;

    size_t start = target;

    while ((start > 0u) && (_reader->source->text[start - 1u] != '\n'))
    {
        --start;
    }

    uint32_t column = 1u;

    // under UTF-8 a continuation byte shares its lead's column
    for (size_t at = start; at < target; ++at)
    {
        const unsigned char byte = (unsigned char)_reader->source->text[at];

        if ( ((_reader->options & D_READER_OPT_UTF8) == 0u) ||
             ((byte & 0xC0u) != 0x80u) )
        {
            ++column;
        }
    }

    _reader->column  = column;
    _reader->pending = 0u;

    return;
}


//==============================================================================
// 4.  POSITION
//==============================================================================


/*
d_reader_at_end
  Reports whether the reader has consumed the source.
*/
bool
d_reader_at_end(
    const struct d_reader* _reader
)
{
    return (d_reader_peek(_reader, 0u) == D_SOURCE_END);
}


/*
d_reader_at_line_start
  Reports whether the reader stands at the first column of a line.
*/
bool
d_reader_at_line_start(
    const struct d_reader* _reader
)
{
    return (_reader && (_reader->column == 1u));
}


/*
d_reader_offset
  Reports the physical byte offset of the position.
*/
size_t
d_reader_offset(
    const struct d_reader* _reader
)
{
    return _reader ? _reader->offset : 0u;
}


/*
d_reader_line
  Reports the one-based physical line of the position.
*/
uint32_t
d_reader_line(
    const struct d_reader* _reader
)
{
    return _reader ? _reader->line : 0u;
}


/*
d_reader_column
  Reports the one-based byte column of the position.
*/
uint32_t
d_reader_column(
    const struct d_reader* _reader
)
{
    return _reader ? _reader->column : 0u;
}
