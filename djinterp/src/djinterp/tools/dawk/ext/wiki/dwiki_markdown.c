/*******************************************************************************
* djinterp [tools]                                              dwiki_markdown.c
*
* The Markdown driver: a GitHub wiki's pages.
*   Blocks are read as CommonMark reads them, a line at a time: the line is
* matched against the containers open -- quotes, lists' items -- and what
* is left either goes on in the leaf open or begins a block. GitHub's tables
* are read as cmark-gfm reads them: a paragraph's last line, then a line of
* dashes with as many cells. A paragraph's definitions, [label]: destination,
* are taken from its start when it closes.
*   Inline content is read after every block, so a reference can name a
* definition anywhere on the page. A leaf's lines are joined into one text,
* each place in it tied to its place on the page, and read left to right:
* code spans, autolinks and HTML before all else; then brackets, kept on a
* stack until a `]` closes one as a link, an image or text; then emphasis,
* which matters here only for what a heading's title says. GitHub's bare
* links -- www., http://, an address -- are read where a bracket is not
* open, and Gollum's [[Text|Page]] before a bracket is.
*   The anchor of a title is GitHub's: lower case, without punctuation, its
* spaces dashes; a second of one anchor has -1 after it, a third -2.
*
* path:      /src/djinterp/tools/dawk/ext/wiki/dwiki_markdown.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.20
*******************************************************************************/
// std
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset, strchr, strlen, strncmp
// djinterp
#include "./dwiki_driver.h"  // d_wiki_driver, d_wiki_add_block


// D_INTERNAL_TAB
//   constant: the columns between tab stops.
#define D_INTERNAL_TAB 4u

// D_INTERNAL_CODE_INDENT
//   constant: the indent that makes a line code.
#define D_INTERNAL_CODE_INDENT 4u

// D_INTERNAL_MOST_LABEL
//   constant: the longest a link's label may be.
#define D_INTERNAL_MOST_LABEL 999u


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Blocks
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_type
//   enum: what a node of the block tree is.
enum d_internal_type
{
    D_INTERNAL_DOCUMENT = 0,
    D_INTERNAL_QUOTE,
    D_INTERNAL_LIST,
    D_INTERNAL_ITEM,
    D_INTERNAL_PARAGRAPH,
    D_INTERNAL_HEADING,
    D_INTERNAL_CODE,
    D_INTERNAL_HTML,
    D_INTERNAL_RULE,
    D_INTERNAL_TABLE
};

// 1.1.2
// d_internal_node
//   struct: one node of the block tree: its record among the blocks, and
//   what its kind needs to go on matching lines.
struct d_internal_node
{
    enum d_internal_type type;
    size_t               parent;
    size_t               last_child;
    size_t               block;
    size_t               last_line;
    size_t               depth;
    size_t               lists;
    bool                 open;
    bool                 has_child;
    bool                 ordered;
    char                 marker;
    size_t               marker_offset;
    size_t               padding;
    bool                 fenced;
    char                 fence;
    size_t               fence_length;
    size_t               fence_offset;
    size_t               skip_line;
    int                  html;
    size_t               level;
    size_t               columns;
    size_t               heading;
};

// 1.1.3
// d_internal_reference
//   struct: a definition, by its label as labels compare.
struct d_internal_reference
{
    char*  key;
    size_t link;
};

// 1.1.4
// d_internal_job
//   struct: a leaf whose inline content is read once every block is: its
//   record, and its heading's when it is one.
struct d_internal_job
{
    size_t block;
    size_t heading;
    size_t columns;
};

// 1.1.5
// d_internal_md
//   struct: the reader: the page, the block tree, the line being read and
//   where in it, the definitions, and the leaves left to read inline.
struct d_internal_md
{
    struct d_wiki*               wiki;
    const char*                  text;
    struct d_internal_node*      nodes;
    size_t                       node_count;
    size_t                       node_capacity;
    size_t                       tip;
    size_t                       old_tip;
    size_t                       last_matched;
    bool                         all_closed;
    size_t                       line;
    size_t                       line_start;
    size_t                       line_end;
    size_t                       offset;
    size_t                       column;
    bool                         partial;
    size_t                       next_nonspace;
    size_t                       next_column;
    size_t                       indent;
    bool                         indented;
    bool                         blank;
    struct d_internal_reference* references;
    size_t                       reference_count;
    size_t                       reference_capacity;
    struct d_internal_job*       jobs;
    size_t                       job_count;
    size_t                       job_capacity;
    bool                         failed;
};

// 1.2    Inline content
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_piece
//   struct: a run of the joined text and where on the page it stands.
struct d_internal_piece
{
    size_t joined;
    size_t source;
    size_t length;
};

// 1.2.2
// d_internal_inline_kind
//   enum: what a node of inline content is.
enum d_internal_inline_kind
{
    D_INTERNAL_TEXT = 0,    // text, decoded
    D_INTERNAL_BREAK,       // a line's end
    D_INTERNAL_SPAN,        // code, decoded
    D_INTERNAL_RAW,         // HTML, which shows no text
    D_INTERNAL_DELIMITER,   // a run of `*`, `_` or `~`
    D_INTERNAL_OPENER,      // `[` or `![`, not yet closed
    D_INTERNAL_LINK         // an opener closed as a link or an image
};

// 1.2.3
// d_internal_inline
//   struct: one node of inline content.  A text's decoded bytes are
//   `length` from `decoded`; a delimiter run's are `count` of `c`, of
//   `original` read, linked to the runs before and after it while it may
//   still match; a link's content is the nodes up to `content_end`.
struct d_internal_inline
{
    enum d_internal_inline_kind kind;
    size_t                      decoded;
    size_t                      length;
    char                        c;
    size_t                      count;
    size_t                      original;
    bool                        can_open;
    bool                        can_close;
    size_t                      previous;
    size_t                      next;
    size_t                      content_end;
    bool                        image;
};

// 1.2.4
// d_internal_bracket
//   struct: a `[` or `![` open: its node, where it is in the joined text,
//   whether it may still begin a link, whether a bracket was opened after
//   it, and the delimiter run on top when it was.
struct d_internal_bracket
{
    size_t node;
    size_t at;
    bool   image;
    bool   active;
    bool   after;
    size_t delimiter;
};

// 1.2.5
// d_internal_waiting
//   struct: a link whose label is read once emphasis is: the link, and the
//   nodes of its content.
struct d_internal_waiting
{
    size_t link;
    size_t first;
    size_t end;
};

// 1.2.6
// d_internal_link_parts
//   struct: what follows a link's `]`, in the joined text: where the link
//   ends; for an inline link, its destination as written, `raw` to
//   `raw_end`, its bytes from `start` to `stop`, and its title, `title` to
//   `title_end`, D_WIKI_NONE for none; for a reference, the definition it
//   names, and its label, `label_from` to `label_to`, D_WIKI_NONE for a
//   label not written after it.
struct d_internal_link_parts
{
    size_t end;
    bool   inline_form;
    size_t raw;
    size_t raw_end;
    size_t start;
    size_t stop;
    size_t title;
    size_t title_end;
    size_t reference;
    size_t label_from;
    size_t label_to;
};

// 1.2.7
// d_internal_html_tag
//   struct: an <a> or an <img> being read, in a text: where it is, `at` to
//   `close`; whether it is an image; the attribute just read, its name
//   `name` to `name_end` and its value `from` to `to`, D_WIKI_NONE for
//   none; and the values wanted -- href or src, `value` to `stop`, and an
//   image's alt, `alt` to `alt_end` -- D_WIKI_NONE for none.
struct d_internal_html_tag
{
    size_t at;
    size_t close;
    bool   image;
    size_t name;
    size_t name_end;
    size_t from;
    size_t to;
    size_t value;
    size_t stop;
    size_t alt;
    size_t alt_end;
};

// 1.2.8
// d_internal_line
//   struct: a leaf's inline content being read: the joined text and its
//   pieces, the nodes and their decoded text, the brackets and delimiter
//   runs open, and the links whose labels wait.
struct d_internal_line
{
    struct d_internal_md*      md;
    size_t                     block;
    struct d_wiki_buffer       joined;
    struct d_internal_piece*   pieces;
    size_t                     piece_count;
    size_t                     piece_capacity;
    struct d_internal_inline*  nodes;
    size_t                     node_count;
    size_t                     node_capacity;
    struct d_wiki_buffer       decoded;
    struct d_internal_bracket* brackets;
    size_t                     bracket_count;
    size_t                     bracket_capacity;
    size_t                     top;
    struct d_internal_waiting* waiting;
    size_t                     waiting_count;
    size_t                     waiting_capacity;
    bool                       failed;
};


//==============================================================================
// 2.  HELPERS
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


/*
d_internal_is_space_or_tab, d_internal_is_alpha, d_internal_is_digit,
d_internal_is_alnum, d_internal_lower
  ASCII's classes, without the locale's say.
*/
static bool
d_internal_is_space_or_tab(
    char _c
)
{
    return ( (_c == ' ') || (_c == '\t') );
}


static bool
d_internal_is_alpha(
    char _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) );
}


static bool
d_internal_is_digit(
    char _c
)
{
    return ( (_c >= '0') && (_c <= '9') );
}


static bool
d_internal_is_alnum(
    char _c
)
{
    return (d_internal_is_alpha(_c)) || (d_internal_is_digit(_c));
}


static char
d_internal_lower(
    char _c
)
{
    return ( (_c >= 'A') && (_c <= 'Z') ) ? (char)(_c + 32) : _c;
}


/*
d_internal_is_ascii_punctuation
  Whether a byte is one a backslash escapes.
*/
static bool
d_internal_is_ascii_punctuation(
    char _c
)
{
    return ( (_c != '\0') &&
             (strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", _c) != NULL) );
}


/*
d_internal_starts_with
  Whether text of a length begins with a word, in any case.
*/
static bool
d_internal_starts_with(
    const char* _text,
    size_t      _length,
    const char* _word
)
{
    const size_t span = strlen(_word);

    // the text must be as long as the word
    if (_length < span)
    {
        return false;
    }

    // byte by byte, without case
    for (size_t at = 0; at < span; ++at)
    {
        // a byte that differs, in any case
        if (d_internal_lower(_text[at]) != _word[at])
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_unescape
  Writes text with its backslash escapes and character references read.
*/
static bool
d_internal_unescape(
    struct d_wiki_buffer* _out,
    const char*           _text,
    size_t                _length
)
{
    size_t at = 0;

    // each byte, an escape or a reference read as what it stands for
    while (at < _length)
    {
        // a backslash before punctuation is that punctuation
        if ( (_text[at] == '\\') &&
             ((at + 1u) < _length) &&
             (d_internal_is_ascii_punctuation(_text[at + 1u])) )
        {
            (void)d_wiki_buffer_add_char(_out, _text[at + 1u]);
            at += 2u;

            continue;
        }

        // a reference is what it stands for
        if (_text[at] == '&')
        {
            const size_t span = d_wiki_entity(&_text[at], _length - at, _out);

            // a reference decoded, or `&` as itself
            if (span > 0u)
            {
                at += span;

                continue;
            }
        }

        (void)d_wiki_buffer_add_char(_out, _text[at]);
        at++;
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_percent_decode
  Writes a path with its %hh read as the bytes they stand for.
*/
static int
d_internal_hex(
    char _c
)
{
    // a table of the digits' values
    if (d_internal_is_digit(_c))
    {
        return _c - '0';
    }

    // a letter a to f, in any case
    if ( (d_internal_lower(_c) >= 'a') &&
         (d_internal_lower(_c) <= 'f') )
    {
        return d_internal_lower(_c) - 'a' + 10;
    }

    return -1;
}


static bool
d_internal_percent_decode(
    struct d_wiki_buffer* _out,
    const char*           _text,
    size_t                _length
)
{
    size_t at = 0;

    // each byte, or the byte two hexadecimal digits after `%` give
    while (at < _length)
    {
        const int high = ((at + 2u) < _length) ? d_internal_hex(_text[at + 1u])
                                               : -1;
        const int low  = ((at + 2u) < _length) ? d_internal_hex(_text[at + 2u])
                                               : -1;

        // a `%` and two hexadecimal digits: the byte they name
        if ( (_text[at] == '%') &&
             (high >= 0) &&
             (low >= 0) )
        {
            (void)d_wiki_buffer_add_char(_out, (char)((high << 4) | low));
            at += 3u;

            continue;
        }

        (void)d_wiki_buffer_add_char(_out, _text[at]);
        at++;
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


//==============================================================================
// 3.  WHERE LINKS LEAD
//==============================================================================


/*
d_internal_set_text
  Replaces a link's text field with a copy of text of a length.
*/
static bool
d_internal_set_text(
    char**      _field,
    const char* _text,
    size_t      _length
)
{
    char* const copy = d_wiki_copy(_text, _length);

    // a copy that cannot be made leaves the field as it was
    if (!copy)
    {
        return false;
    }

    free(*_field);
    *_field = copy;

    return true;
}


/*
d_internal_classify
  Where a link leads, from its destination: an anchor here, a URL, an
address, an image, a file of the wiki, or a page of it -- a relative path
whose last part has no extension, or a page's -- named by that part.
*/
static bool
d_internal_classify(
    struct d_wiki_link* _link,
    bool                _image
)
{
    const char* const target = (_link->target) ? _link->target : "";
    const size_t      length = strlen(target);
    size_t            path   = length;

    _link->embedded = _image;

    // an anchor on this page
    if (target[0] == '#')
    {
        _link->to = D_WIKI_TO_ANCHOR;

        return (d_internal_set_text(&_link->page, "", 0u)) &&
               (d_internal_set_text(&_link->anchor, &target[1],
                                    length - 1u));
    }

    // a URL, or an address
    if ( (d_wiki_has_scheme(target)) ||
         (strncmp(target, "//", 2u) == 0) ||
         (length == 0u) )
    {
        _link->to = (d_internal_starts_with(target, length, "mailto:"))
                        ? D_WIKI_TO_MAIL
                        : (_image) ? D_WIKI_TO_IMAGE : D_WIKI_TO_URL;

        return (d_internal_set_text(&_link->page, "", 0u)) &&
               (d_internal_set_text(&_link->anchor, "", 0u));
    }

    // the path ends at its query or its anchor
    for (size_t at = 0; at < length; ++at)
    {
        // a path ends at its query or its fragment
        if ( (target[at] == '?') ||
             (target[at] == '#') )
        {
            path = at;

            break;
        }
    }

    const char* const hash   = strchr(target, '#');
    size_t            last   = path;
    bool              dotted = false;

    // the path's last part
    while ( (last > 0u) &&
            (target[last - 1u] != '/') )
    {
        last--;
        dotted = (dotted) || (target[last] == '.');
    }

    // the anchor, what follows `#`
    if (!d_internal_set_text(&_link->anchor, (hash) ? (hash + 1) : "",
                             (hash) ? strlen(hash + 1) : 0u))
    {
        return false;
    }

    // an image, shown or linked to
    if ( (_image) ||
         (d_wiki_is_image(target, path)) )
    {
        _link->to = D_WIKI_TO_IMAGE;

        return d_internal_set_text(&_link->page, target, path);
    }

    size_t                     stem   = path - last;
    const char* const          part   = &target[last];
    struct d_wiki_buffer       name   = { NULL, 0u, 0u, false };
    static const char* const   pages[] = { ".md", ".markdown", ".mediawiki" };

    // a page's extension is not its name's
    for (size_t at = 0; at < (sizeof(pages) / sizeof(pages[0])); ++at)
    {
        const size_t span = strlen(pages[at]);

        // a page's extension, taken off its name
        if ( (stem > span) &&
             (d_internal_starts_with(&part[stem - span], span, pages[at])) )
        {
            stem  -= span;
            dotted = false;

            break;
        }
    }

    // a part with another extension is a file
    if ( (dotted) ||
         (stem == 0u) )
    {
        _link->to = D_WIKI_TO_FILE;

        return d_internal_set_text(&_link->page, target, path);
    }

    _link->to = D_WIKI_TO_PAGE;

    const bool ok = (d_internal_percent_decode(&name, part, stem)) &&
                    (d_internal_set_text(&_link->page, name.text,
                                         name.length));

    d_wiki_buffer_free(&name);

    return ok;
}


//==============================================================================
// 4.  LINES
//==============================================================================


/*
d_internal_peek
  The byte at an offset of the line, or '\0' past its end.
*/
static char
d_internal_peek(
    const struct d_internal_md* _md,
    size_t                      _at
)
{
    return (_at < _md->line_end) ? _md->text[_at] : '\0';
}


/*
d_internal_find_next_nonspace
  Where the line's next byte that is not a space or tab is, the column it
is at, and the indent before it.
*/
static void
d_internal_find_next_nonspace(
    struct d_internal_md* _md
)
{
    size_t at      = _md->offset;
    size_t columns = _md->column;

    // spaces and tabs, a tab to the next stop
    while (at < _md->line_end)
    {
        // a space is a column; a tab, to the next stop
        if (_md->text[at] == ' ')
        {
            at++;
            columns++;
        }
        else if (_md->text[at] == '\t')
        {
            at++;
            columns += D_INTERNAL_TAB - (columns % D_INTERNAL_TAB);
        }
        else
        {
            break;
        }
    }

    _md->blank         = (at >= _md->line_end);
    _md->next_nonspace = at;
    _md->next_column   = columns;
    _md->indent        = columns - _md->column;
    _md->indented      = (_md->indent >= D_INTERNAL_CODE_INDENT);

    return;
}


/*
d_internal_advance
  Moves past `_count` bytes of the line, or past `_count` columns, a tab
perhaps taken in part.
*/
static void
d_internal_advance(
    struct d_internal_md* _md,
    size_t                _count,
    bool                  _columns
)
{
    // a byte, or a column, at a time
    while ( (_count > 0u) &&
            (_md->offset < _md->line_end) )
    {
        // a tab is the columns to its stop
        if (_md->text[_md->offset] == '\t')
        {
            const size_t to_tab = D_INTERNAL_TAB -
                                  (_md->column % D_INTERNAL_TAB);

            // a tab taken in part leaves the rest of its columns
            if (_columns)
            {
                const size_t taken = (to_tab > _count) ? _count : to_tab;

                _md->partial  = (to_tab > _count);
                _md->column  += taken;
                _md->offset  += (_md->partial) ? 0u : 1u;
                _count       -= taken;
            }
            else
            {
                _md->partial  = false;
                _md->column  += to_tab;
                _md->offset++;
                _count--;
            }
        }
        else
        {
            _md->partial = false;
            _md->offset++;
            _md->column++;
            _count--;
        }
    }

    return;
}


static void
d_internal_advance_nonspace(
    struct d_internal_md* _md
)
{
    _md->offset  = _md->next_nonspace;
    _md->column  = _md->next_column;
    _md->partial = false;

    return;
}


/*
d_internal_line_after
  Where the line after a line begins, from the line's index.
*/
static size_t
d_internal_line_after(
    const struct d_internal_md* _md,
    size_t                      _line
)
{
    return ((_line + 1u) < _md->wiki->line_count)
               ? _md->wiki->line_starts[_line + 1u]
               : _md->wiki->length;
}


//==============================================================================
// 5.  THE BLOCK TREE
//==============================================================================


/*
d_internal_can_contain
  Whether a node of one type can hold one of another.
*/
static bool
d_internal_can_contain(
    enum d_internal_type _parent,
    enum d_internal_type _child
)
{
    // a table of who holds whom
    switch (_parent)
    {
        case D_INTERNAL_DOCUMENT:
        case D_INTERNAL_QUOTE:
        case D_INTERNAL_ITEM:
            return (_child != D_INTERNAL_ITEM);

        case D_INTERNAL_LIST:
            return (_child == D_INTERNAL_ITEM);

        default:
            return false;
    }
}


/*
d_internal_mark
  Says that a node, and every node holding it, reached the line read.
*/
static void
d_internal_mark(
    struct d_internal_md* _md,
    size_t                _node
)
{
    // the node and those above it
    for (size_t at = _node; at != D_WIKI_NONE; at = _md->nodes[at].parent)
    {
        _md->nodes[at].last_line = _md->line;
    }

    return;
}


/*
d_internal_add_job
  Keeps a leaf to read inline once the blocks are read.
*/
static void
d_internal_add_job(
    struct d_internal_md* _md,
    size_t                _block,
    size_t                _heading,
    size_t                _columns
)
{
    // room for one more
    if (!d_internal_grow((void**)&_md->jobs, &_md->job_capacity,
                         _md->job_count, sizeof(struct d_internal_job)))
    {
        _md->failed = true;

        return;
    }

    _md->jobs[_md->job_count].block   = _block;
    _md->jobs[_md->job_count].heading = _heading;
    _md->jobs[_md->job_count].columns = _columns;
    _md->job_count++;

    return;
}


// the definitions of a paragraph, and the leaves that hold inline content,
// are read by what follows, as it is declared
static void d_internal_take_definitions(struct d_internal_md* _md,
                                        size_t                _node);
static void d_internal_finalize(struct d_internal_md* _md,
                                size_t                _node);


/*
d_internal_add_node
  Adds a node beneath the tip, the tip finalized until it can hold it, and
makes it the tip, with its record among the blocks when it has one.
*/
static size_t
d_internal_add_node(
    struct d_internal_md* _md,
    enum d_internal_type  _type,
    size_t                _start
)
{
    static const int kinds[] =
    {
        -1, D_WIKI_QUOTE, -1, D_WIKI_ITEM, D_WIKI_PARAGRAPH, D_WIKI_HEADING,
        D_WIKI_CODE, D_WIKI_HTML, D_WIKI_RULE, D_WIKI_TABLE
    };

    // the tip is finalized until it can hold the node
    while (!d_internal_can_contain(_md->nodes[_md->tip].type, _type))
    {
        d_internal_finalize(_md, _md->tip);
    }

    // room for one more
    if (!d_internal_grow((void**)&_md->nodes, &_md->node_capacity,
                         _md->node_count, sizeof(struct d_internal_node)))
    {
        _md->failed = true;

        return D_WIKI_NONE;
    }

    const size_t                  index  = _md->node_count++;
    struct d_internal_node* const node   = &_md->nodes[index];
    struct d_internal_node* const parent = &_md->nodes[_md->tip];

    memset(node, 0, sizeof(*node));
    node->type       = _type;
    node->parent     = _md->tip;
    node->last_child = D_WIKI_NONE;
    node->block      = D_WIKI_NONE;
    node->last_line  = _md->line;
    node->open       = true;
    node->skip_line  = D_WIKI_NONE;
    node->heading    = D_WIKI_NONE;
    node->depth      = parent->depth +
                       ( ( (parent->type == D_INTERNAL_QUOTE) ||
                           (parent->type == D_INTERNAL_ITEM) ) ? 1u : 0u);
    node->lists      = parent->lists +
                       ((_type == D_INTERNAL_LIST) ? 1u : 0u);
    parent->last_child = index;
    parent->has_child  = true;
    _md->tip           = index;

    // a record for every kind but the document and a list
    if (kinds[_type] >= 0)
    {
        size_t holder = node->parent;

        node->block = d_wiki_add_block(_md->wiki,
                                       (enum d_wiki_block_kind)kinds[_type],
                                       _start);

        // memory that ran out
        if (node->block == D_WIKI_NONE)
        {
            _md->failed = true;

            return D_WIKI_NONE;
        }

        // the container holding it, past any list
        while ( (holder != D_WIKI_NONE) &&
                (_md->nodes[holder].block == D_WIKI_NONE) )
        {
            holder = _md->nodes[holder].parent;
        }

        _md->wiki->blocks[node->block].parent =
            (holder != D_WIKI_NONE) ? _md->nodes[holder].block : D_WIKI_NONE;
        _md->wiki->blocks[node->block].depth  = _md->nodes[index].depth;
    }

    return index;
}


/*
d_internal_add_line
  Adds the rest of the line to the tip's content, the columns a tab left
as spaces before it.
*/
static void
d_internal_add_line(
    struct d_internal_md* _md
)
{
    size_t pad = 0;

    // a tab taken in part leaves spaces
    if (_md->partial)
    {
        _md->offset++;
        pad = D_INTERNAL_TAB - (_md->column % D_INTERNAL_TAB);
    }

    // a segment of the last block, which is the tip's
    if (!d_wiki_add_segment(_md->wiki, _md->offset, _md->line_end, pad))
    {
        _md->failed = true;

        return;
    }

    d_internal_mark(_md, _md->tip);

    return;
}


/*
d_internal_trim_code
  Drops an indented code block's blank lines at its end.
*/
static void
d_internal_trim_code(
    struct d_internal_md*   _md,
    struct d_internal_node* _node
)
{
    struct d_wiki_block* const block = &_md->wiki->blocks[_node->block];

    // the last lines, while they hold only space
    while (block->segment_count > 0u)
    {
        const struct d_wiki_segment* const segment =
            &_md->wiki->segments[block->first_segment +
                                 block->segment_count - 1u];
        size_t at = segment->start;

        // past the space on the last line
        while ( (at < segment->end) &&
                (d_internal_is_space_or_tab(_md->text[at])) )
        {
            at++;
        }

        // a line with something on it ends the trimming
        if (at < segment->end)
        {
            break;
        }

        block->segment_count--;
        _md->wiki->segment_count--;
    }

    // the block ends with its last line
    if (block->segment_count > 0u)
    {
        const struct d_wiki_segment* const last =
            &_md->wiki->segments[block->first_segment +
                                 block->segment_count - 1u];

        _node->last_line = d_wiki_line_of(_md->wiki, last->start, NULL) - 1u;
    }

    return;
}


/*
d_internal_finalize
  Closes a node: a paragraph gives up its definitions, a leaf with inline
content is kept to read, and the node's record ends with its last line.
*/
static void
d_internal_finalize(
    struct d_internal_md* _md,
    size_t                _node
)
{
    struct d_internal_node* node = &_md->nodes[_node];

    node->open = false;
    _md->tip   = node->parent;

    // a paragraph's definitions come first
    if (node->type == D_INTERNAL_PARAGRAPH)
    {
        d_internal_take_definitions(_md, _node);
        node = &_md->nodes[_node];
    }

    // an indented code block's last blank lines are not its own
    if ( (node->type == D_INTERNAL_CODE) &&
         (!node->fenced) )
    {
        d_internal_trim_code(_md, node);
    }

    // a node with no record has nothing more to close
    if (node->block == D_WIKI_NONE)
    {
        return;
    }

    struct d_wiki_block* const block = &_md->wiki->blocks[node->block];

    block->end = d_internal_line_after(_md, node->last_line);

    // a paragraph left with nothing, all its lines definitions, goes
    if ( (node->type == D_INTERNAL_PARAGRAPH) &&
         (block->segment_count == 0u) &&
         ((node->block + 1u) == _md->wiki->block_count) )
    {
        _md->wiki->block_count--;
        node->block = D_WIKI_NONE;

        return;
    }

    // a leaf with inline content is read once every block is
    if ( (node->type == D_INTERNAL_PARAGRAPH) ||
         (node->type == D_INTERNAL_HEADING) ||
         (node->type == D_INTERNAL_TABLE) )
    {
        d_internal_add_job(_md, node->block, node->heading, node->columns);
    }

    return;
}


/*
d_internal_close_unmatched
  Finalizes the nodes the line did not match, up to the last it did.
*/
static void
d_internal_close_unmatched(
    struct d_internal_md* _md
)
{
    // nothing is open that the line did not match
    if (_md->all_closed)
    {
        return;
    }

    // from the old tip up
    while (_md->old_tip != _md->last_matched)
    {
        const size_t parent = _md->nodes[_md->old_tip].parent;

        d_internal_finalize(_md, _md->old_tip);
        _md->old_tip = parent;
    }

    _md->all_closed = true;

    return;
}


//==============================================================================
// 6.  JOINED TEXT
//==============================================================================


/*
d_internal_join_add
  Adds a run of the page to the joined text.
*/
static bool
d_internal_join_add(
    struct d_internal_line* _line,
    size_t                  _source,
    size_t                  _length
)
{
    // room for one more piece
    if (!d_internal_grow((void**)&_line->pieces, &_line->piece_capacity,
                         _line->piece_count, sizeof(struct d_internal_piece)))
    {
        _line->failed = true;

        return false;
    }

    _line->pieces[_line->piece_count].joined = _line->joined.length;
    _line->pieces[_line->piece_count].source = _source;
    _line->pieces[_line->piece_count].length = _length;
    _line->piece_count++;

    return d_wiki_buffer_add(&_line->joined,
                             &_line->md->text[_source], _length);
}


/*
d_internal_source
  Where a place in the joined text stands on the page: a line feed joining
two lines stands where the first line ends.
*/
static size_t
d_internal_source(
    const struct d_internal_line* _line,
    size_t                        _at
)
{
    size_t low  = 0;
    size_t high = _line->piece_count;

    // nothing joined stands nowhere
    if (_line->piece_count == 0u)
    {
        return 0u;
    }

    // the last piece that begins at or before the place
    while ((low + 1u) < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // those from the middle, or those before it
        if (_line->pieces[middle].joined <= _at)
        {
            low = middle;
        }
        else
        {
            high = middle;
        }
    }

    const struct d_internal_piece* const piece = &_line->pieces[low];
    const size_t                         into  = _at - piece->joined;

    return piece->source + ((into > piece->length) ? piece->length : into);
}


/*
d_internal_join_block
  Joins a leaf's lines, a line feed between each, trimmed of space at
either end.
*/
static bool
d_internal_join_block(
    struct d_internal_line* _line,
    size_t                  _first,
    size_t                  _count
)
{
    const struct d_wiki* const wiki = _line->md->wiki;

    // each line, and a line feed after each but the last
    for (size_t index = 0; index < _count; ++index)
    {
        const struct d_wiki_segment* const segment =
            &wiki->segments[_first + index];
        size_t start = segment->start;
        size_t end   = segment->end;

        // the first line's space before it, and the last's after it
        if (index == 0u)
        {
            // the first line's space at its start
            while ( (start < end) &&
                    (d_internal_is_space_or_tab(wiki->text[start])) )
            {
                start++;
            }
        }

        // the last line's space at its end
        if ((index + 1u) == _count)
        {
            // back to the last byte that is not space
            while ( (end > start) &&
                    (d_internal_is_space_or_tab(wiki->text[end - 1u])) )
            {
                end--;
            }
        }

        // the line joined, or memory ran out
        if (!d_internal_join_add(_line, start, end - start))
        {
            return false;
        }

        // a line feed between lines
        if ( ((index + 1u) < _count) &&
             (!d_wiki_buffer_add_char(&_line->joined, '\n')) )
        {
            _line->failed = true;

            return false;
        }
    }

    // an empty leaf is an empty text
    return d_wiki_buffer_add(&_line->joined, "", 0u);
}


//==============================================================================
// 7.  LINK SYNTAX
//==============================================================================


/*
d_internal_skip_spaces
  Past spaces and tabs, a line ending among them at most once.
*/
static size_t
d_internal_skip_spaces(
    const char* _text,
    size_t      _length,
    size_t      _at,
    bool*       _out_newline
)
{
    bool newline = false;

    // spaces and tabs, and one line feed
    while (_at < _length)
    {
        // a space or a tab
        if (d_internal_is_space_or_tab(_text[_at]))
        {
            _at++;
        }
        else if ( (_text[_at] == '\n') &&
                  (!newline) )
        {
            newline = true;
            _at++;
        }
        else
        {
            break;
        }
    }

    // whether a line ending was passed, when asked
    if (_out_newline)
    {
        *_out_newline = newline;
    }

    return _at;
}


/*
d_internal_label_length
  The length of a link's label at `_at`, its brackets included: at most 999
bytes inside, no bracket unescaped; 0 for none.
*/
static size_t
d_internal_label_length(
    const char* _text,
    size_t      _length,
    size_t      _at
)
{
    size_t inside = 0;
    size_t at     = _at + 1u;

    // a label begins with `[`
    if ( (_at >= _length) ||
         (_text[_at] != '[') )
    {
        return 0u;
    }

    // up to `]`, escapes kept whole
    while ( (at < _length) &&
            (inside <= D_INTERNAL_MOST_LABEL) )
    {
        // the label's `]`
        if (_text[at] == ']')
        {
            return (at - _at) + 1u;
        }

        // a `[` within a label: no label
        if (_text[at] == '[')
        {
            return 0u;
        }

        // an escaped byte is the label's
        if ( (_text[at] == '\\') &&
             ((at + 1u) < _length) )
        {
            at++;
            inside++;
        }

        at++;
        inside++;
    }

    return 0u;
}


/*
d_internal_destination
  Reads a link's destination at `_at`: in <...>, or bytes with no space
and balanced parentheses.  Gives where it ends, and where its text is.
*/
static bool
d_internal_destination(
    const char* _text,
    size_t      _length,
    size_t      _at,
    size_t*     _out_end,
    size_t*     _out_start,
    size_t*     _out_stop
)
{
    size_t at = _at;

    // in angle brackets: to `>`, with no line feed or `<`
    if ( (at < _length) &&
         (_text[at] == '<') )
    {
        at++;

        // to the `>`, on the line
        while (at < _length)
        {
            // the destination between `<` and `>`
            if (_text[at] == '>')
            {
                *_out_start = _at + 1u;
                *_out_stop  = at;
                *_out_end   = at + 1u;

                return true;
            }

            // a line ending, or another `<`: no destination
            if ( (_text[at] == '\n') ||
                 (_text[at] == '<') )
            {
                return false;
            }

            at += ( (_text[at] == '\\') && ((at + 1u) < _length) ) ? 2u : 1u;
        }

        return false;
    }

    size_t parens = 0;

    // bare: to space or a control, or a `)` closing none
    while (at < _length)
    {
        const unsigned char c = (unsigned char)_text[at];

        // an escaped punctuation byte is the destination's
        if ( (c == '\\') &&
             ((at + 1u) < _length) &&
             (d_internal_is_ascii_punctuation(_text[at + 1u])) )
        {
            at += 2u;
        }
        else if (c == '(')
        {
            parens++;
            at++;
        }
        else if (c == ')')
        {
            // a `)` not opened within it ends it
            if (parens == 0u)
            {
                break;
            }

            parens--;
            at++;
        }
        else if ( (c <= 0x20u) ||
                  (c == 0x7Fu) )
        {
            break;
        }
        else
        {
            at++;
        }
    }

    // an empty one is allowed only before `)`
    if ( ( (at == _at) &&
           ( (at >= _length) || (_text[at] != ')') ) ) ||
         (parens != 0u) )
    {
        return false;
    }

    *_out_start = _at;
    *_out_stop  = at;
    *_out_end   = at;

    return true;
}


/*
d_internal_title
  Reads a link's title at `_at`: "...", '...' or (...). Gives where it ends.
*/
static bool
d_internal_title(
    const char* _text,
    size_t      _length,
    size_t      _at,
    size_t*     _out_end
)
{
    char close = '\0';

    // the opening says the close
    if (_at >= _length)
    {
        return false;
    }

    close = (_text[_at] == '"')  ? '"'
          : (_text[_at] == '\'') ? '\''
          : (_text[_at] == '(')  ? ')'
          :                        '\0';

    // a title opens with `"`, `'` or `(`
    if (close == '\0')
    {
        return false;
    }

    // to the close, escapes kept whole; ( may not open again in (...)
    for (size_t at = _at + 1u; at < _length; ++at)
    {
        // an escaped byte is the title's
        if (_text[at] == '\\')
        {
            at++;

            continue;
        }

        // its close
        if (_text[at] == close)
        {
            *_out_end = at + 1u;

            return true;
        }

        // `(` within a title in `()`: no title
        if ( (close == ')') &&
             (_text[at] == '(') )
        {
            return false;
        }
    }

    return false;
}


/*
d_internal_find_reference
  The definition a label names, D_WIKI_NONE for none.
*/
static size_t
d_internal_find_reference(
    const struct d_internal_md* _md,
    const char*                 _label,
    size_t                      _length
)
{
    struct d_wiki_buffer key   = { NULL, 0u, 0u, false };
    size_t               found = D_WIKI_NONE;

    // the label as labels compare; an empty one names nothing
    if ( (!d_wiki_normalize_label(&key, _label, _length)) ||
         (key.length == 0u) )
    {
        d_wiki_buffer_free(&key);

        return D_WIKI_NONE;
    }

    // the first definition of it
    for (size_t index = 0; index < _md->reference_count; ++index)
    {
        // the definition of the label
        if (strcmp(_md->references[index].key, key.text) == 0)
        {
            found = _md->references[index].link;

            break;
        }
    }

    d_wiki_buffer_free(&key);

    return found;
}


/*
d_internal_definition_parse
  [label]: destination "title" at `_at`, to its line's end: where its label
and destination are, and its title, a title with more after it on its line
leaving the definition without it.  Gives where its line ends, or 0 for no
definition.
*/
static size_t
d_internal_definition_parse(
    const struct d_internal_line* _line,
    size_t                        _at,
    struct d_internal_link_parts* _parts
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    const size_t      label  = d_internal_label_length(text, length, _at);
    size_t            at     = _at + label;

    // [label]:
    if ( (label < 3u) ||
         (at >= length) ||
         (text[at] != ':') )
    {
        return 0u;
    }

    memset(_parts, 0, sizeof(*_parts));
    _parts->label_from = _at + 1u;
    _parts->label_to   = _at + label - 1u;
    _parts->title      = D_WIKI_NONE;
    at                 = d_internal_skip_spaces(text, length, at + 1u, NULL);
    _parts->raw        = at;

    // the destination
    if (!d_internal_destination(text, length, at, &at, &_parts->start,
                                &_parts->stop))
    {
        return 0u;
    }

    _parts->raw_end = at;

    const size_t before = at;

    at = d_internal_skip_spaces(text, length, at, NULL);

    // a title, after space, ends the definition; else its destination does
    if ( (at > before) &&
         (d_internal_title(text, length, at, &_parts->title_end)) )
    {
        _parts->title = at;
        _parts->end   = _parts->title_end;
    }
    else
    {
        _parts->end = before;
    }

    // nothing but space to the line's end; a title with more after it is
    // not the definition's
    size_t end = _parts->end;

    // past the space after it
    while ( (end < length) &&
            (d_internal_is_space_or_tab(text[end])) )
    {
        end++;
    }

    // something after a title on its line: the definition without the title
    if ( (end < length) &&
         (text[end] != '\n') &&
         (_parts->title != D_WIKI_NONE) )
    {
        _parts->title = D_WIKI_NONE;
        _parts->end   = before;
        end           = before;

        // past the space after the destination
        while ( (end < length) &&
                (d_internal_is_space_or_tab(text[end])) )
        {
            end++;
        }
    }

    // more on the line: no definition
    if ( (end < length) &&
         (text[end] != '\n') )
    {
        return 0u;
    }

    return (end < length) ? (end + 1u) : end;
}


/*
d_internal_definition_keep
  A definition read kept: its link, and its label among the definitions
when it is the label's first.  Gives whether memory held.
*/
static bool
d_internal_definition_keep(
    struct d_internal_line*             _line,
    size_t                              _at,
    size_t                              _block,
    const struct d_internal_link_parts* _parts,
    struct d_wiki_buffer*               _key
)
{
    struct d_internal_md* const md     = _line->md;
    const char* const           text   = _line->joined.text;
    const size_t                length = _parts->label_to - _parts->label_from;
    struct d_wiki_link          link;
    struct d_wiki_buffer        target = { NULL, 0u, 0u, false };
    struct d_wiki_buffer        shown  = { NULL, 0u, 0u, false };

    d_wiki_new_link(&link, D_WIKI_FORM_DEFINITION,
                    d_internal_source(_line, _at),
                    d_internal_source(_line, _parts->end));
    link.block        = _block;
    link.label_start  = d_internal_source(_line, _parts->label_from);
    link.label_end    = d_internal_source(_line, _parts->label_to);
    link.target_start = d_internal_source(_line, _parts->raw);
    link.target_end   = d_internal_source(_line, _parts->raw_end);

    // its title, written
    if (_parts->title != D_WIKI_NONE)
    {
        link.title_start = d_internal_source(_line, _parts->title);
        link.title_end   = d_internal_source(_line, _parts->title_end);
    }

    const bool ok = (d_internal_unescape(&target, &text[_parts->start],
                                         _parts->stop - _parts->start)) &&
                    (d_wiki_buffer_add(&shown, &text[_parts->label_from],
                                       length));

    link.target = d_wiki_buffer_take(&target);
    link.label  = d_wiki_buffer_take(&shown);

    // the link, where it leads, or memory ran out
    if ( (!ok) ||
         (!link.target) ||
         (!link.label) ||
         (!d_internal_classify(&link, false)) )
    {
        free(link.target);
        free(link.label);
        free(link.page);
        free(link.anchor);

        return false;
    }

    const size_t index = d_wiki_add_link(md->wiki, &link);

    // memory that ran out
    if (index == D_WIKI_NONE)
    {
        return false;
    }

    // the first definition of a label is the one it names
    if ( (d_internal_find_reference(md, &text[_parts->label_from], length)
              == D_WIKI_NONE) &&
         (d_internal_grow((void**)&md->references, &md->reference_capacity,
                          md->reference_count,
                          sizeof(struct d_internal_reference))) )
    {
        md->references[md->reference_count].key  = d_wiki_buffer_take(_key);
        md->references[md->reference_count].link = index;
        md->reference_count++;
    }

    return true;
}


/*
d_internal_definition
  A definition, [label]: destination "title", at `_at` of a paragraph's
joined text, kept for the block `_block`: gives how many bytes it takes,
its line ending with it, or 0 for none.
*/
static size_t
d_internal_definition(
    struct d_internal_line* _line,
    size_t                  _at,
    size_t                  _block
)
{
    struct d_internal_link_parts parts;
    struct d_wiki_buffer         key = { NULL, 0u, 0u, false };
    const size_t                 end = d_internal_definition_parse(_line, _at,
                                                                   &parts);

    // no definition, or a label that is only space, which names nothing
    if ( (end == 0u) ||
         (!d_wiki_normalize_label(&key, &_line->joined.text[parts.label_from],
                                  parts.label_to - parts.label_from)) ||
         (key.length == 0u) )
    {
        d_wiki_buffer_free(&key);

        return 0u;
    }

    // kept, or memory ran out
    if (!d_internal_definition_keep(_line, _at, _block, &parts, &key))
    {
        _line->md->failed = true;
        d_wiki_buffer_free(&key);

        return 0u;
    }

    d_wiki_buffer_free(&key);

    return end - _at;
}


/*
d_internal_take_definitions
  Takes the definitions from the start of a paragraph: each becomes a
block of its own, and what is left of the paragraph a block after them.
  Long as the paragraph's text is joined, read and cut up in one pass.
*/
static void
d_internal_take_definitions(
    struct d_internal_md* _md,
    size_t                _node
)
{
    struct d_internal_node* const node  = &_md->nodes[_node];
    struct d_internal_line        line;
    struct d_wiki*                wiki  = _md->wiki;
    size_t                        at    = 0;
    size_t                        taken = 0;

    // a paragraph that does not begin with `[` has none
    if (node->block == D_WIKI_NONE)
    {
        return;
    }

    const struct d_wiki_block paragraph = wiki->blocks[node->block];

    // a paragraph that does not begin with `[` begins no definition
    if ( (paragraph.segment_count == 0u) ||
         (_md->text[wiki->segments[paragraph.first_segment].start] != '[') )
    {
        return;
    }

    memset(&line, 0, sizeof(line));
    line.md = _md;

    // its lines joined, untrimmed at the end, so each ends a line
    for (size_t index = 0; index < paragraph.segment_count; ++index)
    {
        const struct d_wiki_segment* const segment =
            &wiki->segments[paragraph.first_segment + index];

        (void)d_internal_join_add(&line, segment->start,
                                  segment->end - segment->start);

        // a line feed between lines
        if ((index + 1u) < paragraph.segment_count)
        {
            (void)d_wiki_buffer_add_char(&line.joined, '\n');
        }
    }

    size_t segment = 0;
    size_t first   = paragraph.first_segment;
    bool   reused  = false;

    // a definition at a time, each a block of its own
    while ( (!line.failed) &&
            (!_md->failed) &&
            (at < line.joined.length) &&
            (line.joined.text[at] == '[') )
    {
        const size_t block  = (reused) ? D_WIKI_NONE : node->block;
        const size_t length = d_internal_definition(&line, at,
                                                    (block != D_WIKI_NONE)
                                                        ? block
                                                        : wiki->block_count);

        // no more definitions
        if (length == 0u)
        {
            break;
        }

        // the lines it takes
        size_t lines = 0;

        // a definition may take several lines
        for (size_t scan = at; scan < (at + length); ++scan)
        {
            lines += (line.joined.text[scan] == '\n') ? 1u : 0u;
        }

        lines += ((at + length) >= line.joined.length) ? 1u : 0u;

        size_t index = node->block;

        // the paragraph's record is the first's; a block after for others
        if (reused)
        {
            index = d_wiki_add_block(wiki, D_WIKI_DEFINITION,
                                     wiki->segments[first].start);

            // memory that ran out
            if (index == D_WIKI_NONE)
            {
                _md->failed = true;

                break;
            }

            wiki->blocks[index].parent = paragraph.parent;
            wiki->blocks[index].depth  = paragraph.depth;
        }

        wiki->blocks[index].kind          = D_WIKI_DEFINITION;
        wiki->blocks[index].start         = wiki->segments[first].start;
        wiki->blocks[index].first_segment = first;
        wiki->blocks[index].segment_count = lines;
        wiki->blocks[index].end           =
            d_internal_line_after(_md,
                                  d_wiki_line_of(wiki,
                                                 wiki->segments[first + lines
                                                                - 1u].start,
                                                 NULL) - 1u);
        reused   = true;
        first   += lines;
        segment += lines;
        at      += length;
        taken++;
    }

    d_wiki_buffer_free(&line.joined);
    free(line.pieces);

    // no definition leaves the paragraph as it was
    if (taken == 0u)
    {
        return;
    }

    // what is left is a paragraph after them, perhaps with no lines yet
    const size_t rest = d_wiki_add_block(wiki, D_WIKI_PARAGRAPH,
                                         (segment < paragraph.segment_count)
                                             ? wiki->segments[first].start
                                             : paragraph.end);

    // memory that ran out
    if (rest == D_WIKI_NONE)
    {
        _md->failed = true;

        return;
    }

    wiki->blocks[rest].first_segment = first;
    wiki->blocks[rest].segment_count = paragraph.segment_count - segment;
    wiki->blocks[rest].parent        = paragraph.parent;
    wiki->blocks[rest].depth         = paragraph.depth;
    _md->nodes[_node].block          = rest;

    return;
}


//==============================================================================
// 8.  BLOCK STARTS
//==============================================================================


/*
d_internal_is_tag_name_end
  Whether a byte ends an HTML block's tag name: space, `>`, `/>`, the
line's end.
*/
static bool
d_internal_is_tag_name_end(
    const char* _text,
    size_t      _length,
    size_t      _at
)
{
    return ( (_at >= _length) ||
             (d_internal_is_space_or_tab(_text[_at])) ||
             (_text[_at] == '>') ||
             ( (_text[_at] == '/') &&
               ((_at + 1u) < _length) &&
               (_text[_at + 1u] == '>') ) );
}


/*
d_internal_tag_space
  Past the space at `_at` within a tag: spaces and tabs, and line feeds when
`_lines` allows them.
*/
static size_t
d_internal_tag_space(
    const char* _text,
    size_t      _length,
    size_t      _at,
    bool        _lines
)
{
    // each space, tab, or line feed allowed
    while ( (_at < _length) &&
            ( (d_internal_is_space_or_tab(_text[_at])) ||
              ( (_lines) && (_text[_at] == '\n') ) ) )
    {
        _at++;
    }

    return _at;
}


/*
d_internal_tag_value
  An attribute's value at `_at`, after its `=`: quoted, to its close, or
bare, to the space or the byte that ends it.  Gives where it ends, 0 for
none.
*/
static size_t
d_internal_tag_value(
    const char* _text,
    size_t      _length,
    size_t      _at
)
{
    size_t at = _at;

    // quoted: a quoted value runs to its close
    if ( (_at < _length) &&
         ( (_text[_at] == '"') || (_text[_at] == '\'') ) )
    {
        const char* const close = memchr(&_text[_at + 1u], _text[_at],
                                         _length - _at - 1u);

        return (close) ? ((size_t)(close - _text) + 1u) : 0u;
    }

    // bare: to the space or the byte that ends it
    while ( (at < _length) &&
            ((unsigned char)_text[at] > 0x20u) &&
            (strchr("\"'=<>`", _text[at]) == NULL) )
    {
        at++;
    }

    // an empty bare value is no value
    return (at > _at) ? at : 0u;
}


/*
d_internal_tag
  Reads an HTML open or closing tag at `_at`, all of it before `_length`;
gives where it ends, 0 for none.  `_lines` allows line feeds in its
space.
*/
static size_t
d_internal_tag(
    const char* _text,
    size_t      _length,
    size_t      _at,
    bool        _lines
)
{
    size_t at      = _at + 1u;
    bool   closing = false;

    // `<` and a name
    if ( (at >= _length) ||
         (_text[_at] != '<') )
    {
        return 0u;
    }

    // a closing tag's `/`
    if (_text[at] == '/')
    {
        closing = true;
        at++;
    }

    // a tag's name begins with a letter
    if ( (at >= _length) ||
         (!d_internal_is_alpha(_text[at])) )
    {
        return 0u;
    }

    // its letters, digits and `-`
    while ( (at < _length) &&
            ( (d_internal_is_alnum(_text[at])) || (_text[at] == '-') ) )
    {
        at++;
    }

    // a closing tag: space, then `>`
    if (closing)
    {
        at = d_internal_tag_space(_text, _length, at, _lines);

        return ( (at < _length) && (_text[at] == '>') ) ? (at + 1u) : 0u;
    }

    // attributes, each after space, to `>` or `/>`
    while (at < _length)
    {
        const size_t before = at;

        at = d_internal_tag_space(_text, _length, at, _lines);

        // the end: `>` or `/>`
        if ( (at < _length) &&
             (_text[at] == '>') )
        {
            return at + 1u;
        }

        // `/>` closes it
        if ( ((at + 1u) < _length) &&
             (_text[at] == '/') &&
             (_text[at + 1u] == '>') )
        {
            return at + 2u;
        }

        // an attribute's name needs space before it
        if ( (at == before) ||
             (at >= _length) ||
             (!( (d_internal_is_alpha(_text[at])) ||
                 (_text[at] == '_') ||
                 (_text[at] == ':') )) )
        {
            return 0u;
        }

        // an attribute's name
        while ( (at < _length) &&
                ( (d_internal_is_alnum(_text[at])) ||
                  (strchr("_.:-", _text[at]) != NULL) ) )
        {
            at++;
        }

        const size_t value = d_internal_tag_space(_text, _length, at, _lines);

        // an attribute with no value; else its value, after `=`
        if ( (value >= _length) ||
             (_text[value] != '=') )
        {
            continue;
        }

        at = d_internal_tag_value(_text, _length,
                                  d_internal_tag_space(_text, _length,
                                                       value + 1u, _lines));

        // a value that does not end is no tag
        if (at == 0u)
        {
            return 0u;
        }
    }

    return 0u;
}


/*
d_internal_html_start
  Which kind of HTML block, 1 to 7, the line begins at `_at`; 0 for none.
*/
static int
d_internal_html_start(
    const char* _text,
    size_t      _length,
    size_t      _at
)
{
    static const char* const literal[] =
    {
        "script", "pre", "textarea", "style"
    };
    static const char* const blocks[] =
    {
        "address", "article", "aside", "base", "basefont", "blockquote",
        "body", "caption", "center", "col", "colgroup", "dd", "details",
        "dialog", "dir", "div", "dl", "dt", "fieldset", "figcaption",
        "figure", "footer", "form", "frame", "frameset", "h1", "h2", "h3",
        "h4", "h5", "h6", "head", "header", "hr", "html", "iframe",
        "legend", "li", "link", "main", "menu", "menuitem", "nav",
        "noframes", "ol", "optgroup", "option", "p", "param", "search",
        "section", "summary", "table", "tbody", "td", "tfoot", "th",
        "thead", "title", "tr", "track", "ul"
    };
    const char* const at   = &_text[_at];
    const size_t      left = _length - _at;

    // 1: a tag whose content is literal
    for (size_t index = 0; index < (sizeof(literal) / sizeof(literal[0]));
         ++index)
    {
        const size_t span = strlen(literal[index]);

        // type 1: <script>, <pre>, <style>, <textarea>
        if ( (left > span) &&
             (d_internal_starts_with(&at[1], left - 1u, literal[index])) &&
             ( ((span + 1u) >= left) ||
               (d_internal_is_space_or_tab(at[span + 1u])) ||
               (at[span + 1u] == '>') ) )
        {
            return 1;
        }
    }

    // 2 to 5: a comment, an instruction, a declaration, CDATA
    if (d_internal_starts_with(at, left, "<!--"))
    {
        return 2;
    }

    // type 3: a processing instruction
    if (d_internal_starts_with(at, left, "<?"))
    {
        return 3;
    }

    // type 4: a declaration
    if ( (left > 2u) &&
         (at[1] == '!') &&
         (d_internal_is_alpha(at[2])) )
    {
        return 4;
    }

    // type 5: CDATA
    if ( (left >= 9u) &&
         (strncmp(at, "<![CDATA[", 9u) == 0) )
    {
        return 5;
    }

    const size_t name = (left > 1u) && (at[1] == '/') ? 2u : 1u;

    // 6: a block's tag, open or closing
    for (size_t index = 0; index < (sizeof(blocks) / sizeof(blocks[0]));
         ++index)
    {
        const size_t span = strlen(blocks[index]);

        // type 6: a block tag's name
        if ( (left >= (name + span)) &&
             (d_internal_starts_with(&at[name], left - name,
                                     blocks[index])) &&
             (d_internal_is_tag_name_end(_text, _length, _at + name + span)) )
        {
            return 6;
        }
    }

    const size_t end = d_internal_tag(_text, _length, _at, false);

    // 7: any other whole tag, alone on the line
    if (end > 0u)
    {
        size_t rest = end;

        // type 7: a whole tag, alone on its line, but not type 1's
        for (size_t index = 0; index < (sizeof(literal) / sizeof(literal[0]));
             ++index)
        {
            const size_t span  = strlen(literal[index]);
            const char   after = ((name + span) < left) ? at[name + span]
                                                         : '\0';

            // a tag of type 1 is not type 7's
            if ( (d_internal_starts_with(&at[name], left - name,
                                         literal[index])) &&
                 (!d_internal_is_alnum(after)) &&
                 (after != '-') )
            {
                return 0;
            }
        }

        // nothing but space after it on its line
        while ( (rest < _length) &&
                (d_internal_is_space_or_tab(_text[rest])) )
        {
            rest++;
        }

        return (rest >= _length) ? 7 : 0;
    }

    return 0;
}


/*
d_internal_html_closes
  Whether a line holds what ends an HTML block of a kind, 1 to 5.
*/
static bool
d_internal_html_closes(
    const char* _text,
    size_t      _start,
    size_t      _end,
    int         _kind
)
{
    static const char* const literal[] =
    {
        "</script>", "</pre>", "</textarea>", "</style>"
    };

    // each place on the line
    for (size_t at = _start; at < _end; ++at)
    {
        const size_t left = _end - at;

        // type 1 closes at its tag's close
        if (_kind == 1)
        {
            // each tag of type 1
            for (size_t index = 0;
                 index < (sizeof(literal) / sizeof(literal[0])); ++index)
            {
                // its close on the line
                if (d_internal_starts_with(&_text[at], left, literal[index]))
                {
                    return true;
                }
            }
        }
        else if ( ( (_kind == 2) &&
                    (left >= 3u) &&
                    (strncmp(&_text[at], "-->", 3u) == 0) ) ||
                  ( (_kind == 3) &&
                    (left >= 2u) &&
                    (strncmp(&_text[at], "?>", 2u) == 0) ) ||
                  ( (_kind == 4) &&
                    (_text[at] == '>') ) ||
                  ( (_kind == 5) &&
                    (left >= 3u) &&
                    (strncmp(&_text[at], "]]>", 3u) == 0) ) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_run
  How many of a byte follow from `_at`, spaces and tabs between them
allowed or not; `_out_end` receives where the run, and any space after
it, ends.
*/
static size_t
d_internal_run(
    const struct d_internal_md* _md,
    size_t                      _at,
    char                        _c,
    bool                        _spaced,
    size_t*                     _out_end
)
{
    size_t count = 0;
    size_t at    = _at;

    // the bytes, and space between them when allowed
    while (at < _md->line_end)
    {
        // the byte, counted; space between, passed
        if (_md->text[at] == _c)
        {
            count++;
        }
        else if ( (!_spaced) ||
                  (!d_internal_is_space_or_tab(_md->text[at])) )
        {
            break;
        }

        at++;
    }

    // where the run ends, when asked
    if (_out_end)
    {
        *_out_end = at;
    }

    return count;
}


/*
d_internal_rest_is_blank
  Whether nothing but space is on the line from `_at`.
*/
static bool
d_internal_rest_is_blank(
    const struct d_internal_md* _md,
    size_t                      _at
)
{
    // each byte left must be space
    for (size_t at = _at; at < _md->line_end; ++at)
    {
        // a byte that is not space
        if (!d_internal_is_space_or_tab(_md->text[at]))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_start_atx
  # Heading: its level, its title trimmed of the closing #s.
*/
static int
d_internal_start_atx(
    struct d_internal_md* _md
)
{
    const size_t at    = _md->next_nonspace;
    const size_t level = d_internal_run(_md, at, '#', false, NULL);
    const char   after = d_internal_peek(_md, at + level);

    // one to six, then space or the line's end
    if ( (_md->indented) ||
         (level < 1u) ||
         (level > D_WIKI_MOST_LEVEL) ||
         ( (after != '\0') && (!d_internal_is_space_or_tab(after)) ) )
    {
        return 0;
    }

    d_internal_advance_nonspace(_md);
    d_internal_advance(_md, level, false);
    d_internal_close_unmatched(_md);

    const size_t node = d_internal_add_node(_md, D_INTERNAL_HEADING, at);

    // memory that ran out
    if (node == D_WIKI_NONE)
    {
        return 2;
    }

    size_t start = _md->offset;
    size_t end   = _md->line_end;

    // the title, past space, before space
    while ( (start < end) &&
            (d_internal_is_space_or_tab(_md->text[start])) )
    {
        start++;
    }

    // the title's space at its end
    while ( (end > start) &&
            (d_internal_is_space_or_tab(_md->text[end - 1u])) )
    {
        end--;
    }

    size_t closing = end;

    // a closing run of #, after space or alone
    while ( (closing > start) &&
            (_md->text[closing - 1u] == '#') )
    {
        closing--;
    }

    // a closing sequence of `#`, after space, is no part of the title
    if ( (closing < end) &&
         ( (closing == start) ||
           (d_internal_is_space_or_tab(_md->text[closing - 1u])) ) )
    {
        end = closing;

        // and the space before the closing sequence
        while ( (end > start) &&
                (d_internal_is_space_or_tab(_md->text[end - 1u])) )
        {
            end--;
        }
    }

    struct d_wiki_heading heading;

    memset(&heading, 0, sizeof(heading));
    heading.block       = _md->nodes[node].block;
    heading.level       = level;
    heading.start       = at;
    heading.end         = d_internal_line_after(_md, _md->line);
    heading.title_start = start;
    heading.title_end   = end;
    heading.top         = (_md->nodes[node].depth == 0u);

    _md->nodes[node].heading = d_wiki_add_heading(_md->wiki, &heading);
    _md->nodes[node].level   = level;
    _md->wiki->blocks[_md->nodes[node].block].level = level;

    // the title is the heading's one line of content
    if ( (_md->nodes[node].heading == D_WIKI_NONE) ||
         (!d_wiki_add_segment(_md->wiki, start, end, 0u)) )
    {
        _md->failed = true;
    }

    _md->offset = _md->line_end;

    return 2;
}


/*
d_internal_start_fence
  ``` or ~~~: three or more, a backtick fence's info with no backtick.
*/
static int
d_internal_start_fence(
    struct d_internal_md* _md
)
{
    const size_t at     = _md->next_nonspace;
    const char   c      = d_internal_peek(_md, at);
    const size_t length = ( (c == '`') || (c == '~') )
                              ? d_internal_run(_md, at, c, false, NULL) : 0u;

    // three or more, not indented as code
    if ( (_md->indented) ||
         (length < 3u) )
    {
        return 0;
    }

    // a backtick fence's info has no backtick
    if (c == '`')
    {
        // a fence of backticks has none in its info string
        for (size_t scan = at + length; scan < _md->line_end; ++scan)
        {
            // a backtick after the fence: no fence
            if (_md->text[scan] == '`')
            {
                return 0;
            }
        }
    }

    d_internal_close_unmatched(_md);

    const size_t node = d_internal_add_node(_md, D_INTERNAL_CODE, at);

    // memory that ran out
    if (node == D_WIKI_NONE)
    {
        return 2;
    }

    struct d_internal_node* const code  = &_md->nodes[node];
    struct d_wiki_block* const    block = &_md->wiki->blocks[code->block];
    size_t                        start = at + length;
    size_t                        end   = _md->line_end;

    code->fenced       = true;
    code->fence        = c;
    code->fence_length = length;
    code->fence_offset = _md->indent;
    code->skip_line    = _md->line;
    block->marker      = c;

    // the info, trimmed
    while ( (start < end) &&
            (d_internal_is_space_or_tab(_md->text[start])) )
    {
        start++;
    }

    // the info string's space at its end
    while ( (end > start) &&
            (d_internal_is_space_or_tab(_md->text[end - 1u])) )
    {
        end--;
    }

    // the info string, written
    if (end > start)
    {
        block->info_start = start;
        block->info_end   = end;
    }

    d_internal_advance_nonspace(_md);
    d_internal_advance(_md, length, false);

    return 2;
}


/*
d_internal_start_html
  An HTML block, of the kind the line begins; the seventh may not
interrupt a paragraph.
*/
static int
d_internal_start_html(
    struct d_internal_md* _md,
    size_t                _container
)
{
    // `<`, not indented as code
    if ( (_md->indented) ||
         (d_internal_peek(_md, _md->next_nonspace) != '<') )
    {
        return 0;
    }

    const int kind = d_internal_html_start(_md->text, _md->line_end,
                                           _md->next_nonspace);

    // no HTML block, or type 7 where a paragraph would go on
    if ( (kind == 0) ||
         ( (kind == 7) &&
           ( (_md->nodes[_container].type == D_INTERNAL_PARAGRAPH) ||
             ( (!_md->all_closed) &&
               (!_md->blank) &&
               (_md->nodes[_md->tip].type == D_INTERNAL_PARAGRAPH) ) ) ) )
    {
        return 0;
    }

    d_internal_close_unmatched(_md);

    const size_t node = d_internal_add_node(_md, D_INTERNAL_HTML,
                                            _md->next_nonspace);

    // its type, which says what closes it
    if (node != D_WIKI_NONE)
    {
        _md->nodes[node].html = kind;
    }

    return 2;
}


/*
d_internal_start_setext
  A paragraph underlined with = or -: its definitions taken, what is left
a heading.
*/
static int
d_internal_start_setext(
    struct d_internal_md* _md,
    size_t                _container
)
{
    const size_t at  = _md->next_nonspace;
    const char   c   = d_internal_peek(_md, at);
    size_t       end = 0;

    // = or - alone on the line, under a paragraph the line matched
    if ( (_md->indented) ||
         (_md->nodes[_container].type != D_INTERNAL_PARAGRAPH) ||
         ( (c != '=') && (c != '-') ) )
    {
        return 0;
    }

    (void)d_internal_run(_md, at, c, false, &end);

    // a setext underline is alone on its line
    if (!d_internal_rest_is_blank(_md, end))
    {
        return 0;
    }

    d_internal_close_unmatched(_md);
    d_internal_take_definitions(_md, _container);

    struct d_internal_node* const node  = &_md->nodes[_container];
    struct d_wiki_block* const    block = &_md->wiki->blocks[node->block];

    // a paragraph all definitions is not a heading
    if (block->segment_count == 0u)
    {
        return 0;
    }

    struct d_wiki_heading heading;
    const size_t          last  = block->first_segment +
                                  block->segment_count - 1u;
    size_t                title = _md->wiki->segments[last].end;

    // the title, from the first line to the last's end, trimmed
    while ( (title > block->start) &&
            (d_internal_is_space_or_tab(_md->text[title - 1u])) )
    {
        title--;
    }

    memset(&heading, 0, sizeof(heading));
    heading.block       = node->block;
    heading.level       = (c == '=') ? 1u : 2u;
    heading.start       = block->start;
    heading.end         = d_internal_line_after(_md, _md->line);
    heading.title_start = block->start;
    heading.title_end   = title;
    heading.top         = (node->depth == 0u);
    heading.setext      = true;

    node->type     = D_INTERNAL_HEADING;
    node->level    = heading.level;
    node->heading  = d_wiki_add_heading(_md->wiki, &heading);
    block->kind    = D_WIKI_HEADING;
    block->level   = heading.level;
    block->marker  = c;

    // memory that ran out
    if (node->heading == D_WIKI_NONE)
    {
        _md->failed = true;
    }

    d_internal_mark(_md, _container);
    _md->offset = _md->line_end;

    return 2;
}


/*
d_internal_start_rule
  ***, ---, ___: three or more of one, with space between.
*/
static int
d_internal_start_rule(
    struct d_internal_md* _md
)
{
    const size_t at  = _md->next_nonspace;
    const char   c   = d_internal_peek(_md, at);
    size_t       end = 0;

    // three of one, alone on the line
    if ( (_md->indented) ||
         ( (c != '*') && (c != '-') && (c != '_') ) ||
         (d_internal_run(_md, at, c, true, &end) < 3u) ||
         (end < _md->line_end) )
    {
        return 0;
    }

    d_internal_close_unmatched(_md);
    (void)d_internal_add_node(_md, D_INTERNAL_RULE, at);
    _md->offset = _md->line_end;

    return 2;
}


/*
d_internal_start_item
  A list's item: its marker, and the columns its content begins after.
  Long as commonmark.js's parseListMarker and its list item start are, which
it follows.
*/
static int
d_internal_start_item(
    struct d_internal_md* _md,
    size_t                _container
)
{
    const size_t at      = _md->next_nonspace;
    const char   c       = d_internal_peek(_md, at);
    bool         ordered = false;
    char         marker  = c;
    size_t       length  = 1u;

    // indented as code, unless in a list already
    if ( ( (_md->indented) &&
           (_md->nodes[_container].type != D_INTERNAL_LIST) ) ||
         (_md->indent >= D_INTERNAL_CODE_INDENT) )
    {
        return 0;
    }

    // a bullet, or a number of up to nine digits and `.` or `)`
    if ( (c != '-') &&
         (c != '+') &&
         (c != '*') )
    {
        size_t digits = 0;

        // an ordered item's digits, nine at most
        while ( (digits < 10u) &&
                (d_internal_is_digit(d_internal_peek(_md, at + digits))) )
        {
            digits++;
        }

        marker = d_internal_peek(_md, at + digits);

        // one to nine digits, then `.` or `)`
        if ( (digits < 1u) ||
             (digits > 9u) ||
             ( (marker != '.') && (marker != ')') ) )
        {
            return 0;
        }

        // a number interrupting a paragraph must be 1
        if ( (_md->nodes[_container].type == D_INTERNAL_PARAGRAPH) &&
             ( (digits != 1u) || (_md->text[at] != '1') ) )
        {
            return 0;
        }

        ordered = true;
        length  = digits + 1u;
    }

    const char next = d_internal_peek(_md, at + length);

    // space or the line's end after the marker
    if ( (next != '\0') &&
         (!d_internal_is_space_or_tab(next)) )
    {
        return 0;
    }

    // an empty item may not interrupt a paragraph
    if ( (_md->nodes[_container].type == D_INTERNAL_PARAGRAPH) &&
         (d_internal_rest_is_blank(_md, at + length)) )
    {
        return 0;
    }

    const size_t marker_offset = _md->indent;

    d_internal_advance_nonspace(_md);
    d_internal_advance(_md, length, true);

    const size_t spaces_column = _md->column;
    const size_t spaces_offset = _md->offset;
    size_t       padding       = 0;

    // up to five columns of space after the marker
    do
    {
        d_internal_advance(_md, 1u, true);
    } while ( ((_md->column - spaces_column) < 5u) &&
              (d_internal_is_space_or_tab(d_internal_peek(_md,
                                                          _md->offset))) );

    const bool   blank_item = (_md->offset >= _md->line_end);
    const size_t spaces     = _md->column - spaces_column;

    // five or more, none, or nothing after: the content is a column on
    if ( (spaces >= 5u) ||
         (spaces < 1u) ||
         (blank_item) )
    {
        padding        = length + 1u;
        _md->column    = spaces_column;
        _md->offset    = spaces_offset;
        _md->partial   = false;

        // the content begins after one space
        if (d_internal_is_space_or_tab(d_internal_peek(_md, _md->offset)))
        {
            d_internal_advance(_md, 1u, true);
        }
    }
    else
    {
        padding = length + spaces;
    }

    d_internal_close_unmatched(_md);

    const struct d_internal_node* const tip = &_md->nodes[_md->tip];

    // a new list, unless the tip is one of this kind
    if ( (tip->type != D_INTERNAL_LIST) ||
         (tip->ordered != ordered) ||
         (tip->marker != marker) )
    {
        const size_t list = d_internal_add_node(_md, D_INTERNAL_LIST, at);

        // a list opened for the item, or the reader fails
        if (list == D_WIKI_NONE)
        {
            return 1;
        }

        _md->nodes[list].ordered = ordered;
        _md->nodes[list].marker  = marker;
    }

    const size_t item = d_internal_add_node(_md, D_INTERNAL_ITEM, at);

    // memory that ran out
    if (item == D_WIKI_NONE)
    {
        return 1;
    }

    _md->nodes[item].ordered       = ordered;
    _md->nodes[item].marker        = marker;
    _md->nodes[item].marker_offset = marker_offset;
    _md->nodes[item].padding       = padding;
    _md->wiki->blocks[_md->nodes[item].block].marker = marker;
    _md->wiki->blocks[_md->nodes[item].block].level  = _md->nodes[item].lists;

    return 1;
}


/*
d_internal_cells
  How many cells a table's row has, from `_start` to `_end`: apart by `|`
not escaped, a `|` at either end not one between.  `_out_cells`, when
given, receives each cell's span, trimmed, two to a cell.
*/
static size_t
d_internal_cells(
    const char* _text,
    size_t      _start,
    size_t      _end,
    size_t*     _out_cells,
    size_t      _most
)
{
    size_t count = 0;
    size_t at    = _start;

    // a `|` before the first cell is not between two
    while ( (at < _end) &&
            (d_internal_is_space_or_tab(_text[at])) )
    {
        at++;
    }

    // a row's leading `|`
    if ( (at < _end) &&
         (_text[at] == '|') )
    {
        at++;
    }

    // a cell at a time
    while (at < _end)
    {
        size_t cell = at;

        // to the next `|` not escaped, nor in Gollum's [[...]]
        while ( (at < _end) &&
                (_text[at] != '|') )
        {
            // Gollum's [[...]] is one cell's, its `|` with it
            if ( ((at + 1u) < _end) &&
                 (_text[at] == '[') &&
                 (_text[at + 1u] == '[') )
            {
                size_t close = at + 2u;

                // to the `]]`
                while ( ((close + 1u) < _end) &&
                        ( (_text[close] != ']') ||
                          (_text[close + 1u] != ']') ) )
                {
                    close++;
                }

                // past the link, when it closes
                if ((close + 1u) < _end)
                {
                    at = close + 2u;

                    continue;
                }
            }

            at += ( (_text[at] == '\\') && ((at + 1u) < _end) ) ? 2u : 1u;
        }

        size_t stop = (at < _end) ? at : _end;

        // trimmed
        while ( (cell < stop) &&
                (d_internal_is_space_or_tab(_text[cell])) )
        {
            cell++;
        }

        // the cell's space at its end
        while ( (stop > cell) &&
                (d_internal_is_space_or_tab(_text[stop - 1u])) )
        {
            stop--;
        }

        // the last cell, empty after the last `|`, is not one
        if ( (at >= _end) &&
             (cell == stop) &&
             (count > 0u) )
        {
            break;
        }

        // the cell's span, when asked for
        if ( (_out_cells) &&
             (count < _most) )
        {
            _out_cells[2u * count]      = cell;
            _out_cells[(2u * count) + 1u] = stop;
        }

        count++;

        // past the `|` between cells
        if (at < _end)
        {
            at++;
        }
    }

    return count;
}


/*
d_internal_start_table
  A line of dashes under a paragraph whose last line has as many cells:
that line becomes a table's header, and the paragraph's lines before it
stay a paragraph.
*/
static int
d_internal_start_table(
    struct d_internal_md* _md,
    size_t                _container
)
{
    struct d_internal_node* node = &_md->nodes[_container];

    // under a paragraph the line matched
    if ( (_md->indented) ||
         (node->type != D_INTERNAL_PARAGRAPH) ||
         (node->block == D_WIKI_NONE) ||
         (!_md->all_closed) )
    {
        return 0;
    }

    size_t cells[2u * 64u];
    const size_t columns = d_internal_cells(_md->text, _md->next_nonspace,
                                            _md->line_end, cells, 64u);
    bool         piped   = false;

    // each cell :---:, with at least one `|` on the line
    for (size_t at = _md->next_nonspace; at < _md->line_end; ++at)
    {
        piped = (piped) || (_md->text[at] == '|');
    }

    // no column, or more than a table may have
    if ( (columns == 0u) ||
         (columns > 64u) )
    {
        return 0;
    }

    // each column's delimiter: `:`, dashes, `:`
    for (size_t index = 0; index < columns; ++index)
    {
        size_t at   = cells[2u * index];
        size_t stop = cells[(2u * index) + 1u];

        at   += ( (at < stop) && (_md->text[at] == ':') ) ? 1u : 0u;
        stop -= ( (stop > at) && (_md->text[stop - 1u] == ':') ) ? 1u : 0u;

        // a delimiter with no dash
        if (at >= stop)
        {
            return 0;
        }

        // dashes only
        for (; at < stop; ++at)
        {
            // a byte other than `-`
            if (_md->text[at] != '-')
            {
                return 0;
            }
        }
    }

    struct d_wiki_block* const          block  = &_md->wiki->blocks[
                                                     node->block];
    const struct d_wiki_segment* const  header = &_md->wiki->segments[
                                                     block->first_segment +
                                                     block->segment_count -
                                                     1u];
    bool                                pipes  = piped;

    // the header has a `|`, or the dashes do, and as many cells
    for (size_t at = header->start; at < header->end; ++at)
    {
        pipes = (pipes) || (_md->text[at] == '|');
    }

    // a header with as many cells, and a pipe
    if ( (!pipes) ||
         (d_internal_cells(_md->text, header->start, header->end, NULL, 0u)
              != columns) )
    {
        return 0;
    }

    const size_t header_segment = block->first_segment +
                                  block->segment_count - 1u;
    const size_t header_start   = header->start;

    // the paragraph's lines before the header stay a paragraph
    if (block->segment_count > 1u)
    {
        block->segment_count--;
        node->last_line = _md->line - 2u;
        d_internal_finalize(_md, _container);

        // the table, its first line the header
        const size_t table = d_internal_add_node(_md, D_INTERNAL_TABLE,
                                                 header_start);

        // memory that ran out
        if (table == D_WIKI_NONE)
        {
            return 2;
        }

        _md->wiki->blocks[_md->nodes[table].block].first_segment =
            header_segment;
        _md->wiki->blocks[_md->nodes[table].block].segment_count = 1u;
        node = &_md->nodes[table];
    }
    else
    {
        node->type  = D_INTERNAL_TABLE;
        block->kind = D_WIKI_TABLE;
    }

    node->columns = columns;
    d_internal_add_line(_md);
    _md->offset = _md->line_end;

    return 2;
}


/*
d_internal_start_quote
  >, and one space after it.
*/
static int
d_internal_start_quote(
    struct d_internal_md* _md
)
{
    const size_t at = _md->next_nonspace;

    // `>`, not indented as code
    if ( (_md->indented) ||
         (d_internal_peek(_md, at) != '>') )
    {
        return 0;
    }

    d_internal_advance_nonspace(_md);
    d_internal_advance(_md, 1u, false);

    // the space after `>`, taken
    if (d_internal_is_space_or_tab(d_internal_peek(_md, _md->offset)))
    {
        d_internal_advance(_md, 1u, true);
    }

    d_internal_close_unmatched(_md);
    (void)d_internal_add_node(_md, D_INTERNAL_QUOTE, at);

    return 1;
}


/*
d_internal_start_code
  Four columns of indent, where a paragraph is not open.
*/
static int
d_internal_start_code(
    struct d_internal_md* _md
)
{
    // indented, not blank, with no paragraph to continue
    if ( (!_md->indented) ||
         (_md->blank) ||
         (_md->nodes[_md->tip].type == D_INTERNAL_PARAGRAPH) )
    {
        return 0;
    }

    d_internal_advance(_md, D_INTERNAL_CODE_INDENT, true);
    d_internal_close_unmatched(_md);
    (void)d_internal_add_node(_md, D_INTERNAL_CODE, _md->offset);

    return 2;
}


/*
d_internal_try_starts
  Each kind of block, in CommonMark's order, then GitHub's tables: 1 for a
container begun, 2 for a leaf, 0 for none.
*/
static int
d_internal_try_starts(
    struct d_internal_md* _md,
    size_t                _container
)
{
    int result = d_internal_start_quote(_md);

    // the first that begins
    if (result == 0)
    {
        result = d_internal_start_atx(_md);
    }

    // then a fence
    if (result == 0)
    {
        result = d_internal_start_fence(_md);
    }

    // then HTML
    if (result == 0)
    {
        result = d_internal_start_html(_md, _container);
    }

    // then a setext underline
    if (result == 0)
    {
        result = d_internal_start_setext(_md, _container);
    }

    // then a rule
    if (result == 0)
    {
        result = d_internal_start_rule(_md);
    }

    // then a list's item
    if (result == 0)
    {
        result = d_internal_start_item(_md, _container);
    }

    // then indented code
    if (result == 0)
    {
        result = d_internal_start_code(_md);
    }

    // then a table's delimiter row
    if (result == 0)
    {
        result = d_internal_start_table(_md, _container);
    }

    return result;
}


//==============================================================================
// 9.  CONTINUING
//==============================================================================


/*
d_internal_continue
  Whether a line goes on in a node open: 0 if it does, 1 if not, and 2 if
it closes the node and is done.
  Long as a dispatch on the node's type is.
*/
static int
d_internal_continue(
    struct d_internal_md* _md,
    size_t                _node
)
{
    struct d_internal_node* const node = &_md->nodes[_node];

    // a table of what goes on, by type
    switch (node->type)
    {
        case D_INTERNAL_QUOTE:
            // a quote goes on at a `>` not indented as code
            if ( (_md->indented) ||
                 (d_internal_peek(_md, _md->next_nonspace) != '>') )
            {
                return 1;
            }

            d_internal_advance_nonspace(_md);
            d_internal_advance(_md, 1u, false);

            // the space after `>`, taken
            if (d_internal_is_space_or_tab(d_internal_peek(_md,
                                                           _md->offset)))
            {
                d_internal_advance(_md, 1u, true);
            }

            return 0;

        case D_INTERNAL_ITEM:
            // a blank line goes on in an item that has begun
            if (_md->blank)
            {
                // an item that is empty when it is blank closes
                if (!node->has_child)
                {
                    return 1;
                }

                d_internal_advance_nonspace(_md);

                return 0;
            }

            // an item goes on at its content's indent
            if (_md->indent >= (node->marker_offset + node->padding))
            {
                d_internal_advance(_md, node->marker_offset + node->padding,
                                   true);

                return 0;
            }

            return 1;

        case D_INTERNAL_CODE:
            // a fence goes on to its close
            if (node->fenced)
            {
                size_t end = 0;

                // a closing fence, as long or longer, alone
                if ( (_md->indent <= 3u) &&
                     (d_internal_peek(_md, _md->next_nonspace) ==
                          node->fence) &&
                     (d_internal_run(_md, _md->next_nonspace, node->fence,
                                     false, &end) >= node->fence_length) &&
                     (d_internal_rest_is_blank(_md, end)) )
                {
                    d_internal_mark(_md, _node);
                    d_internal_finalize(_md, _node);

                    return 2;
                }

                // the fence's indent is taken from each line
                for (size_t left = node->fence_offset;
                     ( (left > 0u) &&
                       (d_internal_is_space_or_tab(
                            d_internal_peek(_md, _md->offset))) ); --left)
                {
                    d_internal_advance(_md, 1u, true);
                }

                return 0;
            }

            // indented code goes on at four columns
            if (_md->indent >= D_INTERNAL_CODE_INDENT)
            {
                d_internal_advance(_md, D_INTERNAL_CODE_INDENT, true);

                return 0;
            }

            // and over blank lines
            if (_md->blank)
            {
                d_internal_advance_nonspace(_md);

                return 0;
            }

            return 1;

        case D_INTERNAL_HTML:
            return ( (_md->blank) &&
                     ( (node->html == 6) || (node->html == 7) ) ) ? 1 : 0;

        case D_INTERNAL_PARAGRAPH:
        case D_INTERNAL_TABLE:
            return (_md->blank) ? 1 : 0;

        case D_INTERNAL_HEADING:
        case D_INTERNAL_RULE:
            return 1;

        default:
            return 0;
    }
}


/*
d_internal_maybe_special
  Whether a byte may begin a block, which spares the rest a look.
*/
static bool
d_internal_maybe_special(
    char _c
)
{
    return ( (_c != '\0') &&
             ( (strchr("#`~*+_=<>-|:", _c) != NULL) ||
               (d_internal_is_digit(_c)) ) );
}


/*
d_internal_incorporate
  Reads one line: the containers it goes on in, the blocks it begins, and
the leaf its rest goes to.
  Long as commonmark.js's incorporateLine, which it follows step by step: the
containers matched, the starts tried, then the leaf; split, the state each
step leaves for the next would be passed about by pointer.
*/
static void
d_internal_incorporate(
    struct d_internal_md* _md
)
{
    size_t container   = 0;
    bool   all_matched = true;

    _md->offset  = _md->line_start;
    _md->column  = 0u;
    _md->blank   = false;
    _md->partial = false;
    _md->old_tip = _md->tip;

    // each container open, from the document down, while the line goes on
    while ( (_md->nodes[container].last_child != D_WIKI_NONE) &&
            (_md->nodes[_md->nodes[container].last_child].open) )
    {
        container = _md->nodes[container].last_child;
        d_internal_find_next_nonspace(_md);

        const int result = d_internal_continue(_md, container);

        // a container reaches the line it matches; a leaf, the lines it takes
        if (result == 0)
        {
            // a container matched is on this line
            if ( (_md->nodes[container].type == D_INTERNAL_QUOTE) ||
                 (_md->nodes[container].type == D_INTERNAL_ITEM) ||
                 (_md->nodes[container].type == D_INTERNAL_LIST) )
            {
                _md->nodes[container].last_line = _md->line;
            }

            continue;
        }

        // a closing fence is the whole line
        if (result == 2)
        {
            return;
        }

        all_matched = false;
        container   = _md->nodes[container].parent;

        break;
    }

    (void)all_matched;
    _md->all_closed   = (container == _md->old_tip);
    _md->last_matched = container;

    bool matched_leaf = (_md->nodes[container].type == D_INTERNAL_CODE) ||
                        (_md->nodes[container].type == D_INTERNAL_HTML);

    // blocks begun, a container at a time, until a leaf or the text
    while (!matched_leaf)
    {
        d_internal_find_next_nonspace(_md);

        // the space before a line that begins no block
        if ( (!_md->indented) &&
             (!d_internal_maybe_special(d_internal_peek(_md,
                                                        _md->next_nonspace))) )
        {
            d_internal_advance_nonspace(_md);

            break;
        }

        const int result = d_internal_try_starts(_md, container);

        // memory that ran out
        if (_md->failed)
        {
            return;
        }

        // nothing begins: the rest of the line is the leaf's
        if (result == 0)
        {
            d_internal_advance_nonspace(_md);

            break;
        }

        container = _md->tip;

        // a leaf began: what is left is its content
        if (result == 2)
        {
            matched_leaf = true;
        }
    }

    const enum d_internal_type tip_type = _md->nodes[_md->tip].type;

    // a paragraph's lazy line, which matched not all it is in
    if ( (!_md->all_closed) &&
         (!_md->blank) &&
         (tip_type == D_INTERNAL_PARAGRAPH) )
    {
        d_internal_add_line(_md);

        return;
    }

    d_internal_close_unmatched(_md);

    struct d_internal_node* const node = &_md->nodes[container];

    // a leaf takes the line, unless the line opened it and is done
    if ( (node->type == D_INTERNAL_PARAGRAPH) ||
         (node->type == D_INTERNAL_TABLE) ||
         (node->type == D_INTERNAL_CODE) ||
         (node->type == D_INTERNAL_HTML) )
    {
        // a table's line goes on in it, unless it is blank
        if ( (node->type != D_INTERNAL_TABLE) ||
             (_md->offset < _md->line_end) )
        {
            // the line, unless a start took it
            if (node->skip_line != _md->line)
            {
                d_internal_add_line(_md);
            }
            else
            {
                d_internal_mark(_md, container);
            }
        }

        // an HTML block of the first five kinds ends where it says
        if ( (node->type == D_INTERNAL_HTML) &&
             (node->html >= 1) &&
             (node->html <= 5) &&
             (d_internal_html_closes(_md->text,
                                     (_md->wiki->blocks[node->block].start >=
                                      _md->line_start)
                                         ? _md->wiki->blocks[node->block].start
                                         : _md->line_start,
                                     _md->line_end, node->html)) )
        {
            d_internal_finalize(_md, container);
        }

        return;
    }

    // the rest begins a paragraph
    if ( (_md->offset < _md->line_end) &&
         (!_md->blank) )
    {
        const size_t paragraph = d_internal_add_node(_md,
                                                     D_INTERNAL_PARAGRAPH,
                                                     _md->next_nonspace);

        // the paragraph begun, with the line
        if (paragraph != D_WIKI_NONE)
        {
            d_internal_advance_nonspace(_md);
            d_internal_add_line(_md);
        }
    }

    return;
}


//==============================================================================
// 10.  INLINE CONTENT
//==============================================================================


/*
d_internal_add_inline
  Adds a node of inline content, its decoded text the bytes given.
*/
static size_t
d_internal_add_inline(
    struct d_internal_line*     _line,
    enum d_internal_inline_kind _kind,
    const char*                 _text,
    size_t                      _length
)
{
    // room for one more
    if (!d_internal_grow((void**)&_line->nodes, &_line->node_capacity,
                         _line->node_count, sizeof(struct d_internal_inline)))
    {
        _line->failed = true;

        return D_WIKI_NONE;
    }

    struct d_internal_inline* const node = &_line->nodes[_line->node_count];

    memset(node, 0, sizeof(*node));
    node->kind        = _kind;
    node->decoded     = _line->decoded.length;
    node->length      = _length;
    node->previous    = D_WIKI_NONE;
    node->next        = D_WIKI_NONE;
    node->content_end = D_WIKI_NONE;

    // its decoded text, or memory ran out
    if ( (_length > 0u) &&
         (!d_wiki_buffer_add(&_line->decoded, _text, _length)) )
    {
        _line->failed = true;

        return D_WIKI_NONE;
    }

    return _line->node_count++;
}


/*
d_internal_flush
  Adds the text read since `*_from`, up to `_to`, as a text node.
*/
static void
d_internal_flush(
    struct d_internal_line* _line,
    size_t*                 _from,
    size_t                  _to
)
{
    // nothing read adds nothing
    if (*_from < _to)
    {
        (void)d_internal_add_inline(_line, D_INTERNAL_TEXT,
                                    &_line->joined.text[*_from],
                                    _to - *_from);
    }

    *_from = _to;

    return;
}


/*
d_internal_previous_character, d_internal_next_character
  The character before a place in the joined text, and at one; a line
feed at either end.
*/
static uint32_t
d_internal_previous_character(
    const struct d_internal_line* _line,
    size_t                        _at
)
{
    size_t start = _at;
    size_t bytes = 0;

    // nothing before is a line's end
    if (_at == 0u)
    {
        return '\n';
    }

    // back over continuation bytes, to three
    start--;

    // back to the start of the character before
    while ( (start > 0u) &&
            ((_at - start) < 4u) &&
            ((((unsigned char)_line->joined.text[start]) & 0xC0u) == 0x80u) )
    {
        start--;
    }

    return d_wiki_utf8(&_line->joined.text[start], _at - start, &bytes);
}


static uint32_t
d_internal_next_character(
    const struct d_internal_line* _line,
    size_t                        _at
)
{
    size_t bytes = 0;

    // nothing after is a line's end
    if (_at >= _line->joined.length)
    {
        return '\n';
    }

    return d_wiki_utf8(&_line->joined.text[_at],
                       _line->joined.length - _at, &bytes);
}


/*
d_internal_push_delimiter
  Puts a delimiter run on top of the stack.
*/
static void
d_internal_push_delimiter(
    struct d_internal_line* _line,
    size_t                  _node
)
{
    _line->nodes[_node].previous = _line->top;
    _line->nodes[_node].next     = D_WIKI_NONE;

    // linked after the run on top
    if (_line->top != D_WIKI_NONE)
    {
        _line->nodes[_line->top].next = _node;
    }

    _line->top = _node;

    return;
}


static void
d_internal_remove_delimiter(
    struct d_internal_line* _line,
    size_t                  _node
)
{
    struct d_internal_inline* const node = &_line->nodes[_node];

    // the runs either side meet
    if (node->previous != D_WIKI_NONE)
    {
        _line->nodes[node->previous].next = node->next;
    }

    // its next linked to its previous
    if (node->next != D_WIKI_NONE)
    {
        _line->nodes[node->next].previous = node->previous;
    }
    else
    {
        _line->top = node->previous;
    }

    node->previous = D_WIKI_NONE;
    node->next     = D_WIKI_NONE;

    return;
}


/*
d_internal_delimiter_run
  A run of `*`, `_` or `~`: whether it can open, or close, by what is on
either side of it.  Tildes of three or more are text.
*/
static size_t
d_internal_delimiter_run(
    struct d_internal_line* _line,
    size_t                  _at
)
{
    const char* const text  = _line->joined.text;
    const char        c     = text[_at];
    size_t            end   = _at;

    // the run
    while ( (end < _line->joined.length) &&
            (text[end] == c) )
    {
        end++;
    }

    const size_t count = end - _at;

    // three tildes or more strike nothing
    if ( (c == '~') &&
         (count > 2u) )
    {
        (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, &text[_at],
                                    count);

        return end;
    }

    const uint32_t before       = d_internal_previous_character(_line, _at);
    const uint32_t after        = d_internal_next_character(_line, end);
    const bool     before_space = d_wiki_is_space(before);
    const bool     after_space  = d_wiki_is_space(after);
    const bool     before_punct = d_wiki_is_punctuation(before);
    const bool     after_punct  = d_wiki_is_punctuation(after);
    const bool     left         = (!after_space) &&
                                  ( (!after_punct) || (before_space) ||
                                    (before_punct) );
    const bool     right        = (!before_space) &&
                                  ( (!before_punct) || (after_space) ||
                                    (after_punct) );
    const size_t   node         = d_internal_add_inline(_line,
                                                        D_INTERNAL_DELIMITER,
                                                        NULL, 0u);

    // memory that ran out
    if (node == D_WIKI_NONE)
    {
        return end;
    }

    struct d_internal_inline* const run = &_line->nodes[node];

    run->c        = c;
    run->count    = count;
    run->original = count;

    // `_` may not open or close within a word
    if (c == '_')
    {
        run->can_open  = (left) && ( (!right) || (before_punct) );
        run->can_close = (right) && ( (!left) || (after_punct) );
    }
    else
    {
        run->can_open  = left;
        run->can_close = right;
    }

    // a run that may open or close is pushed
    if ( (run->can_open) ||
         (run->can_close) )
    {
        d_internal_push_delimiter(_line, node);
    }

    return end;
}


/*
d_internal_emphasis
  Matches the delimiter runs above `_bottom`, as CommonMark does, taking
from each run what a match uses; what is left is text.
  Long as CommonMark's process emphasis is, which it follows step by step.
*/
static void
d_internal_emphasis(
    struct d_internal_line* _line,
    size_t                  _bottom
)
{
    size_t openers_bottom[3][6];
    size_t closer = _line->top;

    // the lowest an opener may be, by kind of closer
    for (size_t kind = 0; kind < 3u; ++kind)
    {
        // every bottom at the delimiter given
        for (size_t index = 0; index < 6u; ++index)
        {
            openers_bottom[kind][index] = _bottom;
        }
    }

    // the first run above the bottom
    while ( (closer != D_WIKI_NONE) &&
            (closer != _bottom) &&
            (_line->nodes[closer].previous != _bottom) )
    {
        closer = _line->nodes[closer].previous;
    }

    // none above the bottom
    if (closer == _bottom)
    {
        closer = D_WIKI_NONE;
    }

    // each closer, up the stack
    while (closer != D_WIKI_NONE)
    {
        struct d_internal_inline* const end = &_line->nodes[closer];

        // a run that cannot close: the next
        if (!end->can_close)
        {
            closer = end->next;

            continue;
        }

        const size_t kind  = (end->c == '*') ? 0u : (end->c == '_') ? 1u : 2u;
        const size_t which = ((end->can_open) ? 3u : 0u) + (end->original % 3u);
        size_t       opener = end->previous;
        bool         found  = false;

        // the nearest opener it may close
        while ( (opener != D_WIKI_NONE) &&
                (opener != _bottom) &&
                (opener != openers_bottom[kind][which]) )
        {
            const struct d_internal_inline* const begin =
                &_line->nodes[opener];
            const bool odd = ( (end->can_open) || (begin->can_close) ) &&
                             ((end->original % 3u) != 0u) &&
                             (((begin->original + end->original) % 3u) == 0u);

            // an opener of the same byte, by the rule of three
            if ( (begin->c == end->c) &&
                 (begin->can_open) &&
                 ( ( (kind == 2u) && (begin->count == end->count) ) ||
                   ( (kind != 2u) && (!odd) ) ) )
            {
                found = true;

                break;
            }

            opener = begin->previous;
        }

        // none: the closer may not close below here again
        if (!found)
        {
            const size_t next = end->next;

            openers_bottom[kind][which] = end->previous;

            // a closer that cannot open too is let go
            if (!end->can_open)
            {
                d_internal_remove_delimiter(_line, closer);
            }

            closer = next;

            continue;
        }

        struct d_internal_inline* const begin = &_line->nodes[opener];
        const size_t                    used  = (kind == 2u) ? end->count
                                              : ( (end->count >= 2u) &&
                                                  (begin->count >= 2u) )
                                                    ? 2u : 1u;

        begin->count -= used;
        end->count   -= used;

        // the runs between can match nothing now
        while (begin->next != closer)
        {
            d_internal_remove_delimiter(_line, begin->next);
        }

        // an opener used up is let go
        if (begin->count == 0u)
        {
            d_internal_remove_delimiter(_line, opener);
        }

        // a closer used up: the next
        if (end->count == 0u)
        {
            const size_t next = end->next;

            d_internal_remove_delimiter(_line, closer);
            closer = next;
        }
    }

    // nothing above the bottom matches any more
    while ( (_line->top != D_WIKI_NONE) &&
            (_line->top != _bottom) )
    {
        d_internal_remove_delimiter(_line, _line->top);
    }

    return;
}


/*
d_internal_render
  Writes inline content as the text it shows, from node `_from` to `_to`:
an image shows its text only within another's, `_alt`.
*/
static void
d_internal_render(
    const struct d_internal_line* _line,
    size_t                        _from,
    size_t                        _to,
    bool                          _alt,
    struct d_wiki_buffer*         _out
)
{
    size_t at = _from;

    // each node, an image's content skipped unless in an image
    while (at < _to)
    {
        const struct d_internal_inline* const node = &_line->nodes[at];

        // what each node shows
        switch (node->kind)
        {
            case D_INTERNAL_TEXT:
            case D_INTERNAL_SPAN:
                (void)d_wiki_buffer_add(_out,
                                        &_line->decoded.text[node->decoded],
                                        node->length);
                break;

            case D_INTERNAL_BREAK:
                (void)d_wiki_buffer_add_char(_out, '\n');
                break;

            case D_INTERNAL_DELIMITER:
                // the delimiters left are text
                for (size_t count = 0; count < node->count; ++count)
                {
                    (void)d_wiki_buffer_add_char(_out, node->c);
                }

                break;

            case D_INTERNAL_OPENER:
                (void)d_wiki_buffer_add_text(_out, (node->image) ? "![" : "[");
                break;

            case D_INTERNAL_LINK:
                // an image shows its alternative text, as text
                if ( (node->image) &&
                     (node->content_end != D_WIKI_NONE) )
                {
                    // within another image's alternative text, its own content
                    if (_alt)
                    {
                        d_internal_render(_line, at + 1u, node->content_end,
                                          true, _out);
                    }

                    at = node->content_end;

                    continue;
                }

                break;

            default:
                break;
        }

        at++;
    }

    return;
}


/*
d_internal_keep_link
  Adds a link found inline, its destination decoded already and its place
on the page from places in the joined text.
*/
static size_t
d_internal_keep_link(
    struct d_internal_line* _line,
    struct d_wiki_link*     _link,
    bool                    _image
)
{
    _link->block = _line->block;

    // where it leads, then the link
    if (!d_internal_classify(_link, _image))
    {
        _line->failed = true;
    }

    const size_t index = (_line->failed) ? D_WIKI_NONE
                       : d_wiki_add_link(_line->md->wiki, _link);

    // memory that ran out
    if (index == D_WIKI_NONE)
    {
        free(_link->target);
        free(_link->page);
        free(_link->anchor);
        free(_link->label);
        _line->failed = true;
    }

    return index;
}


/*
d_internal_attribute_value
  An attribute's value from `_at`: quoted, to its close, or to the space,
the `>` or the `/>` after it.  Gives where the attribute ends.
*/
static size_t
d_internal_attribute_value(
    const char*                 _text,
    size_t                      _close,
    size_t                      _at,
    struct d_internal_html_tag* _out
)
{
    size_t to = _at;

    // a quoted value, to its close
    if ( (_at < _close) &&
         ( (_text[_at] == '"') || (_text[_at] == '\'') ) )
    {
        const char quote = _text[_at];

        _out->from = _at + 1u;
        to         = _out->from;

        // to its close
        while ( (to < _close) &&
                (_text[to] != quote) )
        {
            to++;
        }

        _out->to = to;

        return to + 1u;
    }

    _out->from = _at;

    // an unquoted value, to the space or the `>` after it
    while ( (to < _close) &&
            (!d_internal_is_space_or_tab(_text[to])) &&
            (_text[to] != '>') &&
            (_text[to] != '\n') )
    {
        to++;
    }

    // a `/` before the `>` closes the tag, and is not the value's
    to      -= ( (to > _at) && (_text[to - 1u] == '/') &&
                 (_text[to] == '>') ) ? 1u : 0u;
    _out->to = to;

    return to;
}


/*
d_internal_attribute
  The attribute of a tag at `*_scan`, before `_close`: its name, `name` to
`name_end`, and its value, `from` to `to`, unquoted, D_WIKI_NONE for an
attribute with none.  Moves `*_scan` past it; gives false when the tag's
attributes are done.
*/
static bool
d_internal_attribute(
    const char*                 _text,
    size_t                      _close,
    size_t*                     _scan,
    struct d_internal_html_tag* _out
)
{
    size_t scan = *_scan;

    // past the space before an attribute
    while ( (scan < _close) &&
            ( (d_internal_is_space_or_tab(_text[scan])) ||
              (_text[scan] == '\n') ) )
    {
        scan++;
    }

    _out->name = scan;

    // the attribute's name
    while ( (scan < _close) &&
            ( (d_internal_is_alnum(_text[scan])) ||
              (strchr("_.:-", _text[scan]) != NULL) ) )
    {
        scan++;
    }

    _out->name_end = scan;
    _out->from     = D_WIKI_NONE;
    _out->to       = D_WIKI_NONE;

    // no name: the tag's attributes are done
    if (_out->name_end == _out->name)
    {
        return false;
    }

    // past the space before `=`
    while ( (scan < _close) &&
            (d_internal_is_space_or_tab(_text[scan])) )
    {
        scan++;
    }

    // an attribute with no value
    if ( (scan >= _close) ||
         (_text[scan] != '=') )
    {
        *_scan = scan;

        return true;
    }

    scan++;

    // past the space after `=`
    while ( (scan < _close) &&
            (d_internal_is_space_or_tab(_text[scan])) )
    {
        scan++;
    }

    *_scan = d_internal_attribute_value(_text, _close, scan, _out);

    return true;
}


/*
d_internal_html_link
  The link an <a href> or an <img src> makes, kept within a leaf: where it
leads, and, for an image, its alt as its label.
*/
static void
d_internal_html_link(
    struct d_internal_line*           _line,
    const char*                       _text,
    size_t                            _base,
    const struct d_internal_html_tag* _tag
)
{
    struct d_wiki_link   kept;
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };
    struct d_wiki_buffer label  = { NULL, 0u, 0u, false };

    d_wiki_new_link(&kept, D_WIKI_FORM_HTML,
                    (_line) ? d_internal_source(_line, _tag->at)
                            : (_base + _tag->at),
                    (_line) ? d_internal_source(_line, _tag->close)
                            : (_base + _tag->close));
    kept.target_start = (_line) ? d_internal_source(_line, _tag->value)
                                : (_base + _tag->value);
    kept.target_end   = (_line) ? d_internal_source(_line, _tag->stop)
                                : (_base + _tag->stop);

    (void)d_internal_unescape(&target, &_text[_tag->value],
                              _tag->stop - _tag->value);

    // an image's alt is its label
    if (_tag->alt != D_WIKI_NONE)
    {
        (void)d_internal_unescape(&label, &_text[_tag->alt],
                                  _tag->alt_end - _tag->alt);
    }
    else
    {
        (void)d_wiki_buffer_add(&label, "", 0u);
    }

    kept.target = d_wiki_buffer_take(&target);
    kept.label  = d_wiki_buffer_take(&label);

    // the link kept, within a leaf; else let go
    if ( (_line) &&
         (kept.target) &&
         (kept.label) )
    {
        (void)d_internal_keep_link(_line, &kept, _tag->image);
    }
    else
    {
        free(kept.target);
        free(kept.label);
    }

    return;
}


/*
d_internal_html_links
  The links of HTML in a text, `_start` to `_end`: each <a href> and each
<img src>, an image's alt its label.  Within a leaf, `_line` places them on
the page; else the text is the page's from `_base`.
*/
static void
d_internal_html_links(
    struct d_internal_line* _line,
    const char*             _text,
    size_t                  _start,
    size_t                  _end,
    size_t                  _base
)
{
    size_t at = _start;

    // each tag from `<a` or `<img`
    while (at < _end)
    {
        const char* const          found = memchr(&_text[at], '<', _end - at);
        struct d_internal_html_tag tag;

        // the next `<`
        if (!found)
        {
            break;
        }

        at = (size_t)(found - _text);
        memset(&tag, 0, sizeof(tag));
        tag.at    = at;
        tag.image = d_internal_starts_with(&_text[at], _end - at, "<img");

        const bool   link  = d_internal_starts_with(&_text[at], _end - at,
                                                    "<a");
        const size_t name  = (tag.image) ? 4u : 2u;

        // <a or <img, then space, and a tag that closes
        tag.close = ( ( (tag.image) || (link) ) &&
                      ((at + name) < _end) &&
                      (d_internal_is_space_or_tab(_text[at + name])) )
                        ? d_internal_tag(_text, _end, at, true) : 0u;

        // a `<` that opens no such tag is text
        if (tag.close == 0u)
        {
            at++;

            continue;
        }

        const char* const wanted = (tag.image) ? "src" : "href";
        size_t            scan   = at + name;

        tag.value = D_WIKI_NONE;
        tag.alt   = D_WIKI_NONE;

        // each attribute with a value, for the one wanted and an image's alt
        while (d_internal_attribute(_text, tag.close, &scan, &tag))
        {
            const size_t span = tag.name_end - tag.name;

            // an attribute with no value
            if (tag.from == D_WIKI_NONE)
            {
                continue;
            }

            // the attribute wanted, href or src, and an image's alt
            if ( (span == strlen(wanted)) &&
                 (d_internal_starts_with(&_text[tag.name], span, wanted)) )
            {
                tag.value = tag.from;
                tag.stop  = tag.to;
            }
            else if ( (tag.image) &&
                      (span == 3u) &&
                      (d_internal_starts_with(&_text[tag.name], span, "alt")) )
            {
                tag.alt     = tag.from;
                tag.alt_end = tag.to;
            }
        }

        // a tag with what it leads to is a link
        if (tag.value != D_WIKI_NONE)
        {
            d_internal_html_link(_line, _text, _base, &tag);
        }

        at = tag.close;
    }

    return;
}


/*
d_internal_autolink
  <scheme:...> or <address>: gives where it ends, 0 for neither.
*/
static size_t
d_internal_autolink(
    const char* _text,
    size_t      _length,
    size_t      _at,
    bool*       _out_mail
)
{
    size_t at = _at + 1u;

    // a scheme: a letter, then 1 to 31 more, then `:`
    if ( (at < _length) &&
         (d_internal_is_alpha(_text[at])) )
    {
        size_t scheme = 1u;

        at++;

        // the scheme: a letter, then letters, digits, `+`, `.` and `-`
        while ( (at < _length) &&
                (scheme < 32u) &&
                ( (d_internal_is_alnum(_text[at])) ||
                  (strchr("+.-", _text[at]) != NULL) ) )
        {
            at++;
            scheme++;
        }

        // two to thirty-two of them, then `:`
        if ( (scheme >= 2u) &&
             (at < _length) &&
             (_text[at] == ':') )
        {
            at++;

            // to `>`, with no space, control or `<`
            while ( (at < _length) &&
                    ((unsigned char)_text[at] > 0x20u) &&
                    (_text[at] != '<') &&
                    (_text[at] != '>') )
            {
                at++;
            }

            // a URL's `>`
            if ( (at < _length) &&
                 (_text[at] == '>') )
            {
                *_out_mail = false;

                return at + 1u;
            }
        }
    }

    at = _at + 1u;

    // an address: its local part, `@`, labels apart by `.`
    while ( (at < _length) &&
            ( (d_internal_is_alnum(_text[at])) ||
              (strchr(".!#$%&'*+/=?^_`{|}~-", _text[at]) != NULL) ) )
    {
        at++;
    }

    // an address: a local part, then `@`
    if ( (at == (_at + 1u)) ||
         (at >= _length) ||
         (_text[at] != '@') )
    {
        return 0u;
    }

    at++;

    // each label: alphanumeric at either end, dashes between, to 63
    do
    {
        const size_t label = at;

        // a label of the domain: letters, digits and `-`
        while ( (at < _length) &&
                ((at - label) < 63u) &&
                ( (d_internal_is_alnum(_text[at])) || (_text[at] == '-') ) )
        {
            at++;
        }

        // a label begins and ends with a letter or a digit
        if ( (at == label) ||
             (!d_internal_is_alnum(_text[label])) ||
             (!d_internal_is_alnum(_text[at - 1u])) )
        {
            return 0u;
        }

        // another label, after `.`
        if ( (at < _length) &&
             (_text[at] == '.') )
        {
            at++;

            continue;
        }

        break;
    } while (at < _length);

    // an address's `>`
    if ( (at < _length) &&
         (_text[at] == '>') )
    {
        *_out_mail = true;

        return at + 1u;
    }

    return 0u;
}


/*
d_internal_raw_html
  Inline HTML at `_at`: a tag, a comment, an instruction, a declaration,
CDATA.  Gives where it ends, 0 for none.
*/
static size_t
d_internal_raw_html(
    const char* _text,
    size_t      _length,
    size_t      _at
)
{
    const char* const at   = &_text[_at];
    const size_t      left = _length - _at;

    // a comment: <!-->, <!--->, or <!-- ... -->
    if (d_internal_starts_with(at, left, "<!--"))
    {
        // `<!-->` is a comment, empty
        if (d_internal_starts_with(at, left, "<!-->"))
        {
            return _at + 5u;
        }

        // so is `<!--->`
        if (d_internal_starts_with(at, left, "<!--->"))
        {
            return _at + 6u;
        }

        // else to its `-->`
        for (size_t scan = 4u; (scan + 2u) < left; ++scan)
        {
            // the comment's close
            if (strncmp(&at[scan], "-->", 3u) == 0)
            {
                return _at + scan + 3u;
            }
        }

        return 0u;
    }

    // an instruction: <? ... ?>
    if (d_internal_starts_with(at, left, "<?"))
    {
        // to the `?>`
        for (size_t scan = 2u; (scan + 1u) < left; ++scan)
        {
            // the instruction's close
            if ( (at[scan] == '?') &&
                 (at[scan + 1u] == '>') )
            {
                return _at + scan + 2u;
            }
        }

        return 0u;
    }

    // CDATA: <![CDATA[ ... ]]>
    if ( (left >= 9u) &&
         (strncmp(at, "<![CDATA[", 9u) == 0) )
    {
        // to the `]]>`
        for (size_t scan = 9u; (scan + 2u) < left; ++scan)
        {
            // the section's close
            if (strncmp(&at[scan], "]]>", 3u) == 0)
            {
                return _at + scan + 3u;
            }
        }

        return 0u;
    }

    // a declaration: <!X ... >
    if ( (left > 2u) &&
         (at[1] == '!') &&
         (d_internal_is_alpha(at[2])) )
    {
        const char* const close = memchr(&at[2], '>', left - 2u);

        return (close) ? ((size_t)(close - _text) + 1u) : 0u;
    }

    return d_internal_tag(_text, _length, _at, true);
}


/*
d_internal_check_domain
  The length of a domain at `_text`: letters, digits, `-`, `_` and `.`, no
`_` in its last two parts, and, unless short is allowed, a `.`.
*/
static size_t
d_internal_check_domain(
    const char* _text,
    size_t      _length,
    bool        _short
)
{
    size_t at          = 1u;
    size_t dots        = 0;
    size_t underscore1 = 0;
    size_t underscore2 = 0;

    // too short to hold one
    if (_length < 2u)
    {
        return 0u;
    }

    // each byte after the first, to the last but one, as cmark-gfm reads
    for (; at < (_length - 1u); ++at)
    {
        // an escaped byte is the domain's
        if ( (_text[at] == '\\') &&
             (at < (_length - 2u)) )
        {
            at++;
        }

        // an underscore, counted for the labels it may not be in
        if (_text[at] == '_')
        {
            underscore2++;
        }
        else if (_text[at] == '.')
        {
            underscore1 = underscore2;
            underscore2 = 0;
            dots++;
        }
        else if ( (!d_internal_is_alnum(_text[at])) &&
                  (_text[at] != '-') &&
                  ((unsigned char)_text[at] < 0x80u) )
        {
            break;
        }
    }

    // an underscore in either of the last two parts
    if ( (underscore1 > 0u) ||
         (underscore2 > 0u) )
    {
        return 0u;
    }

    return ( (_short) || (dots > 0u) ) ? at : 0u;
}


/*
d_internal_link_end
  Where a bare link ends: before `<`, and before what trails it --
punctuation, a `)` with no `(` to match, a reference's `;`.
*/
static size_t
d_internal_link_end(
    const char* _text,
    size_t      _end
)
{
    // a `<` ends it
    for (size_t at = 0; at < _end; ++at)
    {
        // a link ends before `<`
        if (_text[at] == '<')
        {
            _end = at;

            break;
        }
    }

    // what trails it
    while (_end > 0u)
    {
        const char last = _text[_end - 1u];

        // punctuation at its end is not the link's
        if (strchr("?!.,:*_~'\"", last) != NULL)
        {
            _end--;
        }
        else if (last == ';')
        {
            size_t at = _end - 1u;

            // an entity at its end, `&name;`, is not the link's
            while ( (at > 0u) &&
                    (d_internal_is_alpha(_text[at - 1u])) )
            {
                at--;
            }

            _end = ( (at > 0u) && (at < (_end - 1u)) &&
                     (_text[at - 1u] == '&') ) ? (at - 1u) : (_end - 1u);
        }
        else if (last == ')')
        {
            size_t opening = 0;
            size_t closing = 0;

            // a `)` past those opened is not the link's
            for (size_t at = 0; at < _end; ++at)
            {
                opening += (_text[at] == '(') ? 1u : 0u;
                closing += (_text[at] == ')') ? 1u : 0u;
            }

            // balanced: the end stays
            if (closing <= opening)
            {
                break;
            }

            _end--;
        }
        else
        {
            break;
        }
    }

    return _end;
}


/*
d_internal_bare_link
  Keeps a bare link from `_at` to `_end` of the joined text, its target
the text with a scheme before it.
*/
static void
d_internal_bare_link(
    struct d_internal_line* _line,
    size_t                  _at,
    size_t                  _end,
    const char*             _scheme
)
{
    struct d_wiki_link   link;
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };
    const char* const    text   = &_line->joined.text[_at];
    const size_t         length = _end - _at;

    d_wiki_new_link(&link, D_WIKI_FORM_BARE, d_internal_source(_line, _at),
                    d_internal_source(_line, _end));
    link.target_start = link.start;
    link.target_end   = link.end;

    (void)d_wiki_buffer_add_text(&target, _scheme);
    (void)d_wiki_buffer_add(&target, text, length);
    link.target = d_wiki_buffer_take(&target);
    link.label  = d_wiki_copy(text, length);

    // memory that ran out
    if ( (!link.target) ||
         (!link.label) )
    {
        free(link.target);
        free(link.label);
        _line->failed = true;

        return;
    }

    (void)d_internal_keep_link(_line, &link, false);
    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, text, length);

    return;
}


/*
d_internal_www
  www.domain, after a space, the start, or *, _, ~, (; gives where it
ends, or 0.
*/
static size_t
d_internal_www(
    const struct d_internal_line* _line,
    size_t                        _at
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;

    // after what may stand before a link
    if ( (_at > 0u) &&
         (strchr("*_~(", text[_at - 1u]) == NULL) &&
         (!d_wiki_is_space(d_internal_previous_character(_line, _at))) )
    {
        return 0u;
    }

    // `www.`, at a word's start
    if ( ((length - _at) < 4u) ||
         (strncmp(&text[_at], "www.", 4u) != 0) )
    {
        return 0u;
    }

    size_t end = d_internal_check_domain(&text[_at], length - _at, false);

    // a domain after it
    if (end == 0u)
    {
        return 0u;
    }

    end += _at;

    // the rest, to space or `<`
    while ( (end < length) &&
            (!d_internal_is_space_or_tab(text[end])) &&
            (text[end] != '\n') &&
            (text[end] != '<') )
    {
        end++;
    }

    end = _at + d_internal_link_end(&text[_at], end - _at);

    return (end > _at) ? end : 0u;
}


/*
d_internal_url
  http://, https:// or ftp://, its scheme the letters before `:` at
`_at`, and a domain: gives where it ends, or 0, and where it begins.
*/
static size_t
d_internal_url(
    const struct d_internal_line* _line,
    size_t                        _from,
    size_t                        _at,
    size_t*                       _out_start
)
{
    static const char* const schemes[] = { "http://", "https://", "ftp://" };
    const char* const        text      = _line->joined.text;
    const size_t             length    = _line->joined.length;
    size_t                   start     = _at;

    // `://`
    if ( ((length - _at) < 4u) ||
         (text[_at + 1u] != '/') ||
         (text[_at + 2u] != '/') )
    {
        return 0u;
    }

    // the scheme's letters, within the text read
    while ( (start > _from) &&
            (d_internal_is_alpha(text[start - 1u])) )
    {
        start--;
    }

    bool safe = false;

    // a scheme of the three, and a letter or digit after it
    for (size_t index = 0; index < 3u; ++index)
    {
        const size_t span = strlen(schemes[index]);

        // a scheme GitHub links, then a letter, a digit or more
        if ( ((length - start) > span) &&
             (d_internal_starts_with(&text[start], length - start,
                                     schemes[index])) &&
             (d_internal_is_alnum(text[start + span]) ||
              ((unsigned char)text[start + span] >= 0x80u)) )
        {
            safe = true;
        }
    }

    // no scheme
    if (!safe)
    {
        return 0u;
    }

    size_t end = _at + 3u;
    size_t domain = d_internal_check_domain(&text[end], length - end, true);

    // a domain after `://`
    if (domain == 0u)
    {
        return 0u;
    }

    end += domain;

    // the rest, to space or `<`
    while ( (end < length) &&
            (!d_internal_is_space_or_tab(text[end])) &&
            (text[end] != '\n') &&
            (text[end] != '<') )
    {
        end++;
    }

    end = start + d_internal_link_end(&text[start], end - start);

    // nothing after the scheme
    if (end <= (_at + 3u))
    {
        return 0u;
    }

    *_out_start = start;

    return end;
}


/*
d_internal_email
  An address around the `@` at `_at`, from the text read: gives where it
ends, or 0, and where it begins.
*/
static size_t
d_internal_email(
    const struct d_internal_line* _line,
    size_t                        _from,
    size_t                        _at,
    size_t*                       _out_start
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    size_t            start  = _at;
    size_t            end    = _at;
    size_t            ats    = 0;
    size_t            dots   = 0;

    // the local part, back from the `@`
    while ( (start > _from) &&
            ( (d_internal_is_alnum(text[start - 1u])) ||
              (strchr(".+-_", text[start - 1u]) != NULL) ) )
    {
        start--;
    }

    // no local part before `@`
    if (start == _at)
    {
        return 0u;
    }

    // the domain
    for (; end < length; ++end)
    {
        const char c = text[end];

        // a letter or a digit
        if (d_internal_is_alnum(c))
        {
            continue;
        }

        // one `@`
        if (c == '@')
        {
            ats++;
        }
        else if ( (c == '.') &&
                  ((end + 1u) < length) &&
                  (d_internal_is_alnum(text[end + 1u])) )
        {
            dots++;
        }
        else if ( (c != '-') &&
                  (c != '_') )
        {
            break;
        }
    }

    // a domain with a dot, ending in a letter or a dot
    if ( ((end - _at) < 2u) ||
         (ats != 1u) ||
         (dots == 0u) ||
         ( (!d_internal_is_alpha(text[end - 1u])) &&
           (text[end - 1u] != '.') ) )
    {
        return 0u;
    }

    end = _at + d_internal_link_end(&text[_at], end - _at);

    // nothing after the local part
    if (end <= (_at + 1u))
    {
        return 0u;
    }

    *_out_start = start;

    return end;
}


/*
d_internal_gollum_close
  Where Gollum's [[...]] at `_at` closes: its `]]`, on its line, with
something between, not escaped by a `'` before it, and with no `][`
within it.  Gives 0 for none.
*/
static size_t
d_internal_gollum_close(
    const struct d_internal_line* _line,
    size_t                        _at
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    size_t            close  = _at + 2u;

    // [[, then ]] on the line
    while ( ((close + 1u) < length) &&
            (text[close] != '\n') &&
            ( (text[close] != ']') || (text[close + 1u] != ']') ) )
    {
        close++;
    }

    // a `]]` that closes it, with something between
    if ( ((close + 1u) >= length) ||
         (text[close] != ']') ||
         (close == (_at + 2u)) )
    {
        return 0u;
    }

    // a `'` before, and none after, escapes it
    if ( (_at > 0u) &&
         (text[_at - 1u] == '\'') &&
         ( ((close + 2u) >= length) || (text[close + 2u] != '\'') ) )
    {
        return 0u;
    }

    // `][` within it: not Gollum's
    for (size_t scan = _at + 2u; (scan + 1u) < close; ++scan)
    {
        // `][` is not Gollum's
        if ( (text[scan] == ']') &&
             (text[scan + 1u] == '[') )
        {
            return 0u;
        }
    }

    return close;
}


/*
d_internal_gollum_parts
  The parts of [[...]] between `_inner` and its `]]` at `_close`, apart by
`|` and trimmed, four at most.  Gives how many.
*/
static size_t
d_internal_gollum_parts(
    const char* _text,
    size_t      _inner,
    size_t      _close,
    size_t      _out_parts[4][2]
)
{
    size_t count = 0;
    size_t from  = _inner;

    // each part, ended by `|` or the close
    for (size_t scan = _inner; scan <= _close; ++scan)
    {
        size_t start = from;
        size_t stop  = scan;

        // a part ends at `|` or the close
        if ( (scan != _close) &&
             (_text[scan] != '|') )
        {
            continue;
        }

        // the part's space at its start
        while ( (start < stop) &&
                (d_internal_is_space_or_tab(_text[start])) )
        {
            start++;
        }

        // and its space at its end
        while ( (stop > start) &&
                (d_internal_is_space_or_tab(_text[stop - 1u])) )
        {
            stop--;
        }

        // four parts at most
        if (count < 4u)
        {
            _out_parts[count][0] = start;
            _out_parts[count][1] = stop;
            count++;
        }

        from = scan + 1u;
    }

    return count;
}


/*
d_internal_gollum_label
  The text [[...]] shows: an image's alt=, among its options; the first
part, before the page, of [[Label|Page]], written where it is; or the page
of [[Page]].
*/
static void
d_internal_gollum_label(
    const struct d_internal_line* _line,
    size_t                        _parts[4][2],
    size_t                        _count,
    bool                          _image,
    struct d_wiki_link*           _link
)
{
    const char* const    text  = _line->joined.text;
    struct d_wiki_buffer label = { NULL, 0u, 0u, false };

    // an image's text is its option alt=
    for (size_t index = 1u; ( (_image) && (index < _count) ); ++index)
    {
        // the option alt=
        if ( ((_parts[index][1] - _parts[index][0]) > 4u) &&
             (strncmp(&text[_parts[index][0]], "alt=", 4u) == 0) )
        {
            (void)d_internal_unescape(&label, &text[_parts[index][0] + 4u],
                                      _parts[index][1] - _parts[index][0] -
                                      4u);
        }
    }

    // a label written, or the page
    if ( (!_image) &&
         (_count >= 2u) )
    {
        _link->label_start = d_internal_source(_line, _parts[0][0]);
        _link->label_end   = d_internal_source(_line, _parts[0][1]);
    }

    // the label, or the page
    if (!_image)
    {
        (void)d_internal_unescape(&label, &text[_parts[0][0]],
                                  _parts[0][1] - _parts[0][0]);
    }

    (void)d_wiki_buffer_add(&label, "", 0u);
    _link->label = d_wiki_buffer_take(&label);

    return;
}


/*
d_internal_gollum_kept
  What a Gollum link kept leads to: [[_TOC_]] is a tag, and a page's name
keeps its spaces and `/`s, perhaps with an anchor; then the text it shows.
*/
static void
d_internal_gollum_kept(
    struct d_internal_line* _line,
    size_t                  _index,
    bool                    _image,
    bool                    _include
)
{
    struct d_wiki_link* const kept = &_line->md->wiki->links[_index];
    const char* const         hash = strchr(kept->target, '#');

    kept->embedded = _image;

    // [[_TOC_]] is a tag; a page, perhaps at an anchor, is neither an
    // image, a URL nor a file the wiki's root names
    if ( (!_include) &&
         (strcmp(kept->target, "_TOC_") == 0) )
    {
        kept->to = D_WIKI_TO_TAG;
    }
    else if ( (!_image) &&
              (!d_wiki_has_scheme(kept->target)) &&
              ( (kept->target[0] != '/') ||
                (kept->to != D_WIKI_TO_FILE) ) )
    {
        const size_t name = (hash) ? (size_t)(hash - kept->target)
                                   : strlen(kept->target);

        kept->to = (name == 0u) ? D_WIKI_TO_ANCHOR : D_WIKI_TO_PAGE;

        // its page and its anchor, or the line fails
        if ( (!d_internal_set_text(&kept->page, kept->target, name)) ||
             (!d_internal_set_text(&kept->anchor, (hash) ? (hash + 1) : "",
                                   (hash) ? strlen(hash + 1) : 0u)) )
        {
            _line->failed = true;
        }
    }

    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, kept->label,
                                strlen(kept->label));

    return;
}


/*
d_internal_gollum
  Gollum's [[...]] at `_at`, on one line, not escaped by a `'` before it:
[[Page]], [[Label|Page]], an image's [[file.png|alt=Text]], [[_TOC_]] and
[[include:Page]].  Gives where it ends, and keeps it, or 0.
*/
static size_t
d_internal_gollum(
    struct d_internal_line* _line,
    size_t                  _at
)
{
    const char* const text  = _line->joined.text;
    const size_t      close = d_internal_gollum_close(_line, _at);
    size_t            parts[4][2];
    struct d_wiki_link link;

    // no close: not Gollum's
    if (close == 0u)
    {
        return 0u;
    }

    const size_t count = d_internal_gollum_parts(text, _at + 2u, close,
                                                 parts);
    const bool   image = d_wiki_is_image(&text[parts[0][0]],
                                         parts[0][1] - parts[0][0]);
    const size_t which = ( (image) || (count < 2u) ) ? 0u : 1u;
    const size_t target = parts[which][0];
    const size_t span   = parts[which][1] - parts[which][0];
    const bool   include = d_internal_starts_with(&text[target], span,
                                                  "include:");

    // where it leads: the page, past include:'s `:`
    d_wiki_new_link(&link, D_WIKI_FORM_WIKI, d_internal_source(_line, _at),
                    d_internal_source(_line, close + 2u));
    link.target_start = d_internal_source(_line, target +
                                                 ((include) ? 8u : 0u));
    link.target_end   = d_internal_source(_line, parts[which][1]);
    link.target       = d_wiki_copy(&text[target + ((include) ? 8u : 0u)],
                                    span - ((include) ? 8u : 0u));
    d_internal_gollum_label(_line, parts, count, image, &link);

    // memory that ran out
    if ( (!link.target) ||
         (!link.label) )
    {
        free(link.target);
        free(link.label);
        _line->failed = true;

        return 0u;
    }

    const size_t index = d_internal_keep_link(_line, &link, image);

    // what it leads to, and the text it shows
    if (index != D_WIKI_NONE)
    {
        d_internal_gollum_kept(_line, index, image, include);
    }

    return close + 2u;
}


/*
d_internal_push_bracket
  Opens a bracket: `[`, or `![`.
*/
static void
d_internal_push_bracket(
    struct d_internal_line* _line,
    size_t                  _at,
    bool                    _image
)
{
    const size_t node = d_internal_add_inline(_line, D_INTERNAL_OPENER,
                                              NULL, 0u);

    // room for one more
    if ( (node == D_WIKI_NONE) ||
         (!d_internal_grow((void**)&_line->brackets,
                           &_line->bracket_capacity, _line->bracket_count,
                           sizeof(struct d_internal_bracket))) )
    {
        _line->failed = true;

        return;
    }

    _line->nodes[node].image = _image;

    // a bracket opened after another says so to it
    if (_line->bracket_count > 0u)
    {
        _line->brackets[_line->bracket_count - 1u].after = true;
    }

    struct d_internal_bracket* const bracket =
        &_line->brackets[_line->bracket_count++];

    bracket->node      = node;
    bracket->at        = _at;
    bracket->image     = _image;
    bracket->active    = true;
    bracket->after     = false;
    bracket->delimiter = _line->top;

    return;
}


/*
d_internal_inline_link
  ](destination "title") after the bracket at `_close`: gives where it ends,
or 0, and where its destination and title are.
*/
static size_t
d_internal_inline_link(
    const struct d_internal_line* _line,
    size_t                        _close,
    struct d_internal_link_parts* _parts
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    size_t            at     = _close + 1u;

    // `(`
    if ( (at >= length) ||
         (text[at] != '(') )
    {
        return 0u;
    }

    at = d_internal_skip_spaces(text, length, at + 1u, NULL);

    _parts->raw   = at;
    _parts->title = D_WIKI_NONE;

    // the destination, perhaps empty
    if (!d_internal_destination(text, length, at, &at, &_parts->start,
                                &_parts->stop))
    {
        return 0u;
    }

    _parts->raw_end = at;

    const size_t before = at;

    at = d_internal_skip_spaces(text, length, at, NULL);

    // a title, after space
    if ( (at > before) &&
         (d_internal_title(text, length, at, &_parts->title_end)) )
    {
        _parts->title = at;
        at            = d_internal_skip_spaces(text, length,
                                               _parts->title_end, NULL);
    }

    return ( (at < length) && (text[at] == ')') ) ? (at + 1u) : 0u;
}


/*
d_internal_link_after
  What follows the `]` at `_at` of a bracket: an inline link's destination,
or a reference's label -- full, collapsed, or the bracket's own text for a
shortcut -- that names a definition.  Gives whether either makes a link.
*/
static bool
d_internal_link_after(
    struct d_internal_line*          _line,
    size_t                           _at,
    const struct d_internal_bracket* _opener,
    struct d_internal_link_parts*    _parts
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;

    memset(_parts, 0, sizeof(*_parts));
    _parts->reference   = D_WIKI_NONE;
    _parts->label_from  = D_WIKI_NONE;
    _parts->label_to    = D_WIKI_NONE;
    _parts->title       = D_WIKI_NONE;
    _parts->end         = d_internal_inline_link(_line, _at, _parts);
    _parts->inline_form = (_parts->end > 0u);

    // inline, or else a reference: full, collapsed or a shortcut
    if (_parts->inline_form)
    {
        return true;
    }

    const size_t label = d_internal_label_length(text, length, _at + 1u);

    // a full or collapsed reference's label
    if (label > 2u)
    {
        _parts->label_from = _at + 2u;
        _parts->label_to   = _at + label;
        _parts->reference  = d_internal_find_reference(
                                 _line->md, &text[_parts->label_from],
                                 _parts->label_to - _parts->label_from);
        _parts->end        = _at + 1u + label;
    }
    else if (!_opener->after)
    {
        const size_t from = _opener->at + ((_opener->image) ? 2u : 1u);

        _parts->reference = d_internal_find_reference(_line->md, &text[from],
                                                      _at - from);
        _parts->end       = _at + 1u + label;
    }

    return (_parts->reference != D_WIKI_NONE);
}


/*
d_internal_link_of
  The link a bracket and what follows its `]` make: where it is on the
page, where it leads -- as written, or as its definition says -- and an
empty label, which its content gives once emphasis is read.  Gives whether
memory held.
*/
static bool
d_internal_link_of(
    struct d_internal_line*             _line,
    size_t                              _at,
    const struct d_internal_bracket*    _opener,
    const struct d_internal_link_parts* _parts,
    struct d_wiki_link*                 _out
)
{
    const char* const text = _line->joined.text;

    d_wiki_new_link(_out, (_parts->inline_form) ? D_WIKI_FORM_INLINE
                                                : D_WIKI_FORM_REFERENCE,
                    d_internal_source(_line, _opener->at),
                    d_internal_source(_line, _parts->end));
    _out->label_start = d_internal_source(_line, _opener->at +
                                                 ((_opener->image) ? 2u : 1u));
    _out->label_end   = d_internal_source(_line, _at);

    // where it leads: written, or its definition's
    if (_parts->inline_form)
    {
        struct d_wiki_buffer target = { NULL, 0u, 0u, false };

        _out->target_start = d_internal_source(_line, _parts->raw);
        _out->target_end   = d_internal_source(_line, _parts->raw_end);

        // its title, written
        if (_parts->title != D_WIKI_NONE)
        {
            _out->title_start = d_internal_source(_line, _parts->title);
            _out->title_end   = d_internal_source(_line, _parts->title_end);
        }

        (void)d_internal_unescape(&target, &text[_parts->start],
                                  _parts->stop - _parts->start);
        _out->target = d_wiki_buffer_take(&target);
    }
    else
    {
        const struct d_wiki_link* const definition =
            &_line->md->wiki->links[_parts->reference];

        // a label written after it is where its destination is named
        if (_parts->label_from != D_WIKI_NONE)
        {
            _out->target_start = d_internal_source(_line, _parts->label_from);
            _out->target_end   = d_internal_source(_line, _parts->label_to);
        }

        _out->definition = _parts->reference;
        _out->target     = d_wiki_copy(definition->target,
                                       strlen(definition->target));
    }

    _out->label = d_wiki_copy("", 0u);

    // what memory would not hold is let go
    if ( (!_out->target) ||
         (!_out->label) )
    {
        free(_out->target);
        free(_out->label);

        return false;
    }

    return true;
}


/*
d_internal_link_made
  The bracket a link closed made the link's node: its content read for
emphasis, its label left to wait for the text it shows, and, for a link,
every `[` before it unable to open another.
*/
static void
d_internal_link_made(
    struct d_internal_line*          _line,
    const struct d_internal_bracket* _opener,
    size_t                           _index
)
{
    // the opener is the link now; its content is read for emphasis
    _line->nodes[_opener->node].kind        = D_INTERNAL_LINK;
    _line->nodes[_opener->node].content_end = _line->node_count;
    d_internal_emphasis(_line, _opener->delimiter);
    _line->bracket_count--;

    // the label waits for the text it shows
    if ( (_index != D_WIKI_NONE) &&
         (d_internal_grow((void**)&_line->waiting, &_line->waiting_capacity,
                          _line->waiting_count,
                          sizeof(struct d_internal_waiting))) )
    {
        _line->waiting[_line->waiting_count].link  = _index;
        _line->waiting[_line->waiting_count].first = _opener->node + 1u;
        _line->waiting[_line->waiting_count].end   = _line->node_count;
        _line->waiting_count++;
    }

    // a link holds no other: every `[` before it opens nothing now
    for (size_t at = 0;
         ( (!_opener->image) && (at < _line->bracket_count) );
         ++at)
    {
        // every `[` before it, but an image's
        if (!_line->brackets[at].image)
        {
            _line->brackets[at].active = false;
        }
    }

    return;
}


/*
d_internal_close_bracket
  A `]` at `_at`: closes the top bracket as a link or an image if what
follows makes one, or as text.  Gives where reading goes on.
*/
static size_t
d_internal_close_bracket(
    struct d_internal_line* _line,
    size_t                  _at
)
{
    struct d_internal_link_parts parts;
    struct d_wiki_link           link;

    // no bracket open: `]` is text
    if (_line->bracket_count == 0u)
    {
        (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, "]", 1u);

        return _at + 1u;
    }

    const struct d_internal_bracket opener =
        _line->brackets[_line->bracket_count - 1u];

    // an opener that may not begin a link, or nothing after the `]` that
    // makes one: the bracket is text, as is the `]`
    if ( (!opener.active) ||
         (!d_internal_link_after(_line, _at, &opener, &parts)) )
    {
        _line->bracket_count--;
        (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, "]", 1u);

        return _at + 1u;
    }

    // the link, kept; what memory would not hold is the line's failure
    if (!d_internal_link_of(_line, _at, &opener, &parts, &link))
    {
        _line->failed = true;

        return parts.end;
    }

    d_internal_link_made(_line, &opener,
                         d_internal_keep_link(_line, &link, opener.image));

    return parts.end;
}


/*
d_internal_code_span
  A run of backticks at `_at`: a code span to a run of as many, or text.
Gives where reading goes on.
*/
static size_t
d_internal_code_span(
    struct d_internal_line* _line,
    size_t                  _at
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    size_t            end    = _at;

    // the opening run
    while ( (end < length) &&
            (text[end] == '`') )
    {
        end++;
    }

    const size_t count = end - _at;
    size_t       scan  = end;

    // a closing run of as many
    while (scan < length)
    {
        // to the next run of backticks
        if (text[scan] != '`')
        {
            scan++;

            continue;
        }

        size_t run = scan;

        // the run's length
        while ( (run < length) &&
                (text[run] == '`') )
        {
            run++;
        }

        // a run as long closes the span
        if ((run - scan) == count)
        {
            struct d_wiki_buffer content = { NULL, 0u, 0u, false };
            size_t               from    = end;
            size_t               to      = scan;
            bool                 spaces  = true;

            // line feeds are spaces
            for (size_t at = from; at < to; ++at)
            {
                (void)d_wiki_buffer_add_char(&content,
                                             (text[at] == '\n') ? ' '
                                                                : text[at]);
                spaces = (spaces) &&
                         ( (text[at] == ' ') || (text[at] == '\n') );
            }

            // one space each side goes, when both are and not all is
            if ( (content.length >= 2u) &&
                 (!spaces) &&
                 (content.text[0] == ' ') &&
                 (content.text[content.length - 1u] == ' ') )
            {
                (void)d_internal_add_inline(_line, D_INTERNAL_SPAN,
                                            &content.text[1],
                                            content.length - 2u);
            }
            else
            {
                (void)d_internal_add_inline(_line, D_INTERNAL_SPAN,
                                            content.text, content.length);
            }

            d_wiki_buffer_free(&content);

            return run;
        }

        scan = run;
    }

    // no close: the run is text
    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, &text[_at], count);

    return end;
}


/*
d_internal_angle
  A `<` at `_at`: an autolink, HTML, or text.  Gives where reading goes on.
*/
static size_t
d_internal_angle(
    struct d_internal_line* _line,
    size_t                  _at
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    bool              mail   = false;
    const size_t      auto_end = d_internal_autolink(text, length, _at,
                                                     &mail);

    // an autolink
    if (auto_end > 0u)
    {
        struct d_wiki_link   link;
        struct d_wiki_buffer target = { NULL, 0u, 0u, false };

        d_wiki_new_link(&link, D_WIKI_FORM_AUTO,
                        d_internal_source(_line, _at),
                        d_internal_source(_line, auto_end));
        link.target_start = d_internal_source(_line, _at + 1u);
        link.target_end   = d_internal_source(_line, auto_end - 1u);

        (void)d_wiki_buffer_add_text(&target, (mail) ? "mailto:" : "");
        (void)d_wiki_buffer_add(&target, &text[_at + 1u],
                                auto_end - _at - 2u);
        link.target = d_wiki_buffer_take(&target);
        link.label  = d_wiki_copy(&text[_at + 1u], auto_end - _at - 2u);

        // memory that ran out
        if ( (!link.target) ||
             (!link.label) )
        {
            free(link.target);
            free(link.label);
            _line->failed = true;

            return auto_end;
        }

        (void)d_internal_keep_link(_line, &link, false);
        (void)d_internal_add_inline(_line, D_INTERNAL_SPAN, &text[_at + 1u],
                                    auto_end - _at - 2u);

        return auto_end;
    }

    const size_t html_end = d_internal_raw_html(text, length, _at);

    // HTML, perhaps a link
    if (html_end > 0u)
    {
        d_internal_html_links(_line, text, _at, html_end, 0u);
        (void)d_internal_add_inline(_line, D_INTERNAL_RAW, NULL, 0u);

        return html_end;
    }

    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT, "<", 1u);

    return _at + 1u;
}


/*
d_internal_line_feed
  A line feed at `_at`: a hard break after two spaces, else soft; the
spaces before it, and after it, go.
*/
static size_t
d_internal_line_feed(
    struct d_internal_line* _line,
    size_t*                 _from,
    size_t                  _at
)
{
    const char* const text   = _line->joined.text;
    size_t            end    = _at;

    // the spaces before it, in the text read
    while ( (end > *_from) &&
            (text[end - 1u] == ' ') )
    {
        end--;
    }

    d_internal_flush(_line, _from, end);
    (void)d_internal_add_inline(_line, D_INTERNAL_BREAK, NULL, 0u);

    size_t at = _at + 1u;

    // the spaces after it
    while ( (at < _line->joined.length) &&
            (text[at] == ' ') )
    {
        at++;
    }

    *_from = at;

    return at;
}


/*
d_internal_emoji
  :name: at `_at`, of a name GitHub has an emoji for: gives where it ends,
or 0, and adds its text to `_add`, when given, in place of the name.
*/
static size_t
d_internal_emoji(
    const struct d_internal_line* _line,
    size_t                        _at,
    struct d_internal_line*       _add
)
{
    const char* const    text   = _line->joined.text;
    const size_t         length = _line->joined.length;
    size_t               close  = _at + 1u;
    struct d_wiki_buffer emoji  = { NULL, 0u, 0u, false };

    // a word's bytes, `+` and `-`, to the next `:`
    while ( (close < length) &&
            ((close - _at) <= 64u) &&
            ( (d_internal_is_alnum(text[close])) ||
              (strchr("_+-", text[close]) != NULL) ) )
    {
        close++;
    }

    // a name GitHub has an emoji for, between `:`s
    if ( (close >= length) ||
         (text[close] != ':') ||
         (close == (_at + 1u)) ||
         (!d_wiki_emoji(&text[_at + 1u], close - _at - 1u, &emoji)) )
    {
        d_wiki_buffer_free(&emoji);

        return 0u;
    }

    // the emoji, to the line given
    if (_add)
    {
        (void)d_internal_add_inline(_add, D_INTERNAL_TEXT, emoji.text,
                                    emoji.length);
    }

    d_wiki_buffer_free(&emoji);

    return close + 1u;
}


/*
d_internal_scan
  Reads a leaf's joined text left to right, into nodes and links.
  Long as a dispatch on the byte read is: each case is a few lines, and apart
they would be read in no order.
*/
static void
d_internal_scan(
    struct d_internal_line* _line
)
{
    const char* const text   = _line->joined.text;
    const size_t      length = _line->joined.length;
    size_t            from   = 0;
    size_t            at     = 0;

    // a byte at a time, the special ones each read as what they begin
    while ( (at < length) &&
            (!_line->failed) )
    {
        const char c     = text[at];
        size_t     start = 0;
        size_t     end   = 0;

        // what each byte may begin
        switch (c)
        {
            case '\n':
                at = d_internal_line_feed(_line, &from, at);

                continue;

            case '\\':
                d_internal_flush(_line, &from, at);

                // a hard line break: a backslash before a line ending
                if ( ((at + 1u) < length) &&
                     (text[at + 1u] == '\n') )
                {
                    (void)d_internal_add_inline(_line, D_INTERNAL_BREAK,
                                                NULL, 0u);
                    at += 2u;

                    // past the space after it
                    while ( (at < length) &&
                            (text[at] == ' ') )
                    {
                        at++;
                    }
                }
                else if ( ((at + 1u) < length) &&
                          (d_internal_is_ascii_punctuation(text[at + 1u])) )
                {
                    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT,
                                                &text[at + 1u], 1u);
                    at += 2u;
                }
                else
                {
                    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT,
                                                "\\", 1u);
                    at++;
                }

                from = at;

                continue;

            case '`':
                d_internal_flush(_line, &from, at);
                at   = d_internal_code_span(_line, at);
                from = at;

                continue;

            case '*':
            case '_':
            case '~':
                d_internal_flush(_line, &from, at);
                at   = d_internal_delimiter_run(_line, at);
                from = at;

                continue;

            case '[':
                d_internal_flush(_line, &from, at);

                // Gollum's, before a bracket
                if ( ((at + 1u) < length) &&
                     (text[at + 1u] == '[') )
                {
                    end = d_internal_gollum(_line, at);

                    // Gollum's link, before a bracket opens
                    if (end > 0u)
                    {
                        at   = end;
                        from = at;

                        continue;
                    }
                }

                d_internal_push_bracket(_line, at, false);
                at++;
                from = at;

                continue;

            case '!':
                // `![` opens an image
                if ( ((at + 1u) < length) &&
                     (text[at + 1u] == '[') )
                {
                    d_internal_flush(_line, &from, at);
                    d_internal_push_bracket(_line, at, true);
                    at  += 2u;
                    from = at;

                    continue;
                }

                break;

            case ']':
                d_internal_flush(_line, &from, at);
                at   = d_internal_close_bracket(_line, at);
                from = at;

                continue;

            case '<':
                d_internal_flush(_line, &from, at);
                at   = d_internal_angle(_line, at);
                from = at;

                continue;

            case '&':
            {
                struct d_wiki_buffer decoded = { NULL, 0u, 0u, false };
                const size_t         span    = d_wiki_entity(&text[at],
                                                             length - at,
                                                             &decoded);

                // a reference decoded, after the text before it
                if (span > 0u)
                {
                    d_internal_flush(_line, &from, at);
                    (void)d_internal_add_inline(_line, D_INTERNAL_TEXT,
                                                decoded.text, decoded.length);
                    at  += span;
                    from = at;
                }

                d_wiki_buffer_free(&decoded);

                // past it
                if (span > 0u)
                {
                    continue;
                }

                break;
            }

            case ':':
                // a URL, where no bracket is open
                if (_line->bracket_count == 0u)
                {
                    end = d_internal_url(_line, from, at, &start);

                    // the link, after the text before it
                    if (end > 0u)
                    {
                        d_internal_flush(_line, &from, start);
                        d_internal_bare_link(_line, start, end, "");
                        at   = end;
                        from = at;

                        continue;
                    }
                }

                // GitHub's :emoji:, after the text before it
                end = d_internal_emoji(_line, at, NULL);

                // the emoji, after the text before it
                if (end > 0u)
                {
                    d_internal_flush(_line, &from, at);
                    (void)d_internal_emoji(_line, at, _line);
                    from = end;
                    at   = end;

                    continue;
                }

                break;

            case 'w':
                // `www.`, where no bracket is open
                if (_line->bracket_count == 0u)
                {
                    end = d_internal_www(_line, at);

                    // the link, after the text before it, its target with
                    // http://
                    if (end > 0u)
                    {
                        d_internal_flush(_line, &from, at);
                        d_internal_bare_link(_line, at, end, "http://");
                        at   = end;
                        from = at;

                        continue;
                    }
                }

                break;

            case '@':
                // an address, where no bracket is open
                if (_line->bracket_count == 0u)
                {
                    end = d_internal_email(_line, from, at, &start);

                    // the link, after the text before it, its target with
                    // mailto:
                    if (end > 0u)
                    {
                        d_internal_flush(_line, &from, start);
                        d_internal_bare_link(_line, start, end, "mailto:");
                        at   = end;
                        from = at;

                        continue;
                    }
                }

                break;

            default:
                break;
        }

        at++;
    }

    d_internal_flush(_line, &from, length);

    return;
}


/*
d_internal_read_inline
  Reads one leaf's inline content: its links kept, and a heading's title.
*/
static void
d_internal_read_inline(
    struct d_internal_md*        _md,
    const struct d_internal_job* _job,
    size_t                       _first,
    size_t                       _count,
    bool                         _cell
)
{
    struct d_internal_line line;

    memset(&line, 0, sizeof(line));
    line.md    = _md;
    line.block = _job->block;
    line.top   = D_WIKI_NONE;

    // a cell is joined with its escaped `|`s unescaped
    if (_cell)
    {
        size_t from = _first;

        // a table's cell: its `\|` is a `|` within Gollum's link
        for (size_t at = _first; at < _count; ++at)
        {
            // `\|`: the backslash dropped
            if ( (_md->text[at] == '\\') &&
                 ((at + 1u) < _count) &&
                 (_md->text[at + 1u] == '|') )
            {
                (void)d_internal_join_add(&line, from, at - from);
                from = at + 1u;
                at++;
            }
        }

        (void)d_internal_join_add(&line, from, _count - from);
        (void)d_wiki_buffer_add(&line.joined, "", 0u);
    }
    else
    {
        (void)d_internal_join_block(&line, _first, _count);
    }

    // read, then emphasis
    if (!line.joined.failed)
    {
        d_internal_scan(&line);
        d_internal_emphasis(&line, D_WIKI_NONE);
    }

    // each link's label, now emphasis is read
    for (size_t index = 0; index < line.waiting_count; ++index)
    {
        struct d_wiki_buffer label = { NULL, 0u, 0u, false };
        struct d_wiki_link*  link  = &_md->wiki->links[line.waiting[index]
                                                           .link];

        d_internal_render(&line, line.waiting[index].first,
                          line.waiting[index].end, true, &label);
        (void)d_wiki_buffer_add(&label, "", 0u);

        char* const text = d_wiki_buffer_take(&label);

        // the label's text, for the one it had
        if (text)
        {
            free(link->label);
            link->label = text;
        }
        else
        {
            line.failed = true;
        }
    }

    // a heading's title is the text it shows
    if (_job->heading != D_WIKI_NONE)
    {
        struct d_wiki_buffer title = { NULL, 0u, 0u, false };

        d_internal_render(&line, 0u, line.node_count, false, &title);
        (void)d_wiki_buffer_add(&title, "", 0u);
        _md->wiki->headings[_job->heading].title = d_wiki_buffer_take(&title);
        line.failed = (line.failed) ||
                      (!_md->wiki->headings[_job->heading].title);
    }

    _md->failed = (_md->failed) || (line.failed) || (line.joined.failed) ||
                  (line.decoded.failed);
    d_wiki_buffer_free(&line.joined);
    d_wiki_buffer_free(&line.decoded);
    free(line.pieces);
    free(line.nodes);
    free(line.brackets);
    free(line.waiting);

    return;
}


/*
d_internal_read_job
  A leaf's inline content: a paragraph's or heading's lines joined; a
table's rows, each cell on its own, the line of dashes not among them, and
no more cells than the header has.
*/
static void
d_internal_read_job(
    struct d_internal_md*        _md,
    const struct d_internal_job* _job
)
{
    const struct d_wiki_block* const block = &_md->wiki->blocks[_job->block];

    // a paragraph's or a heading's lines, together
    if (block->kind != D_WIKI_TABLE)
    {
        d_internal_read_inline(_md, _job, block->first_segment,
                               block->segment_count, false);

        return;
    }

    // a table's rows, a cell at a time
    for (size_t row = 0; row < block->segment_count; ++row)
    {
        const struct d_wiki_segment* const segment =
            &_md->wiki->segments[block->first_segment + row];
        size_t                             cells[2u * 64u];

        // the delimiter row holds no content
        if (row == 1u)
        {
            continue;
        }

        const size_t count = d_internal_cells(_md->text, segment->start,
                                              segment->end, cells, 64u);

        // each cell, to as many as the header has
        for (size_t cell = 0; ( (cell < count) &&
                                (cell < _job->columns) &&
                                (cell < 64u) ); ++cell)
        {
            d_internal_read_inline(_md, _job, cells[2u * cell],
                                   cells[(2u * cell) + 1u], true);
        }
    }

    return;
}


/*
d_internal_html_block_links
  The links in an HTML block's tags, a line at a time.
*/
static void
d_internal_html_block_links(
    struct d_internal_md* _md,
    size_t                _block
)
{
    const struct d_wiki_block block = _md->wiki->blocks[_block];

    // each line of the block
    for (size_t index = 0; index < block.segment_count; ++index)
    {
        const struct d_wiki_segment segment =
            _md->wiki->segments[block.first_segment + index];
        struct d_internal_line      line;

        memset(&line, 0, sizeof(line));
        line.md    = _md;
        line.block = _block;
        line.top   = D_WIKI_NONE;
        (void)d_internal_join_add(&line, segment.start,
                                  segment.end - segment.start);
        (void)d_wiki_buffer_add(&line.joined, "", 0u);

        // the block's links, read as HTML's
        if (!line.joined.failed)
        {
            d_internal_html_links(&line, line.joined.text, 0u,
                                  line.joined.length, 0u);
        }

        _md->failed = (_md->failed) || (line.failed) || (line.joined.failed);
        d_wiki_buffer_free(&line.joined);
        free(line.pieces);
    }

    return;
}


/*
d_internal_is_comment
  Whether an HTML block is a comment alone on its lines.
*/
static bool
d_internal_is_comment(
    const struct d_internal_md* _md,
    const struct d_wiki_block*  _block
)
{
    const char* const text = _md->text;
    size_t            end  = _block->end;

    // <!-- at its start
    if ( ((_block->start + 4u) > end) ||
         (strncmp(&text[_block->start], "<!--", 4u) != 0) )
    {
        return false;
    }

    // --> at its end, and only one
    while ( (end > _block->start) &&
            ( (text[end - 1u] == '\n') || (text[end - 1u] == '\r') ||
              (d_internal_is_space_or_tab(text[end - 1u])) ) )
    {
        end--;
    }

    // a comment that ends its block
    if ( ((end - _block->start) < 7u) ||
         (strncmp(&text[end - 3u], "-->", 3u) != 0) )
    {
        return false;
    }

    // and holds no other close
    for (size_t at = _block->start + 4u; (at + 3u) < end; ++at)
    {
        // another `-->` within it
        if (strncmp(&text[at], "-->", 3u) == 0)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_is_fence_line
  Whether a line is a metadata block's fence: `---`, or `...` to close,
with nothing after it but space.
*/
static bool
d_internal_is_fence_line(
    const struct d_internal_md* _md,
    size_t                      _line,
    bool                        _closing
)
{
    const struct d_wiki* const wiki  = _md->wiki;
    size_t                     at    = wiki->line_starts[_line] +
                                       ( ( (_line == 0u) && (wiki->bom) )
                                             ? 3u : 0u);
    const size_t               end   = ((_line + 1u) < wiki->line_count)
                                           ? wiki->line_starts[_line + 1u]
                                           : wiki->length;

    // three of one byte
    if ( ((at + 3u) > end) ||
         ( (strncmp(&_md->text[at], "---", 3u) != 0) &&
           ( (!_closing) || (strncmp(&_md->text[at], "...", 3u) != 0) ) ) )
    {
        return false;
    }

    // then only space and the line's ending
    for (at += 3u; at < end; ++at)
    {
        // a byte other than space after the fence
        if ( (!d_internal_is_space_or_tab(_md->text[at])) &&
             (_md->text[at] != '\n') &&
             (_md->text[at] != '\r') )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_metadata
  A page's metadata, YAML between a first line `---` and the next `---` or
`...`, as Gollum and pandoc read it: a block of its own, its lines between
the fences.  Gives the line the page's blocks begin on.
*/
static size_t
d_internal_metadata(
    struct d_internal_md* _md
)
{
    struct d_wiki* const wiki  = _md->wiki;
    size_t               close = 1u;

    // `---` on the first line, with a line after it
    if ( (wiki->line_count < 2u) ||
         (!d_internal_is_fence_line(_md, 0u, false)) )
    {
        return 0u;
    }

    // the closing fence
    while ( (close < wiki->line_count) &&
            (!d_internal_is_fence_line(_md, close, true)) )
    {
        close++;
    }

    // no closing line: no front matter
    if (close >= wiki->line_count)
    {
        return 0u;
    }

    const size_t block = d_wiki_add_block(wiki, D_WIKI_METADATA,
                                          (wiki->bom) ? 3u : 0u);

    // memory that ran out
    if (block == D_WIKI_NONE)
    {
        _md->failed = true;

        return wiki->line_count;
    }

    // each line between the fences, its ending not its own
    for (size_t line = 1u; line < close; ++line)
    {
        size_t       end   = wiki->line_starts[line + 1u];
        const size_t start = wiki->line_starts[line];

        // the line without its ending
        while ( (end > start) &&
                ( (_md->text[end - 1u] == '\n') ||
                  (_md->text[end - 1u] == '\r') ) )
        {
            end--;
        }

        // the line, or the reader fails
        if (!d_wiki_add_segment(wiki, start, end, 0u))
        {
            _md->failed = true;

            return wiki->line_count;
        }
    }

    wiki->blocks[block].end = d_internal_line_after(_md, close);

    return close + 1u;
}


/*
d_internal_read
  The driver's reader: the blocks, a line at a time; then each leaf's
inline content, and the links of HTML blocks.
  Long as the reader's passes are many, each a few lines, in the order they
must run.
*/
static bool
d_internal_read(
    struct d_wiki* _wiki
)
{
    struct d_internal_md md;

    memset(&md, 0, sizeof(md));
    md.wiki = _wiki;
    md.text = _wiki->text;

    // the document, the tree's root
    md.nodes = malloc(16u * sizeof(struct d_internal_node));

    // memory that ran out
    if (!md.nodes)
    {
        return false;
    }

    md.node_capacity = 16u;
    md.node_count    = 1u;
    memset(&md.nodes[0], 0, sizeof(md.nodes[0]));
    md.nodes[0].type       = D_INTERNAL_DOCUMENT;
    md.nodes[0].parent     = D_WIKI_NONE;
    md.nodes[0].last_child = D_WIKI_NONE;
    md.nodes[0].block      = D_WIKI_NONE;
    md.nodes[0].open       = true;
    md.nodes[0].skip_line  = D_WIKI_NONE;
    md.nodes[0].heading    = D_WIKI_NONE;
    md.tip                 = 0u;

    // each line after any metadata; a last empty line is not one
    for (size_t index = d_internal_metadata(&md);
         ( (index < _wiki->line_count) && (!md.failed) ); ++index)
    {
        md.line       = index;
        md.line_start = _wiki->line_starts[index];
        md.line_end   = ((index + 1u) < _wiki->line_count)
                            ? _wiki->line_starts[index + 1u]
                            : _wiki->length;

        // the line's ending is not its own
        if ( (md.line_end > md.line_start) &&
             (md.text[md.line_end - 1u] == '\n') )
        {
            md.line_end--;
        }

        // a line without its CR
        if ( (md.line_end > md.line_start) &&
             (md.text[md.line_end - 1u] == '\r') )
        {
            md.line_end--;
        }

        // the byte-order mark is read past
        if ( (index == 0u) &&
             (_wiki->bom) )
        {
            md.line_start = 3u;
        }

        // a last line that is empty is no line
        if ( ((index + 1u) == _wiki->line_count) &&
             (md.line_start == _wiki->length) &&
             (index > 0u) )
        {
            break;
        }

        d_internal_incorporate(&md);
    }

    // everything still open closes at the end
    while ( (!md.failed) &&
            (md.tip != 0u) )
    {
        d_internal_finalize(&md, md.tip);
    }

    // a comment standing alone, and the links of other HTML blocks
    for (size_t index = 0; ( (!md.failed) && (index < _wiki->block_count) );
         ++index)
    {
        // an HTML block
        if (_wiki->blocks[index].kind != D_WIKI_HTML)
        {
            continue;
        }

        // a comment alone on its lines is a comment's block
        if (d_internal_is_comment(&md, &_wiki->blocks[index]))
        {
            _wiki->blocks[index].kind = D_WIKI_COMMENT;

            continue;
        }

        d_internal_html_block_links(&md, index);
    }

    // each leaf's inline content, in order
    for (size_t index = 0; ( (!md.failed) && (index < md.job_count) );
         ++index)
    {
        d_internal_read_job(&md, &md.jobs[index]);
    }

    // a heading never read still has a title
    for (size_t index = 0; ( (!md.failed) && (index < _wiki->heading_count) );
         ++index)
    {
        // a heading with no title read has an empty one
        if (!_wiki->headings[index].title)
        {
            _wiki->headings[index].title = d_wiki_copy("", 0u);
            md.failed = (_wiki->headings[index].title == NULL);
        }
    }

    // each definition's key
    for (size_t index = 0; index < md.reference_count; ++index)
    {
        free(md.references[index].key);
    }

    free(md.references);
    free(md.jobs);
    free(md.nodes);

    return !md.failed;
}


//==============================================================================
// 11.  ANCHORS
//==============================================================================


/*
d_internal_anchor
  GitHub's anchor of a title: in lower case, every character not a word's,
a dash or a space dropped, each space a dash.
*/
static bool
d_internal_anchor(
    struct d_wiki_buffer* _out,
    const char*           _title
)
{
    const size_t length = strlen(_title);
    size_t       at     = 0;

    // each character
    while (at < length)
    {
        size_t         bytes     = 0;
        const uint32_t character = d_wiki_utf8(&_title[at], length - at,
                                               &bytes);

        at += bytes;

        // a space is a dash
        if (character == ' ')
        {
            (void)d_wiki_buffer_add_char(_out, '-');
        }
        else if ( (character == '-') ||
                  (d_wiki_is_word(character)) )
        {
            (void)d_wiki_add_utf8(_out, d_wiki_lower(character));
        }
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


//==============================================================================
// 12.  WRITING
//==============================================================================


/*
d_internal_add_lines
  Writes text with every line ending a line feed and each line after the
first indented.
*/
static void
d_internal_add_lines(
    struct d_wiki_buffer* _out,
    const char*           _text,
    size_t                _indent
)
{
    // each byte, a line feed followed by the indent
    for (const char* at = _text; *at != '\0'; ++at)
    {
        // a CR is dropped
        if (*at == '\r')
        {
            continue;
        }

        (void)d_wiki_buffer_add_char(_out, *at);

        // each line after the first, indented
        if ( (*at == '\n') &&
             (at[1] != '\0') )
        {
            // the indent
            for (size_t column = 0; column < _indent; ++column)
            {
                (void)d_wiki_buffer_add_char(_out, ' ');
            }
        }
    }

    return;
}


static bool
d_internal_write_heading(
    struct d_wiki_buffer* _out,
    size_t                _level,
    const char*           _title
)
{
    const size_t length = strlen(_title);

    // the level's #s
    for (size_t count = 0; count < _level; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, '#');
    }

    (void)d_wiki_buffer_add_char(_out, ' ');

    // the title on one line
    for (size_t at = 0; at < length; ++at)
    {
        (void)d_wiki_buffer_add_char(_out,
                                     ( (_title[at] == '\n') ||
                                       (_title[at] == '\r') ) ? ' '
                                                             : _title[at]);
    }

    // a title ending in # keeps it behind a closing run
    if ( (length > 0u) &&
         (_title[length - 1u] == '#') )
    {
        (void)d_wiki_buffer_add_text(_out, " #");
    }

    (void)d_wiki_buffer_add_char(_out, '\n');

    return !_out->failed;
}


/*
d_internal_write_destination
  Writes a destination: bare when it can be, else in <...>, its brackets
escaped.
*/
static void
d_internal_write_destination(
    struct d_wiki_buffer* _out,
    const char*           _url
)
{
    bool   angle  = (_url[0] == '\0') || (_url[0] == '<');
    long   parens = 0;

    // bare needs no space or control, and balanced parentheses
    for (const char* at = _url; *at != '\0'; ++at)
    {
        angle  = (angle) || ((unsigned char)*at <= 0x20u);
        parens += (*at == '(') ? 1 : (*at == ')') ? -1 : 0;
        angle  = (angle) || (parens < 0);
    }

    angle = (angle) || (parens != 0);

    // a destination with space, or unbalanced parentheses, in <>
    if (angle)
    {
        (void)d_wiki_buffer_add_char(_out, '<');
    }

    // each byte, `<`, `>` and `\` escaped
    for (const char* at = _url; *at != '\0'; ++at)
    {
        // a line ending encoded
        if ( (*at == '\n') ||
             (*at == '\r') )
        {
            (void)d_wiki_buffer_add_text(_out, "%0A");

            continue;
        }

        // `<`, `>` and `\` escaped
        if ( (*at == '<') ||
             (*at == '>') ||
             (*at == '\\') )
        {
            (void)d_wiki_buffer_add_char(_out, '\\');
        }

        (void)d_wiki_buffer_add_char(_out, *at);
    }

    // its close
    if (angle)
    {
        (void)d_wiki_buffer_add_char(_out, '>');
    }

    return;
}


/*
d_internal_write_label
  Writes a link's label: its brackets escaped, unless balanced already.
*/
static void
d_internal_write_label(
    struct d_wiki_buffer* _out,
    const char*           _label
)
{
    long depth    = 0;
    bool balanced = true;

    // brackets balanced, and not escaped, need no escape
    for (const char* at = _label; *at != '\0'; ++at)
    {
        // an escaped byte is passed over
        if ( (*at == '\\') &&
             (at[1] != '\0') )
        {
            at++;

            continue;
        }

        depth   += (*at == '[') ? 1 : (*at == ']') ? -1 : 0;
        balanced = (balanced) && (depth >= 0);
    }

    balanced = (balanced) && (depth == 0);

    // each byte, a line ending a space
    for (const char* at = _label; *at != '\0'; ++at)
    {
        // a line ending is a space
        if ( (*at == '\n') ||
             (*at == '\r') )
        {
            (void)d_wiki_buffer_add_char(_out, ' ');

            continue;
        }

        // an unbalanced bracket escaped
        if ( (!balanced) &&
             ( (*at == '[') || (*at == ']') ) &&
             ( (at == _label) || (at[-1] != '\\') ) )
        {
            (void)d_wiki_buffer_add_char(_out, '\\');
        }

        (void)d_wiki_buffer_add_char(_out, *at);
    }

    return;
}


static bool
d_internal_write_page_link(
    struct d_wiki_buffer* _out,
    const char*           _page,
    const char*           _anchor,
    const char*           _label
)
{
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };
    const bool           gollum = (_page[0] != '\0') &&
                                  (strpbrk(_page, "|[]#\n") == NULL) &&
                                  ( (!_label) ||
                                    (strpbrk(_label, "|[]\n") == NULL) ) &&
                                  ( (!_anchor) ||
                                    (strpbrk(_anchor, "|[]\n") == NULL) );

    // Gollum's [[Label|Page#anchor]], when nothing in it breaks it
    if (gollum)
    {
        (void)d_wiki_buffer_add_text(_out, "[[");

        // Gollum's label first
        if (_label)
        {
            (void)d_wiki_buffer_add_text(_out, _label);
            (void)d_wiki_buffer_add_char(_out, '|');
        }

        (void)d_wiki_buffer_add_text(_out, _page);

        // the anchor, after `#`
        if (_anchor)
        {
            (void)d_wiki_buffer_add_char(_out, '#');
            (void)d_wiki_buffer_add_text(_out, _anchor);
        }

        (void)d_wiki_buffer_add_text(_out, "]]");

        return !_out->failed;
    }

    // else [Label](Page-Name#anchor)
    for (const char* at = _page; *at != '\0'; ++at)
    {
        (void)d_wiki_buffer_add_char(&target, (*at == ' ') ? '-' : *at);
    }

    // the anchor, after `#`
    if (_anchor)
    {
        (void)d_wiki_buffer_add_char(&target, '#');
        (void)d_wiki_buffer_add_text(&target, _anchor);
    }

    (void)d_wiki_buffer_add(&target, "", 0u);
    (void)d_wiki_buffer_add_char(_out, '[');
    d_internal_write_label(_out, (_label) ? _label
                                 : (_page[0] != '\0') ? _page
                                 : (_anchor) ? _anchor : "");
    (void)d_wiki_buffer_add_text(_out, "](");
    d_internal_write_destination(_out, (target.text) ? target.text : "");
    (void)d_wiki_buffer_add_char(_out, ')');
    d_wiki_buffer_free(&target);

    return !_out->failed;
}


static bool
d_internal_write_url_link(
    struct d_wiki_buffer* _out,
    const char*           _url,
    const char*           _label
)
{
    bool   autolink = (_label == NULL) && (d_wiki_has_scheme(_url));

    // <url> needs no space, control, `<` or `>`
    for (const char* at = _url; ( (autolink) && (*at != '\0') ); ++at)
    {
        autolink = ((unsigned char)*at > 0x20u) &&
                   (*at != '<') &&
                   (*at != '>');
    }

    // an autolink, when the URL can be one
    if (autolink)
    {
        (void)d_wiki_buffer_add_char(_out, '<');
        (void)d_wiki_buffer_add_text(_out, _url);
        (void)d_wiki_buffer_add_char(_out, '>');

        return !_out->failed;
    }

    (void)d_wiki_buffer_add_char(_out, '[');
    d_internal_write_label(_out, (_label) ? _label : _url);
    (void)d_wiki_buffer_add_text(_out, "](");
    d_internal_write_destination(_out, _url);
    (void)d_wiki_buffer_add_char(_out, ')');

    return !_out->failed;
}


static bool
d_internal_write_image(
    struct d_wiki_buffer* _out,
    const char*           _source,
    const char*           _text
)
{
    (void)d_wiki_buffer_add_text(_out, "![");
    d_internal_write_label(_out, _text);
    (void)d_wiki_buffer_add_text(_out, "](");
    d_internal_write_destination(_out, _source);
    (void)d_wiki_buffer_add_char(_out, ')');

    return !_out->failed;
}


/*
d_internal_longest_run
  The longest run of a byte in text.
*/
static size_t
d_internal_longest_run(
    const char* _text,
    char        _c
)
{
    size_t longest = 0;
    size_t run     = 0;

    // each byte, a run counted while it lasts
    for (const char* at = _text; *at != '\0'; ++at)
    {
        run     = (*at == _c) ? (run + 1u) : 0u;
        longest = (run > longest) ? run : longest;
    }

    return longest;
}


static bool
d_internal_write_code(
    struct d_wiki_buffer* _out,
    const char*           _code,
    const char*           _language
)
{
    const bool   tildes = (_language) && (strchr(_language, '`') != NULL);
    const char   fence  = (tildes) ? '~' : '`';
    const size_t run    = d_internal_longest_run(_code, fence);
    const size_t length = (run >= 3u) ? (run + 1u) : 3u;

    // a fence longer than any run of its byte inside
    for (size_t count = 0; count < length; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, fence);
    }

    // the language, its first word
    for (const char* at = (_language) ? _language : "";
         ( (*at != '\0') && (*at != ' ') && (*at != '\n') ); ++at)
    {
        (void)d_wiki_buffer_add_char(_out, *at);
    }

    (void)d_wiki_buffer_add_char(_out, '\n');
    d_internal_add_lines(_out, _code, 0u);

    // a line ending before the fence that closes it
    if ( (_code[0] != '\0') &&
         (_code[strlen(_code) - 1u] != '\n') )
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    // the closing fence
    for (size_t count = 0; count < length; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, fence);
    }

    (void)d_wiki_buffer_add_char(_out, '\n');

    return !_out->failed;
}


static bool
d_internal_write_inline_code(
    struct d_wiki_buffer* _out,
    const char*           _code
)
{
    const size_t length = strlen(_code);
    const size_t run    = d_internal_longest_run(_code, '`') + 1u;
    const bool   pad    = (length > 0u) &&
                          ( (_code[0] == '`') || (_code[length - 1u] == '`') ||
                            ( (_code[0] == ' ') &&
                              (_code[length - 1u] == ' ') ) );

    // nothing is no code
    if (length == 0u)
    {
        return true;
    }

    // backticks more than any run inside, spaces between when needed
    for (size_t count = 0; count < run; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, '`');
    }

    // a space within the backticks, when the code begins or ends with one
    if (pad)
    {
        (void)d_wiki_buffer_add_char(_out, ' ');
    }

    // the code, a line ending a space
    for (const char* at = _code; *at != '\0'; ++at)
    {
        (void)d_wiki_buffer_add_char(_out, ( (*at == '\n') || (*at == '\r') )
                                               ? ' ' : *at);
    }

    // and the space before the close
    if (pad)
    {
        (void)d_wiki_buffer_add_char(_out, ' ');
    }

    // the closing backticks
    for (size_t count = 0; count < run; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, '`');
    }

    return !_out->failed;
}


static bool
d_internal_write_item(
    struct d_wiki_buffer* _out,
    const char*           _text,
    size_t                _depth,
    bool                  _ordered
)
{
    const size_t indent = (_depth - 1u) * 4u;

    // four columns a level, past any marker
    for (size_t column = 0; column < indent; ++column)
    {
        (void)d_wiki_buffer_add_char(_out, ' ');
    }

    (void)d_wiki_buffer_add_text(_out, (_ordered) ? "1. " : "- ");
    d_internal_add_lines(_out, _text, indent + ((_ordered) ? 3u : 2u));

    // a line ending after the text
    if ( (_text[0] == '\0') ||
         (_text[strlen(_text) - 1u] != '\n') )
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    return !_out->failed;
}


/*
d_internal_write_cell
  Writes a cell, its `|`s escaped.
*/
static void
d_internal_write_cell(
    struct d_wiki_buffer* _out,
    const char*           _cell,
    size_t                _length
)
{
    size_t gollum = 0;

    (void)d_wiki_buffer_add_char(_out, ' ');

    // each byte; a `|` within Gollum's [[...]], read before the table, kept
    for (size_t at = 0; at < _length; ++at)
    {
        // Gollum's [[ opens
        if ( ((at + 1u) < _length) &&
             (_cell[at] == '[') &&
             (_cell[at + 1u] == '[') )
        {
            gollum++;
        }
        else if ( ((at + 1u) < _length) &&
                  (_cell[at] == ']') &&
                  (_cell[at + 1u] == ']') &&
                  (gollum > 0u) )
        {
            gollum--;
        }

        // a cell's `|` escaped, but within [[...]]
        if ( (_cell[at] == '|') &&
             (gollum == 0u) )
        {
            (void)d_wiki_buffer_add_text(_out, "\\|");
        }
        else if (_cell[at] != '\r')
        {
            (void)d_wiki_buffer_add_char(_out, _cell[at]);
        }
    }

    (void)d_wiki_buffer_add_text(_out, " |");

    return;
}


static bool
d_internal_write_table(
    struct d_wiki_buffer* _out,
    const char*           _rows
)
{
    size_t      columns = 0;
    size_t      row     = 0;
    const char* at      = _rows;

    // the most cells a row has
    while (*at != '\0')
    {
        size_t cells = 1u;

        // the header's cells: the table's columns
        for (; ( (*at != '\0') && (*at != '\n') ); ++at)
        {
            cells += (*at == '\t') ? 1u : 0u;
        }

        columns = (cells > columns) ? cells : columns;
        at     += (*at == '\n') ? 1u : 0u;
    }

    // each row, the header's dashes after it
    for (at = _rows; *at != '\0'; ++row)
    {
        size_t cells = 0;

        (void)d_wiki_buffer_add_char(_out, '|');

        // each cell, apart by a tab
        while ( (*at != '\0') &&
                (*at != '\n') )
        {
            const char* const tab = strpbrk(at, "\t\n");
            const size_t      span = (tab) ? (size_t)(tab - at) : strlen(at);

            d_internal_write_cell(_out, at, span);
            cells++;
            at += span;
            at += (*at == '\t') ? 1u : 0u;
        }

        // a row with fewer cells has empty ones
        for (; cells < columns; ++cells)
        {
            (void)d_wiki_buffer_add_text(_out, "  |");
        }

        (void)d_wiki_buffer_add_char(_out, '\n');
        at += (*at == '\n') ? 1u : 0u;

        // the delimiter row after the header
        if (row == 0u)
        {
            (void)d_wiki_buffer_add_char(_out, '|');

            // a delimiter for each column
            for (size_t cell = 0; cell < columns; ++cell)
            {
                (void)d_wiki_buffer_add_text(_out, " --- |");
            }

            (void)d_wiki_buffer_add_char(_out, '\n');
        }
    }

    return !_out->failed;
}


static bool
d_internal_write_escape(
    struct d_wiki_buffer* _out,
    const char*           _text
)
{
    bool line_start = true;

    // each byte that could begin markup, after a backslash
    for (const char* at = _text; *at != '\0'; ++at)
    {
        const char c       = *at;
        bool       escaped = (strchr("\\`*_[]<>|~&@", c) != NULL);

        // `_` within a word opens and closes nothing
        if ( (c == '_') &&
             (at > _text) &&
             (d_internal_is_alnum(at[-1])) &&
             (d_internal_is_alnum(at[1])) )
        {
            escaped = false;
        }

        // what begins a block only at a line's start
        if (line_start)
        {
            const char* scan = at;

            escaped = (escaped) || (strchr("#+-=", c) != NULL);

            // a number at a line's start
            while (d_internal_is_digit(*scan))
            {
                scan++;
            }

            // a number then `.` or `)` begins an item: the `.` escaped
            if ( (scan > at) &&
                 ( (*scan == '.') || (*scan == ')') ) )
            {
                // the digits, as they are
                while (at < scan)
                {
                    (void)d_wiki_buffer_add_char(_out, *at);
                    at++;
                }

                (void)d_wiki_buffer_add_char(_out, '\\');
                (void)d_wiki_buffer_add_char(_out, *at);
                line_start = false;

                continue;
            }
        }

        // a bare link's www. and ://
        if ( ( (c == '.') &&
               (at >= (_text + 3)) &&
               (strncmp(at - 3, "www.", 4u) == 0) ) ||
             ( (c == ':') &&
               (strncmp(at, "://", 3u) == 0) ) )
        {
            escaped = true;
        }

        // a byte that would begin markup, after a backslash
        if (escaped)
        {
            (void)d_wiki_buffer_add_char(_out, '\\');
        }

        (void)d_wiki_buffer_add_char(_out, c);
        line_start = (c == '\n');
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_write_target
  What stands in a link's destination, in its form: Gollum's page as it
is, Markdown's as a destination, an autolink's bare.
*/
static bool
d_internal_write_target(
    struct d_wiki_buffer*     _out,
    const struct d_wiki_link* _link,
    const char*               _target
)
{
    // a table of how each form writes it
    switch (_link->form)
    {
        case D_WIKI_FORM_WIKI:
            // Gollum's page may hold no `|` and no line ending
            if (strpbrk(_target, "|\n") != NULL)
            {
                return false;
            }

            (void)d_wiki_buffer_add_text(_out, _target);
            break;

        case D_WIKI_FORM_INLINE:
        case D_WIKI_FORM_DEFINITION:
            d_internal_write_destination(_out, _target);
            break;

        case D_WIKI_FORM_AUTO:
        case D_WIKI_FORM_BARE:
            // an autolink's destination may hold no space, `<` or `>`
            for (const char* at = _target; *at != '\0'; ++at)
            {
                // a byte it may not hold
                if ( ((unsigned char)*at <= 0x20u) ||
                     (*at == '<') ||
                     (*at == '>') )
                {
                    return false;
                }
            }

            (void)d_wiki_buffer_add_text(_out,
                                         ( (strncmp(_target, "mailto:", 7u)
                                                == 0) &&
                                           (strncmp(_link->label, "mailto:",
                                                    7u) != 0) )
                                             ? (_target + 7) : _target);
            break;

        case D_WIKI_FORM_HTML:
            // an HTML attribute's quotes as references
            for (const char* at = _target; *at != '\0'; ++at)
            {
                // `"`, as its reference
                if (*at == '"')
                {
                    (void)d_wiki_buffer_add_text(_out, "&quot;");
                }
                else if (*at == '&')
                {
                    (void)d_wiki_buffer_add_text(_out, "&amp;");
                }
                else
                {
                    (void)d_wiki_buffer_add_char(_out, *at);
                }
            }

            break;

        default:
            return false;
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_write_link_label
  What stands in a link's label, in its form.
*/
static bool
d_internal_write_link_label(
    struct d_wiki_buffer*     _out,
    const struct d_wiki_link* _link,
    const char*               _label
)
{
    // Gollum's label has no `|` or `]]`; Markdown's brackets balance
    if (_link->form == D_WIKI_FORM_WIKI)
    {
        // Gollum's label may hold no `|`, line ending, or `]]`
        if ( (strpbrk(_label, "|\n") != NULL) ||
             (strstr(_label, "]]") != NULL) )
        {
            return false;
        }

        (void)d_wiki_buffer_add_text(_out, _label);
    }
    else
    {
        d_internal_write_label(_out, _label);
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_wiki_markdown_driver
  The driver's table.
*/
const struct d_wiki_driver*
d_wiki_markdown_driver(void)
{
    static const struct d_wiki_driver driver =
    {
        D_WIKI_MARKDOWN,
        d_internal_read,
        d_internal_anchor,
        '-',
        1u,
        NULL,
        d_internal_write_heading,
        d_internal_write_page_link,
        d_internal_write_url_link,
        d_internal_write_image,
        d_internal_write_code,
        d_internal_write_inline_code,
        d_internal_write_item,
        d_internal_write_table,
        d_internal_write_escape,
        d_internal_write_target,
        d_internal_write_link_label
    };

    return &driver;
}
