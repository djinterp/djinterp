/*******************************************************************************
* djinterp [djinterp]                                              dext_source.h
*
* Source trees for a driver that walks files: read one C or C++ file from disk
* and build its token, group and item tree beneath the driver's file node.
*   dext_c and dext_cpp take a source already in memory, which is what a host
* embedding them wants.  A driver such as dcheck holds a path and a file node
* built by other extensions, and this is the adapter between the two: it
* chooses the language by the path's extension, reads the file, marks the file
* node with `lang`, skips generated files, and hands the text to the right
* binding.  Keeping it here keeps the driver free of any knowledge of C.
*
*
* path:      /inc/djinterp/tools/dawk/ext/dext_source.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Building
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_DEXT_SOURCE_H
#define DJINTERP_TOOLS_DAWK_EXT_DEXT_SOURCE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t
// djinterp
#include "../dnode.h"        // d_node_tree
#include "./dtoken_node.h"  // d_token_node_stats


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Building
//------------------------------------------------------------------------------
// whether a path names C or C++ source or a header, by its extension
bool         d_ext_source_accepts(const char* _path);
/**
 * @brief Reports whether a file's text carries the generator marker.
 *
 * @note The marker is `Auto-generated. Do not edit by hand.`, anywhere in the
 *       first 4 KiB.  The guide exempts a generated file from layout; the
 *       source tree skips one, and the text layer marks one `generated`.
 *
 * @param[in] _text    the file's text.
 * @param[in] _length  its length in bytes.
 * @return `true` if the marker is present, `false` otherwise.
 */
bool         d_ext_source_generated(const char* _text,
                                    size_t      _length);
/**
 * @brief Reads a C or C++ file and builds its source tree beneath a file node.
 *
 * @note The language is the extension's: `.hpp`, `.cpp` and their kin are
 *       C++, anything else accepted is C.  The file node gains `lang`
 *       (`c` or `c++`) first, since that is what the C and C++ sheets key
 *       on.  A generated file -- one whose first 4 KiB carry the tree's
 *       marker `Auto-generated. Do not edit by hand.` -- gets `lang` and no
 *       tree: the guide exempts generated files from layout, and the tree's
 *       largest are over 10 MB of macro tables.
 *
 * @param[in,out] _tree       the tree.
 * @param[in]     _file       the file node to build beneath.
 * @param[in]     _path       the file.
 * @param[out]    _out_stats  receives what was built; zeroed when nothing
 *                            was; may be `NULL`.
 * @return the node the source tree hangs from, which is `_file`, or
 *         `D_DSS_NO_INDEX` when the file was not read, was generated, or is
 *         not C or C++.
 */
uint32_t     d_ext_source_build(struct d_node_tree*        _tree,
                                uint32_t                   _file,
                                const char*                _path,
                                struct d_token_node_stats* _out_stats);


#endif  // DJINTERP_TOOLS_DAWK_EXT_DEXT_SOURCE_H
