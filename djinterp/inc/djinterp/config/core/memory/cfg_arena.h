/*******************************************************************************
* djinterp [config]                                                  cfg_arena.h
*
*   Configuration for the monotonic bump arena. Selects whether an arena may
* chain regions at all, how a chained arena sizes each next region, whether
* the growth policy is a per-instance field or a compile-time constant,
* whether marks and last-allocation tracking are compiled, and the region
* geometry an arena requests from its upstream.
*
*   THE ONE KNOB THAT MATTERS MOST is D_CFG_ARENA_CHAIN. Off, an arena is a
* single region over a single upstream block: no tail pointer, no region
* count, no next-size, no growth policy, and a region header with no link.
* That removes five fields from the arena and one from every region header,
* and it is the shape an embedded caller with one fixed pool of memory
* actually wants. On, the arena grows by chaining, and every pointer it ever
* handed out stays valid across the growth -- which is the shape a compiler,
* a parser, or any tree builder wants.
*
*   Neither shape is a subset of the other in behaviour, only in cost, which
* is why this is a knob and not a policy argument: an arena that CAN chain
* pays for the ability whether or not it uses it.
*
*   targets:  core/memory/arena.h, arena.c  ->  D_INTERNAL_ARENA_CHAIN,
*             D_INTERNAL_ARENA_GROWTH_RUNTIME, D_INTERNAL_ARENA_GROWTH,
*             D_INTERNAL_ARENA_REGION_BYTES, D_INTERNAL_ARENA_MAX_REGION,
*             D_INTERNAL_ARENA_GROWTH_NUM, D_INTERNAL_ARENA_GROWTH_DEN,
*             D_INTERNAL_ARENA_GROWTH_STEP, D_INTERNAL_ARENA_MARKS,
*             D_INTERNAL_ARENA_VALIDATE_MARKS, D_INTERNAL_ARENA_LAST_ALLOC,
*             D_INTERNAL_ARENA_OWNS, D_INTERNAL_ARENA_AS_SOURCE
*   requires: cfg_mem_common.h (vocabulary knobs, presets); cfg_mem_source.h
*
*
* path:      /inc/djinterp/config/core/memory/cfg_arena.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_MEMORY_CFG_ARENA_H
#define DJINTERP_CONFIG_CORE_MEMORY_CFG_ARENA_H 1

// (0) subframework root first, then the dependency whose defaults this file's
//     own defaults read.
#include "./cfg_mem_common.h"
#include "./cfg_mem_source.h"


// ---------------------------------------------------------------------------
//  contents
// ---------------------------------------------------------------------------
//    1.  chaining
//    2.  growth policy
//    3.  region geometry
//    4.  marks and rewind
//    5.  optional capabilities
//    6.  validation
//    7.  derived values
// ---------------------------------------------------------------------------


// ===========================================================================
//  1.  CHAINING
// ===========================================================================

// D_CFG_ARENA_CHAIN
//   brief: 1 lets an arena obtain further regions from its upstream when the
// current one is full, linking them so that every pointer already handed out
// stays valid. 0 confines an arena to one region: exhaustion is permanent, and
// the arena loses its tail pointer, region count, next-size and growth policy,
// while every region header loses its link.
//   Defaults ON, because an arena that cannot grow surprises a caller who did
// not read this file, and a surprise that costs correctness outranks one that
// costs forty bytes. MINIMAL turns it off.
#ifndef D_CFG_ARENA_CHAIN
#   if D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL)
#       define D_CFG_ARENA_CHAIN 0
#   else
#       define D_CFG_ARENA_CHAIN 1
#   endif
#endif


// ===========================================================================
//  2.  GROWTH POLICY
// ===========================================================================

// D_CFG_ARENA_GROWTH_FIXED / _LINEAR / _EXPONENTIAL
//   brief: the three admissible values of D_CFG_ARENA_GROWTH.
//     FIXED       - every region is the same size. Bounded worst-case waste,
//                   and a linear number of upstream calls.
//     LINEAR      - each region is D_CFG_ARENA_GROWTH_STEP bytes larger than
//                   the last. A compromise, and the one to reach for when
//                   upstream calls are cheap but large blocks are not.
//     EXPONENTIAL - each region is NUM/DEN times the last. A logarithmic
//                   number of upstream calls, at the cost of a last region
//                   that may be much larger than what was actually needed.
#define D_CFG_ARENA_GROWTH_FIXED        0
#define D_CFG_ARENA_GROWTH_LINEAR       1
#define D_CFG_ARENA_GROWTH_EXPONENTIAL  2

// D_CFG_ARENA_GROWTH
//   brief: the default growth policy, and -- when
// D_CFG_ARENA_GROWTH_RUNTIME is 0 -- the only one. EXPONENTIAL by default,
// because the number of upstream calls is the cost an arena exists to avoid.
#ifndef D_CFG_ARENA_GROWTH
#   define D_CFG_ARENA_GROWTH D_CFG_ARENA_GROWTH_EXPONENTIAL
#endif

#if !D_CFG_IS_INT_LITERAL(D_CFG_ARENA_GROWTH)
    #error "D_CFG_ARENA_GROWTH must name one of its values; a misspelled name would read as 0"
#endif

// D_CFG_ARENA_GROWTH_RUNTIME
//   brief: 1 carries the growth policy as a per-instance field, so two arenas
// in one program may grow differently. 0 pins it to D_CFG_ARENA_GROWTH at
// compile time, removing the field and turning the growth computation into
// straight-line code with no switch.
//   Defaults ON: the field is one enum, and a program with a bump arena for
// per-frame scratch and another for a parse tree genuinely wants two policies.
#ifndef D_CFG_ARENA_GROWTH_RUNTIME
#   if D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL)
#       define D_CFG_ARENA_GROWTH_RUNTIME 0
#   else
#       define D_CFG_ARENA_GROWTH_RUNTIME 1
#   endif
#endif

// D_CFG_ARENA_GROWTH_NUM / D_CFG_ARENA_GROWTH_DEN
//   brief: the exponential ratio, as an exact integer fraction rather than a
// float -- so that two builds, two languages, and two architectures compute
// the same region sizes, which a float ratio does not guarantee and which the
// parity law requires.
//   Defaults to 2/1. The often-cited 1.5 is chosen so that freed blocks can be
// coalesced and reused by a later request, which is reasoning about a general
// heap; an arena never reuses a released region, so the argument does not
// apply and the smaller number of upstream calls wins.
#ifndef D_CFG_ARENA_GROWTH_NUM
#   define D_CFG_ARENA_GROWTH_NUM 2
#endif
#ifndef D_CFG_ARENA_GROWTH_DEN
#   define D_CFG_ARENA_GROWTH_DEN 1
#endif

// D_CFG_ARENA_GROWTH_STEP
//   brief: the linear increment, in bytes. Defaults to the default region
// size, so LINEAR growth produces regions of 1x, 2x, 3x ... the base.
#ifndef D_CFG_ARENA_GROWTH_STEP
#   define D_CFG_ARENA_GROWTH_STEP 0
#endif


// ===========================================================================
//  3.  REGION GEOMETRY
// ===========================================================================

// D_CFG_ARENA_REGION_BYTES
//   brief: the payload size of an arena's first region when its configuration
// names none, in bytes. 65536 by default -- large enough that the upstream
// call is amortised over thousands of small allocations, small enough that an
// arena which is created and never used costs one page-ish block rather than
// a megabyte.
#ifndef D_CFG_ARENA_REGION_BYTES
#   define D_CFG_ARENA_REGION_BYTES 65536
#endif

// D_CFG_ARENA_MAX_REGION_BYTES
//   brief: the ceiling on a single region's payload, in bytes; 0 means none.
// This is the knob that makes exponential growth safe on a long-lived arena:
// without it, the twentieth region of a doubling arena is 32 GiB. Defaults to
// 16 MiB, which is roughly where a single upstream request stops being cheap
// on every allocator this is likely to sit on.
//   A request LARGER than the ceiling is still satisfied -- the region is
// sized to the request. The ceiling bounds speculative growth, not need.
#ifndef D_CFG_ARENA_MAX_REGION_BYTES
#   define D_CFG_ARENA_MAX_REGION_BYTES (16 * 1024 * 1024)
#endif

// D_CFG_ARENA_REGION_ALIGN
//   brief: the alignment an arena requests its regions at; 0 means the
// configured default. Raising it to a page or a cache line is how a caller
// buys an arena whose every region is page-aligned -- for a mapping-backed
// source, or to keep two arenas off each other's cache lines.
#ifndef D_CFG_ARENA_REGION_ALIGN
#   define D_CFG_ARENA_REGION_ALIGN 0
#endif


// ===========================================================================
//  4.  MARKS AND REWIND
// ===========================================================================

// D_CFG_ARENA_MARKS
//   brief: 1 compiles mark and rewind, which turn the arena into a stack
// allocator: take a mark, allocate freely, rewind, and every byte since the
// mark is available again with no per-allocation bookkeeping at all.
//   A BRANCH knob -- a mark is a value the CALLER holds, so the arena carries
// no field for it and this costs only the two functions. Defaults ON.
#ifndef D_CFG_ARENA_MARKS
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_ARENA_MARKS D_CFG_MEM_ALL
#   else
#       define D_CFG_ARENA_MARKS 1
#   endif
#endif

// D_CFG_ARENA_VALIDATE_MARKS
//   brief: 1 checks, on rewind, that a mark names a region this arena
// actually owns -- which costs a walk of the region chain and catches the
// mistake that otherwise corrupts silently: rewinding one arena with another
// arena's mark. Follows D_CFG_TESTING.
#ifndef D_CFG_ARENA_VALIDATE_MARKS
#   if D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_ARENA_VALIDATE_MARKS 1
#   else
#       define D_CFG_ARENA_VALIDATE_MARKS 0
#   endif
#endif


// ===========================================================================
//  5.  OPTIONAL CAPABILITIES
// ===========================================================================

// D_CFG_ARENA_LAST_ALLOC
//   brief: 1 remembers the most recent allocation, enabling d_arena_extend
// (grow it in place when it is still at the top of the bump) and
// d_arena_pop_last (give it straight back). A FIELD knob: two counters.
//   Its value is a growable buffer built on an arena: append becomes extend,
// which is free while nothing else has allocated in between, which is the
// common case for a buffer being filled in a loop. Defaults OFF, because a
// caller not doing that pays two counters for nothing.
#ifndef D_CFG_ARENA_LAST_ALLOC
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_ARENA_LAST_ALLOC D_CFG_MEM_ALL
#   else
#       define D_CFG_ARENA_LAST_ALLOC 0
#   endif
#endif

// D_CFG_ARENA_OWNS
//   brief: 1 compiles d_arena_owns, which answers whether a pointer came from
// this arena by walking its regions. A BRANCH knob; the walk is O(regions),
// which is why it is a diagnostic rather than something the allocate path
// uses. Defaults ON.
#ifndef D_CFG_ARENA_OWNS
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_ARENA_OWNS D_CFG_MEM_ALL
#   else
#       define D_CFG_ARENA_OWNS 1
#   endif
#endif

// D_CFG_ARENA_AS_SOURCE
//   brief: 1 compiles d_arena_as_source, which presents an arena AS a
// d_mem_source so that a pool or a slab can draw its blocks from it.
//   This is the composition knob. With it, "one upstream call, then a pool
// and three arenas carved out of it" is a configuration rather than a design.
// Costs one static vtable and three small functions. Defaults ON.
#ifndef D_CFG_ARENA_AS_SOURCE
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_ARENA_AS_SOURCE D_CFG_MEM_ALL
#   else
#       define D_CFG_ARENA_AS_SOURCE 1
#   endif
#endif


// ===========================================================================
//  6.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_ARENA_CHAIN) && !D_CFG_IS_OFF(D_CFG_ARENA_CHAIN)
#   error "D_CFG_ARENA_CHAIN must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_ARENA_GROWTH_RUNTIME) &&                               \
    !D_CFG_IS_OFF(D_CFG_ARENA_GROWTH_RUNTIME)
#   error "D_CFG_ARENA_GROWTH_RUNTIME must be 0 or 1"
#endif
#if ( (D_CFG_NORM(D_CFG_ARENA_GROWTH) < D_CFG_ARENA_GROWTH_FIXED) ||          \
      (D_CFG_NORM(D_CFG_ARENA_GROWTH) > D_CFG_ARENA_GROWTH_EXPONENTIAL) )
#   error "D_CFG_ARENA_GROWTH must be _FIXED, _LINEAR or _EXPONENTIAL"
#endif
#if (D_CFG_NORM(D_CFG_ARENA_GROWTH_DEN) < 1)
#   error "D_CFG_ARENA_GROWTH_DEN must be >= 1"
#endif
#if (D_CFG_NORM(D_CFG_ARENA_GROWTH_NUM) < D_CFG_NORM(D_CFG_ARENA_GROWTH_DEN))
#   error "D_CFG_ARENA_GROWTH_NUM/_DEN must be a ratio >= 1 (growth, not decay)"
#endif
#if (D_CFG_NORM(D_CFG_ARENA_REGION_BYTES) < 1)
#   error "D_CFG_ARENA_REGION_BYTES must be >= 1"
#endif
#if ( (D_CFG_NORM(D_CFG_ARENA_MAX_REGION_BYTES) != 0) &&                      \
      (D_CFG_NORM(D_CFG_ARENA_MAX_REGION_BYTES) <                             \
       D_CFG_NORM(D_CFG_ARENA_REGION_BYTES)) )
#   error "D_CFG_ARENA_MAX_REGION_BYTES must be 0 or >= _REGION_BYTES"
#endif
#if ( (D_CFG_NORM(D_CFG_ARENA_REGION_ALIGN) != 0) &&                          \
      ( (D_CFG_NORM(D_CFG_ARENA_REGION_ALIGN) &                               \
         (D_CFG_NORM(D_CFG_ARENA_REGION_ALIGN) - 1)) != 0 ) )
#   error "D_CFG_ARENA_REGION_ALIGN must be 0 or a power of two"
#endif
#if !D_CFG_IS_ON(D_CFG_ARENA_MARKS) && !D_CFG_IS_OFF(D_CFG_ARENA_MARKS)
#   error "D_CFG_ARENA_MARKS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_ARENA_VALIDATE_MARKS) &&                               \
    !D_CFG_IS_OFF(D_CFG_ARENA_VALIDATE_MARKS)
#   error "D_CFG_ARENA_VALIDATE_MARKS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_ARENA_LAST_ALLOC) &&                                   \
    !D_CFG_IS_OFF(D_CFG_ARENA_LAST_ALLOC)
#   error "D_CFG_ARENA_LAST_ALLOC must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_ARENA_OWNS) && !D_CFG_IS_OFF(D_CFG_ARENA_OWNS)
#   error "D_CFG_ARENA_OWNS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_ARENA_AS_SOURCE) && !D_CFG_IS_OFF(D_CFG_ARENA_AS_SOURCE)
#   error "D_CFG_ARENA_AS_SOURCE must be 0 or 1"
#endif


// ===========================================================================
//  7.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_ARENA_CHAIN
//   brief: 1 when an arena may hold more than one region. Read by arena.h for
// the struct shape and by arena.c for every traversal.
#define D_INTERNAL_ARENA_CHAIN        D_CFG_NORM(D_CFG_ARENA_CHAIN)

// D_INTERNAL_ARENA_GROWTH_RUNTIME
//   brief: 1 when the growth policy is a per-instance field. Forced off when
// chaining is off, since an arena that never grows has no policy to carry --
// which is the one place these two knobs interact, resolved here so arena.c
// never tests both.
#if D_CFG_IS_ON(D_CFG_ARENA_CHAIN) && D_CFG_IS_ON(D_CFG_ARENA_GROWTH_RUNTIME)
#   define D_INTERNAL_ARENA_GROWTH_RUNTIME 1
#else
#   define D_INTERNAL_ARENA_GROWTH_RUNTIME 0
#endif

// D_INTERNAL_ARENA_GROWTH
//   brief: the compile-time growth policy -- the default when the policy is a
// field, the only value when it is not.
#define D_INTERNAL_ARENA_GROWTH       D_CFG_NORM(D_CFG_ARENA_GROWTH)

// D_INTERNAL_ARENA_GROWTH_NUM / _DEN / _STEP
//   brief: the exponential ratio and the linear increment. STEP resolves its
// 0 sentinel to the default region size here, so arena.c reads a number.
#define D_INTERNAL_ARENA_GROWTH_NUM                                           \
    ((d_mem_size)(D_CFG_ARENA_GROWTH_NUM))
#define D_INTERNAL_ARENA_GROWTH_DEN                                           \
    ((d_mem_size)(D_CFG_ARENA_GROWTH_DEN))
#if (D_CFG_NORM(D_CFG_ARENA_GROWTH_STEP) == 0)
#   define D_INTERNAL_ARENA_GROWTH_STEP                                       \
        ((d_mem_size)(D_CFG_ARENA_REGION_BYTES))
#else
#   define D_INTERNAL_ARENA_GROWTH_STEP                                       \
        ((d_mem_size)(D_CFG_ARENA_GROWTH_STEP))
#endif

// D_INTERNAL_ARENA_REGION_BYTES / _MAX_REGION / _REGION_ALIGN
//   brief: the region geometry, as d_mem_size values.
#define D_INTERNAL_ARENA_REGION_BYTES                                         \
    ((d_mem_size)(D_CFG_ARENA_REGION_BYTES))
#define D_INTERNAL_ARENA_MAX_REGION                                           \
    ((d_mem_size)(D_CFG_ARENA_MAX_REGION_BYTES))
#define D_INTERNAL_ARENA_REGION_ALIGN                                         \
    ((d_mem_size)(D_CFG_ARENA_REGION_ALIGN))

// D_INTERNAL_ARENA_MARKS / _VALIDATE_MARKS
//   brief: 1 when mark and rewind exist, and 1 when rewind should verify that
// a mark belongs to the arena being rewound. The second is forced off with the
// first.
#define D_INTERNAL_ARENA_MARKS        D_CFG_NORM(D_CFG_ARENA_MARKS)
#if D_CFG_IS_ON(D_CFG_ARENA_MARKS) && D_CFG_IS_ON(D_CFG_ARENA_VALIDATE_MARKS)
#   define D_INTERNAL_ARENA_VALIDATE_MARKS 1
#else
#   define D_INTERNAL_ARENA_VALIDATE_MARKS 0
#endif

// D_INTERNAL_ARENA_LAST_ALLOC / _OWNS / _AS_SOURCE
//   brief: 1 when the corresponding optional capability is compiled.
#define D_INTERNAL_ARENA_LAST_ALLOC   D_CFG_NORM(D_CFG_ARENA_LAST_ALLOC)
#define D_INTERNAL_ARENA_OWNS         D_CFG_NORM(D_CFG_ARENA_OWNS)
#define D_INTERNAL_ARENA_AS_SOURCE    D_CFG_NORM(D_CFG_ARENA_AS_SOURCE)


#endif  // DJINTERP_CONFIG_CORE_MEMORY_CFG_ARENA_H
