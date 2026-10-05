/*******************************************************************************
* djinterp [config]                                         cfg_event_registry.h
*
*   Configuration for the registry and its folds: which of the two outer folds
* compile, and whether the staging path is available.
*
*   Dispatch itself has no knob. It is the inner fold of seq over the masked
* word and it IS the event system; a build without it is not a smaller event
* subframework, it is not one.
*
*   targets:  core/event/event_registry_common.h, event_registry.hpp
*             ->  D_INTERNAL_EVENT_REGISTRY_RUN, _STAGING, _STAGING_ALLOC,
*                 _MERGE
*   requires: cfg_common.h; cfg_event_common.h; cfg_event_table.h
*
*
* path:      /inc/djinterp/config/core/event/cfg_event_registry.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_REGISTRY_H
#define DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_REGISTRY_H 1

// djinterp
#include "../../cfg_common.h"
#include "cfg_event_common.h"
// the registry's storage is the table, so its config is a dependency.
#include "cfg_event_table.h"


// ===========================================================================
//  1.  KNOBS
// ===========================================================================

// D_CFG_EVENT_REGISTRY_RUN
//   brief: 1 compiles run -- the outer fold of dispatch over a homogeneous
// trace. A caller can always write the loop itself; what run adds is one
// definition of it, shared by both languages.
#ifndef D_CFG_EVENT_REGISTRY_RUN
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_REGISTRY_RUN D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_REGISTRY_RUN 1
#   endif
#endif

// D_CFG_EVENT_REGISTRY_STAGING
//   brief: 1 compiles the staging path -- compile() into a fused word, and
// drive() over it. This is goals section 9, evaluation freedom: the same
// dispatch with its binding moved earlier. Switching it off costs the fused
// path and nothing else, because the fused and erased paths share one fold.
#ifndef D_CFG_EVENT_REGISTRY_STAGING
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_REGISTRY_STAGING D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_REGISTRY_STAGING 1
#   endif
#endif


// ===========================================================================
//  2.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_EVENT_REGISTRY_RUN) &&                                 \
    !D_CFG_IS_OFF(D_CFG_EVENT_REGISTRY_RUN)
#   error "D_CFG_EVENT_REGISTRY_RUN must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_REGISTRY_STAGING) &&                             \
    !D_CFG_IS_OFF(D_CFG_EVENT_REGISTRY_STAGING)
#   error "D_CFG_EVENT_REGISTRY_STAGING must be 0 or 1"
#endif


// ===========================================================================
//  3.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_EVENT_REGISTRY_RUN / _STAGING
//   brief: 1 when the corresponding fold should be declared.
#define D_INTERNAL_EVENT_REGISTRY_RUN                                         \
    D_CFG_NORM(D_CFG_EVENT_REGISTRY_RUN)
#define D_INTERNAL_EVENT_REGISTRY_STAGING                                     \
    D_CFG_NORM(D_CFG_EVENT_REGISTRY_STAGING)

// D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC
//   brief: 1 when the allocating compile() should be declared -- staging on,
// and allocation available. The compile-into-caller-storage form is declared
// whenever staging is on, so the fused path survives a no-allocator build.
#if D_CFG_IS_ON(D_CFG_EVENT_REGISTRY_STAGING) &&                              \
    D_CFG_IS_ON(D_CFG_EVENT_TABLE_ALLOC)
#   define D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC 1
#else
#   define D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC 0
#endif

// D_INTERNAL_EVENT_REGISTRY_MERGE
//   brief: 1 when registry merge should be declared. Propagated from the
// table rather than given its own knob: the registry's merge IS the table's
// merge, and a second switch over one operation is a second place to be
// wrong.
#define D_INTERNAL_EVENT_REGISTRY_MERGE                                       \
    D_INTERNAL_EVENT_TABLE_MERGE


#endif  // DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_REGISTRY_H
