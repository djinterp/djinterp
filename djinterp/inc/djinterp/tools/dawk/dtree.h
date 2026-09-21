/*******************************************************************************
* djinterp [dawk]                                                        dtree.h
*
* Filesystem tree record source:
*   Declares the one entry point that fills a d_awk_source with a walk over a
* directory tree.  Files are yielded in document order and the record text is
* the file's path.
*   POSIX directory reading lives behind this seam, so the conforming core
* never links it.
*
* path:      /inc/djinterp/tools/dawk/dtree.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DTREE_H
#define DJINTERP_TOOLS_DAWK_DTREE_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "./dsource.h"  // d_awk_source


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Construction
//------------------------------------------------------------------------------
bool d_awk_source_tree_init(struct d_awk_source* _source,
                            const char*          _root);


#endif  // DJINTERP_TOOLS_DAWK_DTREE_H
