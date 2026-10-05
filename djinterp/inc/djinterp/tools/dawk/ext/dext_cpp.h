/*******************************************************************************
* djinterp [djinterp]                                                 dext_cpp.h
*
* The C++ extension: binds C++ source to a DSS node tree.
*   Separate from the C extension and linkable without it; this file
* depends on the C++ dialect and the shared builder and on nothing else.  It
* is thin on purpose.  Everything the two extensions have in common is in
* dtoken_node.c, so this file is a dialect choice, a file-type test, and a
* call.
*   `.h` is not claimed.  The C extension owns it by default, since `.h`
* is C in this tree; a host that knows a header is C++ binds it here
* explicitly, which is what the path argument to d_ext_cpp_build is for.
*
*
* path:      /inc/djinterp/tools/dawk/ext/dext_cpp.h
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

#ifndef DJINTERP_TOOLS_DAWK_EXT_DEXT_CPP_H
#define DJINTERP_TOOLS_DAWK_EXT_DEXT_CPP_H 1

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
// d_ext_cpp_accepts reports whether a path is C++ by its extension.
// d_ext_cpp_build lexes a source as C++ at `_level` -- zero for the newest --
// and builds its tree beneath `_parent`, or beneath a new `file` node when
// `_parent` is D_DSS_NO_INDEX.  The source is borrowed for the call only.
bool         d_ext_cpp_accepts(const char* _path);
/**
 * @brief Builds the DSS node tree for C++ source.
 *
 * @param[in,out] _tree       the tree to add to.
 * @param[in]     _parent     the node to build beneath, or `D_DSS_NO_INDEX`.
 * @param[in]     _level      a `D_LEX_CPP*` level; zero selects the newest.
 * @param[in]     _input      the source, borrowed for the call, and the sink;
 *                            see d_token_node_input.  Skip a byte-order mark
 *                            with d_source_skip_bom first when the text may
 *                            carry one.
 * @param[out]    _out_stats  receives what was built; may be `NULL`.
 * @return the node the tree hangs from, or `D_DSS_NO_INDEX` on failure.
 */
uint32_t     d_ext_cpp_build(struct d_node_tree*              _tree,
                             uint32_t                         _parent,
                             unsigned                         _level,
                             const struct d_token_node_input* _input,
                             struct d_token_node_stats*       _out_stats);


#endif  // DJINTERP_TOOLS_DAWK_EXT_DEXT_CPP_H
