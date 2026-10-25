/*******************************************************************************
* djinterp [tools]                                                dwiki_driver.h
*
* What a dialect's driver gives the wiki reader, and what the reader gives
* its drivers.
*   A driver reads a page's text into the records of dwiki.h -- its blocks
* and their segments, its headings, its links, and, in MediaWiki, its
* templates -- and makes the anchor of a title, and writes each thing a page
* holds. The reader does the rest, the same for every dialect: the lines,
* the records' places, the anchors made unique, the sections, the regions.
*   Private to the wiki units: dwiki.c, dwiki_doc.c and the drivers.
*
* path:      /src/djinterp/tools/dawk/ext/wiki/dwiki_driver.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Drivers
         1.  d_wiki_driver
2.  OPERATIONS
    ----------
    1.  The drivers
    2.  Records
    3.  Characters
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_DRIVER_H
#define DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_DRIVER_H 1

// std
#include <stdbool.h>                                     // bool
#include <stddef.h>                                      // size_t
#include <stdint.h>                                      // uint32_t
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/ext/wiki/dwiki.h"  // d_wiki


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Drivers
//------------------------------------------------------------------------------
// 1.1.1
// d_wiki_driver
//   struct: one dialect.  `read` fills the records the driver makes from
//   the text, with their places, and a heading's `title`; the reader fills
//   the rest.  `anchor` writes a title's anchor before it is made unique,
//   and a second heading of one anchor is told apart by `separator` and a
//   number, counting from `first_repeat` -- or, for a dialect with
//   `number`, as that tells them apart, each heading's `anchor` its
//   title's when it is called.  The writers are those of dwiki.h,
//   less the dialect; `target` and `label` write what goes in place of a
//   link's destination and of its label, as written, for the form it has.
struct d_wiki_driver
{
    enum d_wiki_dialect dialect;
    bool   (*read)(struct d_wiki* _wiki);
    bool   (*anchor)(struct d_wiki_buffer* _out,
                     const char*           _title);
    char   separator;
    size_t first_repeat;
    bool   (*number)(struct d_wiki* _wiki);
    bool   (*heading)(struct d_wiki_buffer* _out,
                      size_t                _level,
                      const char*           _title);
    bool   (*page_link)(struct d_wiki_buffer* _out,
                        const char*           _page,
                        const char*           _anchor,
                        const char*           _label);
    bool   (*url_link)(struct d_wiki_buffer* _out,
                       const char*           _url,
                       const char*           _label);
    bool   (*image)(struct d_wiki_buffer* _out,
                    const char*           _source,
                    const char*           _text);
    bool   (*code)(struct d_wiki_buffer* _out,
                   const char*           _code,
                   const char*           _language);
    bool   (*inline_code)(struct d_wiki_buffer* _out,
                          const char*           _code);
    bool   (*item)(struct d_wiki_buffer* _out,
                   const char*           _text,
                   size_t                _depth,
                   bool                  _ordered);
    bool   (*table)(struct d_wiki_buffer* _out,
                    const char*           _rows);
    bool   (*escape)(struct d_wiki_buffer* _out,
                     const char*           _text);
    bool   (*target)(struct d_wiki_buffer*     _out,
                     const struct d_wiki_link* _link,
                     const char*               _target);
    bool   (*label)(struct d_wiki_buffer*     _out,
                    const struct d_wiki_link* _link,
                    const char*               _label);
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The drivers
//------------------------------------------------------------------------------
// d_wiki_driver_of is a dialect's driver, NULL for one with none.
const struct d_wiki_driver* d_wiki_driver_of(enum d_wiki_dialect _dialect);
const struct d_wiki_driver* d_wiki_markdown_driver(void);
const struct d_wiki_driver* d_wiki_mediawiki_driver(void);

// 2.2    Records
//------------------------------------------------------------------------------
// Each adds a record and gives its index, or D_WIKI_NONE when memory runs
// out.  d_wiki_add_block begins a block of a kind at an offset, its other
// fields D_WIKI_NONE or 0, its segments those added after it;
// d_wiki_add_segment adds a line to the last block's content.  A link, a
// template or an argument is copied, and its text taken.
size_t d_wiki_add_block(struct d_wiki*         _wiki,
                        enum d_wiki_block_kind _kind,
                        size_t                 _start);
bool   d_wiki_add_segment(struct d_wiki* _wiki,
                          size_t         _start,
                          size_t         _end,
                          size_t         _pad);
size_t d_wiki_add_heading(struct d_wiki*                _wiki,
                          const struct d_wiki_heading*  _heading);
size_t d_wiki_add_link(struct d_wiki*             _wiki,
                       const struct d_wiki_link*  _link);
size_t d_wiki_add_template(struct d_wiki*                 _wiki,
                           const struct d_wiki_template*  _template);
size_t d_wiki_add_argument(struct d_wiki*                 _wiki,
                           const struct d_wiki_argument*  _argument);
// d_wiki_add_problem keeps what a reader would want to know, at an offset.
bool   d_wiki_add_problem(struct d_wiki* _wiki,
                          size_t         _offset,
                          const char*    _what);
// d_wiki_new_link is a link of a form with nothing written, at a place.
void   d_wiki_new_link(struct d_wiki_link* _link,
                       enum d_wiki_form    _form,
                       size_t              _start,
                       size_t              _end);

// 2.3    Characters
//------------------------------------------------------------------------------
// d_wiki_utf8 reads the character at `_text`, of `_length` bytes left,
// giving how many bytes it takes; a byte that begins no character is read
// as U+FFFD, one byte long.  d_wiki_add_utf8 writes a character.
uint32_t d_wiki_utf8(const char* _text,
                     size_t      _length,
                     size_t*     _out_bytes);
bool     d_wiki_add_utf8(struct d_wiki_buffer* _out,
                         uint32_t              _character);
// d_wiki_entity reads a character reference at `_text` -- &name;, &#nn;,
// &#xhh; -- writing what it stands for, and gives its length, or 0 for
// text that is not one.
size_t   d_wiki_entity(const char*           _text,
                       size_t                _length,
                       struct d_wiki_buffer* _out);
// d_wiki_emoji writes the emoji GitHub names `_name` -- "bulb" is U+1F4A1
// -- and says whether there is one.
bool     d_wiki_emoji(const char*           _name,
                      size_t                _length,
                      struct d_wiki_buffer* _out);
// d_wiki_is_punctuation, d_wiki_is_space and d_wiki_is_word classify a
// character as Unicode does: punctuation and symbols; space separators,
// tab, line feed, form feed and carriage return; a word's letters, marks,
// digits and connectors.  d_wiki_lower is its simple lower case.
bool     d_wiki_is_punctuation(uint32_t _character);
bool     d_wiki_is_space(uint32_t _character);
bool     d_wiki_is_word(uint32_t _character);
uint32_t d_wiki_lower(uint32_t _character);
// d_wiki_has_scheme is whether a destination begins with a URL's scheme,
// `https:`, `mailto:`; d_wiki_is_image whether a path names an image by
// its extension.
bool     d_wiki_has_scheme(const char* _target);
bool     d_wiki_is_image(const char* _path,
                         size_t      _length);
// d_wiki_normalize_label writes a label as Markdown compares labels: its
// runs of space one space, none at either end, in lower case.
bool     d_wiki_normalize_label(struct d_wiki_buffer* _out,
                                const char*           _label,
                                size_t                _length);


#endif  // DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_DRIVER_H
