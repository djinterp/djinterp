/*******************************************************************************
* djinterp [c]                                                       container.h
*
*   The root of the C container module: its configuration
* (djinterp/config/c/container/cfg_container.h) and the merge-conflict flags
* its maps and registries share. Each container has its own header, under array/,
* buffer/, map/, node/, registry/, stack/, table/, tree/ and vector/, and is
* included by name where it is used.
*   Thread safety: no function in the module keeps state of its own. Each
* container is its caller's to synchronise; circular_array_mtx.h is the
* circular array with a lock of its own.
*
*
* path:      /inc/djinterp/c/container/container.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.31
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_CONTAINER_H
#define DJINTERP_C_CONTAINER_CONTAINER_H 1

// djinterp
#include "../djinterp.h"                                // framework root
#include "../../config/c/container/cfg_container.h"  // D_CFG_CONTAINER_*

// ===========================================================================
// 0.   CONTAINER CONFIGURATION
// ===========================================================================
//   The container knobs (D_CFG_CONTAINER_FILTER, D_CFG_CONTAINER_ARRAY_FILTER,
// the D_CFG_CONTAINER_ALL and D_CFG_FILTER aggregates) are resolved and
// validated in djinterp/config/c/container/cfg_container.h, included above:
// a module reads its configuration, it does not derive it (the localization
// rule of docs/dconfig_readme.md).


// dj_merge_conflict_flag
//   enum: how a merge resolves a key that already exists in the destination
// container -- leave the incumbent, replace it, or retain both entries.
enum dj_merge_conflict_flag
{
    D_MERGE_CONFLICT_FLAG_IGNORE    = 0x00,
    D_MERGE_CONFLICT_FLAG_OVERWRITE,
    D_MERGE_CONFLICT_FLAG_KEEP_BOTH
};


#endif  // DJINTERP_C_CONTAINER_CONTAINER_H
