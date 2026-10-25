/*******************************************************************************
* djinterp [tools]                                             dwiki_mediawiki.c
*
* The MediaWiki driver: wikitext.
*   Read as MediaWiki 1.43 reads it, in its order, and checked against it,
* over its own parser tests. First its preprocessor: a comment is dropped,
* an extension tag's content kept apart -- literal, as <nowiki> and <pre>
* are, or read on its own, as <ref> is, and <includeonly>'s, which a page
* does not show -- and braces, brackets, the language converter's -{ }-,
* pipes and line-start `=`s kept on a stack, so {{...}} and {{{...}}} close
* as MediaWiki closes them, each with its arguments, and a heading is a
* line between `=` runs: the page's own unless a template or a heading
* holds it. A template's name says what it is by MediaWiki's own magic
* words: a parser function, a variable, or a template.
*   Then its parser. Lines are blocks: a heading, a table from {| to |},
* a rule, a list's item, a line that begins with a space, code in <pre> or
* <syntaxhighlight>, a comment alone; the rest paragraphs. A table's rows
* are split into cells, which no URL crosses. Links are found as MediaWiki
* finds them: the text is split at each [[, and each piece that begins
* with a title and ends with ]] is a link -- an image's caption, which
* holds links of its own, looked ahead for -- and the trail of letters
* after it shows as part of it; then [url text], in the page and within
* each image; then bare URLs, ended as MediaWiki's sanitizer, emphasis and
* references have left the text. An image's link= is a link, and so is
* each line of a <gallery>. Templates are not expanded: what is in their
* arguments is read where it is written.
*   A heading's anchor is MediaWiki's, as its finalizeHeadings makes it:
* its text, each run of white space one underscore, a direction's marks
* gone; one an anchor before it has, compared without the case of ASCII
* letters, has the first of _2, _3, ... that no anchor before it has.
*
* path:      /src/djinterp/tools/dawk/ext/wiki/dwiki_mediawiki.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, calloc, qsort
#include <string.h>  // memcpy, memset, memchr, strchr, strlen, strncmp
// djinterp
#include "./dwiki_driver.h"  // d_wiki_driver, d_wiki_add_block


// D_INTERNAL_MOST_DEPTH
//   constant: how deep extension tags are read within one another.
#define D_INTERNAL_MOST_DEPTH 16u

// D_INTERNAL_MARK
//   constant: what stands in the text for an extension tag's element.
#define D_INTERNAL_MARK '\x7f'


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    The preprocessor
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_hidden_kind
//   enum: what the preprocessor sets apart from the text.
enum d_internal_hidden_kind
{
    D_INTERNAL_COMMENT = 0,  // <!-- ... -->, dropped
    D_INTERNAL_IGNORED,      // <noinclude>, <onlyinclude>: dropped
    D_INTERNAL_LITERAL,      // <nowiki>, <pre>: a mark, its content as is
    D_INTERNAL_ELEMENT,      // <ref>: a mark, its content read apart
    D_INTERNAL_INCLUDED      // <includeonly>: dropped, its content read
                             // apart, for what it uses
};

// 1.1.2
// d_internal_hidden
//   struct: one thing set apart: its span, and an element's content and
//   tag's name.
struct d_internal_hidden
{
    enum d_internal_hidden_kind kind;
    size_t                      start;
    size_t                      end;
    size_t                      content;
    size_t                      content_end;
    size_t                      name;
    size_t                      name_length;
    size_t                      attributes;
    size_t                      attributes_end;
};

// 1.1.3
// d_internal_part
//   struct: one part of a piece: where it begins, and its first `=`.
struct d_internal_part
{
    size_t start;
    size_t equals;
};

// 1.1.4
// d_internal_piece
//   struct: one opening on the preprocessor's stack: `{`, `[`, `-` for
//   the language converter's `-{`, or a heading's `=`s at a line's start,
//   how many are left to match, where they begin, and the parts after
//   them.  `dash` says a `-` before braces is held back, for `-{` to open
//   again when one brace is left.
struct d_internal_piece
{
    char                    open;
    size_t                  count;
    size_t                  start;
    bool                    dash;
    struct d_internal_part* parts;
    size_t                  part_count;
    size_t                  part_capacity;
};

// 1.1.5
// d_internal_found_heading
//   struct: a heading the preprocessor found: its line, its level, its
//   title, whether it stands at the top, and whether a block has kept it.
struct d_internal_found_heading
{
    size_t start;
    size_t end;
    size_t title_start;
    size_t title_end;
    size_t level;
    bool   top;
    bool   kept;
};

// 1.1.6
// d_internal_mw
//   struct: the reader: the page, what is set apart, the boundaries of
//   templates and of tables' cells, the headings, for each line how far
//   what began before it reaches past its start, and where the cells'
//   attributes are, which hold no link, two places to a cell.
struct d_internal_mw
{
    struct d_wiki*                   wiki;
    const char*                      text;
    size_t                           length;
    struct d_internal_hidden*        hidden;
    size_t                           hidden_count;
    size_t                           hidden_capacity;
    size_t*                          boundaries;
    size_t                           boundary_count;
    size_t                           boundary_capacity;
    struct d_internal_found_heading* headings;
    size_t                           heading_count;
    size_t                           heading_capacity;
    size_t*                          reach;
    size_t*                          shut;
    size_t                           shut_count;
    size_t                           shut_capacity;
    bool                             failed;
};

// 1.1.7
// d_internal_magic
//   struct: one of MediaWiki's magic words: its name, in lower case
//   unless `sensitive` says its case is its own.
struct d_internal_magic
{
    const char* name;
    bool        sensitive;
};

// 1.1.8
// d_internal_named
//   struct: what a template's first part says.  Given: where the part is,
//   `start` to `end`, how many parts the template has, and whether it is
//   a parameter's, {{{...}}}, whose name is only cleaned.  Found: its
//   name, the comments and dropped tags within it gone, past the prefixes
//   MediaWiki takes off it -- `safesubst:`, `msgnw:`, `msg:`, `raw:` --
//   and trimmed; where that name stands on the page, `name_start` to
//   `name_end`; and a function's `:`, D_WIKI_NONE for none.
struct d_internal_named
{
    size_t start;
    size_t end;
    size_t parts;
    bool   parameter;
    char*  name;
    size_t name_start;
    size_t name_end;
    size_t colon;
};

// 1.1.9
// d_internal_offsets
//   struct: places on the page, a list that grows.
struct d_internal_offsets
{
    size_t* at;
    size_t  count;
    size_t  capacity;
};

// 1.2    Streams
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_piece_map
//   struct: a run of a stream's text and where on the page it stands.
struct d_internal_piece_map
{
    size_t joined;
    size_t source;
    size_t length;
};

// 1.2.2
// d_internal_cover
//   enum: what holds a byte of a stream, of the links found so far, for
//   the links found after: MediaWiki finds [[...]] first, then [url text]
//   -- in the page, and within each image -- then bare URLs.
enum d_internal_cover
{
    D_INTERNAL_FREE = 0,   // nothing: any link may be here
    D_INTERNAL_LINKED,     // a link: no other may be within it
    D_INTERNAL_PICTURED,   // an image: [url text] within it is its own
    D_INTERNAL_CAPTIONED   // an image's caption, shown: a bare URL too
};

// 1.2.3
// d_internal_stream
//   struct: text read as one: the page, or an element's content, with what
//   is dropped gone and each element a mark; each place tied to the page.
//   `covered` holds what covers each byte, a d_internal_cover, and
//   `images` where each image's parts begin and end, two to an image.
struct d_internal_stream
{
    struct d_internal_mw*        mw;
    struct d_wiki_buffer         joined;
    struct d_internal_piece_map* pieces;
    size_t                       piece_count;
    size_t                       piece_capacity;
    char*                        covered;
    size_t*                      images;
    size_t                       image_count;
    size_t                       image_capacity;
    bool                         failed;
};

// 1.2.4
// d_internal_opens
//   struct: the `[[`s of a stream, in order, `count` of them, and the one
//   whose piece -- to the next, or the stream's end -- is being read.
struct d_internal_opens
{
    const size_t* at;
    size_t        count;
    size_t        index;
};

// 1.2.5
// d_internal_internal
//   struct: an internal link being read: its title, `title_start` to
//   `title_end`; its `]]`; its label, `label` to `label_end`, D_WIKI_NONE
//   for none; whether it is an image, its caption still to be closed;
//   whether a `:` before its title links to the page a namespace would put
//   in place; and its title's namespace.
struct d_internal_internal
{
    size_t title_start;
    size_t title_end;
    size_t close;
    size_t label;
    size_t label_end;
    bool   image;
    bool   noforce;
    int    space;
};

// 1.2.6
// d_internal_image
//   struct: what an image's parts say, as places in a stream: its caption,
//   the last part that is no option; its alt=; its link='s value, each
//   D_WIKI_NONE for none; and whether it is framed, its caption shown.
struct d_internal_image
{
    size_t caption;
    size_t caption_end;
    size_t alt;
    size_t alt_end;
    size_t link;
    size_t link_end;
    bool   framed;
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
d_internal_lower, d_internal_is_alpha, d_internal_is_alnum,
d_internal_is_hex, d_internal_is_space_or_tab
  ASCII's classes, without the locale's say.
*/
static char
d_internal_lower(
    char _c
)
{
    return ( (_c >= 'A') && (_c <= 'Z') ) ? (char)(_c + 32) : _c;
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
d_internal_is_alnum(
    char _c
)
{
    return ( (d_internal_is_alpha(_c)) ||
             ( (_c >= '0') && (_c <= '9') ) );
}


static bool
d_internal_is_hex(
    char _c
)
{
    return ( ( (_c >= '0') && (_c <= '9') ) ||
             ( (_c >= 'a') && (_c <= 'f') ) ||
             ( (_c >= 'A') && (_c <= 'F') ) );
}


static bool
d_internal_is_space_or_tab(
    char _c
)
{
    return ( (_c == ' ') || (_c == '\t') );
}


/*
d_internal_is_space
  Whether a byte is white space as a regular expression's \s is: space,
a tab, a line ending, a form feed or a vertical tab.
*/
static bool
d_internal_is_space(
    char _c
)
{
    return (_c != '\0') &&
           (strchr(" \t\n\r\f\v", _c) != NULL);
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
d_internal_in_list
  Whether a name of a length is one of a list's, in any case.
*/
static bool
d_internal_in_list(
    const char*        _name,
    size_t             _length,
    const char* const* _list,
    size_t             _count
)
{
    // each name of the list
    for (size_t index = 0; index < _count; ++index)
    {
        // a name of the list, whole, in any case
        if ( (strlen(_list[index]) == _length) &&
             (d_internal_starts_with(_name, _length, _list[index])) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_protocol
  The length of a URL's protocol at `_text`, MediaWiki's own list, in any
case; 0 for none.  `_relative` allows `//`.
*/
static size_t
d_internal_protocol(
    const char* _text,
    size_t      _length,
    bool        _relative
)
{
    static const char* const protocols[] =
    {
        "bitcoin:", "ftp://", "ftps://", "geo:", "git://", "gopher://",
        "http://", "https://", "irc://", "ircs://", "magnet:", "mailto:",
        "matrix:", "mms://", "news:", "nntp://", "redis://", "sftp://",
        "sip:", "sips:", "sms:", "ssh://", "svn://", "tel:", "telnet://",
        "urn:", "worldwind://", "xmpp:"
    };

    // each protocol MediaWiki links
    for (size_t index = 0; index < (sizeof(protocols) / sizeof(protocols[0]));
         ++index)
    {
        // a protocol the text begins with
        if (d_internal_starts_with(_text, _length, protocols[index]))
        {
            return strlen(protocols[index]);
        }
    }

    return ( (_relative) &&
             (_length >= 2u) &&
             (_text[0] == '/') &&
             (_text[1] == '/') ) ? 2u : 0u;
}


/*
d_internal_is_url_byte
  Whether a byte may be in a URL: not a bracket, `<`, `>`, `"`, a control,
space or the mark.
*/
static bool
d_internal_is_url_byte(
    char _c
)
{
    const unsigned char c = (unsigned char)_c;

    return ( (c > 0x20u) &&
             (c != 0x7Fu) &&
             (strchr("[]<>\"", _c) == NULL) );
}


/*
d_internal_is_title_byte
  Whether a byte may be in a link's title as MediaWiki reads one: not a
bracket, `|`, `<`, `>`, a control or the mark.  Braces are allowed, for
the templates a title may hold.
*/
static bool
d_internal_is_title_byte(
    char _c
)
{
    const unsigned char c = (unsigned char)_c;

    return ( (c >= 0x20u) &&
             (c != 0x7Fu) &&
             (strchr("[]|<>", _c) == NULL) );
}


/*
d_internal_is_magic
  Whether a name of a length is one of a list of magic words: in its own
case for one whose case is its own, else in any.
*/
static bool
d_internal_is_magic(
    const char*                    _name,
    size_t                         _length,
    const struct d_internal_magic* _list,
    size_t                         _count
)
{
    // each word of the list
    for (size_t index = 0; index < _count; ++index)
    {
        const struct d_internal_magic* const word = &_list[index];

        // a word of another length is not this one
        if (strlen(word->name) != _length)
        {
            continue;
        }

        // the word, in its own case or in any
        if ( (word->sensitive)
                 ? (memcmp(_name, word->name, _length) == 0)
                 : (d_internal_starts_with(_name, _length, word->name)) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_zs_width
  The length of a space of Unicode's own, \p{Zs}, at `_at` but the ASCII
space -- U+00A0, U+1680, U+2000 to U+200A, U+202F, U+205F, U+3000 -- 0 for
another character.
*/
static size_t
d_internal_zs_width(
    const char* _at
)
{
    const unsigned char* const c = (const unsigned char*)_at;

    // U+00A0, no-break space
    if ( (c[0] == 0xC2u) &&
         (c[1] == 0xA0u) )
    {
        return 2u;
    }

    return ( ( (c[0] == 0xE1u) &&
               (c[1] == 0x9Au) &&
               (c[2] == 0x80u) ) ||
             ( (c[0] == 0xE2u) &&
               ( ( (c[1] == 0x80u) &&
                   ( (c[2] <= 0x8Au) || (c[2] == 0xAFu) ) &&
                   (c[2] >= 0x80u) ) ||
                 ( (c[1] == 0x81u) && (c[2] == 0x9Fu) ) ) ) ||
             ( (c[0] == 0xE3u) &&
               (c[1] == 0x80u) &&
               (c[2] == 0x80u) ) ) ? 3u : 0u;
}


/*
d_internal_url_char
  The length of a character a URL may hold at `_at`, as MediaWiki's
EXT_LINK_URL_CLASS has it: not a bracket, `<`, `>`, `"`, a control, a
space -- Unicode's spaces among them -- nor U+FFFD; 0 for one it may not.
*/
static size_t
d_internal_url_char(
    const char* _at
)
{
    const unsigned char c     = (unsigned char)_at[0];
    size_t              width = 1u;

    // ASCII: not a bracket, `<`, `>`, `"`, a control or space
    if (c < 0x80u)
    {
        return ( (c > 0x20u) &&
                 (c != 0x7Fu) &&
                 (strchr("[]<>\"", (char)c) == NULL) ) ? 1u : 0u;
    }

    // a space of Unicode's, or U+FFFD
    if ( (d_internal_zs_width(_at) > 0u) ||
         (strncmp(_at, "\xEF\xBF\xBD", 3u) == 0) )
    {
        return 0u;
    }

    // the rest of the character's bytes
    while ( (width < 4u) &&
            ((((unsigned char)_at[width]) & 0xC0u) == 0x80u) )
    {
        width++;
    }

    return width;
}


/*
d_internal_url_stop
  Whether a reference at `_at` ends a URL, as MediaWiki cuts one: &lt; and
&gt;; and, for a bare URL, &nbsp; and the numbers of all three.
*/
static bool
d_internal_url_stop(
    const char* _at,
    bool        _bare
)
{
    const char* at = _at + 2u;

    // &lt; and &gt;, and &nbsp; for a bare URL
    if ( (strncmp(_at, "&lt;", 4u) == 0) ||
         (strncmp(_at, "&gt;", 4u) == 0) ||
         ( (_bare) && (strncmp(_at, "&nbsp;", 6u) == 0) ) )
    {
        return true;
    }

    // a number's reference, only for a bare URL
    if ( (!_bare) ||
         (strncmp(_at, "&#", 2u) != 0) )
    {
        return false;
    }

    const bool hex = (*at == 'x');

    at += (hex) ? 1u : 0u;

    // the zeros before the number
    while (*at == '0')
    {
        at++;
    }

    return (hex)
               ? ( ( (strncmp(at, "3C;", 3u) == 0) ||
                     (strncmp(at, "3c;", 3u) == 0) ||
                     (strncmp(at, "3E;", 3u) == 0) ||
                     (strncmp(at, "3e;", 3u) == 0) ||
                     (strncmp(at, "A0;", 3u) == 0) ||
                     (strncmp(at, "a0;", 3u) == 0) ) )
               : ( (strncmp(at, "60;", 3u) == 0) ||
                   (strncmp(at, "62;", 3u) == 0) ||
                   (strncmp(at, "160;", 4u) == 0) );
}


/*
d_internal_ends_reference
  Whether the text from `_from` to `_at` ends as a reference does before
its `;`: `&` and a name, `&#` and digits, or `&#x` and hex digits.
*/
static bool
d_internal_ends_reference(
    const char* _text,
    size_t      _from,
    size_t      _at
)
{
    size_t start = _at;

    // the letters and digits before the `;`
    while ( (start > _from) &&
            (d_internal_is_alnum(_text[start - 1u])) )
    {
        start--;
    }

    // nothing before the `;` that a reference ends with
    if ( (start == _at) ||
         (start == _from) )
    {
        return false;
    }

    // `&` and a name of letters
    if (_text[start - 1u] == '&')
    {
        // a name all letters
        for (size_t at = start; at < _at; ++at)
        {
            // a digit is no name's
            if (!d_internal_is_alpha(_text[at]))
            {
                return false;
            }
        }

        return true;
    }

    // `&#`, then digits, or `x` and hex digits
    if ( (_text[start - 1u] != '#') ||
         ((start - 1u) == _from) ||
         (_text[start - 2u] != '&') )
    {
        return false;
    }

    const bool hex = ( (_text[start] == 'x') || (_text[start] == 'X') ) &&
                     ((start + 1u) < _at);

    // each digit, hex after `x`
    for (size_t at = start + ((hex) ? 1u : 0u); at < _at; ++at)
    {
        // a byte no number of this kind holds
        if ( ( (hex) && (!d_internal_is_hex(_text[at])) ) ||
             ( (!hex) && ( (_text[at] < '0') || (_text[at] > '9') ) ) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_html_tag
  The length of an HTML tag at `_at` that MediaWiki's sanitizer keeps --
open, closing or empty, its name one of its elements' -- to its `>`, with
no `<` before that; 0 for none, the `<` then text.
  Long as the sanitizer's list of elements is, held here.
*/
static size_t
d_internal_html_tag(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    static const char* const elements[] =
    {
        "abbr", "b", "bdi", "bdo", "big", "blockquote", "br", "caption",
        "center", "cite", "code", "data", "dd", "del", "dfn", "div", "dl",
        "dt", "em", "font", "h1", "h2", "h3", "h4", "h5", "h6", "hr", "i",
        "ins", "kbd", "li", "link", "mark", "meta", "ol", "p", "pre", "q",
        "rb", "rp", "rt", "rtc", "ruby", "s", "samp", "small", "span",
        "strike", "strong", "sub", "sup", "table", "td", "th", "time", "tr",
        "tt", "u", "ul", "var", "wbr"
    };
    size_t name = _at + 1u;

    // `<` begins it
    if (_text[_at] != '<')
    {
        return 0u;
    }

    // a closing tag's `/`
    if ( (name < _end) &&
         (_text[name] == '/') )
    {
        name++;
    }

    // then a letter
    if ( (name >= _end) ||
         (!d_internal_is_alpha(_text[name])) )
    {
        return 0u;
    }

    size_t stop = name;

    // the name, to space, `/` or `>`
    while ( (stop < _end) &&
            (_text[stop] != '\0') &&
            (strchr("\t\n\v />", _text[stop]) == NULL) )
    {
        stop++;
    }

    // a name the sanitizer does not keep: text
    if (!d_internal_in_list(&_text[name], stop - name, elements,
                            sizeof(elements) / sizeof(elements[0])))
    {
        return 0u;
    }

    // to its `>`, with no `<` before it
    for (size_t at = stop; at < _end; ++at)
    {
        // the tag's end, or a `<` that makes it none
        if (_text[at] == '>')
        {
            return at + 1u - _at;
        }
        else if (_text[at] == '<')
        {
            return 0u;
        }
    }

    return 0u;
}


/*
d_internal_double_word
  The length of a magic word of MediaWiki's between double underscores at
`_at` -- __TOC__, __NOTOC__ -- which a page does not show; 0 for none.
*/
static size_t
d_internal_double_word(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    static const struct d_internal_magic words[] =
    {
        { "__notoc__", false }, { "__nogallery__", false },
        { "__forcetoc__", false }, { "__toc__", false },
        { "__noeditsection__", false }, { "__NEWSECTIONLINK__", true },
        { "__NONEWSECTIONLINK__", true }, { "__HIDDENCAT__", true },
        { "__EXPECTUNUSEDCATEGORY__", true },
        { "__EXPECTUNUSEDTEMPLATE__", true }, { "__INDEX__", true },
        { "__NOINDEX__", true }, { "__STATICREDIRECT__", true },
        { "__notitleconvert__", false }, { "__notc__", false },
        { "__nocontentconvert__", false }, { "__nocc__", false }
    };

    // two underscores open one
    if ( ((_at + 4u) > _end) ||
         (_text[_at] != '_') ||
         (_text[_at + 1u] != '_') )
    {
        return 0u;
    }

    // the two that close it, a word's length away at most
    for (size_t close = _at + 2u; ( ((close + 1u) < _end) &&
                                    ((close - _at) < 32u) ); ++close)
    {
        // the first two after the open
        if ( (_text[close] == '_') &&
             (_text[close + 1u] == '_') )
        {
            const size_t length = close + 2u - _at;

            return (d_internal_is_magic(&_text[_at], length, words,
                                        sizeof(words) / sizeof(words[0])))
                       ? length : 0u;
        }
    }

    return 0u;
}


/*
d_internal_unescape
  Writes text with its character references read.
*/
static bool
d_internal_unescape(
    struct d_wiki_buffer* _out,
    const char*           _text,
    size_t                _length
)
{
    size_t at = 0;

    // each byte, or what a reference stands for
    while (at < _length)
    {
        // a reference decoded, or `&` as itself
        if (_text[at] == '&')
        {
            const size_t span = d_wiki_entity(&_text[at], _length - at, _out);

            // past the reference read
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


//==============================================================================
// 3.  THE PREPROCESSOR
//==============================================================================


/*
d_internal_hide
  Keeps something set apart.
*/
static struct d_internal_hidden*
d_internal_hide(
    struct d_internal_mw*       _mw,
    enum d_internal_hidden_kind _kind,
    size_t                      _start,
    size_t                      _end
)
{
    // room for one more
    if (!d_internal_grow((void**)&_mw->hidden, &_mw->hidden_capacity,
                         _mw->hidden_count, sizeof(struct d_internal_hidden)))
    {
        _mw->failed = true;

        return NULL;
    }

    struct d_internal_hidden* const hidden = &_mw->hidden[_mw->hidden_count++];

    memset(hidden, 0, sizeof(*hidden));
    hidden->kind           = _kind;
    hidden->start          = _start;
    hidden->end            = _end;
    hidden->content        = _end;
    hidden->content_end    = _end;
    hidden->attributes     = _end;
    hidden->attributes_end = _end;

    return hidden;
}


/*
d_internal_first_hidden
  The first record of what is hidden that starts at or after `_at`; the
count of them for none.  The records are in the order of their starts.
*/
static size_t
d_internal_first_hidden(
    const struct d_internal_mw* _mw,
    size_t                      _at
)
{
    size_t low  = 0;
    size_t high = _mw->hidden_count;

    // the first that starts at or after the place
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // those after the middle, or those to it
        if (_mw->hidden[middle].start < _at)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return low;
}


/*
d_internal_add_boundary
  Keeps a place a template's syntax stands: a brace, or a pipe between its
parts, which an external link may not cross.
*/
static void
d_internal_add_boundary(
    struct d_internal_mw* _mw,
    size_t                _at
)
{
    // room for one more
    if (!d_internal_grow((void**)&_mw->boundaries, &_mw->boundary_capacity,
                         _mw->boundary_count, sizeof(size_t)))
    {
        _mw->failed = true;

        return;
    }

    _mw->boundaries[_mw->boundary_count++] = _at;

    return;
}


/*
d_internal_tag_name
  The length of a tag's name at `_at`, after `<` or `</`: letters, digits.
*/
static size_t
d_internal_tag_name(
    const char* _text,
    size_t      _end,
    size_t      _at
)
{
    size_t at = _at;

    // letters first, then letters and digits
    if ( (at >= _end) ||
         (!d_internal_is_alpha(_text[at])) )
    {
        return 0u;
    }

    // the name's letters and digits
    while ( (at < _end) &&
            (d_internal_is_alnum(_text[at])) )
    {
        at++;
    }

    return at - _at;
}


/*
d_internal_find_close_tag
  Where `</name>` begins, in any case, from `_at`; D_WIKI_NONE for none,
and where it ends.
*/
static size_t
d_internal_find_close_tag(
    const char* _text,
    size_t      _end,
    size_t      _at,
    const char* _name,
    size_t      _length,
    size_t*     _out_end
)
{
    // each `</` that names it
    for (size_t at = _at; (at + _length + 2u) < (_end + 1u); ++at)
    {
        size_t close = at + 2u + _length;

        // only `</` can begin a close
        if ( (_text[at] != '<') ||
             (_text[at + 1u] != '/') )
        {
            continue;
        }

        size_t index = 0;

        // the name, in any case
        while ( (index < _length) &&
                (d_internal_lower(_text[at + 2u + index]) ==
                     d_internal_lower(_name[index])) )
        {
            index++;
        }

        // another name, or a longer one
        if ( (index < _length) ||
             (d_internal_is_alnum(_text[close])) )
        {
            continue;
        }

        // space, then `>`
        while ( (close < _end) &&
                (d_internal_is_space(_text[close])) )
        {
            close++;
        }

        // the close's `>`, past the space before it
        if ( (close < _end) &&
             (_text[close] == '>') )
        {
            *_out_end = close + 1u;

            return at;
        }
    }

    return D_WIKI_NONE;
}


/*
d_internal_angle
  A `<` the preprocessor reads at `_at`: a comment, a tag it drops, or an
extension tag's element.  Gives where reading goes on, or 0 for text.
  Long as the three kinds of what a `<` begins are read in turn, as the
preprocessor reads them.
*/
static size_t
d_internal_angle(
    struct d_internal_mw* _mw,
    size_t                _at,
    size_t                _end,
    size_t                _depth
);


/*
d_internal_new_piece
  An opening pushed on the stack, with its first part after it.
*/
static bool
d_internal_new_piece(
    struct d_internal_piece* _piece,
    char                     _open,
    size_t                   _count,
    size_t                   _start,
    size_t                   _content
)
{
    memset(_piece, 0, sizeof(*_piece));
    _piece->open  = _open;
    _piece->count = _count;
    _piece->start = _start;
    _piece->parts = malloc(4u * sizeof(struct d_internal_part));

    // a piece needs room for its parts
    if (!_piece->parts)
    {
        return false;
    }

    _piece->part_capacity      = 4u;
    _piece->part_count         = 1u;
    _piece->parts[0].start     = _content;
    _piece->parts[0].equals    = D_WIKI_NONE;

    return true;
}


/*
d_internal_add_part
  A part after a pipe.
*/
static bool
d_internal_add_part(
    struct d_internal_piece* _piece,
    size_t                   _start
)
{
    // room for one more
    if (!d_internal_grow((void**)&_piece->parts, &_piece->part_capacity,
                         _piece->part_count, sizeof(struct d_internal_part)))
    {
        return false;
    }

    _piece->parts[_piece->part_count].start  = _start;
    _piece->parts[_piece->part_count].equals = D_WIKI_NONE;
    _piece->part_count++;

    return true;
}


/*
d_internal_trimmed
  Bounds a span past the space at either end.
*/
static void
d_internal_trimmed(
    const char* _text,
    size_t*     _start,
    size_t*     _end
)
{
    // space, tabs and line endings either side
    while ( (*_start < *_end) &&
            ( (d_internal_is_space_or_tab(_text[*_start])) ||
              (_text[*_start] == '\n') || (_text[*_start] == '\r') ) )
    {
        (*_start)++;
    }

    // and the space at its end
    while ( (*_end > *_start) &&
            ( (d_internal_is_space_or_tab(_text[*_end - 1u])) ||
              (_text[*_end - 1u] == '\n') || (_text[*_end - 1u] == '\r') ) )
    {
        (*_end)--;
    }

    return;
}


/*
d_internal_is_function
  Whether a name is one of MediaWiki's parser functions, as written
before its `:`.
*/
static bool
d_internal_is_function(
    const char* _name,
    size_t      _length
)
{
    static const struct d_internal_magic functions[] =
    {
        { "#bcp47", true }, { "#dateformat", false }, { "#dir", true },
        { "#FORMAL", true }, { "#formatdate", false }, { "#language", false },
        { "#special", false }, { "#speciale", false }, { "#tag", false },
        { "anchorencode", false }, { "ARTICLEPAGENAME", true },
        { "ARTICLEPAGENAMEE", true }, { "ARTICLESPACE", true },
        { "ARTICLESPACEE", true }, { "BASEPAGENAME", true },
        { "BASEPAGENAMEE", true }, { "bidi", false },
        { "canonicalurl", false }, { "canonicalurle", false },
        { "CASCADINGSOURCES", true }, { "DEFAULTCATEGORYSORT", true },
        { "DEFAULTSORT", true }, { "DEFAULTSORTKEY", true },
        { "DISPLAYTITLE", true }, { "filepath", false },
        { "formatnum", false }, { "FULLPAGENAME", true },
        { "FULLPAGENAMEE", true }, { "fullurl", false },
        { "fullurle", false }, { "gender", false }, { "grammar", false },
        { "int", false }, { "lc", false }, { "lcfirst", false },
        { "localurl", false }, { "localurle", false }, { "NAMESPACE", true },
        { "NAMESPACEE", true }, { "NAMESPACENUMBER", true }, { "ns", false },
        { "nse", false }, { "NUMBERINGROUP", true },
        { "NUMBEROFACTIVEUSERS", true }, { "NUMBEROFADMINS", true },
        { "NUMBEROFARTICLES", true }, { "NUMBEROFEDITS", true },
        { "NUMBEROFFILES", true }, { "NUMBEROFPAGES", true },
        { "NUMBEROFUSERS", true }, { "NUMINGROUP", true },
        { "padleft", false }, { "padright", false }, { "pageid", false },
        { "PAGENAME", true }, { "PAGENAMEE", true }, { "PAGESINCAT", true },
        { "PAGESINCATEGORY", true }, { "PAGESIZE", true },
        { "plural", false }, { "PROTECTIONEXPIRY", true },
        { "PROTECTIONLEVEL", true }, { "REVISIONDAY", true },
        { "REVISIONDAY2", true }, { "REVISIONID", true },
        { "REVISIONMONTH", true }, { "REVISIONMONTH1", true },
        { "REVISIONTIMESTAMP", true }, { "REVISIONUSER", true },
        { "REVISIONYEAR", true }, { "ROOTPAGENAME", true },
        { "ROOTPAGENAMEE", true }, { "SUBJECTPAGENAME", true },
        { "SUBJECTPAGENAMEE", true }, { "SUBJECTSPACE", true },
        { "SUBJECTSPACEE", true }, { "SUBPAGENAME", true },
        { "SUBPAGENAMEE", true }, { "TALKPAGENAME", true },
        { "TALKPAGENAMEE", true }, { "TALKSPACE", true },
        { "TALKSPACEE", true }, { "uc", false }, { "ucfirst", false },
        { "urlencode", false }
    };

    return d_internal_is_magic(_name, _length, functions,
                               sizeof(functions) / sizeof(functions[0]));
}


/*
d_internal_is_variable
  Whether a name is one of MediaWiki's variables, written alone.
*/
static bool
d_internal_is_variable(
    const char* _name,
    size_t      _length
)
{
    static const struct d_internal_magic variables[] =
    {
        { "!", true }, { "#bcp47", true }, { "#dir", true },
        { "#language", false }, { "=", true }, { "ARTICLEPAGENAME", true },
        { "ARTICLEPAGENAMEE", true }, { "articlepath", false },
        { "ARTICLESPACE", true }, { "ARTICLESPACEE", true },
        { "BASEPAGENAME", true }, { "BASEPAGENAMEE", true },
        { "CASCADINGSOURCES", true }, { "CONTENTLANG", true },
        { "CONTENTLANGUAGE", true }, { "CURRENTDAY", true },
        { "CURRENTDAY2", true }, { "CURRENTDAYNAME", true },
        { "CURRENTDOW", true }, { "CURRENTHOUR", true },
        { "CURRENTMONTH", true }, { "CURRENTMONTH1", true },
        { "CURRENTMONTH2", true }, { "CURRENTMONTHABBREV", true },
        { "CURRENTMONTHNAME", true }, { "CURRENTMONTHNAMEGEN", true },
        { "CURRENTTIME", true }, { "CURRENTTIMESTAMP", true },
        { "CURRENTVERSION", true }, { "CURRENTWEEK", true },
        { "CURRENTYEAR", true }, { "DIRECTIONMARK", true },
        { "DIRMARK", true }, { "FULLPAGENAME", true },
        { "FULLPAGENAMEE", true }, { "LOCALDAY", true },
        { "LOCALDAY2", true }, { "LOCALDAYNAME", true }, { "LOCALDOW", true },
        { "LOCALHOUR", true }, { "LOCALMONTH", true }, { "LOCALMONTH1", true },
        { "LOCALMONTH2", true }, { "LOCALMONTHABBREV", true },
        { "LOCALMONTHNAME", true }, { "LOCALMONTHNAMEGEN", true },
        { "LOCALTIME", true }, { "LOCALTIMESTAMP", true },
        { "LOCALWEEK", true }, { "LOCALYEAR", true }, { "NAMESPACE", true },
        { "NAMESPACEE", true }, { "NAMESPACENUMBER", true },
        { "NUMBEROFACTIVEUSERS", true }, { "NUMBEROFADMINS", true },
        { "NUMBEROFARTICLES", true }, { "NUMBEROFEDITS", true },
        { "NUMBEROFFILES", true }, { "NUMBEROFPAGES", true },
        { "NUMBEROFUSERS", true }, { "pageid", false },
        { "PAGELANGUAGE", true }, { "PAGENAME", true }, { "PAGENAMEE", true },
        { "REVISIONDAY", true }, { "REVISIONDAY2", true },
        { "REVISIONID", true }, { "REVISIONMONTH", true },
        { "REVISIONMONTH1", true }, { "REVISIONSIZE", true },
        { "REVISIONTIMESTAMP", true }, { "REVISIONUSER", true },
        { "REVISIONYEAR", true }, { "ROOTPAGENAME", true },
        { "ROOTPAGENAMEE", true }, { "scriptpath", false },
        { "server", false }, { "servername", false }, { "SITENAME", true },
        { "stylepath", false }, { "SUBJECTPAGENAME", true },
        { "SUBJECTPAGENAMEE", true }, { "SUBJECTSPACE", true },
        { "SUBJECTSPACEE", true }, { "SUBPAGENAME", true },
        { "SUBPAGENAMEE", true }, { "TALKPAGENAME", true },
        { "TALKPAGENAMEE", true }, { "TALKSPACE", true },
        { "TALKSPACEE", true }, { "USERLANGUAGE", true }
    };

    return d_internal_is_magic(_name, _length, variables,
                               sizeof(variables) / sizeof(variables[0]));
}


/*
d_internal_clean_name
  Copies the bytes from `_start` to `_end` but those of the comments and
dropped tags among them, as MediaWiki reads a template's name, each one's
place on the page kept in `_at`; gives how many there are.
*/
static size_t
d_internal_clean_name(
    const struct d_internal_mw* _mw,
    size_t                      _start,
    size_t                      _end,
    char*                       _out,
    size_t*                     _at
)
{
    size_t count = 0;
    size_t index = d_internal_first_hidden(_mw, _start);
    size_t at    = _start;

    // each byte, past what is dropped
    while (at < _end)
    {
        const struct d_internal_hidden* const hidden =
            (index < _mw->hidden_count) ? &_mw->hidden[index] : NULL;

        // a comment, or a tag dropped, here: past it
        if ( (hidden) &&
             (hidden->start == at) &&
             ( (hidden->kind == D_INTERNAL_COMMENT) ||
               (hidden->kind == D_INTERNAL_IGNORED) ||
               (hidden->kind == D_INTERNAL_INCLUDED) ) )
        {
            at = hidden->end;

            // and what is hidden within it
            while ( (index < _mw->hidden_count) &&
                    (_mw->hidden[index].start < at) )
            {
                index++;
            }

            continue;
        }

        // what is hidden past here
        while ( (index < _mw->hidden_count) &&
                (_mw->hidden[index].start <= at) )
        {
            index++;
        }

        _out[count] = _mw->text[at];
        _at[count]  = at;
        count++;
        at++;
    }

    _out[count] = '\0';
    _at[count]  = _end;

    return count;
}


/*
d_internal_take_prefix
  Takes a prefix, in any case, off the start of a name, `*_first` to
`_end`; gives whether it was there.
*/
static bool
d_internal_take_prefix(
    const char* _name,
    size_t*     _first,
    size_t      _end,
    const char* _prefix
)
{
    const size_t length = strlen(_prefix);

    // the prefix, whole, in any case
    if ( ((_end - *_first) < length) ||
         (!d_internal_starts_with(&_name[*_first], length, _prefix)) )
    {
        return false;
    }

    *_first += length;

    return true;
}


/*
d_internal_function_colon
  Past `msgnw:` or `msg:`, then `raw:`, which MediaWiki takes off a
template's name, `*_first` to `_end`, where a parser function's `:` is: a
name of MediaWiki's before it, or any after `#`, the extensions' --
the name's end for one with none; D_WIKI_NONE for no function.
*/
static size_t
d_internal_function_colon(
    const char* _name,
    size_t*     _first,
    size_t      _end
)
{
    // msgnw: or msg:, then raw:, are taken off the name
    if (!d_internal_take_prefix(_name, _first, _end, "msgnw:"))
    {
        (void)d_internal_take_prefix(_name, _first, _end, "msg:");
    }

    (void)d_internal_take_prefix(_name, _first, _end, "raw:");

    const char* const found = memchr(&_name[*_first], ':', _end - *_first);

    // a function: a name of MediaWiki's before `:`, or one after `#`
    if ( ( (found) &&
           (d_internal_is_function(&_name[*_first],
                                   (size_t)(found - &_name[*_first]))) ) ||
         ( (*_first < _end) &&
           (_name[*_first] == '#') ) )
    {
        return (found) ? (size_t)(found - _name) : _end;
    }

    return D_WIKI_NONE;
}


/*
d_internal_template_kind
  What a template's name says it is, as MediaWiki's braceSubstitution
reads it: `subst:` a function of its own, as the page shows it; past
`safesubst:`, a variable, by a name of MediaWiki's with no arguments;
past `msgnw:` or `msg:`, then `raw:`, a parser function, by a name of
MediaWiki's before `:`, or by `#` for the extensions' functions; else a
template.  Fills in the rest of `_named`.
  Long as braceSubstitution's order is, which it follows: each prefix,
then each kind, read in turn.
*/
static enum d_wiki_template_kind
d_internal_template_kind(
    struct d_internal_mw*    _mw,
    struct d_internal_named* _named
)
{
    const size_t              span  = _named->end - _named->start;
    char* const               name  = malloc(span + 1u);
    size_t* const             at    = malloc((span + 1u) * sizeof(size_t));
    enum d_wiki_template_kind kind  = D_WIKI_TEMPLATE;
    size_t                    first = 0;
    size_t                    end   = 0;
    size_t                    colon = D_WIKI_NONE;

    _named->colon = D_WIKI_NONE;

    // room for the name, and for where its bytes are
    if ( (!name) ||
         (!at) )
    {
        free(name);
        free(at);
        _mw->failed = true;

        return D_WIKI_TEMPLATE;
    }

    end = d_internal_clean_name(_mw, _named->start, _named->end, name, at);
    d_internal_trimmed(name, &first, &end);

    // `subst:` is a function, as the page shows; `safesubst:` is taken off
    if ( (!_named->parameter) &&
         (d_internal_starts_with(&name[first], end - first, "subst:")) )
    {
        kind  = D_WIKI_FUNCTION;
        colon = first + 5u;
    }
    else if ( (!_named->parameter) &&
              (d_internal_take_prefix(name, &first, end, "safesubst:")) )
    {
        d_internal_trimmed(name, &first, &end);
    }

    // a variable: its name alone, with no arguments
    if ( (!_named->parameter) &&
         (kind == D_WIKI_TEMPLATE) &&
         (_named->parts == 1u) &&
         (d_internal_is_variable(&name[first], end - first)) )
    {
        kind = D_WIKI_VARIABLE;
    }
    else if ( (!_named->parameter) &&
              (kind == D_WIKI_TEMPLATE) )
    {
        colon = d_internal_function_colon(name, &first, end);
        kind  = (colon != D_WIKI_NONE) ? D_WIKI_FUNCTION : kind;
    }

    // a function's name is before its `:`, which is kept
    if (colon != D_WIKI_NONE)
    {
        _named->colon = (colon < end) ? at[colon] : _named->end;
        end           = colon;
        d_internal_trimmed(name, &first, &end);
    }

    _named->name_start = at[first];
    _named->name_end   = (end > first) ? (at[end - 1u] + 1u) : at[first];
    _named->name       = d_wiki_copy(&name[first], end - first);
    _mw->failed        = (_mw->failed) || (!_named->name);
    free(name);
    free(at);

    return kind;
}


/*
d_internal_add_argument
  Keeps one argument of a template, from `_start` to `_end`, its `=` at
`_equals` when it is named; a numbered one is given `_number`.
*/
static bool
d_internal_add_argument(
    struct d_internal_mw* _mw,
    size_t                _start,
    size_t                _end,
    size_t                _equals,
    size_t*               _number
)
{
    struct d_wiki_argument argument;
    char                   key[32];

    memset(&argument, 0, sizeof(argument));
    argument.start       = _start;
    argument.end         = _end;
    argument.name_start  = D_WIKI_NONE;
    argument.name_end    = D_WIKI_NONE;
    argument.value_start = _start;
    argument.value_end   = _end;

    // named: its name trimmed is its key
    if (_equals != D_WIKI_NONE)
    {
        size_t name_start = _start;
        size_t name_end   = _equals;

        argument.name_start  = _start;
        argument.name_end    = _equals;
        argument.value_start = _equals + 1u;
        d_internal_trimmed(_mw->text, &name_start, &name_end);
        argument.key = d_wiki_copy(&_mw->text[name_start],
                                   name_end - name_start);
    }
    else
    {
        (void)snprintf(key, sizeof(key), "%zu", (*_number)++);
        argument.key = d_wiki_copy(key, strlen(key));
    }

    // the key, kept with its argument, or the reader fails
    if ( (!argument.key) ||
         (d_wiki_add_argument(_mw->wiki, &argument) == D_WIKI_NONE) )
    {
        free(argument.key);
        _mw->failed = true;

        return false;
    }

    return true;
}


/*
d_internal_template_boundaries
  A template's syntax, as boundaries an external link may not cross: its
braces, `_start` to its first part and its last part's end to `_end`, and
each `|` between its parts.
*/
static void
d_internal_template_boundaries(
    struct d_internal_mw*          _mw,
    const struct d_internal_piece* _piece,
    size_t                         _start,
    size_t                         _end
)
{
    const size_t braces = _piece->parts[0].start - _start;

    // its braces
    for (size_t index = 0; index < braces; ++index)
    {
        d_internal_add_boundary(_mw, _start + index);
        d_internal_add_boundary(_mw, _end - braces + index);
    }

    // each `|` between its parts
    for (size_t index = 1u; index < _piece->part_count; ++index)
    {
        d_internal_add_boundary(_mw, _piece->parts[index].start - 1u);
    }

    return;
}


/*
d_internal_add_template
  Keeps a template closed: a piece's last `_braces` braces to `_close`.
  Long as a template's record is filled from its piece in turn: its name,
its first argument after a function's `:`, then each other part.
*/
static void
d_internal_add_template(
    struct d_internal_mw*          _mw,
    const struct d_internal_piece* _piece,
    size_t                         _braces,
    size_t                         _close
)
{
    struct d_wiki_template  tmpl;
    struct d_internal_named named;
    size_t                  number = 1u;
    const size_t            start  = _piece->start + _piece->count - _braces;
    const size_t            name_e = (_piece->part_count > 1u)
                                         ? (_piece->parts[1].start - 1u)
                                         : _close;

    memset(&tmpl, 0, sizeof(tmpl));
    memset(&named, 0, sizeof(named));
    named.start         = _piece->parts[0].start;
    named.end           = name_e;
    named.parts         = _piece->part_count;
    named.parameter     = (_braces == 3u);
    tmpl.start          = start;
    tmpl.end            = _close + _braces;
    tmpl.first_argument = _mw->wiki->argument_count;
    tmpl.parent         = D_WIKI_NONE;
    tmpl.kind           = d_internal_template_kind(_mw, &named);
    tmpl.kind           = (named.parameter) ? D_WIKI_PARAMETER : tmpl.kind;
    tmpl.name_start     = named.name_start;
    tmpl.name_end       = named.name_end;
    tmpl.name           = named.name;

    // a name that could not be kept
    if (!tmpl.name)
    {
        return;
    }

    // a function's first argument is after its `:`
    if ( (named.colon != D_WIKI_NONE) &&
         (named.colon < name_e) &&
         (!d_internal_add_argument(_mw, named.colon + 1u, name_e, D_WIKI_NONE,
                                   &number)) )
    {
        free(tmpl.name);

        return;
    }

    // the other parts; a function's are numbered all
    for (size_t index = 1u; index < _piece->part_count; ++index)
    {
        const size_t end = ((index + 1u) < _piece->part_count)
                               ? (_piece->parts[index + 1u].start - 1u)
                               : _close;

        // each argument, a function's numbered from its second
        if (!d_internal_add_argument(_mw, _piece->parts[index].start, end,
                                     (tmpl.kind == D_WIKI_FUNCTION)
                                         ? D_WIKI_NONE
                                         : _piece->parts[index].equals,
                                     &number))
        {
            free(tmpl.name);

            return;
        }
    }

    tmpl.argument_count = _mw->wiki->argument_count - tmpl.first_argument;

    // the template, kept, or the reader fails
    if (d_wiki_add_template(_mw->wiki, &tmpl) == D_WIKI_NONE)
    {
        free(tmpl.name);
        _mw->failed = true;

        return;
    }

    d_internal_template_boundaries(_mw, _piece, start, _close + _braces);

    return;
}


/*
d_internal_enclose
  Takes the headings found since a template or a heading opened at
`_start` off the top: what one holds is not a heading of the page's own.
Those found since are the last found, as one closes only after all it
holds.
*/
static void
d_internal_enclose(
    struct d_internal_mw* _mw,
    size_t                _start
)
{
    // the headings found since it opened, the last found first
    for (size_t index = _mw->heading_count; index > 0u; --index)
    {
        struct d_internal_found_heading* const heading =
            &_mw->headings[index - 1u];

        // one found before it opened, as are all before that one
        if (heading->start < _start)
        {
            break;
        }

        heading->top = false;
    }

    return;
}


/*
d_internal_one_line
  Whether a heading's title, `_start` to `_end`, is on one line once
MediaWiki expands it: a line ending within it must be in a comment, an
element or a template, which leave none.
*/
static bool
d_internal_one_line(
    const struct d_internal_mw* _mw,
    size_t                      _start,
    size_t                      _end
)
{
    const struct d_wiki* const wiki = _mw->wiki;

    // each line ending within the title
    for (size_t at = _start; at < _end; ++at)
    {
        bool within = false;

        // a byte that ends no line
        if (_mw->text[at] != '\n')
        {
            continue;
        }

        // within what is set apart
        for (size_t index = d_internal_first_hidden(_mw, _start);
             ( (!within) &&
               (index < _mw->hidden_count) &&
               (_mw->hidden[index].start < _end) ); ++index)
        {
            within = (_mw->hidden[index].start <= at) &&
                     (at < _mw->hidden[index].end);
        }

        // within a template closed since the heading opened, the last first
        for (size_t index = wiki->template_count;
             ( (!within) &&
               (index > 0u) &&
               (wiki->templates[index - 1u].start >= _start) ); --index)
        {
            within = (wiki->templates[index - 1u].start <= at) &&
                     (at < wiki->templates[index - 1u].end);
        }

        // a line ending the expansion keeps
        if (!within)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_close_heading
  Closes the heading open at a line's end, `_at`: its level is the fewer
`=`s of its two ends, past space and comments at the end; a line all `=`s
is a heading of half of them.  A heading holds those found within it, as
MediaWiki's does, though it is none itself when its title is on more
than one line once expanded.
  Long as the preprocessor's line-end case is, which it follows: the close
sought back past space and comments, the level, then the heading kept.
*/
static void
d_internal_close_heading(
    struct d_internal_mw*          _mw,
    const struct d_internal_piece* _piece,
    size_t                         _at,
    bool                           _top
)
{
    const char* const text  = _mw->text;
    size_t            end   = _at;
    size_t            level = 0;

    // back past space, and comments, at the line's end
    for (;;)
    {
        // past the space and the CR at the line's end
        while ( (end > _piece->start) &&
                ( (d_internal_is_space_or_tab(text[end - 1u])) ||
                  (text[end - 1u] == '\r') ) )
        {
            end--;
        }

        bool past = false;

        // past a comment at the line's end, the last first
        for (size_t index = _mw->hidden_count; index > 0u; --index)
        {
            const struct d_internal_hidden* const hidden =
                &_mw->hidden[index - 1u];

            // a comment that ends where the line does
            if ( (hidden->kind == D_INTERNAL_COMMENT) &&
                 (hidden->end == end) &&
                 (hidden->start > _piece->start) )
            {
                end  = hidden->start;
                past = true;

                break;
            }

            // a comment before the line's end: nothing more to pass
            if (hidden->end < end)
            {
                break;
            }
        }

        // nothing more passed: the line ends here
        if (!past)
        {
            break;
        }
    }

    size_t equals = 0;

    // the `=`s that close it
    while ( (end - equals > _piece->start) &&
            (text[end - equals - 1u] == '=') )
    {
        equals++;
    }

    // a line that does not end with `=` is no heading
    if (equals == 0u)
    {
        return;
    }

    // a line all `=`s
    if ((end - equals) == _piece->start)
    {
        level = (equals < 3u) ? 0u : (equals - 1u) / 2u;
        level = (level > D_WIKI_MOST_LEVEL) ? D_WIKI_MOST_LEVEL : level;
    }
    else
    {
        level = (equals < _piece->count) ? equals : _piece->count;
    }

    // no `=` left on either side: no heading
    if (level == 0u)
    {
        return;
    }

    // the headings it holds are its own, not the page's
    d_internal_enclose(_mw, _piece->start);

    // a heading MediaWiki's lines would not show as one
    if (!d_internal_one_line(_mw, _piece->start + level, end - level))
    {
        return;
    }

    // room for one more
    if (!d_internal_grow((void**)&_mw->headings, &_mw->heading_capacity,
                         _mw->heading_count,
                         sizeof(struct d_internal_found_heading)))
    {
        _mw->failed = true;

        return;
    }

    struct d_internal_found_heading* const heading =
        &_mw->headings[_mw->heading_count++];

    heading->start       = _piece->start;
    heading->end         = _at;
    heading->title_start = _piece->start + level;
    heading->title_end   = end - level;
    heading->level       = level;
    heading->top         = _top;
    heading->kept        = false;
    d_internal_trimmed(text, &heading->title_start, &heading->title_end);

    return;
}


/*
d_internal_free_stack
  Frees the pieces left on a stack, keeping a problem for each template
left open.
*/
static void
d_internal_free_stack(
    struct d_internal_mw*    _mw,
    struct d_internal_piece* _stack,
    size_t                   _count
)
{
    // each piece, a brace left open told of
    for (size_t index = 0; index < _count; ++index)
    {
        // a template still open is a problem
        if ( (_stack[index].open == '{') &&
             (_stack[index].count >= 2u) &&
             (!d_wiki_add_problem(_mw->wiki, _stack[index].start,
                                  "a template is not closed")) )
        {
            _mw->failed = true;
        }

        free(_stack[index].parts);
    }

    free(_stack);

    return;
}


/*
d_internal_preprocess
  Reads `_from` to `_end` as MediaWiki's preprocessor does, with a stack of
its own; `_depth` counts the elements it is within.
  Long as Preprocessor_Hash's main loop is, which it follows: one state
machine over the bytes, its stack the state.
*/
static void
d_internal_preprocess(
    struct d_internal_mw* _mw,
    size_t                _from,
    size_t                _end,
    size_t                _depth
)
{
    const char* const        text     = _mw->text;
    struct d_internal_piece* stack    = NULL;
    size_t                   count    = 0;
    size_t                   capacity = 0;
    size_t                   at       = _from;
    bool                     start    = true;

    // each byte, the special ones as the stack's top makes them
    while ( (!_mw->failed) &&
            (at <= _end) )
    {
        struct d_internal_piece* const top = (count > 0u)
                                                 ? &stack[count - 1u] : NULL;
        const char c = (at < _end) ? text[at] : '\0';

        // a heading open closes at the line's end, or the text's
        if ( (top) &&
             (top->open == '\n') &&
             ( (at >= _end) || (c == '\n') ) )
        {
            d_internal_close_heading(_mw, top, at, (_depth == 0u));
            free(top->parts);
            count--;

            continue;
        }

        // the text's end
        if (at >= _end)
        {
            break;
        }

        // a line's start, perhaps a heading's
        if ( (start) ||
             (c == '\n') )
        {
            size_t equals = 0;

            at   += (start) ? 0u : 1u;
            start = false;

            // the `=`s that may open a heading, at a line's start
            while ( ((at + equals) < _end) &&
                    (equals < D_WIKI_MOST_LEVEL) &&
                    (text[at + equals] == '=') )
            {
                equals++;
            }

            // one `=` where a named argument's could be is that
            if ( (equals == 0u) ||
                 ( (equals == 1u) &&
                   (top) &&
                   ( (top->open == '{') || (top->open == '-') ) &&
                   (top->part_count > 1u) &&
                   (top->parts[top->part_count - 1u].equals == D_WIKI_NONE) ) )
            {
                continue;
            }

            // room for one more
            if ( (!d_internal_grow((void**)&stack, &capacity, count,
                                   sizeof(*stack))) ||
                 (!d_internal_new_piece(&stack[count], '\n', equals, at,
                                        at + equals)) )
            {
                _mw->failed = true;

                break;
            }

            count++;
            at += equals;

            continue;
        }

        // a comment, a tag dropped, an extension's element
        if (c == '<')
        {
            const size_t next = d_internal_angle(_mw, at, _end, _depth);

            at = (next > 0u) ? next : (at + 1u);

            continue;
        }

        // two or more braces, or brackets, open; and `-{`, the language
        // converter's markup, but in `-{{` the braces open, `-` held back
        if ( (c == '{') ||
             (c == '[') ||
             ( (c == '-') &&
               ((at + 1u) < _end) &&
               (text[at + 1u] == '{') ) )
        {
            const bool dash  = (c == '-');
            const char brace = (dash) ? '{' : c;
            size_t     run   = (dash) ? 1u : 0u;

            // a run of `{` or `[`, after the `-`
            while ( ((at + run) < _end) &&
                    (text[at + run] == brace) )
            {
                run++;
            }

            const bool   held = (dash) && (run > 2u);
            const size_t many = (held) ? (run - 1u) : run;

            // two or more open a piece; `-{` alone is the converter's
            if (many >= 2u)
            {
                // the piece pushed, or the reader fails
                if ( (!d_internal_grow((void**)&stack, &capacity, count,
                                       sizeof(*stack))) ||
                     (!d_internal_new_piece(&stack[count],
                                            ( (dash) && (!held) ) ? '-'
                                                                  : brace,
                                            many, at + ((held) ? 1u : 0u),
                                            at + run)) )
                {
                    _mw->failed = true;

                    break;
                }

                stack[count].dash = held;
                count++;
            }

            at += run;

            continue;
        }

        // `}-` closes the language converter's markup, which leaves text
        if ( (top) &&
             (top->open == '-') &&
             (c == '}') &&
             ((at + 1u) < _end) &&
             (text[at + 1u] == '-') )
        {
            free(top->parts);
            count--;
            at += 2u;

            continue;
        }

        // the close of the top
        if ( (top) &&
             ( ( (c == '}') && (top->open == '{') ) ||
               ( (c == ']') && (top->open == '[') ) ) )
        {
            size_t run = 0;

            // the run of the close, to as many as the piece opened
            while ( ((at + run) < _end) &&
                    (text[at + run] == c) &&
                    (run < top->count) )
            {
                run++;
            }

            const size_t matching = (c == '}') ? ( (run >= 3u) ? 3u
                                                 : (run == 2u) ? 2u : 0u )
                                               : ( (run >= 2u) ? 2u : 0u );

            // a run that closes nothing is text
            if (matching == 0u)
            {
                at += run;

                continue;
            }

            const size_t element = top->start + top->count - matching;

            // a template, which holds the headings within it; brackets
            // closed leave text
            if (c == '}')
            {
                d_internal_add_template(_mw, top, matching, at);
                d_internal_enclose(_mw, element);
            }

            at         += matching;
            top->count -= matching;

            // the openings left over open again, enough for a piece, or a
            // brace after a `-` held back as `-{`
            if ( (top->count >= 2u) ||
                 ( (top->count == 1u) && (top->dash) ) )
            {
                const bool dash = (top->count == 1u);
                const bool held = top->dash;

                free(top->parts);

                // what is left open is opened again after what closed
                if (!d_internal_new_piece(top, (dash) ? '-' : top->open,
                                          (dash) ? 2u : top->count,
                                          top->start - ((dash) ? 1u : 0u),
                                          element))
                {
                    _mw->failed = true;

                    break;
                }

                top->dash = (held) && (!dash);
            }
            else
            {
                free(top->parts);
                count--;
            }

            continue;
        }

        // a pipe between a template's parts, or the converter's, and the
        // first `=` of one
        if ( (top) &&
             ( (top->open == '{') || (top->open == '-') ) )
        {
            // a `|` begins a part; the first `=` in a part names it
            if (c == '|')
            {
                // the part kept, or the reader fails
                if (!d_internal_add_part(top, at + 1u))
                {
                    _mw->failed = true;

                    break;
                }
            }
            else if ( (c == '=') &&
                      (top->part_count > 1u) &&
                      (top->parts[top->part_count - 1u].equals ==
                           D_WIKI_NONE) )
            {
                top->parts[top->part_count - 1u].equals = at;
            }
        }

        at++;
    }

    d_internal_free_stack(_mw, stack, count);

    return;
}


/*
d_internal_attribute
  Where the value of a tag's attribute of a name begins, among its
attributes from `_from` to `_to`, quoted or not, and where it ends;
D_WIKI_NONE for none.
*/
static size_t
d_internal_attribute(
    const char* _text,
    size_t      _from,
    size_t      _to,
    const char* _name,
    size_t*     _out_end
)
{
    const size_t length = strlen(_name);

    // the name, as a word of its own, then `=`
    for (size_t at = _from; (at + length + 1u) <= _to; ++at)
    {
        size_t start = at + length;

        // the name, after what is no part of a name
        if ( (!d_internal_starts_with(&_text[at], length, _name)) ||
             ( (at > _from) && (!d_internal_is_space(_text[at - 1u])) ) )
        {
            continue;
        }

        // space, then `=`, then space
        while ( (start < _to) &&
                (d_internal_is_space(_text[start])) )
        {
            start++;
        }

        // no `=`: not this attribute's value
        if ( (start >= _to) ||
             (_text[start] != '=') )
        {
            continue;
        }

        start++;

        // and the space after it
        while ( (start < _to) &&
                (d_internal_is_space(_text[start])) )
        {
            start++;
        }

        const char quote = (start < _to) ? _text[start] : '\0';
        size_t     end   = start + ( ( (quote == '"') || (quote == '\'') )
                                         ? 1u : 0u );

        start = end;

        // to its close quote, or to space or the `/` of `/>`
        while ( (end < _to) &&
                ( ( (quote == '"') || (quote == '\'') )
                      ? (_text[end] != quote)
                      : ( (!d_internal_is_space(_text[end])) &&
                          (_text[end] != '/') ) ) )
        {
            end++;
        }

        *_out_end = end;

        return start;
    }

    return D_WIKI_NONE;
}


/*
d_internal_include_only
  <includeonly>, its open tag at `_at` to `_open_end`, set apart with
what it holds to its close, or the text's end: a page shows none of it,
but what it holds is read apart, for the pages and templates it uses.
Gives where reading goes on.
*/
static size_t
d_internal_include_only(
    struct d_internal_mw* _mw,
    size_t                _at,
    size_t                _open_end,
    size_t                _end,
    size_t                _depth
)
{
    const char* const text   = _mw->text;
    size_t            end_at = _end;
    const bool        empty  = (_open_end >= (_at + 2u)) &&
                               (text[_open_end - 2u] == '/');
    const size_t      close  = (empty)
                                   ? _open_end
                                   : d_internal_find_close_tag(text, _end,
                                                               _open_end,
                                                               "includeonly",
                                                               11u, &end_at);
    const size_t      inner  = (close == D_WIKI_NONE) ? _end : close;
    struct d_internal_hidden* const hidden =
        d_internal_hide(_mw, D_INTERNAL_INCLUDED, _at,
                        (empty) ? _open_end
                                : (close == D_WIKI_NONE) ? _end : end_at);

    // memory that ran out
    if (!hidden)
    {
        _mw->failed = true;

        return _end;
    }

    hidden->content     = _open_end;
    hidden->content_end = inner;
    hidden->name        = _at + 1u;
    hidden->name_length = 11u;

    // what it holds, read apart
    if ( (!empty) &&
         (_depth < D_INTERNAL_MOST_DEPTH) )
    {
        d_internal_preprocess(_mw, _open_end, inner, _depth + 1u);
    }

    return hidden->end;
}


static size_t
d_internal_angle(
    struct d_internal_mw* _mw,
    size_t                _at,
    size_t                _end,
    size_t                _depth
)
{
    static const char* const literal[] =
    {
        "nowiki", "pre", "math", "syntaxhighlight", "source", "chem", "ce",
        "score", "graph", "timeline", "templatedata", "templatestyles",
        "hiero", "inputbox", "gallery", "imagemap", "categorytree",
        "mapframe", "maplink", "charinsert", "dynamicpagelist", "rss"
    };
    static const char* const elements[] =
    {
        "ref", "references", "poem", "section", "indicator", "tabber"
    };
    static const char* const ignored[] =
    {
        "noinclude", "onlyinclude"
    };
    const char* const text = _mw->text;

    // a comment, to its close or the text's end
    if ( ((_at + 4u) <= _end) &&
         (strncmp(&text[_at], "<!--", 4u) == 0) )
    {
        size_t close = _at + 4u;

        // to the comment's `-->`
        while ( ((close + 3u) <= _end) &&
                (strncmp(&text[close], "-->", 3u) != 0) )
        {
            close++;
        }

        const bool closed = ((close + 3u) <= _end);

        close = (closed) ? (close + 3u) : _end;

        // the comment set apart; one never closed is a problem
        if ( (!d_internal_hide(_mw, D_INTERNAL_COMMENT, _at, close)) ||
             ( (!closed) &&
               (!d_wiki_add_problem(_mw->wiki, _at,
                                    "a comment is not closed")) ) )
        {
            _mw->failed = true;
        }

        return close;
    }

    const bool   closing = ((_at + 1u) < _end) && (text[_at + 1u] == '/');
    const size_t name    = _at + ((closing) ? 2u : 1u);
    const size_t length  = d_internal_tag_name(text, _end, name);

    const char after = ((name + length) < _end) ? text[name + length] : '\0';

    // no name, or one that goes on: the `<` is text
    if ( (length == 0u) ||
         ( (!d_internal_is_space(after)) &&
           (after != '>') &&
           ( (after != '/') ||
             ((name + length + 1u) >= _end) ||
             (text[name + length + 1u] != '>') ) ) )
    {
        return 0u;
    }

    const char* const tag_end = memchr(&text[name + length], '>',
                                       _end - name - length);

    // no `>`: the `<` is text
    if (!tag_end)
    {
        return 0u;
    }

    const size_t close_at = (size_t)(tag_end - text) + 1u;
    const bool   empty    = (tag_end > &text[name + length]) &&
                            (tag_end[-1] == '/');

    // <includeonly> and what it holds, which a page shows nothing of
    if ( (!closing) &&
         (length == 11u) &&
         (d_internal_starts_with(&text[name], length, "includeonly")) )
    {
        return d_internal_include_only(_mw, _at, close_at, _end, _depth);
    }

    // a tag dropped, open or closing
    if ( (d_internal_in_list(&text[name], length, ignored,
                             sizeof(ignored) / sizeof(ignored[0]))) ||
         ( (length == 11u) &&
           (d_internal_starts_with(&text[name], length, "includeonly")) ) )
    {
        // the tag set apart, or the reader fails
        if (!d_internal_hide(_mw, D_INTERNAL_IGNORED, _at, close_at))
        {
            _mw->failed = true;
        }

        return close_at;
    }

    const bool is_literal = d_internal_in_list(&text[name], length, literal,
                                               sizeof(literal) /
                                                   sizeof(literal[0]));
    const bool is_element = d_internal_in_list(&text[name], length, elements,
                                               sizeof(elements) /
                                                   sizeof(elements[0]));

    // only an extension's open tag begins an element
    if ( (closing) ||
         ( (!is_literal) && (!is_element) ) )
    {
        return 0u;
    }

    struct d_internal_hidden* hidden = NULL;

    // <nowiki/>: an element with nothing in it
    if (empty)
    {
        hidden = d_internal_hide(_mw, (is_literal) ? D_INTERNAL_LITERAL
                                                   : D_INTERNAL_ELEMENT,
                                 _at, close_at);

        // the element's name, and its tags
        if (hidden)
        {
            hidden->name           = name;
            hidden->name_length    = length;
            hidden->attributes     = name + length;
            hidden->attributes_end = close_at - 2u;
        }
        else
        {
            _mw->failed = true;
        }

        return close_at;
    }

    size_t       end_at = 0;
    const size_t close  = d_internal_find_close_tag(text, _end, close_at,
                                                    &text[name], length,
                                                    &end_at);

    // an open tag never closed is text, as MediaWiki makes it
    if (close == D_WIKI_NONE)
    {
        // the problem kept, or the reader fails
        if (!d_wiki_add_problem(_mw->wiki, _at,
                                "an extension's tag is not closed"))
        {
            _mw->failed = true;
        }

        return close_at;
    }

    hidden = d_internal_hide(_mw, (is_literal) ? D_INTERNAL_LITERAL
                                               : D_INTERNAL_ELEMENT,
                             _at, end_at);

    // memory that ran out
    if (!hidden)
    {
        _mw->failed = true;

        return end_at;
    }

    hidden->content        = close_at;
    hidden->content_end    = close;
    hidden->name           = name;
    hidden->name_length    = length;
    hidden->attributes     = name + length;
    hidden->attributes_end = close_at - 1u;

    const bool   gallery = (length == 7u) &&
                           (d_internal_starts_with(&text[name], length,
                                                   "gallery"));
    size_t       caption_end = 0;
    const size_t caption     = (gallery)
                                   ? d_internal_attribute(text,
                                                          name + length,
                                                          close_at - 1u,
                                                          "caption",
                                                          &caption_end)
                                   : D_WIKI_NONE;

    // a gallery's caption is wikitext, before its lines
    if ( (caption != D_WIKI_NONE) &&
         (_depth < D_INTERNAL_MOST_DEPTH) )
    {
        d_internal_preprocess(_mw, caption, caption_end, _depth + 1u);
    }

    // an element's content is read on its own, and a gallery's, whose
    // lines hold wikitext
    if ( ( (is_element) || (gallery) ) &&
         (_depth < D_INTERNAL_MOST_DEPTH) )
    {
        d_internal_preprocess(_mw, close_at, close, _depth + 1u);
    }

    return end_at;
}


//==============================================================================
// 4.  STREAMS
//==============================================================================


/*
d_internal_stream_add
  Adds a run of the page to a stream.
*/
static void
d_internal_stream_add(
    struct d_internal_stream* _stream,
    size_t                    _source,
    size_t                    _length,
    const char*               _text
)
{
    // room for one more piece
    if (!d_internal_grow((void**)&_stream->pieces, &_stream->piece_capacity,
                         _stream->piece_count,
                         sizeof(struct d_internal_piece_map)))
    {
        _stream->failed = true;

        return;
    }

    _stream->pieces[_stream->piece_count].joined = _stream->joined.length;
    _stream->pieces[_stream->piece_count].source = _source;
    _stream->pieces[_stream->piece_count].length = _length;
    _stream->piece_count++;

    // the bytes joined, or the stream fails
    if (!d_wiki_buffer_add(&_stream->joined, _text, _length))
    {
        _stream->failed = true;
    }

    return;
}


/*
d_internal_stream_source
  Where a place in a stream stands on the page.
*/
static size_t
d_internal_stream_source(
    const struct d_internal_stream* _stream,
    size_t                          _at
)
{
    size_t low  = 0;
    size_t high = _stream->piece_count;

    // nothing joined stands at the start
    if (_stream->piece_count == 0u)
    {
        return 0u;
    }

    // the last piece that begins at or before the place
    while ((low + 1u) < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // the pieces from the middle, or those before it
        if (_stream->pieces[middle].joined <= _at)
        {
            low = middle;
        }
        else
        {
            high = middle;
        }
    }

    const struct d_internal_piece_map* const piece = &_stream->pieces[low];
    const size_t                             into  = _at - piece->joined;

    return piece->source + ((into > piece->length) ? piece->length : into);
}


/*
d_internal_shut
  Covers what of a stream is a table's cells' attributes, which hold no
link.
*/
static void
d_internal_shut(
    struct d_internal_stream* _stream
)
{
    const struct d_internal_mw* const mw = _stream->mw;

    // each run of the stream
    for (size_t piece = 0; piece < _stream->piece_count; ++piece)
    {
        const struct d_internal_piece_map* const run = &_stream->pieces[piece];
        const size_t                             end = run->source +
                                                       run->length;

        // each span of attributes within the run
        for (size_t span = 0; (span + 1u) < mw->shut_count; span += 2u)
        {
            const size_t from = (mw->shut[span] > run->source)
                                    ? mw->shut[span] : run->source;
            const size_t to   = (mw->shut[span + 1u] < end)
                                    ? mw->shut[span + 1u] : end;

            // a span that meets the run
            if (from < to)
            {
                memset(&_stream->covered[run->joined + (from - run->source)],
                       D_INTERNAL_LINKED, to - from);
            }
        }
    }

    return;
}


/*
d_internal_build_stream
  The text from `_from` to `_end` as a stream: what is dropped gone, each
element a mark, from the record of what is hidden at `_hidden` on.
*/
static void
d_internal_build_stream(
    struct d_internal_stream* _stream,
    size_t                    _from,
    size_t                    _end,
    size_t                    _hidden
)
{
    const struct d_internal_mw* const mw    = _stream->mw;
    size_t                            at    = _from;
    size_t                            index = _hidden;

    // runs of text between what is hidden
    while (at < _end)
    {
        // what is hidden before here is past
        while ( (index < mw->hidden_count) &&
                (mw->hidden[index].start < at) )
        {
            index++;
        }

        const size_t next = ( (index < mw->hidden_count) &&
                              (mw->hidden[index].start < _end) )
                                ? mw->hidden[index].start : _end;

        // the bytes up to what is set apart
        if (next > at)
        {
            d_internal_stream_add(_stream, at, next - at, &mw->text[at]);
        }

        // nothing set apart past the end
        if (next >= _end)
        {
            break;
        }

        const struct d_internal_hidden* const hidden = &mw->hidden[index];

        // an element is a mark; a comment and a dropped tag are nothing
        if ( (hidden->kind == D_INTERNAL_LITERAL) ||
             (hidden->kind == D_INTERNAL_ELEMENT) )
        {
            const char mark = D_INTERNAL_MARK;

            d_internal_stream_add(_stream, hidden->start, 1u, &mark);
        }

        at = hidden->end;
    }

    (void)d_wiki_buffer_add(&_stream->joined, "", 0u);
    _stream->covered = calloc(_stream->joined.length + 1u, 1u);

    // memory that ran out
    if (!_stream->covered)
    {
        _stream->failed = true;

        return;
    }

    d_internal_shut(_stream);

    return;
}


/*
d_internal_free_stream
  Frees what a stream holds.
*/
static void
d_internal_free_stream(
    struct d_internal_stream* _stream
)
{
    d_wiki_buffer_free(&_stream->joined);
    free(_stream->pieces);
    free(_stream->covered);
    free(_stream->images);

    return;
}


/*
d_internal_crosses
  Whether a span of the page holds a boundary of a template.
*/
static bool
d_internal_crosses(
    const struct d_internal_mw* _mw,
    size_t                      _start,
    size_t                      _end
)
{
    size_t low  = 0;
    size_t high = _mw->boundary_count;

    // the first boundary at or after the start, as they are sorted
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // the boundaries after the start, or those to the middle
        if (_mw->boundaries[middle] < _start)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < _mw->boundary_count) &&
             (_mw->boundaries[low] < _end) );
}


/*
d_internal_compare_offsets
  Orders offsets.
*/
static int
d_internal_compare_offsets(
    const void* _left,
    const void* _right
)
{
    const size_t left  = *(const size_t*)_left;
    const size_t right = *(const size_t*)_right;

    return (left < right) ? -1 : (left > right) ? 1 : 0;
}


//==============================================================================
// 5.  PLAIN TEXT
//==============================================================================


/*
d_internal_plain
  Writes what wikitext of a stream shows, `_from` to `_to`: links as their
labels, templates as nothing, emphasis' apostrophes, the HTML tags the
sanitizer keeps, magic words and marks gone, references read; another
tag is text.
  Long as a dispatch on what the byte read begins is.
*/
static void
d_internal_plain(
    const struct d_internal_stream* _stream,
    size_t                          _from,
    size_t                          _to,
    struct d_wiki_buffer*           _out
);


//==============================================================================
// 6.  LINKS
//==============================================================================


/*
d_internal_namespace
  The namespace a title is in, by its prefix: 1 for File or Image, 2 for
Media, 3 for Category, 0 for any other.
*/
static int
d_internal_namespace(
    const char* _title,
    size_t      _length
)
{
    const char* const colon = memchr(_title, ':', _length);

    // a prefix before `:`, trimmed, in any case
    if (!colon)
    {
        return 0;
    }

    size_t start = 0;
    size_t end   = (size_t)(colon - _title);

    // past the space before the namespace
    while ( (start < end) &&
            (d_internal_is_space_or_tab(_title[start])) )
    {
        start++;
    }

    // and before the colon
    while ( (end > start) &&
            (d_internal_is_space_or_tab(_title[end - 1u])) )
    {
        end--;
    }

    const size_t span = end - start;

    return ( ( (span == 4u) &&
               (d_internal_starts_with(&_title[start], span, "file")) ) ||
             ( (span == 5u) &&
               (d_internal_starts_with(&_title[start], span, "image")) ) )
               ? 1
         : ( (span == 5u) &&
             (d_internal_starts_with(&_title[start], span, "media")) )
               ? 2
         : ( (span == 8u) &&
             (d_internal_starts_with(&_title[start], span, "category")) )
               ? 3
         : 0;
}


/*
d_internal_listed
  Whether a part, in its own case, is one of a list's words -- or, with
`_prefix`, begins with one.
*/
static bool
d_internal_listed(
    const char*        _part,
    size_t             _length,
    const char* const* _list,
    size_t             _count,
    bool               _prefix
)
{
    // each word of the list
    for (size_t index = 0; index < _count; ++index)
    {
        const size_t length = strlen(_list[index]);

        // the word, whole or beginning the part
        if ( ( (_prefix) ? (_length >= length) : (_length == length) ) &&
             (memcmp(_part, _list[index], length) == 0) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_image_part
  Reads one part of an image's link, trimmed, as MediaWiki's makeImage
reads its options, in their own case: a format -- the first only,
`*_seen` set, and framed unless frameless; alt= and link=, where their
values are; another option; or, the last of what is none, its caption.
  Long as makeImage's option words are, listed here.
*/
static void
d_internal_image_part(
    const char*              _text,
    size_t                   _start,
    size_t                   _end,
    bool*                    _seen,
    struct d_internal_image* _out
)
{
    static const char* const framed[] =
    {
        "thumb", "thumbnail", "frame", "framed", "enframed"
    };
    static const char* const manual[] =
    {
        "thumb=", "thumbnail="
    };
    static const char* const options[] =
    {
        "border", "left", "right", "center", "centre", "none", "baseline",
        "sub", "super", "sup", "top", "text-top", "middle", "bottom",
        "text-bottom", "upright"
    };
    static const char* const named[] =
    {
        "page=", "page ", "lang=", "class=", "upright=", "upright "
    };
    const char* const part      = &_text[_start];
    const size_t      length    = _end - _start;
    const bool        frame     = ( (d_internal_listed(part, length, framed,
                                                       5u, false)) ||
                                    (d_internal_listed(part, length, manual,
                                                       2u, true)) );
    const bool        frameless = (length == 9u) &&
                                  (memcmp(part, "frameless", 9u) == 0);
    size_t            at        = 0;

    // a format, the first only, framed unless frameless; one seen already
    // is the caption, as MediaWiki takes it
    if ( (frame) ||
         (frameless) )
    {
        _out->framed      = (*_seen) ? _out->framed : frame;
        _out->caption     = (*_seen) ? _start : _out->caption;
        _out->caption_end = (*_seen) ? _end : _out->caption_end;
        *_seen            = true;

        return;
    }

    // alt= and link=, their values after them
    if ( (length >= 4u) &&
         (memcmp(part, "alt=", 4u) == 0) )
    {
        _out->alt     = _start + 4u;
        _out->alt_end = _end;

        return;
    }
    else if ( (length >= 5u) &&
              (memcmp(part, "link=", 5u) == 0) )
    {
        _out->link     = _start + 5u;
        _out->link_end = _end;

        return;
    }

    // a size: 200px, x100px, 200x100px
    while ( (at < length) &&
            ( ( (part[at] >= '0') && (part[at] <= '9') ) ||
              (part[at] == 'x') ) )
    {
        at++;
    }

    // another option, or a size
    if ( (d_internal_listed(part, length, options,
                            sizeof(options) / sizeof(options[0]), false)) ||
         (d_internal_listed(part, length, named,
                            sizeof(named) / sizeof(named[0]), true)) ||
         ( (at > 0u) &&
           ((length - at) == 2u) &&
           (part[at] == 'p') &&
           (part[at + 1u] == 'x') ) )
    {
        return;
    }

    _out->caption     = _start;
    _out->caption_end = _end;

    return;
}


/*
d_internal_image_parts
  Reads an image's parts, `_from` to `_to` of a stream: apart by `|`, but
for one within a link or a template.
*/
static void
d_internal_image_parts(
    const struct d_internal_stream* _stream,
    size_t                          _from,
    size_t                          _to,
    struct d_internal_image*        _out
)
{
    const char* const text  = _stream->joined.text;
    size_t            part  = _from;
    size_t            depth = 0;
    bool              seen  = false;

    memset(_out, 0, sizeof(*_out));
    _out->caption     = D_WIKI_NONE;
    _out->caption_end = D_WIKI_NONE;
    _out->alt         = D_WIKI_NONE;
    _out->alt_end     = D_WIKI_NONE;
    _out->link        = D_WIKI_NONE;
    _out->link_end    = D_WIKI_NONE;

    // each part, apart by `|` outside links within
    for (size_t at = _from; at <= _to; ++at)
    {
        const bool two = ((at + 1u) < _to) && (text[at] == text[at + 1u]);

        // a link within the caption opens
        if ( (two) &&
             (text[at] == '[') )
        {
            depth++;
            at++;

            continue;
        }

        // and closes
        if ( (two) &&
             (text[at] == ']') &&
             (depth > 0u) )
        {
            depth--;
            at++;

            continue;
        }

        // a part ends at `|`, not within a link or a template, or at the end
        if ( (at == _to) ||
             ( (text[at] == '|') &&
               (depth == 0u) &&
               (!d_internal_crosses(_stream->mw,
                                    d_internal_stream_source(_stream, at),
                                    d_internal_stream_source(_stream, at) +
                                    1u)) ) )
        {
            size_t start = part;
            size_t end   = at;

            d_internal_trimmed(text, &start, &end);
            d_internal_image_part(text, start, end, &seen, _out);
            part = at + 1u;
        }
    }

    return;
}


/*
d_internal_image_label
  An image's text: its caption, or its alt=.
*/
static void
d_internal_image_label(
    const struct d_internal_stream* _stream,
    size_t                          _from,
    size_t                          _to,
    struct d_wiki_buffer*           _out
)
{
    struct d_internal_image image;

    d_internal_image_parts(_stream, _from, _to, &image);

    // the caption, or the alt=, as plain text
    if (image.caption != D_WIKI_NONE)
    {
        d_internal_plain(_stream, image.caption, image.caption_end, _out);
    }
    else if (image.alt != D_WIKI_NONE)
    {
        d_internal_plain(_stream, image.alt, image.alt_end, _out);
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return;
}


/*
d_internal_find_close
  Where the first `]]` is in a stream from `_at` to `_end`; D_WIKI_NONE for
none.
*/
static size_t
d_internal_find_close(
    const struct d_internal_stream* _stream,
    size_t                          _at,
    size_t                          _end
)
{
    const char* const text = _stream->joined.text;

    // the first ]]
    for (size_t at = _at; (at + 1u) < _end; ++at)
    {
        // a `]]`
        if ( (text[at] == ']') &&
             (text[at + 1u] == ']') )
        {
            return at;
        }
    }

    return D_WIKI_NONE;
}


/*
d_internal_keep
  Adds a link found in a stream, its places in the stream given.
*/
static size_t
d_internal_keep(
    struct d_internal_stream* _stream,
    struct d_wiki_link*       _link
)
{
    const size_t index = d_wiki_add_link(_stream->mw->wiki, _link);

    // a link not kept frees what it holds
    if (index == D_WIKI_NONE)
    {
        free(_link->target);
        free(_link->page);
        free(_link->anchor);
        free(_link->label);
        _stream->failed = true;
    }

    return index;
}


/*
d_internal_page_and_anchor
  Splits a link's title, decoded, at its `#`: the page before, its leading
`:` and space gone, and the anchor after.
*/
static bool
d_internal_page_and_anchor(
    struct d_wiki_link* _link
)
{
    const char* const title = _link->target;
    const char* const hash  = strchr(title, '#');
    size_t            start = 0;
    size_t            end   = (hash) ? (size_t)(hash - title) : strlen(title);

    // the page, past `:` and space
    while ( (start < end) &&
            ( (title[start] == ':') ||
              (d_internal_is_space_or_tab(title[start])) ) )
    {
        start++;
    }

    // a title's space at its end
    while ( (end > start) &&
            (d_internal_is_space_or_tab(title[end - 1u])) )
    {
        end--;
    }

    _link->page   = d_wiki_copy(&title[start], end - start);
    _link->anchor = d_wiki_copy((hash) ? (hash + 1) : "",
                                (hash) ? strlen(hash + 1) : 0u);

    return ( (_link->page) && (_link->anchor) );
}


/*
d_internal_link_title
  The start of a piece after `[[` at `_open`, to `_next`: a title, then
]] or a label and ]] -- or, with no ]] in the piece, an image whose caption
holds links.  Gives whether it begins a link, and what it holds.
  Long as a title's end is read four ways: ]], `|`, {{!}}, or no ]] at
all for an image whose caption holds links.
*/
static bool
d_internal_link_title(
    const struct d_internal_stream* _stream,
    size_t                          _open,
    size_t                          _next,
    struct d_internal_internal*     _out
)
{
    const char* const text  = _stream->joined.text;
    const size_t      start = _open + 2u;
    size_t            title = start;

    // the title: the bytes a title may hold, and `<` and `>` that are no
    // tag the sanitizer keeps, which it has made references by then; to
    // `|`, or {{!}}, the pipe MediaWiki makes of it
    while ( (title < _next) &&
            ( (d_internal_is_title_byte(text[title])) ||
              (text[title] == '>') ||
              ( (text[title] == '<') &&
                (d_internal_html_tag(text, title, _next) == 0u) ) ) &&
            (strncmp(&text[title], "{{!}}", 5u) != 0) )
    {
        title++;
    }

    const size_t bar = (title >= _next) ? 0u
                     : (text[title] == '|') ? 1u
                     : ( ((title + 5u) <= _next) &&
                         (strncmp(&text[title], "{{!}}", 5u) == 0) ) ? 5u
                     : 0u;

    memset(_out, 0, sizeof(*_out));
    _out->close     = D_WIKI_NONE;
    _out->label     = D_WIKI_NONE;
    _out->label_end = D_WIKI_NONE;

    // no title, or neither ]] nor a pipe after it
    if ( (title == start) ||
         (title >= _next) ||
         ( (bar == 0u) &&
           ( ((title + 1u) >= _next) ||
             (text[title] != ']') ||
             (text[title + 1u] != ']') ) ) )
    {
        return false;
    }

    // ]] after it, or a label and ]]; no ]] in the piece makes an image
    // whose caption holds links, perhaps
    if (bar == 0u)
    {
        _out->close = title;
    }
    else
    {
        _out->close     = d_internal_find_close(_stream, title + bar, _next);
        _out->label     = title + bar;
        _out->label_end = _out->close;
        _out->image     = (_out->close == D_WIKI_NONE);
    }

    _out->title_start = start;
    _out->title_end   = title;

    // the title past its space
    while ( (_out->title_start < _out->title_end) &&
            (text[_out->title_start] == ' ') )
    {
        _out->title_start++;
    }

    const size_t span = _out->title_end - _out->title_start;

    _out->noforce = (text[_out->title_start] != ':');
    _out->space   = d_internal_namespace(&text[_out->title_start] +
                                         ((_out->noforce) ? 0 : 1),
                                         span - ((_out->noforce) ? 0u : 1u));

    // `[[a|]]` links to nothing, and a URL's protocol makes no link, `//`
    // among them
    return ( (bar == 0u) || (_out->close != (title + bar)) ) &&
           (d_internal_protocol(&text[_out->title_start], span, true) == 0u);
}


/*
d_internal_image_close
  Where an image whose caption holds links closes: the `]]` after the link
the first `]]` of a later piece closes.  Gives whether it does; an image
needs its namespace written, with nothing before it.
*/
static bool
d_internal_image_close(
    const struct d_internal_stream* _stream,
    const struct d_internal_opens*  _opens,
    struct d_internal_internal*     _link
)
{
    // an image needs its namespace written, and nothing before it
    if ( (_link->space != 1) ||
         (!_link->noforce) )
    {
        return false;
    }

    // each piece after, until one closes a link and then the image
    for (size_t index = _opens->index + 1u; index < _opens->count; ++index)
    {
        const size_t piece_end = ((index + 1u) < _opens->count)
                                     ? _opens->at[index + 1u]
                                     : _stream->joined.length;
        const size_t first     = d_internal_find_close(_stream,
                                                       _opens->at[index] + 2u,
                                                       piece_end);
        const size_t second    = (first != D_WIKI_NONE)
                                     ? d_internal_find_close(_stream,
                                                             first + 2u,
                                                             piece_end)
                                     : D_WIKI_NONE;

        // no `]]` in this piece: the image is not closed
        if (first == D_WIKI_NONE)
        {
            return false;
        }

        // a second `]]`, after the link the first closed
        if (second != D_WIKI_NONE)
        {
            _link->close     = second;
            _link->label_end = second;

            return true;
        }
    }

    return false;
}


/*
d_internal_percent_decode
  Writes text with each `%` and two hex digits the byte they stand for, as
PHP's rawurldecode does.
*/
static bool
d_internal_percent_decode(
    struct d_wiki_buffer* _out,
    const char*           _text,
    size_t                _length
)
{
    static const char hex[] = "0123456789abcdef";

    // each byte, or the byte a `%` stands for
    for (size_t at = 0; at < _length; ++at)
    {
        // `%` and two hex digits
        if ( (_text[at] == '%') &&
             ((at + 2u) < _length) &&
             (d_internal_is_hex(_text[at + 1u])) &&
             (d_internal_is_hex(_text[at + 2u])) )
        {
            const size_t high = (size_t)(strchr(hex, d_internal_lower(
                                                         _text[at + 1u])) -
                                         hex);
            const size_t low  = (size_t)(strchr(hex, d_internal_lower(
                                                         _text[at + 2u])) -
                                         hex);

            (void)d_wiki_buffer_add_char(_out, (char)((high << 4) | low));
            at += 2u;

            continue;
        }

        (void)d_wiki_buffer_add_char(_out, _text[at]);
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_title_text
  Writes a link's title as MediaWiki reads it: percent-decoded when it
holds `%`, then its references read.
*/
static bool
d_internal_title_text(
    struct d_wiki_buffer* _out,
    const char*           _title,
    size_t                _length
)
{
    struct d_wiki_buffer decoded = { NULL, 0u, 0u, false };

    // no `%`: its references read
    if (!memchr(_title, '%', _length))
    {
        return d_internal_unescape(_out, _title, _length);
    }

    const bool ok = (d_internal_percent_decode(&decoded, _title, _length)) &&
                    (d_internal_unescape(_out, decoded.text,
                                         decoded.length));

    d_wiki_buffer_free(&decoded);

    return ok;
}


/*
d_internal_valid_page
  Whether a page's name, decoded, is one MediaWiki's titles may have: no
control, bracket, `|`, `<` or `>`; no `%` and two hex digits, nor a
reference, left in it; no `~~~`; not `.` or `..` as a part of a path.
*/
static bool
d_internal_valid_page(
    const char* _page
)
{
    const size_t length = strlen(_page);

    // each byte, and what begins at it
    for (size_t at = 0; at < length; ++at)
    {
        const unsigned char c = (unsigned char)_page[at];

        // a byte no title holds, `%` and hex, or `~~~`
        if ( (c < 0x20u) ||
             (c == 0x7Fu) ||
             (strchr("[]|<>", (char)c) != NULL) ||
             ( (c == '%') &&
               ((at + 2u) < length) &&
               (d_internal_is_hex(_page[at + 1u])) &&
               (d_internal_is_hex(_page[at + 2u])) ) ||
             (strncmp(&_page[at], "~~~", 3u) == 0) )
        {
            return false;
        }

        // a reference: `&`, a name or a number, `;`
        if (c == ';')
        {
            // what it ends is a reference
            if (d_internal_ends_reference(_page, 0u, at))
            {
                return false;
            }
        }
    }

    return (strcmp(_page, ".") != 0) &&
           (strcmp(_page, "..") != 0) &&
           (strncmp(_page, "./", 2u) != 0) &&
           (strncmp(_page, "../", 3u) != 0) &&
           (strstr(_page, "/./") == NULL) &&
           (strstr(_page, "/../") == NULL) &&
           ( (length < 2u) || (strcmp(&_page[length - 2u], "/.") != 0) ) &&
           ( (length < 3u) || (strcmp(&_page[length - 3u], "/..") != 0) );
}


/*
d_internal_whole_url
  Whether a text is a URL, whole, its protocol `_scheme` bytes: an
address -- IPv6's in brackets -- then the characters a URL may hold, to
its end.
*/
static bool
d_internal_whole_url(
    const char* _text,
    size_t      _scheme,
    size_t      _length
)
{
    size_t at = _scheme;

    // an IPv6 address, in brackets
    if (_text[at] == '[')
    {
        size_t close = at + 1u;

        // its hex digits, `:`s and `.`s
        while ( (close < _length) &&
                ( (d_internal_is_hex(_text[close])) ||
                  (_text[close] == ':') || (_text[close] == '.') ) )
        {
            close++;
        }

        // its close
        if ( (close > (at + 1u)) &&
             (close < _length) &&
             (_text[close] == ']') )
        {
            at = close + 1u;
        }
    }

    // each character, one a URL may hold
    while (at < _length)
    {
        const size_t width = d_internal_url_char(&_text[at]);

        // one no URL holds
        if (width == 0u)
        {
            return false;
        }

        at += width;
    }

    return (at > _scheme);
}


/*
d_internal_holds_pair
  Whether a stream's text from `_from` to `_to` holds two of a byte in a
row: `[[`, `{{`.
*/
static bool
d_internal_holds_pair(
    const char* _text,
    size_t      _from,
    size_t      _to,
    char        _c
)
{
    // each place but the last
    for (size_t at = _from; (at + 1u) < _to; ++at)
    {
        // two of the byte
        if ( (_text[at] == _c) &&
             (_text[at + 1u] == _c) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_option_target
  Where an image's link= leads, from what its value shows: a URL as it is,
or a page by its title percent-decoded, the link then kept when MediaWiki
would have a title of it.
*/
static void
d_internal_option_target(
    struct d_internal_stream* _stream,
    struct d_wiki_link*       _link,
    struct d_wiki_buffer*     _shown,
    size_t                    _scheme
)
{
    struct d_wiki_buffer decoded = { NULL, 0u, 0u, false };

    // a URL, as it is shown
    if (_scheme > 0u)
    {
        _link->to     = (d_internal_starts_with(_shown->text, _scheme,
                                                "mailto:"))
                            ? D_WIKI_TO_MAIL : D_WIKI_TO_URL;
        _link->target = d_wiki_buffer_take(_shown);
        _link->page   = d_wiki_copy("", 0u);
        _link->anchor = d_wiki_copy("", 0u);
    }
    else
    {
        (void)d_internal_percent_decode(&decoded, _shown->text,
                                        _shown->length);
        _link->target = d_wiki_buffer_take(&decoded);
        _link->to     = D_WIKI_TO_PAGE;
    }

    // memory that ran out
    if ( (!_link->target) ||
         (!_link->label) ||
         ( (_scheme > 0u) && ( (!_link->page) || (!_link->anchor) ) ) ||
         ( (_scheme == 0u) && (!d_internal_page_and_anchor(_link)) ) )
    {
        free(_link->target);
        free(_link->label);
        free(_link->page);
        free(_link->anchor);
        _stream->failed = true;

        return;
    }

    // a page no title can name makes no link
    if ( (_scheme == 0u) &&
         (!d_internal_valid_page(_link->page)) )
    {
        free(_link->target);
        free(_link->label);
        free(_link->page);
        free(_link->anchor);

        return;
    }

    // an anchor of this page
    if ( (_scheme == 0u) &&
         (_link->page[0] == '\0') )
    {
        _link->to = D_WIKI_TO_ANCHOR;
    }

    (void)d_internal_keep(_stream, _link);

    return;
}


/*
d_internal_option_link
  An image's link=, its value `_value` to `_end` of a stream, kept as a link
of its own, as MediaWiki's parseLinkParameter reads it: none for nothing,
a URL for a value with a protocol that is one whole, else a page by its
title percent-decoded.  A value that holds a link or a template makes
none: what it holds is the link.
*/
static void
d_internal_option_link(
    struct d_internal_stream* _stream,
    size_t                    _value,
    size_t                    _end
)
{
    const char* const    text  = _stream->joined.text;
    struct d_wiki_buffer shown = { NULL, 0u, 0u, false };
    struct d_wiki_link   link;

    // nothing, or a link or a template within
    if ( (_value >= _end) ||
         (d_internal_holds_pair(text, _value, _end, '[')) ||
         (d_internal_holds_pair(text, _value, _end, '{')) )
    {
        return;
    }

    d_internal_plain(_stream, _value, _end, &shown);
    (void)d_wiki_buffer_add(&shown, "", 0u);

    const size_t scheme = (shown.length > 0u)
                              ? d_internal_protocol(shown.text, shown.length,
                                                    true)
                              : 0u;

    // nothing shown links nowhere; a protocol must begin a URL whole
    if ( (shown.failed) ||
         (shown.length == 0u) ||
         ( (scheme > 0u) &&
           (!d_internal_whole_url(shown.text, scheme, shown.length)) ) )
    {
        _stream->failed = (_stream->failed) || (shown.failed);
        d_wiki_buffer_free(&shown);

        return;
    }

    d_wiki_new_link(&link, D_WIKI_FORM_OPTION,
                    d_internal_stream_source(_stream, _value - 5u),
                    d_internal_stream_source(_stream, _end));
    link.close        = link.end;
    link.target_start = d_internal_stream_source(_stream, _value);
    link.target_end   = link.end;
    link.label        = d_wiki_copy("", 0u);
    d_internal_option_target(_stream, &link, &shown, scheme);
    d_wiki_buffer_free(&shown);

    return;
}


/*
d_internal_image_kept
  What an image kept holds: its caption, shown when it is framed, may hold
bare URLs; its parts are read again, on their own, for [url text]; and
its link= is a link of its own.
*/
static void
d_internal_image_kept(
    struct d_internal_stream*         _stream,
    const struct d_internal_internal* _parts
)
{
    struct d_internal_image image;

    // an image with nothing after its name holds nothing
    if (_parts->label == D_WIKI_NONE)
    {
        return;
    }

    d_internal_image_parts(_stream, _parts->label, _parts->label_end, &image);

    // a caption shown
    if ( (image.framed) &&
         (image.caption != D_WIKI_NONE) )
    {
        memset(&_stream->covered[image.caption], D_INTERNAL_CAPTIONED,
               image.caption_end - image.caption);
    }

    // room for where its parts are
    if (!d_internal_grow((void**)&_stream->images, &_stream->image_capacity,
                         _stream->image_count + 1u, sizeof(size_t)))
    {
        _stream->failed = true;

        return;
    }

    _stream->images[_stream->image_count++] = _parts->label;
    _stream->images[_stream->image_count++] = _parts->label_end;

    // its link=, a link of its own
    if (image.link != D_WIKI_NONE)
    {
        d_internal_option_link(_stream, image.link, image.link_end);
    }

    return;
}


/*
d_internal_destination
  Where a link's title, decoded, leads by its namespace: Media: to a file,
with a `:` before it or not; File:, Image: and Category: in place, without
one; any other to a page.
*/
static enum d_wiki_target
d_internal_destination(
    const char* _title
)
{
    const bool   noforce = (_title[0] != ':');
    const size_t skip    = (noforce) ? 0u : 1u;
    const int    space   = d_internal_namespace(_title + skip,
                                                strlen(_title) - skip);

    return (space == 2) ? D_WIKI_TO_FILE
         : (!noforce) ? D_WIKI_TO_PAGE
         : (space == 1) ? D_WIKI_TO_IMAGE
         : (space == 3) ? D_WIKI_TO_CATEGORY
         :                D_WIKI_TO_PAGE;
}


/*
d_internal_internal_record
  The link a piece read makes, kept: where it is, its trail with it for a
link to a page; where it leads, its title percent-decoded and its
references read, none for a title MediaWiki would not have; and what it
holds, which no other link may be within -- but for an image's.
  Long as a link's record is filled in turn: its places, its title
decoded, its trail, its page and anchor, then what it covers.
*/
static void
d_internal_internal_record(
    struct d_internal_stream*         _stream,
    size_t                            _open,
    const struct d_internal_internal* _parts
)
{
    const char* const    text   = _stream->joined.text;
    struct d_wiki_link   link;
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };
    size_t               end    = _parts->close + 2u;

    d_wiki_new_link(&link, D_WIKI_FORM_WIKI,
                    d_internal_stream_source(_stream, _open),
                    d_internal_stream_source(_stream, end));
    link.target_start = d_internal_stream_source(_stream, _parts->title_start);
    link.target_end   = d_internal_stream_source(_stream, _parts->title_end);

    // a label written, where it is
    if (_parts->label != D_WIKI_NONE)
    {
        link.label_start = d_internal_stream_source(_stream, _parts->label);
        link.label_end   = d_internal_stream_source(_stream,
                                                    _parts->label_end);
    }

    (void)d_internal_title_text(&target, &text[_parts->title_start],
                                _parts->title_end - _parts->title_start);
    link.target   = d_wiki_buffer_take(&target);
    link.to       = (link.target) ? d_internal_destination(link.target)
                                  : D_WIKI_TO_PAGE;
    link.embedded = (link.to == D_WIKI_TO_IMAGE);

    // the trail: the letters after `]]` that join a link that is shown
    while ( (link.to != D_WIKI_TO_IMAGE) &&
            (link.to != D_WIKI_TO_CATEGORY) &&
            (end < _stream->joined.length) &&
            (text[end] >= 'a') &&
            (text[end] <= 'z') )
    {
        end++;
    }

    link.end = d_internal_stream_source(_stream, end);

    // memory that ran out, or a page that cannot be named
    if ( (!link.target) ||
         (!d_internal_page_and_anchor(&link)) ||
         (!d_internal_valid_page(link.page)) )
    {
        _stream->failed = (_stream->failed) || (!link.target) ||
                          (!link.page) || (!link.anchor);
        free(link.target);
        free(link.label);
        free(link.page);
        free(link.anchor);

        return;
    }

    // a link to an anchor of this page
    if ( (link.to == D_WIKI_TO_PAGE) &&
         (link.page[0] == '\0') &&
         (strchr(link.target, '#') != NULL) )
    {
        link.to = D_WIKI_TO_ANCHOR;
    }

    const bool image = (link.to == D_WIKI_TO_IMAGE);

    // kept: what a link holds is no other link's; an image's is its own
    if (d_internal_keep(_stream, &link) != D_WIKI_NONE)
    {
        memset(&_stream->covered[_open],
               (image) ? D_INTERNAL_PICTURED : D_INTERNAL_LINKED,
               end - _open);

        // what an image holds
        if (image)
        {
            d_internal_image_kept(_stream, _parts);
        }
    }

    return;
}


/*
d_internal_internal_link
  One piece after a `[[` of a stream, to the next `[[` or the stream's end:
a link if it begins with a title and ends with ]], or an image whose
caption holds links, ending at the ]] after theirs.
*/
static void
d_internal_internal_link(
    struct d_internal_stream*      _stream,
    const struct d_internal_opens* _opens
)
{
    const size_t               open = _opens->at[_opens->index];
    const size_t               next = ((_opens->index + 1u) < _opens->count)
                                          ? _opens->at[_opens->index + 1u]
                                          : _stream->joined.length;
    struct d_internal_internal parts;

    // a title, and its close: here, or past an image's caption
    if ( (!d_internal_link_title(_stream, open, next, &parts)) ||
         ( (parts.image) &&
           (!d_internal_image_close(_stream, _opens, &parts)) ) )
    {
        return;
    }

    d_internal_internal_record(_stream, open, &parts);

    return;
}


/*
d_internal_may_begin
  Whether a link may begin over a byte a cover holds: in the page, where
nothing does; within an image, where it does alone -- its caption shown
or not; and, for a bare URL, in a caption shown besides.
*/
static bool
d_internal_may_begin(
    char _cover,
    bool _image,
    bool _bare
)
{
    return (_bare)
               ? ( (_cover == D_INTERNAL_FREE) ||
                   (_cover == D_INTERNAL_CAPTIONED) )
           : (_image)
               ? ( (_cover == D_INTERNAL_PICTURED) ||
                   (_cover == D_INTERNAL_CAPTIONED) )
               : (_cover == D_INTERNAL_FREE);
}


/*
d_internal_url_end
  Where a URL whose protocol ends at `_from` ends in a stream: an address
first -- IPv6's in brackets -- then the characters a URL may hold, to two
apostrophes or more, emphasis by then, but for those left over; a magic
word; a reference that ends one; or, for a bare URL, what covers it or a
template's boundary.
  Long as each of what ends a URL is tested in turn, as MediaWiki's
earlier passes have left the text.
*/
static size_t
d_internal_url_end(
    const struct d_internal_stream* _stream,
    size_t                          _from,
    bool                            _bare
)
{
    const char* const text   = _stream->joined.text;
    const size_t      length = _stream->joined.length;
    size_t            end    = _from;

    // an IPv6 address, in brackets
    if ( (end < length) &&
         (text[end] == '[') )
    {
        size_t close = end + 1u;

        // its hex digits, `:`s and `.`s
        while ( (close < length) &&
                ( (d_internal_is_hex(text[close])) ||
                  (text[close] == ':') || (text[close] == '.') ) )
        {
            close++;
        }

        end = ( (close > (end + 1u)) &&
                (close < length) &&
                (text[close] == ']') ) ? (close + 1u) : end;
    }

    // each character a URL may hold, to what ends it
    while (end < length)
    {
        const size_t width  = d_internal_url_char(&text[end]);
        const size_t source = d_internal_stream_source(_stream, end);
        size_t       run    = 0;

        // what no URL holds; a reference or a magic word that ends one; for
        // a bare URL what covers it, or a template's boundary
        if ( (width == 0u) ||
             ( (text[end] == '&') &&
               (d_internal_url_stop(&text[end], _bare)) ) ||
             ( (text[end] == '_') &&
               (d_internal_double_word(text, end, length) > 0u) ) ||
             ( (_bare) &&
               ( (!d_internal_may_begin(_stream->covered[end], false,
                                        true)) ||
                 (d_internal_crosses(_stream->mw, source, source + 1u)) ) ) )
        {
            break;
        }

        // a run of apostrophes
        while ( ((end + run) < length) &&
                (text[end + run] == '\'') )
        {
            run++;
        }

        // two or more are emphasis, but for those left over
        if (run >= 2u)
        {
            end += (run == 4u) ? 1u : (run > 5u) ? (run - 5u) : 0u;

            break;
        }

        end += width;
    }

    return end;
}


/*
d_internal_external_link
  Keeps [url label] from `_at` to the `]` at `_close` of a stream, its URL
to `_url` and its label from `_label`; what it covers is not read again.
*/
static void
d_internal_external_link(
    struct d_internal_stream* _stream,
    size_t                    _at,
    size_t                    _url,
    size_t                    _label,
    size_t                    _close
)
{
    const char* const    text   = _stream->joined.text;
    const size_t         scheme = d_internal_protocol(&text[_at + 1u],
                                                      _url - _at - 1u, true);
    struct d_wiki_link   link;
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };

    d_wiki_new_link(&link, D_WIKI_FORM_EXTERNAL,
                    d_internal_stream_source(_stream, _at),
                    d_internal_stream_source(_stream, _close + 1u));
    link.target_start = d_internal_stream_source(_stream, _at + 1u);
    link.target_end   = d_internal_stream_source(_stream, _url);

    // a label written, where it is
    if (_close > _label)
    {
        link.label_start = d_internal_stream_source(_stream, _label);
        link.label_end   = d_internal_stream_source(_stream, _close);
    }

    (void)d_internal_unescape(&target, &text[_at + 1u], _url - _at - 1u);
    link.target = d_wiki_buffer_take(&target);
    link.page   = d_wiki_copy("", 0u);
    link.anchor = d_wiki_copy("", 0u);
    link.to     = (d_internal_starts_with(&text[_at + 1u], scheme,
                                          "mailto:"))
                      ? D_WIKI_TO_MAIL : D_WIKI_TO_URL;

    // memory that ran out
    if ( (!link.target) ||
         (!link.page) ||
         (!link.anchor) )
    {
        free(link.target);
        free(link.page);
        free(link.anchor);
        _stream->failed = true;

        return;
    }

    // the link kept: what it covers is not read again
    if (d_internal_keep(_stream, &link) != D_WIKI_NONE)
    {
        memset(&_stream->covered[_at], D_INTERNAL_LINKED, _close + 1u - _at);
    }

    return;
}


/*
d_internal_label_end
  Where an external link's label, from `_label`, ends in a stream: at its
`]`, a line ending or a control but a tab, what is not the link's own --
another link, or in the page an image -- passed whole.
*/
static size_t
d_internal_label_end(
    const struct d_internal_stream* _stream,
    size_t                          _label,
    size_t                          _to,
    bool                            _image
)
{
    const char* const text  = _stream->joined.text;
    size_t            close = _label;

    // to `]`, what is not its own taken whole
    while ( (close < _to) &&
            (text[close] != ']') &&
            ( (text[close] == '\t') ||
              ((unsigned char)text[close] >= 0x20u) ) )
    {
        // what is not its own, past it whole
        if (!d_internal_may_begin(_stream->covered[close], _image, false))
        {
            // past what it covers
            while ( (close < _to) &&
                    (!d_internal_may_begin(_stream->covered[close], _image,
                                           false)) )
            {
                close++;
            }

            continue;
        }

        close++;
    }

    return close;
}


/*
d_internal_external_links
  [url label] in a stream, `_from` to `_to`: in the page, where no link
is, what another holds taken whole; or within an image, `_image`, where
what it holds is its own.  An HTML tag the sanitizer keeps is passed
whole, and none crosses a template's boundary.
  Long as handleExternalLinks' expression is read a part at a time: the
`[`, the protocol, the URL, the space, the label and the `]`.
*/
static void
d_internal_external_links(
    struct d_internal_stream* _stream,
    size_t                    _from,
    size_t                    _to,
    bool                      _image
)
{
    const char* const text = _stream->joined.text;

    // each `[` followed by a protocol
    for (size_t at = _from; at < _to; ++at)
    {
        const size_t tag = (text[at] == '<')
                               ? d_internal_html_tag(text, at, _to) : 0u;

        // a tag, passed whole; a `[` that may begin one here
        if ( (tag > 0u) ||
             (text[at] != '[') ||
             (!d_internal_may_begin(_stream->covered[at], _image, false)) )
        {
            at += (tag > 0u) ? (tag - 1u) : 0u;

            continue;
        }

        const size_t scheme = d_internal_protocol(&text[at + 1u],
                                                  _to - at - 1u, true);
        const size_t url    = (scheme > 0u)
                                  ? d_internal_url_end(_stream,
                                                       at + 1u + scheme,
                                                       false)
                                  : at;

        // a protocol after it, and a URL past it
        if ( (scheme == 0u) ||
             (url <= (at + 1u + scheme)) ||
             (url > _to) )
        {
            continue;
        }

        size_t label = url;

        // past the space before the label, Unicode's among it
        while ( (label < _to) &&
                ( (text[label] == ' ') ||
                  (d_internal_zs_width(&text[label]) > 0u) ) )
        {
            label += (text[label] == ' ') ? 1u
                                          : d_internal_zs_width(&text[label]);
        }

        const size_t close = d_internal_label_end(_stream, label, _to,
                                                  _image);

        // a `]` that closes it, within the template it began in
        if ( (close < _to) &&
             (text[close] == ']') &&
             (!d_internal_crosses(_stream->mw,
                                  d_internal_stream_source(_stream, at),
                                  d_internal_stream_source(_stream,
                                                           close + 1u))) )
        {
            d_internal_external_link(_stream, at, url, label, close);
            at = close;
        }
    }

    return;
}


/*
d_internal_url_trail
  Where a bare URL ends, from `_at`, once what trails it to `_end` is left
off, as MediaWiki's makeFreeExternalLink leaves it: `,;\.:!?`, and `)` in
one without `(`; but a `;` that ends a reference stays.
*/
static size_t
d_internal_url_trail(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    const bool parens = (memchr(&_text[_at], '(', _end - _at) != NULL);
    size_t     end    = _end;

    // what trails it
    while ( (end > _at) &&
            ( (strchr(",;\\.:!?", _text[end - 1u]) != NULL) ||
              ( (!parens) && (_text[end - 1u] == ')') ) ) )
    {
        end--;
    }

    // the first of what trails, a `;` that ends a reference
    if ( (end < _end) &&
         (_text[end] == ';') &&
         (d_internal_ends_reference(_text, _at, end)) )
    {
        end++;
    }

    return end;
}


/*
d_internal_clean_url
  A URL, its references read, as MediaWiki's cleanUrl makes it, and a bare
one shows: a bracket, `<`, `>`, `"`, `|`, a control and space each `%` and
two hex digits.
*/
static char*
d_internal_clean_url(
    const char* _url
)
{
    static const char    hex[] = "0123456789ABCDEF";
    struct d_wiki_buffer out   = { NULL, 0u, 0u, false };

    // each byte, or its escape
    for (const char* at = _url; *at != '\0'; ++at)
    {
        const unsigned char c = (unsigned char)*at;

        // a byte a URL shows escaped
        if ( (c <= 0x20u) ||
             (c == 0x7Fu) ||
             (strchr("[]<>\"|", (char)c) != NULL) )
        {
            (void)d_wiki_buffer_add_char(&out, '%');
            (void)d_wiki_buffer_add_char(&out, hex[c >> 4]);
            (void)d_wiki_buffer_add_char(&out, hex[c & 0x0Fu]);

            continue;
        }

        (void)d_wiki_buffer_add_char(&out, (char)c);
    }

    (void)d_wiki_buffer_add(&out, "", 0u);

    return d_wiki_buffer_take(&out);
}


/*
d_internal_bare_link
  Keeps a bare URL from `_at` to `_end` of a stream, its protocol
`_scheme` bytes; it shows itself as MediaWiki's cleanUrl leaves it.
*/
static void
d_internal_bare_link(
    struct d_internal_stream* _stream,
    size_t                    _at,
    size_t                    _scheme,
    size_t                    _end
)
{
    const char* const    text   = _stream->joined.text;
    struct d_wiki_link   link;
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };

    d_wiki_new_link(&link, D_WIKI_FORM_BARE,
                    d_internal_stream_source(_stream, _at),
                    d_internal_stream_source(_stream, _end));
    link.target_start = link.start;
    link.target_end   = link.end;
    (void)d_internal_unescape(&target, &text[_at], _end - _at);
    link.target = d_wiki_buffer_take(&target);
    link.label  = (link.target) ? d_internal_clean_url(link.target) : NULL;
    link.page   = d_wiki_copy("", 0u);
    link.anchor = d_wiki_copy("", 0u);
    link.to     = (d_internal_starts_with(&text[_at], _scheme, "mailto:"))
                      ? D_WIKI_TO_MAIL : D_WIKI_TO_URL;

    // memory that ran out
    if ( (!link.target) ||
         (!link.label) ||
         (!link.page) ||
         (!link.anchor) )
    {
        free(link.target);
        free(link.label);
        free(link.page);
        free(link.anchor);
        _stream->failed = true;

        return;
    }

    (void)d_internal_keep(_stream, &link);

    return;
}


/*
d_internal_bare_links
  Bare URLs in a stream: a protocol after a byte not a word's, the URL to
what ends it and short of what trails it; outside links, but for a caption
shown, and the HTML tags the sanitizer keeps.
*/
static void
d_internal_bare_links(
    struct d_internal_stream* _stream
)
{
    const char* const text   = _stream->joined.text;
    const size_t      length = _stream->joined.length;

    // each place, past tags and what the links hold
    for (size_t at = 0; at < length; ++at)
    {
        const size_t tag = (text[at] == '<')
                               ? d_internal_html_tag(text, at, length) : 0u;

        // what a link covers already, or a tag, passed whole
        if ( (tag > 0u) ||
             (!d_internal_may_begin(_stream->covered[at], false, true)) )
        {
            at += (tag > 0u) ? (tag - 1u) : 0u;

            continue;
        }

        const size_t scheme = (d_internal_is_alpha(text[at]))
                                  ? d_internal_protocol(&text[at],
                                                        length - at, false)
                                  : 0u;

        // a protocol that begins no word
        if ( (scheme == 0u) ||
             ( (at > 0u) &&
               ( (d_internal_is_alnum(text[at - 1u])) ||
                 (text[at - 1u] == '_') ||
                 ((unsigned char)text[at - 1u] >= 0x80u) ) ) )
        {
            continue;
        }

        const size_t end = d_internal_url_trail(
                               text, at + scheme,
                               d_internal_url_end(_stream, at + scheme,
                                                  true));

        // a URL that is only its protocol is no link
        if (end == (at + scheme))
        {
            continue;
        }

        d_internal_bare_link(_stream, at, scheme, end);
        at = end - 1u;
    }

    return;
}


/*
d_internal_find_opens
  Each `[[` of a stream, as MediaWiki splits its text at them, left to
right; none in the attributes of a tag the sanitizer keeps.
*/
static void
d_internal_find_opens(
    struct d_internal_stream*  _stream,
    struct d_internal_offsets* _out
)
{
    const char* const text   = _stream->joined.text;
    const size_t      length = _stream->joined.length;

    // each `[[`, and each tag passed whole
    for (size_t at = 0; (at + 1u) < length; ++at)
    {
        const size_t tag = (text[at] == '<')
                               ? d_internal_html_tag(text, at, length) : 0u;

        // a tag the sanitizer keeps: what its attributes hold is no link
        if (tag > 0u)
        {
            at += tag - 1u;

            continue;
        }

        // each `[[`, where it opens
        if ( (text[at] == '[') &&
             (text[at + 1u] == '[') )
        {
            // the opening kept, or the stream fails
            if (!d_internal_grow((void**)&_out->at, &_out->capacity,
                                 _out->count, sizeof(size_t)))
            {
                _stream->failed = true;

                return;
            }

            _out->at[_out->count++] = at;
            at++;
        }
    }

    return;
}


/*
d_internal_stream_links
  Every link of a stream: [[...]] first, then [url label] -- in the page,
and within each image -- then bare.
*/
static void
d_internal_stream_links(
    struct d_internal_stream* _stream
)
{
    const size_t              length = _stream->joined.length;
    struct d_internal_offsets opens  = { NULL, 0u, 0u };

    d_internal_find_opens(_stream, &opens);

    // each piece
    for (size_t index = 0; ( (!_stream->failed) && (index < opens.count) );
         ++index)
    {
        const struct d_internal_opens piece = { opens.at, opens.count, index };

        d_internal_internal_link(_stream, &piece);
    }

    free(opens.at);

    // then external links, `[url label]`, in the page and within each
    // image, on its own
    if (!_stream->failed)
    {
        d_internal_external_links(_stream, 0u, length, false);
    }

    // within each image
    for (size_t index = 0; ( (!_stream->failed) &&
                             ((index + 1u) < _stream->image_count) );
         index += 2u)
    {
        d_internal_external_links(_stream, _stream->images[index],
                                  _stream->images[index + 1u], true);
    }

    // then bare URLs, where nothing covers them
    if (!_stream->failed)
    {
        d_internal_bare_links(_stream);
    }

    return;
}


static void
d_internal_plain(
    const struct d_internal_stream* _stream,
    size_t                          _from,
    size_t                          _to,
    struct d_wiki_buffer*           _out
)
{
    const struct d_internal_mw* const mw    = _stream->mw;
    const char* const                 text  = _stream->joined.text;
    size_t                            at    = _from;

    // each byte, markup read as what it shows
    while (at < _to)
    {
        const size_t source = d_internal_stream_source(_stream, at);

        // a template shows nothing here
        if ( (text[at] == '{') &&
             ((at + 1u) < _to) &&
             (text[at + 1u] == '{') )
        {
            size_t skip = D_WIKI_NONE;

            // a template shows nothing of itself: past its end
            for (size_t index = 0; index < mw->wiki->template_count; ++index)
            {
                // the template that starts here
                if (mw->wiki->templates[index].start == source)
                {
                    skip = mw->wiki->templates[index].end;

                    break;
                }
            }

            // past every byte it covers
            if (skip != D_WIKI_NONE)
            {
                // to its end
                while ( (at < _to) &&
                        (d_internal_stream_source(_stream, at) < skip) )
                {
                    at++;
                }

                continue;
            }
        }

        // a link within shows its label: [[...]], [url text], a bare URL
        if ( ( (text[at] == '[') ||
               (d_internal_is_alpha(text[at])) ) &&
             ((at + 1u) < _to) )
        {
            size_t found = D_WIKI_NONE;

            // a link shows its label
            for (size_t index = 0; index < mw->wiki->link_count; ++index)
            {
                // the link that starts here
                if (mw->wiki->links[index].start == source)
                {
                    found = index;

                    break;
                }
            }

            // its label, then past it
            if (found != D_WIKI_NONE)
            {
                const struct d_wiki_link* const link = &mw->wiki->links[found];

                // an image and a category show nothing
                if ( (link->to != D_WIKI_TO_IMAGE) &&
                     (link->to != D_WIKI_TO_CATEGORY) )
                {
                    (void)d_wiki_buffer_add_text(_out, link->label);
                }

                // past every byte it covers
                while ( (at < _to) &&
                        (d_internal_stream_source(_stream, at) < link->end) )
                {
                    at++;
                }

                continue;
            }
        }

        // two or more apostrophes are emphasis, five at most
        if (text[at] == '\'')
        {
            size_t run = 0;

            // a run of `'`
            while ( ((at + run) < _to) &&
                    (text[at + run] == '\'') )
            {
                run++;
            }

            // two or more mark bold or italic, but for what is left over
            if (run >= 2u)
            {
                const size_t kept = (run == 4u) ? 1u
                                  : (run > 5u) ? (run - 5u) : 0u;

                // the `'`s left over are text
                for (size_t count = 0; count < kept; ++count)
                {
                    (void)d_wiki_buffer_add_char(_out, '\'');
                }

                at += run;

                continue;
            }
        }

        const size_t tag  = (text[at] == '<')
                                ? d_internal_html_tag(text, at, _to) : 0u;
        const size_t word = (text[at] == '_')
                                ? d_internal_double_word(text, at, _to) : 0u;

        // an HTML tag the sanitizer keeps shows nothing, its content stays;
        // a magic word shows nothing
        if ( (tag > 0u) ||
             (word > 0u) )
        {
            at += (tag > 0u) ? tag : word;

            continue;
        }

        // an element's mark: nowiki's content is shown as it is
        if (text[at] == D_INTERNAL_MARK)
        {
            // what is set apart: <nowiki>'s content shows as itself
            for (size_t index = 0; index < mw->hidden_count; ++index)
            {
                const struct d_internal_hidden* const hidden =
                    &mw->hidden[index];

                // the element that starts here
                if ( (hidden->start == source) &&
                     (hidden->kind == D_INTERNAL_LITERAL) &&
                     (hidden->name_length == 6u) &&
                     (d_internal_starts_with(&mw->text[hidden->name], 6u,
                                             "nowiki")) )
                {
                    (void)d_internal_unescape(_out, &mw->text[hidden->content],
                                              hidden->content_end -
                                              hidden->content);

                    break;
                }
            }

            at++;

            continue;
        }

        // a reference is what it stands for
        if (text[at] == '&')
        {
            const size_t span = d_wiki_entity(&text[at], _to - at, _out);

            // a reference decoded, or `&` as itself
            if (span > 0u)
            {
                at += span;

                continue;
            }
        }

        (void)d_wiki_buffer_add_char(_out, text[at]);
        at++;
    }

    return;
}


//==============================================================================
// 7.  BLOCKS
//==============================================================================


/*
d_internal_line_end
  Where a line's text ends, its ending not among it.
*/
static size_t
d_internal_line_end(
    const struct d_wiki* _wiki,
    size_t               _line
)
{
    size_t end = ((_line + 1u) < _wiki->line_count)
                     ? _wiki->line_starts[_line + 1u] : _wiki->length;

    // the ending: \n, \r\n or \r
    if ( (end > _wiki->line_starts[_line]) &&
         (_wiki->text[end - 1u] == '\n') )
    {
        end--;
    }

    // and past its CR
    if ( (end > _wiki->line_starts[_line]) &&
         (_wiki->text[end - 1u] == '\r') )
    {
        end--;
    }

    return end;
}


/*
d_internal_line_after
  Where the line after a line begins: past its ending, or the text's end.
*/
static size_t
d_internal_line_after(
    const struct d_wiki* _wiki,
    size_t               _line
)
{
    return ((_line + 1u) < _wiki->line_count)
               ? _wiki->line_starts[_line + 1u] : _wiki->length;
}


/*
d_internal_compare_spans
  Orders spans, two offsets each, by where they start.
*/
static int
d_internal_compare_spans(
    const void* _left,
    const void* _right
)
{
    const size_t* const left  = _left;
    const size_t* const right = _right;

    return (left[0] < right[0]) ? -1 : (left[0] > right[0]) ? 1 : 0;
}


/*
d_internal_find_reach
  For each line, how far what began before its start -- a comment, an
element, a template -- reaches past it: the line's start for nothing.
*/
static void
d_internal_find_reach(
    struct d_internal_mw* _mw
)
{
    const struct d_wiki* const wiki  = _mw->wiki;
    const size_t               count = _mw->hidden_count +
                                       wiki->template_count;
    size_t* const              spans = malloc((count + 1u) * 2u *
                                              sizeof(size_t));

    _mw->reach = malloc((wiki->line_count + 1u) * sizeof(size_t));

    // both, or nothing
    if ( (!spans) ||
         (!_mw->reach) )
    {
        free(spans);
        _mw->failed = true;

        return;
    }

    // what is hidden, and the templates, by where they start
    for (size_t index = 0; index < _mw->hidden_count; ++index)
    {
        spans[2u * index]        = _mw->hidden[index].start;
        spans[(2u * index) + 1u] = _mw->hidden[index].end;
    }

    // each template's span, after what is set apart
    for (size_t index = 0; index < wiki->template_count; ++index)
    {
        const size_t at = _mw->hidden_count + index;

        spans[2u * at]        = wiki->templates[index].start;
        spans[(2u * at) + 1u] = wiki->templates[index].end;
    }

    qsort(spans, count, 2u * sizeof(size_t), d_internal_compare_spans);

    size_t next  = 0;
    size_t reach = 0;

    // each line, with the farthest end of the spans begun before it
    for (size_t line = 0; line < wiki->line_count; ++line)
    {
        const size_t start = wiki->line_starts[line];

        // the spans that begin before the line, the furthest end among them
        while ( (next < count) &&
                (spans[2u * next] < start) )
        {
            reach = (spans[(2u * next) + 1u] > reach)
                        ? spans[(2u * next) + 1u] : reach;
            next++;
        }

        _mw->reach[line] = (reach > start) ? reach : start;
    }

    free(spans);

    return;
}


/*
d_internal_first_non_space
  The first byte of a line that is not a space or tab.
*/
static size_t
d_internal_first_non_space(
    const struct d_wiki* _wiki,
    size_t               _start,
    size_t               _end
)
{
    size_t at = _start;

    // past space and tabs
    while ( (at < _end) &&
            (d_internal_is_space_or_tab(_wiki->text[at])) )
    {
        at++;
    }

    return at;
}


/*
d_internal_hidden_at
  The first record of what is hidden that starts at `_at`; NULL for none.
The records are in the order of their starts.
*/
static const struct d_internal_hidden*
d_internal_hidden_at(
    const struct d_internal_mw* _mw,
    size_t                      _at
)
{
    size_t low  = 0;
    size_t high = _mw->hidden_count;

    // the first that starts at or after the place
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // those after the middle, or those to it
        if (_mw->hidden[middle].start < _at)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < _mw->hidden_count) &&
             (_mw->hidden[low].start == _at) ) ? &_mw->hidden[low] : NULL;
}


/*
d_internal_is_comment_line
  Whether a line holds a comment and space alone, and where the comment's
last line ends.
*/
static bool
d_internal_is_comment_line(
    const struct d_internal_mw* _mw,
    size_t                      _start,
    size_t                      _end,
    size_t*                     _out_end
)
{
    const size_t                          at     = d_internal_first_non_space(
                                                       _mw->wiki, _start,
                                                       _end);
    const struct d_internal_hidden* const hidden = d_internal_hidden_at(_mw,
                                                                        at);

    // a comment that begins the line
    if ( (hidden) &&
         (hidden->kind == D_INTERNAL_COMMENT) )
    {
        const size_t line = d_wiki_line_of(_mw->wiki, (hidden->end > 0u)
                                                           ? (hidden->end - 1u)
                                                           : 0u, NULL) - 1u;
        const size_t stop = d_internal_line_end(_mw->wiki, line);
        size_t       rest = hidden->end;

        // nothing after it but space
        while ( (rest < stop) &&
                (d_internal_is_space_or_tab(_mw->text[rest])) )
        {
            rest++;
        }

        *_out_end = line;

        return (rest >= stop);
    }

    return false;
}


/*
d_internal_literal_at
  The literal element that begins at `_at`, if it is <pre>,
<syntaxhighlight> or <source>; NULL for none.
*/
static const struct d_internal_hidden*
d_internal_literal_at(
    const struct d_internal_mw* _mw,
    size_t                      _at
)
{
    const struct d_internal_hidden* const hidden = d_internal_hidden_at(_mw,
                                                                        _at);

    // an element hidden that begins there
    if (hidden)
    {
        const char* const name = &_mw->text[hidden->name];

        // a block of code's element
        if ( (hidden->kind == D_INTERNAL_LITERAL) &&
             ( ( (hidden->name_length == 3u) &&
                 (d_internal_starts_with(name, 3u, "pre")) ) ||
               ( (hidden->name_length == 15u) &&
                 (d_internal_starts_with(name, 15u, "syntaxhighlight")) ) ||
               ( (hidden->name_length == 6u) &&
                 (d_internal_starts_with(name, 6u, "source")) ) ) )
        {
            return hidden;
        }
    }

    return NULL;
}


/*
d_internal_language
  The lang="..." of a code element, kept as a block's info.
*/
static void
d_internal_language(
    const struct d_internal_mw*     _mw,
    const struct d_internal_hidden* _hidden,
    struct d_wiki_block*            _block
)
{
    size_t       end   = 0;
    const size_t start = d_internal_attribute(_mw->text, _hidden->attributes,
                                              _hidden->attributes_end,
                                              "lang", &end);

    // the language, written
    if ( (start != D_WIKI_NONE) &&
         (end > start) )
    {
        _block->info_start = start;
        _block->info_end   = end;
    }

    return;
}


/*
d_internal_add_lines
  Adds lines `_first` to `_last` to the last block's content, from
`_skip` bytes into each.
*/
static void
d_internal_add_lines(
    struct d_internal_mw* _mw,
    size_t                _first,
    size_t                _last,
    size_t                _skip
)
{
    // each line
    for (size_t line = _first; line <= _last; ++line)
    {
        const size_t start = _mw->wiki->line_starts[line];
        const size_t end   = d_internal_line_end(_mw->wiki, line);
        const size_t from  = ((start + _skip) < end) ? (start + _skip) : end;

        // the line's content, or the reader fails
        if (!d_wiki_add_segment(_mw->wiki, from, end, 0u))
        {
            _mw->failed = true;

            return;
        }
    }

    return;
}


/*
d_internal_code_lines
  A code block's content: its element's, a line at a time, without the
line endings after its open tag and before its close.
  Long as the line endings either side of the content are passed one
at a time, CR and LF.
*/
static void
d_internal_code_lines(
    struct d_internal_mw*           _mw,
    const struct d_internal_hidden* _code,
    size_t                          _block
)
{
    struct d_wiki* const wiki  = _mw->wiki;
    size_t               start = _code->content;
    size_t               end   = _code->content_end;

    // the line ending after the open tag, and before the close
    if ( (start < end) &&
         (_mw->text[start] == '\r') )
    {
        start++;
    }

    // the line ending after the opening tag
    if ( (start < end) &&
         (_mw->text[start] == '\n') )
    {
        start++;
    }

    // the line ending before the closing tag
    if ( (end > start) &&
         (_mw->text[end - 1u] == '\n') )
    {
        end--;
    }

    // and its CR
    if ( (end > start) &&
         (_mw->text[end - 1u] == '\r') )
    {
        end--;
    }

    wiki->segment_count                 = wiki->blocks[_block].first_segment;
    wiki->blocks[_block].segment_count  = 0u;

    // each line of it
    for (size_t at = start; ; )
    {
        size_t stop = at;

        // to the line's end
        while ( (stop < end) &&
                (_mw->text[stop] != '\n') &&
                (_mw->text[stop] != '\r') )
        {
            stop++;
        }

        // the line, or the reader fails
        if (!d_wiki_add_segment(wiki, at, stop, 0u))
        {
            _mw->failed = true;

            return;
        }

        // the last line
        if (stop >= end)
        {
            break;
        }

        at = stop + ( ( (_mw->text[stop] == '\r') &&
                        ((stop + 1u) < end) &&
                        (_mw->text[stop + 1u] == '\n') ) ? 2u : 1u );
    }

    return;
}


/*
d_internal_heading_at
  The heading the preprocessor found at a line's start; NULL for none.
*/
static struct d_internal_found_heading*
d_internal_heading_at(
    struct d_internal_mw* _mw,
    size_t                _at
)
{
    size_t low  = 0;
    size_t high = _mw->heading_count;

    // the first that starts at or after the place, as they are in order
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // those after the middle, or those to it
        if (_mw->headings[middle].start < _at)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < _mw->heading_count) &&
             (_mw->headings[low].start == _at) ) ? &_mw->headings[low] : NULL;
}


/*
d_internal_table_end
  The line a table that begins on `_line` ends on: its |}, tables within
it counted; the last line for one not closed.
*/
static size_t
d_internal_table_end(
    struct d_internal_mw* _mw,
    size_t                _line
)
{
    const struct d_wiki* const wiki  = _mw->wiki;
    size_t                     depth = 1u;

    // each line after it not covered by what began before it
    for (size_t line = _line + 1u; line < wiki->line_count; ++line)
    {
        const size_t start = wiki->line_starts[line];
        const size_t end   = d_internal_line_end(wiki, line);

        // a line within what began before it
        if (_mw->reach[line] > start)
        {
            continue;
        }

        size_t at = d_internal_first_non_space(wiki, start, end);

        // past the `:`s that indent a table
        while ( (at < end) &&
                (wiki->text[at] == ':') )
        {
            at++;
        }

        // a table within it
        if ( ((at + 1u) < end + 1u) &&
             ((at + 2u) <= end) &&
             (strncmp(&wiki->text[at], "{|", 2u) == 0) )
        {
            depth++;
        }
        else if ( ((at + 2u) <= end) &&
                  (strncmp(&wiki->text[at], "|}", 2u) == 0) )
        {
            depth--;

            // the table's own close
            if (depth == 0u)
            {
                return line;
            }
        }
    }

    // a table never closed is a problem
    if (!d_wiki_add_problem(_mw->wiki, wiki->line_starts[_line],
                            "a table is not closed"))
    {
        _mw->failed = true;
    }

    return (wiki->line_count > 0u) ? (wiki->line_count - 1u) : 0u;
}


/*
d_internal_begin_block
  A block of a kind on lines `_first` to `_last`, its content those lines.
*/
static size_t
d_internal_begin_block(
    struct d_internal_mw*  _mw,
    enum d_wiki_block_kind _kind,
    size_t                 _start,
    size_t                 _first,
    size_t                 _last,
    size_t                 _skip
)
{
    const size_t block = d_wiki_add_block(_mw->wiki, _kind, _start);

    // a block that cannot be kept
    if (block == D_WIKI_NONE)
    {
        _mw->failed = true;

        return D_WIKI_NONE;
    }

    d_internal_add_lines(_mw, _first, _last, _skip);
    _mw->wiki->blocks[block].end = d_internal_line_after(_mw->wiki, _last);

    // the first line's content from where the block begins on it
    if (_mw->wiki->blocks[block].segment_count > 0u)
    {
        struct d_wiki_segment* const first =
            &_mw->wiki->segments[_mw->wiki->blocks[block].first_segment];

        // the first line's content from where the block begins on it
        if (first->start < _start)
        {
            first->start = (_start < first->end) ? _start : first->end;
        }
    }

    return block;
}


/*
d_internal_add_heading
  The heading of a block, from what the preprocessor found.
*/
static void
d_internal_add_heading(
    struct d_internal_mw*                  _mw,
    const struct d_internal_found_heading* _found,
    size_t                                 _block,
    bool                                   _top
)
{
    struct d_wiki_heading heading;

    memset(&heading, 0, sizeof(heading));
    heading.block       = _block;
    heading.level       = _found->level;
    heading.start       = _found->start;
    heading.end         = d_internal_line_after(_mw->wiki,
                              d_wiki_line_of(_mw->wiki, _found->end, NULL) -
                              1u);
    heading.title_start = _found->title_start;
    heading.title_end   = _found->title_end;
    heading.top         = _top;

    // the heading kept, or the reader fails
    if (d_wiki_add_heading(_mw->wiki, &heading) == D_WIKI_NONE)
    {
        _mw->failed = true;
    }

    return;
}


/*
d_internal_classify_line
  The kind of block a line begins, and the line it ends on.
  Long as a dispatch on what a line begins with is.
*/
static enum d_wiki_block_kind
d_internal_classify_line(
    struct d_internal_mw* _mw,
    size_t                _line,
    size_t*               _out_last
)
{
    const struct d_wiki* const wiki  = _mw->wiki;
    const size_t               start = wiki->line_starts[_line];
    const size_t               end   = d_internal_line_end(wiki, _line);
    const char* const          text  = wiki->text;
    size_t                     at    = d_internal_first_non_space(wiki, start,
                                                                  end);
    size_t                     last  = _line;

    *_out_last = _line;

    // a table, after any `:`s
    while ( (at < end) &&
            (text[at] == ':') )
    {
        at++;
    }

    // a table, to its close
    if ( ((at + 2u) <= end) &&
         (strncmp(&text[at], "{|", 2u) == 0) )
    {
        *_out_last = d_internal_table_end(_mw, _line);

        return D_WIKI_TABLE;
    }

    // a comment alone
    if (d_internal_is_comment_line(_mw, start, end, &last))
    {
        *_out_last = last;

        return D_WIKI_COMMENT;
    }

    // code, in an element that begins the line
    const struct d_internal_hidden* const code =
        d_internal_literal_at(_mw, d_internal_first_non_space(wiki, start,
                                                              end));

    // a block of code's element, to its close
    if (code)
    {
        *_out_last = d_wiki_line_of(wiki, (code->end > 0u) ? (code->end - 1u)
                                                           : 0u, NULL) - 1u;

        return D_WIKI_CODE;
    }

    // a rule, an item, a line that begins with a space
    if ( ((start + 4u) <= end) &&
         (strncmp(&text[start], "----", 4u) == 0) )
    {
        return D_WIKI_RULE;
    }

    // a list's item
    if ( (start < end) &&
         (strchr("*#:;", text[start]) != NULL) )
    {
        return D_WIKI_ITEM;
    }

    // a line that begins with a space
    if ( (start < end) &&
         (text[start] == ' ') )
    {
        return D_WIKI_PREFORMATTED;
    }

    return D_WIKI_PARAGRAPH;
}


/*
d_internal_read_blocks
  The page's blocks, a line at a time: a line within what began on an
earlier line -- a comment, an element, a template -- goes with it.
  Long as a dispatch on what each line begins is.
*/
static void
d_internal_read_blocks(
    struct d_internal_mw* _mw
)
{
    struct d_wiki* const wiki    = _mw->wiki;
    size_t               current = D_WIKI_NONE;
    size_t               line    = 0;

    // each line
    while ( (!_mw->failed) &&
            (line < wiki->line_count) )
    {
        const size_t start = wiki->line_starts[line];
        const size_t end   = d_internal_line_end(wiki, line);
        const size_t reach = _mw->reach[line];

        // the last empty line, after a line ending
        if ( ((line + 1u) == wiki->line_count) &&
             (start == wiki->length) &&
             (line > 0u) )
        {
            break;
        }

        // within what began before: the block it began in goes on
        if (reach > start)
        {
            const size_t last = d_wiki_line_of(wiki, reach - 1u, NULL) - 1u;

            // the block open goes on over those lines
            if ( (current != D_WIKI_NONE) &&
                 ((current + 1u) == wiki->block_count) )
            {
                d_internal_add_lines(_mw, line, last, 0u);
                wiki->blocks[current].end = d_internal_line_after(wiki, last);
            }

            line = last + 1u;

            continue;
        }

        // a blank line ends a paragraph
        if (d_internal_first_non_space(wiki, start, end) >= end)
        {
            current = D_WIKI_NONE;
            line++;

            continue;
        }

        struct d_internal_found_heading* const found =
            d_internal_heading_at(_mw, start);

        // a heading the preprocessor found here
        if ( (found) &&
             (found->top) )
        {
            const size_t last  = d_wiki_line_of(wiki, (found->end > start)
                                                          ? (found->end - 1u)
                                                          : start, NULL) - 1u;
            const size_t block = d_internal_begin_block(_mw, D_WIKI_HEADING,
                                                        start, line, last, 0u);

            // its level, and the heading
            if (block != D_WIKI_NONE)
            {
                wiki->blocks[block].level = found->level;
                d_internal_add_heading(_mw, found, block, true);
            }

            found->kept = true;
            current     = D_WIKI_NONE;
            line        = last + 1u;

            continue;
        }

        size_t                       last = line;
        const enum d_wiki_block_kind kind = d_internal_classify_line(_mw, line,
                                                                     &last);

        // a paragraph, or lines that begin with a space, go on together
        if ( ( (kind == D_WIKI_PARAGRAPH) || (kind == D_WIKI_PREFORMATTED) ) &&
             (current != D_WIKI_NONE) &&
             ((current + 1u) == wiki->block_count) &&
             (wiki->blocks[current].kind == kind) )
        {
            d_internal_add_lines(_mw, line, line,
                                 (kind == D_WIKI_PREFORMATTED) ? 1u : 0u);
            wiki->blocks[current].end = d_internal_line_after(wiki, line);
            line++;

            continue;
        }

        // a block begins at its line, but for what may stand after space
        const size_t block = d_internal_begin_block(
                                 _mw, kind,
                                 ( (kind == D_WIKI_PARAGRAPH) ||
                                   (kind == D_WIKI_PREFORMATTED) )
                                     ? start
                                     : d_internal_first_non_space(wiki, start,
                                                                  end),
                                 line, last,
                                 (kind == D_WIKI_PREFORMATTED) ? 1u : 0u);

        // memory that ran out
        if (block == D_WIKI_NONE)
        {
            return;
        }

        // an item's depth and marker; code's language
        if (kind == D_WIKI_ITEM)
        {
            size_t prefix = start;

            // the markers before the item's text
            while ( (prefix < end) &&
                    (strchr("*#:;", wiki->text[prefix]) != NULL) )
            {
                prefix++;
            }

            wiki->blocks[block].level  = prefix - start;
            wiki->blocks[block].marker = wiki->text[prefix - 1u];
        }
        else if (kind == D_WIKI_CODE)
        {
            const struct d_internal_hidden* const code =
                d_internal_literal_at(_mw, wiki->blocks[block].start);

            d_internal_language(_mw, code, &wiki->blocks[block]);
            d_internal_code_lines(_mw, code, block);
        }

        current = ( (kind == D_WIKI_PARAGRAPH) ||
                    (kind == D_WIKI_PREFORMATTED) ) ? block : D_WIKI_NONE;
        line    = last + 1u;
    }

    return;
}


//==============================================================================
// 8.  READING
//==============================================================================


/*
d_internal_cell_attributes
  A cell's attributes, from `_cell` to `_end`: its text before its first
`|` that is no template's, when no `[[` is before that, kept as a span
that holds no link, and the `|` a boundary in `_out`.
*/
static void
d_internal_cell_attributes(
    struct d_internal_mw*      _mw,
    size_t                     _cell,
    size_t                     _end,
    struct d_internal_offsets* _out
)
{
    const char* const text  = _mw->text;
    size_t            index = d_internal_first_hidden(_mw, _cell);
    size_t            at    = _cell;

    // each byte, past what is hidden, to the first `|` or `[[`
    while (at < _end)
    {
        // what is hidden here is passed
        if ( (index < _mw->hidden_count) &&
             (_mw->hidden[index].start == at) )
        {
            at = _mw->hidden[index].end;

            // and what is hidden within it
            while ( (index < _mw->hidden_count) &&
                    (_mw->hidden[index].start < at) )
            {
                index++;
            }

            continue;
        }

        // `[[` first: the cell is all text
        if ( ((at + 1u) < _end) &&
             (text[at] == '[') &&
             (text[at + 1u] == '[') )
        {
            return;
        }

        // the first `|` no template's: the attributes end
        if ( (text[at] == '|') &&
             (!d_internal_crosses(_mw, at, at + 1u)) )
        {
            // room for the span and the boundary
            if ( (!d_internal_grow((void**)&_mw->shut, &_mw->shut_capacity,
                                   _mw->shut_count + 1u, sizeof(size_t))) ||
                 (!d_internal_grow((void**)&_out->at, &_out->capacity,
                                   _out->count, sizeof(size_t))) )
            {
                _mw->failed = true;

                return;
            }

            _mw->shut[_mw->shut_count++] = _cell;
            _mw->shut[_mw->shut_count++] = at;
            _out->at[_out->count++]      = at;

            return;
        }

        at++;
    }

    return;
}


/*
d_internal_row_cells
  A table's line of cells, as MediaWiki's handleTables splits one: after
its `|` or `!`, at `||` -- and `!!` in a row of headers -- that is no
template's, each a boundary in `_out`; then each cell's attributes.
*/
static void
d_internal_row_cells(
    struct d_internal_mw*      _mw,
    size_t                     _line,
    struct d_internal_offsets* _out
)
{
    const char* const text  = _mw->text;
    const size_t      end   = d_internal_line_end(_mw->wiki, _line);
    const size_t      start = d_internal_first_non_space(
                                  _mw->wiki, _mw->wiki->line_starts[_line],
                                  end);
    const char        first = (start < end) ? text[start] : '\0';
    size_t            cell  = start + 1u;

    // a line of cells begins with `|` or `!`, but `|}`, `|-` and `|+`
    if ( ( (first != '|') && (first != '!') ) ||
         ( (first == '|') &&
           ((start + 1u) < end) &&
           (strchr("}-+", text[start + 1u]) != NULL) ) )
    {
        return;
    }

    // each `||`, or `!!` among headers, that is no template's
    for (size_t at = start + 1u; ( (!_mw->failed) && (at <= end) ); ++at)
    {
        const bool split = ((at + 1u) < end) &&
                           ( ( (text[at] == '|') &&
                               (text[at + 1u] == '|') ) ||
                             ( (first == '!') &&
                               (text[at] == '!') &&
                               (text[at + 1u] == '!') ) ) &&
                           (!d_internal_crosses(_mw, at, at + 2u));

        // a cell ends at the line's end, or where it splits
        if ( (at < end) &&
             (!split) )
        {
            continue;
        }

        d_internal_cell_attributes(_mw, cell, at, _out);

        // the split, a boundary, and the next cell after it
        if ( (split) &&
             (d_internal_grow((void**)&_out->at, &_out->capacity,
                              _out->count + 1u, sizeof(size_t))) )
        {
            _out->at[_out->count++] = at;
            _out->at[_out->count++] = at + 1u;
            at++;
            cell = at + 1u;
        }
        else if (split)
        {
            _mw->failed = true;
        }
    }

    return;
}


/*
d_internal_table_cells
  The cells of each table's rows, as MediaWiki splits them before it finds
links: the splits are boundaries no URL crosses, and the attributes hold
no link.  A line within what began on one before it is no row's.
*/
static void
d_internal_table_cells(
    struct d_internal_mw* _mw
)
{
    const struct d_wiki* const wiki  = _mw->wiki;
    struct d_internal_offsets  found = { NULL, 0u, 0u };

    // each table
    for (size_t index = 0; ( (!_mw->failed) && (index < wiki->block_count) );
         ++index)
    {
        const struct d_wiki_block* const block = &wiki->blocks[index];

        // a block of another kind
        if ( (block->kind != D_WIKI_TABLE) ||
             (block->end <= block->start) )
        {
            continue;
        }

        const size_t first = d_wiki_line_of(wiki, block->start, NULL) - 1u;
        const size_t last  = d_wiki_line_of(wiki, block->end - 1u, NULL) - 1u;

        // each of its lines not within what began before it
        for (size_t line = first + 1u; line <= last; ++line)
        {
            // a line its own
            if (_mw->reach[line] <= wiki->line_starts[line])
            {
                d_internal_row_cells(_mw, line, &found);
            }
        }
    }

    // the splits, among the boundaries, in order
    for (size_t index = 0; index < found.count; ++index)
    {
        d_internal_add_boundary(_mw, found.at[index]);
    }

    free(found.at);

    // the boundaries in order again
    if (_mw->boundary_count > 1u)
    {
        qsort(_mw->boundaries, _mw->boundary_count, sizeof(size_t),
              d_internal_compare_offsets);
    }

    return;
}


/*
d_internal_gallery_file
  The file a line of a <gallery> names, `_start` to `_end`, as MediaWiki's
renderImageGallery reads it -- percent-decoded when the line holds `%`,
in File: when it names no namespace -- kept as a link to an image, its
caption after `|` to `_line_end`; none for a page no title can name or a
page not a file.
  Long as renderImageGallery's reading of a name is followed: its
namespace, its decoding, and its title's validity.
*/
static void
d_internal_gallery_file(
    struct d_internal_mw* _mw,
    size_t                _start,
    size_t                _end,
    size_t                _line_end
)
{
    struct d_wiki_buffer target = { NULL, 0u, 0u, false };
    struct d_wiki_link   link;
    const bool           percent = (memchr(&_mw->text[_start], '%',
                                           _line_end - _start) != NULL);
    const int            space   = d_internal_namespace(&_mw->text[_start],
                                                        _end - _start);

    // a page of another namespace than File's is no file
    if ( (space == 2) ||
         (space == 3) )
    {
        return;
    }

    d_wiki_new_link(&link, D_WIKI_FORM_GALLERY, _start, _line_end);
    link.close        = _line_end;
    link.target_start = _start;
    link.target_end   = _end;
    link.to           = D_WIKI_TO_IMAGE;
    link.embedded     = true;

    // a caption, after `|`
    if (_end < _line_end)
    {
        link.label_start = _end + 1u;
        link.label_end   = _line_end;
    }

    // File: before a name that has no namespace
    if (space == 0)
    {
        (void)d_wiki_buffer_add_text(&target, "File:");
    }

    (void)((percent)
               ? d_internal_title_text(&target, &_mw->text[_start],
                                       _end - _start)
               : d_internal_unescape(&target, &_mw->text[_start],
                                     _end - _start));
    link.target = d_wiki_buffer_take(&target);

    // memory that ran out, or a page that cannot be named
    if ( (!link.target) ||
         (!d_internal_page_and_anchor(&link)) ||
         (!d_internal_valid_page(link.page)) )
    {
        _mw->failed = (_mw->failed) || (!link.target) || (!link.page) ||
                      (!link.anchor);
        free(link.target);
        free(link.page);
        free(link.anchor);

        return;
    }

    // the link kept, or the reader fails
    if (d_wiki_add_link(_mw->wiki, &link) == D_WIKI_NONE)
    {
        free(link.target);
        free(link.page);
        free(link.anchor);
        _mw->failed = true;
    }

    return;
}


/*
d_internal_gallery_line
  One line of a <gallery>, `_start` to `_end`: the file it names, to its
first `|`, and what follows, wikitext read on its own, as MediaWiki reads
it -- its caption, alt= and link= among it.
*/
static void
d_internal_gallery_line(
    struct d_internal_mw* _mw,
    size_t                _start,
    size_t                _end
)
{
    const char* const        bar   = memchr(&_mw->text[_start], '|',
                                            _end - _start);
    const size_t             split = (bar) ? (size_t)(bar - _mw->text) : _end;
    size_t                   name  = _start;
    size_t                   stop  = split;
    struct d_internal_stream stream;
    struct d_internal_image  image;

    d_internal_trimmed(_mw->text, &name, &stop);

    // a line that names nothing
    if (name == stop)
    {
        return;
    }

    d_internal_gallery_file(_mw, name, stop, _end);

    // nothing after the name
    if ( (_mw->failed) ||
         (split >= _end) )
    {
        return;
    }

    memset(&stream, 0, sizeof(stream));
    stream.mw = _mw;
    d_internal_build_stream(&stream, split + 1u, _end,
                            d_internal_first_hidden(_mw, split + 1u));

    // what follows the name, read on its own; then its link=
    if (!stream.failed)
    {
        d_internal_stream_links(&stream);
        d_internal_image_parts(&stream, 0u, stream.joined.length, &image);
    }

    // its link=, a link of its own
    if ( (!stream.failed) &&
         (image.link != D_WIKI_NONE) )
    {
        d_internal_option_link(&stream, image.link, image.link_end);
    }

    _mw->failed = (_mw->failed) || (stream.failed);
    d_internal_free_stream(&stream);

    return;
}


/*
d_internal_gallery
  A <gallery>'s caption, and its lines, each read on its own, as
MediaWiki's gallery reads them.
*/
static void
d_internal_gallery(
    struct d_internal_mw*           _mw,
    const struct d_internal_hidden* _gallery
)
{
    size_t                   at          = _gallery->content;
    size_t                   caption_end = 0;
    const size_t             caption     = d_internal_attribute(
                                               _mw->text,
                                               _gallery->attributes,
                                               _gallery->attributes_end,
                                               "caption", &caption_end);
    struct d_internal_stream stream;

    // its caption, wikitext read on its own
    if (caption != D_WIKI_NONE)
    {
        memset(&stream, 0, sizeof(stream));
        stream.mw = _mw;
        d_internal_build_stream(&stream, caption, caption_end,
                                d_internal_first_hidden(_mw, caption));

        // the caption's links
        if (!stream.failed)
        {
            d_internal_stream_links(&stream);
        }

        _mw->failed = (_mw->failed) || (stream.failed);
        d_internal_free_stream(&stream);
    }

    // each line of its content
    while ( (!_mw->failed) &&
            (at < _gallery->content_end) )
    {
        const char* const ending = memchr(&_mw->text[at], '\n',
                                          _gallery->content_end - at);
        size_t            end    = (ending) ? (size_t)(ending - _mw->text)
                                            : _gallery->content_end;
        const size_t      next   = end + 1u;

        // the line's CR
        if ( (end > at) &&
             (_mw->text[end - 1u] == '\r') )
        {
            end--;
        }

        d_internal_gallery_line(_mw, at, end);
        at = next;
    }

    return;
}


/*
d_internal_read_streams
  The links of the page's stream, then of each element's content and of
what each <includeonly> holds, each a stream of its own; and each line of
a <gallery>.
*/
static void
d_internal_read_streams(
    struct d_internal_mw* _mw
)
{
    struct d_internal_stream stream;

    memset(&stream, 0, sizeof(stream));
    stream.mw = _mw;
    d_internal_build_stream(&stream, 0u, _mw->length, 0u);

    // the page's own stream
    if (!stream.failed)
    {
        d_internal_stream_links(&stream);
    }

    _mw->failed = (_mw->failed) || (stream.failed);
    d_internal_free_stream(&stream);

    // each element's content, and what <includeonly> holds
    for (size_t index = 0; ( (!_mw->failed) && (index < _mw->hidden_count) );
         ++index)
    {
        const struct d_internal_hidden* const hidden = &_mw->hidden[index];

        // a gallery's lines
        if ( (hidden->kind == D_INTERNAL_LITERAL) &&
             (hidden->name_length == 7u) &&
             (d_internal_starts_with(&_mw->text[hidden->name], 7u,
                                     "gallery")) )
        {
            d_internal_gallery(_mw, hidden);

            continue;
        }

        // an element whose content is wikitext
        if ( (hidden->kind != D_INTERNAL_ELEMENT) &&
             (hidden->kind != D_INTERNAL_INCLUDED) )
        {
            continue;
        }

        memset(&stream, 0, sizeof(stream));
        stream.mw = _mw;
        d_internal_build_stream(&stream, _mw->hidden[index].content,
                                _mw->hidden[index].content_end, index + 1u);

        // its content's links, a stream of its own
        if (!stream.failed)
        {
            d_internal_stream_links(&stream);
        }

        _mw->failed = (_mw->failed) || (stream.failed);
        d_internal_free_stream(&stream);
    }

    return;
}


/*
d_internal_plain_span
  Writes what the page shows from `_start` to `_end`, read as a stream of
its own; an image's caption, when `_image` says it is one.
*/
static bool
d_internal_plain_span(
    struct d_internal_mw* _mw,
    size_t                _start,
    size_t                _end,
    bool                  _image,
    struct d_wiki_buffer* _out
)
{
    struct d_internal_stream stream;
    size_t                   first = 0;
    size_t                   high  = _mw->hidden_count;

    // what is hidden from the start on
    while (first < high)
    {
        const size_t middle = first + ((high - first) / 2u);

        // those after the middle, or those to it
        if (_mw->hidden[middle].start < _start)
        {
            first = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    memset(&stream, 0, sizeof(stream));
    stream.mw = _mw;
    d_internal_build_stream(&stream, _start, _end, first);

    // the caption's parts, or the text
    if (!stream.failed)
    {
        // an image's caption, or the text
        if (_image)
        {
            d_internal_image_label(&stream, 0u, stream.joined.length, _out);
        }
        else
        {
            d_internal_plain(&stream, 0u, stream.joined.length, _out);
        }
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    const bool ok = (!stream.failed) && (!_out->failed);

    d_internal_free_stream(&stream);

    return ok;
}


/*
d_internal_compare_length
  Orders links, as their lengths and indices, the shortest first.
*/
static int
d_internal_compare_length(
    const void* _left,
    const void* _right
)
{
    const size_t* const left  = _left;
    const size_t* const right = _right;

    return (left[0] < right[0]) ? -1
         : (left[0] > right[0]) ?  1
         : (left[1] < right[1]) ? -1
         : (left[1] > right[1]) ?  1 : 0;
}


/*
d_internal_label
  What a link shows: a page's link its label, or its title, and its trail;
an image its caption; a category its sort key; an external link its
label.
*/
static bool
d_internal_label(
    struct d_internal_mw* _mw,
    struct d_wiki_link*   _link
)
{
    struct d_wiki_buffer shown = { NULL, 0u, 0u, false };
    bool                 ok    = true;

    // a bare link shows itself, as it was kept
    if (_link->label)
    {
        return true;
    }

    // a label written, as plain text
    if (_link->label_start != D_WIKI_NONE)
    {
        ok = d_internal_plain_span(_mw, _link->label_start, _link->label_end,
                                   (_link->to == D_WIKI_TO_IMAGE), &shown);
    }
    else if ( (_link->form == D_WIKI_FORM_WIKI) &&
              (_link->to != D_WIKI_TO_IMAGE) &&
              (_link->to != D_WIKI_TO_CATEGORY) )
    {
        size_t start = _link->target_start;

        // a title shows without its leading `:`
        while ( (start < _link->target_end) &&
                ( (_mw->text[start] == ':') || (_mw->text[start] == ' ') ) )
        {
            start++;
        }

        ok = d_internal_plain_span(_mw, start, _link->target_end, false,
                                   &shown);
    }

    // a page's link's trail
    if ( (ok) &&
         (_link->form == D_WIKI_FORM_WIKI) &&
         (_link->end > _link->close) )
    {
        ok = d_internal_plain_span(_mw, _link->close, _link->end, false,
                                   &shown);
    }

    (void)d_wiki_buffer_add(&shown, "", 0u);
    _link->label = d_wiki_buffer_take(&shown);

    return (ok) && (_link->label != NULL);
}


/*
d_internal_labels
  Each link's label, the shortest links first, so a link within another
shows its own label in the other's.
*/
static void
d_internal_labels(
    struct d_internal_mw* _mw
)
{
    struct d_wiki* const wiki  = _mw->wiki;
    size_t* const        order = malloc((wiki->link_count + 1u) * 2u *
                                        sizeof(size_t));

    // memory that ran out
    if (!order)
    {
        _mw->failed = true;

        return;
    }

    // the links by length
    for (size_t index = 0; index < wiki->link_count; ++index)
    {
        order[2u * index]        = wiki->links[index].end -
                                   wiki->links[index].start;
        order[(2u * index) + 1u] = index;
    }

    qsort(order, wiki->link_count, 2u * sizeof(size_t),
          d_internal_compare_length);

    // the labels
    for (size_t index = 0; ( (!_mw->failed) && (index < wiki->link_count) );
         ++index)
    {
        // each, the shortest first
        if (!d_internal_label(_mw, &wiki->links[order[(2u * index) + 1u]]))
        {
            _mw->failed = true;
        }
    }

    free(order);

    return;
}


/*
d_internal_titles
  The headings no block has kept -- within templates, elements, tables --
kept as headings not at the top; then each heading's title as plain text.
*/
static void
d_internal_titles(
    struct d_internal_mw* _mw
)
{
    struct d_wiki* const wiki = _mw->wiki;

    // the headings not at the top, each held by the leaf it is in
    for (size_t index = 0; index < _mw->heading_count; ++index)
    {
        const struct d_internal_found_heading* const found =
            &_mw->headings[index];
        size_t                                       block = D_WIKI_NONE;

        // a heading a block holds already
        if (found->kept)
        {
            continue;
        }

        // the leaf it is in
        for (size_t at = 0; at < wiki->block_count; ++at)
        {
            // one that holds it, the innermost so far
            if ( (wiki->blocks[at].start <= found->start) &&
                 (wiki->blocks[at].end > found->start) )
            {
                block = at;
            }
        }

        d_internal_add_heading(_mw, found, block, false);
    }

    // each heading's title
    for (size_t index = 0; ( (!_mw->failed) && (index < wiki->heading_count) );
         ++index)
    {
        struct d_wiki_heading* const heading = &wiki->headings[index];
        struct d_wiki_buffer         title   = { NULL, 0u, 0u, false };
        const bool                   ok      = d_internal_plain_span(
                                                   _mw, heading->title_start,
                                                   heading->title_end, false,
                                                   &title);
        size_t                       start   = 0;
        size_t                       end     = title.length;

        d_internal_trimmed((title.text) ? title.text : "", &start, &end);
        heading->title = d_wiki_copy((title.text) ? &title.text[start] : "",
                                     end - start);
        _mw->failed = (_mw->failed) || (!ok) || (!heading->title);
        d_wiki_buffer_free(&title);
    }

    return;
}


/*
d_internal_redirect
  #REDIRECT [[Page]] at the page's start: the link it leads by.
*/
static void
d_internal_redirect(
    struct d_internal_mw* _mw
)
{
    struct d_wiki* const wiki = _mw->wiki;
    size_t               at   = (wiki->bom) ? 3u : 0u;

    // space, then #REDIRECT, then perhaps `:`, then space
    while ( (at < _mw->length) &&
            ( (d_internal_is_space_or_tab(_mw->text[at])) ||
              (_mw->text[at] == '\n') || (_mw->text[at] == '\r') ) )
    {
        at++;
    }

    // no #REDIRECT
    if (!d_internal_starts_with(&_mw->text[at], _mw->length - at,
                                "#redirect"))
    {
        return;
    }

    at += 9u;

    // the first link after it
    for (size_t index = 0; index < wiki->link_count; ++index)
    {
        const struct d_wiki_link* const link = &wiki->links[index];

        // the first link after #REDIRECT
        if ( (link->start >= at) &&
             (link->form == D_WIKI_FORM_WIKI) )
        {
            size_t between = at;

            // past the space and the `:` between them
            while ( (between < link->start) &&
                    ( (d_internal_is_space_or_tab(_mw->text[between])) ||
                      (_mw->text[between] == ':') ) )
            {
                between++;
            }

            // nothing but those between: the redirect's link
            if (between == link->start)
            {
                wiki->redirect = index;
            }

            return;
        }
    }

    return;
}


/*
d_internal_compare_headings
  Orders the headings found by where they start.
*/
static int
d_internal_compare_headings(
    const void* _left,
    const void* _right
)
{
    const struct d_internal_found_heading* const left  = _left;
    const struct d_internal_found_heading* const right = _right;

    return (left->start < right->start) ? -1
         : (left->start > right->start) ?  1 : 0;
}


/*
d_internal_read
  Reads a page as MediaWiki does: its preprocessor, then its blocks, its
tables' cells, its links -- of the page, of each element, of each
gallery -- the labels, the titles and a redirect.
  Long as the passes run in MediaWiki's order, each after the last has
succeeded.
*/
static bool
d_internal_read(
    struct d_wiki* _wiki
)
{
    struct d_internal_mw mw;

    memset(&mw, 0, sizeof(mw));
    mw.wiki   = _wiki;
    mw.text   = _wiki->text;
    mw.length = _wiki->length;

    // the preprocessor, then the blocks, the links, the titles
    d_internal_preprocess(&mw, (_wiki->bom) ? 3u : 0u, mw.length, 0u);

    // the headings in order, as one closes after those it holds
    if (mw.heading_count > 1u)
    {
        qsort(mw.headings, mw.heading_count,
              sizeof(struct d_internal_found_heading),
              d_internal_compare_headings);
    }

    // the boundaries in order
    if (mw.boundary_count > 1u)
    {
        qsort(mw.boundaries, mw.boundary_count, sizeof(size_t),
              d_internal_compare_offsets);
    }

    // how far each line's opening reaches
    if (!mw.failed)
    {
        d_internal_find_reach(&mw);
    }

    // the blocks, by lines
    if (!mw.failed)
    {
        d_internal_read_blocks(&mw);
    }

    // the tables' cells, which URLs do not cross
    if (!mw.failed)
    {
        d_internal_table_cells(&mw);
    }

    // the links, of the page and of each element
    if (!mw.failed)
    {
        d_internal_read_streams(&mw);
    }

    // the labels, the shortest links first
    if (!mw.failed)
    {
        d_internal_labels(&mw);
    }

    // the titles, and the headings no block kept
    if (!mw.failed)
    {
        d_internal_titles(&mw);
    }

    // a redirect
    if (!mw.failed)
    {
        d_internal_redirect(&mw);
    }

    free(mw.hidden);
    free(mw.boundaries);
    free(mw.headings);
    free(mw.reach);
    free(mw.shut);

    return !mw.failed;
}


//==============================================================================
// 9.  ANCHORS
//==============================================================================


/*
d_internal_space_width
  The length of a character MediaWiki's titles count as white space at
`_at` -- space, `_`, and Unicode's spaces -- 0 for another.
*/
static size_t
d_internal_space_width(
    const char* _at
)
{
    const unsigned char* const c = (const unsigned char*)_at;

    // space and `_`
    if ( (c[0] == ' ') ||
         (c[0] == '_') )
    {
        return 1u;
    }

    // U+00A0, no-break space
    if ( (c[0] == 0xC2u) &&
         (c[1] == 0xA0u) )
    {
        return 2u;
    }

    // U+1680 and U+180E; U+2000 to U+200A, U+2028, U+2029, U+202F, U+205F;
    // U+3000
    return ( ( (c[0] == 0xE1u) &&
               ( ( (c[1] == 0x9Au) && (c[2] == 0x80u) ) ||
                 ( (c[1] == 0xA0u) && (c[2] == 0x8Eu) ) ) ) ||
             ( (c[0] == 0xE2u) &&
               ( ( (c[1] == 0x80u) &&
                   ( ( (c[2] >= 0x80u) && (c[2] <= 0x8Au) ) ||
                     (c[2] == 0xA8u) || (c[2] == 0xA9u) ||
                     (c[2] == 0xAFu) ) ) ||
                 ( (c[1] == 0x81u) && (c[2] == 0x9Fu) ) ) ) ||
             ( (c[0] == 0xE3u) &&
               (c[1] == 0x80u) &&
               (c[2] == 0x80u) ) ) ? 3u : 0u;
}


/*
d_internal_mark_width
  The length of a mark that sets a direction at `_at`, which MediaWiki's
titles drop -- U+200E, U+200F, U+202A to U+202E -- 0 for another.
*/
static size_t
d_internal_mark_width(
    const char* _at
)
{
    const unsigned char* const c = (const unsigned char*)_at;

    return ( (c[0] == 0xE2u) &&
             (c[1] == 0x80u) &&
             ( (c[2] == 0x8Eu) || (c[2] == 0x8Fu) ||
               ( (c[2] >= 0xAAu) && (c[2] <= 0xAEu) ) ) ) ? 3u : 0u;
}


/*
d_internal_add_id_byte
  Writes one byte of an anchor as an id escapes it: HTML5's, a tab, a line
ending or a form feed `_`; else the legacy form's, a byte but a letter, a
digit, `_`, `.`, `-` or `:` written `.` and two hex digits.
*/
static void
d_internal_add_id_byte(
    struct d_wiki_buffer* _out,
    char                  _c,
    bool                  _legacy
)
{
    static const char hex[] = "0123456789ABCDEF";
    char              escaped[4];

    // HTML5's: the space that is no part of an id
    if (!_legacy)
    {
        (void)d_wiki_buffer_add_char(_out, (strchr("\t\n\f\r", _c) != NULL)
                                               ? '_' : _c);

        return;
    }

    // the legacy form's: what a URL holds as it is
    if ( (d_internal_is_alnum(_c)) ||
         (strchr("_.-:", _c) != NULL) )
    {
        (void)d_wiki_buffer_add_char(_out, _c);

        return;
    }

    escaped[0] = '.';
    escaped[1] = hex[((unsigned char)_c) >> 4];
    escaped[2] = hex[((unsigned char)_c) & 0x0Fu];
    escaped[3] = '\0';
    (void)d_wiki_buffer_add_text(_out, escaped);

    return;
}


/*
d_internal_fragment
  Writes a title, its runs of space and `_` one space already, as
MediaWiki makes it a fragment, then an id: the marks that set a direction
gone, each run of white space one `_` but for one at its end, 1024
characters at most -- unless it holds U+FFFD, which it leaves as it is --
then escaped, HTML5's way or the legacy way.
*/
static bool
d_internal_fragment(
    struct d_wiki_buffer* _out,
    const char*           _title,
    bool                  _legacy
)
{
    const bool  odd   = (strstr(_title, "\xEF\xBF\xBD") != NULL);
    const char* at    = _title;
    size_t      count = 0;
    bool        run   = false;

    // each character, to the 1024th
    while ( (*at != '\0') &&
            (count < 1024u) )
    {
        const size_t mark  = (odd) ? 0u : d_internal_mark_width(at);
        const size_t space = ( (odd) || (mark > 0u) )
                                 ? 0u : d_internal_space_width(at);

        // a mark is dropped, and a run of space is one `_` once past it
        if ( (mark > 0u) ||
             (space > 0u) )
        {
            run = (run) || (space > 0u);
            at += (mark > 0u) ? mark : space;

            continue;
        }

        // the run of space before this character, one `_`
        if (run)
        {
            (void)d_wiki_buffer_add_char(_out, '_');
            count++;
            run = false;
        }

        size_t width = 1u;

        // the rest of the character's bytes
        while ( (((unsigned char)at[width] & 0xC0u) == 0x80u) &&
                (width < 4u) )
        {
            width++;
        }

        // each of its bytes, as the id escapes it
        for (size_t index = 0; index < width; ++index)
        {
            d_internal_add_id_byte(_out, (at[index] == ' ') ? '_' : at[index],
                                   _legacy);
        }

        at += width;
        count++;
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_anchor_form
  MediaWiki's anchor of a title, as its finalizeHeadings makes it: trimmed,
each run of spaces and underscores one space, trimmed again; then the
fragment that is, as an id, escaped HTML5's way or the legacy way.
*/
static bool
d_internal_anchor_form(
    struct d_wiki_buffer* _out,
    const char*           _title,
    bool                  _legacy
)
{
    struct d_wiki_buffer spaced  = { NULL, 0u, 0u, false };
    size_t               start   = 0;
    size_t               end     = strlen(_title);
    bool                 between = false;

    // the title trimmed, as PHP trims
    while ( (start < end) &&
            (strchr(" \t\n\r\v", _title[start]) != NULL) )
    {
        start++;
    }

    // and at its end
    while ( (end > start) &&
            (strchr(" \t\n\r\v", _title[end - 1u]) != NULL) )
    {
        end--;
    }

    // each run of space and `_` one space, none at either end
    for (size_t at = start; at < end; ++at)
    {
        // a run of space or `_` is one space, between words
        if ( (_title[at] == ' ') ||
             (_title[at] == '_') )
        {
            between = true;

            continue;
        }

        // one space between words, none at the start
        if ( (between) &&
             (spaced.length > 0u) )
        {
            (void)d_wiki_buffer_add_char(&spaced, ' ');
        }

        between = false;
        (void)d_wiki_buffer_add_char(&spaced, _title[at]);
    }

    (void)d_wiki_buffer_add(&spaced, "", 0u);

    const bool ok = (!spaced.failed) &&
                    (d_internal_fragment(_out, spaced.text, _legacy));

    d_wiki_buffer_free(&spaced);

    return ok;
}


/*
d_internal_anchor
  MediaWiki's anchor of a title, its HTML5 id.
*/
static bool
d_internal_anchor(
    struct d_wiki_buffer* _out,
    const char*           _title
)
{
    return d_internal_anchor_form(_out, _title, false);
}


/*
d_internal_key_kept
  Whether a key is among those kept, `_count` of them; a key made of the
key given and `_2`, `_3`, ... when `_number` is not 0.
*/
static bool
d_internal_key_kept(
    char* const* _keys,
    size_t       _count,
    const char*  _key,
    size_t       _number
)
{
    char         suffix[32];
    const size_t length = strlen(_key);

    (void)snprintf(suffix, sizeof(suffix), "_%zu", _number);

    // each key kept
    for (size_t index = 0; index < _count; ++index)
    {
        // the key, and the number after it when there is one
        if ( (strncmp(_keys[index], _key, length) == 0) &&
             (strcmp(&_keys[index][length], (_number > 0u) ? suffix : "")
                  == 0) )
        {
            return true;
        }
    }

    return false;
}


/*
d_internal_keep_key
  Keeps a key, lower case, or one made of it and the first number from 2
that makes it a key not kept; gives that number, 0 for none, and
D_WIKI_NONE for memory that ran out.
*/
static size_t
d_internal_keep_key(
    char**      _keys,
    size_t*     _count,
    const char* _key
)
{
    struct d_wiki_buffer key    = { NULL, 0u, 0u, false };
    size_t               number = 0;
    char                 suffix[32];

    // the key, in lower case as PHP's strtolower makes it
    for (const char* at = _key; *at != '\0'; ++at)
    {
        (void)d_wiki_buffer_add_char(&key, d_internal_lower(*at));
    }

    (void)d_wiki_buffer_add(&key, "", 0u);

    // a key kept already: the first number that makes another
    if ( (!key.failed) &&
         (d_internal_key_kept(_keys, *_count, key.text, 0u)) )
    {
        number = 2u;

        // the first number from 2 no key kept has
        while (d_internal_key_kept(_keys, *_count, key.text, number))
        {
            number++;
        }

        (void)snprintf(suffix, sizeof(suffix), "_%zu", number);
        (void)d_wiki_buffer_add_text(&key, suffix);
    }

    _keys[*_count] = d_wiki_buffer_take(&key);

    // memory that ran out
    if (!_keys[*_count])
    {
        return D_WIKI_NONE;
    }

    (*_count)++;

    return number;
}


/*
d_internal_number
  Tells the headings' anchors apart as MediaWiki's finalizeHeadings does:
each anchor and its legacy form are looked up, without the case of ASCII
letters, among those kept so far; one kept already is given the first
`_2`, `_3`, ... that is not, and kept so.  A legacy form that is its
anchor's is none, kept as `0`, as PHP keeps `false` as a key.
*/
static bool
d_internal_number(
    struct d_wiki* _wiki
)
{
    char** const keys  = calloc((2u * _wiki->heading_count) + 1u,
                                sizeof(char*));
    size_t       count = 0;
    bool         ok    = (keys != NULL);

    // each heading, in order
    for (size_t index = 0; ( (ok) && (index < _wiki->heading_count) );
         ++index)
    {
        struct d_wiki_heading* const heading = &_wiki->headings[index];
        struct d_wiki_buffer         legacy  = { NULL, 0u, 0u, false };
        const size_t                 number  = d_internal_keep_key(
                                                   keys, &count,
                                                   heading->anchor);

        ok = (number != D_WIKI_NONE) &&
             (d_internal_anchor_form(&legacy, (heading->title)
                                                  ? heading->title : "",
                                     true));

        const bool same = (ok) && (strcmp(legacy.text, heading->anchor) == 0);

        // the legacy form, kept after it, told apart as the anchor is; one
        // that is the anchor is no form of its own, kept once as `0`
        if ( (ok) &&
             ( (!same) ||
               (!d_internal_key_kept(keys, count, "0", 0u)) ) )
        {
            ok = (d_internal_keep_key(keys, &count, (same) ? "0"
                                                           : legacy.text)
                      != D_WIKI_NONE);
        }

        d_wiki_buffer_free(&legacy);

        // a number told it apart
        if ( (ok) &&
             (number > 0u) )
        {
            struct d_wiki_buffer anchor = { NULL, 0u, 0u, false };
            char                 suffix[32];

            (void)snprintf(suffix, sizeof(suffix), "_%zu", number);
            (void)d_wiki_buffer_add_text(&anchor, heading->anchor);
            (void)d_wiki_buffer_add_text(&anchor, suffix);
            free(heading->anchor);
            heading->anchor = d_wiki_buffer_take(&anchor);
            ok              = (heading->anchor != NULL);
        }
    }

    // the keys go
    for (size_t index = 0; index < count; ++index)
    {
        free(keys[index]);
    }

    free(keys);

    return ok;
}


//==============================================================================
// 10.  WRITING
//==============================================================================


/*
d_internal_add_line
  Writes text on one line: each line ending a space.
*/
static void
d_internal_add_line(
    struct d_wiki_buffer* _out,
    const char*           _text
)
{
    // each byte, a line ending a space
    for (const char* at = _text; *at != '\0'; ++at)
    {
        // a line ending is a space
        if ( (*at == '\n') ||
             (*at == '\r') )
        {
            // a CR LF one space, not two
            if ( (*at == '\n') ||
                 (at[1] != '\n') )
            {
                (void)d_wiki_buffer_add_char(_out, ' ');
            }

            continue;
        }

        (void)d_wiki_buffer_add_char(_out, *at);
    }

    return;
}


/*
d_internal_add_literal
  Writes text to be read as it is within <pre> or <nowiki>: `&` and `<`
as references.
*/
static void
d_internal_add_literal(
    struct d_wiki_buffer* _out,
    const char*           _text
)
{
    // each byte, `&` and `<` written as references
    for (const char* at = _text; *at != '\0'; ++at)
    {
        // `&` and `<` as references, and a CR dropped
        if (*at == '&')
        {
            (void)d_wiki_buffer_add_text(_out, "&amp;");
        }
        else if (*at == '<')
        {
            (void)d_wiki_buffer_add_text(_out, "&lt;");
        }
        else if (*at != '\r')
        {
            (void)d_wiki_buffer_add_char(_out, *at);
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
    // the level's `=`s, either side
    for (size_t count = 0; count < _level; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, '=');
    }

    (void)d_wiki_buffer_add_char(_out, ' ');
    d_internal_add_line(_out, _title);
    (void)d_wiki_buffer_add_char(_out, ' ');

    // as many `=` after
    for (size_t count = 0; count < _level; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, '=');
    }

    (void)d_wiki_buffer_add_char(_out, '\n');

    return !_out->failed;
}


/*
d_internal_write_page_link
  Writes [[Page#anchor|label]]: refused for a title MediaWiki's cannot hold
-- one with `#`, `|`, a bracket or a brace, `<` or `>` -- an anchor with
what would end the link, or a label with a link's brackets.
*/
static bool
d_internal_write_page_link(
    struct d_wiki_buffer* _out,
    const char*           _page,
    const char*           _anchor,
    const char*           _label
)
{
    // what would read as another link, or end this one
    if ( (strpbrk(_page, "#|[]{}<>") != NULL) ||
         ( (_anchor) && (strpbrk(_anchor, "|[]{}") != NULL) ) ||
         ( (_label) &&
           ( (strstr(_label, "[[") != NULL) ||
             (strstr(_label, "]]") != NULL) ) ) )
    {
        return false;
    }

    (void)d_wiki_buffer_add_text(_out, "[[");

    // a page in a namespace that is put in place is linked to after `:`
    if ( (_page[0] != ':') &&
         (d_internal_namespace(_page, strlen(_page)) != 0) )
    {
        (void)d_wiki_buffer_add_char(_out, ':');
    }

    d_internal_add_line(_out, _page);

    // the anchor, after `#`
    if (_anchor)
    {
        (void)d_wiki_buffer_add_char(_out, '#');
        d_internal_add_line(_out, _anchor);
    }

    // the label, after `|`
    if (_label)
    {
        (void)d_wiki_buffer_add_char(_out, '|');
        d_internal_add_line(_out, _label);
    }

    (void)d_wiki_buffer_add_text(_out, "]]");

    return !_out->failed;
}


/*
d_internal_add_url
  Writes a URL with what a URL may not hold in wikitext encoded: space,
brackets, `<`, `>`, `"`.
*/
static void
d_internal_add_url(
    struct d_wiki_buffer* _out,
    const char*           _url
)
{
    static const char digits[] = "0123456789ABCDEF";

    // each byte, or its %hh
    for (const char* at = _url; *at != '\0'; ++at)
    {
        // a byte a URL may hold, or its %hh
        if (d_internal_is_url_byte(*at))
        {
            (void)d_wiki_buffer_add_char(_out, *at);
        }
        else
        {
            (void)d_wiki_buffer_add_char(_out, '%');
            (void)d_wiki_buffer_add_char(_out,
                                         digits[((unsigned char)*at) >> 4]);
            (void)d_wiki_buffer_add_char(_out,
                                         digits[((unsigned char)*at) & 15u]);
        }
    }

    return;
}


static bool
d_internal_write_url_link(
    struct d_wiki_buffer* _out,
    const char*           _url,
    const char*           _label
)
{
    const size_t length = strlen(_url);
    const size_t scheme = d_internal_protocol(_url, length, false);

    // a bare URL, when it has nothing a bare one would lose
    if ( (!_label) &&
         (scheme > 0u) &&
         (length > scheme) &&
         (strchr(",;.:!?)", _url[length - 1u]) == NULL) )
    {
        bool bare = true;

        // a URL that is all URL bytes
        for (const char* at = _url; *at != '\0'; ++at)
        {
            bare = (bare) && (d_internal_is_url_byte(*at));
        }

        // written as itself
        if (bare)
        {
            (void)d_wiki_buffer_add_text(_out, _url);

            return !_out->failed;
        }
    }

    (void)d_wiki_buffer_add_char(_out, '[');
    d_internal_add_url(_out, _url);

    // the label, after a space
    if (_label)
    {
        (void)d_wiki_buffer_add_char(_out, ' ');

        // a label holds no `]`
        for (const char* at = _label; *at != '\0'; ++at)
        {
            // a `]` would close the link
            if (*at == ']')
            {
                (void)d_wiki_buffer_add_text(_out, "&#93;");
            }
            else
            {
                (void)d_wiki_buffer_add_char(_out,
                                             ( (*at == '\n') || (*at == '\r') )
                                                 ? ' ' : *at);
            }
        }
    }

    (void)d_wiki_buffer_add_char(_out, ']');

    return !_out->failed;
}


/*
d_internal_write_image
  Writes [[File:source|alt=text]]: refused for a file's name a title cannot
hold, or a text that would end the option or the link.
*/
static bool
d_internal_write_image(
    struct d_wiki_buffer* _out,
    const char*           _source,
    const char*           _text
)
{
    // what would end the option, or the link
    if ( (strpbrk(_source, "#|[]{}<>") != NULL) ||
         (strchr(_text, '|') != NULL) ||
         (strstr(_text, "[[") != NULL) ||
         (strstr(_text, "]]") != NULL) )
    {
        return false;
    }

    (void)d_wiki_buffer_add_text(_out, "[[");

    // the file's namespace, unless it is given
    if (d_internal_namespace(_source, strlen(_source)) != 1)
    {
        (void)d_wiki_buffer_add_text(_out, "File:");
    }

    d_internal_add_line(_out, _source);

    // the alternative text
    if (_text[0] != '\0')
    {
        (void)d_wiki_buffer_add_text(_out, "|alt=");
        d_internal_add_line(_out, _text);
    }

    (void)d_wiki_buffer_add_text(_out, "]]");

    return !_out->failed;
}


static bool
d_internal_write_code(
    struct d_wiki_buffer* _out,
    const char*           _code,
    const char*           _language
)
{
    bool highlight = (_language != NULL);

    // <syntaxhighlight> holds its code as it is, so not its own close
    for (const char* at = _code; ( (highlight) && (*at != '\0') ); ++at)
    {
        highlight = !( (at[0] == '<') &&
                       (at[1] == '/') &&
                       (d_internal_starts_with(&at[2], strlen(&at[2]),
                                               "syntaxhighlight")) );
    }

    // a language: <syntaxhighlight>, else <pre>
    if (highlight)
    {
        (void)d_wiki_buffer_add_text(_out, "<syntaxhighlight lang=\"");

        // the language's name, its letters and the few others names have
        for (const char* at = _language; *at != '\0'; ++at)
        {
            // a byte a language's name may hold
            if ( (d_internal_is_alnum(*at)) ||
                 (strchr("+#-_.", *at) != NULL) )
            {
                (void)d_wiki_buffer_add_char(_out, *at);
            }
        }

        (void)d_wiki_buffer_add_text(_out, "\">\n");
        (void)d_wiki_buffer_add_text(_out, _code);
    }
    else
    {
        (void)d_wiki_buffer_add_text(_out, "<pre>\n");
        d_internal_add_literal(_out, _code);
    }

    // a line ending before the close
    if ( (_code[0] != '\0') &&
         (_code[strlen(_code) - 1u] != '\n') )
    {
        (void)d_wiki_buffer_add_char(_out, '\n');
    }

    (void)d_wiki_buffer_add_text(_out, (highlight) ? "</syntaxhighlight>\n"
                                                   : "</pre>\n");

    return !_out->failed;
}


static bool
d_internal_write_inline_code(
    struct d_wiki_buffer* _out,
    const char*           _code
)
{
    // nothing is no code
    if (_code[0] == '\0')
    {
        return true;
    }

    (void)d_wiki_buffer_add_text(_out, "<code><nowiki>");

    // each byte: `&` and `<` as references, so <nowiki> cannot be closed
    for (const char* at = _code; *at != '\0'; ++at)
    {
        // a reference, or the byte, a line ending a space
        if (*at == '&')
        {
            (void)d_wiki_buffer_add_text(_out, "&amp;");
        }
        else if (*at == '<')
        {
            (void)d_wiki_buffer_add_text(_out, "&lt;");
        }
        else
        {
            (void)d_wiki_buffer_add_char(_out,
                                         ( (*at == '\n') || (*at == '\r') )
                                             ? ' ' : *at);
        }
    }

    (void)d_wiki_buffer_add_text(_out, "</nowiki></code>");

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
    // a marker for each level
    for (size_t count = 0; count < _depth; ++count)
    {
        (void)d_wiki_buffer_add_char(_out, (_ordered) ? '#' : '*');
    }

    (void)d_wiki_buffer_add_char(_out, ' ');
    d_internal_add_line(_out, _text);
    (void)d_wiki_buffer_add_char(_out, '\n');

    return !_out->failed;
}


/*
d_internal_write_cell
  Writes a table's cell: its `|` a reference, and in a header its `!` too,
but for those within a link's or a template's brackets, which keep them.
*/
static void
d_internal_write_cell(
    struct d_wiki_buffer* _out,
    const char*           _cell,
    size_t                _length,
    bool                  _header
)
{
    size_t within = 0;

    // each byte
    for (size_t at = 0; at < _length; ++at)
    {
        const bool pair = (at + 1u < _length) &&
                          (_cell[at + 1u] == _cell[at]);

        // a link's or a template's brackets, opened and closed
        if ( (pair) &&
             ( (_cell[at] == '[') || (_cell[at] == '{') ) )
        {
            within++;
            (void)d_wiki_buffer_add(_out, _cell + at, 2u);
            at++;

            continue;
        }

        // and closed
        if ( (pair) &&
             (within > 0u) &&
             ( (_cell[at] == ']') || (_cell[at] == '}') ) )
        {
            within--;
            (void)d_wiki_buffer_add(_out, _cell + at, 2u);
            at++;

            continue;
        }

        // a cell's `|` is a reference, but within brackets
        if ( (_cell[at] == '|') &&
             (within == 0u) )
        {
            (void)d_wiki_buffer_add_text(_out, "&#124;");
        }
        else if ( (_header) &&
                  (_cell[at] == '!') &&
                  (within == 0u) )
        {
            (void)d_wiki_buffer_add_text(_out, "&#33;");
        }
        else if (_cell[at] != '\r')
        {
            (void)d_wiki_buffer_add_char(_out, _cell[at]);
        }
    }

    return;
}


static bool
d_internal_write_table(
    struct d_wiki_buffer* _out,
    const char*           _rows
)
{
    const char* at  = _rows;
    size_t      row = 0;

    (void)d_wiki_buffer_add_text(_out, "{| class=\"wikitable\"\n");

    // each row: the header's cells after `!`, the others' after `|`
    while (*at != '\0')
    {
        const bool header = (row == 0u);
        size_t     cell   = 0;

        // a row after the header begins with `|-`
        if (!header)
        {
            (void)d_wiki_buffer_add_text(_out, "|-\n");
        }

        (void)d_wiki_buffer_add_text(_out, (header) ? "! " : "| ");

        // each cell, apart by a tab
        while ( (*at != '\0') &&
                (*at != '\n') )
        {
            const char* const tab  = strpbrk(at, "\t\n");
            const size_t      span = (tab) ? (size_t)(tab - at) : strlen(at);

            // cells apart by `!!` or `||`
            if (cell > 0u)
            {
                (void)d_wiki_buffer_add_text(_out, (header) ? " !! " : " || ");
            }

            d_internal_write_cell(_out, at, span, header);
            cell++;
            at += span;
            at += (*at == '\t') ? 1u : 0u;
        }

        (void)d_wiki_buffer_add_char(_out, '\n');
        at += (*at == '\n') ? 1u : 0u;
        row++;
    }

    (void)d_wiki_buffer_add_text(_out, "|}\n");

    return !_out->failed;
}


/*
d_internal_ends_protocol
  Whether the `:` at `_colon` of a text ends the name of a URL's protocol,
mailto:, that begins a word.
*/
static bool
d_internal_ends_protocol(
    const char* _text,
    const char* _colon
)
{
    const char* start = _colon;

    // back over the letters before it
    while ( (start > _text) &&
            (d_internal_is_alpha(start[-1])) )
    {
        start--;
    }

    // a word of its own, that a protocol's name with its `:` is
    if ( (start == _colon) ||
         ( (start > _text) &&
           ( (d_internal_is_alnum(start[-1])) || (start[-1] == '_') ||
             (((unsigned char)start[-1]) >= 0x80u) ) ) )
    {
        return false;
    }

    return (d_internal_protocol(start, (size_t)(_colon - start) + 1u, false)
                == (size_t)(_colon - start) + 1u);
}


/*
d_internal_write_escape
  Writes text so it reads as itself: each byte that would begin markup, as
a reference -- `&`, `<`, `>`, brackets, braces, `|`; two apostrophes,
`~~~` and `__`; `:` at a line's start, before `//` or ending a protocol's
name, as mailto: does; what begins a line's markup there; and a `=` that
ends a line.
  Long as a dispatch on what each byte would begin is.
*/
static bool
d_internal_write_escape(
    struct d_wiki_buffer* _out,
    const char*           _text
)
{
    bool line_start = true;

    // each byte that could begin markup, as a reference
    for (const char* at = _text; *at != '\0'; ++at)
    {
        const char  c    = *at;
        const char* code = NULL;

        // a reference for each byte that begins markup
        switch (c)
        {
            case '&':  code = "&amp;";   break;
            case '<':  code = "&lt;";    break;
            case '>':  code = "&gt;";    break;
            case '[':  code = "&#91;";   break;
            case ']':  code = "&#93;";   break;
            case '{':  code = "&#123;";  break;
            case '}':  code = "&#125;";  break;
            case '|':  code = "&#124;";  break;
            case '\'': code = (at[1] == '\'') ? "&#39;" : NULL; break;
            case '~':  code = ( (at[1] == '~') && (at[2] == '~') )
                                  ? "&#126;" : NULL; break;
            case '_':  code = (at[1] == '_') ? "&#95;" : NULL; break;
            case ':':  code = ( (line_start) ||
                                ( (at[1] == '/') && (at[2] == '/') ) ||
                                (d_internal_ends_protocol(_text, at)) )
                                  ? "&#58;" : NULL; break;
            default:   break;
        }

        // what begins a line's markup, at its start
        if ( (line_start) &&
             (!code) )
        {
            code = (c == '*') ? "&#42;"
                 : (c == '#') ? "&#35;"
                 : (c == ';') ? "&#59;"
                 : (c == '=') ? "&#61;"
                 : (c == ' ') ? "&#32;"
                 : (c == '-') ? "&#45;"
                 :              NULL;
        }

        // a `=` that ends a line
        if ( (!code) &&
             (c == '=') &&
             ( (at[1] == '\0') || (at[1] == '\n') ) )
        {
            code = "&#61;";
        }

        // a reference for the byte, or the byte
        if (code)
        {
            (void)d_wiki_buffer_add_text(_out, code);
        }
        else
        {
            (void)d_wiki_buffer_add_char(_out, c);
        }

        line_start = (c == '\n');
    }

    (void)d_wiki_buffer_add(_out, "", 0u);

    return !_out->failed;
}


/*
d_internal_write_option
  What stands in an image's link=: a URL, whole, or a page's title, which
holds no `|`, bracket, brace, `<`, `>` or line ending.
*/
static bool
d_internal_write_option(
    struct d_wiki_buffer* _out,
    const char*           _target
)
{
    const size_t length = strlen(_target);
    const size_t scheme = d_internal_protocol(_target, length, true);

    // a URL, whole
    if ( (scheme > 0u) &&
         (!d_internal_whole_url(_target, scheme, length)) )
    {
        return false;
    }

    // a title's bytes
    for (const char* at = _target; ( (scheme == 0u) && (*at != '\0') ); ++at)
    {
        // a byte no title may hold here
        if ( (!d_internal_is_title_byte(*at)) ||
             (*at == '{') ||
             (*at == '}') )
        {
            return false;
        }
    }

    // nothing is no link's
    if (length == 0u)
    {
        return false;
    }

    (void)d_wiki_buffer_add_text(_out, _target);

    return !_out->failed;
}


/*
d_internal_write_target
  What stands in a link's destination, in its form: a title, kept a page's
link with `:` where its namespace would put it in place; a URL.
  Long as each form's destination has rules of its own: a title's, a
gallery's file's, a URL's.
*/
static bool
d_internal_write_target(
    struct d_wiki_buffer*     _out,
    const struct d_wiki_link* _link,
    const char*               _target
)
{
    const size_t length = strlen(_target);

    // an image's link=: a URL, whole, or a page's title
    if (_link->form == D_WIKI_FORM_OPTION)
    {
        return d_internal_write_option(_out, _target);
    }

    // a title holds only what a title may; a gallery's names a file
    if ( (_link->form == D_WIKI_FORM_WIKI) ||
         (_link->form == D_WIKI_FORM_GALLERY) )
    {
        // a title's bytes, and no template's brace
        for (const char* at = _target; *at != '\0'; ++at)
        {
            // a byte no title may hold
            if ( (!d_internal_is_title_byte(*at)) ||
                 (*at == '{') ||
                 (*at == '}') )
            {
                return false;
            }
        }

        // no title, or a URL where a title goes
        if ( (length == 0u) ||
             (d_internal_protocol(_target, length, true) > 0u) )
        {
            return false;
        }

        // a page's link stays one; a gallery's names a file
        if ( (_link->form == D_WIKI_FORM_WIKI) &&
             (_target[0] != ':') &&
             ( (_link->target[0] == ':') ||
               ( (_link->to == D_WIKI_TO_PAGE) &&
                 (d_internal_namespace(_target, length) != 0) ) ) )
        {
            (void)d_wiki_buffer_add_char(_out, ':');
        }
        else if ( (_link->form == D_WIKI_FORM_GALLERY) &&
                  ( (_target[0] == ':') ||
                    (d_internal_namespace(_target, length) >= 2) ) )
        {
            return false;
        }

        (void)d_wiki_buffer_add_text(_out, _target);

        return !_out->failed;
    }

    // a URL, its protocol first, a bare one with nothing it would lose
    if ( (_link->form != D_WIKI_FORM_EXTERNAL) &&
         (_link->form != D_WIKI_FORM_BARE) )
    {
        return false;
    }

    // a URL the form takes: an external link's protocol, a bare one's end
    if ( (d_internal_protocol(_target, length,
                              (_link->form == D_WIKI_FORM_EXTERNAL)) == 0u) ||
         ( (_link->form == D_WIKI_FORM_BARE) &&
           (strchr(",;.:!?)", _target[length - 1u]) != NULL) ) )
    {
        return false;
    }

    // every byte a URL may hold
    for (const char* at = _target; *at != '\0'; ++at)
    {
        // a byte a URL may not hold
        if (!d_internal_is_url_byte(*at))
        {
            return false;
        }
    }

    (void)d_wiki_buffer_add_text(_out, _target);

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
    // a title's label holds no [[ or ]]; a URL's no `]` or line ending; a
    // gallery's caption no line ending
    if (_link->form == D_WIKI_FORM_WIKI)
    {
        // a label that would end the link or begin another
        if ( (strstr(_label, "]]") != NULL) ||
             (strstr(_label, "[[") != NULL) )
        {
            return false;
        }
    }
    else if ( (_link->form == D_WIKI_FORM_EXTERNAL) ||
              (_link->form == D_WIKI_FORM_GALLERY) )
    {
        // a label that would end an external link, or a gallery's line
        if (strpbrk(_label, (_link->form == D_WIKI_FORM_EXTERNAL)
                                ? "]\n\r" : "\n\r") != NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    (void)d_wiki_buffer_add_text(_out, _label);

    return !_out->failed;
}


/*
d_wiki_mediawiki_driver
  The driver's table.
*/
const struct d_wiki_driver*
d_wiki_mediawiki_driver(void)
{
    static const struct d_wiki_driver driver =
    {
        D_WIKI_MEDIAWIKI,
        d_internal_read,
        d_internal_anchor,
        '_',
        2u,
        d_internal_number,
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
