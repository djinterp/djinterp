/*******************************************************************************
* djinterp [c][desc]                                                  mem_desc.h
*
*   desc's describers of memory's types (desc's decisions 34, 68 and 82):
* the arena, the pool, a memory source, a memory block and an allocation
* tally (struct dj_mem_stats), each a struct dj_desc_type, so d_desc_say
* writes it in prose and d_desc_facts as facts. They are desc's, written
* with desc rather than by memory, in a library above both: memory includes
* nothing of desc, and a program that describes none of these links none
* of this.
*   Each describer reads the type's public fields and never calls memory's
* functions, so the library needs memory's headers and not its units. The
* fields it reads are recorded beside it (section 2), and the layout rows
* check each record against its type, and the library's build against the
* caller's (desc_of.h): d_desc_of_mem_agrees is 0 where the library was
* built with other memory knobs than the caller, whose describers would
* read the wrong bytes.
*   None of memory's types is thread-safe and none holds a lock, so every
* public field is described: an object is read as its one thread sees it,
* and a describer must not be called on an object another thread is using.
* A value of 0 in max_bytes or max_slots is the type's: unbounded.
*
* HOW TO USE
* ----------
*   d_desc_say(D_DESC_OF(dj_arena), &arena, ring, layers, 0, put, context)
* writes an arena in prose; d_desc_facts(D_DESC_OF(dj_pool), &pool, sink)
* gives a pool's facts. A program linked against djinterp_c_desc_of can ask
* d_desc_of_mem_agrees whether the library was built with its own memory
* knobs.
*
* OPTIONS
* -------
*   desc's own: desc.ring, desc.layers and the rest choose how a
* description is written; desc.arena.ring, desc.pool.ring and so on, once
* d_desc_declare has declared the type, set one type's ring.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and D_CFG_DESC_OF_MEMORY (cfg_desc_of.h); desc's knobs;
* and memory's layout knobs, which the records follow.
*
*
* path:      /inc/djinterp/c/desc/of/mem_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIPTIONS
    ------------
    1.  d_desc_type_dj_arena
    2.  d_desc_type_dj_pool
    3.  d_desc_type_dj_mem_source
    4.  d_desc_type_dj_mem_block
    5.  d_desc_type_dj_mem_stats
2.  RECORDS
    -------
    1.  fn_mem_desc_any
    2.  dj_mem_desc_*_record
3.  LAYOUT
    ------
    1.  d_desc_of_mem_layout_here
    2.  d_desc_of_mem_layout
    3.  d_desc_of_mem_agrees
*/

#ifndef DJINTERP_C_DESC_OF_MEM_DESC_H
#define DJINTERP_C_DESC_OF_MEM_DESC_H 1

// djinterp
#include "./desc_of.h"                 // the layout row, the knobs
#include "../../memory/mem_common.h"   // struct dj_mem_block, dj_mem_stats
#include "../../memory/mem_source.h"   // struct dj_mem_source
#include "../../memory/arena.h"        // struct dj_arena
#include "../../memory/pool.h"         // struct dj_pool


D_EXTERN_C_BEGIN


#if D_INTERNAL_DESC_OF_MEMORY

//==============================================================================
// 1.  DESCRIPTIONS
//==============================================================================
//   Each is named for D_DESC_OF: D_DESC_OF(dj_arena) is
// &d_desc_type_dj_arena. Each type's name is the scope of its options:
// desc.arena.ring, once d_desc_declare has declared it.

// 1.1
// d_desc_type_dj_arena
//   constant: an arena: "an arena with 4096 of 65536 bytes used". Its
// facts: used, capacity, region_count, max_bytes, align, next_bytes, fixed,
// caller_buffer, growth and source; its contents, where memory keeps them,
// the arena's tally, under "stats".
extern const struct dj_desc_type d_desc_type_dj_arena;

// 1.2
// d_desc_type_dj_pool
//   constant: a pool: "a pool of 32-byte slots, with 3 of 64 slots in use".
// Its facts: live, capacity, slot_size, policy, max_slots, free_count,
// block_count, slots_per_block, stride, align and source; its contents, as
// the arena's.
extern const struct dj_desc_type d_desc_type_dj_pool;

// 1.3
// d_desc_type_dj_mem_source
//   constant: a memory source: "a memory source named malloc", or "an unset
// memory source", for which the configured default stands in. Its facts:
// set, name and reallocates.
extern const struct dj_desc_type d_desc_type_dj_mem_source;

// 1.4
// d_desc_type_dj_mem_block
//   constant: a memory block, a view of bytes: "a memory block with 64
// bytes". Its fact: size.
extern const struct dj_desc_type d_desc_type_dj_mem_block;

// 1.5
// d_desc_type_dj_mem_stats
//   constant: an allocation tally: "an allocation tally with 4096 live bytes
// in 12 allocations". Its facts: bytes_live, live_count, bytes_reserved,
// bytes_committed, acquire_count, release_count and upstream_count, and,
// where memory keeps peaks, peak_bytes_live and peak_live_count.
extern const struct dj_desc_type d_desc_type_dj_mem_stats;

#endif  // D_INTERNAL_DESC_OF_MEMORY


#if D_INTERNAL_DESC_OF_MEMORY

//==============================================================================
// 2.  RECORDS
//==============================================================================
//   Each described type as its describer knows it: the type's fields, in
// its order, under the same knobs. A pointer the describer does not follow
// is a void*, and a function pointer any one, since only its place and
// width are recorded. When a type changes, its record changes with the
// describer's code, never alone: a record changed to match a type its
// describer has not followed defeats the check. They are here, not in the
// unit, so that a program compiles them with its own knobs (section 3).

// fn_mem_desc_any
//   function pointer: a function pointer's place in a record.
typedef void (*fn_mem_desc_any)(void);

struct dj_mem_desc_stats_record
{
    dj_mem_size bytes_reserved;
    dj_mem_size bytes_committed;
    dj_mem_size bytes_live;
    dj_mem_size acquire_count;
    dj_mem_size release_count;
    dj_mem_size upstream_count;
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
    dj_mem_size peak_bytes_live;
    dj_mem_size peak_live_count;
#endif
};

struct dj_mem_desc_block_record
{
    void*       ptr;
    dj_mem_size size;
};

struct dj_mem_desc_vtable_record
{
    fn_mem_desc_any allocate;
    fn_mem_desc_any release;
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    fn_mem_desc_any reallocate;
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    fn_mem_desc_any max_size;
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    const char*     name;
#endif
};

struct dj_mem_desc_source_record
{
    const void* vtable;
    void*       context;
};

struct dj_mem_desc_region_record
{
#if (D_INTERNAL_ARENA_CHAIN == 1)
    void*       next;
#endif
    dj_mem_size capacity;
    dj_mem_size used;
};

struct dj_mem_desc_arena_record
{
    struct dj_mem_desc_source_record source;
    void*                            head;
    dj_mem_size                      next_bytes;
#if (D_INTERNAL_ARENA_CHAIN == 1)
    void*                            current;
    void*                            tail;
    dj_mem_size                      region_count;
#endif
#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    void*                            last_ptr;
    dj_mem_size                      last_size;
#endif
    dj_mem_size                      capacity;
    dj_mem_size                      max_bytes;
    dj_mem_size                      align;
    uint32_t                         flags;
#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
    enum dj_arena_growth             growth;
#endif
#if (D_INTERNAL_MEM_STATS == 1)
    struct dj_mem_desc_stats_record  stats;
#endif
};

struct dj_mem_desc_pool_record
{
    struct dj_mem_desc_source_record source;
    void*                            head;
    void*                            tail;
#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    void*                            blocks;
    dj_mem_size                      block_capacity;
#endif
    dj_mem_size                      stride;
    dj_mem_size                      payload;
    dj_mem_size                      align;
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    dj_mem_size                      generation_offset;
#endif
    dj_pool_index                    slots_per_block;
    dj_pool_index                    capacity;
    dj_pool_index                    live;
    dj_pool_index                    bump;
    dj_pool_index                    max_slots;
    dj_mem_size                      block_count;
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    dj_pool_index                    free_head;
    dj_pool_index                    free_count;
#endif
    uint8_t                          policy;
#if (D_INTERNAL_MEM_STATS == 1)
    struct dj_mem_desc_stats_record  stats;
#endif
};

#endif  // D_INTERNAL_DESC_OF_MEMORY


//==============================================================================
// 3.  LAYOUT
//==============================================================================

/**
 * @brief Memory's layout rows as the caller is built: each described type's
 *        size, and the offset and width of each field its describer reads,
 *        as recorded and as the type has them, under the caller's knobs.
 *
 * @param[out] _rows  set to the rows, static; null when memory's describers
 *                    are not built. May be null.
 * @return the rows' count; 0 when memory's describers are not built.
 */
D_INLINE size_t
d_desc_of_mem_layout_here(
    const struct dj_desc_of_layout** _rows
)
{
#if D_INTERNAL_DESC_OF_MEMORY
    // each type's size, then each field its describer reads
    static const struct dj_desc_of_layout rows[] =
    {
        D_DESC_OF_LAYOUT_SIZE(dj_mem_stats, dj_mem_desc_stats_record),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               bytes_reserved),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               bytes_committed),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               bytes_live),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               acquire_count),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               release_count),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               upstream_count),
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               peak_bytes_live),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_stats, dj_mem_desc_stats_record,
                               peak_live_count),
#endif
        D_DESC_OF_LAYOUT_SIZE(dj_mem_block, dj_mem_desc_block_record),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_block, dj_mem_desc_block_record, ptr),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_block, dj_mem_desc_block_record, size),
        D_DESC_OF_LAYOUT_SIZE(dj_mem_source_vtable, dj_mem_desc_vtable_record),
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_mem_source_vtable, dj_mem_desc_vtable_record,
                               reallocate),
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_mem_source_vtable, dj_mem_desc_vtable_record,
                               name),
#endif
        D_DESC_OF_LAYOUT_SIZE(dj_mem_source, dj_mem_desc_source_record),
        D_DESC_OF_LAYOUT_FIELD(dj_mem_source, dj_mem_desc_source_record,
                               vtable),
        D_DESC_OF_LAYOUT_SIZE(dj_arena_region, dj_mem_desc_region_record),
#if (D_INTERNAL_ARENA_CHAIN == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_arena_region, dj_mem_desc_region_record,
                               next),
#endif
        D_DESC_OF_LAYOUT_FIELD(dj_arena_region, dj_mem_desc_region_record,
                               used),
        D_DESC_OF_LAYOUT_SIZE(dj_arena, dj_mem_desc_arena_record),
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, source),
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, head),
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, next_bytes),
#if (D_INTERNAL_ARENA_CHAIN == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record,
                               region_count),
#endif
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, capacity),
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, max_bytes),
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, align),
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, flags),
#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, growth),
#endif
#if (D_INTERNAL_MEM_STATS == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_arena, dj_mem_desc_arena_record, stats),
#endif
        D_DESC_OF_LAYOUT_SIZE(dj_pool, dj_mem_desc_pool_record),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, source),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, stride),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, payload),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, align),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record,
                               slots_per_block),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, capacity),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, live),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, max_slots),
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, block_count),
#if (D_INTERNAL_POOL_FREE_LIST == 1)
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, free_count),
#endif
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, policy)
#if (D_INTERNAL_MEM_STATS == 1)
        ,
        D_DESC_OF_LAYOUT_FIELD(dj_pool, dj_mem_desc_pool_record, stats)
#endif
    };

    if (_rows != NULL)
    {
        *_rows = rows;
    }

    return sizeof(rows) / sizeof(rows[0]);
#else
    if (_rows != NULL)
    {
        *_rows = NULL;
    }

    return 0u;
#endif
}

/**
 * @brief Memory's layout rows as the library was built: the same table,
 *        under the library's knobs. Where they are the caller's, its rows
 *        are d_desc_of_mem_layout_here's.
 *
 * @param[out] _rows  as d_desc_of_mem_layout_here.
 * @return as d_desc_of_mem_layout_here.
 */
size_t d_desc_of_mem_layout(const struct dj_desc_of_layout** _rows);

/**
 * @brief Whether the library lays memory's described types out as the
 *        caller does: built with the same memory knobs, as far as its
 *        describers read. A describer called across builds that disagree
 *        reads the wrong bytes.
 *
 * @param[out] _index  set to the first row the two differ in; may be null.
 * @return 1 when they agree, or neither has memory's describers; else 0.
 */
D_INLINE int
d_desc_of_mem_agrees(
    size_t* _index
)
{
    const struct dj_desc_of_layout* built       = NULL;
    const struct dj_desc_of_layout* here        = NULL;
    size_t                          built_count = d_desc_of_mem_layout(&built);
    size_t                          here_count  =
        d_desc_of_mem_layout_here(&here);

    return d_desc_of_agree(built, built_count, here, here_count, _index);
}


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_OF_MEM_DESC_H
