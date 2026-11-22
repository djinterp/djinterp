/*******************************************************************************
* djinterp [c][text]                                               text_source.h
*
*   Text source: a text as it was read, by offset and by line.
*   A source holds a text and the offset where each of its lines starts, so
* that a byte's offset can be told as a line and column, and a line's number
* as the span of its bytes, without rescanning. A line runs to a newline or
* to the end of the text: a text ending with a newline has no empty line
* after it, an empty text has no lines, and a CR before a newline is left in
* the line, for a caller that measures line endings to see. Lines and columns
* count from 1, and a column counts bytes.
*   A source either copies its text or borrows it; a borrowed text must
* outlive the source.
*   d_text_read and d_text_write read a whole file and write one whole, so
* that a reader of the file sees the old text or the new, never a part.
*
*   New on 2026.11.18: dawk's line layer (dline) and its edits (dsplice) each
* worked lines and files out for themselves. d_text_read and d_text_write moved
* here from dsplice; d_text_write no longer deletes its new text when it cannot
* put it in place.
*
*
* path:      /inc/djinterp/c/text/text_source.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  dj_text_source
2.  SOURCES
    -------
    1.  Lifecycle
    2.  The text
    3.  Lines
3.  FILES
    -----
    1.  d_text_read
    2.  d_text_write
*/
#ifndef DJINTERP_C_TEXT_SOURCE_H
#define DJINTERP_C_TEXT_SOURCE_H 1
// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"  // framework root
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    dj_text_source
//------------------------------------------------------------------------------
// dj_text_source
//   struct: a text and where each of its lines starts. Its layout is private.
struct dj_text_source;


//==============================================================================
// 2.  SOURCES
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Makes a source over a copy of a text.
 *
 * @param[in] _text    the text; need not be terminated; may be NULL only when
 *                     _length is 0.
 * @param[in] _length  its length in bytes.
 * @return the source, whose copy is terminated; NULL on a bad argument or
 *         when memory runs out.
 */
struct dj_text_source* d_text_source_new(const char* _text,
                                         size_t      _length);
/**
 * @brief Makes a source over a text it does not copy.
 *
 * @param[in] _text    the text, which must outlive the source; may be NULL
 *                     only when _length is 0.
 * @param[in] _length  its length in bytes.
 * @return the source; NULL on a bad argument or when memory runs out.
 */
struct dj_text_source* d_text_source_borrow(const char* _text,
                                            size_t      _length);
/**
 * @brief Makes a source over a whole file, as d_text_read reads it.
 *
 * @param[in] _path  the file.
 * @return the source, which owns the text; NULL when the file cannot be
 *         read or memory runs out.
 */
struct dj_text_source* d_text_source_read(const char* _path);
/**
 * @brief Frees a source, and its text when it owns it; NULL is allowed.
 *
 * @param[in] _source  the source.
 */
void d_text_source_free(struct dj_text_source* _source);

// 2.2    The text
//------------------------------------------------------------------------------
/**
 * @brief Gives the source's text.
 *
 * @param[in]  _source      the source.
 * @param[out] _out_length  receives its length; may be NULL.
 * @return the text; terminated when the source owns it. NULL for NULL.
 */
const char* d_text_source_text(const struct dj_text_source* _source,
                               size_t*                      _out_length);

// 2.3    Lines
//------------------------------------------------------------------------------
/**
 * @brief Gives how many lines the text has.
 *
 * @param[in] _source  the source; may be NULL.
 * @return the count; 0 for an empty text, or NULL.
 */
size_t d_text_source_line_count(const struct dj_text_source* _source);
/**
 * @brief Gives the span of a line's bytes.
 *
 * @param[in]  _source     the source.
 * @param[in]  _line       the line's number, from 1.
 * @param[out] _out_start  receives the offset of its first byte; may be NULL.
 * @param[out] _out_end    receives the offset just past its last byte, which
 *                         is its newline's, or the text's end; may be NULL.
 * @return true when the source has the line; false otherwise, and nothing is
 *         written.
 */
bool d_text_source_line(const struct dj_text_source* _source,
                        size_t                       _line,
                        size_t*                      _out_start,
                        size_t*                      _out_end);
/**
 * @brief Tells a byte's offset as a line and a column.
 *
 * @note The text's end, an offset equal to its length, is a place too: on
 *       the last line, after its last byte; or, when the text ends with a
 *       newline or is empty, at column 1 of the line that would follow.
 *
 * @param[in]  _source      the source.
 * @param[in]  _offset      the offset, at most the text's length.
 * @param[out] _out_line    receives the line, from 1; may be NULL.
 * @param[out] _out_column  receives the column in bytes, from 1; may be
 *                          NULL.
 * @return true for an offset within the text or at its end; false otherwise,
 *         and nothing is written.
 */
bool d_text_source_locate(const struct dj_text_source* _source,
                          size_t                       _offset,
                          size_t*                      _out_line,
                          size_t*                      _out_column);


//==============================================================================
// 3.  FILES
//==============================================================================


// 3.1    d_text_read
//------------------------------------------------------------------------------
/**
 * @brief Reads a whole file, in binary, so its line endings are its own.
 *
 * @param[in]  _path        the file.
 * @param[out] _out_length  receives the text's length; 0 on failure. May be
 *                          NULL.
 * @return the text, terminated, for the caller to free; NULL when the file
 *         cannot be opened or read through, or memory runs out.
 */
char* d_text_read(const char* _path,
                  size_t*     _out_length);

// 3.2    d_text_write
//------------------------------------------------------------------------------
/**
 * @brief Writes a whole file: `<path>.part` first, renamed over the path
 *        once it is whole. Where a rename will not replace a file, as on
 *        Windows, the old file is removed and the rename tried again.
 *
 * @param[in]  _path      the file.
 * @param[in]  _text      the text; may be NULL only when _length is 0.
 * @param[in]  _length    its length.
 * @param[out] _problem   receives why it failed, terminated; may be NULL.
 * @param[in]  _capacity  _problem's size.
 * @return true when the file holds the text. On false, the file is as it
 *         was, or, where its old text was removed and the second rename
 *         failed, the new text is left whole in `<path>.part`, as _problem
 *         says.
 */
bool d_text_write(const char* _path,
                  const char* _text,
                  size_t      _length,
                  char*       _problem,
                  size_t      _capacity);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_SOURCE_H
