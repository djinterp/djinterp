/*******************************************************************************
* djinterp [djinterp]                                                    dedit.h
*
* Line editing:
*   Reads one line of a file by number, and replaces one line of a file in
* place, replaces a run of lines, or moves one sideways, whole or not at
* all.  Both preserve what they do not change: a line's own terminator is
* reproduced, so a fixer never normalises line endings it was not asked
* about.
*   This is deliberately the whole of the write path.  A repair that needs
* more than one line, or needs two edits ordered against each other, does not
* belong here -- it belongs in the edit-collection engine that replaces this
* file once more than one extension produces repairs.
*
*
* path:      /inc/djinterp/tools/dawk/dedit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DEDIT_H
#define DJINTERP_TOOLS_DAWK_DEDIT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Reading
//------------------------------------------------------------------------------
size_t d_edit_trim_end(
    char*  _text,
    size_t _length
);
bool d_edit_read_line(
    const char* _path,
    uint32_t    _line,
    char*       _out,
    size_t      _out_size
);

// 1.2    Writing
//------------------------------------------------------------------------------
bool d_edit_rewrite_line(
    const char* _path,
    uint32_t    _line,
    const char* _replacement
);
bool d_edit_delete_line(
    const char* _path,
    uint32_t    _line
);
bool d_edit_insert_line(
    const char* _path,
    uint32_t    _line,
    const char* _text
);

/**
 * @brief Replaces lines `_first` to `_last` of a file with `_text`.
 *
 * @param[in] _path   the file, rewritten in place.
 * @param[in] _first  the first line replaced, 1-based.
 * @param[in] _last   the last line replaced, at or after `_first`.
 * @param[in] _text   the lines that take their place, separated by `\n`
 *                    and with no terminator after the last.
 * @return true when the lines were replaced; false, with the file untouched,
 *         when it cannot be read or has fewer lines, and false when it
 *         cannot be written.
 * @post Each new line ends as `_first` did, LF or CRLF, and the last as
 *       `_last` did, so no line ending changes that was not asked about.
 */
bool d_edit_replace_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    const char* _text
);

// 1.3    Shifting
//------------------------------------------------------------------------------
/**
 * @brief Reports whether lines `_first` to `_last` of a file can all move
 *        `_delta` columns.
 *
 * @param[in] _path   the file.
 * @param[in] _first  the first line that moves, 1-based.
 * @param[in] _last   the last line that moves, at or after `_first`.
 * @param[in] _delta  columns right, or left where negative.
 * @return true when every line that holds anything can move: to the left,
 *         over spaces it has; to the right, ending within 80 characters.
 *         False otherwise, or when the file cannot be read or has fewer
 *         lines.
 */
bool d_edit_may_shift_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    int         _delta
);
/**
 * @brief Moves lines `_first` to `_last` of a file `_delta` columns, whole
 *        or not at all.
 *
 * @param[in] _path   the file, rewritten in place.
 * @param[in] _first  the first line that moves, 1-based.
 * @param[in] _last   the last line that moves, at or after `_first`.
 * @param[in] _delta  columns right, or left where negative.
 * @return true when the lines moved; false, with the file untouched, when
 *         d_edit_may_shift_lines refuses or the file cannot be read, and
 *         false when it cannot be written.
 * @post An empty line stays empty, and every line keeps its terminator.
 */
bool d_edit_shift_lines(
    const char* _path,
    uint32_t    _first,
    uint32_t    _last,
    int         _delta
);


#endif  // DJINTERP_TOOLS_DAWK_DEDIT_H
