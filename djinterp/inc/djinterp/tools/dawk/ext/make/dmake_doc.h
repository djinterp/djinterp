/*******************************************************************************
* djinterp [tools]                                                   dmake_doc.h
*
* A makefile held for change.
*   A document is a file's text, the make it is read as, and its records, read
* by dmake. A change is made at once: the text is spliced, the result read
* again, and the change kept only if everything outside the splice reads as
* it did -- every statement before it where it was, each of its parts, its
* depth among the conditionals and the branch after it, every one after it
* the same but moved by the splice, every comment likewise -- and the change
* itself reads back as asked: a value holds the text it was given, a word
* the text in its place, a command the text under its rule, statements
* inserted the statements they were written as. A change that would do more
* is refused, and the file left as it was. Every byte a change does not
* touch is the file's own; text a change writes takes the file's line
* endings.
*   The changes a file takes: a statement's text set -- an assignment's
* value, a rule's prerequisites, a command, a directive's argument, a loop's
* values -- its name, its operator, or its body; a word of a rule's targets,
* prerequisites or order-only prerequisites set, inserted or removed, the
* `|` written when the first order-only one is, and taken when the last
* goes; a command set, added under its rule, or removed; statements
* inserted, replaced and removed. Each change takes the layout about it: a
* word a space from its neighbours, a command with its rule's prefix.
*   Indices are into the records as they are now, from 0: a change that adds
* or removes records moves those after it.
*   A document is made from a file, or from a text not yet on disk, and saved
* to its own path or another; saving writes only a file that would change,
* beside its target and renamed over it, and makes the directories it needs.
*
* path:      /inc/djinterp/tools/dawk/ext/make/dmake_doc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Documents
         1.  d_make_doc
         2.  d_make_saved
         3.  d_make_part
2.  OPERATIONS
    ----------
    1.  Documents
    2.  Changes
    3.  Parts
    4.  Words
    5.  Commands
    6.  Statements
    7.  Saving
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_DOC_H
#define DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_DOC_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "./dmake.h"  // d_make, d_make_dialect, d_make_problem


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_make_doc
//   struct: one file's text, its make, its records, and why the last change
//   failed.  Layout is private.
struct d_make_doc;

// 1.1.2
// d_make_saved
//   enum: what saving did.
enum d_make_saved
{
    D_MAKE_SAVE_FAILED = 0,
    D_MAKE_SAVE_WRITTEN,
    D_MAKE_SAVE_UNCHANGED   // the file holds the text already
};

// 1.1.3
// d_make_part
//   enum: a part of a statement a change names.
enum d_make_part
{
    D_MAKE_PART_TEXT = 0,    // the statement's text, as dmake.h has it
    D_MAKE_PART_NAME,        // an assignment's, a define's, an undefine's
    D_MAKE_PART_SEPARATOR,   // an assignment's operator
    D_MAKE_PART_BODY,        // a define's or a loop's lines
    D_MAKE_PART_TARGETS,     // a rule's targets
    D_MAKE_PART_ORDER_ONLY   // a rule's order-only prerequisites
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
/**
 * @brief Reads a makefile.
 *
 * @param[in]  _path     the file.
 * @param[in]  _dialect  the make it is read as.
 * @param[out] _problem  receives why it could not be read, its make's own
 *                       refusals among them, with their lines; may be
 *                       `NULL`.
 * @post   The caller frees the document with d_make_doc_free.
 * @return the document, or `NULL` when the file cannot be read, does not
 *         read as its make reads one, or memory runs out.
 */
struct d_make_doc*   d_make_doc_read(const char*            _path,
                                     enum d_make_dialect    _dialect,
                                     struct d_make_problem* _problem);
/**
 * @brief Makes a document of a text, to be saved to a path.
 *
 * @param[in]  _path     where it is to be saved.
 * @param[in]  _text     the text.
 * @param[in]  _length   its length in bytes.
 * @param[in]  _dialect  the make it is read as.
 * @param[out] _problem  receives why it does not read; may be `NULL`.
 * @post   The caller frees the document with d_make_doc_free.
 * @return the document, or `NULL` when the text does not read as its make
 *         reads one, or memory runs out.
 */
struct d_make_doc*   d_make_doc_make(const char*            _path,
                                     const char*            _text,
                                     size_t                 _length,
                                     enum d_make_dialect    _dialect,
                                     struct d_make_problem* _problem);
void                 d_make_doc_free(struct d_make_doc* _doc);
// What a document holds: its records, as of its last change; its path, how
// many changes it has taken, why the last change failed, and its text.
// d_make_doc_changed says where the last change kept was: the text before
// it from `start` to `old_end` is the text now from `start` to `new_end`,
// and all else is as it was; false before any change.
const struct d_make* d_make_doc_records(const struct d_make_doc* _doc);
const char*          d_make_doc_path(const struct d_make_doc* _doc);
size_t               d_make_doc_changes(const struct d_make_doc* _doc);
bool                 d_make_doc_changed(const struct d_make_doc* _doc,
                                        size_t*                  _out_start,
                                        size_t* _out_old_end,
                                        size_t* _out_new_end);
const char*          d_make_doc_problem(const struct d_make_doc* _doc);
const char*          d_make_doc_text(const struct d_make_doc* _doc,
                                     size_t*                  _out_length);

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
 *         d_make_doc_problem saying why, and the file as it was.
 */
bool                 d_make_doc_splice(struct d_make_doc* _doc,
                                       size_t             _start,
                                       size_t             _end,
                                       const char*        _text,
                                       size_t             _length);

// 2.3    Parts
//------------------------------------------------------------------------------
// A part's text is as it is written in the file, its references and
// escapes and all; a body's is its lines, a line feed ending the last.
//   d_make_doc_set  statement `_statement`'s part takes the text: its text,
//                   its name, its operator or its body
// It gives `true` if the change was kept, and `false` otherwise,
// d_make_doc_problem saying why: a part the statement has not, an operator
// its make does not read, text that would read as more or other than the
// part.
bool                 d_make_doc_set(struct d_make_doc* _doc,
                                    size_t             _statement,
                                    enum d_make_part   _part,
                                    const char*        _text);

// 2.4    Words
//------------------------------------------------------------------------------
// A rule's targets, its prerequisites -- D_MAKE_PART_TEXT -- and its
// order-only ones are words, as d_make_words splits them, numbered from 0.
//   d_make_doc_set_word     word `_word` of the part takes the text
//   d_make_doc_insert_word  a word is written as the `_place`th of the
//                           part, a space from its neighbours; one past
//                           the last appends, and a first order-only one
//                           is written after a `|`, in GNU's make alone
//   d_make_doc_remove_word  word `_word` goes, and the blanks before it,
//                           the `|` with the last order-only one
// Each gives `true` if the change was kept, and `false` otherwise,
// d_make_doc_problem saying why: a rule left with no target is refused.
bool                 d_make_doc_set_word(struct d_make_doc* _doc,
                                         size_t             _statement,
                                         enum d_make_part   _part,
                                         size_t             _word,
                                         const char*        _text);
bool                 d_make_doc_insert_word(struct d_make_doc* _doc,
                                            size_t             _statement,
                                            enum d_make_part   _part,
                                            size_t             _place,
                                            const char*        _text);
bool                 d_make_doc_remove_word(struct d_make_doc* _doc,
                                            size_t             _statement,
                                            enum d_make_part   _part,
                                            size_t             _word);

// 2.5    Commands
//------------------------------------------------------------------------------
// A rule's commands are the recipe's lines that follow it, numbered from 0.
//   d_make_doc_add_command     a line is written as the `_place`th of rule
//                              `_rule`'s commands, led by the prefix its
//                              others have, a tab for the first; one past
//                              the last appends
//   d_make_doc_remove_command  recipe line `_statement` goes, with its line
// Each gives `true` if the change was kept, and `false` otherwise,
// d_make_doc_problem saying why. A command's text is set with d_make_doc_set.
bool                 d_make_doc_add_command(struct d_make_doc* _doc,
                                            size_t             _rule,
                                            size_t             _place,
                                            const char*        _text);
bool                 d_make_doc_remove_command(struct d_make_doc* _doc,
                                               size_t             _statement);

// 2.6    Statements
//------------------------------------------------------------------------------
// A statement's text is one or more whole statements, as they are written,
// with comments among them if wished; a line feed is added to end it.
//   d_make_doc_insert_statements  before statement `_before`, and above the
//                                 comment lines that lead into it, or after
//                                 the last at `_before` the count
//   d_make_doc_set_statement      a statement replaced by one other of its
//                                 kind
//   d_make_doc_remove_statements  statements `_first` to `_last`, their
//                                 lines and the comments among them
// Each gives `true` if the change was kept, and `false` otherwise,
// d_make_doc_problem saying why.
bool                 d_make_doc_insert_statements(struct d_make_doc* _doc,
                                                  size_t      _before,
                                                  const char* _text);
bool                 d_make_doc_set_statement(struct d_make_doc* _doc,
                                              size_t             _statement,
                                              const char*        _text);
bool                 d_make_doc_remove_statements(struct d_make_doc* _doc,
                                                  size_t             _first,
                                                  size_t             _last);

// 2.7    Saving
//------------------------------------------------------------------------------
/**
 * @brief Writes the file, if it does not hold the text already.
 *
 * @param[in,out] _doc      the document.
 * @param[in]     _path     where; `NULL` or "" for its own path.
 * @param[out]    _problem  receives why it could not be written; may be
 *                          `NULL`.
 * @return what saving did.
 */
enum d_make_saved    d_make_doc_save(struct d_make_doc*     _doc,
                                     const char*            _path,
                                     struct d_make_problem* _problem);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_DOC_H
