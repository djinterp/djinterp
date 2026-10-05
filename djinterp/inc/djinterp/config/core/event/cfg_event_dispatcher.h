/*******************************************************************************
* djinterp [config]                                       cfg_event_dispatcher.h
*
*   Configuration for deferred delivery and the facade: whether the queue
* exists at all, how large it is by default, and what it does when full.
*
*   LAYOUT WARNING. D_CFG_EVENT_QUEUE is the one knob in this subframework
* that changes a shared struct: at 0, struct d_event_dispatcher loses its
* queue member and shrinks from 152 bytes to 80. Goals section 4 sanctions
* this ("representation is a policy, not a type"), and both languages read the
* same knob so parity is unaffected -- but two differently-configured builds
* then disagree on the wire. D_INTERNAL_EVENT_LAYOUT_ID in cfg_event.h exists
* to be stamped into anything serialized, so that disagreement is detected
* rather than silently decoded.
*
*   targets:  core/event/event_dispatcher_common.h, event_dispatcher.hpp
*             ->  D_INTERNAL_EVENT_QUEUE, _QUEUE_ALLOC, _QUEUE_OVERFLOW,
*                 _QUEUE_DEFAULT_CAPACITY, _QUEUE_DEFAULT_BYTES,
*                 _DISPATCHER_MERGE
*   requires: cfg_common.h; cfg_event_common.h; cfg_event_registry.h
*
*
* path:      /inc/djinterp/config/core/event/cfg_event_dispatcher.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_DISPATCHER_H
#define DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_DISPATCHER_H 1

// djinterp
#include "../../cfg_common.h"
#include "cfg_event_common.h"
#include "cfg_event_registry.h"


// ===========================================================================
//  1.  DEFERRAL
// ===========================================================================

// D_CFG_EVENT_QUEUE
//   brief: 1 compiles the deferred-delivery path -- struct d_event_queue, the
// dispatcher's queue member, enqueue, and process. 0 leaves immediate
// dispatch only, and the facade becomes a registry with a facade's names.
//   See the LAYOUT WARNING in this file's header before setting it to 0.
#ifndef D_CFG_EVENT_QUEUE
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_QUEUE D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_QUEUE 1
#   endif
#endif

// D_CFG_EVENT_QUEUE_DEFAULT_CAPACITY
//   brief: slot capacity chosen when an allocating queue is created with
// capacity 0 -- the number of occurrences that may be pending at once.
#ifndef D_CFG_EVENT_QUEUE_DEFAULT_CAPACITY
#   define D_CFG_EVENT_QUEUE_DEFAULT_CAPACITY 64
#endif

// D_CFG_EVENT_QUEUE_DEFAULT_BYTES
//   brief: payload-arena size in bytes chosen when an allocating queue is
// created with size 0. Deferral copies each payload by value, so this bounds
// total pending payload rather than any single one.
#ifndef D_CFG_EVENT_QUEUE_DEFAULT_BYTES
#   define D_CFG_EVENT_QUEUE_DEFAULT_BYTES 4096
#endif


// ===========================================================================
//  2.  OVERFLOW POLICY
// ===========================================================================

// D_CFG_EVENT_QUEUE_REJECT / D_CFG_EVENT_QUEUE_DROP_OLDEST
//   brief: the admissible values of D_CFG_EVENT_QUEUE_OVERFLOW.
#define D_CFG_EVENT_QUEUE_REJECT        0
#define D_CFG_EVENT_QUEUE_DROP_OLDEST   1

// D_CFG_EVENT_QUEUE_OVERFLOW
//   brief: what enqueue does when either arena is full. REJECT (0, the
// default) enqueues nothing and returns D_EVENT_ERR_CAPACITY, so the caller
// decides. DROP_OLDEST (1) discards pending occurrences from the head until
// the new one fits, which keeps the newest input flowing in a real-time loop
// at the cost of silently losing older occurrences.
//   This changes WHAT IS COMPUTED, not merely how fast. Both languages read
// the same knob, so C and C++ still agree -- but two builds under different
// policies will not, and only REJECT is loss-free.
#ifndef D_CFG_EVENT_QUEUE_OVERFLOW
#   define D_CFG_EVENT_QUEUE_OVERFLOW D_CFG_EVENT_QUEUE_REJECT
#endif

#if !D_CFG_IS_INT_LITERAL(D_CFG_EVENT_QUEUE_OVERFLOW)
    #error "D_CFG_EVENT_QUEUE_OVERFLOW must name one of its values; a misspelled name would read as 0"
#endif


// ===========================================================================
//  3.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_EVENT_QUEUE) && !D_CFG_IS_OFF(D_CFG_EVENT_QUEUE)
#   error "D_CFG_EVENT_QUEUE must be 0 or 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_QUEUE_DEFAULT_CAPACITY) < 1)
#   error "D_CFG_EVENT_QUEUE_DEFAULT_CAPACITY must be at least 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_QUEUE_DEFAULT_BYTES) < 1)
#   error "D_CFG_EVENT_QUEUE_DEFAULT_BYTES must be at least 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_QUEUE_OVERFLOW) != D_CFG_EVENT_QUEUE_REJECT) &&   \
    (D_CFG_NORM(D_CFG_EVENT_QUEUE_OVERFLOW) != D_CFG_EVENT_QUEUE_DROP_OLDEST)
#   error "D_CFG_EVENT_QUEUE_OVERFLOW must be _REJECT or _DROP_OLDEST"
#endif


// ===========================================================================
//  4.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_EVENT_QUEUE
//   brief: 1 when the queue and the dispatcher's deferral surface should be
// declared.
#define D_INTERNAL_EVENT_QUEUE                                                \
    D_CFG_NORM(D_CFG_EVENT_QUEUE)

// D_INTERNAL_EVENT_QUEUE_ALLOC
//   brief: 1 when the allocating queue constructor should be declared --
// queue on, and allocation available. The fixed form survives either way.
#if D_CFG_IS_ON(D_CFG_EVENT_QUEUE) && D_CFG_IS_ON(D_CFG_EVENT_TABLE_ALLOC)
#   define D_INTERNAL_EVENT_QUEUE_ALLOC 1
#else
#   define D_INTERNAL_EVENT_QUEUE_ALLOC 0
#endif

// D_INTERNAL_EVENT_QUEUE_OVERFLOW
//   brief: the resolved overflow policy, as one of the two pinned codes.
#define D_INTERNAL_EVENT_QUEUE_OVERFLOW                                       \
    D_CFG_NORM(D_CFG_EVENT_QUEUE_OVERFLOW)

// D_INTERNAL_EVENT_QUEUE_DEFAULT_CAPACITY / _DEFAULT_BYTES
//   brief: the capacities an allocating queue falls back to.
#define D_INTERNAL_EVENT_QUEUE_DEFAULT_CAPACITY                               \
    D_CFG_NORM(D_CFG_EVENT_QUEUE_DEFAULT_CAPACITY)
#define D_INTERNAL_EVENT_QUEUE_DEFAULT_BYTES                                  \
    D_CFG_NORM(D_CFG_EVENT_QUEUE_DEFAULT_BYTES)

// D_INTERNAL_EVENT_DISPATCHER_MERGE
//   brief: 1 when the facade's merge should be declared. Propagated from the
// registry, which propagates it from the table.
#define D_INTERNAL_EVENT_DISPATCHER_MERGE                                     \
    D_INTERNAL_EVENT_REGISTRY_MERGE


#endif  // DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_DISPATCHER_H
