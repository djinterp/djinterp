/*******************************************************************************
* djinterp [tools]                                                       dwiki.c
*
* Definitions for the declarations in dwiki.h and dwiki_driver.h.
*   Reading is the same for every dialect but its middle: the lines are
* found, the driver reads its records, and then the records are put in the
* text's order, each told its line and column, each link the leaf and the
* template holding it; the headings are given anchors, made unique as the
* dialect makes them; the sections are made from the headings at the top
* level; and the regions from the comments standing alone there.
*
* path:      /src/djinterp/tools/dawk/ext/wiki/dwiki.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memcpy, memset, strcmp, strlen, strncmp
// djinterp
#include "./dwiki_driver.h"  // d_wiki_driver
#include "./dwiki_tables.h"  // d_internal_dwiki_entities


// D_INTERNAL_BEGIN, D_INTERNAL_END
//   constant: what a region's markers say, before its name.
#define D_INTERNAL_BEGIN "dawk:begin"
#define D_INTERNAL_END   "dawk:end"

//==============================================================================
// 1.  TEXT
//==============================================================================


/*
d_internal_room
  Room in a buffer for `_more` bytes and a terminator.
*/
static bool
d_internal_room(
    struct d_wiki_buffer* _buffer,
    size_t                _more
)
{
    // a buffer that has failed stays failed
    if (_buffer->failed)
    {
        return false;
    }

    // the room grows by doubling
    if ((_buffer->length + _more + 1u) > _buffer->capacity)
    {
        size_t wanted = (_buffer->capacity > 0u) ? _buffer->capacity : 64u;

        // doubled until the text and its terminator fit
        while (wanted < (_buffer->length + _more + 1u))
        {
            wanted *= 2u;
        }

        char* const grown = realloc(_buffer->text, wanted);

        // memory that ran out: the buffer holds what it held, and says so
        if (!grown)
        {
            _buffer->failed = true;

            return false;
        }

        _buffer->text     = grown;
        _buffer->capacity = wanted;
    }

    return true;
}


bool
d_wiki_buffer_add(
    struct d_wiki_buffer* _buffer,
    const char*           _text,
    size_t                _length
)
{
    // there must be room for the text
    if (!d_internal_room(_buffer, _length))
    {
        return false;
    }

    // nothing to copy leaves only the terminator
    if (_length > 0u)
    {
        memcpy(&_buffer->text[_buffer->length], _text, _length);
    }

    _buffer->length               += _length;
    _buffer->text[_buffer->length] = '\0';

    return true;
}


bool
d_wiki_buffer_add_text(
    struct d_wiki_buffer* _buffer,
    const char*           _text
)
{
    return d_wiki_buffer_add(_buffer, (_text) ? _text : "",
                             (_text) ? strlen(_text) : 0u);
}


bool
d_wiki_buffer_add_char(
    struct d_wiki_buffer* _buffer,
    char                  _byte
)
{
    return d_wiki_buffer_add(_buffer, &_byte, 1u);
}


char*
d_wiki_buffer_take(
    struct d_wiki_buffer* _buffer
)
{
    // a failed buffer gives nothing, and is emptied
    if (_buffer->failed)
    {
        d_wiki_buffer_free(_buffer);

        return NULL;
    }

    // an empty buffer gives an empty text
    if (!_buffer->text)
    {
        return d_wiki_copy("", 0u);
    }

    char* const text = _buffer->text;

    memset(_buffer, 0, sizeof(*_buffer));

    return text;
}


void
d_wiki_buffer_free(
    struct d_wiki_buffer* _buffer
)
{
    free(_buffer->text);
    memset(_buffer, 0, sizeof(*_buffer));

    return;
}


char*
d_wiki_copy(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // a copy that cannot be made is NULL
    if (!copy)
    {
        return NULL;
    }

    // the bytes, when there are any
    if (_length > 0u)
    {
        memcpy(copy, _text, _length);
    }

    copy[_length] = '\0';

    return copy;
}


//==============================================================================
// 2.  CHARACTERS
//==============================================================================


uint32_t
d_wiki_utf8(
    const char* _text,
    size_t      _length,
    size_t*     _out_bytes
)
{
    const unsigned char* const at    = (const unsigned char*)_text;
    uint32_t                   value = 0;
    size_t                     bytes = 0;

    *_out_bytes = 1u;

    // ASCII is itself
    if (at[0] < 0x80u)
    {
        return at[0];
    }

    // the length a lead byte says, and its bits
    if ((at[0] & 0xE0u) == 0xC0u)
    {
        bytes = 2u;
        value = at[0] & 0x1Fu;
    }
    else if ((at[0] & 0xF0u) == 0xE0u)
    {
        bytes = 3u;
        value = at[0] & 0x0Fu;
    }
    else if ((at[0] & 0xF8u) == 0xF0u)
    {
        bytes = 4u;
        value = at[0] & 0x07u;
    }
    else
    {
        return 0xFFFDu;
    }

    // a character cut short is not one
    if (bytes > _length)
    {
        return 0xFFFDu;
    }

    // each continuation byte adds six bits
    for (size_t index = 1u; index < bytes; ++index)
    {
        // a byte that does not continue the character
        if ((at[index] & 0xC0u) != 0x80u)
        {
            return 0xFFFDu;
        }

        value = (value << 6) | (at[index] & 0x3Fu);
    }

    // overlong forms, surrogates and values past Unicode are not characters
    if ( ( (bytes == 2u) && (value < 0x80u) )     ||
         ( (bytes == 3u) && (value < 0x800u) )    ||
         ( (bytes == 4u) && (value < 0x10000u) )  ||
         ( (value >= 0xD800u) && (value <= 0xDFFFu) ) ||
         (value > 0x10FFFFu) )
    {
        return 0xFFFDu;
    }

    *_out_bytes = bytes;

    return value;
}


bool
d_wiki_add_utf8(
    struct d_wiki_buffer* _out,
    uint32_t              _character
)
{
    char   bytes[4];
    size_t length = 0;

    // what is not a character is written as U+FFFD
    if ( (_character > 0x10FFFFu) ||
         ( (_character >= 0xD800u) && (_character <= 0xDFFFu) ) )
    {
        _character = 0xFFFDu;
    }

    // one byte to four, as the value needs
    if (_character < 0x80u)
    {
        bytes[length++] = (char)_character;
    }
    else if (_character < 0x800u)
    {
        bytes[length++] = (char)(0xC0u | (_character >> 6));
        bytes[length++] = (char)(0x80u | (_character & 0x3Fu));
    }
    else if (_character < 0x10000u)
    {
        bytes[length++] = (char)(0xE0u | (_character >> 12));
        bytes[length++] = (char)(0x80u | ((_character >> 6) & 0x3Fu));
        bytes[length++] = (char)(0x80u | (_character & 0x3Fu));
    }
    else
    {
        bytes[length++] = (char)(0xF0u | (_character >> 18));
        bytes[length++] = (char)(0x80u | ((_character >> 12) & 0x3Fu));
        bytes[length++] = (char)(0x80u | ((_character >> 6) & 0x3Fu));
        bytes[length++] = (char)(0x80u | (_character & 0x3Fu));
    }

    return d_wiki_buffer_add(_out, bytes, length);
}


/*
d_internal_in_ranges
  Whether a character is in a sorted table of ranges.
*/
static bool
d_internal_in_ranges(
    const uint32_t (*_ranges)[2],
    size_t           _count,
    uint32_t         _character
)
{
    size_t low  = 0;
    size_t high = _count;

    // a binary search for the range at or below the character
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // the ranges above it, or those to its middle
        if (_ranges[middle][1] < _character)
        {
            low = middle + 1u;
        }
        else if (_ranges[middle][0] > _character)
        {
            high = middle;
        }
        else
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_is_ascii_punctuation
  Whether a byte is ASCII's punctuation, as CommonMark names it.
*/
static bool
d_internal_is_ascii_punctuation(
    uint32_t _character
)
{
    return ( ( (_character >= 0x21u) && (_character <= 0x2Fu) ) ||
             ( (_character >= 0x3Au) && (_character <= 0x40u) ) ||
             ( (_character >= 0x5Bu) && (_character <= 0x60u) ) ||
             ( (_character >= 0x7Bu) && (_character <= 0x7Eu) ) );
}


bool
d_wiki_is_punctuation(
    uint32_t _character
)
{
    // ASCII's, then Unicode's punctuation and symbols
    if (_character < 0x80u)
    {
        return d_internal_is_ascii_punctuation(_character);
    }

    return d_internal_in_ranges(d_internal_dwiki_punctuation,
                                D_INTERNAL_DWIKI_PUNCTUATION_COUNT,
                                _character);
}


bool
d_wiki_is_space(
    uint32_t _character
)
{
    // ASCII's, then the space separators
    if (_character < 0x80u)
    {
        return ( (_character == ' ')  ||
                 (_character == '\t') ||
                 (_character == '\n') ||
                 (_character == '\f') ||
                 (_character == '\r') );
    }

    return d_internal_in_ranges(d_internal_dwiki_space,
                                D_INTERNAL_DWIKI_SPACE_COUNT,
                                _character);
}


bool
d_wiki_is_word(
    uint32_t _character
)
{
    // ASCII's letters, digits and underscore, then Unicode's
    if (_character < 0x80u)
    {
        return ( ( (_character >= 'a') && (_character <= 'z') ) ||
                 ( (_character >= 'A') && (_character <= 'Z') ) ||
                 ( (_character >= '0') && (_character <= '9') ) ||
                 (_character == '_') );
    }

    return d_internal_in_ranges(d_internal_dwiki_word,
                                D_INTERNAL_DWIKI_WORD_COUNT,
                                _character);
}


uint32_t
d_wiki_lower(
    uint32_t _character
)
{
    // ASCII's capitals are 32 below their small letters
    if (_character < 0x80u)
    {
        return ( (_character >= 'A') && (_character <= 'Z') )
                   ? (_character + 32u) : _character;
    }

    // each run of capitals is a distance from its small letters
    for (size_t index = 0; index < D_INTERNAL_DWIKI_LOWERS_COUNT; ++index)
    {
        const struct d_internal_dwiki_lower* const run =
            &d_internal_dwiki_lowers[index];

        // the runs are sorted, so one past the character ends the search
        if (run->first > _character)
        {
            break;
        }

        // a capital within the run, at its step
        if ( (_character <= run->last) &&
             (((_character - run->first) % run->step) == 0u) )
        {
            return (uint32_t)((int32_t)_character + run->delta);
        }
    }

    return _character;
}


/*
d_internal_compare_entity
  Orders an entity's name against a looked-for name of a length.
*/
static int
d_internal_compare_entity(
    const char* _name,
    const char* _wanted,
    size_t      _length
)
{
    const int order = strncmp(_name, _wanted, _length);

    // the same bytes, but a longer name, comes after
    if (order != 0)
    {
        return order;
    }

    return (_name[_length] == '\0') ? 0 : 1;
}


size_t
d_wiki_entity(
    const char*           _text,
    size_t                _length,
    struct d_wiki_buffer* _out
)
{
    size_t at = 1u;

    // a reference begins with `&`
    if ( (_length < 3u) ||
         (_text[0] != '&') )
    {
        return 0u;
    }

    // a number: &#nnn; or &#xhhh;
    if (_text[1] == '#')
    {
        const bool hex    = ( (_length > 2u) &&
                              ( (_text[2] == 'x') || (_text[2] == 'X') ) );
        uint32_t   value  = 0;
        size_t     digits = 0;

        at = (hex) ? 3u : 2u;

        // up to seven decimal digits, or six hexadecimal
        while ( (at < _length) &&
                (digits < ((hex) ? 6u : 7u)) )
        {
            const char c = _text[at];
            uint32_t   digit;

            // a digit, decimal or hexadecimal
            if ( (c >= '0') && (c <= '9') )
            {
                digit = (uint32_t)(c - '0');
            }
            else if ( (hex) && (c >= 'a') && (c <= 'f') )
            {
                digit = (uint32_t)(c - 'a' + 10);
            }
            else if ( (hex) && (c >= 'A') && (c <= 'F') )
            {
                digit = (uint32_t)(c - 'A' + 10);
            }
            else
            {
                break;
            }

            value = (value * ((hex) ? 16u : 10u)) + digit;
            digits++;
            at++;
        }

        // a number needs a digit, and a `;`
        if ( (digits == 0u) ||
             (at >= _length) ||
             (_text[at] != ';') )
        {
            return 0u;
        }

        // U+0000 is written as U+FFFD
        (void)d_wiki_add_utf8(_out, (value == 0u) ? 0xFFFDu : value);

        return at + 1u;
    }

    // a name: letters and digits, then `;`
    while ( (at < _length) &&
            (at < 40u) &&
            ( ( (_text[at] >= 'a') && (_text[at] <= 'z') ) ||
              ( (_text[at] >= 'A') && (_text[at] <= 'Z') ) ||
              ( (_text[at] >= '0') && (_text[at] <= '9') ) ) )
    {
        at++;
    }

    // a reference with no digits or no `;` is not one
    if ( (at == 1u) ||
         (at >= _length) ||
         (_text[at] != ';') )
    {
        return 0u;
    }

    size_t low  = 0;
    size_t high = D_INTERNAL_DWIKI_ENTITIES_COUNT;

    // a binary search among the names
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);
        const int    order  = d_internal_compare_entity(
                                  d_internal_dwiki_entities[middle].name,
                                  &_text[1], at - 1u);

        // the names after it, or those to its middle
        if (order < 0)
        {
            low = middle + 1u;
        }
        else if (order > 0)
        {
            high = middle;
        }
        else
        {
            (void)d_wiki_buffer_add_text(_out,
                                         d_internal_dwiki_entities[middle]
                                             .text);

            return at + 1u;
        }
    }

    return 0u;
}


bool
d_wiki_emoji(
    const char*           _name,
    size_t                _length,
    struct d_wiki_buffer* _out
)
{
    size_t low  = 0;
    size_t high = D_INTERNAL_DWIKI_EMOJI_COUNT;

    // a binary search among the names, sorted by their bytes
    while ( (_length > 0u) &&
            (low < high) )
    {
        const size_t middle = low + ((high - low) / 2u);
        const int    order  = d_internal_compare_entity(
                                  d_internal_dwiki_emoji[middle].name,
                                  _name, _length);

        // the names after it, or those to its middle
        if (order < 0)
        {
            low = middle + 1u;
        }
        else if (order > 0)
        {
            high = middle;
        }
        else
        {
            return d_wiki_buffer_add_text(_out,
                                          d_internal_dwiki_emoji[middle].text);
        }
    }

    return false;
}


bool
d_wiki_has_scheme(
    const char* _target
)
{
    size_t at = 0;

    // a scheme is a letter, then letters, digits, `+`, `.` or `-`, then `:`
    if (!( ( (_target[0] >= 'a') && (_target[0] <= 'z') ) ||
           ( (_target[0] >= 'A') && (_target[0] <= 'Z') ) ))
    {
        return false;
    }

    // the scheme's other bytes, up to 32
    while ( (_target[at] != '\0') &&
            (at < 32u) &&
            ( ( (_target[at] >= 'a') && (_target[at] <= 'z') ) ||
              ( (_target[at] >= 'A') && (_target[at] <= 'Z') ) ||
              ( (_target[at] >= '0') && (_target[at] <= '9') ) ||
              (_target[at] == '+') ||
              (_target[at] == '.') ||
              (_target[at] == '-') ) )
    {
        at++;
    }

    // one letter before `:` is a drive, C:\, not a scheme
    return ( (at >= 2u) &&
             (_target[at] == ':') );
}


bool
d_wiki_is_image(
    const char* _path,
    size_t      _length
)
{
    static const char* const extensions[] =
    {
        "png", "jpg", "jpeg", "gif", "svg", "webp", "bmp", "ico", "avif",
        "tif", "tiff"
    };
    size_t dot = _length;

    // the extension is after the last `.` of the last part
    while ( (dot > 0u) &&
            (_path[dot - 1u] != '.') &&
            (_path[dot - 1u] != '/') )
    {
        dot--;
    }

    // no extension
    if ( (dot == 0u) ||
         (_path[dot - 1u] != '.') )
    {
        return false;
    }

    const size_t span  = _length - dot;
    const size_t count = sizeof(extensions) / sizeof(extensions[0]);

    // each extension, in any case
    for (size_t index = 0; index < count; ++index)
    {
        size_t at = 0;

        // an extension of another length
        if (strlen(extensions[index]) != span)
        {
            continue;
        }

        // byte by byte, without case
        while ( (at < span) &&
                ((char)d_wiki_lower((unsigned char)_path[dot + at])
                     == extensions[index][at]) )
        {
            at++;
        }

        // the extension, in any case
        if (at == span)
        {
            return true;
        }
    }

    return false;
}


bool
d_wiki_normalize_label(
    struct d_wiki_buffer* _out,
    const char*           _label,
    size_t                _length
)
{
    bool   space = false;
    size_t at    = 0;

    // each character, its runs of space one space, in lower case
    while (at < _length)
    {
        size_t         bytes     = 0;
        const uint32_t character = d_wiki_utf8(&_label[at], _length - at,
                                               &bytes);

        at += bytes;

        // a run of space is written once, before what follows it
        if (d_wiki_is_space(character))
        {
            space = true;

            continue;
        }

        // a space between words, none at the start
        if ( (space) &&
             (_out->length > 0u) )
        {
            (void)d_wiki_buffer_add_char(_out, ' ');
        }

        space = false;
        (void)d_wiki_add_utf8(_out, d_wiki_lower(character));
    }

    return !_out->failed;
}


//==============================================================================
// 3.  DIALECTS
//==============================================================================


/*
d_internal_ends_with
  Whether a name ends with an extension, in any case.
*/
static bool
d_internal_ends_with(
    const char* _name,
    size_t      _length,
    const char* _extension
)
{
    const size_t span = strlen(_extension);

    // the name must be longer than the extension
    if (_length <= span)
    {
        return false;
    }

    // byte by byte, without case
    for (size_t at = 0; at < span; ++at)
    {
        // a byte that differs, in any case
        if ((char)d_wiki_lower((unsigned char)_name[_length - span + at])
                != _extension[at])
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_extension_of
  The dialect an extension names, and its length with the `.`, 0 for none.
*/
static size_t
d_internal_extension_of(
    const char*          _name,
    size_t               _length,
    enum d_wiki_dialect* _out_dialect
)
{
    static const struct
    {
        const char*         extension;
        enum d_wiki_dialect dialect;
    } extensions[] =
    {
        { ".md",        D_WIKI_MARKDOWN  },
        { ".markdown",  D_WIKI_MARKDOWN  },
        { ".mdown",     D_WIKI_MARKDOWN  },
        { ".mkd",       D_WIKI_MARKDOWN  },
        { ".mkdn",      D_WIKI_MARKDOWN  },
        { ".mediawiki", D_WIKI_MEDIAWIKI },
        { ".wiki",      D_WIKI_MEDIAWIKI },
        { ".wikitext",  D_WIKI_MEDIAWIKI }
    };
    const size_t count = sizeof(extensions) / sizeof(extensions[0]);

    // each extension a dialect has
    for (size_t index = 0; index < count; ++index)
    {
        // the extension the name ends with
        if (d_internal_ends_with(_name, _length,
                                 extensions[index].extension))
        {
            *_out_dialect = extensions[index].dialect;

            return strlen(extensions[index].extension);
        }
    }

    *_out_dialect = D_WIKI_OTHER;

    return 0u;
}


enum d_wiki_dialect
d_wiki_dialect_of(
    const char* _path
)
{
    enum d_wiki_dialect dialect = D_WIKI_OTHER;

    // a path that is not there names nothing
    if (!_path)
    {
        return D_WIKI_OTHER;
    }

    (void)d_internal_extension_of(_path, strlen(_path), &dialect);

    return dialect;
}


enum d_wiki_dialect
d_wiki_dialect_named(
    const char* _name
)
{
    static const struct
    {
        const char*         name;
        enum d_wiki_dialect dialect;
    } names[] =
    {
        { "markdown",  D_WIKI_MARKDOWN  },
        { "md",        D_WIKI_MARKDOWN  },
        { "github",    D_WIKI_MARKDOWN  },
        { "gfm",       D_WIKI_MARKDOWN  },
        { "gollum",    D_WIKI_MARKDOWN  },
        { "mediawiki", D_WIKI_MEDIAWIKI },
        { "wiki",      D_WIKI_MEDIAWIKI },
        { "wikitext",  D_WIKI_MEDIAWIKI }
    };

    // a name not there names nothing
    if (!_name)
    {
        return D_WIKI_OTHER;
    }

    // each name, in any case
    for (size_t index = 0; index < (sizeof(names) / sizeof(names[0])); ++index)
    {
        const char* left  = _name;
        const char* right = names[index].name;

        // byte by byte, without case
        while ( (*left != '\0') &&
                ((char)d_wiki_lower((unsigned char)*left) == *right) )
        {
            left++;
            right++;
        }

        // the whole name, in any case
        if ( (*left == '\0') &&
             (*right == '\0') )
        {
            return names[index].dialect;
        }
    }

    return D_WIKI_OTHER;
}


const char*
d_wiki_dialect_name(
    enum d_wiki_dialect _dialect
)
{
    // a table of the names, by dialect
    static const char* const names[] =
    {
        "other", "markdown", "mediawiki"
    };

    return ((size_t)_dialect < (sizeof(names) / sizeof(names[0])))
               ? names[_dialect] : "other";
}


const struct d_wiki_driver*
d_wiki_driver_of(
    enum d_wiki_dialect _dialect
)
{
    // a table of the drivers, by dialect
    switch (_dialect)
    {
        case D_WIKI_MARKDOWN:
            return d_wiki_markdown_driver();

        case D_WIKI_MEDIAWIKI:
            return d_wiki_mediawiki_driver();

        default:
            return NULL;
    }
}


//==============================================================================
// 4.  RECORDS
//==============================================================================


/*
d_internal_grow
  Room in an array for one more item.
*/
static bool
d_internal_grow(
    void**  _items,
    size_t* _capacity,
    size_t  _count,
    size_t  _size
)
{
    // there is room already
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t wanted = (*_capacity > 0u) ? (*_capacity * 2u) : 16u;
    void* const  grown  = realloc(*_items, wanted * _size);

    // memory that ran out: the items are as they were
    if (!grown)
    {
        return false;
    }

    *_items    = grown;
    *_capacity = wanted;

    return true;
}


size_t
d_wiki_add_block(
    struct d_wiki*         _wiki,
    enum d_wiki_block_kind _kind,
    size_t                 _start
)
{
    // room for one more
    if (!d_internal_grow((void**)&_wiki->blocks, &_wiki->block_capacity,
                         _wiki->block_count, sizeof(struct d_wiki_block)))
    {
        return D_WIKI_NONE;
    }

    struct d_wiki_block* const block = &_wiki->blocks[_wiki->block_count];

    memset(block, 0, sizeof(*block));
    block->kind          = _kind;
    block->start         = _start;
    block->end           = _start;
    block->parent        = D_WIKI_NONE;
    block->first_segment = _wiki->segment_count;
    block->info_start    = D_WIKI_NONE;
    block->info_end      = D_WIKI_NONE;

    return _wiki->block_count++;
}


bool
d_wiki_add_segment(
    struct d_wiki* _wiki,
    size_t         _start,
    size_t         _end,
    size_t         _pad
)
{
    // a segment belongs to the last block
    if ( (_wiki->block_count == 0u) ||
         (!d_internal_grow((void**)&_wiki->segments,
                           &_wiki->segment_capacity, _wiki->segment_count,
                           sizeof(struct d_wiki_segment))) )
    {
        return false;
    }

    _wiki->segments[_wiki->segment_count].start = _start;
    _wiki->segments[_wiki->segment_count].end   = _end;
    _wiki->segments[_wiki->segment_count].pad   = _pad;
    _wiki->segment_count++;
    _wiki->blocks[_wiki->block_count - 1u].segment_count++;

    return true;
}


size_t
d_wiki_add_heading(
    struct d_wiki*               _wiki,
    const struct d_wiki_heading* _heading
)
{
    // room for one more
    if (!d_internal_grow((void**)&_wiki->headings, &_wiki->heading_capacity,
                         _wiki->heading_count,
                         sizeof(struct d_wiki_heading)))
    {
        return D_WIKI_NONE;
    }

    _wiki->headings[_wiki->heading_count] = *_heading;

    return _wiki->heading_count++;
}


size_t
d_wiki_add_link(
    struct d_wiki*            _wiki,
    const struct d_wiki_link* _link
)
{
    // room for one more
    if (!d_internal_grow((void**)&_wiki->links, &_wiki->link_capacity,
                         _wiki->link_count, sizeof(struct d_wiki_link)))
    {
        return D_WIKI_NONE;
    }

    _wiki->links[_wiki->link_count] = *_link;

    return _wiki->link_count++;
}


size_t
d_wiki_add_template(
    struct d_wiki*                _wiki,
    const struct d_wiki_template* _template
)
{
    // room for one more
    if (!d_internal_grow((void**)&_wiki->templates,
                         &_wiki->template_capacity, _wiki->template_count,
                         sizeof(struct d_wiki_template)))
    {
        return D_WIKI_NONE;
    }

    _wiki->templates[_wiki->template_count] = *_template;

    return _wiki->template_count++;
}


size_t
d_wiki_add_argument(
    struct d_wiki*                _wiki,
    const struct d_wiki_argument* _argument
)
{
    // room for one more
    if (!d_internal_grow((void**)&_wiki->arguments,
                         &_wiki->argument_capacity, _wiki->argument_count,
                         sizeof(struct d_wiki_argument)))
    {
        return D_WIKI_NONE;
    }

    _wiki->arguments[_wiki->argument_count] = *_argument;

    return _wiki->argument_count++;
}


bool
d_wiki_add_problem(
    struct d_wiki* _wiki,
    size_t         _offset,
    const char*    _what
)
{
    char* const what = d_wiki_copy(_what, strlen(_what));

    // room for one more
    if ( (!what) ||
         (!d_internal_grow((void**)&_wiki->problems,
                           &_wiki->problem_capacity, _wiki->problem_count,
                           sizeof(struct d_wiki_problem))) )
    {
        free(what);

        return false;
    }

    _wiki->problems[_wiki->problem_count].line =
        d_wiki_line_of(_wiki, _offset, NULL);
    _wiki->problems[_wiki->problem_count].what = what;
    _wiki->problem_count++;

    return true;
}


void
d_wiki_new_link(
    struct d_wiki_link* _link,
    enum d_wiki_form    _form,
    size_t              _start,
    size_t              _end
)
{
    memset(_link, 0, sizeof(*_link));
    _link->form         = _form;
    _link->to           = D_WIKI_TO_URL;
    _link->start        = _start;
    _link->end          = _end;
    _link->close        = _end;
    _link->target_start = D_WIKI_NONE;
    _link->target_end   = D_WIKI_NONE;
    _link->label_start  = D_WIKI_NONE;
    _link->label_end    = D_WIKI_NONE;
    _link->title_start  = D_WIKI_NONE;
    _link->title_end    = D_WIKI_NONE;
    _link->definition   = D_WIKI_NONE;
    _link->block        = D_WIKI_NONE;
    _link->parent       = D_WIKI_NONE;

    return;
}


//==============================================================================
// 5.  READING
//==============================================================================


/*
d_internal_find_lines
  The start of every line, a line ending `\n`, `\r\n` or `\r`; the line
after a last line ending is a line too, empty.
*/
static bool
d_internal_find_lines(
    struct d_wiki* _wiki
)
{
    size_t count = 1u;

    // one line, and one more after each line ending
    for (size_t at = 0; at < _wiki->length; ++at)
    {
        // a line feed, or a CR alone, ends a line
        if ( (_wiki->text[at] == '\n') ||
             ( (_wiki->text[at] == '\r') &&
               ( ((at + 1u) >= _wiki->length) ||
                 (_wiki->text[at + 1u] != '\n') ) ) )
        {
            count++;
        }
    }

    _wiki->line_starts = malloc(count * sizeof(size_t));

    // memory that ran out
    if (!_wiki->line_starts)
    {
        return false;
    }

    _wiki->line_starts[0] = 0u;
    _wiki->line_count     = 1u;

    // the starts, in order
    for (size_t at = 0; at < _wiki->length; ++at)
    {
        // a line ending, and where the next line starts
        if ( (_wiki->text[at] == '\n') ||
             ( (_wiki->text[at] == '\r') &&
               ( ((at + 1u) >= _wiki->length) ||
                 (_wiki->text[at + 1u] != '\n') ) ) )
        {
            // the first line ending says how the text ends its lines
            if (_wiki->line_count == 1u)
            {
                _wiki->crlf = (_wiki->text[at] == '\n') &&
                              (at > 0u) &&
                              (_wiki->text[at - 1u] == '\r');
            }

            _wiki->line_starts[_wiki->line_count++] = at + 1u;
        }
    }

    return true;
}


size_t
d_wiki_line_of(
    const struct d_wiki* _wiki,
    size_t               _offset,
    size_t*              _out_column
)
{
    size_t low  = 0;
    size_t high = _wiki->line_count;

    // no lines found yet is the first line
    if (_wiki->line_count == 0u)
    {
        // a text with no lines: one line, its column the offset's
        if (_out_column)
        {
            *_out_column = _offset + 1u;
        }

        return 1u;
    }

    // the last line that starts at or before the offset
    while ((low + 1u) < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // the lines from the middle, or those before it
        if (_wiki->line_starts[middle] <= _offset)
        {
            low = middle;
        }
        else
        {
            high = middle;
        }
    }

    // the column within the line, from 1
    if (_out_column)
    {
        *_out_column = _offset - _wiki->line_starts[low] + 1u;
    }

    return low + 1u;
}


/*
d_internal_compare_links, d_internal_compare_templates
  Order records by where they start, and those that start together by the
order they were found in, which a record's place among the found keeps.
*/
struct d_internal_order
{
    size_t start;
    size_t end;
    size_t index;
};


static int
d_internal_compare_order(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_order* const left  = _left;
    const struct d_internal_order* const right = _right;

    // by start, then the longer first, then as found
    if (left->start != right->start)
    {
        return (left->start < right->start) ? -1 : 1;
    }

    // the longer of two that start together first
    if (left->end != right->end)
    {
        return (left->end > right->end) ? -1 : 1;
    }

    return (left->index < right->index) ? -1
         : (left->index > right->index) ?  1 : 0;
}


/*
d_internal_order_links
  Puts the links in the text's order, keeping each reference's definition.
*/
static bool
d_internal_order_links(
    struct d_wiki* _wiki
)
{
    const size_t                   count  = _wiki->link_count;
    struct d_internal_order* const order  = malloc((count + 1u) *
                                                   sizeof(*order));
    size_t* const                  placed = malloc((count + 1u) *
                                                   sizeof(size_t));
    struct d_wiki_link* const      sorted = malloc((count + 1u) *
                                                   sizeof(*sorted));

    // everything at once, or nothing
    if ( (!order) ||
         (!placed) ||
         (!sorted) )
    {
        free(order);
        free(placed);
        free(sorted);

        return false;
    }

    // where each link starts
    for (size_t index = 0; index < count; ++index)
    {
        order[index].start = _wiki->links[index].start;
        order[index].end   = _wiki->links[index].end;
        order[index].index = index;
    }

    qsort(order, count, sizeof(*order), d_internal_compare_order);

    // where each link went
    for (size_t index = 0; index < count; ++index)
    {
        placed[order[index].index] = index;
        sorted[index]              = _wiki->links[order[index].index];
    }

    // a definition's index follows it
    for (size_t index = 0; index < count; ++index)
    {
        // a definition's index, where it is now
        if (sorted[index].definition != D_WIKI_NONE)
        {
            sorted[index].definition = placed[sorted[index].definition];
        }
    }

    // the links, sorted
    if (count > 0u)
    {
        memcpy(_wiki->links, sorted, count * sizeof(*sorted));
    }

    free(order);
    free(placed);
    free(sorted);

    return true;
}


/*
d_internal_order_templates
  Puts the templates in the text's order, and gives each the template
whose arguments hold it.
*/
static bool
d_internal_order_templates(
    struct d_wiki* _wiki
)
{
    const size_t                   count  = _wiki->template_count;
    struct d_internal_order* const order  = malloc((count + 1u) *
                                                   sizeof(*order));
    struct d_wiki_template* const  sorted = malloc((count + 1u) *
                                                   sizeof(*sorted));
    size_t* const                  stack  = malloc((count + 1u) *
                                                   sizeof(size_t));
    size_t                         depth  = 0;

    // everything at once, or nothing
    if ( (!order) ||
         (!sorted) ||
         (!stack) )
    {
        free(order);
        free(sorted);
        free(stack);

        return false;
    }

    // where each template starts
    for (size_t index = 0; index < count; ++index)
    {
        order[index].start = _wiki->templates[index].start;
        order[index].end   = _wiki->templates[index].end;
        order[index].index = index;
    }

    qsort(order, count, sizeof(*order), d_internal_compare_order);

    // in order, each held by the last one open around it
    for (size_t index = 0; index < count; ++index)
    {
        sorted[index] = _wiki->templates[order[index].index];

        // those that end before it are no longer open
        while ( (depth > 0u) &&
                (sorted[stack[depth - 1u]].end <= sorted[index].start) )
        {
            depth--;
        }

        sorted[index].parent = (depth > 0u) ? stack[depth - 1u]
                                            : D_WIKI_NONE;
        stack[depth++]       = index;
    }

    // the templates, sorted
    if (count > 0u)
    {
        memcpy(_wiki->templates, sorted, count * sizeof(*sorted));
    }

    free(order);
    free(sorted);
    free(stack);

    return true;
}


/*
d_internal_place_links
  Gives each link its line and column, the template whose arguments hold
it, and, when its driver did not, the leaf that holds it.
*/
static void
d_internal_place_links(
    struct d_wiki* _wiki
)
{
    size_t leaf = 0;

    // each link, in order, as the leaves are, in order
    for (size_t index = 0; index < _wiki->link_count; ++index)
    {
        struct d_wiki_link* const link = &_wiki->links[index];

        link->line = d_wiki_line_of(_wiki, link->start, &link->column);

        // the innermost template around it, the last that starts before it
        link->parent = D_WIKI_NONE;

        // the templates that start before it
        for (size_t at = 0; at < _wiki->template_count; ++at)
        {
            // a template past it: none after it holds it
            if (_wiki->templates[at].start > link->start)
            {
                break;
            }

            // one that holds it, the innermost so far
            if (_wiki->templates[at].end >= link->end)
            {
                link->parent = at;
            }
        }

        // the driver has said which leaf holds it
        if (link->block != D_WIKI_NONE)
        {
            continue;
        }

        // the last leaf that starts at or before it
        while ( ((leaf + 1u) < _wiki->block_count) &&
                (_wiki->blocks[leaf + 1u].start <= link->start) )
        {
            leaf++;
        }

        // the leaf found holds it, if it reaches it
        if ( (leaf < _wiki->block_count) &&
             (_wiki->blocks[leaf].start <= link->start) &&
             (_wiki->blocks[leaf].end > link->start) )
        {
            link->block = leaf;
        }
    }

    return;
}


/*
d_internal_order_headings
  Puts the headings in the text's order, as a driver may find them out of
it: an insertion sort, as most are in order already.
*/
static void
d_internal_order_headings(
    struct d_wiki* _wiki
)
{
    // each heading moved back past those that start after it
    for (size_t index = 1u; index < _wiki->heading_count; ++index)
    {
        const struct d_wiki_heading moving = _wiki->headings[index];
        size_t                      at     = index;

        // those after it moved up, to its place
        while ( (at > 0u) &&
                (_wiki->headings[at - 1u].start > moving.start) )
        {
            _wiki->headings[at] = _wiki->headings[at - 1u];
            at--;
        }

        _wiki->headings[at] = moving;
    }

    return;
}


/*
d_internal_place
  Gives every other record its line and column.
*/
static void
d_internal_place(
    struct d_wiki* _wiki
)
{
    // a block from its first line to its last
    for (size_t index = 0; index < _wiki->block_count; ++index)
    {
        struct d_wiki_block* const block = &_wiki->blocks[index];

        block->line     = d_wiki_line_of(_wiki, block->start, NULL);
        block->end_line = d_wiki_line_of(_wiki,
                                         (block->end > block->start)
                                             ? (block->end - 1u)
                                             : block->start, NULL);
    }

    // a heading from its first line; one at the top from that line's start
    for (size_t index = 0; index < _wiki->heading_count; ++index)
    {
        struct d_wiki_heading* const heading = &_wiki->headings[index];

        heading->line = d_wiki_line_of(_wiki, heading->start, NULL);

        // a heading at the top starts at its line, past the indent before it
        if (heading->top)
        {
            heading->start = _wiki->line_starts[heading->line - 1u];
        }
    }

    // a template where it opens
    for (size_t index = 0; index < _wiki->template_count; ++index)
    {
        struct d_wiki_template* const tmpl = &_wiki->templates[index];

        tmpl->line = d_wiki_line_of(_wiki, tmpl->start, &tmpl->column);
    }

    return;
}


/*
d_internal_name_anchors
  Gives every heading its anchor: its title's, and, for one that another
heading before it has, that anchor told apart by a number -- or as its
dialect tells them apart, for one that has its own way.
*/
static bool
d_internal_name_anchors(
    struct d_wiki*              _wiki,
    const struct d_wiki_driver* _driver
)
{
    bool ok = true;

    // each heading's anchor from its title
    for (size_t index = 0; ( (ok) && (index < _wiki->heading_count) );
         ++index)
    {
        struct d_wiki_buffer   base    = { NULL, 0u, 0u, false };
        struct d_wiki_heading* heading = &_wiki->headings[index];

        ok = _driver->anchor(&base, (heading->title) ? heading->title : "");
        heading->anchor = d_wiki_buffer_take(&base);
        ok = (ok) && (heading->anchor != NULL);
    }

    // a dialect's own way of telling them apart
    if ( (!ok) ||
         (_driver->number) )
    {
        return (ok) && (_driver->number(_wiki));
    }

    // each heading after the first, numbered by those before it that share
    // its anchor, the latest first, as each is numbered in its turn
    for (size_t index = _wiki->heading_count; ( (ok) && (index > 0u) );
         --index)
    {
        struct d_wiki_heading* const heading = &_wiki->headings[index - 1u];
        size_t                       repeats = 0;

        // how many before it had the same anchor
        for (size_t before = 0; before < (index - 1u); ++before)
        {
            // the same anchor before it
            if (strcmp(_wiki->headings[before].anchor, heading->anchor) == 0)
            {
                repeats++;
            }
        }

        // a repeated anchor, numbered as its dialect numbers it
        if (repeats > 0u)
        {
            struct d_wiki_buffer anchor = { NULL, 0u, 0u, false };
            char                 number[32];

            (void)snprintf(number, sizeof(number), "%c%zu",
                           _driver->separator,
                           _driver->first_repeat + repeats - 1u);
            (void)d_wiki_buffer_add_text(&anchor, heading->anchor);
            (void)d_wiki_buffer_add_text(&anchor, number);
            free(heading->anchor);
            heading->anchor = d_wiki_buffer_take(&anchor);
            ok              = (heading->anchor != NULL);
        }
    }

    return ok;
}


/*
d_internal_make_sections
  The lead, and a section for each heading at the top level.
*/
static bool
d_internal_make_sections(
    struct d_wiki* _wiki
)
{
    const size_t begin = (_wiki->bom) ? 3u : 0u;
    size_t       count = 1u;

    // one more for each heading at the top
    for (size_t index = 0; index < _wiki->heading_count; ++index)
    {
        count += (_wiki->headings[index].top) ? 1u : 0u;
    }

    _wiki->sections = malloc(count * sizeof(struct d_wiki_section));

    // memory that ran out
    if (!_wiki->sections)
    {
        return false;
    }

    _wiki->section_count = count;

    struct d_wiki_section* const lead = &_wiki->sections[0];

    lead->heading = D_WIKI_NONE;
    lead->level   = 0u;
    lead->start   = begin;
    lead->body    = begin;
    lead->parent  = D_WIKI_NONE;

    size_t at = 1u;

    // each section from its heading
    for (size_t index = 0; index < _wiki->heading_count; ++index)
    {
        const struct d_wiki_heading* const heading = &_wiki->headings[index];

        // a heading within a list or a template is no section's
        if (!heading->top)
        {
            continue;
        }

        _wiki->sections[at].heading = index;
        _wiki->sections[at].level   = heading->level;
        _wiki->sections[at].start   = heading->start;
        _wiki->sections[at].body    = heading->end;
        _wiki->sections[at].parent  = D_WIKI_NONE;
        at++;
    }

    // each section ends where the next at its level or above begins
    for (size_t index = 0; index < count; ++index)
    {
        struct d_wiki_section* const section = &_wiki->sections[index];
        const size_t                 next    = ((index + 1u) < count)
                                                   ? _wiki->sections[index + 1u]
                                                         .start
                                                   : _wiki->length;

        section->own_end = next;
        section->end     = _wiki->length;

        // the lead ends at the first heading
        if (index == 0u)
        {
            section->end = next;

            continue;
        }

        // the first after it at its level or above
        for (size_t after = index + 1u; after < count; ++after)
        {
            // the next section of its level or higher ends it
            if (_wiki->sections[after].level <= section->level)
            {
                section->end = _wiki->sections[after].start;

                break;
            }
        }

        // the last before it at a level above that reaches it holds it
        for (size_t before = index - 1u; before > 0u; --before)
        {
            // the nearest before it of a higher level that holds it
            if ( (_wiki->sections[before].level < section->level) &&
                 (_wiki->sections[before].end >= section->end) )
            {
                section->parent = before;

                break;
            }
        }
    }

    return true;
}


/*
d_internal_marker
  Reads a comment that is a region's marker: "dawk:begin NAME" or
"dawk:end NAME", alone in the comment.  Gives 1 for a begin, 2 for an end,
and 0 for a comment that is neither, with the name's place.
*/
static int
d_internal_marker(
    const struct d_wiki*       _wiki,
    const struct d_wiki_block* _block,
    size_t*                    _out_name,
    size_t*                    _out_length
)
{
    const char* const text = _wiki->text;
    size_t            at   = _block->start;
    size_t            end  = _block->end;
    int               kind = 0;

    // the comment's opening
    if ( ((at + 4u) > end) ||
         (strncmp(&text[at], "<!--", 4u) != 0) )
    {
        return 0;
    }

    at += 4u;

    // its close, the last `-->` before the end of its lines
    while ( (end > at) &&
            ( (text[end - 1u] == '\n') || (text[end - 1u] == '\r') ||
              (text[end - 1u] == ' ')  || (text[end - 1u] == '\t') ) )
    {
        end--;
    }

    // a comment ends with `-->`
    if ( ((end - at) < 3u) ||
         (strncmp(&text[end - 3u], "-->", 3u) != 0) )
    {
        return 0;
    }

    end -= 3u;

    // space before what it says
    while ( (at < end) &&
            ( (text[at] == ' ') || (text[at] == '\t') ) )
    {
        at++;
    }

    // what it says
    if ( ((end - at) > strlen(D_INTERNAL_BEGIN)) &&
         (strncmp(&text[at], D_INTERNAL_BEGIN,
                  strlen(D_INTERNAL_BEGIN)) == 0) )
    {
        kind = 1;
        at  += strlen(D_INTERNAL_BEGIN);
    }
    else if ( ((end - at) > strlen(D_INTERNAL_END)) &&
              (strncmp(&text[at], D_INTERNAL_END,
                       strlen(D_INTERNAL_END)) == 0) )
    {
        kind = 2;
        at  += strlen(D_INTERNAL_END);
    }
    else
    {
        return 0;
    }

    // space before the name, at least one
    if ( (text[at] != ' ') &&
         (text[at] != '\t') )
    {
        return 0;
    }

    // past the space after `dawk:begin` or `dawk:end`
    while ( (at < end) &&
            ( (text[at] == ' ') || (text[at] == '\t') ) )
    {
        at++;
    }

    *_out_name = at;

    // the name runs to the next space
    while ( (at < end) &&
            (text[at] != ' ') &&
            (text[at] != '\t') &&
            (text[at] != '\n') &&
            (text[at] != '\r') )
    {
        at++;
    }

    *_out_length = at - *_out_name;

    // nothing but space after it
    while ( (at < end) &&
            ( (text[at] == ' ') || (text[at] == '\t') ) )
    {
        at++;
    }

    return ( (at == end) && (*_out_length > 0u) ) ? kind : 0;
}


/*
d_internal_add_region
  Keeps a region, from its begin marker's block to its end marker's.
*/
static bool
d_internal_add_region(
    struct d_wiki*             _wiki,
    const struct d_wiki_block* _begin,
    const struct d_wiki_block* _end,
    size_t                     _name,
    size_t                     _length
)
{
    char* const name = d_wiki_copy(&_wiki->text[_name], _length);

    // room for one more
    if ( (!name) ||
         (!d_internal_grow((void**)&_wiki->regions, &_wiki->region_capacity,
                           _wiki->region_count,
                           sizeof(struct d_wiki_region))) )
    {
        free(name);

        return false;
    }

    struct d_wiki_region* const region = &_wiki->regions[_wiki->region_count];

    region->name    = name;
    region->begin   = _wiki->line_starts[_begin->line - 1u];
    region->content = _begin->end;
    region->close   = _wiki->line_starts[_end->line - 1u];
    region->end     = _end->end;
    region->line    = _begin->line;
    _wiki->region_count++;

    return true;
}


/*
d_internal_find_regions
  The regions, from the markers that stand at the top level: each begin
closed by the end of its name, with nothing begun between.
*/
static bool
d_internal_find_regions(
    struct d_wiki* _wiki
)
{
    const struct d_wiki_block* open        = NULL;
    size_t                     open_name   = 0;
    size_t                     open_length = 0;
    char                       why[160];

    // each comment at the top level, in order
    for (size_t index = 0; index < _wiki->block_count; ++index)
    {
        const struct d_wiki_block* const block = &_wiki->blocks[index];
        size_t                           name  = 0;
        size_t                           span  = 0;

        // a comment at the page's top only
        if ( (block->kind != D_WIKI_COMMENT) ||
             (block->parent != D_WIKI_NONE) )
        {
            continue;
        }

        const int kind = d_internal_marker(_wiki, block, &name, &span);

        // a begin: one already open is left open, and this one ignored
        if (kind == 1)
        {
            // a region begun within another
            if (open)
            {
                (void)snprintf(why, sizeof(why),
                               "region %.*s begins inside region %.*s",
                               (int)span, &_wiki->text[name],
                               (int)open_length, &_wiki->text[open_name]);

                // the problem kept, or memory ran out
                if (!d_wiki_add_problem(_wiki, block->start, why))
                {
                    return false;
                }

                continue;
            }

            open        = block;
            open_name   = name;
            open_length = span;

            continue;
        }

        // an end closes the region of its name
        if (kind == 2)
        {
            // an end with no begin, or of another name
            if ( (!open) ||
                 (span != open_length) ||
                 (strncmp(&_wiki->text[name], &_wiki->text[open_name],
                          span) != 0) )
            {
                (void)snprintf(why, sizeof(why),
                               "region %.*s ends where it has not begun",
                               (int)span, &_wiki->text[name]);

                // the problem kept, or memory ran out
                if (!d_wiki_add_problem(_wiki, block->start, why))
                {
                    return false;
                }

                continue;
            }

            char* const named = d_wiki_copy(&_wiki->text[name], span);

            // memory that ran out
            if (!named)
            {
                return false;
            }

            // a region whose name another has is a problem, and kept
            if (d_wiki_find_region(_wiki, named) != D_WIKI_NONE)
            {
                (void)snprintf(why, sizeof(why),
                               "region %s is not the first of its name",
                               named);

                // the problem kept, or memory ran out
                if (!d_wiki_add_problem(_wiki, open->start, why))
                {
                    free(named);

                    return false;
                }
            }

            free(named);

            // the region kept, or memory ran out
            if (!d_internal_add_region(_wiki, open, block, open_name,
                                       open_length))
            {
                return false;
            }

            open = NULL;
        }
    }

    // a region left open
    if (open)
    {
        (void)snprintf(why, sizeof(why), "region %.*s does not end",
                       (int)open_length, &_wiki->text[open_name]);

        return d_wiki_add_problem(_wiki, open->start, why);
    }

    return true;
}


/*
d_internal_order_problems
  Orders problems by line, keeping the order of those on one line: an
insertion sort, as a page has few.
*/
static void
d_internal_order_problems(
    struct d_wiki* _wiki
)
{
    // each problem moved back past those on later lines
    for (size_t index = 1u; index < _wiki->problem_count; ++index)
    {
        const struct d_wiki_problem moving = _wiki->problems[index];
        size_t                      at     = index;

        // those after it moved up, to its place
        while ( (at > 0u) &&
                (_wiki->problems[at - 1u].line > moving.line) )
        {
            _wiki->problems[at] = _wiki->problems[at - 1u];
            at--;
        }

        _wiki->problems[at] = moving;
    }

    return;
}


bool
d_wiki_read(
    struct d_wiki*      _out,
    const char*         _text,
    size_t              _length,
    enum d_wiki_dialect _dialect,
    char*               _problem,
    size_t              _capacity
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    // parameter validation first
    if (!_out)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    _out->text     = (_text) ? _text : "";
    _out->length   = (_text) ? _length : 0u;
    _out->dialect  = _dialect;
    _out->redirect = D_WIKI_NONE;

    // a dialect with no driver cannot be read
    if (!driver)
    {
        (void)snprintf(_problem, _capacity,
                       "no driver reads the dialect %s",
                       d_wiki_dialect_name(_dialect));

        return false;
    }

    _out->bom = (_out->length >= 3u) &&
                ((unsigned char)_out->text[0] == 0xEFu) &&
                ((unsigned char)_out->text[1] == 0xBBu) &&
                ((unsigned char)_out->text[2] == 0xBFu);

    // the lines, the driver's records, then the reader's
    if ( (!d_internal_find_lines(_out)) ||
         (!driver->read(_out)) ||
         (!d_internal_order_links(_out)) ||
         (!d_internal_order_templates(_out)) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    d_internal_order_headings(_out);
    d_internal_place(_out);
    d_internal_place_links(_out);

    // the records made from the blocks and the headings
    if ( (!d_internal_name_anchors(_out, driver)) ||
         (!d_internal_make_sections(_out)) ||
         (!d_internal_find_regions(_out)) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    d_internal_order_problems(_out);

    return true;
}


void
d_wiki_release(
    struct d_wiki* _wiki
)
{
    // releasing nothing is allowed
    if (!_wiki)
    {
        return;
    }

    // each record's own text
    for (size_t index = 0; index < _wiki->heading_count; ++index)
    {
        free(_wiki->headings[index].title);
        free(_wiki->headings[index].anchor);
    }

    // each link's texts
    for (size_t index = 0; index < _wiki->link_count; ++index)
    {
        free(_wiki->links[index].target);
        free(_wiki->links[index].page);
        free(_wiki->links[index].anchor);
        free(_wiki->links[index].label);
    }

    // each template's name
    for (size_t index = 0; index < _wiki->template_count; ++index)
    {
        free(_wiki->templates[index].name);
    }

    // each argument's key
    for (size_t index = 0; index < _wiki->argument_count; ++index)
    {
        free(_wiki->arguments[index].key);
    }

    // each region's name
    for (size_t index = 0; index < _wiki->region_count; ++index)
    {
        free(_wiki->regions[index].name);
    }

    // each problem's text
    for (size_t index = 0; index < _wiki->problem_count; ++index)
    {
        free(_wiki->problems[index].what);
    }

    free(_wiki->blocks);
    free(_wiki->segments);
    free(_wiki->headings);
    free(_wiki->sections);
    free(_wiki->links);
    free(_wiki->templates);
    free(_wiki->arguments);
    free(_wiki->regions);
    free(_wiki->problems);
    free(_wiki->line_starts);
    memset(_wiki, 0, sizeof(*_wiki));
    _wiki->redirect = D_WIKI_NONE;

    return;
}


//==============================================================================
// 6.  FINDING
//==============================================================================


bool
d_wiki_content(
    const struct d_wiki*  _wiki,
    size_t                _block,
    struct d_wiki_buffer* _out
)
{
    // a block not there has no content
    if (_block >= _wiki->block_count)
    {
        return false;
    }

    const struct d_wiki_block* const block = &_wiki->blocks[_block];

    // each segment, padded, a line feed between
    for (size_t index = 0; index < block->segment_count; ++index)
    {
        const struct d_wiki_segment* const segment =
            &_wiki->segments[block->first_segment + index];

        // a line feed between lines
        if (index > 0u)
        {
            (void)d_wiki_buffer_add_char(_out, '\n');
        }

        // the spaces a tab left
        for (size_t pad = 0; pad < segment->pad; ++pad)
        {
            (void)d_wiki_buffer_add_char(_out, ' ');
        }

        (void)d_wiki_buffer_add(_out, &_wiki->text[segment->start],
                                segment->end - segment->start);
    }

    // an empty content is still a text
    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


size_t
d_wiki_find_region(
    const struct d_wiki* _wiki,
    const char*          _name
)
{
    // the first of the name
    for (size_t index = 0; index < _wiki->region_count; ++index)
    {
        // the region of that name
        if (strcmp(_wiki->regions[index].name, _name) == 0)
        {
            return index;
        }
    }

    return D_WIKI_NONE;
}


size_t
d_wiki_find_definition(
    const struct d_wiki* _wiki,
    const char*          _label,
    size_t               _length
)
{
    struct d_wiki_buffer wanted = { NULL, 0u, 0u, false };
    size_t               found  = D_WIKI_NONE;

    // the label as labels compare
    if (!d_wiki_normalize_label(&wanted, _label, _length))
    {
        d_wiki_buffer_free(&wanted);

        return D_WIKI_NONE;
    }

    // the first definition of it
    for (size_t index = 0; ( (found == D_WIKI_NONE) &&
                             (index < _wiki->link_count) ); ++index)
    {
        const struct d_wiki_link* const link = &_wiki->links[index];
        struct d_wiki_buffer            have = { NULL, 0u, 0u, false };

        // a definition, with a label written
        if ( (link->form != D_WIKI_FORM_DEFINITION) ||
             (link->label_start == D_WIKI_NONE) )
        {
            continue;
        }

        // the label, as labels compare
        if ( (d_wiki_normalize_label(&have, &_wiki->text[link->label_start],
                                     link->label_end - link->label_start)) &&
             (have.length == wanted.length) &&
             ( (have.length == 0u) ||
               (memcmp(have.text, wanted.text, have.length) == 0) ) )
        {
            found = index;
        }

        d_wiki_buffer_free(&have);
    }

    d_wiki_buffer_free(&wanted);

    return found;
}


//==============================================================================
// 7.  NAMES
//==============================================================================


/*
d_internal_base_name
  Where a path's last part begins, after its last `/` or `\`.
*/
static const char*
d_internal_base_name(
    const char* _path
)
{
    const char* base = _path;

    // past every separator
    for (const char* at = _path; *at != '\0'; ++at)
    {
        // the last separator, `/` or `\`
        if ( (*at == '/') ||
             (*at == '\\') )
        {
            base = at + 1;
        }
    }

    return base;
}


char*
d_wiki_page_name(
    const char*         _path,
    enum d_wiki_dialect _dialect
)
{
    const char* const   base    = d_internal_base_name((_path) ? _path : "");
    enum d_wiki_dialect named   = D_WIKI_OTHER;
    size_t              length  = strlen(base);

    char*               name    = NULL;

    // a page's extension is not its name's; it names the dialect, unless
    // one is given
    length -= d_internal_extension_of(base, length, &named);
    named   = (_dialect != D_WIKI_OTHER) ? _dialect : named;
    name    = d_wiki_copy(base, length);

    // memory that ran out
    if (!name)
    {
        return NULL;
    }

    // a GitHub wiki's `-` between words is a space; MediaWiki's `_` too
    for (size_t at = 0; at < length; ++at)
    {
        // a space written as the dialect writes it in a file's name
        if ( (name[at] == '-') ||
             ( (named == D_WIKI_MEDIAWIKI) && (name[at] == '_') ) )
        {
            name[at] = ' ';
        }
    }

    return name;
}


char*
d_wiki_page_file(
    const char*         _name,
    enum d_wiki_dialect _dialect
)
{
    const char* const    extension = (_dialect == D_WIKI_MEDIAWIKI)
                                         ? ".mediawiki" : ".md";
    struct d_wiki_buffer file      = { NULL, 0u, 0u, false };

    // each byte, or `-` for one a file's name cannot hold
    for (const char* at = (_name) ? _name : ""; *at != '\0'; ++at)
    {
        (void)d_wiki_buffer_add_char(&file,
                                     (strchr(" /\\:*?\"<>|", *at) != NULL)
                                         ? '-' : *at);
    }

    (void)d_wiki_buffer_add_text(&file, extension);

    return d_wiki_buffer_take(&file);
}


/*
d_wiki_page_key
  A page's name as names compare: trimmed, its runs of space, `_` and `-`
-- a file names a page with dashes for its spaces -- one space, and in
lower case, all of it or its first letter.
*/
bool
d_wiki_page_key(
    struct d_wiki_buffer* _out,
    const char*           _name,
    enum d_wiki_dialect   _dialect
)
{
    const char* at    = _name;
    bool        space = false;
    bool        first = true;

    // MediaWiki's leading `:` links to what would be put in place
    while ( (_dialect == D_WIKI_MEDIAWIKI) &&
            ( (*at == ':') || (*at == ' ') ) )
    {
        at++;
    }

    // each character
    while (*at != '\0')
    {
        size_t         bytes     = 0;
        const uint32_t character = d_wiki_utf8(at, strlen(at), &bytes);
        const bool     between   = (d_wiki_is_space(character)) ||
                                   (character == '_') ||
                                   (character == '-');

        at += bytes;

        // a run of space is one space, between words
        if (between)
        {
            space = true;

            continue;
        }

        // a space between words, none at the start
        if ( (space) &&
             (_out->length > 0u) )
        {
            (void)d_wiki_buffer_add_char(_out, ' ');
        }

        space = false;
        (void)d_wiki_add_utf8(_out,
                              ( (first) || (_dialect != D_WIKI_MEDIAWIKI) )
                                  ? d_wiki_lower(character) : character);
        first = false;
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


bool
d_wiki_same_page(
    const char*         _left,
    const char*         _right,
    enum d_wiki_dialect _dialect
)
{
    struct d_wiki_buffer left  = { NULL, 0u, 0u, false };
    struct d_wiki_buffer right = { NULL, 0u, 0u, false };
    bool                 same  = false;

    // both as names compare
    if ( (d_wiki_page_key(&left, (_left) ? _left : "", _dialect)) &&
         (d_wiki_page_key(&right, (_right) ? _right : "", _dialect)) )
    {
        same = (strcmp(left.text, right.text) == 0);
    }

    d_wiki_buffer_free(&left);
    d_wiki_buffer_free(&right);

    return same;
}


bool
d_wiki_anchor(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _title
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) && (driver->anchor(_out, (_title) ? _title : ""));
}


/*
d_wiki_anchors
  The anchors headings of titles are given, one to a line, on a page that
has them in that order: each title's, made unique among those before it
as the dialect makes them.
*/
bool
d_wiki_anchors(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _titles
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);
    const char*                       at     = (_titles) ? _titles : "";
    struct d_wiki                     page;
    size_t                            count  = 0;
    bool                              ok     = (driver != NULL);

    memset(&page, 0, sizeof(page));

    // a heading for each line
    for (const char* line = at; ( (ok) && (*line != '\0') ); ++count)
    {
        const char* const ending = strchr(line, '\n');

        line = (ending) ? (ending + 1) : (line + strlen(line));
    }

    page.headings      = calloc(count + 1u, sizeof(struct d_wiki_heading));
    page.heading_count = count;
    ok                 = (ok) && (page.headings != NULL);

    // each line's title
    for (size_t index = 0; ( (ok) && (index < count) ); ++index)
    {
        const char* const ending = strchr(at, '\n');
        const size_t      length = (ending) ? (size_t)(ending - at)
                                            : strlen(at);

        page.headings[index].title = d_wiki_copy(at, length);
        ok                         = (page.headings[index].title != NULL);
        at                        += length + ((ending) ? 1u : 0u);
    }

    ok = (ok) && (d_internal_name_anchors(&page, driver));

    // the anchors, one to a line
    for (size_t index = 0; ( (ok) && (index < count) ); ++index)
    {
        (void)d_wiki_buffer_add_text(_out, page.headings[index].anchor);
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    // the titles and anchors go
    for (size_t index = 0; ( (page.headings) && (index < count) ); ++index)
    {
        free(page.headings[index].title);
        free(page.headings[index].anchor);
    }

    free(page.headings);
    (void)d_wiki_buffer_add(_out, "", 0u);

    return (ok) && (!_out->failed);
}


//==============================================================================
// 8.  WRITING
//==============================================================================


bool
d_wiki_write_heading(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    size_t                _level,
    const char*           _title
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    // a level from 1 to the deepest
    if ( (!driver) ||
         (_level < 1u) ||
         (_level > D_WIKI_MOST_LEVEL) )
    {
        return false;
    }

    return driver->heading(_out, _level, (_title) ? _title : "");
}


bool
d_wiki_write_page_link(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _page,
    const char*           _anchor,
    const char*           _label
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->page_link(_out, (_page) ? _page : "",
                              ( (_anchor) && (_anchor[0] != '\0') )
                                  ? _anchor : NULL,
                              ( (_label) && (_label[0] != '\0') )
                                  ? _label : NULL));
}


bool
d_wiki_write_url_link(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _url,
    const char*           _label
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->url_link(_out, (_url) ? _url : "",
                             ( (_label) && (_label[0] != '\0') )
                                 ? _label : NULL));
}


bool
d_wiki_write_image(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _source,
    const char*           _text
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->image(_out, (_source) ? _source : "",
                          (_text) ? _text : ""));
}


bool
d_wiki_write_code(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _code,
    const char*           _language
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->code(_out, (_code) ? _code : "",
                         ( (_language) && (_language[0] != '\0') )
                             ? _language : NULL));
}


bool
d_wiki_write_inline_code(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _code
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->inline_code(_out, (_code) ? _code : ""));
}


bool
d_wiki_write_item(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _text,
    size_t                _depth,
    bool                  _ordered
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (_depth >= 1u) &&
           (driver->item(_out, (_text) ? _text : "", _depth, _ordered));
}


bool
d_wiki_write_table(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _rows
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->table(_out, (_rows) ? _rows : ""));
}


bool
d_wiki_write_escape(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _text
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_dialect);

    return (driver) &&
           (driver->escape(_out, (_text) ? _text : ""));
}


bool
d_wiki_write_comment(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _text
)
{
    const char* at = (_text) ? _text : "";

    // either dialect's comment is HTML's
    if (!d_wiki_driver_of(_dialect))
    {
        return false;
    }

    (void)d_wiki_buffer_add_text(_out, "<!-- ");

    // each byte, a `-` after another kept apart, and `>` after `-`
    for (; *at != '\0'; ++at)
    {
        (void)d_wiki_buffer_add_char(_out, *at);

        // `--` and `-->` broken, as a comment may hold neither
        if ( (*at == '-') &&
             ( (at[1] == '-') || (at[1] == '>') || (at[1] == '\0') ) )
        {
            (void)d_wiki_buffer_add_char(_out, ' ');
        }
    }

    (void)d_wiki_buffer_add_text(_out, " -->");

    return !_out->failed;
}


bool
d_wiki_write_region(
    struct d_wiki_buffer* _out,
    enum d_wiki_dialect   _dialect,
    const char*           _name,
    const char*           _text
)
{
    const char* const text = (_text) ? _text : "";

    // a name is one word, with nothing a comment cannot hold
    if ( (!d_wiki_driver_of(_dialect)) ||
         (!_name) ||
         (_name[0] == '\0') ||
         (strpbrk(_name, " \t\r\n>") != NULL) ||
         (strstr(_name, "--") != NULL) ||
         (_name[strlen(_name) - 1u] == '-') )
    {
        return false;
    }

    (void)d_wiki_buffer_add_text(_out, "<!-- " D_INTERNAL_BEGIN " ");
    (void)d_wiki_buffer_add_text(_out, _name);
    (void)d_wiki_buffer_add_text(_out, " -->\n");
    (void)d_wiki_buffer_add_text(_out, text);

    // the end marker on a line of its own
    if ( (text[0] != '\0') &&
         (text[strlen(text) - 1u] != '\n') )
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    (void)d_wiki_buffer_add_text(_out, "<!-- " D_INTERNAL_END " ");
    (void)d_wiki_buffer_add_text(_out, _name);
    (void)d_wiki_buffer_add_text(_out, " -->\n");

    return !_out->failed;
}


bool
d_wiki_write_toc(
    struct d_wiki_buffer* _out,
    const struct d_wiki*  _wiki,
    size_t                _least,
    size_t                _most
)
{
    const struct d_wiki_driver* const driver = d_wiki_driver_of(_wiki->dialect);
    size_t                            last   = 0;

    // a list needs a dialect that writes one
    if (!driver)
    {
        return false;
    }

    // each heading at the top within the levels
    for (size_t index = 0; index < _wiki->heading_count; ++index)
    {
        const struct d_wiki_heading* const heading = &_wiki->headings[index];
        struct d_wiki_buffer               title   = { NULL, 0u, 0u, false };
        struct d_wiki_buffer               link    = { NULL, 0u, 0u, false };

        // a heading at the top, of a level asked for
        if ( (!heading->top) ||
             (heading->level < _least) ||
             (heading->level > _most) )
        {
            continue;
        }

        // its depth in the list, from the least level asked for
        size_t depth = heading->level - _least + 1u;

        // a level deeper than the last by more than one is one deeper
        if (depth > (last + 1u))
        {
            depth = last + 1u;
        }

        last = depth;

        const bool ok = (driver->escape(&title, heading->title)) &&
                        (driver->page_link(&link, "", heading->anchor,
                                           title.text)) &&
                        (driver->item(_out, link.text, depth, false));

        d_wiki_buffer_free(&title);
        d_wiki_buffer_free(&link);

        // memory that ran out
        if (!ok)
        {
            return false;
        }
    }

    return !_out->failed;
}
