/*******************************************************************************
* djinterp [djinterp]                                                   dguard.h
*
* Include-guard extension:
*   Builds the node tree for a header's include guard -- the `#ifndef`, the
* `#define` and the closing `#endif` comment -- as three nodes beneath one
* `guard` node, each carrying the symbol it names.
*   This is the second extension, and it exists partly to prove that one is
* not special.  It contributes its own node types into the same tree the
* banner extension is filling, beneath the same file node, and neither knows
* the other is there.  A sheet may select across both.
*
*
* path:      /inc/djinterp/tools/dawk/dguard.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DGUARD_H
#define DJINTERP_TOOLS_DAWK_DGUARD_H 1

// std
#include <stdint.h>  // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Construction
//------------------------------------------------------------------------------
uint32_t d_guard_build(struct d_node_tree* _tree,
                       uint32_t            _file,
                       const char*         _path);


#endif  // DJINTERP_TOOLS_DAWK_DGUARD_H
