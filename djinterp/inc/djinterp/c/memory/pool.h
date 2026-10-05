/*******************************************************************************
* djinterp [c]                                                            pool.h
*
* Fixed-slot pool -- the shared C core (tier 0):
*   An allocator that vends one size. Because every slot is the same, there is
* no size class to search, no header to store, and no fragmentation to manage:
* acquiring a slot is a pop, releasing one is a push, and both are O(1) with
* no arithmetic beyond an index.
*
*   THE POOL IS SIZE-PARAMETERIZED AT RUN TIME, not at compile time. That is
* the deliberate difference from a template, and it is what makes this the C
* core: one compiled implementation serves every element type in the program.
* The C++ face supplies the size and alignment as compile-time constants and
* is a wrapper of exactly zero bytes over this struct -- it computes the same
* geometry earlier, not differently.
*
* THREE RELEASE POLICIES, chosen per instance at init:
*
*   MONOTONIC     release is a no-op; slots come back only on reset. The
*                 fastest, and correct whenever the pool's contents all die
*                 together.
*
*   FREE_LIST     released slots are threaded onto a list, by INDEX, inside
*                 the slot's own storage. Acquire pops, release pushes. The
*                 policy most callers mean by "a pool".
*
*   GENERATIONAL  as free-list, plus a counter beside each slot that advances
*                 on every release. A handle carries the counter it was issued
*                 against, so resolving a handle whose slot has been reused
*                 reports D_MEM_ERR_STALE rather than handing back the new
*                 occupant. This converts a class of use-after-free from
*                 silent corruption into a checked failure, and it is what
*                 makes this usable as a slot map or an entity table.
*
* POINTER STABILITY:
*   ABSOLUTE. A pool grows by taking another block from its source; it never
* moves a slot it has handed out. Interior pointers, intrusive lists, and
* graphs built out of pooled nodes all stay valid across any amount of growth.
*
* WHAT THE POOL DOES NOT DO:
*   It does not construct or destroy anything. Slots are raw aligned storage,
* and object lifetime is the caller's -- or, in C++, the wrapper's. It is not
* thread-safe, for the same reason the arena is not: a lock on the acquire
* path would cost every single-threaded caller an atomic.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/memory/pool.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SCALARS AND TYPES
      -----------------
      a. d_pool_index / d_pool_generation
      b. enum d_pool_policy
      c. struct d_pool_handle
      d. struct d_pool_block
      e. struct d_pool
      f. struct d_pool_config
II.   LIFECYCLE
      ---------
      a. d_pool_config_default / d_pool_config_for
      b. d_pool_init / _reserve / _reset / _release
III.  ACQUIRE AND RELEASE
      -------------------
      a. d_pool_acquire / _acquire_ex
      b. d_pool_release_slot / _release_slot_ex
IV.   HANDLES
      -------
      a. d_pool_acquire_handle / _resolve / _release_handle
      b. d_pool_handle_null / _handle_is_null / _handle_is_live
V.    ADDRESSING
      ----------
      a. d_pool_at / _index_of
VI.   ITERATION
      ---------
      a. d_pool_first / _next
VII.  QUERIES
      -------
      a. d_pool_size / _capacity / _slot_size / _slot_align / _block_count
      b. d_pool_owns / _utilization / _stats
*/

#ifndef DJINTERP_C_MEMORY_POOL_H
#define DJINTERP_C_MEMORY_POOL_H 1

// djinterp
#include "./mem_common.h"
#include "./mem_source.h"
#include "../../config/core/memory/cfg_pool.h"


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///                    I.   SCALARS AND TYPES                               ///
///////////////////////////////////////////////////////////////////////////////

// d_pool_index
//   type: names one slot of one pool. Its width is D_CFG_POOL_INDEX_BITS,
// and it is also the free-list link stored inside a free slot -- so it is
// simultaneously the pool's addressing range and its minimum slot size.
#if (D_INTERNAL_POOL_INDEX_BITS == 16)
    typedef uint16_t d_pool_index;
#   define D_POOL_INDEX_MAX         ((d_pool_index)UINT16_MAX)
#elif (D_INTERNAL_POOL_INDEX_BITS == 64)
    typedef uint64_t d_pool_index;
#   define D_POOL_INDEX_MAX         ((d_pool_index)UINT64_MAX)
#else
    typedef uint32_t d_pool_index;
#   define D_POOL_INDEX_MAX         ((d_pool_index)UINT32_MAX)
#endif

// D_POOL_INDEX_NONE
//   constant: the index that names no slot. The free list terminates on it,
// and a null handle carries it. Taken as the maximum rather than zero, so
// that slot zero is an ordinary slot and no index arithmetic needs a bias.
#define D_POOL_INDEX_NONE           D_POOL_INDEX_MAX

// d_pool_generation
//   type: the reuse counter stored beside a generational pool's slot. Its
// width is D_CFG_POOL_GENERATION_BITS, and it is paid on every slot of such a
// pool -- see the note in cfg_pool.h about why 16 bits is usually right.
#if (D_INTERNAL_POOL_GENERATION_BITS == 8)
    typedef uint8_t  d_pool_generation;
#elif (D_INTERNAL_POOL_GENERATION_BITS == 32)
    typedef uint32_t d_pool_generation;
#elif (D_INTERNAL_POOL_GENERATION_BITS == 64)
    typedef uint64_t d_pool_generation;
#else
    typedef uint16_t d_pool_generation;
#endif

// d_pool_policy
//   enum: what release() does with a slot. Declared whatever the build
// compiles, so code naming a policy still compiles where that policy is
// absent -- d_pool_init reports D_MEM_ERR_INVALID rather than failing to
// build. Degrade, never error.
//
//   NAMED `policy` RATHER THAN `release` DELIBERATELY, and the reason is a
// parity hazard worth recording. C keeps enum TAGS in a separate namespace
// from ordinary identifiers, so `enum d_pool_release` and a function
// `d_pool_release()` coexist happily -- the header compiles clean as C. C++
// has no such separation: the function name hides the type, `::d_pool_release`
// names the function, and every declaration using the type fails. One
// declaration would then have meant two different things in the two
// languages, which is precisely what the parity law forbids. Renaming the
// enum costs nothing and removes the collision at the source.
enum d_pool_policy
{
    // release is a no-op; slots return only on reset. No free list, no
    // generation, and therefore the smallest possible slot
    D_POOL_POLICY_MONOTONIC    = 0,

    // released slots are threaded onto an index-linked free list inside
    // their own storage and reused by the next acquire
    D_POOL_POLICY_FREE_LIST    = 1,

    // as free-list, plus a per-slot generation counter, so that a handle
    // outliving its slot is detected rather than silently redirected
    D_POOL_POLICY_GENERATIONAL = 2
};

// d_pool_handle
//   struct: a generation-checked reference to a slot. Two integers, copied by
// value, and -- unlike a pointer -- SAFE TO STORE ACROSS A RELEASE, because
// resolving it after the slot has been reused reports D_MEM_ERR_STALE instead
// of returning the new occupant.
//   A handle is also relocatable: it names a slot by index, so a pool that
// has been moved, serialized, or reloaded still answers the same handles.
struct d_pool_handle
{
    d_pool_index      index;
    d_pool_generation generation;
};

// d_pool_block
//   struct: the header of one block of slots, placed at the front of the
// block obtained from the upstream source. The slots follow it, aligned.
//   EVERY BLOCK IN A POOL HOLDS EXACTLY THE SAME NUMBER OF SLOTS. That is
// what turns "which block holds slot n" into a division instead of a walk,
// and it is why the count is here for checking rather than for arithmetic.
struct d_pool_block
{
    struct d_pool_block* next;         // the next block, or NULL
    d_pool_index         first_index;  // the index of this block's first slot
    d_pool_index         slot_count;   // slots in this block
};

// d_pool
//   struct: a fixed-slot allocator over one or more blocks.
struct d_pool
{
    // where blocks come from. Resolved at init
    struct d_mem_source   source;

    struct d_pool_block*  head;             // first block
    struct d_pool_block*  tail;             // the block the bump cursor is in

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    // block pointers by block number, so that slot index -> address is a
    // division and an index rather than a walk. Grown by doubling
    struct d_pool_block** blocks;
    d_mem_size            block_capacity;   // entries the table can hold
#endif

    // --- slot geometry, computed once at init ---
    d_mem_size            stride;           // bytes from one slot to the next
    d_mem_size            payload;          // caller-visible bytes per slot
    d_mem_size            align;            // slot alignment
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    d_mem_size            generation_offset; // where the counter sits in a slot
#endif

    // --- population ---
    d_pool_index          slots_per_block;
    d_pool_index          capacity;         // slots that exist
    d_pool_index          live;             // slots currently acquired
    d_pool_index          bump;             // next never-yet-used slot index
    d_pool_index          max_slots;        // hard ceiling; 0 means unbounded
    d_mem_size            block_count;

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    d_pool_index          free_head;        // first free slot, or _INDEX_NONE
    d_pool_index          free_count;
#endif

    uint8_t               policy;           // enum d_pool_policy, narrowed

#if (D_INTERNAL_MEM_STATS == 1)
    struct d_mem_stats    stats;
#endif
};

// d_pool_config
//   struct: how one pool differs from the configured defaults. The two size
// fields are MANDATORY -- a pool with no slot size is not a pool -- and
// everything else may be zero.
struct d_pool_config
{
    // the caller-visible bytes in one slot. Must be non-zero
    d_mem_size          slot_size;

    // the alignment each slot must satisfy; 0 means the configured default.
    // Raised where the free list needs more, which is the one place the pool
    // may hand back a stricter alignment than was asked for -- never a weaker
    // one
    d_mem_size          slot_align;

    // where blocks come from. Unset (zeroed) means the configured default
    struct d_mem_source source;

    // slots in every block; 0 means initial_slots, or the configured default.
    // Note that this fixes the size of EVERY block, not just the first
    d_pool_index        slots_per_block;

    // slots to reserve at init; 0 reserves nothing, so a pool that is created
    // and never used costs its struct and no more
    d_pool_index        initial_slots;

    // a ceiling on total slots; 0 means unbounded. The knob that turns a pool
    // into a budget: past it, acquire reports exhaustion rather than growing
    d_pool_index        max_slots;

    // what release() does with a slot
    enum d_pool_policy policy;
};

#if (D_INTERNAL_POOL_ITERATE == 1)

// d_pool_cursor
//   struct: a position in a slot walk. A VALUE THE CALLER HOLDS, so iteration
// costs the pool nothing and several walks may run at once.
struct d_pool_cursor
{
    struct d_pool_block* block;
    d_pool_index         index;        // global slot index
    d_pool_index         offset;       // slot number within the block
};

#endif  // D_INTERNAL_POOL_ITERATE


// I.    lifecycle
struct d_pool_config d_pool_config_default(d_mem_size _slot_size,
                                           d_mem_size _slot_align);
enum d_mem_status    d_pool_init(struct d_pool*              _pool,
                                 const struct d_pool_config* _config);
enum d_mem_status    d_pool_reserve(struct d_pool* _pool,
                                    d_pool_index   _slots);
void                 d_pool_reset(struct d_pool* _pool);
void                 d_pool_release(struct d_pool* _pool);

// II.   acquire and release
void*                d_pool_acquire(struct d_pool* _pool);
enum d_mem_status    d_pool_acquire_ex(struct d_pool*      _pool,
                                       struct d_mem_block* _out);
enum d_mem_status    d_pool_release_slot(struct d_pool* _pool,
                                         void*          _slot);

// III.  handles
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
struct d_pool_handle d_pool_handle_null(void);
bool                 d_pool_handle_is_null(struct d_pool_handle _handle);
enum d_mem_status    d_pool_acquire_handle(struct d_pool*        _pool,
                                           struct d_pool_handle* _out);
void*                d_pool_resolve(const struct d_pool* _pool,
                                    struct d_pool_handle _handle);
enum d_mem_status    d_pool_resolve_ex(const struct d_pool* _pool,
                                       struct d_pool_handle _handle,
                                       void**               _out);
enum d_mem_status    d_pool_release_handle(struct d_pool*       _pool,
                                           struct d_pool_handle _handle);
bool                 d_pool_handle_is_live(const struct d_pool* _pool,
                                           struct d_pool_handle _handle);
#endif

// IV.   addressing
void*                d_pool_at(const struct d_pool* _pool,
                               d_pool_index         _index);
d_pool_index         d_pool_index_of(const struct d_pool* _pool,
                                     const void*          _slot);

// V.    iteration
#if (D_INTERNAL_POOL_ITERATE == 1)
struct d_pool_cursor d_pool_first(const struct d_pool* _pool);
bool                 d_pool_cursor_valid(const struct d_pool*        _pool,
                                         const struct d_pool_cursor* _cursor);
void*                d_pool_cursor_slot(const struct d_pool*        _pool,
                                        const struct d_pool_cursor* _cursor);
void                 d_pool_next(const struct d_pool*  _pool,
                                 struct d_pool_cursor* _cursor);
#endif

// VI.   queries
bool                 d_pool_is_valid(const struct d_pool* _pool);
d_pool_index         d_pool_size(const struct d_pool* _pool);
d_pool_index         d_pool_capacity(const struct d_pool* _pool);
d_pool_index         d_pool_available(const struct d_pool* _pool);
d_mem_size           d_pool_slot_size(const struct d_pool* _pool);
d_mem_size           d_pool_slot_align(const struct d_pool* _pool);
d_mem_size           d_pool_block_count(const struct d_pool* _pool);
d_mem_size           d_pool_bytes_allocated(const struct d_pool* _pool);
double               d_pool_utilization(const struct d_pool* _pool);
enum d_mem_status    d_pool_stats(const struct d_pool* _pool,
                                  struct d_mem_stats*  _out);
#if (D_INTERNAL_POOL_OWNS == 1)
bool                 d_pool_owns(const struct d_pool* _pool,
                                 const void*          _slot);
#endif

// VII.  geometry helpers (usable before a pool exists)
d_mem_size           d_pool_stride_for(d_mem_size         _slot_size,
                                       d_mem_size         _slot_align,
                                       enum d_pool_policy _policy);
d_mem_size           d_pool_align_for(d_mem_size         _slot_align,
                                      enum d_pool_policy _policy);


///////////////////////////////////////////////////////////////////////////////
///                    VIII.   LAYOUT ASSERTIONS                            ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_pool, source) == 0,
                "d_pool.source must lead the struct");

D_STATIC_ASSERT(offsetof(struct d_pool_block, next) == 0,
                "d_pool_block.next must lead the header");

D_STATIC_ASSERT(offsetof(struct d_pool_handle, index) == 0,
                "d_pool_handle.index must lead the handle");

//   A handle is two integers and must stay that way: it is copied by value
// everywhere, and it is the form that would go on a wire if a pool were ever
// serialized.
D_STATIC_ASSERT(sizeof(struct d_pool_handle) <=
                    (2 * sizeof(d_pool_index)),
                "d_pool_handle has grown beyond two indices");

//   The block header is the pool's only per-block cost. Asserting it makes an
// added field a deliberate act rather than an accident.
D_STATIC_ASSERT(sizeof(struct d_pool_block) <=
                    (sizeof(void*) + (2 * sizeof(d_pool_index)) +
                     sizeof(void*)),
                "d_pool_block header has grown beyond its budget");

#endif  // D_INTERNAL_MEM_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // DJINTERP_C_MEMORY_POOL_H
