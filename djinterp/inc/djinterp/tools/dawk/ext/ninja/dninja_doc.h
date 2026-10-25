/*******************************************************************************
* djinterp [tools]                                                  dninja_doc.h
*
* A Ninja file held for change.
*   A document is a file's text and its records, read by dninja. A change is
* made at once: the text is spliced, the result read again, and the change
* kept only if everything outside the splice reads as it did -- every
* statement before it where it was, its name, its paths in their sections
* and its bindings, every one after it the same but moved by the splice,
* every comment likewise -- and the change itself reads back as asked: a
* value holds the text it was given, a path the text and the section, a
* statement inserted the statements it was written as. A change that would
* do more is refused, and the file left as it was. Every byte a change does
* not touch is the file's own; text a change writes takes the file's line
* endings.
*   The changes a file takes: a variable's value set; a name set -- a
* variable's, a rule's, a pool's, the rule a build uses; a binding set,
* added or removed; a path set, inserted into its section, the section's
* pipe written when it has none, or removed, its pipe with it when it was
* the last; statements inserted, replaced and removed. Each change takes
* the layout about it: a binding indented as its statement's are, a path a
* space from its neighbour.
*   Indices are into the records as they are now, from 0: a change that adds
* or removes records moves those after it.
*   A document is made from a file, or from a text not yet on disk, and saved
* to its own path or another; saving writes only a file that would change,
* beside its target and renamed over it, and makes the directories it needs.
*
* path:      /inc/djinterp/tools/dawk/ext/ninja/dninja_doc.h
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
         1.  d_ninja_doc
         2.  d_ninja_saved
2.  OPERATIONS
    ----------
    1.  Documents
    2.  Changes
    3.  Values and names
    4.  Bindings
    5.  Paths
    6.  Statements
    7.  Saving
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_DOC_H
#define DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_DOC_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
// djinterp
#include "./dninja.h"  // d_ninja


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_ninja_doc
//   struct: one file's text, its records, and why the last change failed.
//   Layout is private.
struct d_ninja_doc;

// 1.1.2
// d_ninja_saved
//   enum: what saving did.
enum d_ninja_saved
{
    D_NINJA_SAVE_FAILED = 0,
    D_NINJA_SAVE_WRITTEN,
    D_NINJA_SAVE_UNCHANGED   // the file holds the text already
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
/**
 * @brief Reads a Ninja file.
 *
 * @param[in]  _path      the file.
 * @param[out] _problem   receives why it could not be read, ninja's own
 *                        refusals among them, with their lines.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_ninja_doc_free.
 * @return the document, or `NULL` when the file cannot be read, does not
 *         read as Ninja, or memory runs out.
 */
struct d_ninja_doc*   d_ninja_doc_read(const char* _path,
                                       char*       _problem,
                                       size_t      _capacity);
// d_ninja_doc_make makes a document of a text, to be saved to `_path`; the
// caller frees it with d_ninja_doc_free, and it is `NULL` when the text does
// not read as Ninja, `_problem` saying why.
struct d_ninja_doc*   d_ninja_doc_make(const char* _path,
                                       const char* _text,
                                       size_t      _length,
                                       char*       _problem,
                                       size_t      _capacity);
void                  d_ninja_doc_free(struct d_ninja_doc* _doc);
// What a document holds: its records, as of its last change; its path, how
// many changes it has taken, why the last change failed, and its text.
// d_ninja_doc_changed says where the last change kept was: the text before
// it from `start` to `old_end` is the text now from `start` to `new_end`,
// and all else is as it was; false before any change.
const struct d_ninja* d_ninja_doc_records(const struct d_ninja_doc* _doc);
const char*           d_ninja_doc_path(const struct d_ninja_doc* _doc);
size_t                d_ninja_doc_changes(const struct d_ninja_doc* _doc);
bool                  d_ninja_doc_changed(const struct d_ninja_doc* _doc,
                                          size_t* _out_start,
                                          size_t* _out_old_end,
                                          size_t* _out_new_end);
const char*           d_ninja_doc_problem(const struct d_ninja_doc* _doc);
const char*           d_ninja_doc_text(const struct d_ninja_doc* _doc,
                                       size_t*                   _out_length);

// 2.2    Changes
//------------------------------------------------------------------------------
/**
 * @brief Replaces bytes of the file, kept only if the file outside them
 *        reads as it did.
 *
 * @param[in,out] _doc     the document.
 * @param[in]     _start   the first byte replaced.
 * @param[in]     _end     past the last; `_start` inserts.
 * @param[in]     _text    what replaces them, its line feeds written as the
 *                         file ends its lines.
 * @param[in]     _length  its length.
 * @return `true` if the change was kept; `false` otherwise, with
 *         d_ninja_doc_problem saying why, and the file as it was.
 */
bool                  d_ninja_doc_splice(struct d_ninja_doc* _doc,
                                         size_t              _start,
                                         size_t              _end,
                                         const char*         _text,
                                         size_t              _length);

// 2.3    Values and names
//------------------------------------------------------------------------------
// A value's text is as it is written in the file, its `$` escapes and all,
// to its line's end; d_ninja_escape writes one.
//   d_ninja_doc_set_value  a variable's value
//   d_ninja_doc_set_name   a variable's, a rule's or a pool's name, or the
//                          rule a build uses
// Each gives `true` if the change was kept, and `false` otherwise,
// d_ninja_doc_problem saying why.
bool                  d_ninja_doc_set_value(struct d_ninja_doc* _doc,
                                            size_t              _statement,
                                            const char*         _text);
bool                  d_ninja_doc_set_name(struct d_ninja_doc* _doc,
                                           size_t              _statement,
                                           const char*         _name);

// 2.4    Bindings
//------------------------------------------------------------------------------
//   d_ninja_doc_set_binding     the last binding of a name of a rule, a
//                               build or a pool takes the value, or one is
//                               added after its last, indented as its
//                               bindings are, two spaces for the first
//   d_ninja_doc_remove_binding  every binding of the name goes, each with
//                               its line
// Each gives `true` if the change was kept, and `false` otherwise,
// d_ninja_doc_problem saying why: a rule or a pool refuses a binding ninja
// would refuse of it.
bool                  d_ninja_doc_set_binding(struct d_ninja_doc* _doc,
                                              size_t              _statement,
                                              const char*         _name,
                                              const char*         _text);
bool                  d_ninja_doc_remove_binding(struct d_ninja_doc* _doc,
                                                 size_t      _statement,
                                                 const char* _name);

// 2.5    Paths
//------------------------------------------------------------------------------
// A path's text is as it is written, its `$` escapes and all; paths are
// numbered across the statement in the order written, from 0.
//   d_ninja_doc_set_path     path `_path` takes the text
//   d_ninja_doc_insert_path  a path is written as the `_place`th of its
//                            section, a space from its neighbours; one past
//                            the section's last appends to it, and a
//                            section a build has none of is written with
//                            its pipe where ninja reads it
//   d_ninja_doc_remove_path  path `_path` goes, and the space before it,
//                            and its section's pipe when it was the last
// Each gives `true` if the change was kept, and `false` otherwise,
// d_ninja_doc_problem saying why: a build left with no output, or a
// default with no target, is refused, as ninja refuses one.
bool                  d_ninja_doc_set_path(struct d_ninja_doc* _doc,
                                           size_t              _statement,
                                           size_t              _path,
                                           const char*         _text);
bool                  d_ninja_doc_insert_path(struct d_ninja_doc* _doc,
                                              size_t _statement,
                                              enum d_ninja_section _section,
                                              size_t               _place,
                                              const char*          _text);
bool                  d_ninja_doc_remove_path(struct d_ninja_doc* _doc,
                                              size_t _statement,
                                              size_t _path);

// 2.6    Statements
//------------------------------------------------------------------------------
// A statement's text is one or more whole statements, as they are written,
// with comments among them if wished.
//   d_ninja_doc_insert_statements  before statement `_before`, and above
//                                  the comments that lead into it, or
//                                  after the last at `_before` the count
//   d_ninja_doc_set_statement      a statement replaced by one other of its
//                                  kind
//   d_ninja_doc_remove_statements  statements `_first` to `_last`, their
//                                  lines and the comments among them
// Each gives `true` if the change was kept, and `false` otherwise,
// d_ninja_doc_problem saying why.
bool                  d_ninja_doc_insert_statements(struct d_ninja_doc* _doc,
                                                    size_t      _before,
                                                    const char* _text);
bool                  d_ninja_doc_set_statement(struct d_ninja_doc* _doc,
                                                size_t              _statement,
                                                const char*         _text);
bool                  d_ninja_doc_remove_statements(struct d_ninja_doc* _doc,
                                                    size_t _first,
                                                    size_t _last);

// 2.7    Saving
//------------------------------------------------------------------------------
/**
 * @brief Writes the file, if it does not hold the text already.
 *
 * @param[in,out] _doc       the document.
 * @param[in]     _path      where; `NULL` or "" for its own path.
 * @param[out]    _problem   receives why it could not be written.
 * @param[in]     _capacity  the size of `_problem`.
 * @return what saving did.
 */
enum d_ninja_saved    d_ninja_doc_save(struct d_ninja_doc* _doc,
                                       const char*         _path,
                                       char*               _problem,
                                       size_t              _capacity);


#endif  // DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_DOC_H
