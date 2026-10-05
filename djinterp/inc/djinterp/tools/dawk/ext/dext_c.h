/*******************************************************************************
* djinterp [djinterp]                                                   dext_c.h
*
* The C extension: binds C source to a DSS node tree.
*   Separate from the C++ extension and linkable without it; this file
* depends on the C dialect and the shared builder and on nothing else.  It
* is thin on purpose.  Everything the two extensions have in common is in
* dtoken_node.c, so this file is a dialect choice, a file-type test, and a
* call.
*   `.h` is claimed here, not by the C++ extension.  A header that is C++
* in fact -- one that uses `::` -- lexes wrongly as C, and a host that knows
* better binds it to dext_cpp explicitly.  The default follows the tree,
* where `.h` is C and C++ headers are `.hpp`.
*
*
* path:      /inc/djinterp/tools/dawk/ext/dext_c.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Binding
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_DEXT_C_H
#define DJINTERP_TOOLS_DAWK_EXT_DEXT_C_H 1

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


// 1.1    Binding
//------------------------------------------------------------------------------
// d_ext_c_accepts reports whether a path is C by its extension.
// d_ext_c_build lexes a source as C at `_level` -- zero for the newest --
// and builds its tree beneath `_parent`, or beneath a new `file` node when
// `_parent` is D_DSS_NO_INDEX.  The source is borrowed for the call only.
bool         d_ext_c_accepts(const char* _path);
/**
 * @brief Builds the DSS node tree for C source.
 *
 * @param[in,out] _tree       the tree to add to.
 * @param[in]     _parent     the node to build beneath, or `D_DSS_NO_INDEX`.
 * @param[in]     _level      a `D_LEX_C*` level; zero selects the newest.
 * @param[in]     _input      the source, borrowed for the call, and the sink;
 *                            see d_token_node_input.  Skip a byte-order mark
 *                            with d_source_skip_bom first when the text may
 *                            carry one.
 * @param[out]    _out_stats  receives what was built; may be `NULL`.
 * @return the node the tree hangs from, or `D_DSS_NO_INDEX` on failure.
 */
uint32_t     d_ext_c_build(struct d_node_tree*              _tree,
                           uint32_t                         _parent,
                           unsigned                         _level,
                           const struct d_token_node_input* _input,
                           struct d_token_node_stats*       _out_stats);


#endif  // DJINTERP_TOOLS_DAWK_EXT_DEXT_C_H
