/*******************************************************************************
* djinterp [c]                                                       container.h
*
*   The module
*
*
* path:      /inc/djinterp/c/container/container.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.31
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_CONTAINER_H
#define DJINTERP_C_CONTAINER_CONTAINER_H 1

// djinterp
#include "../djinterp.h"                                // framework root
#include "../../config/core/container/cfg_container.h"  // D_CFG_CONTAINER_*

// ===========================================================================
// 0.   CONTAINER CONFIGURATION
// ===========================================================================
//   The container knobs (D_CFG_CONTAINER_FILTER, D_CFG_CONTAINER_ARRAY_FILTER,
// the D_CFG_CONTAINER_ALL and D_CFG_FILTER aggregates) are resolved and
// validated in config/core/container/cfg_container.h, included above: a module
// reads its configuration, it does not derive it (config/README.md, the
// localization rule).


// DMergeConflictFlag
//   enum: how a merge resolves a key that already exists in the destination
// container -- leave the incumbent, replace it, or retain both entries.
enum DMergeConflictFlag
{
    D_MERGE_CONFLICT_FLAG_IGNORE    = 0x00,
    D_MERGE_CONFLICT_FLAG_OVERWRITE,
    D_MERGE_CONFLICT_FLAG_KEEP_BOTH
};


#endif  // DJINTERP_C_CONTAINER_CONTAINER_H
