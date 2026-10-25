/*******************************************************************************
* djinterp [tools]                                                    dgit_doc.h
*
* A git text file held for change.
*   A document is the text of an ignore file, an attribute file or a file of
* git's configuration syntax, and its records, read by dgit_text. A change is
* made at once: the text is spliced, the result read again, and the change
* kept only if everything outside the splice reads as it did -- every
* pattern, state, section and entry before it where it was, every one after
* it where it was but moved by the splice, each read as before: an entry
* after a new section header still in its own section -- and the change
* itself reads back as asked: a key has the value it was given, a pattern
* the text, a section the name. A change that would do more is refused, and
* the file left as it was. A change of several splices is kept whole or not
* at all. Every byte a change does not touch is the file's own, its line
* endings and byte-order mark among them; text a change writes takes the
* file's line endings.
*   The changes a file takes: a line inserted, removed or replaced -- a
* pattern, a comment; an attribute's state set on a pattern's line, or
* taken off; and in a configuration what git config does: a key set or
* added, a value given to one entry, a key unset, a section renamed or
* removed. Text written for a configuration is written as git config writes
* it: an entry "\tname = value" after the last entry of the last section it
* belongs in, a new section at the end; a section left with no entry, and no
* comment within or about it, goes with its last.
*   A document is made from a file, or from a text not yet on disk, and saved
* to its own path or another; saving writes only a file that would change,
* beside its target and renamed over it, and makes the directories it needs.
*
* path:      /inc/djinterp/tools/dawk/ext/git/dgit_doc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Documents
         1.  d_git_doc
         2.  d_git_saved
2.  OPERATIONS
    ----------
    1.  Documents
    2.  Changes
    3.  Lines
    4.  Attributes
    5.  Configuration
    6.  Saving
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_DOC_H
#define DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_DOC_H 1

// std
#include <stdbool.h>      // bool
#include <stddef.h>       // size_t
// djinterp
#include "./dgit_text.h"  // d_git_text, d_git_text_kind, d_git_text_state


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_git_doc
//   struct: one file's text, its records, and why the last change failed.
//   Layout is private.
struct d_git_doc;

// 1.1.2
// d_git_saved
//   enum: what saving did.
enum d_git_saved
{
    D_GIT_SAVE_FAILED = 0,
    D_GIT_SAVE_UNCHANGED,   // the file there holds the text already
    D_GIT_SAVE_WRITTEN
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
/**
 * @brief Reads a file into a document.
 *
 * @param[in]  _path      the file.
 * @param[in]  _kind      what it is; D_GIT_TEXT_OTHER for what its name
 *                        says.
 * @param[out] _problem   receives why reading failed.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_git_doc_free.
 * @return the document; `NULL` when the file cannot be read, its kind is
 *         none git reads, or memory ran out.
 */
struct d_git_doc*        d_git_doc_read(const char*          _path,
                                        enum d_git_text_kind _kind,
                                        char*                _problem,
                                        size_t               _capacity);
/**
 * @brief Makes a document of a text not yet on disk.
 *
 * @param[in]  _path      where it is to be saved; may be empty.
 * @param[in]  _kind      what it is; D_GIT_TEXT_OTHER for what the path's
 *                        name says.
 * @param[in]  _text      its text, copied; `NULL` for none.
 * @param[in]  _length    the text's length.
 * @param[out] _problem   receives why making it failed.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_git_doc_free.
 * @return the document; `NULL` for a kind git reads none of, or when
 *         memory ran out.
 */
struct d_git_doc*        d_git_doc_make(const char*          _path,
                                        enum d_git_text_kind _kind,
                                        const char*          _text,
                                        size_t               _length,
                                        char*                _problem,
                                        size_t               _capacity);
void                     d_git_doc_free(struct d_git_doc* _doc);
// the document's records, path, kind, and changes kept; whether its text
// differs from the one it was made from; why the last change failed, ""
// when it did not; and its text, `_out_length` bytes, which may be NULL.
// The records and text are the document's until its next change.
const struct d_git_text* d_git_doc_records(const struct d_git_doc* _doc);
const char*              d_git_doc_path(const struct d_git_doc* _doc);
enum d_git_text_kind     d_git_doc_kind(const struct d_git_doc* _doc);
size_t                   d_git_doc_changes(const struct d_git_doc* _doc);
bool                     d_git_doc_changed(const struct d_git_doc* _doc);
const char*              d_git_doc_problem(const struct d_git_doc* _doc);
const char*              d_git_doc_text(const struct d_git_doc* _doc,
                                        size_t*                 _out_length);

// 2.2    Changes
//------------------------------------------------------------------------------
/**
 * @brief Replaces the text from `_start` to `_end` with other text, kept
 *        only if everything outside it reads as it did.
 *
 * @param[in,out] _doc     the document.
 * @param[in]     _start   where the replaced text begins.
 * @param[in]     _end     where it ends.
 * @param[in]     _text    what replaces it; `NULL` for nothing.
 * @param[in]     _length  its length.
 * @post   On success the records are read again, and every pointer into
 *         the old ones is invalid.
 * @return `true` if the change was kept; `false`, the document as it was
 *         and d_git_doc_problem saying why, otherwise.
 */
bool                     d_git_doc_splice(struct d_git_doc* _doc,
                                          size_t            _start,
                                          size_t            _end,
                                          const char*       _text,
                                          size_t            _length);

// 2.3    Lines
//------------------------------------------------------------------------------
// Each is kept as d_git_doc_splice keeps a change; each invalidates the
// records on success. A line's text is one line, with no ending: the
// document's is added.
// d_git_doc_insert_line puts a line before line `_before` -- or after the
// last for the line count -- and d_git_doc_set_line replaces a line's text,
// each giving the index of the line it made, D_GIT_TEXT_NONE when refused;
// d_git_doc_remove_line takes a line and its ending away.
size_t                   d_git_doc_insert_line(struct d_git_doc* _doc,
                                               size_t            _before,
                                               const char*       _text);
size_t                   d_git_doc_set_line(struct d_git_doc* _doc,
                                            size_t            _line,
                                            const char*       _text);
bool                     d_git_doc_remove_line(struct d_git_doc* _doc,
                                               size_t            _line);

// 2.4    Attributes
//------------------------------------------------------------------------------
// d_git_doc_set_state writes an attribute's state on a pattern's line --
// replacing the state of that name the line ends with, or after the line's
// last -- for a name git allows and a value with no blank;
// d_git_doc_remove_state takes every state of the name off the line, the
// blank before each with it. Each is kept as d_git_doc_splice keeps a
// change, and invalidates the records on success.
bool                     d_git_doc_set_state(struct d_git_doc*     _doc,
                                             size_t                _pattern,
                                             const char*           _name,
                                             enum d_git_text_state _state,
                                             const char*           _value);
bool                     d_git_doc_remove_state(struct d_git_doc* _doc,
                                                size_t            _pattern,
                                                const char*       _name);

// 2.5    Configuration
//------------------------------------------------------------------------------
// Keys are as git config takes them, in any case; a section and a name
// written new are written as given. Each is kept whole or not at all, as
// d_git_doc_splice keeps a change, and invalidates the records on success.
// d_git_doc_set gives a key a value: the one entry it has, or a new entry,
// as git config key value -- refused for a key of several entries;
// d_git_doc_add adds an entry whatever the key has, as git config --add;
// d_git_doc_set_entry gives one entry a value, NULL for its name alone;
// d_git_doc_unset takes a key's entry away -- every one with `_all`, else
// refused for several -- as git config --unset and --unset-all; and
// d_git_doc_remove_entry takes one entry away.
bool                     d_git_doc_set(struct d_git_doc* _doc,
                                       const char*       _key,
                                       const char*       _value);
bool                     d_git_doc_add(struct d_git_doc* _doc,
                                       const char*       _key,
                                       const char*       _value);
bool                     d_git_doc_set_entry(struct d_git_doc* _doc,
                                             size_t            _entry,
                                             const char*       _value);
bool                     d_git_doc_unset(struct d_git_doc* _doc,
                                         const char*       _key,
                                         bool              _all);
bool                     d_git_doc_remove_entry(struct d_git_doc* _doc,
                                                size_t            _entry);
// d_git_doc_rename_section gives every section of a name -- "section" or
// "section.subsection" -- another, as git config --rename-section, and
// d_git_doc_remove_section takes every one away, its entries and comments
// to the next section with it, as git config --remove-section; each is
// refused when there is no such section.
bool                     d_git_doc_rename_section(struct d_git_doc* _doc,
                                                  const char*       _name,
                                                  const char*       _to);
bool                     d_git_doc_remove_section(struct d_git_doc* _doc,
                                                  const char*       _name);

// 2.6    Saving
//------------------------------------------------------------------------------
/**
 * @brief Saves a document, unless the file there holds its text already.
 *
 * @param[in]  _doc       the document.
 * @param[in]  _path      where; `NULL` or empty for its own path.
 * @param[out] _problem   receives why saving failed.
 * @param[in]  _capacity  the size of `_problem`.
 * @return D_GIT_SAVE_WRITTEN, D_GIT_SAVE_UNCHANGED, or D_GIT_SAVE_FAILED.
 */
enum d_git_saved         d_git_doc_save(const struct d_git_doc* _doc,
                                        const char*             _path,
                                        char*                   _problem,
                                        size_t                  _capacity);


#endif  // DJINTERP_TOOLS_DAWK_EXT_GIT_DGIT_DOC_H
