/*******************************************************************************
* djinterp [djinterp]                                                 dinclude.h
*
* Include extension:
*   Builds include groups: each maximal run of consecutive `#include` lines is
* one `include-group`, each line an `include`, and each include carries a
* `directive` cell and, when the line has one, a `summary` cell for its
* trailing `//` comment.  A category comment such as `// std` is not an
* include, so it ends a run, which is exactly how the guide groups them.
*   An include with no summary carries an absent summary node, so the guide's
* rule that every include has one can be stated and reported.
*
*
* path:      /inc/djinterp/tools/dawk/dinclude.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DINCLUDE_H
#define DJINTERP_TOOLS_DAWK_DINCLUDE_H 1

// std
#include <stdint.h>  // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Construction
//------------------------------------------------------------------------------
uint32_t d_include_build(struct d_node_tree* _tree,
                         uint32_t            _file,
                         const char*         _path);


#endif  // DJINTERP_TOOLS_DAWK_DINCLUDE_H
