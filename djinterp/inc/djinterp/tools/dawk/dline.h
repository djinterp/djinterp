/*******************************************************************************
* djinterp [tools]                                                       dline.h
*
* Text-layer extension: the file as lines, before any language reads it.
*   Emits one `text` node beneath a file node, carrying the facts that are
* about the file as a whole -- a byte-order mark, its line endings, how many
* newlines it ends with, whether a generator wrote it -- and beneath that one
* `line` node per physical line, carrying what the style guide's Line Length
* and Whitespace sections measure: length in characters, tabs, trailing
* space, carriage returns, and characters that are not one column wide.
*   Every attribute is a fact, present when it holds.  None is a verdict: a
* line of 81 characters carries `length=81`, and whether that is too long is
* the sheet's to say, so the limit and its exemptions live in one place.
*
*
* path:      /inc/djinterp/tools/dawk/dline.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Building
*/

#ifndef DJINTERP_TOOLS_DAWK_DLINE_H
#define DJINTERP_TOOLS_DAWK_DLINE_H 1

// std
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree


//==============================================================================
// 1.  OPERATIONS
//==============================================================================
// The `text` node carries `bom`, `eol` (`lf`, `crlf`, `mixed` or `none`),
// `final-newlines` (terminators the file ends with: 0 when its last line has
// none, 2 or more when blank lines follow the last) and `generated`.  Each
// `line` carries `length` (code points, the terminator excluded, a malformed
// byte counting as one) and, when they hold, `blank`, `tab`, `trailing-space`,
// `cr`, `control`, `zero-width`, `wide` and `malformed`.  A line's geometry is
// its number, column 1, and its length.


// 1.1    Building
//------------------------------------------------------------------------------
/**
 * @brief Reads a file and builds its text layer beneath a file node.
 *
 * @param[in,out] _tree  the tree to add to.
 * @param[in]     _file  the file node to build beneath.
 * @param[in]     _path  the file to read.
 * @return the `text` node, or `D_DSS_NO_INDEX` when the file could not be
 *         read or the tree refused a node.
 */
uint32_t d_line_build(struct d_node_tree* _tree,
                      uint32_t            _file,
                      const char*         _path);
/**
 * @brief Builds the text layer of text already in memory.
 *
 * @note The text is borrowed for the call; nothing retains it.
 *
 * @param[in,out] _tree    the tree to add to.
 * @param[in]     _file    the file node to build beneath.
 * @param[in]     _text    the file's bytes; need not be terminated.
 * @param[in]     _length  their count.
 * @return the `text` node, or `D_DSS_NO_INDEX` when the tree refused a node.
 */
uint32_t d_line_build_text(struct d_node_tree* _tree,
                           uint32_t            _file,
                           const char*         _text,
                           size_t              _length);


#endif  // DJINTERP_TOOLS_DAWK_DLINE_H
