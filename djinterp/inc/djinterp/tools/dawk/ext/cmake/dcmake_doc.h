/*******************************************************************************
* djinterp [tools]                                                  dcmake_doc.h
*
* A CMake file held for change.
*   A document is a file's text and its records, read by dcmake. A change is
* made at once: the text is spliced, the result read again, and the change
* kept only if everything outside the splice reads as it did -- every
* command before it where it was, its arguments, its block and the command
* it pairs with, every one after it the same but moved by the splice, every
* comment likewise -- and the change itself reads back as asked: an argument
* holds the text it was given, a command the name, a command inserted the
* arguments it was written with. A change that would do more is refused, and
* the file left as it was. Every byte a change does not touch is the file's
* own, its line endings and byte-order mark among them; text a change writes
* takes the file's line endings.
*   The changes a file takes: an argument set, inserted or removed; a
* command's name set, or the whole command replaced; commands inserted, and
* a run of them removed. Neither a command's name nor its replacement may
* change the part it takes in a block, and a nested parenthesis is never
* changed alone. Each change takes the layout about it: an argument on a line
* of its own where its neighbour stands on one, a command at the indentation
* of the commands of its block.
*   Indices are into the records as they are now, from 0: a change that adds
* or removes records moves those after it.
*   A document is made from a file, or from a text not yet on disk, and saved
* to its own path or another; saving writes only a file that would change,
* beside its target and renamed over it, and makes the directories it needs.
*
* path:      /inc/djinterp/tools/dawk/ext/cmake/dcmake_doc.h
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
         1.  d_cmake_doc
         2.  d_cmake_saved
2.  OPERATIONS
    ----------
    1.  Documents
    2.  Changes
    3.  Arguments
    4.  Commands
    5.  Saving
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_CMAKE_DCMAKE_DOC_H
#define DJINTERP_TOOLS_DAWK_EXT_CMAKE_DCMAKE_DOC_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
// djinterp
#include "./dcmake.h"  // d_cmake


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_cmake_doc
//   struct: one file's text, its records, and why the last change failed.
//   Layout is private.
struct d_cmake_doc;

// 1.1.2
// d_cmake_saved
//   enum: what saving did.
enum d_cmake_saved
{
    D_CMAKE_SAVE_FAILED = 0,
    D_CMAKE_SAVE_WRITTEN,
    D_CMAKE_SAVE_UNCHANGED   // the file holds the text already
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
/**
 * @brief Reads a CMake file.
 *
 * @param[in]  _path      the file.
 * @param[out] _problem   receives why it could not be read, CMake's own
 *                        refusals among them, with where.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_cmake_doc_free.
 * @return the document, or `NULL` when the file cannot be read, does not
 *         read as CMake, or memory runs out.
 */
struct d_cmake_doc*   d_cmake_doc_read(const char* _path,
                                       char*       _problem,
                                       size_t      _capacity);
// d_cmake_doc_make makes a document of a text, to be saved to `_path`; the
// caller frees it with d_cmake_doc_free, and it is `NULL` when the text does
// not read as CMake, `_problem` saying why.
struct d_cmake_doc*   d_cmake_doc_make(const char* _path,
                                       const char* _text,
                                       size_t      _length,
                                       char*       _problem,
                                       size_t      _capacity);
void                  d_cmake_doc_free(struct d_cmake_doc* _doc);
// What a document holds: its records, as of its last change; its path, how
// many changes it has taken, why the last change failed, and its text.
// d_cmake_doc_changed says where the last change kept was: the text before
// it from `start` to `old_end` is the text now from `start` to `new_end`,
// and all else is as it was; false before any change.
const struct d_cmake* d_cmake_doc_records(const struct d_cmake_doc* _doc);
const char*           d_cmake_doc_path(const struct d_cmake_doc* _doc);
size_t                d_cmake_doc_changes(const struct d_cmake_doc* _doc);
bool                  d_cmake_doc_changed(const struct d_cmake_doc* _doc,
                                          size_t* _out_start,
                                          size_t* _out_old_end,
                                          size_t* _out_new_end);
const char*           d_cmake_doc_problem(const struct d_cmake_doc* _doc);
const char*           d_cmake_doc_text(const struct d_cmake_doc* _doc,
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
 *         d_cmake_doc_problem saying why, and the file as it was.
 */
bool                  d_cmake_doc_splice(struct d_cmake_doc* _doc,
                                         size_t              _start,
                                         size_t              _end,
                                         const char*         _text,
                                         size_t              _length);

// 2.3    Arguments
//------------------------------------------------------------------------------
// An argument's text is as it is written in the file -- quotes, brackets
// and escapes and all -- and must read as one argument and nothing else.
//   d_cmake_doc_set_argument     replaces argument `_argument` of a command
//   d_cmake_doc_insert_argument  adds one beside argument `_argument`:
//                                before it, or `_after` it, in its layout;
//                                `_argument` the command's count appends
//                                after the last, or into an empty list
//   d_cmake_doc_remove_argument  removes one, with its line when it stood
//                                alone there, the comment ending that line
//                                with it
// Each is refused for a nested parenthesis, and wherever two arguments would
// run together; each gives `true` if the change was kept, and `false`
// otherwise, d_cmake_doc_problem saying why.
bool                  d_cmake_doc_set_argument(struct d_cmake_doc* _doc,
                                               size_t              _command,
                                               size_t              _argument,
                                               const char*         _text);
bool                  d_cmake_doc_insert_argument(struct d_cmake_doc* _doc,
                                                  size_t _command,
                                                  size_t _argument,
                                                  const char* _text,
                                                  bool        _after);
bool                  d_cmake_doc_remove_argument(struct d_cmake_doc* _doc,
                                                  size_t _command,
                                                  size_t _argument);

// 2.4    Commands
//------------------------------------------------------------------------------
// A command's text is one or more whole commands, as they are written, with
// comments among them if wished.
//   d_cmake_doc_set_name         renames a command, its role in its block
//                                kept
//   d_cmake_doc_set_command      replaces a command with one other, its role
//                                in its block kept
//   d_cmake_doc_insert_commands  writes commands on lines of their own before
//                                command `_before`, and above the comments
//                                that lead into it, or after the last at
//                                `_before` the count; each line indented as
//                                the commands of the block they go in
//   d_cmake_doc_remove_commands  removes commands `_first` to `_last`, each
//                                with its line when it stands alone there,
//                                and the comments among them
// Each gives `true` if the change was kept, and `false` otherwise,
// d_cmake_doc_problem saying why.
bool                  d_cmake_doc_set_name(struct d_cmake_doc* _doc,
                                           size_t              _command,
                                           const char*         _name);
bool                  d_cmake_doc_set_command(struct d_cmake_doc* _doc,
                                              size_t              _command,
                                              const char*         _text);
bool                  d_cmake_doc_insert_commands(struct d_cmake_doc* _doc,
                                                  size_t      _before,
                                                  const char* _text);
bool                  d_cmake_doc_remove_commands(struct d_cmake_doc* _doc,
                                                  size_t _first,
                                                  size_t _last);
// d_cmake_doc_indent writes the indentation a command inserted before
// command `_before` takes -- as d_cmake_doc_insert_commands takes it -- into
// `_out`, which holds `_capacity` bytes, and gives its length.
size_t                d_cmake_doc_indent(const struct d_cmake_doc* _doc,
                                         size_t                    _before,
                                         char*                     _out,
                                         size_t                    _capacity);

// 2.5    Saving
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
enum d_cmake_saved    d_cmake_doc_save(struct d_cmake_doc* _doc,
                                       const char*         _path,
                                       char*               _problem,
                                       size_t              _capacity);


#endif  // DJINTERP_TOOLS_DAWK_EXT_CMAKE_DCMAKE_DOC_H
