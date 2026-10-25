/*******************************************************************************
* djinterp [tools]                                                   dwiki_doc.h
*
* A wiki's page held for change.
*   A document is a page's text and its records, read by dwiki. A change is
* made at once: the text is spliced, the result read again, and the change
* kept only if everything outside the splice reads as it did -- every block,
* heading, link, template and region before it where it was, every one after
* it where it was but moved by the splice -- and the change itself reads
* back as asked: a link leads where it was told to, a section has the
* heading it was given, a region holds the text it was given. A change that
* would do more is refused, and the page left as it was. Every byte a change
* does not touch is the page's own, its line endings and byte-order mark
* among them; text a change writes takes the page's line endings.
*   The changes a page takes: a link's destination or label set, or the link
* undone to its text; a section's body set, the whole of it replaced, its
* heading retitled, a section inserted or removed; a region's text set, a
* region added or removed; a template's argument set or removed. A link
* that is a reference is changed in its definition, which every link using
* it shares.
*   Indices are into the records as they are now: a change that adds or
* removes records moves those after it.
*   A document is made from a file, or from a text not yet on disk, and saved
* to its own path or another; saving writes only a file that would change,
* beside its target and renamed over it, and makes the directories it needs.
*
* path:      /inc/djinterp/tools/dawk/ext/wiki/dwiki_doc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Documents
         1.  d_wiki_doc
         2.  d_wiki_saved
2.  OPERATIONS
    ----------
    1.  Documents
    2.  Changes
    3.  Links
    4.  Sections
    5.  Regions
    6.  Templates
    7.  Saving
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_DOC_H
#define DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_DOC_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "./dwiki.h"  // d_wiki


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_wiki_doc
//   struct: one page's text, its records, and why the last change failed.
//   Layout is private.
struct d_wiki_doc;

// 1.1.2
// d_wiki_saved
//   enum: what saving did.
enum d_wiki_saved
{
    D_WIKI_SAVE_FAILED = 0,
    D_WIKI_SAVE_WRITTEN,
    D_WIKI_SAVE_UNCHANGED   // the file holds the text already
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
/**
 * @brief Reads a page from a file.
 *
 * @param[in]  _path      the file.
 * @param[in]  _dialect   what it is written in; D_WIKI_OTHER takes it from
 *                        the file's name.
 * @param[out] _problem   receives why it could not be read.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_wiki_doc_free.
 * @return the document, or `NULL` when the file cannot be read, its
 *         dialect is not known, or memory runs out.
 */
struct d_wiki_doc*   d_wiki_doc_read(const char*         _path,
                                     enum d_wiki_dialect _dialect,
                                     char*               _problem,
                                     size_t              _capacity);
// d_wiki_doc_make makes a document of a text, to be saved to `_path`;
// the dialect, D_WIKI_OTHER, is the path's.
struct d_wiki_doc*   d_wiki_doc_make(const char*         _path,
                                     const char*         _text,
                                     size_t              _length,
                                     enum d_wiki_dialect _dialect,
                                     char*               _problem,
                                     size_t              _capacity);
void                 d_wiki_doc_free(struct d_wiki_doc* _doc);
// What a document holds: its records, as of its last change; its path, its
// dialect, how many changes it has taken, why the last change failed, and
// its text. d_wiki_doc_changed says where the last change kept was: the
// text before it from `start` to `old_end` is the text now from `start` to
// `new_end`, and all else is as it was; false before any change.
const struct d_wiki* d_wiki_doc_records(const struct d_wiki_doc* _doc);
const char*          d_wiki_doc_path(const struct d_wiki_doc* _doc);
enum d_wiki_dialect  d_wiki_doc_dialect(const struct d_wiki_doc* _doc);
size_t               d_wiki_doc_changes(const struct d_wiki_doc* _doc);
bool                 d_wiki_doc_changed(const struct d_wiki_doc* _doc,
                                        size_t*                  _out_start,
                                        size_t*                  _out_old_end,
                                        size_t*                  _out_new_end);
const char*          d_wiki_doc_problem(const struct d_wiki_doc* _doc);
const char*          d_wiki_doc_text(const struct d_wiki_doc* _doc,
                                     size_t*                  _out_length);

// 2.2    Changes
//------------------------------------------------------------------------------
/**
 * @brief Replaces bytes of the page, kept only if the page outside them
 *        reads as it did.
 *
 * @param[in,out] _doc     the document.
 * @param[in]     _start   the first byte replaced.
 * @param[in]     _end     past the last; `_start` inserts.
 * @param[in]     _text    what replaces them, its line feeds written as the
 *                         page ends its lines.
 * @param[in]     _length  its length.
 * @return `true` if the change was kept; `false` otherwise, with
 *         d_wiki_doc_problem saying why, and the page as it was.
 */
bool                 d_wiki_doc_splice(struct d_wiki_doc* _doc,
                                       size_t             _start,
                                       size_t             _end,
                                       const char*        _text,
                                       size_t             _length);

// 2.3    Links
//------------------------------------------------------------------------------
// d_wiki_doc_set_link gives a link a destination, `_target`, and a label,
// `_label`, either `NULL` to keep it: written in the link's own form, the
// destination as the dialect writes one and the label as markup.  A label
// "" leaves Gollum's or MediaWiki's [[...]] showing its page.  A link that
// shows its destination -- <https://...>, a bare one -- has no label to
// set.  d_wiki_doc_unlink replaces a link with the text it shows; an image
// or a category it removes.
bool                 d_wiki_doc_set_link(struct d_wiki_doc* _doc,
                                         size_t             _link,
                                         const char*        _target,
                                         const char*        _label);
bool                 d_wiki_doc_unlink(struct d_wiki_doc* _doc,
                                       size_t             _link);

// 2.4    Sections
//------------------------------------------------------------------------------
// Sections are counted as the records count them, the lead 0:
//   d_wiki_doc_set_body       a section's own text, past its heading to its
//                             first subsection, which may hold no heading
//   d_wiki_doc_set_section    all of a section, its heading and its
//                             subsections, which must begin with a heading
//   d_wiki_doc_rename_section a heading's title, as markup, its level kept
//   d_wiki_doc_insert_section a section before another, or after the last
//                             at `_before` past them; its body may be NULL
//   d_wiki_doc_remove_section a section and its subsections; the lead's
//                             body only
// A body or a section is written with a blank line before the heading that
// follows it.
bool                 d_wiki_doc_set_body(struct d_wiki_doc* _doc,
                                         size_t             _section,
                                         const char*        _text);
bool                 d_wiki_doc_set_section(struct d_wiki_doc* _doc,
                                            size_t             _section,
                                            const char*        _text);
bool                 d_wiki_doc_rename_section(struct d_wiki_doc* _doc,
                                               size_t             _section,
                                               const char*        _title);
bool                 d_wiki_doc_insert_section(struct d_wiki_doc* _doc,
                                               size_t             _before,
                                               size_t             _level,
                                               const char*        _title,
                                               const char*        _body);
bool                 d_wiki_doc_remove_section(struct d_wiki_doc* _doc,
                                               size_t             _section);

// 2.5    Regions
//------------------------------------------------------------------------------
// d_wiki_doc_set_region sets the text between a region's markers;
// d_wiki_doc_add_region adds a region of a name no other has, at the end of
// a section's own text, or of the page for D_WIKI_NONE;
// d_wiki_doc_remove_region removes a region's markers, and its text unless
// `_keep` says to keep it.
bool                 d_wiki_doc_set_region(struct d_wiki_doc* _doc,
                                           const char*        _name,
                                           const char*        _text);
bool                 d_wiki_doc_add_region(struct d_wiki_doc* _doc,
                                           const char*        _name,
                                           const char*        _text,
                                           size_t             _section);
bool                 d_wiki_doc_remove_region(struct d_wiki_doc* _doc,
                                              const char*        _name,
                                              bool               _keep);

// 2.6    Templates
//------------------------------------------------------------------------------
// d_wiki_doc_set_argument sets a template's argument of a key -- the last of
// it, as MediaWiki reads the last -- keeping the space about a named one's
// value, or adds it before the template's close, numbered when its key is
// the next number; d_wiki_doc_remove_argument removes it, and its `|`.
bool                 d_wiki_doc_set_argument(struct d_wiki_doc* _doc,
                                             size_t             _template,
                                             const char*        _key,
                                             const char*        _value);
bool                 d_wiki_doc_remove_argument(struct d_wiki_doc* _doc,
                                                size_t             _template,
                                                const char*        _key);

// 2.7    Saving
//------------------------------------------------------------------------------
/**
 * @brief Writes the page, if the file does not hold it already.
 *
 * @param[in,out] _doc       the document.
 * @param[in]     _path      where; `NULL` for its own path.
 * @param[out]    _problem   receives why it could not be written.
 * @param[in]     _capacity  the size of `_problem`.
 * @return what saving did.
 */
enum d_wiki_saved    d_wiki_doc_save(struct d_wiki_doc* _doc,
                                     const char*        _path,
                                     char*              _problem,
                                     size_t             _capacity);


#endif  // DJINTERP_TOOLS_DAWK_EXT_WIKI_DWIKI_DOC_H
