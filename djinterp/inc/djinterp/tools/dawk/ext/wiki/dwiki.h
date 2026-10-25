/*******************************************************************************
* djinterp [tools]                                                       dwiki.h
*
* A wiki's pages, read without loss.
*   One page model, read from any of the dialects a wiki is written in, each
* by a driver of its own: Markdown as a GitHub wiki writes it -- CommonMark,
* GitHub's tables, strikethrough and bare links, and Gollum's [[Text|Page]]
* links -- and MediaWiki's wikitext. A page is read into records, each with
* its place in the text, so a change to one is a splice that leaves every
* other byte of the page as it was.
*   The records: the page's blocks, in document order, containers before
* what they hold, each leaf with the lines of its content; its headings,
* each with its title as plain text and the anchor a link to it uses; its
* sections, made from the headings at the page's top level, the lead before
* the first of them; its links, of every form the dialect has, each with
* where it leads -- a page, an anchor on this page, a URL, a file, an image,
* a category -- and the text it shows; MediaWiki's templates, parser
* functions and parameters, each with its arguments; and the regions a
* program manages, between two comments:
*       <!-- dawk:begin NAME -->
*       ...
*       <!-- dawk:end NAME -->
* each on a line of its own, at the page's top level.
*   Reading never fails on what a page holds: any text is a page. What a
* reader of the page would want to know -- a region left open, a template
* never closed -- is kept as a problem, with its line.
*   A page's name, its file's, and the anchor of a title are each a
* dialect's own: a GitHub wiki keeps "Page Name" in Page-Name.md and makes
* an anchor of a title as GitHub does; MediaWiki writes Page_Name and makes
* its anchors its own way. And each dialect writes what a generator needs:
* a heading, a link, code, a list's item, a table, text escaped to read as
* itself.
*   A stand-in, local to dawk: markup belongs to the parse tier, and when
* that tier reads it, this unit gives way to it.
*
* path:      /inc/djinterp/tools/dawk/ext/wiki/dwiki.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_WIKI_NONE
         2.  D_WIKI_MOST_LEVEL
    2.  Kinds
         1.  d_wiki_dialect
         2.  d_wiki_block_kind
         3.  d_wiki_form
         4.  d_wiki_target
         5.  d_wiki_template_kind
    3.  Records
         1.  d_wiki_segment
         2.  d_wiki_block
         3.  d_wiki_heading
         4.  d_wiki_section
         5.  d_wiki_link
         6.  d_wiki_template
         7.  d_wiki_argument
         8.  d_wiki_region
         9.  d_wiki_problem
        10.  d_wiki
    4.  Text
         1.  d_wiki_buffer
2.  OPERATIONS
    ----------
    1.  Dialects
    2.  Reading
    3.  Records
    4.  Names
    5.  Text
    6.  Writing
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_H
#define DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_WIKI_NONE
//   constant: the index of a record that is not there, and the place of
//   text that is not written -- a link with no label of its own.
#define D_WIKI_NONE ((size_t)-1)

// 1.1.2
// D_WIKI_MOST_LEVEL
//   constant: the deepest level a heading has, in either dialect.
#define D_WIKI_MOST_LEVEL 6u

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_wiki_dialect
//   enum: what a page is written in.
enum d_wiki_dialect
{
    D_WIKI_OTHER = 0,
    D_WIKI_MARKDOWN,   // .md, .markdown: a GitHub wiki's
    D_WIKI_MEDIAWIKI   // .mediawiki, .wiki, .wikitext
};

// 1.2.2
// d_wiki_block_kind
//   enum: what a block is.
enum d_wiki_block_kind
{
    D_WIKI_PARAGRAPH = 0,
    D_WIKI_HEADING,
    D_WIKI_CODE,          // fenced, indented; <pre>, <syntaxhighlight>
    D_WIKI_PREFORMATTED,  // MediaWiki's lines that begin with a space
    D_WIKI_QUOTE,         // a container: > in Markdown
    D_WIKI_ITEM,          // a list's item: a container in Markdown
    D_WIKI_TABLE,
    D_WIKI_RULE,
    D_WIKI_HTML,          // an HTML block
    D_WIKI_COMMENT,       // a comment standing alone on its lines
    D_WIKI_DEFINITION,    // Markdown's [label]: destination
    D_WIKI_METADATA       // YAML between --- lines, a page's first
};

// 1.2.3
// d_wiki_form
//   enum: how a link is written.
enum d_wiki_form
{
    D_WIKI_FORM_WIKI = 0,    // [[...]], Gollum's or MediaWiki's
    D_WIKI_FORM_INLINE,      // [text](destination)
    D_WIKI_FORM_REFERENCE,   // [text][label], [label][], [label]
    D_WIKI_FORM_AUTO,        // <https://...>
    D_WIKI_FORM_BARE,        // https://... in the text
    D_WIKI_FORM_EXTERNAL,    // MediaWiki's [https://... text]
    D_WIKI_FORM_DEFINITION,  // [label]: destination
    D_WIKI_FORM_HTML,        // <a href="...">, <img src="...">
    D_WIKI_FORM_OPTION,      // MediaWiki's image's link=Page, link=url
    D_WIKI_FORM_GALLERY      // a line of MediaWiki's <gallery>
};

// 1.2.4
// d_wiki_target
//   enum: where a link leads.
enum d_wiki_target
{
    D_WIKI_TO_PAGE = 0,   // a page of the wiki, perhaps at an anchor
    D_WIKI_TO_ANCHOR,     // an anchor on this page
    D_WIKI_TO_URL,        // anywhere else
    D_WIKI_TO_MAIL,       // an address
    D_WIKI_TO_FILE,       // a file the wiki holds
    D_WIKI_TO_IMAGE,      // an image, shown in place when embedded
    D_WIKI_TO_CATEGORY,   // MediaWiki: the page is put in a category
    D_WIKI_TO_TAG         // Gollum: [[_TOC_]], [[include:Page]]
};

// 1.2.5
// d_wiki_template_kind
//   enum: what MediaWiki's double braces hold.
enum d_wiki_template_kind
{
    D_WIKI_TEMPLATE = 0,   // {{Name|...}}
    D_WIKI_FUNCTION,       // {{#if:...}}, {{lc:...}}
    D_WIKI_VARIABLE,       // {{PAGENAME}}
    D_WIKI_PARAMETER       // {{{1|default}}}
};

// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// d_wiki_segment
//   struct: one line of a leaf's content: the bytes from `start` to `end`,
//   its line ending not among them, with `pad` spaces before them where a
//   tab was taken in part by the indent before the content.
struct d_wiki_segment
{
    size_t start;
    size_t end;
    size_t pad;
};

// 1.3.2
// d_wiki_block
//   struct: one block.  `start` is where its text begins -- a container's
//   marker, a leaf's first byte -- and `end` is past its last line's
//   ending.  A leaf's content is `segment_count` segments from
//   `first_segment` -- an HTML block's first, and a comment's, with the
//   space before its start, as its content keeps it.  A code block's
//   language, as written, is `info_start` to `info_end`, D_WIKI_NONE when
//   there is none.  `level` is a heading's
//   level and an item's depth among lists; `marker` is an item's -- `-`,
//   `*`, `+`, `.` or `)` after a number, MediaWiki's last `*#:;` -- or a
//   fence's.  `parent` is the container holding it, and `depth` how many
//   containers do.
struct d_wiki_block
{
    enum d_wiki_block_kind kind;
    size_t                 start;
    size_t                 end;
    size_t                 line;
    size_t                 end_line;
    size_t                 depth;
    size_t                 parent;
    size_t                 first_segment;
    size_t                 segment_count;
    size_t                 info_start;
    size_t                 info_end;
    size_t                 level;
    char                   marker;
};

// 1.3.3
// d_wiki_heading
//   struct: one heading.  `start` and `end` bound its lines, its last
//   line's ending included -- from the start of its first line when it
//   stands at the page's top level -- and `title_start` and `title_end` its
//   title as written.  `title` is that title as plain text, and `anchor`
//   the anchor a link to it uses, made unique on the page as the dialect
//   makes one.  `top` says it stands at the page's top level: in no quote,
//   list or template.  `setext` says Markdown underlines it.
struct d_wiki_heading
{
    size_t block;
    size_t level;
    size_t start;
    size_t end;
    size_t title_start;
    size_t title_end;
    size_t line;
    char*  title;
    char*  anchor;
    bool   top;
    bool   setext;
};

// 1.3.4
// d_wiki_section
//   struct: one section: a heading at the top level and what follows it,
//   up to the next heading at its level or above.  The first section is the
//   lead, what comes before the first heading, with no heading, at level 0.
//   `start` is its heading's line; `body` is past the heading; `own_end` is
//   where its first subsection begins, or its end; `end` is where the next
//   section at its level or above begins, or the page's end.  `parent` is
//   the section it is a subsection of, D_WIKI_NONE for one at the top.
struct d_wiki_section
{
    size_t heading;
    size_t level;
    size_t start;
    size_t body;
    size_t own_end;
    size_t end;
    size_t parent;
};

// 1.3.5
// d_wiki_link
//   struct: one link.  `start` and `end` bound it, MediaWiki's trail of
//   letters after `]]` included, and `close` is past its own close.  The
//   destination as written is `target_start` to `target_end` -- for a
//   reference, the label it is looked up by -- the text it shows as
//   written `label_start` to `label_end`, and a Markdown link's title,
//   its quotes with it, `title_start` to `title_end`; each D_WIKI_NONE
//   when not written.
//   `target` is the destination, decoded -- a reference's from its
//   definition, `definition`; `page` the page, file, image or category it
//   names, and `anchor` what follows `#`, each "" when there is none; and
//   `label` the text it shows, as plain text.  `embedded` says an image is
//   shown in place.  `block` is the leaf holding it and `parent` the
//   template whose arguments do, D_WIKI_NONE for none.
//   MediaWiki's titles are read as MediaWiki reads them: percent-decoded
//   when they hold `%`, then their references; a title no page can have
//   makes no link.  An image's link= is a link of its own, its destination
//   the option's value; a gallery's line is a link to the image it names,
//   in File: when it names no namespace, its caption its label.
struct d_wiki_link
{
    enum d_wiki_form   form;
    enum d_wiki_target to;
    bool               embedded;
    size_t             start;
    size_t             end;
    size_t             close;
    size_t             target_start;
    size_t             target_end;
    size_t             label_start;
    size_t             label_end;
    size_t             title_start;
    size_t             title_end;
    size_t             definition;
    size_t             block;
    size_t             parent;
    size_t             line;
    size_t             column;
    char*              target;
    char*              page;
    char*              anchor;
    char*              label;
};

// 1.3.6
// d_wiki_template
//   struct: one of MediaWiki's double or triple braces.  `start` and `end`
//   bound it; its name is `name_start` to `name_end`, past the prefixes
//   MediaWiki takes off it -- `safesubst:`, `msgnw:`, `msg:`, `raw:` --
//   and `name` that name as MediaWiki reads it, trimmed, its comments
//   gone; a function's name is what is before its `:`.  Its arguments are
//   `argument_count` from `first_argument`: a parser function's first is
//   what follows its `:`.  `parent` is the template whose arguments hold
//   it.
struct d_wiki_template
{
    enum d_wiki_template_kind kind;
    size_t                    start;
    size_t                    end;
    size_t                    name_start;
    size_t                    name_end;
    size_t                    first_argument;
    size_t                    argument_count;
    size_t                    parent;
    size_t                    line;
    size_t                    column;
    char*                     name;
};

// 1.3.7
// d_wiki_argument
//   struct: one argument: from past the `|` before it to the `|` or braces
//   after.  A named argument's name is `name_start` to `name_end`, before
//   its `=`, and its value what follows; a numbered one has no name, and
//   its value is all of it.  `key` is the name trimmed, or the number it
//   is given, counting the numbered from 1.
struct d_wiki_argument
{
    size_t start;
    size_t end;
    size_t name_start;
    size_t name_end;
    size_t value_start;
    size_t value_end;
    char*  key;
};

// 1.3.8
// d_wiki_region
//   struct: one region a program manages: `begin` is the start of its
//   begin marker's line and `content` past that line; `close` is the start
//   of its end marker's line and `end` past that line.
struct d_wiki_region
{
    char*  name;
    size_t begin;
    size_t content;
    size_t close;
    size_t end;
    size_t line;
};

// 1.3.9
// d_wiki_problem
//   struct: something a reader of the page would want to know, at a line.
struct d_wiki_problem
{
    size_t line;
    char*  what;
};

// 1.3.10
// d_wiki
//   struct: one page read.  The text is the caller's, borrowed while the
//   records are used.  `sections` begins with the lead.  `redirect` is the
//   link a MediaWiki redirect leads by, D_WIKI_NONE for a page that is not
//   one.  `bom` and `crlf` say how the text begins and ends its lines.
//   The capacities and the starts of lines are the reader's.
struct d_wiki
{
    const char*             text;
    size_t                  length;
    enum d_wiki_dialect     dialect;
    bool                    bom;
    bool                    crlf;
    struct d_wiki_block*    blocks;
    size_t                  block_count;
    size_t                  block_capacity;
    struct d_wiki_segment*  segments;
    size_t                  segment_count;
    size_t                  segment_capacity;
    struct d_wiki_heading*  headings;
    size_t                  heading_count;
    size_t                  heading_capacity;
    struct d_wiki_section*  sections;
    size_t                  section_count;
    struct d_wiki_link*     links;
    size_t                  link_count;
    size_t                  link_capacity;
    struct d_wiki_template* templates;
    size_t                  template_count;
    size_t                  template_capacity;
    struct d_wiki_argument* arguments;
    size_t                  argument_count;
    size_t                  argument_capacity;
    struct d_wiki_region*   regions;
    size_t                  region_count;
    size_t                  region_capacity;
    struct d_wiki_problem*  problems;
    size_t                  problem_count;
    size_t                  problem_capacity;
    size_t                  redirect;
    size_t*                 line_starts;
    size_t                  line_count;
};

// 1.4    Text
//------------------------------------------------------------------------------
// 1.4.1
// d_wiki_buffer
//   struct: text being written, terminated, in room that grows.  `failed`
//   says memory ran out, and that `text` is not all that was written.
struct d_wiki_buffer
{
    char*  text;
    size_t length;
    size_t capacity;
    bool   failed;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Dialects
//------------------------------------------------------------------------------
// d_wiki_dialect_of names a page's dialect by its file's name, in any case;
// d_wiki_dialect_named by a name a program gives -- "markdown", "md",
// "github", "gfm", "mediawiki", "wiki", "wikitext"; and d_wiki_dialect_name
// gives the dialect's own name, "markdown" or "mediawiki", or "other".
enum d_wiki_dialect d_wiki_dialect_of(const char* _path);
enum d_wiki_dialect d_wiki_dialect_named(const char* _name);
const char*         d_wiki_dialect_name(enum d_wiki_dialect _dialect);

// 2.2    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a page's text into records.
 *
 * @note Any text is a page; what is odd in one is kept among its problems.
 *       A byte-order mark is kept, and read past.
 *
 * @param[out] _out       receives the records; release it with
 *                        d_wiki_release, whether or not reading succeeded.
 * @param[in]  _text      the text, borrowed for as long as `_out` is used.
 * @param[in]  _length    its length in bytes.
 * @param[in]  _dialect   what it is written in: D_WIKI_MARKDOWN or
 *                        D_WIKI_MEDIAWIKI.
 * @param[out] _problem   receives why reading failed.
 * @param[in]  _capacity  the size of `_problem`.
 * @return `true` if the text was read; `false` when the dialect is not one
 *         a driver reads, or memory ran out.
 */
bool   d_wiki_read(struct d_wiki*      _out,
                   const char*         _text,
                   size_t              _length,
                   enum d_wiki_dialect _dialect,
                   char*               _problem,
                   size_t              _capacity);
void   d_wiki_release(struct d_wiki* _wiki);

// 2.3    Records
//------------------------------------------------------------------------------
// d_wiki_line_of gives the line, from 1, of an offset into the text, and
// its column, from 1, in bytes; `_out_column` may be NULL.
size_t d_wiki_line_of(const struct d_wiki* _wiki,
                      size_t               _offset,
                      size_t*              _out_column);
// d_wiki_content writes a block's content as its reader takes it: each of
// its segments, padded, a line feed after each but the last.
bool   d_wiki_content(const struct d_wiki*  _wiki,
                      size_t                _block,
                      struct d_wiki_buffer* _out);
// d_wiki_find_region is the region of a name, d_wiki_find_definition the
// definition a label names, compared as Markdown compares labels; each
// D_WIKI_NONE when there is none.
size_t d_wiki_find_region(const struct d_wiki* _wiki,
                          const char*          _name);
size_t d_wiki_find_definition(const struct d_wiki* _wiki,
                              const char*          _label,
                              size_t               _length);

// 2.4    Names
//------------------------------------------------------------------------------
/**
 * @brief Gives the page a file holds: its name without its directory or
 *        extension, written as the dialect's links write it.
 *
 * @note A GitHub wiki shows Page-Name.md, and Page-Name.mediawiki, as
 *       "Page Name"; MediaWiki's Page_Name is "Page Name" too.
 *
 * @param[in] _path     the file's path.
 * @param[in] _dialect  the dialect; `D_WIKI_OTHER` for the file's own.
 * @post   The caller frees the name.
 * @return the name, allocated; `NULL` when memory runs out.
 */
char*  d_wiki_page_name(const char*         _path,
                        enum d_wiki_dialect _dialect);
/**
 * @brief Gives the name of the file that holds a page, in a GitHub wiki:
 *        "Page Name" in Page-Name.md, or Page-Name.mediawiki.
 *
 * @note A space, and each byte a file's name cannot hold -- `/`, `\`, `:`,
 *       `*`, `?`, `"`, `<`, `>`, `|` -- becomes `-`.
 *
 * @param[in] _name     the page's name.
 * @param[in] _dialect  the dialect, which gives the extension.
 * @post   The caller frees the name.
 * @return the file's name, allocated; `NULL` when memory runs out.
 */
char*  d_wiki_page_file(const char*         _name,
                        enum d_wiki_dialect _dialect);
// d_wiki_same_page is whether two names name one page: a GitHub wiki's
// compare in any case, with spaces and dashes alike; MediaWiki's with
// spaces, underscores and dashes alike -- a page kept in a file is named
// with dashes -- and the first letter in any case.
// d_wiki_page_key writes a name as names compare, so two that name one
// page are written alike.
bool   d_wiki_same_page(const char*         _left,
                        const char*         _right,
                        enum d_wiki_dialect _dialect);
bool   d_wiki_page_key(struct d_wiki_buffer* _out,
                       const char*           _name,
                       enum d_wiki_dialect   _dialect);
// d_wiki_anchor writes the anchor a title is linked by, before it is made
// unique on its page: GitHub's lower case, its punctuation dropped and its
// spaces dashes; MediaWiki's as its finalizeHeadings makes it, each run of
// white space -- Unicode's among it -- an underscore but for one at its
// end, and the marks that set a direction gone.
bool   d_wiki_anchor(struct d_wiki_buffer* _out,
                     enum d_wiki_dialect   _dialect,
                     const char*           _title);
// d_wiki_anchors writes the anchors headings of titles, one to a line, are
// given on a page that has them in that order: each made unique among
// those before it, as the dialect makes them -- what a table of contents
// written before its page links to.
bool   d_wiki_anchors(struct d_wiki_buffer* _out,
                      enum d_wiki_dialect   _dialect,
                      const char*           _titles);

// 2.5    Text
//------------------------------------------------------------------------------
// d_wiki_buffer_add and its kin write to a buffer, and say whether it holds
// all that has been written; d_wiki_buffer_take gives its text, which the
// caller frees, leaving it empty; d_wiki_buffer_free frees it.
bool   d_wiki_buffer_add(struct d_wiki_buffer* _buffer,
                         const char*           _text,
                         size_t                _length);
bool   d_wiki_buffer_add_text(struct d_wiki_buffer* _buffer,
                              const char*           _text);
bool   d_wiki_buffer_add_char(struct d_wiki_buffer* _buffer,
                              char                  _byte);
char*  d_wiki_buffer_take(struct d_wiki_buffer* _buffer);
void   d_wiki_buffer_free(struct d_wiki_buffer* _buffer);
// d_wiki_copy copies text of a length, terminated, for the caller to free;
// NULL when memory runs out.
char*  d_wiki_copy(const char* _text,
                   size_t      _length);

// 2.6    Writing
//------------------------------------------------------------------------------
// Each writes, in a dialect, what a page holds, as that dialect's reader
// reads it back:
//   d_wiki_write_heading     a heading's line, at a level from 1
//   d_wiki_write_page_link   a link to a page, perhaps at an anchor, perhaps
//                            showing a label: `_anchor` and `_label` may be
//                            NULL; a page "" links to an anchor of this one
//   d_wiki_write_url_link    a link to a URL, perhaps showing a label
//   d_wiki_write_image       an image, shown in place, with its text
//   d_wiki_write_code        a block of code, as it is, perhaps naming its
//                            language; `_language` may be NULL
//   d_wiki_write_inline_code code within a line
//   d_wiki_write_item        a list's item, at a depth from 1
//   d_wiki_write_table       a table: its rows a line each, a row's cells
//                            apart by tabs, the first row its header
//   d_wiki_write_escape      text, to be read as itself
//   d_wiki_write_comment     a comment
//   d_wiki_write_region      a region a program manages, its markers about
//                            the text
// Each line written ends with a line feed, and inline text with none.
// Text given -- a title, a label, an item's, a cell's -- is the dialect's
// markup, written as it is but for what would break what holds it: a
// label's unbalanced brackets, a line ending within a line. Text that must
// read as itself is escaped first, by d_wiki_write_escape.
bool   d_wiki_write_heading(struct d_wiki_buffer* _out,
                            enum d_wiki_dialect   _dialect,
                            size_t                _level,
                            const char*           _title);
bool   d_wiki_write_page_link(struct d_wiki_buffer* _out,
                              enum d_wiki_dialect   _dialect,
                              const char*           _page,
                              const char*           _anchor,
                              const char*           _label);
bool   d_wiki_write_url_link(struct d_wiki_buffer* _out,
                             enum d_wiki_dialect   _dialect,
                             const char*           _url,
                             const char*           _label);
bool   d_wiki_write_image(struct d_wiki_buffer* _out,
                          enum d_wiki_dialect   _dialect,
                          const char*           _source,
                          const char*           _text);
bool   d_wiki_write_code(struct d_wiki_buffer* _out,
                         enum d_wiki_dialect   _dialect,
                         const char*           _code,
                         const char*           _language);
bool   d_wiki_write_inline_code(struct d_wiki_buffer* _out,
                                enum d_wiki_dialect   _dialect,
                                const char*           _code);
bool   d_wiki_write_item(struct d_wiki_buffer* _out,
                         enum d_wiki_dialect   _dialect,
                         const char*           _text,
                         size_t                _depth,
                         bool                  _ordered);
bool   d_wiki_write_table(struct d_wiki_buffer* _out,
                          enum d_wiki_dialect   _dialect,
                          const char*           _rows);
bool   d_wiki_write_escape(struct d_wiki_buffer* _out,
                           enum d_wiki_dialect   _dialect,
                           const char*           _text);
bool   d_wiki_write_comment(struct d_wiki_buffer* _out,
                            enum d_wiki_dialect   _dialect,
                            const char*           _text);
bool   d_wiki_write_region(struct d_wiki_buffer* _out,
                           enum d_wiki_dialect   _dialect,
                           const char*           _name,
                           const char*           _text);
/**
 * @brief Writes a page's table of contents: a list of links to its
 *        headings' anchors, each at a depth its level gives.
 *
 * @param[out] _out    receives the list, a line to each heading.
 * @param[in]  _wiki   the page.
 * @param[in]  _least  the shallowest level listed, from 1.
 * @param[in]  _most   the deepest level listed.
 * @return `true` if the buffer holds all of it.
 */
bool   d_wiki_write_toc(struct d_wiki_buffer* _out,
                        const struct d_wiki*  _wiki,
                        size_t                _least,
                        size_t                _most);


#endif  // DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_H
