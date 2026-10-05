/*******************************************************************************
* djinterp [config]                                                  cfg_event.h
*
*   Umbrella configuration for the event subframework: includes every
* cfg_event_*.h in dependency order and computes the cross-cutting values that
* no single one of them can, because each sees only its own dependencies.
*
*   Include this only from core/event/event.h, or from dconfig.h. A module
* that needs one layer's config includes that layer's config; pulling the
* umbrella from event_table_common.h would load the queue's defaults for a
* caller who never included the queue, which is the demand-loading principle
* going backwards.
*
*   targets:  core/event/event.h, event.hpp
*             ->  D_INTERNAL_EVENT_LAYOUT_ID, D_INTERNAL_EVENT_PROFILE
*   requires: cfg_common.h; every cfg_event_*.h
*
*
* path:      /inc/djinterp/config/core/event/cfg_event.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_H
#define DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_H 1

// djinterp
#include "../../cfg_common.h"
// dependency order: root, then each layer over the one below it.
#include "cfg_event_common.h"
#include "cfg_event_handler.h"
#include "cfg_event_table.h"
#include "cfg_event_registry.h"
#include "cfg_event_dispatcher.h"


// ===========================================================================
//  1.  CROSS-CUTTING DERIVED VALUES
// ===========================================================================

// D_INTERNAL_EVENT_LAYOUT_ID
//   brief: a small integer identifying the byte layout this build produces
// for the event subframework's shared structs. Stamp it into anything
// serialized, and refuse to decode bytes carrying a different value.
//
//   Only knobs that change a STRUCT contribute. D_CFG_EVENT_QUEUE is
// currently the only one: at 0 the dispatcher loses its queue member. Knobs
// that add or remove functions do not change any layout and are deliberately
// absent from this computation -- an id that moved when a statistics function
// was compiled out would make every build look incompatible with every other,
// and an incompatibility signal that always fires is one nobody reads.
//
//   Bit 0: queue present.
#define D_INTERNAL_EVENT_LAYOUT_ID                                            \
    ( (D_INTERNAL_EVENT_QUEUE ? 1 : 0) )

// D_INTERNAL_EVENT_PROFILE
//   brief: a coarse label for the build, for diagnostics and for the header
// of a dump. 0 minimal (no allocation, no deferral), 1 standard, 2 full.
#if D_CFG_IS_OFF(D_CFG_EVENT_TABLE_ALLOC) && D_CFG_IS_OFF(D_CFG_EVENT_QUEUE)
#   define D_INTERNAL_EVENT_PROFILE 0
#elif D_CFG_IS_ON(D_CFG_EVENT_PRESET_FULL)
#   define D_INTERNAL_EVENT_PROFILE 2
#else
#   define D_INTERNAL_EVENT_PROFILE 1
#endif


// ===========================================================================
//  2.  CROSS-CUTTING VALIDATION
// ===========================================================================
// These are the checks a single layer's config cannot make, because it cannot
// see the layer above it. They are warnings about coherence, not about
// syntax -- each names a combination that compiles and then disappoints.

// staging without a table is not a smaller build, it is an empty one.
#if D_CFG_IS_ON(D_CFG_EVENT_REGISTRY_STAGING) &&                              \
    D_CFG_IS_OFF(D_CFG_EVENT_TABLE_ORDERED_ITERATION)
    // permitted, but worth knowing: a fused word snapshots the enabled
    // letters in WORD order, which is unaffected by iteration order. No
    // conflict -- this block documents the non-interaction so nobody has to
    // re-derive it.
#endif

// a dump that is diffed needs a deterministic order and something to report.
#if D_CFG_IS_ON(D_CFG_TESTING) &&                                             \
    D_CFG_IS_OFF(D_CFG_EVENT_TABLE_ORDERED_ITERATION)
#   error "a testing build needs D_CFG_EVENT_TABLE_ORDERED_ITERATION: "       \
          "bucket order differs between two tables holding the same "         \
          "bindings, so a dump taken under it cannot serve the parity oracle"
#endif

// the minimal profile and an allocating queue are contradictory.
#if D_CFG_IS_OFF(D_CFG_EVENT_TABLE_ALLOC) &&                                  \
    D_CFG_IS_ON(D_CFG_EVENT_QUEUE) &&                                         \
    D_CFG_IS_ON(D_CFG_EVENT_QUEUE_DEFAULT_BYTES)
    // not an error: the queue still works over caller-provided storage. The
    // default sizes simply go unused, exactly as the table's do.
#endif


#endif  // DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_H
