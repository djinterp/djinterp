/*******************************************************************************
* djinterp [tools]                                                       dline.c
*
* Definitions for the non-inline declarations in dline.h.
*   One pass decodes each line as UTF-8 and classifies every code point as
* it goes; the whole-file facts need a pass of their own first, because the
* `text` node's attributes must all be written before its first `line`
* child takes any.  Decoding follows the Unicode Standard's Table 3-7, so an
* overlong form, a surrogate or a code point past U+10FFFF is `malformed`,
* and one bad byte costs one character, which keeps later columns honest.
*
*
* path:      /src/djinterp/tools/dawk/dline.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dline.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // FILE, fopen, fread, fclose, snprintf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcmp, memset
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dss.h"              // D_DSS_NO_INDEX
#include "../../../../inc/djinterp/tools/dawk/ext/dext_source.h"  // d_ext_source_generated
#include "./dline_width.h"                                         // width tables


// D_INTERNAL_DLINE_NUMBER_MAX
//   constant: room for a decimal attribute value, terminator included.
#define D_INTERNAL_DLINE_NUMBER_MAX 24

// d_internal_line_facts
//   struct: what one line was found to hold, gathered before its node is
// written, since the node's attributes must be contiguous.
struct d_internal_line_facts
{
    size_t  length;
    bool    blank;
    bool    tab;
    bool    trailing_space;
    bool    cr;
    bool    control;
    bool    zero_width;
    bool    wide;
    bool    malformed;
};


/**
 * @brief Reports whether a code point lies in one of a table's ranges.
 *
 * @param[in] _table  sorted, disjoint `{first, last}` ranges.
 * @param[in] _count  how many.
 * @param[in] _cp     the code point.
 * @return `true` if some range holds it, `false` otherwise.
 */
static bool
d_internal_in_table(
    const uint32_t (*_table)[2],
    size_t         _count,
    uint32_t       _cp
)
{
    size_t low  = 0u;
    size_t high = _count;

    // bisect for the last range beginning at or before the code point
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        if (_table[middle][0] <= _cp)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low > 0u) && (_cp <= _table[low - 1u][1]) );
}


/**
 * @brief Decodes one UTF-8 sequence, per the Unicode Standard's Table 3-7.
 *
 * @param[in]  _text    the bytes.
 * @param[in]  _length  how many remain, at least one.
 * @param[out] _cp      the code point, or the first byte when malformed.
 * @return the bytes consumed, or 0 when the sequence is malformed, in which
 *         case the caller consumes one byte.
 */
static size_t
d_internal_decode(
    const unsigned char* _text,
    size_t               _length,
    uint32_t*            _cp
)
{
    const unsigned char lead = _text[0];

    *_cp = lead;

    if (lead < 0x80u)
    {
        return 1u;
    }

    size_t        need = 0u;
    unsigned char low  = 0x80u;
    unsigned char high = 0xBFu;

    // the lead fixes the length and the range of the second byte
    if ( (lead >= 0xC2u) && (lead <= 0xDFu) )
    {
        need = 1u;
    }
    else if ( (lead >= 0xE0u) && (lead <= 0xEFu) )
    {
        need = 2u;
        low  = (lead == 0xE0u) ? 0xA0u : 0x80u;
        high = (lead == 0xEDu) ? 0x9Fu : 0xBFu;
    }
    else if ( (lead >= 0xF0u) && (lead <= 0xF4u) )
    {
        need = 3u;
        low  = (lead == 0xF0u) ? 0x90u : 0x80u;
        high = (lead == 0xF4u) ? 0x8Fu : 0xBFu;
    }

    if ( (need == 0u) || (_length <= need) )
    {
        return 0u;
    }

    uint32_t cp = lead & (0x3Fu >> need);

    for (size_t at = 1u; at <= need; ++at)
    {
        const unsigned char next  = _text[at];
        const unsigned char floor = (at == 1u) ? low : 0x80u;
        const unsigned char roof  = (at == 1u) ? high : 0xBFu;

        // a continuation outside its range ends the sequence here
        if ( (next < floor) || (next > roof) )
        {
            return 0u;
        }

        cp = (cp << 6) | (uint32_t)(next & 0x3Fu);
    }

    *_cp = cp;

    return need + 1u;
}


/**
 * @brief Classifies one decoded code point into a line's facts.
 *
 * @param[in,out] _facts  the line's facts.
 * @param[in]     _cp     the code point.
 */
static void
d_internal_classify(
    struct d_internal_line_facts* _facts,
    uint32_t                      _cp
)
{
    const size_t wide_count =
        sizeof(d_internal_dline_wide) / sizeof(d_internal_dline_wide[0]);
    const size_t zero_count =
        sizeof(d_internal_dline_zero_width) /
        sizeof(d_internal_dline_zero_width[0]);

    // a space or a tab leaves a line blank; anything else does not
    if ( (_cp != ' ') && (_cp != '\t') )
    {
        _facts->blank = false;
    }

    if (_cp == '\t')
    {
        _facts->tab = true;
    }
    else if ( (_cp < 0x20u) || ( (_cp >= 0x7Fu) && (_cp < 0xA0u) ) )
    {
        _facts->control = true;
    }
    else if (_cp >= 0xA0u)
    {
        if (d_internal_in_table(d_internal_dline_wide, wide_count, _cp))
        {
            _facts->wide = true;
        }
        else if (d_internal_in_table(d_internal_dline_zero_width,
                                     zero_count,
                                     _cp))
        {
            _facts->zero_width = true;
        }
    }

    return;
}


/**
 * @brief Measures one line's content, its terminator already removed.
 *
 * @param[in]  _text    the line's first byte.
 * @param[in]  _length  its bytes, the terminator excluded.
 * @param[out] _facts   receives what the line holds; `cr` is left alone.
 */
static void
d_internal_measure(
    const unsigned char*          _text,
    size_t                        _length,
    struct d_internal_line_facts* _facts
)
{
    size_t at = 0u;

    _facts->length = 0u;
    _facts->blank  = true;

    while (at < _length)
    {
        uint32_t     cp   = 0u;
        const size_t used = d_internal_decode(_text + at, _length - at, &cp);

        // one bad byte is one character, so later columns stay honest
        if (used == 0u)
        {
            _facts->malformed = true;
            _facts->blank     = false;
            ++at;
        }
        else
        {
            d_internal_classify(_facts, cp);
            at += used;
        }

        ++_facts->length;
    }

    _facts->trailing_space = ( (_length > 0u) &&
                               ( (_text[_length - 1u] == ' ') ||
                                 (_text[_length - 1u] == '\t') ) );

    return;
}


/**
 * @brief Sets a numeric attribute.
 *
 * @param[in,out] _tree   the tree.
 * @param[in]     _node   the node.
 * @param[in]     _name   the attribute's name.
 * @param[in]     _value  its value.
 */
static void
d_internal_number(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _name,
    size_t              _value
)
{
    char text[D_INTERNAL_DLINE_NUMBER_MAX];

    (void)snprintf(text, sizeof(text), "%zu", _value);
    (void)d_node_set_attribute(_tree, _node, _name, text);

    return;
}


/**
 * @brief Writes the `text` node and its whole-file facts.
 *
 * @param[in,out] _tree    the tree.
 * @param[in]     _file    the file node.
 * @param[in]     _text    the file's bytes.
 * @param[in]     _length  their count.
 * @return the `text` node, or `D_DSS_NO_INDEX`.
 */
static uint32_t
d_internal_emit_text(
    struct d_node_tree* _tree,
    uint32_t            _file,
    const char*         _text,
    size_t              _length
)
{
    size_t lf   = 0u;
    size_t crlf = 0u;

    // every terminator, and whether a carriage return precedes it
    for (size_t at = 0u; at < _length; ++at)
    {
        if (_text[at] == '\n')
        {
            if ( (at > 0u) && (_text[at - 1u] == '\r') )
            {
                ++crlf;
            }
            else
            {
                ++lf;
            }
        }
    }

    size_t finals = 0u;
    size_t end    = _length;

    // the terminators the file ends with, a CR of CRLF passed over
    while ( (end > 0u) && (_text[end - 1u] == '\n') )
    {
        ++finals;
        --end;

        if ( (end > 0u) && (_text[end - 1u] == '\r') )
        {
            --end;
        }
    }

    const char* eol = "none";

    if ( (lf > 0u) && (crlf > 0u) )
    {
        eol = "mixed";
    }
    else if (crlf > 0u)
    {
        eol = "crlf";
    }
    else if (lf > 0u)
    {
        eol = "lf";
    }

    const uint32_t node = d_node_add(_tree, _file, "text");

    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    if ( (_length >= 3u) &&
         (memcmp(_text, "\xEF\xBB\xBF", 3u) == 0) )
    {
        (void)d_node_set_attribute(_tree, node, "bom", NULL);
    }

    (void)d_node_set_attribute(_tree, node, "eol", eol);
    d_internal_number(_tree, node, "final-newlines", finals);

    if (d_ext_source_generated(_text, _length))
    {
        (void)d_node_set_attribute(_tree, node, "generated", NULL);
    }

    struct d_node* const text = d_node_at(_tree, node);

    text->line         = 1u;
    text->start_column = 1u;
    text->end_column   = 1u;
    text->width        = 0u;

    return node;
}


/**
 * @brief Writes one `line` node from its facts.
 *
 * @param[in,out] _tree    the tree.
 * @param[in]     _parent  the `text` node.
 * @param[in]     _number  the line's number, from 1.
 * @param[in]     _facts   what the line holds.
 * @param[in]     _bytes   the line's content, the terminator excluded.
 * @param[in]     _size    its byte count.
 * @return the node, or `D_DSS_NO_INDEX`.
 */
static uint32_t
d_internal_emit_line(
    struct d_node_tree*                 _tree,
    uint32_t                            _parent,
    uint32_t                            _number,
    const struct d_internal_line_facts* _facts,
    const char*                         _bytes,
    size_t                              _size
)
{
    static const char* const names[] =
    {
        "blank", "tab", "trailing-space", "cr",
        "control", "zero-width", "wide", "malformed"
    };

    const bool holds[] =
    {
        _facts->blank,   _facts->tab,        _facts->trailing_space,
        _facts->cr,      _facts->control,    _facts->zero_width,
        _facts->wide,    _facts->malformed
    };

    const uint32_t node = d_node_add(_tree, _parent, "line");

    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    d_internal_number(_tree, node, "length", _facts->length);

    for (size_t at = 0u; at < (sizeof(names) / sizeof(names[0])); ++at)
    {
        if (holds[at])
        {
            (void)d_node_set_attribute(_tree, node, names[at], NULL);
        }
    }

    struct d_node* const line = d_node_at(_tree, node);

    line->line         = _number;
    line->start_column = 1u;
    line->width        = (uint32_t)_facts->length;
    line->end_column   = (_facts->length > 0u) ? (uint32_t)_facts->length
                                               : 1u;

    (void)d_node_set_text(_tree, node, _bytes, _size);

    return node;
}


/*
d_line_build_text
  Two passes: the whole-file facts first, because the `text` node's attributes
must be written before any child's, then the lines.  A byte-order mark is the
text node's fact alone; line 1 is measured from after it, as the lexer reads.
*/
uint32_t
d_line_build_text(
    struct d_node_tree* _tree,
    uint32_t            _file,
    const char*         _text,
    size_t              _length
)
{
    // parameter validation first
    if ( (!_tree)                       ||
         (_file == D_DSS_NO_INDEX)      ||
         ( (!_text) && (_length > 0u) ) )
    {
        return D_DSS_NO_INDEX;
    }

    const uint32_t text = d_internal_emit_text(_tree, _file, _text, _length);

    if (text == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    const bool bom    = ( (_length >= 3u) &&
                          (memcmp(_text, "\xEF\xBB\xBF", 3u) == 0) );
    size_t     start  = bom ? 3u : 0u;
    uint32_t   number = 0u;

    // each line runs to its terminator, or to the end of the text
    while (start < _length)
    {
        size_t end = start;

        while ( (end < _length) && (_text[end] != '\n') )
        {
            ++end;
        }

        struct d_internal_line_facts facts;

        memset(&facts, 0, sizeof(facts));

        size_t content = end;

        // a CR that precedes a terminator is the terminator's, not the line's
        if ( (end < _length) && (content > start) &&
             (_text[content - 1u] == '\r') )
        {
            facts.cr = true;
            --content;
        }

        d_internal_measure((const unsigned char*)_text + start,
                           content - start,
                           &facts);

        ++number;

        if (d_internal_emit_line(_tree, text, number, &facts,
                                 _text + start, content - start)
            == D_DSS_NO_INDEX)
        {
            break;
        }

        start = end + 1u;
    }

    return text;
}


/*
d_line_build
  Reads the whole file rather than a line at a time: a fixed line buffer would
split a long line in two and number every later line wrong, and long lines are
exactly what this extension exists to measure.
*/
uint32_t
d_line_build(
    struct d_node_tree* _tree,
    uint32_t            _file,
    const char*         _path
)
{
    // parameter validation first
    if ( (!_tree) || (!_path) || (_file == D_DSS_NO_INDEX) )
    {
        return D_DSS_NO_INDEX;
    }

    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return D_DSS_NO_INDEX;
    }

    const long size = ( (fseek(handle, 0, SEEK_END) == 0) ? ftell(handle)
                                                          : -1L );

    // a size that cannot be told is a file that cannot be read
    if ( (size < 0L) || (fseek(handle, 0, SEEK_SET) != 0) )
    {
        (void)fclose(handle);

        return D_DSS_NO_INDEX;
    }

    char* const bytes = malloc((size_t)size + 1u);

    if (!bytes)
    {
        (void)fclose(handle);

        return D_DSS_NO_INDEX;
    }

    const size_t read = fread(bytes, 1u, (size_t)size, handle);

    (void)fclose(handle);

    const uint32_t text = d_line_build_text(_tree, _file, bytes, read);

    free(bytes);

    return text;
}
