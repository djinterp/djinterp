/*******************************************************************************
* djinterp [c][tools]                                                drenumber.c
*
* Headings made decimal, and the tables of contents that agree with them:
* definitions for drenumber.h. Replaces tools/renumber_headings.py, which was
* lost, from the pairs of tests/dawk/renumber and the style guide's
* "Header Structure, Numbering, and Tables of Contents".
*   A file is read as lines, its line ends kept; its section boxes, the
* subsections under them and its table of contents are found; anything that
* would leave it numbered two ways leaves it whole; then the lines are
* written again, converted, and if the header has no table, one is written
* from what was converted.
*
*
* path:      /src/djinterp/tools/dawk/drenumber.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/drenumber.h"  // corresponding header
// std
#include <ctype.h>   // isalpha, isdigit, islower, isupper, toupper
#include <stdarg.h>  // va_list
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memcmp, memcpy, strcmp, strlen, strrchr
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_open.h"               // streams
#include "../../../../inc/djinterp/tools/dawk/dfs.h"                    // walk
#include "../../../../inc/djinterp/tools/dawk/ext/check/dext_source.h"  // C


//==============================================================================
// 1.  LINES
//==============================================================================

// dj_internal_view
//   struct: one line of a text, its end left out; it points into the text.
struct dj_internal_view
{
    const char* text;
    size_t      length;
};

// dj_internal_views
//   struct: a text's lines, in order.
struct dj_internal_views
{
    struct dj_internal_view* items;
    size_t                   count;
};

// dj_internal_lines
//   struct: lines being written, each allocated.
struct dj_internal_lines
{
    char**  items;
    size_t* lengths;
    size_t  count;
    size_t  capacity;
};

/*
d_internal_views_split
  The lines between each `_nl`, and the text after the last, which a text
ending in a line end has empty: joined with `_nl` they give the text back.
*/
static
bool d_internal_views_split(
    const char*               _text,
    size_t                    _length,
    const char*               _nl,
    struct dj_internal_views* _views
)
{
    const size_t width = strlen(_nl);
    size_t       count = 1u;

    for (size_t at = 0u; at + width <= _length; ++at)
    {
        count += (memcmp(_text + at, _nl, width) == 0) ? 1u : 0u;
    }

    _views->items = malloc(count * sizeof(struct dj_internal_view));
    _views->count = 0u;

    if (!_views->items)
    {
        return false;
    }

    size_t start = 0u;

    for (size_t at = 0u; at + width <= _length; ++at)
    {
        if (memcmp(_text + at, _nl, width) == 0)
        {
            _views->items[_views->count].text     = _text + start;
            _views->items[_views->count].length   = at - start;
            ++_views->count;
            start = at + width;
            at   += width - 1u;
        }
    }

    _views->items[_views->count].text   = _text + start;
    _views->items[_views->count].length = _length - start;
    ++_views->count;

    return true;
}

/*
d_internal_lines_push
  Appends a copy of `_length` bytes as a line.
*/
static
bool d_internal_lines_push(
    struct dj_internal_lines* _lines,
    const char*               _text,
    size_t                    _length
)
{
    if (_lines->count == _lines->capacity)
    {
        const size_t more  = (_lines->capacity) ? _lines->capacity * 2u : 64u;
        char** const items = realloc(_lines->items, more * sizeof(char*));

        if (!items)
        {
            return false;
        }

        _lines->items = items;

        size_t* const lengths = realloc(_lines->lengths, more * sizeof(size_t));

        if (!lengths)
        {
            return false;
        }

        _lines->lengths  = lengths;
        _lines->capacity = more;
    }

    char* const copy = malloc(_length + 1u);

    if (!copy)
    {
        return false;
    }

    memcpy(copy, _text, _length);
    copy[_length] = '\0';
    _lines->items[_lines->count]   = copy;
    _lines->lengths[_lines->count] = _length;
    ++_lines->count;

    return true;
}

/*
d_internal_lines_printf
  Appends a line written as printf writes it.
*/
static
bool d_internal_lines_printf(
    struct dj_internal_lines* _lines,
    const char*               _format,
    ...
)
{
    va_list args;
    char    small[512];

    va_start(args, _format);
    const int needed = vsnprintf(small, sizeof(small), _format, args);
    va_end(args);

    if (needed < 0)
    {
        return false;
    }

    if ((size_t)needed < sizeof(small))
    {
        return d_internal_lines_push(_lines, small, (size_t)needed);
    }

    char* const large = malloc((size_t)needed + 1u);

    if (!large)
    {
        return false;
    }

    va_start(args, _format);
    (void)vsnprintf(large, (size_t)needed + 1u, _format, args);
    va_end(args);

    const bool pushed = d_internal_lines_push(_lines, large, (size_t)needed);

    free(large);

    return pushed;
}

/*
d_internal_lines_free
  Every line, and the arrays.
*/
static
void d_internal_lines_free(
    struct dj_internal_lines* _lines
)
{
    for (size_t at = 0u; at < _lines->count; ++at)
    {
        free(_lines->items[at]);
    }

    free(_lines->items);
    free(_lines->lengths);
    memset(_lines, 0, sizeof(*_lines));

    return;
}

/*
d_internal_lines_views
  Views of written lines, to be read as a text's are.
*/
static
bool d_internal_lines_views(
    const struct dj_internal_lines* _lines,
    struct dj_internal_views*       _views
)
{
    _views->count = _lines->count;
    _views->items = malloc(((_lines->count) ? _lines->count : 1u) *
                           sizeof(struct dj_internal_view));

    if (!_views->items)
    {
        return false;
    }

    for (size_t at = 0u; at < _lines->count; ++at)
    {
        _views->items[at].text   = _lines->items[at];
        _views->items[at].length = _lines->lengths[at];
    }

    return true;
}


//==============================================================================
// 2.  READING A LINE
//==============================================================================

/*
d_internal_space
  A blank or a tab.
*/
static
bool d_internal_space(
    char _c
)
{
    return ( (_c == ' ') || (_c == '\t') );
}

/*
d_internal_blank
  Whether a line holds nothing but blanks.
*/
static
bool d_internal_blank(
    const struct dj_internal_view* _line
)
{
    for (size_t at = 0u; at < _line->length; ++at)
    {
        if (!d_internal_space(_line->text[at]))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_trimmed
  Whether a line, its blanks at either end left out, is `_word`.
*/
static
bool d_internal_trimmed(
    const struct dj_internal_view* _line,
    const char*                    _word
)
{
    size_t start = 0u;
    size_t end   = _line->length;

    while ( (start < end) && (d_internal_space(_line->text[start])) )
    {
        ++start;
    }

    while ( (end > start) && (d_internal_space(_line->text[end - 1u])) )
    {
        --end;
    }

    return ( (end - start == strlen(_word)) &&
             (memcmp(_line->text + start, _word, end - start) == 0) );
}

/*
d_internal_rule
  Whether a line is a rule: `//`, perhaps a space, and one or more of
`_fill` (`=` or `-`), or with `_fill` `/`, three or more slashes alone. Its
indentation goes to `_indent`.
*/
static
bool d_internal_rule(
    const struct dj_internal_view* _line,
    char                           _fill,
    size_t*                        _indent
)
{
    const char* const text  = _line->text;
    const size_t      n     = _line->length;
    size_t            at    = 0u;
    size_t            count = 0u;

    while ( (at < n) && (d_internal_space(text[at])) )
    {
        ++at;
    }

    *_indent = at;

    if (_fill != '/')
    {
        if ( (at + 2u > n) || (text[at] != '/') || (text[at + 1u] != '/') )
        {
            return false;
        }

        at += 2u;
        at += ( (at < n) && (text[at] == ' ') ) ? 1u : 0u;
    }

    while ( (at < n) && (text[at] == _fill) )
    {
        ++at;
        ++count;
    }

    while ( (at < n) && (d_internal_space(text[at])) )
    {
        ++at;
    }

    return ( (at == n) && (count >= ((_fill == '/') ? 3u : 1u)) );
}

/*
d_internal_box_rule
  A rule a section's box is made of: of `=`, or of slashes.
*/
static
bool d_internal_box_rule(
    const struct dj_internal_view* _line,
    size_t*                        _indent
)
{
    return ( (d_internal_rule(_line, '=', _indent)) ||
             (d_internal_rule(_line, '/', _indent)) );
}

/*
d_internal_comment_line
  Whether a line, after its indentation, begins `//`.
*/
static
bool d_internal_comment_line(
    const struct dj_internal_view* _line
)
{
    size_t at = 0u;

    while ( (at < _line->length) && (d_internal_space(_line->text[at])) )
    {
        ++at;
    }

    return ( (at + 2u <= _line->length) &&
             (_line->text[at] == '/') &&
             (_line->text[at + 1u] == '/') );
}

/*
d_internal_roman_digit
  One Roman digit's value, in either case; 0 for anything else.
*/
static
unsigned d_internal_roman_digit(
    char _c
)
{
    switch (toupper((unsigned char)_c))
    {
        case 'I': return 1u;
        case 'V': return 5u;
        case 'X': return 10u;
        case 'L': return 50u;
        case 'C': return 100u;
        case 'D': return 500u;
        case 'M': return 1000u;
        default:  return 0u;
    }
}

/*
d_internal_roman
  A numeral's value when it is a Roman numeral written as one is written, in
one case; 0 otherwise. The value is read, then written again: a numeral that
doesn't come back as it was (`IIII`, `VX`, `CMC`) is not one.
*/
static
unsigned d_internal_roman(
    const char* _text,
    size_t      _length
)
{
    static const char* const symbols[] = { "M", "CM", "D", "CD", "C", "XC",
                                           "L", "XL", "X", "IX", "V", "IV",
                                           "I" };
    static const unsigned    values[]  = { 1000u, 900u, 500u, 400u, 100u,
                                           90u, 50u, 40u, 10u, 9u, 5u, 4u,
                                           1u };
    bool     upper = true;
    bool     lower = true;
    long     value = 0;
    char     again[32];
    size_t   used  = 0u;

    for (size_t at = 0u; at < _length; ++at)
    {
        const unsigned digit = d_internal_roman_digit(_text[at]);
        const unsigned next  = (at + 1u < _length)
                                   ? d_internal_roman_digit(_text[at + 1u])
                                   : 0u;

        upper = (upper) && (isupper((unsigned char)_text[at]));
        lower = (lower) && (islower((unsigned char)_text[at]));

        if (digit == 0u)
        {
            return 0u;
        }

        value = (next > digit) ? value - (long)digit : value + (long)digit;
    }

    if ( (_length == 0u) || (_length > 15u) || (value <= 0) ||
         (value >= 4000) || ( (!upper) && (!lower) ) )
    {
        return 0u;
    }

    for (size_t s = 0u, rest = (size_t)value; s < 13u; ++s)
    {
        while (rest >= values[s])
        {
            for (const char* c = symbols[s]; *c; ++c)
            {
                again[used++] = *c;
            }

            rest -= values[s];
        }
    }

    if (used != _length)
    {
        return 0u;
    }

    for (size_t at = 0u; at < _length; ++at)
    {
        if (toupper((unsigned char)_text[at]) != again[at])
        {
            return 0u;
        }
    }

    return (unsigned)value;
}

/*
d_internal_pad
  Writes `_number` and blanks after it to `_width` characters, one blank at
least, into `_buffer`.
*/
static
const char* d_internal_pad(
    const char* _number,
    size_t      _width,
    char*       _buffer,
    size_t      _size
)
{
    const size_t length = strlen(_number);
    const size_t blanks = (length < _width) ? _width - length : 1u;

    (void)snprintf(_buffer, _size, "%s%*s", _number, (int)blanks, "");

    return _buffer;
}


//==============================================================================
// 3.  HEADINGS
//==============================================================================

// D_INTERNAL_RENUMBER_*
//   enum: what numbers a section's box: nothing, a Roman numeral, a decimal.
enum
{
    D_INTERNAL_RENUMBER_UNTITLED = 0,
    D_INTERNAL_RENUMBER_ROMAN,
    D_INTERNAL_RENUMBER_DECIMAL
};

// dj_internal_section
//   struct: a section's box, its first and last rule, how it is numbered and
// its name.
struct dj_internal_section
{
    size_t      start;
    size_t      end;
    int         kind;
    unsigned    value;
    const char* name;
    size_t      name_length;
};

// dj_internal_sub
//   struct: a subsection's heading of one line: where, its marker and name,
// whether a rule of `-` stands just above it (a box of rules), the section
// it is under, and the value it converts to (0: left as written).
struct dj_internal_sub
{
    size_t      line;
    const char* marker;
    size_t      marker_length;
    const char* name;
    size_t      name_length;
    bool        boxed;
    size_t      section;
    unsigned    value;
};

// dj_internal_doc
//   struct: what a text is read into.
struct dj_internal_doc
{
    struct dj_internal_section* sections;
    size_t                      section_count;
    struct dj_internal_sub*     subs;
    size_t                      sub_count;
    const char*                 why;
};

/*
d_internal_heading
  Reads the text inside a box: a marker of letters or digits, a period,
perhaps a second part right after it (`I.b`), blanks, and the name, its
blanks at the end left out. False when the text has no such marker.
*/
static
bool d_internal_heading(
    const char*  _text,
    size_t       _length,
    size_t*      _marker,
    bool*        _second,
    const char** _name,
    size_t*      _name_length
)
{
    size_t at = 0u;

    while ( (at < _length) && (isalnum((unsigned char)_text[at])) )
    {
        ++at;
    }

    const bool letters = (at > 0u) && (isalpha((unsigned char)_text[0]));
    bool       same    = (at > 0u);

    for (size_t k = 0u; (same) && (k < at); ++k)
    {
        same = (letters) ? (isalpha((unsigned char)_text[k]) != 0)
                         : (isdigit((unsigned char)_text[k]) != 0);
    }

    if ( (!same) || (at >= _length) || (_text[at] != '.') )
    {
        return false;
    }

    *_marker = at++;
    *_second = ( (at < _length) && (!d_internal_space(_text[at])) );

    while ( (at < _length) && (!d_internal_space(_text[at])) )
    {
        ++at;
    }

    if ( (at >= _length) || (!d_internal_space(_text[at])) )
    {
        return false;
    }

    while ( (at < _length) && (d_internal_space(_text[at])) )
    {
        ++at;
    }

    size_t end = _length;

    while ( (end > at) && (d_internal_space(_text[end - 1u])) )
    {
        --end;
    }

    *_name        = _text + at;
    *_name_length = end - at;

    return (end > at);
}

/*
d_internal_box_text
  The text inside a box's one line: after `///` and before a closing `///`,
or after `//`, its blanks at either end left out.
*/
static
void d_internal_box_text(
    const struct dj_internal_view* _line,
    const char**                   _text,
    size_t*                        _length
)
{
    const char* text = _line->text;
    size_t      n    = _line->length;

    while ( (n > 0u) && (d_internal_space(*text)) )
    {
        ++text;
        --n;
    }

    while ( (n > 0u) && (d_internal_space(text[n - 1u])) )
    {
        --n;
    }

    if ( (n >= 3u) && (memcmp(text, "///", 3u) == 0) )
    {
        text += 3;
        n    -= 3u;

        if ( (n >= 3u) && (memcmp(text + n - 3u, "///", 3u) == 0) )
        {
            n -= 3u;
        }
    }
    else if ( (n >= 2u) && (memcmp(text, "//", 2u) == 0) )
    {
        text += 2;
        n    -= 2u;
    }

    while ( (n > 0u) && (d_internal_space(*text)) )
    {
        ++text;
        --n;
    }

    while ( (n > 0u) && (d_internal_space(text[n - 1u])) )
    {
        --n;
    }

    *_text   = text;
    *_length = n;

    return;
}

/*
d_internal_section_add
  Appends one; false when memory runs out.
*/
static
bool d_internal_section_add(
    struct dj_internal_doc*           _doc,
    const struct dj_internal_section* _section
)
{
    const size_t size = sizeof(struct dj_internal_section);
    struct dj_internal_section* const grown =
        realloc(_doc->sections, (_doc->section_count + 1u) * size);

    if (!grown)
    {
        return false;
    }

    _doc->sections                        = grown;
    _doc->sections[_doc->section_count++] = *_section;

    return true;
}

/*
d_internal_classify
  A box's one line: numbered in Roman (in capitals), in decimal, or not at
all; a numeral with a second part leaves the file whole.
*/
static
bool d_internal_classify(
    struct dj_internal_doc*        _doc,
    const struct dj_internal_view* _inside,
    struct dj_internal_section*    _section
)
{
    const char* text   = NULL;
    size_t      length = 0u;
    size_t      marker = 0u;
    bool        second = false;

    d_internal_box_text(_inside, &text, &length);
    _section->kind = D_INTERNAL_RENUMBER_UNTITLED;

    if (!d_internal_heading(text, length, &marker, &second,
                            &_section->name, &_section->name_length))
    {
        return true;
    }

    if (second)
    {
        _doc->why = "a numeral has a second part, which the guide has no "
                    "form for";

        return false;
    }

    if (isdigit((unsigned char)text[0]))
    {
        _section->kind  = D_INTERNAL_RENUMBER_DECIMAL;
        _section->value = (unsigned)strtoul(text, NULL, 10);
    }
    else if ( (isupper((unsigned char)text[0])) &&
              (d_internal_roman(text, marker) != 0u) )
    {
        _section->kind  = D_INTERNAL_RENUMBER_ROMAN;
        _section->value = d_internal_roman(text, marker);
    }

    return true;
}

/*
d_internal_sections
  Finds every box: a rule, lines of comment, a rule. One whose rule is
indented, or that holds more than one line, leaves the file whole.
*/
static
bool d_internal_sections(
    const struct dj_internal_views* _lines,
    struct dj_internal_doc*         _doc
)
{
    size_t at = 0u;

    while (at < _lines->count)
    {
        size_t indent = 0u;
        size_t other  = 0u;
        size_t end    = at + 1u;

        if (!d_internal_box_rule(&_lines->items[at], &indent))
        {
            ++at;

            continue;
        }

        while ( (end < _lines->count) &&
                (!d_internal_box_rule(&_lines->items[end], &other)) &&
                (d_internal_comment_line(&_lines->items[end])) )
        {
            ++end;
        }

        if ( (end >= _lines->count) || (end == at + 1u) ||
             (!d_internal_box_rule(&_lines->items[end], &other)) )
        {
            ++at;

            continue;
        }

        if (indent > 0u)
        {
            _doc->why = "a boxed heading is indented, and the guide's box "
                        "starts in the first column";

            return false;
        }

        if (end - at > 2u)
        {
            _doc->why = "a heading takes several lines in its box";

            return false;
        }

        struct dj_internal_section section;

        memset(&section, 0, sizeof(section));
        section.start = at;
        section.end   = end;

        if ( (!d_internal_classify(_doc, &_lines->items[at + 1u], &section)) ||
             (!d_internal_section_add(_doc, &section)) )
        {
            return false;
        }

        at = end + 1u;
    }

    return true;
}

/*
d_internal_sub_line
  Reads a subsection's heading: `//`, a blank, a marker of letters and a
period or of numbers with periods between (`1.1`), two blanks or more, and
the name.
*/
static
bool d_internal_sub_line(
    const struct dj_internal_view* _line,
    struct dj_internal_sub*        _sub
)
{
    const char* const text = _line->text;
    const size_t      n    = _line->length;
    size_t            at   = 3u;

    if ( (n < 4u) || (text[0] != '/') || (text[1] != '/') ||
         (!d_internal_space(text[2])) )
    {
        return false;
    }

    if (isalpha((unsigned char)text[at]))
    {
        while ( (at < n) && (isalpha((unsigned char)text[at])) )
        {
            ++at;
        }

        if ( (at >= n) || (text[at] != '.') )
        {
            return false;
        }

        _sub->marker_length = at - 3u;
        ++at;
    }
    else
    {
        size_t dots = 0u;

        while ( (at < n) &&
                ( (isdigit((unsigned char)text[at])) ||
                  ( (text[at] == '.') && (at + 1u < n) &&
                    (isdigit((unsigned char)text[at + 1u])) &&
                    (isdigit((unsigned char)text[at - 1u])) ) ) )
        {
            dots += (text[at] == '.') ? 1u : 0u;
            ++at;
        }

        if ( (dots == 0u) || (at == 3u) )
        {
            return false;
        }

        _sub->marker_length = at - 3u;
    }

    const size_t gap = at;

    while ( (at < n) && (d_internal_space(text[at])) )
    {
        ++at;
    }

    size_t end = n;

    while ( (end > at) && (d_internal_space(text[end - 1u])) )
    {
        --end;
    }

    _sub->marker      = text + 3;
    _sub->name        = text + at;
    _sub->name_length = end - at;

    return ( (at - gap >= 2u) && (end > at) );
}

/*
d_internal_subs
  Every subsection heading under each box, up to the next.
*/
static
bool d_internal_subs(
    const struct dj_internal_views* _lines,
    struct dj_internal_doc*         _doc
)
{
    for (size_t s = 0u; s < _doc->section_count; ++s)
    {
        const size_t stop = (s + 1u < _doc->section_count)
                                ? _doc->sections[s + 1u].start
                                : _lines->count;

        for (size_t k = _doc->sections[s].end + 1u; k < stop; ++k)
        {
            struct dj_internal_sub sub;
            size_t                 indent = 0u;

            memset(&sub, 0, sizeof(sub));

            if (!d_internal_sub_line(&_lines->items[k], &sub))
            {
                continue;
            }

            sub.line    = k;
            sub.section = s;
            sub.boxed   = ( (k > 0u) &&
                            (d_internal_rule(&_lines->items[k - 1u], '-',
                                             &indent)) );

            const size_t size = sizeof(struct dj_internal_sub);
            struct dj_internal_sub* const grown =
                realloc(_doc->subs, (_doc->sub_count + 1u) * size);

            if (!grown)
            {
                return false;
            }

            _doc->subs                    = grown;
            _doc->subs[_doc->sub_count++] = sub;
        }
    }

    return true;
}

// D_INTERNAL_GROUP_*
//   enum: how a list of siblings is counted.
enum
{
    D_INTERNAL_GROUP_LETTERS = 0,
    D_INTERNAL_GROUP_ROMAN,
    D_INTERNAL_GROUP_DECIMAL
};

/*
d_internal_group_kind
  Siblings are counted in decimal when every marker is decimal; in Roman
when every one is a Roman numeral, all in one case, and they rise; in
letters otherwise - a letter that is no numeral among them (`A`, `B`)
makes `C` and `I` letters too.
*/
static
int d_internal_group_kind(
    const char* const* _markers,
    const size_t*      _lengths,
    size_t             _count
)
{
    bool     decimal = true;
    bool     upper   = true;
    bool     lower   = true;
    unsigned last    = 0u;

    for (size_t at = 0u; at < _count; ++at)
    {
        decimal = (decimal) && (isdigit((unsigned char)_markers[at][0]));
    }

    if (decimal)
    {
        return D_INTERNAL_GROUP_DECIMAL;
    }

    for (size_t at = 0u; at < _count; ++at)
    {
        const unsigned value = d_internal_roman(_markers[at], _lengths[at]);

        upper = (upper) && (isupper((unsigned char)_markers[at][0]));
        lower = (lower) && (islower((unsigned char)_markers[at][0]));

        if ( (value == 0u) || (value <= last) )
        {
            return D_INTERNAL_GROUP_LETTERS;
        }

        last = value;
    }

    return ( (_count > 0u) && ( (upper) || (lower) ) )
               ? D_INTERNAL_GROUP_ROMAN
               : D_INTERNAL_GROUP_LETTERS;
}

/*
d_internal_sub_values
  Gives the subsections of each numbered section their values, when they are
counted in Roman; one of those in a box of rules leaves the file whole.
*/
static
bool d_internal_sub_values(
    struct dj_internal_doc* _doc
)
{
    const char* markers[256];
    size_t      lengths[256];

    for (size_t s = 0u; s < _doc->section_count; ++s)
    {
        size_t count = 0u;
        bool   boxed = false;

        if (_doc->sections[s].kind == D_INTERNAL_RENUMBER_UNTITLED)
        {
            continue;
        }

        for (size_t k = 0u; (k < _doc->sub_count) && (count < 256u); ++k)
        {
            if (_doc->subs[k].section == s)
            {
                markers[count]   = _doc->subs[k].marker;
                lengths[count++] = _doc->subs[k].marker_length;
                boxed            = (boxed) || (_doc->subs[k].boxed);
            }
        }

        if ( (count == 0u) ||
             (d_internal_group_kind(markers, lengths, count) !=
              D_INTERNAL_GROUP_ROMAN) )
        {
            continue;
        }

        if (boxed)
        {
            _doc->why = "a Roman subsection stands in a box of rules, which "
                        "the guide has no form for";

            return false;
        }

        for (size_t k = 0u; k < _doc->sub_count; ++k)
        {
            if (_doc->subs[k].section == s)
            {
                _doc->subs[k].value = d_internal_roman(
                                          _doc->subs[k].marker,
                                          _doc->subs[k].marker_length);
            }
        }
    }

    return true;
}

/*
d_internal_doc_read
  Sections, subsections and their values; false with `why` when the file is
left whole, and with no `why` when memory runs out.
*/
static
bool d_internal_doc_read(
    const struct dj_internal_views* _lines,
    struct dj_internal_doc*         _doc
)
{
    memset(_doc, 0, sizeof(*_doc));

    return ( (d_internal_sections(_lines, _doc)) &&
             (d_internal_subs(_lines, _doc)) &&
             (d_internal_sub_values(_doc)) );
}

/*
d_internal_doc_free
  What d_internal_doc_read made.
*/
static
void d_internal_doc_free(
    struct dj_internal_doc* _doc
)
{
    free(_doc->sections);
    free(_doc->subs);
    _doc->sections = NULL;
    _doc->subs     = NULL;

    return;
}


//==============================================================================
// 4.  THE TABLE OF CONTENTS
//==============================================================================

/*
d_internal_toc_find
  The table: a line opening a comment, `TABLE OF CONTENTS` under it, its
rule, entries, and a line closing the comment. False when there is none.
*/
static
bool d_internal_toc_find(
    const struct dj_internal_views* _lines,
    size_t*                         _start,
    size_t*                         _end
)
{
    for (size_t at = 1u; at < _lines->count; ++at)
    {
        if ( (d_internal_trimmed(&_lines->items[at], "TABLE OF CONTENTS")) &&
             (d_internal_trimmed(&_lines->items[at - 1u], "/*")) )
        {
            size_t end = at + 2u;

            while ( (end < _lines->count) &&
                    (!d_internal_trimmed(&_lines->items[end], "*/")) )
            {
                ++end;
            }

            *_start = at - 1u;
            *_end   = end;

            return (end < _lines->count);
        }
    }

    return false;
}

/*
d_internal_toc_entry
  Reads a table's numbered line, from `_from`: blanks (the indentation, none
for a section), a marker of letters or digits, a period, blanks, and the
text. The marker's start and length, and the text's start, go out.
*/
static
bool d_internal_toc_entry(
    const struct dj_internal_view* _line,
    size_t                         _from,
    size_t*                        _marker,
    size_t*                        _marker_length,
    size_t*                        _text
)
{
    const char* const t  = _line->text;
    const size_t      n  = _line->length;
    size_t            at = _from;

    while ( (at < n) && (d_internal_space(t[at])) )
    {
        ++at;
    }

    *_marker = at;

    const bool letters = (at < n) && (isalpha((unsigned char)t[at]));

    while ( (at < n) &&
            ( (letters) ? (isalpha((unsigned char)t[at]) != 0)
                        : (isdigit((unsigned char)t[at]) != 0) ) )
    {
        ++at;
    }

    *_marker_length = at - *_marker;

    if ( (*_marker_length == 0u) || (at >= n) || (t[at] != '.') )
    {
        return false;
    }

    const size_t gap = ++at;

    while ( (at < n) && (d_internal_space(t[at])) )
    {
        ++at;
    }

    *_text = at;

    return ( (at > gap) && (at < n) );
}

/*
d_internal_toc_shifted
  A line moved `_shift` columns: blanks added before it, or as many of its
leading blanks as there are taken away.
*/
static
bool d_internal_toc_shifted(
    struct dj_internal_lines*      _out,
    const struct dj_internal_view* _line,
    long                           _shift,
    size_t*                        _indent_after
)
{
    size_t lead = 0u;

    while ( (lead < _line->length) && (_line->text[lead] == ' ') )
    {
        ++lead;
    }

    if (_shift >= 0)
    {
        *_indent_after = lead + (size_t)_shift;

        return d_internal_lines_printf(_out, "%*s%.*s", (int)_shift, "",
                                       (int)_line->length, _line->text);
    }

    const size_t take = ((size_t)(-_shift) < lead) ? (size_t)(-_shift) : lead;

    *_indent_after = lead - take;

    return d_internal_lines_push(_out, _line->text + take,
                                 _line->length - take);
}

// d_internal_why
//   variable: the words of a reason that carries numbers.
static char d_internal_why[192];

/*
d_internal_name_agrees
  Whether a table's text names a section: the name alone, or the name and
two blanks before whatever the table adds.
*/
static
bool d_internal_name_agrees(
    const char*                       _text,
    size_t                            _length,
    const struct dj_internal_section* _section
)
{
    const size_t n = _section->name_length;

    return ( (_length >= n) &&
             (memcmp(_text, _section->name, n) == 0) &&
             ( (_length == n) ||
               ( (_length >= n + 2u) && (_text[n] == ' ') &&
                 (_text[n + 1u] == ' ') ) ) );
}

/*
d_internal_toc_children
  Whether an entry's children are counted in Roman, and how many there are;
the lines are `_rest`'s.
*/
static
int d_internal_toc_children(
    const struct dj_internal_views* _lines,
    const size_t*                   _rest,
    size_t                          _count,
    size_t*                         _children
)
{
    const char* markers[256];
    size_t      lengths[256];
    size_t      found = 0u;

    for (size_t at = 0u; (at < _count) && (found < 256u); ++at)
    {
        const struct dj_internal_view* const line = &_lines->items[_rest[at]];
        size_t marker = 0u;
        size_t length = 0u;
        size_t text   = 0u;

        if ( (line->length > 0u) && (line->text[0] == ' ') &&
             (d_internal_toc_entry(line, 0u, &marker, &length, &text)) )
        {
            markers[found]   = line->text + marker;
            lengths[found++] = length;
        }
    }

    *_children = found;

    return (found) ? d_internal_group_kind(markers, lengths, found)
                   : D_INTERNAL_GROUP_LETTERS;
}

/*
d_internal_toc_child
  A Roman child, moved with its entry and laid out as the guide's: its new
indentation, its number padded to four columns, its text.
*/
static
bool d_internal_toc_child(
    struct dj_internal_lines*      _out,
    const struct dj_internal_view* _line,
    long                           _shift
)
{
    size_t marker = 0u;
    size_t length = 0u;
    size_t text   = 0u;
    char   number[16];
    char   padded[24];

    (void)d_internal_toc_entry(_line, 0u, &marker, &length, &text);

    const long indent = ((long)marker + _shift > 0) ? (long)marker + _shift
                                                    : 0;

    (void)snprintf(number, sizeof(number), "%u.",
                   d_internal_roman(_line->text + marker, length));

    return d_internal_lines_printf(_out, "%*s%s%.*s", (int)indent, "",
                                   d_internal_pad(number, 4u, padded,
                                                  sizeof(padded)),
                                   (int)(_line->length - text),
                                   _line->text + text);
}

/*
d_internal_toc_section
  One section's entry and the lines under it: the entry checked against the
section, and written with its number; the lines under it moved as far as its
name moved; Roman children laid out again, when the body counts that
section's subsections in Roman too, as many.
*/
static
bool d_internal_toc_section(
    const struct dj_internal_views*   _lines,
    size_t                            _entry,
    const size_t*                     _rest,
    size_t                            _count,
    const struct dj_internal_doc*     _doc,
    size_t                            _s,
    struct dj_internal_lines*         _out,
    const char**                      _why
)
{
    const struct dj_internal_view* const    line    = &_lines->items[_entry];
    const struct dj_internal_section* const section = &_doc->sections[_s];
    size_t   marker = 0u;
    size_t   length = 0u;
    size_t   text   = 0u;
    size_t   subs   = 0u;
    size_t   kids   = 0u;
    char     number[16];
    char     padded[24];

    (void)d_internal_toc_entry(line, 0u, &marker, &length, &text);

    const unsigned value =
        (isdigit((unsigned char)line->text[0]))
            ? (unsigned)strtoul(line->text, NULL, 10)
            : d_internal_roman(line->text, length);

    if ( (value != section->value) ||
         (!d_internal_name_agrees(line->text + text, line->length - text,
                                  section)) )
    {
        (void)snprintf(d_internal_why, sizeof(d_internal_why),
                       "the table of contents has %.*s where the body has "
                       "%.*s", (int)(line->length), line->text,
                       (int)section->name_length, section->name);
        *_why = d_internal_why;

        return false;
    }

    const int kind = d_internal_toc_children(_lines, _rest, _count, &kids);

    for (size_t k = 0u; k < _doc->sub_count; ++k)
    {
        subs += ( (_doc->subs[k].section == _s) &&
                  (_doc->subs[k].value != 0u) ) ? 1u : 0u;
    }

    if ( (kind == D_INTERNAL_GROUP_ROMAN) && (kids != subs) )
    {
        (void)snprintf(d_internal_why, sizeof(d_internal_why),
                       "the table of contents and the body disagree under "
                       "%.*s", (int)section->name_length, section->name);
        *_why = d_internal_why;

        return false;
    }

    (void)snprintf(number, sizeof(number), "%u.", value);
    d_internal_pad(number, 4u, padded, sizeof(padded));

    const long shift = (long)strlen(padded) - (long)text;

    if (!d_internal_lines_printf(_out, "%s%.*s", padded,
                                 (int)(line->length - text),
                                 line->text + text))
    {
        return false;
    }

    for (size_t at = 0u; at < _count; ++at)
    {
        const struct dj_internal_view* const under = &_lines->items[_rest[at]];
        size_t                               indent = 0u;
        size_t                               m      = 0u;
        size_t                               l      = 0u;
        size_t                               t      = 0u;
        const bool child = ( (under->length > 0u) && (under->text[0] == ' ') &&
                             (d_internal_toc_entry(under, 0u, &m, &l, &t)) );
        const bool ok    = ( (child) && (kind == D_INTERNAL_GROUP_ROMAN) )
                               ? d_internal_toc_child(_out, under, shift)
                               : d_internal_toc_shifted(_out, under, shift,
                                                        &indent);

        if (!ok)
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_toc_convert
  The table converted with its body, empty lines taken out; false with
`why` when it disagrees with the body, without when memory runs out.
*/
static
bool d_internal_toc_convert(
    const struct dj_internal_views* _lines,
    size_t                          _start,
    size_t                          _end,
    const struct dj_internal_doc*   _doc,
    struct dj_internal_lines*       _out,
    const char**                    _why
)
{
    size_t* const entries = malloc((_end - _start + 1u) * sizeof(size_t));
    size_t* const rest    = malloc((_end - _start + 1u) * sizeof(size_t));
    size_t        count   = 0u;
    size_t        numbered = 0u;
    bool          ok      = ( (entries) && (rest) );

    for (size_t at = _start + 3u; (ok) && (at < _end); ++at)
    {
        const struct dj_internal_view* const line = &_lines->items[at];
        size_t                               m    = 0u;
        size_t                               l    = 0u;
        size_t                               t    = 0u;

        if (d_internal_blank(line))
        {
            continue;
        }

        if ( (line->text[0] != ' ') &&
             (d_internal_toc_entry(line, 0u, &m, &l, &t)) )
        {
            entries[count++] = at;
        }
        else if (count == 0u)
        {
            *_why = "the table of contents begins with an indented line";
            ok    = false;
        }
    }

    for (size_t s = 0u; s < _doc->section_count; ++s)
    {
        numbered += (_doc->sections[s].kind != D_INTERNAL_RENUMBER_UNTITLED)
                        ? 1u : 0u;
    }

    if ( (ok) && (count != numbered) )
    {
        (void)snprintf(d_internal_why, sizeof(d_internal_why),
                       "the table of contents names %zu sections and the "
                       "body has %zu", count, numbered);
        *_why = d_internal_why;
        ok    = false;
    }

    for (size_t at = _start; (ok) && (at < _start + 3u); ++at)
    {
        ok = d_internal_lines_push(_out, _lines->items[at].text,
                                   _lines->items[at].length);
    }

    for (size_t e = 0u, s = 0u; (ok) && (e < count); ++e, ++s)
    {
        const size_t stop  = (e + 1u < count) ? entries[e + 1u] : _end;
        size_t       under = 0u;

        while (_doc->sections[s].kind == D_INTERNAL_RENUMBER_UNTITLED)
        {
            ++s;
        }

        for (size_t at = entries[e] + 1u; at < stop; ++at)
        {
            if (!d_internal_blank(&_lines->items[at]))
            {
                rest[under++] = at;
            }
        }

        ok = d_internal_toc_section(_lines, entries[e], rest, under, _doc, s,
                                    _out, _why);
    }

    ok = (ok) && (d_internal_lines_push(_out, _lines->items[_end].text,
                                        _lines->items[_end].length));
    free(entries);
    free(rest);

    return ok;
}


//==============================================================================
// 5.  WRITING
//==============================================================================

// D_INTERNAL_RULE_EQUALS, D_INTERNAL_RULE_DASHES
//   constant: the guide's rules, the full eighty columns.
#define D_INTERNAL_RULE_EQUALS                                                \
    "//======================================="                               \
    "======================================="
#define D_INTERNAL_RULE_DASHES                                                \
    "//---------------------------------------"                               \
    "---------------------------------------"

/*
d_internal_write_section
  The guide's box for a numbered section.
*/
static
bool d_internal_write_section(
    struct dj_internal_lines*         _out,
    const struct dj_internal_section* _section
)
{
    char number[16];
    char padded[24];

    (void)snprintf(number, sizeof(number), "%u.", _section->value);

    return ( (d_internal_lines_printf(_out, "%s", D_INTERNAL_RULE_EQUALS)) &&
             (d_internal_lines_printf(_out, "// %s%.*s",
                                      d_internal_pad(number, 4u, padded,
                                                     sizeof(padded)),
                                      (int)_section->name_length,
                                      _section->name)) &&
             (d_internal_lines_printf(_out, "%s", D_INTERNAL_RULE_EQUALS)) );
}

/*
d_internal_write_sub
  A Roman subsection, numbered in decimal, with the guide's rule after it;
the rule it had, and empty lines, are taken out: the guide has none between
the rule and what follows. The line after what was taken out is returned.
*/
static
size_t d_internal_write_sub(
    const struct dj_internal_views* _lines,
    const struct dj_internal_doc*   _doc,
    const struct dj_internal_sub*   _sub,
    struct dj_internal_lines*       _out,
    bool*                           _ok
)
{
    char   number[32];
    char   padded[40];
    size_t indent = 0u;
    size_t next   = _sub->line + 1u;

    (void)snprintf(number, sizeof(number), "%u.%u",
                   _doc->sections[_sub->section].value, _sub->value);
    *_ok = ( (d_internal_lines_printf(_out, "// %s%.*s",
                                      d_internal_pad(number, 7u, padded,
                                                     sizeof(padded)),
                                      (int)_sub->name_length, _sub->name)) &&
             (d_internal_lines_printf(_out, "%s", D_INTERNAL_RULE_DASHES)) );

    if ( (next < _lines->count) &&
         (d_internal_rule(&_lines->items[next], '-', &indent)) )
    {
        ++next;
    }

    while ( (next < _lines->count) &&
            (d_internal_blank(&_lines->items[next])) )
    {
        ++next;
    }

    return next;
}

/*
d_internal_write_body
  Every line again: the table as converted, each numbered section's box as
the guide's, each Roman subsection numbered; the rest as it was.
*/
static
bool d_internal_write_body(
    const struct dj_internal_views* _lines,
    const struct dj_internal_doc*   _doc,
    const struct dj_internal_lines* _toc,
    size_t                          _toc_start,
    size_t                          _toc_end,
    struct dj_internal_lines*       _out
)
{
    size_t section = 0u;
    size_t sub     = 0u;
    size_t k       = 0u;
    bool   ok      = true;

    while ( (ok) && (k < _lines->count) )
    {
        while ( (section < _doc->section_count) &&
                (_doc->sections[section].start < k) )
        {
            ++section;
        }

        while ( (sub < _doc->sub_count) && (_doc->subs[sub].line < k) )
        {
            ++sub;
        }

        if ( (_toc) && (k == _toc_start) )
        {
            for (size_t at = 0u; (ok) && (at < _toc->count); ++at)
            {
                ok = d_internal_lines_push(_out, _toc->items[at],
                                           _toc->lengths[at]);
            }

            k = _toc_end + 1u;
        }
        else if ( (section < _doc->section_count) &&
                  (_doc->sections[section].start == k) &&
                  (_doc->sections[section].kind !=
                   D_INTERNAL_RENUMBER_UNTITLED) )
        {
            ok = d_internal_write_section(_out, &_doc->sections[section]);
            k  = _doc->sections[section].end + 1u;
        }
        else if ( (sub < _doc->sub_count) && (_doc->subs[sub].line == k) &&
                  (_doc->subs[sub].value != 0u) )
        {
            k = d_internal_write_sub(_lines, _doc, &_doc->subs[sub], _out,
                                     &ok);
        }
        else
        {
            ok = d_internal_lines_push(_out, _lines->items[k].text,
                                       _lines->items[k].length);
            ++k;
        }
    }

    return ok;
}


//==============================================================================
// 6.  WRITING A TABLE
//==============================================================================

/*
d_internal_item_line
  Reads an item's marker alone on its line, `// 1.1.1` or `// 1.1.1.1`: its
level (3 or 4) and its last number. False for anything else.
*/
static
bool d_internal_item_line(
    const struct dj_internal_view* _line,
    unsigned*                      _level,
    unsigned*                      _ordinal
)
{
    const char* const t     = _line->text;
    const size_t      n     = _line->length;
    size_t            at    = 3u;
    unsigned          dots  = 0u;
    size_t            last  = 3u;

    if ( (n < 4u) || (t[0] != '/') || (t[1] != '/') ||
         (!d_internal_space(t[2])) || (!isdigit((unsigned char)t[3])) )
    {
        return false;
    }

    while ( (at < n) &&
            ( (isdigit((unsigned char)t[at])) ||
              ( (t[at] == '.') && (at + 1u < n) &&
                (isdigit((unsigned char)t[at + 1u])) ) ) )
    {
        if (t[at] == '.')
        {
            ++dots;
            last = at + 1u;
        }

        ++at;
    }

    while ( (at < n) && (d_internal_space(t[at])) )
    {
        ++at;
    }

    *_level   = dots + 1u;
    *_ordinal = (unsigned)strtoul(t + last, NULL, 10);

    return ( (at == n) && (dots >= 2u) && (dots <= 3u) );
}

/*
d_internal_dots
  How many periods a marker holds.
*/
static
size_t d_internal_dots(
    const char* _marker,
    size_t      _length
)
{
    size_t dots = 0u;

    for (size_t at = 0u; at < _length; ++at)
    {
        dots += (_marker[at] == '.') ? 1u : 0u;
    }

    return dots;
}

/*
d_internal_write_entries
  One section's entries, below its own: its subsections (`// 1.1    name`)
four columns in, its items (`// 1.1.1` with the name on the next line) nine,
and theirs fourteen, each with its own last number.
*/
static
bool d_internal_write_entries(
    const struct dj_internal_views* _lines,
    size_t                          _from,
    size_t                          _stop,
    struct dj_internal_lines*       _toc
)
{
    static const int indents[5] = { 0, 0, 4, 9, 14 };
    bool             ok         = true;

    for (size_t k = _from; (ok) && (k < _stop); ++k)
    {
        const struct dj_internal_view* const line = &_lines->items[k];
        struct dj_internal_sub               sub;
        unsigned                             level   = 0u;
        unsigned                             ordinal = 0u;
        char                                 number[16];
        char                                 padded[24];

        memset(&sub, 0, sizeof(sub));

        if ( (d_internal_sub_line(line, &sub)) &&
             (isdigit((unsigned char)sub.marker[0])) &&
             (d_internal_dots(sub.marker, sub.marker_length) == 1u) )
        {
            const char* const dot = memchr(sub.marker, '.', sub.marker_length);

            (void)snprintf(number, sizeof(number), "%u.",
                           (unsigned)strtoul(dot + 1, NULL, 10));
            ok = d_internal_lines_printf(_toc, "    %s%.*s",
                                         d_internal_pad(number, 4u, padded,
                                                        sizeof(padded)),
                                         (int)sub.name_length, sub.name);

            continue;
        }

        if ( (d_internal_item_line(line, &level, &ordinal)) &&
             (k + 1u < _stop) &&
             (_lines->items[k + 1u].length > 3u) &&
             (memcmp(_lines->items[k + 1u].text, "// ", 3u) == 0) )
        {
            const struct dj_internal_view* const name  = &_lines->items[k + 1u];
            size_t                               start = 3u;
            size_t                               end   = name->length;

            while ( (start < end) && (d_internal_space(name->text[start])) )
            {
                ++start;
            }

            while ( (end > start) && (d_internal_space(name->text[end - 1u])) )
            {
                --end;
            }

            (void)snprintf(number, sizeof(number), "%u.", ordinal);
            ok = d_internal_lines_printf(_toc, "%*s%s%.*s",
                                         indents[level], "",
                                         d_internal_pad(number, 4u, padded,
                                                        sizeof(padded)),
                                         (int)(end - start),
                                         name->text + start);
        }
    }

    return ok;
}

/*
d_internal_underline
  A rule of `-` four columns in, as wide as a name: as many as it has
characters, a character of UTF-8 counted once however many bytes it takes.
*/
static
bool d_internal_underline(
    struct dj_internal_lines* _toc,
    const char*               _name,
    size_t                    _length
)
{
    size_t width = 0u;

    for (size_t at = 0u; at < _length; ++at)
    {
        width += ((((unsigned char)_name[at]) & 0xC0u) != 0x80u) ? 1u : 0u;
    }

    char* const line = malloc(width + 5u);

    if (!line)
    {
        return false;
    }

    memset(line, ' ', 4u);
    memset(line + 4, '-', width);

    const bool pushed = d_internal_lines_push(_toc, line, width + 4u);

    free(line);

    return pushed;
}

/*
d_internal_write_toc
  A table from the converted lines: each section with its rule of `-` under
its name, and its entries.
*/
static
bool d_internal_write_toc(
    const struct dj_internal_views* _lines,
    const struct dj_internal_doc*   _doc,
    struct dj_internal_lines*       _toc
)
{
    bool ok = ( (d_internal_lines_printf(_toc, "/*")) &&
                (d_internal_lines_printf(_toc, "TABLE OF CONTENTS")) &&
                (d_internal_lines_printf(_toc, "=================")) );

    for (size_t s = 0u; (ok) && (s < _doc->section_count); ++s)
    {
        const struct dj_internal_section* const section = &_doc->sections[s];
        const size_t stop = (s + 1u < _doc->section_count)
                                ? _doc->sections[s + 1u].start
                                : _lines->count;
        char         number[16];
        char         padded[24];

        (void)snprintf(number, sizeof(number), "%u.", section->value);
        ok = ( (d_internal_lines_printf(_toc, "%s%.*s",
                                        d_internal_pad(number, 4u, padded,
                                                       sizeof(padded)),
                                        (int)section->name_length,
                                        section->name)) &&
               (d_internal_underline(_toc, section->name,
                                     section->name_length)) &&
               (d_internal_write_entries(_lines, section->end + 1u, stop,
                                         _toc)) );
    }

    return ( (ok) && (d_internal_lines_printf(_toc, "*/")) );
}

/*
d_internal_after_first_comment
  Where a table goes: after the comment a file begins with, and the empty
line after it. False when the file doesn't begin with a comment, or the
comment is not followed by an empty line.
*/
static
bool d_internal_after_first_comment(
    const struct dj_internal_lines* _lines,
    size_t*                         _at
)
{
    size_t k = 0u;

    if ( (_lines->count == 0u) || (_lines->lengths[0] < 2u) ||
         (memcmp(_lines->items[0], "/*", 2u) != 0) )
    {
        return false;
    }

    while ( (k < _lines->count) && (!strstr(_lines->items[k], "*/")) )
    {
        ++k;
    }

    ++k;

    if (k >= _lines->count)
    {
        return false;
    }

    const struct dj_internal_view line = { _lines->items[k],
                                           _lines->lengths[k] };

    *_at = k + 1u;

    return d_internal_blank(&line);
}

/*
d_internal_wants_toc
  Whether a header with no table gets one: numbered sections, every one of
them numbered, each one more than the last. A box with no number would be
left out of the table, and a number skipped would be a section the table
can't name.
*/
static
bool d_internal_wants_toc(
    const struct dj_internal_doc* _doc
)
{
    if (_doc->section_count == 0u)
    {
        return false;
    }

    for (size_t s = 0u; s < _doc->section_count; ++s)
    {
        const unsigned previous = (s > 0u) ? _doc->sections[s - 1u].value
                                           : 0u;

        if ( (_doc->sections[s].kind == D_INTERNAL_RENUMBER_UNTITLED) ||
             ( (s > 0u) && (_doc->sections[s].value != previous + 1u) ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_add_toc
  Reads what was written again, and writes its table after the first
comment, with an empty line after it.
*/
static
bool d_internal_add_toc(
    struct dj_internal_lines* _out
)
{
    struct dj_internal_views written = { NULL, 0u };
    struct dj_internal_doc   doc;
    struct dj_internal_lines toc     = { NULL, NULL, 0u, 0u };
    struct dj_internal_lines whole   = { NULL, NULL, 0u, 0u };
    size_t                   at      = 0u;
    bool                     ok      = d_internal_lines_views(_out, &written);

    memset(&doc, 0, sizeof(doc));

    if ( (!ok) ||
         (!d_internal_after_first_comment(_out, &at)) ||
         (!d_internal_doc_read(&written, &doc)) )
    {
        // a reading that leaves the file whole now writes no table
        ok = (ok) && ( (doc.why != NULL) ||
                       (!d_internal_after_first_comment(_out, &at)) );
        d_internal_doc_free(&doc);
        free(written.items);

        return ok;
    }

    ok = d_internal_write_toc(&written, &doc, &toc);

    for (size_t k = 0u; (ok) && (k < _out->count); ++k)
    {
        if (k == at)
        {
            for (size_t t = 0u; (ok) && (t < toc.count); ++t)
            {
                ok = d_internal_lines_push(&whole, toc.items[t],
                                           toc.lengths[t]);
            }

            ok = (ok) && (d_internal_lines_push(&whole, "", 0u));
        }

        ok = (ok) && (d_internal_lines_push(&whole, _out->items[k],
                                            _out->lengths[k]));
    }

    d_internal_doc_free(&doc);
    free(written.items);
    d_internal_lines_free(&toc);

    if (ok)
    {
        d_internal_lines_free(_out);
        *_out = whole;
    }
    else
    {
        d_internal_lines_free(&whole);
    }

    return ok;
}


//==============================================================================
// 7.  CONVERTING
//==============================================================================

/*
d_internal_join
  The lines joined with `_nl` into one allocation.
*/
static
char* d_internal_join(
    const struct dj_internal_lines* _lines,
    const char*                     _nl,
    size_t*                         _length
)
{
    const size_t width = strlen(_nl);
    size_t       total = 0u;

    for (size_t at = 0u; at < _lines->count; ++at)
    {
        total += _lines->lengths[at] + ((at + 1u < _lines->count) ? width : 0u);
    }

    char* const text = malloc(total + 1u);
    size_t      used = 0u;

    if (!text)
    {
        return NULL;
    }

    for (size_t at = 0u; at < _lines->count; ++at)
    {
        memcpy(text + used, _lines->items[at], _lines->lengths[at]);
        used += _lines->lengths[at];

        if (at + 1u < _lines->count)
        {
            memcpy(text + used, _nl, width);
            used += width;
        }
    }

    text[used] = '\0';
    *_length   = used;

    return text;
}

/*
d_internal_convert
  The conversion of a text split into lines, into `_out`; false with
`_doc->why` when the file is left whole, without when memory runs out.
*/
static
bool d_internal_convert(
    const struct dj_internal_views* _lines,
    bool                            _header,
    struct dj_internal_doc*         _doc,
    struct dj_internal_lines*       _out
)
{
    struct dj_internal_lines toc       = { NULL, NULL, 0u, 0u };
    size_t                   toc_start = 0u;
    size_t                   toc_end   = 0u;
    const bool               has_toc   = d_internal_toc_find(_lines,
                                                             &toc_start,
                                                             &toc_end);
    bool                     ok        = d_internal_doc_read(_lines, _doc);

    if ( (ok) && (has_toc) )
    {
        ok = d_internal_toc_convert(_lines, toc_start, toc_end, _doc, &toc,
                                    &_doc->why);
    }

    ok = (ok) && (d_internal_write_body(_lines, _doc,
                                        (has_toc) ? &toc : NULL,
                                        toc_start, toc_end, _out));

    if ( (ok) && (!has_toc) && (_header) && (d_internal_wants_toc(_doc)) )
    {
        ok = d_internal_add_toc(_out);
    }

    d_internal_lines_free(&toc);

    return ok;
}

/*
d_internal_crlf
  Whether a text has a `\r\n` anywhere.
*/
static
bool d_internal_crlf(
    const char* _text,
    size_t      _length
)
{
    for (size_t at = 0u; at + 1u < _length; ++at)
    {
        if ( (_text[at] == '\r') && (_text[at + 1u] == '\n') )
        {
            return true;
        }
    }

    return false;
}

/*
d_renumber_text
  Splits at the text's own line end, `\r\n` if it has one, converts, and
joins again; a conversion that changes nothing gives no text.
*/
bool
d_renumber_text(
    const char*  _text,
    size_t       _length,
    bool         _header,
    char**       _out,
    size_t*      _length_out,
    const char** _why
)
{
    const char* const        nl    = (d_internal_crlf(_text, _length))
                                         ? "\r\n" : "\n";
    struct dj_internal_views lines = { NULL, 0u };
    struct dj_internal_lines out   = { NULL, NULL, 0u, 0u };
    struct dj_internal_doc   doc;
    bool                     ok    = d_internal_views_split(_text, _length,
                                                            nl, &lines);

    memset(&doc, 0, sizeof(doc));
    *_out        = NULL;
    *_length_out = 0u;
    *_why        = NULL;

    if ( (ok) && (!d_internal_convert(&lines, _header, &doc, &out)) )
    {
        *_why = doc.why;
        ok    = (doc.why != NULL);
    }
    else if (ok)
    {
        size_t      length = 0u;
        char* const joined = d_internal_join(&out, nl, &length);

        ok = (joined != NULL);

        if ( (joined) &&
             ( (length != _length) || (memcmp(joined, _text, length) != 0) ) )
        {
            *_out        = joined;
            *_length_out = length;
        }
        else
        {
            free(joined);
        }
    }

    d_internal_doc_free(&doc);
    d_internal_lines_free(&out);
    free(lines.items);

    return ok;
}


//==============================================================================
// 8.  THE TREE
//==============================================================================

// dj_internal_walk
//   struct: a walk's tallies, and the lines it will print after its summary.
struct dj_internal_walk
{
    size_t                   converted;
    struct dj_internal_lines said;
    FILE*                    report;
    bool                     ok;
};

/*
d_internal_by_name
  qsort's order for names: as strcmp has them.
*/
static
int d_internal_by_name(
    const void* _a,
    const void* _b
)
{
    return strcmp(*(const char* const*)_a, *(const char* const*)_b);
}

/*
d_internal_header
  Whether a path names a header, which a table of contents may be written
into.
*/
static
bool d_internal_header(
    const char* _path
)
{
    const char* const dot = strrchr(_path, '.');

    return ( (dot) &&
             ( (strcmp(dot, ".h") == 0) || (strcmp(dot, ".hpp") == 0) ||
               (strcmp(dot, ".hh") == 0) || (strcmp(dot, ".hxx") == 0) ) );
}

/*
d_internal_slurp
  The whole file, with a terminator after it, or NULL.
*/
static
char* d_internal_slurp(
    const char* _path,
    size_t*     _length
)
{
    FILE* const in = d_file_open_stream(_path, "rb");

    if (!in)
    {
        return NULL;
    }

    size_t used     = 0u;
    size_t capacity = 4096u;
    char*  text     = malloc(capacity + 1u);

    while (text)
    {
        if (used == capacity)
        {
            char* const grown = realloc(text, capacity * 2u + 1u);

            if (!grown)
            {
                free(text);
                text = NULL;

                break;
            }

            text      = grown;
            capacity *= 2u;
        }

        const size_t got = fread(text + used, 1u, capacity - used, in);

        used += got;

        if (got == 0u)
        {
            break;
        }
    }

    (void)d_file_close_stream(in);

    if (text)
    {
        text[used] = '\0';
        *_length   = used;
    }

    return text;
}

/*
d_internal_renumber_file
  One file: read, converted, written back when anything changed, and a line
said of it when it was converted or left whole.
*/
static
void d_internal_renumber_file(
    struct dj_internal_walk* _walk,
    const char*              _path
)
{
    size_t      length    = 0u;
    char* const text      = d_internal_slurp(_path, &length);
    char*       converted = NULL;
    size_t      written   = 0u;
    const char* why       = NULL;

    if (!text)
    {
        (void)fprintf(_walk->report, "%s: cannot read\n", _path);
        _walk->ok = false;

        return;
    }

    if (d_ext_source_generated(text, length))
    {
        free(text);

        return;
    }

    if (!d_renumber_text(text, length, d_internal_header(_path),
                         &converted, &written, &why))
    {
        (void)fprintf(_walk->report, "%s: out of memory\n", _path);
        _walk->ok = false;
    }
    else if (why)
    {
        _walk->ok = (_walk->ok) &&
                    (d_internal_lines_printf(&_walk->said,
                                             "left whole: %s: %s", _path,
                                             why));
    }
    else if (converted)
    {
        FILE* const out = d_file_open_stream(_path, "wb");

        if ( (!out) || (fwrite(converted, 1u, written, out) != written) )
        {
            (void)fprintf(_walk->report, "%s: cannot write\n", _path);
            _walk->ok = false;
        }

        if ( (out) && (d_file_close_stream(out) != 0) )
        {
            (void)fprintf(_walk->report, "%s: cannot write\n", _path);
            _walk->ok = false;
        }

        ++_walk->converted;
        _walk->ok = (_walk->ok) &&
                    (d_internal_lines_printf(&_walk->said, "converted: %s",
                                             _path));
    }

    free(converted);
    free(text);

    return;
}

/*
d_internal_renumber_directory
  Every C and C++ file under a directory, its names in order.
*/
static
void d_internal_renumber_directory(
    struct dj_internal_walk* _walk,
    const char*              _directory
)
{
    char** names = NULL;
    size_t count = 0u;

    if (!d_fs_names(_directory, &names, &count))
    {
        return;
    }

    if (count > 1u)
    {
        qsort(names, count, sizeof(char*), d_internal_by_name);
    }

    for (size_t at = 0u; at < count; ++at)
    {
        const size_t length = strlen(_directory) + strlen(names[at]) + 2u;
        char* const  path   = malloc(length);

        if (!path)
        {
            _walk->ok = false;

            break;
        }

        (void)snprintf(path, length, "%s/%s", _directory, names[at]);

        if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            d_internal_renumber_directory(_walk, path);
        }
        else if (d_ext_source_accepts(path))
        {
            d_internal_renumber_file(_walk, path);
        }

        free(path);
    }

    d_fs_names_free(names, count);

    return;
}

/*
d_renumber_tree
  `inc` and `src` under the base, then the summary and what was said.
*/
int
d_renumber_tree(
    const char* _base,
    FILE*       _out,
    FILE*       _report
)
{
    struct dj_internal_walk walk;
    const char* const       tops[2] = { "inc", "src" };

    memset(&walk, 0, sizeof(walk));
    walk.report = _report;
    walk.ok     = true;

    for (size_t t = 0u; t < 2u; ++t)
    {
        const size_t length = strlen(_base) + strlen(tops[t]) + 2u;
        char* const  path   = malloc(length);

        if (!path)
        {
            walk.ok = false;

            break;
        }

        (void)snprintf(path, length, "%s/%s", _base, tops[t]);

        if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            d_internal_renumber_directory(&walk, path);
        }

        free(path);
    }

    (void)fprintf(_out, "converted: %zu file%s\n", walk.converted,
                  (walk.converted == 1u) ? "" : "s");

    for (size_t at = 0u; at < walk.said.count; ++at)
    {
        (void)fprintf(_out, "    %s\n", walk.said.items[at]);
    }

    d_internal_lines_free(&walk.said);

    return (walk.ok) ? 0 : 2;
}
