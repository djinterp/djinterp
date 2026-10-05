/*******************************************************************************
* djinterp [config]                                             cfg_mem_common.h
*
*   Root configuration for the memory subframework. Owns the aggregate and the
* presets, the vocabulary knobs every memory module reads (counter width,
* alignment floor, accounting, poisoning, redzones), and the derived symbols
* that select the linkage model, the assertion level, and the C++ notation
* tier.
*
*   Every other cfg_arena.h / cfg_pool.h / cfg_slab.h / cfg_mem_source.h
* includes this file first, so a preset resolved here is in force before any
* child's defaults run. That ordering is what lets the memory subframework
* carry presets at all under demand-loading: a cross-cutting rule normally has
* to live in dconfig.h, but a rule confined to one subframework can live at
* that subframework's config root, because the root is on every child's
* include path by construction.
*
*   THE BYTE BUDGET. This subframework's premise is that the caller decides
* what an allocator costs, so nearly every knob here removes or adds STRUCT
* FIELDS rather than merely gating a branch. D_CFG_MEM_SIZE_BITS halves every
* counter; D_CFG_MEM_STATS removes the accounting block outright;
* D_CFG_MEM_REDZONE moves bytes per allocation, not per allocator. A knob that
* only guards a branch is noted as such, so the two kinds are never confused.
*
*   A note on parity (framework_goals.md section 2): every knob here is read by
* BOTH languages, because both compile this same header. A knob therefore
* cannot make C and C++ disagree -- it can only move both together. Config
* that lived in one face would be a parity hazard; config that lives here is
* not.
*
*   targets:  core/memory/mem_common.h, mem_source.h, arena.h, pool.h, slab.h,
*             mem_common.hpp  ->  D_INTERNAL_MEM_SIZE_BITS,
*             D_INTERNAL_MEM_STATS, D_INTERNAL_MEM_STATS_PEAK,
*             D_INTERNAL_MEM_POISON, D_INTERNAL_MEM_REDZONE,
*             D_INTERNAL_MEM_REDZONE_BYTES, D_INTERNAL_MEM_DEFAULT_ALIGN,
*             D_INTERNAL_MEM_ZERO, D_INTERNAL_MEM_STRICT_ARGS,
*             D_INTERNAL_MEM_ASSERT_LAYOUT, D_INTERNAL_MEM_ASSERT_SIZES,
*             D_INTERNAL_MEM_HEADER_ONLY, D_INTERNAL_MEM_USE_CONCEPTS,
*             D_INTERNAL_MEM_TRAIT_DETECTORS
*   requires: cfg_common.h (helpers, D_CFG_TESTING)
*
*
* path:      /inc/djinterp/config/core/memory/cfg_mem_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_MEMORY_CFG_MEM_COMMON_H
#define DJINTERP_CONFIG_CORE_MEMORY_CFG_MEM_COMMON_H 1

// (0) root first: helpers, user overrides, testing flag and preset.
#include "../../cfg_common.h"


// ---------------------------------------------------------------------------
//  contents
// ---------------------------------------------------------------------------
//    1.  aggregate and presets
//    2.  counter width (the byte-budget knob)
//    3.  alignment floor and default
//    4.  accounting
//    5.  hardening: poison, redzone, zeroing
//    6.  argument policy and assertion level
//    7.  linkage model
//    8.  C++ notation tier
//    9.  validation
//   10.  derived values
// ---------------------------------------------------------------------------


// ===========================================================================
//  1.  AGGREGATE AND PRESETS
// ===========================================================================

// D_CFG_MEM_ALL
//   brief: aggregate fallback for every optional memory feature. Not defined
// by default -- when the user defines it, each child knob whose own value is
// unset falls back to it. Set it to 0 for the smallest allocator that still
// allocates, or 1 to opt every optional feature in. Individual knobs always
// outrank it.

// D_CFG_MEM_PRESET_MINIMAL
//   brief: 1 selects the freestanding profile -- no accounting, no poisoning,
// no redzones, no zeroing, no reallocate slot, and the null source as the
// default upstream. Intended for a build that supplies its own memory and
// wants the allocator structs as small as the algorithm allows. Implemented as
// guarded #defines rather than a forced block, so an explicit knob still wins.
#ifndef D_CFG_MEM_PRESET_MINIMAL
#   define D_CFG_MEM_PRESET_MINIMAL 0
#endif

// D_CFG_MEM_PRESET_DEBUG
//   brief: 1 selects the diagnostic profile -- accounting with peak tracking,
// poisoning on both edges, redzones, double-free detection, and strict
// argument checking. This is what D_CFG_TESTING turns on by default; the knob
// exists so a release build can be given the same treatment without also
// taking every other testing-mode change in the framework.
#ifndef D_CFG_MEM_PRESET_DEBUG
#   if D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_MEM_PRESET_DEBUG 1
#   else
#       define D_CFG_MEM_PRESET_DEBUG 0
#   endif
#endif

// D_CFG_MEM_PRESET_FULL
//   brief: 1 opts every optional memory feature in, including the ones that
// default off. Useful as a conformance-build switch: the widest surface is
// also the widest test target.
#ifndef D_CFG_MEM_PRESET_FULL
#   define D_CFG_MEM_PRESET_FULL 0
#endif

#if D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL)
#   ifndef D_CFG_MEM_STATS
#       define D_CFG_MEM_STATS 0
#   endif
#   ifndef D_CFG_MEM_POISON
#       define D_CFG_MEM_POISON 0
#   endif
#   ifndef D_CFG_MEM_REDZONE
#       define D_CFG_MEM_REDZONE 0
#   endif
#   ifndef D_CFG_MEM_ZERO_ON_ALLOC
#       define D_CFG_MEM_ZERO_ON_ALLOC 0
#   endif
#endif  // D_CFG_MEM_PRESET_MINIMAL

#if D_CFG_IS_ON(D_CFG_MEM_PRESET_DEBUG)
#   ifndef D_CFG_MEM_STATS
#       define D_CFG_MEM_STATS 1
#   endif
#   ifndef D_CFG_MEM_STATS_PEAK
#       define D_CFG_MEM_STATS_PEAK 1
#   endif
#   ifndef D_CFG_MEM_POISON
#       define D_CFG_MEM_POISON 1
#   endif
#   ifndef D_CFG_MEM_REDZONE
#       define D_CFG_MEM_REDZONE 1
#   endif
#endif  // D_CFG_MEM_PRESET_DEBUG

#if D_CFG_IS_ON(D_CFG_MEM_PRESET_FULL)
#   ifndef D_CFG_MEM_ALL
#       define D_CFG_MEM_ALL 1
#   endif
#endif  // D_CFG_MEM_PRESET_FULL


// ===========================================================================
//  2.  COUNTER WIDTH (the byte-budget knob)
// ===========================================================================
//   Every allocator in this subframework counts bytes and slots in one type,
// d_mem_size. Making that type narrower than size_t is the single largest
// lever on struct size: an arena carries five counters, a pool carries seven,
// and a slab carries them once per size class. On LP64 the 32-bit setting
// halves all of them.
//
//   This is a REPRESENTATION choice, not a policy one, so it is stated in bits
// rather than as a boolean, and the resulting type is published by
// mem_common.h as d_mem_size with D_MEM_SIZE_MAX beside it. Both languages
// compile the same setting, so it cannot break parity -- but it DOES change
// the wire layout, which is why layout assertions key off it.

// D_CFG_MEM_SIZE_NATIVE / _32 / _64
//   brief: the three admissible values of D_CFG_MEM_SIZE_BITS. NATIVE (0)
// tracks size_t, which is the only setting that can address whatever the
// platform can address. 32 and 64 pin the width, at the cost of capping any
// single allocator at 4 GiB (32) or of paying eight bytes per counter on a
// 32-bit target (64).
#define D_CFG_MEM_SIZE_NATIVE   0
#define D_CFG_MEM_SIZE_32       32
#define D_CFG_MEM_SIZE_64       64

// D_CFG_MEM_SIZE_BITS
//   brief: the width of d_mem_size. Defaults to NATIVE, because "degrade,
// never error" argues against a default that silently caps capacity; the
// narrow setting is a deliberate act.
#ifndef D_CFG_MEM_SIZE_BITS
#   define D_CFG_MEM_SIZE_BITS  D_CFG_MEM_SIZE_NATIVE
#endif

#if !D_CFG_IS_INT_LITERAL(D_CFG_MEM_SIZE_BITS)
    #error "D_CFG_MEM_SIZE_BITS must name one of its values; a misspelled name would read as 0"
#endif


// ===========================================================================
//  3.  ALIGNMENT FLOOR AND DEFAULT
// ===========================================================================

// D_CFG_MEM_DEFAULT_ALIGN
//   brief: the alignment applied when a caller passes 0 for an alignment
// argument. Defaults to 0, which mem_common.h resolves to the platform's
// max_align_t -- the only value that is correct for an allocation of unknown
// type. Set it to a smaller power of two (8, or 4) when every allocation's
// type is known to the caller and the padding matters; that is the setting
// that recovers the last few bytes of a densely packed arena.
#ifndef D_CFG_MEM_DEFAULT_ALIGN
#   define D_CFG_MEM_DEFAULT_ALIGN 0
#endif

// D_CFG_MEM_MIN_ALIGN
//   brief: the alignment floor. Every request is raised to at least this,
// whatever the caller asked for. Defaults to 1 (no floor), so the caller's
// request is honoured exactly. Raising it to the cache line is how a caller
// buys freedom from false sharing across an entire subsystem without editing
// a single call site -- and it costs padding on every allocation, which is
// why it is not the default.
#ifndef D_CFG_MEM_MIN_ALIGN
#   define D_CFG_MEM_MIN_ALIGN 1
#endif

// D_CFG_MEM_MAX_ALIGN
//   brief: the largest alignment any module will honour, as a power of two.
// Requests above it are rejected with D_MEM_ERR_INVALID rather than silently
// satisfied, because an over-aligned request that the backing source cannot
// meet is a corruption waiting to be blamed on the caller. Defaults to 4096
// (a page), which covers every over-aligned type in practice.
#ifndef D_CFG_MEM_MAX_ALIGN
#   define D_CFG_MEM_MAX_ALIGN 4096
#endif


// ===========================================================================
//  4.  ACCOUNTING
// ===========================================================================

// D_CFG_MEM_STATS
//   brief: 1 embeds struct d_mem_stats in every allocator and maintains it.
// This is a FIELD knob: off, the block is absent and the update calls compile
// to nothing, so an arena loses six counters. Follows D_CFG_TESTING, because
// accounting is a diagnostic and a release build should not pay for one it
// never reads.
#ifndef D_CFG_MEM_STATS
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_STATS D_CFG_MEM_ALL
#   elif D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_MEM_STATS 1
#   else
#       define D_CFG_MEM_STATS 0
#   endif
#endif

// D_CFG_MEM_STATS_PEAK
//   brief: 1 adds the high-water fields (peak live bytes, peak live count) to
// the accounting block. Separate from D_CFG_MEM_STATS because a peak costs a
// compare-and-store on the allocation hot path where the running totals cost
// only an add, and a caller sampling totals periodically does not need one.
#ifndef D_CFG_MEM_STATS_PEAK
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_STATS_PEAK D_CFG_MEM_ALL
#   else
#       define D_CFG_MEM_STATS_PEAK 0
#   endif
#endif


// ===========================================================================
//  5.  HARDENING: POISON, REDZONE, ZEROING
// ===========================================================================

// D_CFG_MEM_POISON
//   brief: 1 fills freshly vended bytes with D_CFG_MEM_POISON_ALLOC_BYTE and
// released bytes with D_CFG_MEM_POISON_FREE_BYTE. A BRANCH knob -- it moves no
// struct field, only time -- so it is safe to leave on longer than the field
// knobs. Its value is that a read of uninitialised or freed memory produces a
// recognisable value rather than a plausible one.
#ifndef D_CFG_MEM_POISON
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_POISON D_CFG_MEM_ALL
#   elif D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_MEM_POISON 1
#   else
#       define D_CFG_MEM_POISON 0
#   endif
#endif

// D_CFG_MEM_POISON_ALLOC_BYTE / D_CFG_MEM_POISON_FREE_BYTE
//   brief: the two fill bytes. The defaults follow the MSVC debug-heap
// convention (0xCD for "clean memory", 0xDD for "dead memory") because those
// values are already recognised on sight by anyone who has debugged a heap,
// and because as pointers they land in unmapped space on every mainstream
// target.
#ifndef D_CFG_MEM_POISON_ALLOC_BYTE
#   define D_CFG_MEM_POISON_ALLOC_BYTE 0xCD
#endif
#ifndef D_CFG_MEM_POISON_FREE_BYTE
#   define D_CFG_MEM_POISON_FREE_BYTE  0xDD
#endif

// D_CFG_MEM_REDZONE
//   brief: 1 pads every vended block with a guard band on each side, filled
// with D_CFG_MEM_REDZONE_BYTE and verified on release. A FIELD knob at the
// allocation level: it changes the number of bytes each request consumes, so
// an arena's capacity in objects moves with it. Off by default outside the
// debug preset for exactly that reason.
#ifndef D_CFG_MEM_REDZONE
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_REDZONE D_CFG_MEM_ALL
#   else
#       define D_CFG_MEM_REDZONE 0
#   endif
#endif

// D_CFG_MEM_REDZONE_BYTES
//   brief: the width of ONE guard band, in bytes; a block therefore grows by
// twice this. Defaults to 8 -- wide enough to catch the single-element
// overrun that is the common case, narrow enough that it does not dominate a
// small slot. Rounded up to the block's alignment in use.
#ifndef D_CFG_MEM_REDZONE_BYTES
#   define D_CFG_MEM_REDZONE_BYTES 8
#endif

// D_CFG_MEM_REDZONE_BYTE
//   brief: the guard fill byte. Distinct from both poison bytes so that a
// dump distinguishes "wrote past the end" from "read uninitialised".
#ifndef D_CFG_MEM_REDZONE_BYTE
#   define D_CFG_MEM_REDZONE_BYTE 0xAB
#endif

// D_CFG_MEM_ZERO_ON_ALLOC
//   brief: 1 zeroes every vended block. Mutually exclusive with poisoning in
// effect (zero wins, since it is a correctness contract and poison is a
// diagnostic), and the two are resolved against each other in section 10 so
// no module has to. Off by default: a caller that needs zeroed memory can ask
// for it per call, and one that does not should not pay for a memset.
#ifndef D_CFG_MEM_ZERO_ON_ALLOC
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_ZERO_ON_ALLOC D_CFG_MEM_ALL
#   else
#       define D_CFG_MEM_ZERO_ON_ALLOC 0
#   endif
#endif


// ===========================================================================
//  6.  ARGUMENT POLICY AND ASSERTION LEVEL
// ===========================================================================

// D_CFG_MEM_STRICT_ARGS
//   brief: 0 (default) makes the core tolerate a null or malformed argument
// and report it through the return value -- a null pointer, or an explicit
// enum d_mem_status from the _ex forms. 1 makes it a hard assertion instead,
// for a build that would rather find the caller than absorb the mistake.
#ifndef D_CFG_MEM_STRICT_ARGS
#   if D_CFG_IS_ON(D_CFG_MEM_PRESET_DEBUG)
#       define D_CFG_MEM_STRICT_ARGS 1
#   else
#       define D_CFG_MEM_STRICT_ARGS 0
#   endif
#endif

// D_CFG_MEM_ASSERT_LAYOUT
//   brief: 1 emits the sizeof/offsetof assertions that make layout drift a
// compile error rather than a wire-format bug. Defaults ON because goals
// section 4 requires layout be asserted rather than assumed; 0 exists for a
// tier whose D_STATIC_ASSERT fallback is itself unavailable, not as a
// convenience.
#ifndef D_CFG_MEM_ASSERT_LAYOUT
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_ASSERT_LAYOUT D_CFG_MEM_ALL
#   else
#       define D_CFG_MEM_ASSERT_LAYOUT 1
#   endif
#endif

// D_CFG_MEM_ASSERT_SIZES_OFF_LP64
//   brief: 1 asserts the exact struct sizes even where the data model is not
// LP64. Defaults OFF: on a model with a different pointer or size_t width the
// sizes legitimately differ, and a hard failure there would breach "degrade,
// never error". Offsets are asserted unconditionally, since they are model
// independent.
#ifndef D_CFG_MEM_ASSERT_SIZES_OFF_LP64
#   define D_CFG_MEM_ASSERT_SIZES_OFF_LP64 0
#endif


// ===========================================================================
//  7.  LINKAGE MODEL
// ===========================================================================

// D_CFG_MEM_HEADER_ONLY
//   brief: 1 gives every header-defined kernel `static inline` linkage, so no
// .c file need be compiled or linked. 0 (the default) gives them `inline`,
// with each module's .c emitting exactly one out-of-line definition under C99
// inline rules -- which is what lets a non-inlined call and a taken address
// resolve. Mirrors D_COLOR_HEADER_ONLY; the two subframeworks answer the same
// question the same way on purpose.
#ifndef D_CFG_MEM_HEADER_ONLY
#   define D_CFG_MEM_HEADER_ONLY 0
#endif


// ===========================================================================
//  8.  C++ NOTATION TIER
// ===========================================================================

// D_CFG_MEM_USE_CONCEPTS
//   brief: 1 compiles the memory subframework's concept constraints. Left
// unset it follows env detection of C++20 concepts. Setting it to 0 on a tier
// that has them keeps the traits and drops the concepts, which is a legitimate
// choice for a codebase standardising on the trait spelling -- the concepts
// are notation over the traits, and dropping notation changes nothing about
// what is computed (goals section 2).
#ifndef D_CFG_MEM_USE_CONCEPTS
#   if defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                            \
       (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1)
#       define D_CFG_MEM_USE_CONCEPTS 1
#   else
#       define D_CFG_MEM_USE_CONCEPTS 0
#   endif
#endif

// D_CFG_MEM_TRAIT_DETECTORS
//   brief: 1 compiles the structural detection traits that let a caller
// substitute its own resource behind the C++ faces. Defaults ON; 0 trims a
// large amount of template instantiation from a build that never substitutes.
#ifndef D_CFG_MEM_TRAIT_DETECTORS
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_TRAIT_DETECTORS D_CFG_MEM_ALL
#   else
#       define D_CFG_MEM_TRAIT_DETECTORS 1
#   endif
#endif


// ===========================================================================
//  9.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL) &&                                 \
    !D_CFG_IS_OFF(D_CFG_MEM_PRESET_MINIMAL)
#   error "D_CFG_MEM_PRESET_MINIMAL must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_PRESET_DEBUG) &&                                   \
    !D_CFG_IS_OFF(D_CFG_MEM_PRESET_DEBUG)
#   error "D_CFG_MEM_PRESET_DEBUG must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_PRESET_FULL) &&                                    \
    !D_CFG_IS_OFF(D_CFG_MEM_PRESET_FULL)
#   error "D_CFG_MEM_PRESET_FULL must be 0 or 1"
#endif
#if D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL) &&                                  \
    D_CFG_IS_ON(D_CFG_MEM_PRESET_FULL)
#   error "D_CFG_MEM_PRESET_MINIMAL and _FULL are mutually exclusive"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_SIZE_BITS) != D_CFG_MEM_SIZE_NATIVE) &&           \
      (D_CFG_NORM(D_CFG_MEM_SIZE_BITS) != D_CFG_MEM_SIZE_32)     &&           \
      (D_CFG_NORM(D_CFG_MEM_SIZE_BITS) != D_CFG_MEM_SIZE_64) )
#   error "D_CFG_MEM_SIZE_BITS must be D_CFG_MEM_SIZE_NATIVE, _32 or _64"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_MIN_ALIGN) < 1) ||                                \
      ( (D_CFG_NORM(D_CFG_MEM_MIN_ALIGN) &                                    \
         (D_CFG_NORM(D_CFG_MEM_MIN_ALIGN) - 1)) != 0 ) )
#   error "D_CFG_MEM_MIN_ALIGN must be a power of two"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_DEFAULT_ALIGN) != 0) &&                           \
      ( (D_CFG_NORM(D_CFG_MEM_DEFAULT_ALIGN) &                                \
         (D_CFG_NORM(D_CFG_MEM_DEFAULT_ALIGN) - 1)) != 0 ) )
#   error "D_CFG_MEM_DEFAULT_ALIGN must be 0 or a power of two"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_MAX_ALIGN) < D_CFG_NORM(D_CFG_MEM_MIN_ALIGN)) ||  \
      ( (D_CFG_NORM(D_CFG_MEM_MAX_ALIGN) &                                    \
         (D_CFG_NORM(D_CFG_MEM_MAX_ALIGN) - 1)) != 0 ) )
#   error "D_CFG_MEM_MAX_ALIGN must be a power of two >= D_CFG_MEM_MIN_ALIGN"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_STATS) && !D_CFG_IS_OFF(D_CFG_MEM_STATS)
#   error "D_CFG_MEM_STATS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_STATS_PEAK) && !D_CFG_IS_OFF(D_CFG_MEM_STATS_PEAK)
#   error "D_CFG_MEM_STATS_PEAK must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_POISON) && !D_CFG_IS_OFF(D_CFG_MEM_POISON)
#   error "D_CFG_MEM_POISON must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_REDZONE) && !D_CFG_IS_OFF(D_CFG_MEM_REDZONE)
#   error "D_CFG_MEM_REDZONE must be 0 or 1"
#endif
#if D_CFG_IS_ON(D_CFG_MEM_REDZONE) && (D_CFG_NORM(D_CFG_MEM_REDZONE_BYTES) < 1)
#   error "D_CFG_MEM_REDZONE_BYTES must be >= 1 when redzones are enabled"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_ZERO_ON_ALLOC) &&                                  \
    !D_CFG_IS_OFF(D_CFG_MEM_ZERO_ON_ALLOC)
#   error "D_CFG_MEM_ZERO_ON_ALLOC must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_STRICT_ARGS) && !D_CFG_IS_OFF(D_CFG_MEM_STRICT_ARGS)
#   error "D_CFG_MEM_STRICT_ARGS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_ASSERT_LAYOUT) &&                                  \
    !D_CFG_IS_OFF(D_CFG_MEM_ASSERT_LAYOUT)
#   error "D_CFG_MEM_ASSERT_LAYOUT must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_HEADER_ONLY) && !D_CFG_IS_OFF(D_CFG_MEM_HEADER_ONLY)
#   error "D_CFG_MEM_HEADER_ONLY must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_USE_CONCEPTS) &&                                   \
    !D_CFG_IS_OFF(D_CFG_MEM_USE_CONCEPTS)
#   error "D_CFG_MEM_USE_CONCEPTS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_TRAIT_DETECTORS) &&                                \
    !D_CFG_IS_OFF(D_CFG_MEM_TRAIT_DETECTORS)
#   error "D_CFG_MEM_TRAIT_DETECTORS must be 0 or 1"
#endif


// ===========================================================================
// 10.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_MEM_SIZE_BITS
//   brief: the resolved counter width -- 0 for "track size_t", or 32 / 64.
// Read by mem_common.h, which turns it into d_mem_size and D_MEM_SIZE_MAX.
#define D_INTERNAL_MEM_SIZE_BITS      D_CFG_NORM(D_CFG_MEM_SIZE_BITS)

// D_INTERNAL_MEM_STATS
//   brief: 1 when every allocator should carry and maintain an accounting
// block. Read by mem_common.h (the struct), arena.h, pool.h and slab.h (the
// embedded field and the update calls).
#define D_INTERNAL_MEM_STATS          D_CFG_NORM(D_CFG_MEM_STATS)

// D_INTERNAL_MEM_STATS_PEAK
//   brief: 1 when the accounting block should carry high-water fields. Forced
// off when accounting itself is off, so a module never has to test both.
#if D_CFG_IS_ON(D_CFG_MEM_STATS) && D_CFG_IS_ON(D_CFG_MEM_STATS_PEAK)
#   define D_INTERNAL_MEM_STATS_PEAK  1
#else
#   define D_INTERNAL_MEM_STATS_PEAK  0
#endif

// D_INTERNAL_MEM_ZERO
//   brief: 1 when vended bytes should be zeroed. Read by every module's
// allocate path.
#define D_INTERNAL_MEM_ZERO           D_CFG_NORM(D_CFG_MEM_ZERO_ON_ALLOC)

// D_INTERNAL_MEM_POISON
//   brief: 1 when vended and released bytes should be filled with the poison
// values. Resolved AGAINST zeroing here rather than in five modules: zeroing
// is a contract the caller may rely on, poisoning is a diagnostic, so where
// both are asked for the contract wins on the allocate edge. The release edge
// is unaffected -- released bytes have no contract -- so poisoning on free
// survives, which is where most of its value is.
#if D_CFG_IS_ON(D_CFG_MEM_POISON) && D_CFG_IS_OFF(D_CFG_MEM_ZERO_ON_ALLOC)
#   define D_INTERNAL_MEM_POISON_ALLOC 1
#else
#   define D_INTERNAL_MEM_POISON_ALLOC 0
#endif
#define D_INTERNAL_MEM_POISON_FREE    D_CFG_NORM(D_CFG_MEM_POISON)
#define D_INTERNAL_MEM_POISON         D_CFG_NORM(D_CFG_MEM_POISON)

// D_INTERNAL_MEM_REDZONE
//   brief: 1 when blocks should carry guard bands. Read by arena.h and pool.h.
#define D_INTERNAL_MEM_REDZONE        D_CFG_NORM(D_CFG_MEM_REDZONE)

// D_INTERNAL_MEM_REDZONE_BYTES
//   brief: the width of one guard band, or 0 when redzones are off, so a
// module can add it unconditionally rather than branching.
#if D_CFG_IS_ON(D_CFG_MEM_REDZONE)
#   define D_INTERNAL_MEM_REDZONE_BYTES                                       \
        ((d_mem_size)(D_CFG_MEM_REDZONE_BYTES))
#else
#   define D_INTERNAL_MEM_REDZONE_BYTES ((d_mem_size)0)
#endif

// D_INTERNAL_MEM_MIN_ALIGN / D_INTERNAL_MEM_MAX_ALIGN
//   brief: the resolved alignment floor and ceiling, as d_mem_size values.
#define D_INTERNAL_MEM_MIN_ALIGN      ((d_mem_size)(D_CFG_MEM_MIN_ALIGN))
#define D_INTERNAL_MEM_MAX_ALIGN      ((d_mem_size)(D_CFG_MEM_MAX_ALIGN))

// D_INTERNAL_MEM_DEFAULT_ALIGN_CFG
//   brief: the configured default alignment, still possibly 0. mem_common.h
// resolves 0 to alignof(max_align_t), which cannot be spelled here because
// this file is pure preprocessor and that value is not.
#define D_INTERNAL_MEM_DEFAULT_ALIGN_CFG D_CFG_NORM(D_CFG_MEM_DEFAULT_ALIGN)

// D_INTERNAL_MEM_STRICT_ARGS
//   brief: 1 when a malformed argument should assert rather than be reported.
#define D_INTERNAL_MEM_STRICT_ARGS    D_CFG_NORM(D_CFG_MEM_STRICT_ARGS)

// D_INTERNAL_MEM_ASSERT_LAYOUT
//   brief: 1 when the shared headers should emit their layout assertions.
#define D_INTERNAL_MEM_ASSERT_LAYOUT  D_CFG_NORM(D_CFG_MEM_ASSERT_LAYOUT)

// D_INTERNAL_MEM_ASSERT_SIZES
//   brief: 1 when exact struct sizes should be asserted -- on LP64 with the
// native counter width, or anywhere the user has forced it. Offsets are
// asserted regardless. A narrowed counter changes every size, so the sizes
// are only pinned where the width is the platform's own.
#if D_CFG_IS_ON(D_CFG_MEM_ASSERT_LAYOUT)
#   if D_CFG_IS_ON(D_CFG_MEM_ASSERT_SIZES_OFF_LP64)
#       define D_INTERNAL_MEM_ASSERT_SIZES 1
#   elif defined(UINTPTR_MAX) && (UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu) &&      \
         (D_CFG_NORM(D_CFG_MEM_SIZE_BITS) == D_CFG_MEM_SIZE_NATIVE)
#       define D_INTERNAL_MEM_ASSERT_SIZES 1
#   else
#       define D_INTERNAL_MEM_ASSERT_SIZES 0
#   endif
#else
#   define D_INTERNAL_MEM_ASSERT_SIZES 0
#endif

// D_INTERNAL_MEM_HEADER_ONLY
//   brief: 1 when header kernels take `static inline` and no .c is required.
#define D_INTERNAL_MEM_HEADER_ONLY    D_CFG_NORM(D_CFG_MEM_HEADER_ONLY)

// D_INTERNAL_MEM_USE_CONCEPTS
//   brief: 1 when the C++ faces should compile their concept constraints.
#define D_INTERNAL_MEM_USE_CONCEPTS   D_CFG_NORM(D_CFG_MEM_USE_CONCEPTS)

// D_INTERNAL_MEM_TRAIT_DETECTORS
//   brief: 1 when the C++ faces should compile their structural detectors.
#define D_INTERNAL_MEM_TRAIT_DETECTORS                                        \
    D_CFG_NORM(D_CFG_MEM_TRAIT_DETECTORS)


#endif  // DJINTERP_CONFIG_CORE_MEMORY_CFG_MEM_COMMON_H
