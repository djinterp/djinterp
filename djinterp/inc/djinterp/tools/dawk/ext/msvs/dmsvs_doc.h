/*******************************************************************************
* djinterp [tools]                                                   dmsvs_doc.h
*
* A Visual Studio file held for change.
*   A document is a file's text and its records, read by dmsvs. A change is
* made at once: the text is spliced, the result read again, and the change
* kept only if the file reads whole -- so a document is always a file Visual
* Studio could open, and its records always the text's own. Every byte a
* change does not touch is the old text's, its line endings, its indenting,
* its byte-order mark among them.
*   An XML element is known by a handle, a number that stays its own while
* changes are made around it, though its index among the elements moves;
* the handle of an element a change removes names nothing after. A handle
* is never used again.
*   The changes an XML file takes: an attribute set or removed, an element's
* text set, an element added beneath another or removed. A name a change
* writes has no prefix: MSBuild reads namespaces, and a prefix not bound to
* one is an error there. An element added
* is written on a line of its own, indented as its siblings are, or one
* step past its parent, the step the file uses -- spaces or a tab.
*   A document is made from a file, or from a text not yet on disk, and saved
* to its own path or another; saving writes only a file that would change,
* and makes the directories it needs.
*
* path:      /inc/djinterp/tools/dawk/ext/msvs/dmsvs_doc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Documents
         1.  d_msvs_doc
         2.  d_msvs_new_element
         3.  d_msvs_saved
2.  OPERATIONS
    ----------
    1.  Documents
    2.  Handles
    3.  Changes
    4.  Saving
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MSVS_DMSVS_DOC_H
#define DJINTERP_TOOLS_DAWK_EXT_MSVS_DMSVS_DOC_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "./dmsvs.h"  // d_msvs


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Documents
//------------------------------------------------------------------------------
// 1.1.1
// d_msvs_doc
//   struct: one file's text, its records, the handles of its elements, and
//   why the last change failed.  Layout is private.
struct d_msvs_doc;

// 1.1.2
// d_msvs_new_element
//   struct: an element to add: its name, its text or `NULL` for none, and
//   `attribute_count` attributes as name and value in turn, both decoded,
//   from `attributes`.
struct d_msvs_new_element
{
    const char*        name;
    const char*        text;
    const char* const* attributes;
    size_t             attribute_count;
};

// 1.1.3
// d_msvs_saved
//   enum: what saving did.
enum d_msvs_saved
{
    D_MSVS_SAVE_FAILED = 0,
    D_MSVS_SAVE_WRITTEN,    // the file was written
    D_MSVS_SAVE_UNCHANGED   // the file held the text already
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Documents
//------------------------------------------------------------------------------
/**
 * @brief Reads a file into a document.
 *
 * @param[in]  _path      the file; its kind is its name's, by
 *                        d_msvs_kind_of.
 * @param[out] _problem   receives why it could not be read.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_msvs_doc_free.
 * @return the document, or `NULL`.
 */
struct d_msvs_doc*   d_msvs_doc_read(const char* _path,
                                     char*       _problem,
                                     size_t      _capacity);
/**
 * @brief Makes a document from a text, as if read from a path it is not yet
 *        saved to.
 *
 * @param[in]  _path      where it belongs, which names its kind.
 * @param[in]  _text      the text, copied.
 * @param[in]  _length    its length.
 * @param[out] _problem   receives why it does not read as that kind.
 * @param[in]  _capacity  the size of `_problem`.
 * @post   The caller frees the document with d_msvs_doc_free.
 * @return the document, or `NULL`.
 */
struct d_msvs_doc*   d_msvs_doc_make(const char* _path,
                                     const char* _text,
                                     size_t      _length,
                                     char*       _problem,
                                     size_t      _capacity);
void                 d_msvs_doc_free(struct d_msvs_doc* _doc);

// what a document holds: its records, read from its text as it is now; the
// path it saves to; its kind; how many changes it has taken since it was
// read, made or saved; and why its last change or save failed, "" when
// none has
const struct d_msvs* d_msvs_doc_records(const struct d_msvs_doc* _doc);
const char*          d_msvs_doc_path(const struct d_msvs_doc* _doc);
enum d_msvs_kind     d_msvs_doc_kind(const struct d_msvs_doc* _doc);
size_t               d_msvs_doc_changes(const struct d_msvs_doc* _doc);
const char*          d_msvs_doc_problem(const struct d_msvs_doc* _doc);

// 2.2    Handles
//------------------------------------------------------------------------------
// d_msvs_doc_handle is the handle of the element at an index, and 0 for an
// index past the last; d_msvs_doc_element is the index of the element a
// handle names now, and D_MSVS_NONE for a handle no element has.
size_t               d_msvs_doc_handle(const struct d_msvs_doc* _doc,
                                       size_t                   _element);
size_t               d_msvs_doc_element(const struct d_msvs_doc* _doc,
                                        size_t                   _handle);

// 2.3    Changes
//------------------------------------------------------------------------------
// Each change is made whole or not at all; one that fails leaves the
// document as it was, and says why in d_msvs_doc_problem.  Elements are
// named by their index, as it is when the change is asked for.
/**
 * @brief Replaces the bytes from `_start` to `_end` with a text.
 *
 * @note The change every other is made of; it is kept only when the file
 *       then reads whole.
 *
 * @param[in,out] _doc     the document.
 * @param[in]     _start   the first byte replaced.
 * @param[in]     _end     one past the last; `_start` for an insertion.
 * @param[in]     _text    the new bytes.
 * @param[in]     _length  how many.
 * @return `true` if the change was kept.
 */
bool                 d_msvs_doc_splice(struct d_msvs_doc* _doc,
                                       size_t             _start,
                                       size_t             _end,
                                       const char*        _text,
                                       size_t             _length);
// d_msvs_doc_set_attribute gives an element's attribute a value, decoded,
// adding the attribute after the last when it has none of that name;
// d_msvs_doc_remove_attribute removes one, and is true when it was there.
bool                 d_msvs_doc_set_attribute(struct d_msvs_doc* _doc,
                                              size_t             _element,
                                              const char*        _name,
                                              const char*        _value);
bool                 d_msvs_doc_remove_attribute(struct d_msvs_doc* _doc,
                                                 size_t             _element,
                                                 const char*        _name);
// d_msvs_doc_set_text gives an element that holds no element a text,
// decoded, in place of its content.
bool                 d_msvs_doc_set_text(struct d_msvs_doc* _doc,
                                         size_t             _element,
                                         const char*        _text);
/**
 * @brief Adds an element beneath another.
 *
 * @param[in,out] _doc      the document.
 * @param[in]     _parent   the element it goes beneath.
 * @param[in]     _before   the child it goes before, or D_MSVS_NONE to go
 *                          after the last.
 * @param[in]     _element  the element.
 * @return the new element's index, or D_MSVS_NONE when it was not added.
 */
size_t               d_msvs_doc_insert(
                         struct d_msvs_doc*               _doc,
                         size_t                           _parent,
                         size_t                           _before,
                         const struct d_msvs_new_element* _element);
// d_msvs_doc_remove removes an element and everything in it, with the line
// it stood on when it stood alone there; a parent left holding nothing is
// closed up, <ItemGroup />.  The root cannot be removed.
bool                 d_msvs_doc_remove(struct d_msvs_doc* _doc,
                                       size_t             _element);

// 2.4    Saving
//------------------------------------------------------------------------------
/**
 * @brief Saves a document.
 *
 * @note The text must read whole as the kind the path names. A file that
 *       holds the text already is not written. The directories the path
 *       needs are made.
 *
 * @param[in,out] _doc   the document.
 * @param[in]     _path  where to save it, which is then its path; `NULL`
 *                       for its own.
 * @return what was done.
 */
enum d_msvs_saved    d_msvs_doc_save(struct d_msvs_doc* _doc,
                                     const char*        _path);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MSVS_DMSVS_DOC_H
