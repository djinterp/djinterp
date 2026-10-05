/*******************************************************************************
* djinterp [config]                                            cfg_event_table.h
*
*   Configuration for the type-erased handler store: storage strategy, default
* capacities, the rehash policy, and which optional operations compile.
*
*   The sizing knobs here were literal constants in the pre-config header.
* That is exactly what the localization rule forbids -- a default living in a
* module header is a default nobody can override without editing the tree.
*
*   targets:  core/event/event_table_common.h, event_table.hpp
*             ->  D_INTERNAL_EVENT_TABLE_ALLOC, _STATS, _MERGE,
*                 _ORDERED_ITERATION, _DEFAULT_BUCKETS, _DEFAULT_CAPACITY,
*                 _LOAD_NUM, _LOAD_DEN, _GROWTH
*   requires: cfg_common.h; cfg_event_common.h
*
*
* path:      /inc/djinterp/config/core/event/cfg_event_table.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_TABLE_H
#define DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_TABLE_H 1

// djinterp
#include "../../cfg_common.h"
#include "cfg_event_common.h"


// ===========================================================================
//  1.  STORAGE STRATEGY
// ===========================================================================

// D_CFG_EVENT_TABLE_ALLOC
//   brief: 1 compiles the allocating table -- d_event_table_init, _rehash,
// _reserve, and the growth path. 0 leaves only the fixed form over
// caller-provided storage, which is the freestanding profile: the event
// subframework then performs no allocation anywhere and needs no allocator to
// exist.
//   This is a TIER-LAW knob, not a behaviour knob: at 0 the allocating
// symbols are ABSENT, they do not fail to compile.
#ifndef D_CFG_EVENT_TABLE_ALLOC
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_TABLE_ALLOC D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_TABLE_ALLOC 1
#   endif
#endif

// D_CFG_EVENT_TABLE_DEFAULT_CAPACITY
//   brief: entry-arena capacity chosen when an allocating table is created
// with capacity 0. Ignored entirely when D_CFG_EVENT_TABLE_ALLOC is 0.
#ifndef D_CFG_EVENT_TABLE_DEFAULT_CAPACITY
#   define D_CFG_EVENT_TABLE_DEFAULT_CAPACITY 64
#endif

// D_CFG_EVENT_TABLE_DEFAULT_BUCKETS
//   brief: bucket count chosen when an allocating table is created with
// bucket count 0. Rounded up to the next prime by the core, so a non-prime
// value here is legal and simply gets corrected.
#ifndef D_CFG_EVENT_TABLE_DEFAULT_BUCKETS
#   define D_CFG_EVENT_TABLE_DEFAULT_BUCKETS 101
#endif


// ===========================================================================
//  2.  REHASH POLICY
// ===========================================================================
// The threshold is an exact rational rather than a float so that both
// languages take the decision at the same entry count. A double literal would
// make the rehash point depend on rounding, and a rehash that happens at a
// different moment in C than in C++ changes iteration order -- which is
// observable in a statistics dump, and therefore a parity break.

// D_CFG_EVENT_TABLE_LOAD_NUM / D_CFG_EVENT_TABLE_LOAD_DEN
//   brief: the rehash threshold as num/den. The default 3/4 rehashes once
// live entries reach three quarters of the bucket count.
#ifndef D_CFG_EVENT_TABLE_LOAD_NUM
#   define D_CFG_EVENT_TABLE_LOAD_NUM 3
#endif
#ifndef D_CFG_EVENT_TABLE_LOAD_DEN
#   define D_CFG_EVENT_TABLE_LOAD_DEN 4
#endif

// D_CFG_EVENT_TABLE_GROWTH
//   brief: multiplier applied to the bucket count on rehash. Must be at
// least 2; a factor of 1 would rehash forever without growing.
#ifndef D_CFG_EVENT_TABLE_GROWTH
#   define D_CFG_EVENT_TABLE_GROWTH 2
#endif


// ===========================================================================
//  3.  OPTIONAL OPERATIONS
// ===========================================================================

// D_CFG_EVENT_TABLE_STATS
//   brief: 1 compiles struct d_event_table_stats and d_event_table_get_stats.
// Instrumentation, not formal content -- the note defines no statistics
// record -- so it is genuinely optional. It is declared in the SHARED header
// rather than once per language because the pre-core forms had different
// fields on each side, and two shapes for one report is a silent parity break
// the moment either is printed.
#ifndef D_CFG_EVENT_TABLE_STATS
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_TABLE_STATS D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_TABLE_STATS 1
#   endif
#endif

// D_CFG_EVENT_TABLE_MERGE
//   brief: 1 compiles the pointwise concatenation rho (+) rho'. Formal
// content, so switching it off narrows the algebra the build offers; do that
// only for a build that provably never merges two registries.
#ifndef D_CFG_EVENT_TABLE_MERGE
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_TABLE_MERGE D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_TABLE_MERGE 1
#   endif
#endif

// D_CFG_EVENT_TABLE_ORDERED_ITERATION
//   brief: 1 makes d_event_table_for_each visit entries in ascending handler
// id -- global bind order. 0 visits them in bucket order, which is cheaper
// and is fine for a build that only counts.
//   Turning this off has a stated consequence: bucket order depends on the
// hash and the capacity, so two tables holding the same bindings iterate
// differently, and a dump taken under bucket order cannot be compared against
// another build's. A dump that cannot be compared cannot serve the parity
// oracle, so leave this ON for any build whose output is diffed.
#ifndef D_CFG_EVENT_TABLE_ORDERED_ITERATION
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_TABLE_ORDERED_ITERATION D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_TABLE_ORDERED_ITERATION 1
#   endif
#endif


// ===========================================================================
//  4.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_EVENT_TABLE_ALLOC) &&                                  \
    !D_CFG_IS_OFF(D_CFG_EVENT_TABLE_ALLOC)
#   error "D_CFG_EVENT_TABLE_ALLOC must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_TABLE_STATS) &&                                  \
    !D_CFG_IS_OFF(D_CFG_EVENT_TABLE_STATS)
#   error "D_CFG_EVENT_TABLE_STATS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_TABLE_MERGE) &&                                  \
    !D_CFG_IS_OFF(D_CFG_EVENT_TABLE_MERGE)
#   error "D_CFG_EVENT_TABLE_MERGE must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_TABLE_ORDERED_ITERATION) &&                      \
    !D_CFG_IS_OFF(D_CFG_EVENT_TABLE_ORDERED_ITERATION)
#   error "D_CFG_EVENT_TABLE_ORDERED_ITERATION must be 0 or 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_TABLE_DEFAULT_CAPACITY) < 1)
#   error "D_CFG_EVENT_TABLE_DEFAULT_CAPACITY must be at least 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_TABLE_DEFAULT_BUCKETS) < 1)
#   error "D_CFG_EVENT_TABLE_DEFAULT_BUCKETS must be at least 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_TABLE_LOAD_DEN) < 1)
#   error "D_CFG_EVENT_TABLE_LOAD_DEN must be at least 1"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_TABLE_LOAD_NUM) < 1) ||                           \
    (D_CFG_NORM(D_CFG_EVENT_TABLE_LOAD_NUM) >                                 \
     D_CFG_NORM(D_CFG_EVENT_TABLE_LOAD_DEN))
#   error "D_CFG_EVENT_TABLE_LOAD_NUM must be in 1..D_CFG_EVENT_TABLE_LOAD_DEN"
#endif
#if (D_CFG_NORM(D_CFG_EVENT_TABLE_GROWTH) < 2)
#   error "D_CFG_EVENT_TABLE_GROWTH must be at least 2 (1 never grows)"
#endif


// ===========================================================================
//  5.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_EVENT_TABLE_ALLOC
//   brief: 1 when the allocating table symbols should be declared.
#define D_INTERNAL_EVENT_TABLE_ALLOC                                          \
    D_CFG_NORM(D_CFG_EVENT_TABLE_ALLOC)

// D_INTERNAL_EVENT_TABLE_STATS / _MERGE / _ORDERED_ITERATION
//   brief: 1 when the corresponding optional operation should be declared.
#define D_INTERNAL_EVENT_TABLE_STATS                                          \
    D_CFG_NORM(D_CFG_EVENT_TABLE_STATS)
#define D_INTERNAL_EVENT_TABLE_MERGE                                          \
    D_CFG_NORM(D_CFG_EVENT_TABLE_MERGE)
#define D_INTERNAL_EVENT_TABLE_ORDERED_ITERATION                              \
    D_CFG_NORM(D_CFG_EVENT_TABLE_ORDERED_ITERATION)

// D_INTERNAL_EVENT_TABLE_DEFAULT_CAPACITY / _DEFAULT_BUCKETS
//   brief: the capacities an allocating table falls back to.
#define D_INTERNAL_EVENT_TABLE_DEFAULT_CAPACITY                               \
    D_CFG_NORM(D_CFG_EVENT_TABLE_DEFAULT_CAPACITY)
#define D_INTERNAL_EVENT_TABLE_DEFAULT_BUCKETS                                \
    D_CFG_NORM(D_CFG_EVENT_TABLE_DEFAULT_BUCKETS)

// D_INTERNAL_EVENT_TABLE_LOAD_NUM / _LOAD_DEN / _GROWTH
//   brief: the rehash threshold and growth factor, as exact integers.
#define D_INTERNAL_EVENT_TABLE_LOAD_NUM                                       \
    D_CFG_NORM(D_CFG_EVENT_TABLE_LOAD_NUM)
#define D_INTERNAL_EVENT_TABLE_LOAD_DEN                                       \
    D_CFG_NORM(D_CFG_EVENT_TABLE_LOAD_DEN)
#define D_INTERNAL_EVENT_TABLE_GROWTH                                         \
    D_CFG_NORM(D_CFG_EVENT_TABLE_GROWTH)


#endif  // DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_TABLE_H
