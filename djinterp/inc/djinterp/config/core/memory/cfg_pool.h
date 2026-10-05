/*******************************************************************************
* djinterp [config]                                                   cfg_pool.h
*
*   Configuration for the fixed-slot pool. Selects which release policies are
* compiled at all, how free slots are threaded, how wide a slot index and a
* generation counter are, whether the block table that makes handle
* resolution O(1) exists, and how large a block is by default.
*
*   THE TWO KNOBS THAT MOVE THE MOST BYTES:
*
*     D_CFG_POOL_INDEX_BITS sets the width of every slot index -- which is the
*   free-list link stored inside each free slot, so it is also the pool's
*   MINIMUM SLOT SIZE. At 32 bits a pool of 8-byte objects has 8-byte slots;
*   at 64 it still has 8-byte slots, but a pool of 4-byte objects has 8-byte
*   slots at 64 bits and 4-byte slots at 32. Halving the slot size of a
*   million-slot pool is four megabytes.
*
*     D_CFG_POOL_GENERATION_BITS sets the width of the per-slot generation
*   counter that a generational pool stores beside each payload. It is paid on
*   every slot of such a pool, and 16 bits is usually enough: the counter
*   exists to catch a stale handle, and a handle that survives 65536
*   reuses of one slot is not a bug this mechanism was ever going to find.
*
*   WHY THE FREE LIST IS THREADED BY INDEX AND NOT BY POINTER: an index-linked
* free list is smaller (see above), and it is RELOCATABLE -- a pool whose
* internal links are offsets can be memcpy'd, written to a file, and read back
* somewhere else. That is the framework's standing offsets-not-pointers
* decision applied to the one data structure in this subframework that has a
* choice about it.
*
*   targets:  core/memory/pool.h, pool.c  ->  D_INTERNAL_POOL_FREE_LIST,
*             D_INTERNAL_POOL_GENERATIONAL, D_INTERNAL_POOL_INDEXED_LINKS,
*             D_INTERNAL_POOL_INDEX_BITS, D_INTERNAL_POOL_GENERATION_BITS,
*             D_INTERNAL_POOL_BLOCK_TABLE, D_INTERNAL_POOL_SLOTS_PER_BLOCK,
*             D_INTERNAL_POOL_DOUBLE_FREE_CHECK, D_INTERNAL_POOL_OWNS,
*             D_INTERNAL_POOL_ITERATE
*   requires: cfg_mem_common.h (vocabulary knobs, presets); cfg_mem_source.h
*
*
* path:      /inc/djinterp/config/core/memory/cfg_pool.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_MEMORY_CFG_POOL_H
#define DJINTERP_CONFIG_CORE_MEMORY_CFG_POOL_H 1

// (0) subframework root first, then the dependency whose defaults this file's
//     own defaults read.
#include "./cfg_mem_common.h"
#include "./cfg_mem_source.h"


// ---------------------------------------------------------------------------
//  contents
// ---------------------------------------------------------------------------
//    1.  which release policies exist
//    2.  index and generation width
//    3.  free-list threading
//    4.  block geometry and the block table
//    5.  optional capabilities
//    6.  validation
//    7.  derived values
// ---------------------------------------------------------------------------


// ===========================================================================
//  1.  WHICH RELEASE POLICIES EXIST
// ===========================================================================
//   A pool's release policy is chosen per instance, at init -- so a program
// may hold a monotonic pool and a generational one at once. These knobs decide
// which policies are COMPILED; asking for one that is not returns
// D_MEM_ERR_INVALID rather than silently taking a different one.
//
//   The monotonic policy is unconditional. It is the policy with no
// bookkeeping at all, so removing it would save nothing and would leave a pool
// with no guaranteed answer.

// D_CFG_POOL_FREE_LIST
//   brief: 1 compiles the free-list release policy -- released slots are
// threaded onto a list and reused by the next acquire. This is what most
// callers mean by "a pool", so it defaults ON everywhere including MINIMAL.
#ifndef D_CFG_POOL_FREE_LIST
#   define D_CFG_POOL_FREE_LIST 1
#endif

// D_CFG_POOL_GENERATIONAL
//   brief: 1 compiles the generational release policy -- each slot carries a
// counter that advances on every release, and a handle carries the counter it
// was issued against, so resolving a handle to a slot that has since been
// reused reports D_MEM_ERR_STALE instead of returning the new occupant.
//   This turns a whole class of use-after-free from silent corruption into a
// checked failure, which is why it exists; it costs the counter on every slot
// and the block table below, which is why it is a knob. Defaults ON outside
// MINIMAL.
#ifndef D_CFG_POOL_GENERATIONAL
#   if D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL)
#       define D_CFG_POOL_GENERATIONAL 0
#   else
#       define D_CFG_POOL_GENERATIONAL 1
#   endif
#endif


// ===========================================================================
//  2.  INDEX AND GENERATION WIDTH
// ===========================================================================

// D_CFG_POOL_INDEX_BITS
//   brief: the width of a slot index, in bits: 16, 32 or 64. This is also the
// free-list link and therefore the pool's minimum slot size, so it is the
// knob that decides whether a pool of small objects is dense or padded.
//   32 by default, which caps one pool at about four billion slots. 16 is a
// real choice for an embedded pool of a few hundred entries -- it makes the
// minimum slot two bytes -- and 64 exists for the pool that genuinely needs
// more slots than a 32-bit index can name.
#ifndef D_CFG_POOL_INDEX_BITS
#   define D_CFG_POOL_INDEX_BITS 32
#endif

// D_CFG_POOL_GENERATION_BITS
//   brief: the width of a slot's generation counter, in bits: 8, 16, 32 or
// 64. Paid on every slot of a generational pool.
//   16 by default. The counter's job is to catch a handle held across a
// release, and it fails only when a slot is reused exactly 2^n times between
// the handle being taken and being used -- which at 16 bits is 65536 reuses
// of one specific slot, and a program doing that has a different problem. 8
// is defensible for a pool that is reset rather than recycled; 32 matches the
// conventional slot-map and is the setting to take if a handle may be
// persisted.
#ifndef D_CFG_POOL_GENERATION_BITS
#   define D_CFG_POOL_GENERATION_BITS 16
#endif


// ===========================================================================
//  3.  FREE-LIST THREADING
// ===========================================================================

// D_CFG_POOL_INDEXED_LINKS
//   brief: 1 threads the free list by slot INDEX, 0 by pointer.
//   Indexed is the default and is better on both counts that matter: the link
// is D_CFG_POOL_INDEX_BITS wide rather than a whole pointer, and a pool whose
// internal links are offsets is relocatable -- memcpy it, write it out, read
// it back at a different address, and the free list is still correct.
//   Pointer links exist for the pool that needs more slots than the configured
// index can name and does not want to widen the index for every other pool in
// the program.
#ifndef D_CFG_POOL_INDEXED_LINKS
#   define D_CFG_POOL_INDEXED_LINKS 1
#endif


// ===========================================================================
//  4.  BLOCK GEOMETRY AND THE BLOCK TABLE
// ===========================================================================

// D_CFG_POOL_SLOTS_PER_BLOCK
//   brief: how many slots one block holds, when a pool's configuration names
// no other number. EVERY BLOCK IN A POOL HOLDS EXACTLY THIS MANY, which is
// what makes an index-to-block computation a division rather than a walk.
//   256 by default: large enough that the upstream call is amortised, small
// enough that a pool created for a handful of objects does not reserve a page.
#ifndef D_CFG_POOL_SLOTS_PER_BLOCK
#   define D_CFG_POOL_SLOTS_PER_BLOCK 256
#endif

// D_CFG_POOL_BLOCK_TABLE
//   brief: 1 maintains an array of block pointers alongside the block chain,
// so that resolving a slot index to an address is a division and an index
// rather than a walk of the chain.
//   REQUIRED BY THE GENERATIONAL POLICY, and forced on below when that policy
// is compiled -- resolving a handle is the generational pool's whole purpose,
// and an O(blocks) resolve would make it useless at the sizes it is for. For
// a monotonic or free-list pool the table buys only d_pool_at and
// d_pool_index_of, so it is optional there.
//   Costs one pointer array, grown by doubling from the pool's own source.
#ifndef D_CFG_POOL_BLOCK_TABLE
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_POOL_BLOCK_TABLE D_CFG_MEM_ALL
#   else
#       define D_CFG_POOL_BLOCK_TABLE 1
#   endif
#endif


// ===========================================================================
//  5.  OPTIONAL CAPABILITIES
// ===========================================================================

// D_CFG_POOL_DOUBLE_FREE_CHECK
//   brief: 1 walks the free list on every release to confirm the slot is not
// already on it.
//   O(free slots) PER RELEASE -- this is a testing-mode check and nothing
// else. Its value is that a double free into a free-list pool otherwise
// produces a cycle in the list, after which two callers are handed the same
// slot and the corruption appears arbitrarily far from its cause. Follows
// D_CFG_TESTING.
#ifndef D_CFG_POOL_DOUBLE_FREE_CHECK
#   if D_CFG_IS_ON(D_CFG_TESTING)
#       define D_CFG_POOL_DOUBLE_FREE_CHECK 1
#   else
#       define D_CFG_POOL_DOUBLE_FREE_CHECK 0
#   endif
#endif

// D_CFG_POOL_OWNS
//   brief: 1 compiles d_pool_owns, which answers whether a pointer names a
// slot of this pool. O(blocks) without the block table, O(blocks) with it
// too -- the table indexes by slot, not by address -- so it stays a
// diagnostic. Defaults ON.
#ifndef D_CFG_POOL_OWNS
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_POOL_OWNS D_CFG_MEM_ALL
#   else
#       define D_CFG_POOL_OWNS 1
#   endif
#endif

// D_CFG_POOL_ITERATE
//   brief: 1 compiles the slot walk (d_pool_first / d_pool_next), which
// visits every slot the pool has ever handed out, in index order.
//   NOTE WHAT IT CANNOT DO: a pool holds raw storage and does not know which
// slots are live, so the walk visits released slots too unless the pool is
// generational (where a released slot is recognisable by its counter). The
// header says so at the call site; this knob only decides whether the walk
// exists. Defaults ON.
#ifndef D_CFG_POOL_ITERATE
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_POOL_ITERATE D_CFG_MEM_ALL
#   else
#       define D_CFG_POOL_ITERATE 1
#   endif
#endif


// ===========================================================================
//  6.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_POOL_FREE_LIST) && !D_CFG_IS_OFF(D_CFG_POOL_FREE_LIST)
#   error "D_CFG_POOL_FREE_LIST must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_POOL_GENERATIONAL) &&                                  \
    !D_CFG_IS_OFF(D_CFG_POOL_GENERATIONAL)
#   error "D_CFG_POOL_GENERATIONAL must be 0 or 1"
#endif
#if ( (D_CFG_NORM(D_CFG_POOL_INDEX_BITS) != 16) &&                            \
      (D_CFG_NORM(D_CFG_POOL_INDEX_BITS) != 32) &&                            \
      (D_CFG_NORM(D_CFG_POOL_INDEX_BITS) != 64) )
#   error "D_CFG_POOL_INDEX_BITS must be 16, 32 or 64"
#endif
#if ( (D_CFG_NORM(D_CFG_POOL_GENERATION_BITS) != 8)  &&                       \
      (D_CFG_NORM(D_CFG_POOL_GENERATION_BITS) != 16) &&                       \
      (D_CFG_NORM(D_CFG_POOL_GENERATION_BITS) != 32) &&                       \
      (D_CFG_NORM(D_CFG_POOL_GENERATION_BITS) != 64) )
#   error "D_CFG_POOL_GENERATION_BITS must be 8, 16, 32 or 64"
#endif
#if !D_CFG_IS_ON(D_CFG_POOL_INDEXED_LINKS) &&                                 \
    !D_CFG_IS_OFF(D_CFG_POOL_INDEXED_LINKS)
#   error "D_CFG_POOL_INDEXED_LINKS must be 0 or 1"
#endif
#if (D_CFG_NORM(D_CFG_POOL_SLOTS_PER_BLOCK) < 1)
#   error "D_CFG_POOL_SLOTS_PER_BLOCK must be >= 1"
#endif
#if !D_CFG_IS_ON(D_CFG_POOL_BLOCK_TABLE) &&                                   \
    !D_CFG_IS_OFF(D_CFG_POOL_BLOCK_TABLE)
#   error "D_CFG_POOL_BLOCK_TABLE must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_POOL_DOUBLE_FREE_CHECK) &&                             \
    !D_CFG_IS_OFF(D_CFG_POOL_DOUBLE_FREE_CHECK)
#   error "D_CFG_POOL_DOUBLE_FREE_CHECK must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_POOL_OWNS) && !D_CFG_IS_OFF(D_CFG_POOL_OWNS)
#   error "D_CFG_POOL_OWNS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_POOL_ITERATE) && !D_CFG_IS_OFF(D_CFG_POOL_ITERATE)
#   error "D_CFG_POOL_ITERATE must be 0 or 1"
#endif


// ===========================================================================
//  7.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_POOL_FREE_LIST
//   brief: 1 when the free-list release policy is compiled. Forced on when
// the generational policy is, since a generational pool recycles through the
// same list -- resolved here so pool.c never tests both.
#if D_CFG_IS_ON(D_CFG_POOL_FREE_LIST) || D_CFG_IS_ON(D_CFG_POOL_GENERATIONAL)
#   define D_INTERNAL_POOL_FREE_LIST 1
#else
#   define D_INTERNAL_POOL_FREE_LIST 0
#endif

// D_INTERNAL_POOL_GENERATIONAL
//   brief: 1 when the generational release policy is compiled.
#define D_INTERNAL_POOL_GENERATIONAL  D_CFG_NORM(D_CFG_POOL_GENERATIONAL)

// D_INTERNAL_POOL_INDEX_BITS / _GENERATION_BITS
//   brief: the resolved widths, read by pool.h to pick d_pool_index and
// d_pool_generation.
#define D_INTERNAL_POOL_INDEX_BITS    D_CFG_NORM(D_CFG_POOL_INDEX_BITS)
#define D_INTERNAL_POOL_GENERATION_BITS                                       \
    D_CFG_NORM(D_CFG_POOL_GENERATION_BITS)

// D_INTERNAL_POOL_INDEXED_LINKS
//   brief: 1 when the free list is threaded by index rather than by pointer.
#define D_INTERNAL_POOL_INDEXED_LINKS D_CFG_NORM(D_CFG_POOL_INDEXED_LINKS)

// D_INTERNAL_POOL_BLOCK_TABLE
//   brief: 1 when the pool maintains its block-pointer array. Forced on by
// the generational policy, whose handle resolution would otherwise walk the
// chain on every dereference.
#if D_CFG_IS_ON(D_CFG_POOL_BLOCK_TABLE) || D_CFG_IS_ON(D_CFG_POOL_GENERATIONAL)
#   define D_INTERNAL_POOL_BLOCK_TABLE 1
#else
#   define D_INTERNAL_POOL_BLOCK_TABLE 0
#endif

// D_INTERNAL_POOL_SLOTS_PER_BLOCK
//   brief: the default slots per block, as a d_mem_size.
#define D_INTERNAL_POOL_SLOTS_PER_BLOCK                                       \
    ((d_mem_size)(D_CFG_POOL_SLOTS_PER_BLOCK))

// D_INTERNAL_POOL_DOUBLE_FREE_CHECK
//   brief: 1 when release should verify the slot is not already free. Forced
// off where there is no free list to walk.
#if D_INTERNAL_POOL_FREE_LIST && D_CFG_IS_ON(D_CFG_POOL_DOUBLE_FREE_CHECK)
#   define D_INTERNAL_POOL_DOUBLE_FREE_CHECK 1
#else
#   define D_INTERNAL_POOL_DOUBLE_FREE_CHECK 0
#endif

// D_INTERNAL_POOL_OWNS / _ITERATE
//   brief: 1 when the corresponding optional capability is compiled.
#define D_INTERNAL_POOL_OWNS          D_CFG_NORM(D_CFG_POOL_OWNS)
#define D_INTERNAL_POOL_ITERATE       D_CFG_NORM(D_CFG_POOL_ITERATE)


#endif  // DJINTERP_CONFIG_CORE_MEMORY_CFG_POOL_H
