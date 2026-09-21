/*******************************************************************************
* djinterp [dawk]                                                      dbanner.h
*
* Banner extension:
*   Builds the node tree for one header's banner.  This is the reference
* extension: it shows what a host has to supply for a stylesheet to say
* anything about a file, and nothing else in the tool knows what a banner is.
*   An extension supplies three things and no more.  Node types, which the
* sheet names with type selectors.  Attributes, which the sheet tests with
* `[name=value]` and which derivations read by name.  Source geometry -- line,
* start column, end column, width -- which the layout properties compare
* against.  Everything the sheet can say is a consequence of those three.
*   Fields the guide requires but the banner does not carry are emitted as
* nodes with `present` false, because a rule cannot match a node that does not
* exist and `required` would otherwise assert nothing.
*
* path:      /inc/djinterp/tools/dawk/dbanner.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DBANNER_H
#define DJINTERP_TOOLS_DAWK_DBANNER_H 1

// std
#include <stdint.h>  // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_BANNER_LINE_MAX
//   constant: the longest banner line read.  A line beyond this is truncated
// rather than rejected; a banner line that long is already a defect and the
// width rule reports it.
#define D_BANNER_LINE_MAX 512


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Construction
//------------------------------------------------------------------------------
uint32_t d_banner_build(struct d_node_tree* _tree,
                        const char*         _path,
                        const char*         _root);


#endif  // DJINTERP_TOOLS_DAWK_DBANNER_H
