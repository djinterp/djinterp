/*******************************************************************************
* djinterp [c]                                                           arena.h
*
* Monotonic bump arena -- the shared C core (tier 0):
*   The simplest allocator that is still general: a pointer that moves
* forward. allocate() aligns the cursor, returns it, and advances; there is no
* per-allocation header, no free list, and no size class, so an allocation
* costs an add and a compare and an arena's overhead per object is exactly the
* alignment padding the object itself demanded.
*
*   Individual release is NOT SUPPORTED, and that is the trade being made
* rather than a limitation being tolerated. Everything comes back at once, by
* one of three routes:
*     reset()   - rewind every region, keep the memory for reuse
*     rewind()  - rewind to a mark, so the arena is a stack allocator
*     release() - give every region back to the upstream source
*
*   WHEN THE ARENA IS THE RIGHT ANSWER: a phase with a known end. A parse, a
* frame, a request, a compilation unit. The allocations within it have wildly
* varying lifetimes on paper and exactly one lifetime in practice -- the
* phase's -- so tracking them individually is bookkeeping that buys nothing.
*
* POINTER STABILITY:
*   Chained arenas NEVER move a byte they have handed out. Growth adds a
* region; it does not reallocate an existing one. So a tree, a graph, or any
* structure holding interior pointers can be built directly in an arena, which
* is the property that distinguishes this from a growable buffer and is why
* D_CFG_ARENA_CHAIN defaults on.
*
* COMPOSITION:
*   d_arena_as_source presents an arena as a d_mem_source, so a pool or a slab
* can draw its blocks from one. One upstream call at startup, an arena over
* it, and every other allocator in the program carved out of that -- with no
* allocator in the chain aware of the arrangement.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/memory/arena.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TYPES
      -----
      a. enum d_arena_growth
      b. struct d_arena_region
      c. struct d_arena
      d. struct d_arena_config
      e. struct d_arena_mark
II.   LIFECYCLE
      ---------
      a. d_arena_config_default / d_arena_init / _init_buffer
      b. d_arena_reset / _trim / _release
III.  ALLOCATION
      ----------
      a. d_arena_allocate / _allocate_ex
      b. d_arena_allocate_array
      c. d_arena_duplicate / _duplicate_string
      d. d_arena_extend / _pop_last
IV.   MARK AND REWIND
      ---------------
      a. d_arena_mark_get / _rewind
V.    QUERIES
      -------
      a. d_arena_owns / _used / _capacity / _remaining / _region_count
      b. d_arena_stats
VI.   COMPOSITION
      -----------
      a. d_arena_as_source
*/

#ifndef DJINTERP_C_MEMORY_ARENA_H
#define DJINTERP_C_MEMORY_ARENA_H 1

// djinterp
#include "./mem_common.h"
#include "./mem_source.h"
#include "../../config/core/memory/cfg_arena.h"


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///                          I.   TYPES                                     ///
///////////////////////////////////////////////////////////////////////////////

// d_arena_growth
//   enum: how a chained arena sizes each next region. Declared whatever
// D_CFG_ARENA_CHAIN says, so that code naming a policy still compiles on a
// build that cannot chain -- it simply never grows. Degrade, never error.
enum d_arena_growth
{
    // every region is the same size: bounded waste, linear upstream calls
    D_ARENA_GROWTH_FIXED       = 0,

    // each region exceeds the last by a constant step
    D_ARENA_GROWTH_LINEAR      = 1,

    // each region is NUM/DEN times the last: logarithmic upstream calls
    D_ARENA_GROWTH_EXPONENTIAL = 2
};

// d_arena_region
//   struct: the header of one contiguous region, placed at the front of the
// block obtained from the upstream source. The payload follows it, aligned to
// the arena's region alignment.
//   The header is deliberately three fields (two without chaining). It is
// paid once per REGION, not once per allocation, which is the whole reason an
// arena is cheap -- so the temptation to record more here should be weighed
// against the fact that nothing in the header is ever read on the hot path
// except `used`.
struct d_arena_region
{
#if (D_INTERNAL_ARENA_CHAIN == 1)
    struct d_arena_region* next;      // the next region, or NULL
#endif
    d_mem_size             capacity;  // payload bytes in this region
    d_mem_size             used;      // payload bytes handed out
};

// d_arena
//   struct: a monotonic bump allocator over one or more regions.
//   NOT THREAD-SAFE, and deliberately so: a lock inside the allocate path
// would cost every single-threaded caller an atomic, and a caller that needs
// one knows where to put it. Give each thread its own arena, or wrap this one.
struct d_arena
{
    // where regions come from. Resolved at init, so the "is it set" question
    // is asked once per arena rather than once per allocation
    struct d_mem_source    source;

    // the first region. When chaining is off this is the ONLY region, and
    // every traversal below collapses to it
    struct d_arena_region* head;

    // the payload size of the next region to request. Present in BOTH shapes:
    // an unchained arena requests exactly one region, and a caller asking for
    // a one-megabyte fixed arena must get one rather than the default size
    d_mem_size             next_bytes;

#if (D_INTERNAL_ARENA_CHAIN == 1)
    struct d_arena_region* current;      // the region the bump cursor is in
    struct d_arena_region* tail;         // the last region, for O(1) append
    d_mem_size             region_count;
#endif

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    void*                  last_ptr;     // the most recent allocation
    d_mem_size             last_size;    // its size, or 0 if none is popable
#endif

    d_mem_size             capacity;     // total payload bytes across regions
    d_mem_size             max_bytes;    // hard ceiling; 0 means unbounded
    d_mem_size             align;        // alignment regions are requested at
    uint32_t               flags;

#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
    enum d_arena_growth    growth;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    struct d_mem_stats     stats;
#endif
};

// D_ARENA_FLAG_FOREIGN_HEAD
//   constant: set when the first region lives inside memory the caller
// supplied, so release must not hand it to the source. This is what makes
// d_arena_init_buffer possible without a second arena type.
#define D_ARENA_FLAG_FOREIGN_HEAD   ((uint32_t)0x00000001u)

// D_ARENA_FLAG_FIXED
//   constant: set when this arena must never obtain another region, whatever
// the build supports. The per-instance form of D_CFG_ARENA_CHAIN=0.
#define D_ARENA_FLAG_FIXED          ((uint32_t)0x00000002u)

// d_arena_config
//   struct: how one arena differs from the configured defaults. EVERY FIELD
// MAY BE ZERO, and a zeroed config is exactly "the defaults" -- so a caller
// that wants an ordinary arena passes NULL and one that wants to change a
// single thing changes a single field.
struct d_arena_config
{
    // where regions come from. Unset (zeroed) means the configured default
    struct d_mem_source source;

    // the payload size of the first region; 0 means D_CFG_ARENA_REGION_BYTES
    d_mem_size          initial_bytes;

    // a ceiling on TOTAL payload across every region; 0 means unbounded.
    // This is the knob that turns an arena into a budget: exceed it and
    // allocation reports D_MEM_ERR_EXHAUSTED rather than asking upstream
    d_mem_size          max_bytes;

    // the alignment regions are requested at; 0 means the configured default
    d_mem_size          align;

    // how the next region is sized. Ignored when the growth policy is pinned
    // at compile time
    enum d_arena_growth growth;
};

#if (D_INTERNAL_ARENA_MARKS == 1)

// d_arena_mark
//   struct: a position in an arena, taken by d_arena_mark_get and restored by
// d_arena_rewind. A VALUE THE CALLER HOLDS -- the arena carries no field for
// it -- which is why marks nest naturally and cost the arena nothing.
//   A mark is invalidated by reset, release, and by rewinding to an EARLIER
// mark. Using an invalidated mark is undefined unless
// D_CFG_ARENA_VALIDATE_MARKS is on, in which case rewind reports
// D_MEM_ERR_FOREIGN instead.
struct d_arena_mark
{
    struct d_arena_region* region;
    d_mem_size             used;
};

#endif  // D_INTERNAL_ARENA_MARKS


// I.    lifecycle
struct d_arena_config d_arena_config_default(void);
enum d_mem_status     d_arena_init(struct d_arena*              _arena,
                                   const struct d_arena_config* _config);
enum d_mem_status     d_arena_init_buffer(struct d_arena* _arena,
                                          void*           _buffer,
                                          d_mem_size      _bytes);
void                  d_arena_reset(struct d_arena* _arena);
void                  d_arena_trim(struct d_arena* _arena);
void                  d_arena_release(struct d_arena* _arena);

// II.   allocation
void*                 d_arena_allocate(struct d_arena* _arena,
                                       d_mem_size      _bytes,
                                       d_mem_size      _align);
enum d_mem_status     d_arena_allocate_ex(struct d_arena*     _arena,
                                          d_mem_size          _bytes,
                                          d_mem_size          _align,
                                          struct d_mem_block* _out);
void*                 d_arena_allocate_array(struct d_arena* _arena,
                                             d_mem_size      _count,
                                             d_mem_size      _element_size,
                                             d_mem_size      _align);
void*                 d_arena_duplicate(struct d_arena* _arena,
                                        const void*     _source,
                                        d_mem_size      _bytes,
                                        d_mem_size      _align);
char*                 d_arena_duplicate_string(struct d_arena* _arena,
                                               const char*     _text);

// III.  last-allocation operations
#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
void*                 d_arena_extend(struct d_arena* _arena,
                                     void*           _ptr,
                                     d_mem_size      _old_bytes,
                                     d_mem_size      _new_bytes,
                                     d_mem_size      _align);
bool                  d_arena_pop_last(struct d_arena* _arena,
                                       void*           _ptr,
                                       d_mem_size      _bytes);
#endif

// IV.   mark and rewind
#if (D_INTERNAL_ARENA_MARKS == 1)
struct d_arena_mark   d_arena_mark_get(const struct d_arena* _arena);
enum d_mem_status     d_arena_rewind(struct d_arena*     _arena,
                                     struct d_arena_mark _mark);
#endif

// V.    queries
bool                  d_arena_is_valid(const struct d_arena* _arena);
d_mem_size            d_arena_used(const struct d_arena* _arena);
d_mem_size            d_arena_capacity(const struct d_arena* _arena);
d_mem_size            d_arena_remaining(const struct d_arena* _arena);
d_mem_size            d_arena_region_count(const struct d_arena* _arena);
enum d_mem_status     d_arena_stats(const struct d_arena* _arena,
                                    struct d_mem_stats*   _out);
#if (D_INTERNAL_ARENA_OWNS == 1)
bool                  d_arena_owns(const struct d_arena* _arena,
                                   const void*           _ptr);
#endif

// VI.   composition
#if (D_INTERNAL_ARENA_AS_SOURCE == 1)
struct d_mem_source   d_arena_as_source(struct d_arena* _arena);
#endif


///////////////////////////////////////////////////////////////////////////////
///                     VII.   LAYOUT ASSERTIONS                            ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_arena, source) == 0,
                "d_arena.source must lead the struct");

#   if (D_INTERNAL_ARENA_CHAIN == 1)
D_STATIC_ASSERT(offsetof(struct d_arena_region, next) == 0,
                "d_arena_region.next must lead the header");
#   else
D_STATIC_ASSERT(offsetof(struct d_arena_region, capacity) == 0,
                "d_arena_region.capacity must lead the unchained header");
#   endif

//   The region header is the arena's only per-region cost, so its size is
// asserted rather than assumed: an added field here is paid on every region
// in every arena in the program, and this is the line that makes such an
// addition a deliberate act.
D_STATIC_ASSERT(sizeof(struct d_arena_region) <=
                    (3 * sizeof(d_mem_size)) + sizeof(void*),
                "d_arena_region header has grown beyond its budget");

#endif  // D_INTERNAL_MEM_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // DJINTERP_C_MEMORY_ARENA_H
