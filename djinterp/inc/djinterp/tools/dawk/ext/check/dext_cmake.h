/*******************************************************************************
* djinterp [tools]                                                  dext_cmake.h
*
* The CMake extension: binds a CMake file to a DSS node tree.
*   A sheet that names `file[lang=cmake]` reads what dcmake reads, as nodes:
*     command    one invocation; its text is its name as written.
*                [name] the name in lower case, as CMake compares it;
*                [case] lower, upper or mixed, as written; [depth] how many
*                blocks hold it; [role] open, branch or close, for a command
*                that takes part in a block; [arguments] how many it has;
*                [lines] how many lines it spans; [unpaired] a closer or a
*                branch with nothing to pair with, or an opener never closed.
*     argument   one argument, beneath its command; its text is as written.
*                [kind] unquoted, quoted or bracket; [value] what CMake reads
*                it as, before expansion; [paren] `(` or `)` for a nested
*                parenthesis; [legacy] an unquoted argument holding a quoted
*                run or a make variable.
*     comment    one comment, its `#` included, beneath the command whose
*                arguments it sits among or beneath the block or file that
*                holds it.  [kind] line or bracket; [trailing] it follows
*                something else on its line.
*     block      one if, foreach, while, function, macro or block, from its
*                opener to its closer, which are its first and last
*                commands; its branches, elseif and else, stand between,
*                beside the body.  [kind] the opener's name; [name] for a
*                function or a macro, the name it defines; [unclosed] the
*                file ends, or another block's closer comes, before its own.
*     refusal    where the text stopped being CMake, when it does; its text
*                is why.  The tree holds every command before it.
* The file node gains [lang] cmake and, for a file CMake or a generator
* wrote, [generated]; a generated file gets no tree, as a generated C file
* gets none.  Geometry is each node's first line: its line, the columns it
* covers there, and their width, in bytes as every extension counts them.
*   The extension supplies facts and decides nothing: which case a name takes,
* how a block is indented, whether an endif repeats its condition, are the
* sheet's to say.
*
* path:      /inc/djinterp/tools/dawk/ext/check/dext_cmake.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Results
         1.  d_ext_cmake_stats
2.  OPERATIONS
    ----------
    1.  Binding
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_CHECK_DEXT_CMAKE_H
#define DJINTERP_TOOLS_DAWK_EXT_CHECK_DEXT_CMAKE_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
#include <stdint.h>    // uint32_t
// djinterp
#include "../../dnode.h"  // d_node_tree


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Results
//------------------------------------------------------------------------------
// 1.1.1
// d_ext_cmake_stats
//   struct: what one build made.  `refused` is a text the reader stopped
//   in, `generated` a file that was given no tree.
struct d_ext_cmake_stats
{
    size_t commands;
    size_t arguments;
    size_t comments;
    size_t blocks;
    size_t unpaired;
    bool   generated;
    bool   refused;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Binding
//------------------------------------------------------------------------------
// d_ext_cmake_accepts reports whether a path is CMake's by its name:
// CMakeLists.txt, or a name ending in .cmake or .cmake.in.
bool         d_ext_cmake_accepts(const char* _path);
/**
 * @brief Builds the DSS node tree for a CMake file's text.
 *
 * @param[in,out] _tree       the tree to add to.
 * @param[in]     _parent     the file node to build beneath, or
 *                            `D_DSS_NO_INDEX` for a new one.
 * @param[in]     _text       the text, borrowed for the call; a leading
 *                            byte-order mark is skipped.
 * @param[in]     _length     its length in bytes.
 * @param[in]     _path       the file's path, which says whether a
 *                            generator wrote it; may be `NULL`.
 * @param[out]    _out_stats  receives what was built; may be `NULL`.
 * @return the file node, or `D_DSS_NO_INDEX` when no node could be added.
 */
uint32_t     d_ext_cmake_build(struct d_node_tree*       _tree,
                               uint32_t                  _parent,
                               const char*               _text,
                               size_t                    _length,
                               const char*               _path,
                               struct d_ext_cmake_stats* _out_stats);
// d_ext_cmake_build_path reads the file and builds its tree beneath a file
// node a driver already holds, as d_ext_source_build does for C; a file
// that cannot be read gets [lang] alone.  It returns the file node, or
// `D_DSS_NO_INDEX` for a path that is not CMake's.
uint32_t     d_ext_cmake_build_path(struct d_node_tree*       _tree,
                                    uint32_t                  _file,
                                    const char*               _path,
                                    struct d_ext_cmake_stats* _out_stats);


#endif  // DJINTERP_TOOLS_DAWK_EXT_CHECK_DEXT_CMAKE_H
