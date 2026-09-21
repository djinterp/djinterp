/*******************************************************************************
* djinterp [dawk]                                                        dedit.h
*
* Line editing:
*   Reads one line of a file by number, and replaces one line of a file in
* place.  Both preserve what they do not change: a line's own terminator is
* reproduced, so a fixer never normalises line endings it was not asked
* about.
*   This is deliberately the whole of the write path.  A repair that needs
* more than one line, or needs two edits ordered against each other, does not
* belong here -- it belongs in the edit-collection engine that replaces this
* file once more than one extension produces repairs.
*
* path:      /inc/djinterp/tools/dawk/dedit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
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
size_t d_edit_trim_end(char* _text, size_t _length);
bool   d_edit_read_line(const char* _path,
                        uint32_t    _line,
                        char*       _out,
                        size_t      _out_size);

// 1.2    Writing
//------------------------------------------------------------------------------
bool   d_edit_rewrite_line(const char* _path,
                           uint32_t    _line,
                           const char* _replacement);


#endif  // DJINTERP_TOOLS_DAWK_DEDIT_H
