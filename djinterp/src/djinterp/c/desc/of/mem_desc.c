/*******************************************************************************
* djinterp [c][desc]                                                  mem_desc.c
*
*   desc's describers of memory's types: for each, its plan, its reader,
* its units and its field list; and the layout rows as this library was
* built. A describer reads fields only, so this unit calls none of
* memory's functions; the records of the fields it reads are in the
* header, so that a program compiles them with its own knobs too.
*
*
* path:      /src/djinterp/c/desc/of/mem_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "../../../../../inc/djinterp/c/desc/of/mem_desc.h"  // corresponding
                                                              // header

#if D_INTERNAL_DESC_OF_MEMORY

//==============================================================================
// 1.  COMMON
//==============================================================================

/*
d_internal_mem_desc_size
  A dj_mem_size as a stream value, whatever its width: two shifts of 16, so
a 32-bit size shifts to 0, never too far.
*/
static struct dj_ostream_value
d_internal_mem_desc_size(
    dj_mem_size _value
)
{
    struct dj_ostream_value value = d_ostream_value_size(0u);

    value.low  = (uint32_t)(_value & 0xFFFFFFFFu);
    value.high = (uint32_t)(((_value >> 16) >> 16) & 0xFFFFFFFFu);

    return value;
}

/*
d_internal_mem_desc_index
  A dj_pool_index as a stream value, as d_internal_mem_desc_size.
*/
static struct dj_ostream_value
d_internal_mem_desc_index(
    dj_pool_index _value
)
{
    struct dj_ostream_value value = d_ostream_value_size(0u);

    value.low  = (uint32_t)(_value & 0xFFFFFFFFu);
    value.high = (uint32_t)(((_value >> 16) >> 16) & 0xFFFFFFFFu);

    return value;
}

/*
d_internal_mem_desc_source_name
  A source's name, static; null for an unset source, an unnamed one, or a
build whose sources carry no names.
*/
static const char*
d_internal_mem_desc_source_name(
    const struct dj_mem_source* _source
)
{
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    if (_source->vtable != NULL)
    {
        return _source->vtable->name;
    }
#else
    (void)_source;
#endif

    return NULL;
}

#if (D_INTERNAL_MEM_STATS == 1)
/*
d_internal_mem_desc_stats_contents
  An allocator's tally as its contents: the tally's facts, one level down,
under "stats".
*/
static int
d_internal_mem_desc_stats_contents(
    const struct dj_mem_stats* _stats,
    struct dj_desc_sink*       _sink
)
{
    int stopped;

    if (d_desc_enter(_sink, "stats", "allocation tally") == 0)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    stopped = d_desc_facts(&d_desc_type_dj_mem_stats, _stats, _sink);
    d_desc_leave(_sink);

    return ( (stopped > 0) || (_sink->stopped != 0u) ) ? 1 : 0;
}
#endif  // D_INTERNAL_MEM_STATS

// the op array three types share
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_EMPTY[] =
    { D_DESC_T("empty"), D_DESC_END };
#endif


//==============================================================================
// 2.  THE ALLOCATION TALLY
//==============================================================================

// the fields, in the field list's order
#define D_MEM_DESC_STATS_BYTES_LIVE   0u
#define D_MEM_DESC_STATS_LIVE_COUNT   1u
#define D_MEM_DESC_STATS_RESERVED     2u
#define D_MEM_DESC_STATS_COMMITTED    3u
#define D_MEM_DESC_STATS_ACQUIRES     4u
#define D_MEM_DESC_STATS_RELEASES     5u
#define D_MEM_DESC_STATS_UPSTREAM     6u
#define D_MEM_DESC_STATS_PEAK_BYTES   7u
#define D_MEM_DESC_STATS_PEAK_COUNT   8u

// the plan's bit
#define D_MEM_DESC_STATS_IF_IDLE      (1u << 0)

/*
d_internal_mem_desc_stats_plan
  Idle: nothing acquired and nothing reserved.
*/
static uint32_t
d_internal_mem_desc_stats_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_mem_stats* stats = (const struct dj_mem_stats*)_object;

    if ( (_ring >= D_DESC_RING_LINE)    &&
         (stats->acquire_count == 0u)  &&
         (stats->bytes_reserved == 0u) )
    {
        return D_MEM_DESC_STATS_IF_IDLE;
    }

    return 0u;
}

/*
d_internal_mem_desc_stats_read
  Reads one field; the live count is acquires less releases, and 0 should a
tally have more releases than acquires.
*/
static struct dj_ostream_value
d_internal_mem_desc_stats_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_mem_stats* stats = (const struct dj_mem_stats*)_object;

    switch (_field)
    {
        case D_MEM_DESC_STATS_BYTES_LIVE:
            return d_internal_mem_desc_size(stats->bytes_live);
        case D_MEM_DESC_STATS_LIVE_COUNT:
            return d_internal_mem_desc_size(
                (stats->acquire_count >= stats->release_count)
                    ? (dj_mem_size)(stats->acquire_count -
                                    stats->release_count)
                    : (dj_mem_size)0u);
        case D_MEM_DESC_STATS_RESERVED:
            return d_internal_mem_desc_size(stats->bytes_reserved);
        case D_MEM_DESC_STATS_COMMITTED:
            return d_internal_mem_desc_size(stats->bytes_committed);
        case D_MEM_DESC_STATS_ACQUIRES:
            return d_internal_mem_desc_size(stats->acquire_count);
        case D_MEM_DESC_STATS_RELEASES:
            return d_internal_mem_desc_size(stats->release_count);
        case D_MEM_DESC_STATS_UPSTREAM:
            return d_internal_mem_desc_size(stats->upstream_count);
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
        case D_MEM_DESC_STATS_PEAK_BYTES:
            return d_internal_mem_desc_size(stats->peak_bytes_live);
        case D_MEM_DESC_STATS_PEAK_COUNT:
            return d_internal_mem_desc_size(stats->peak_live_count);
#endif
        default:
            return d_ostream_value_none();
    }
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_UNUSED[] =
    { D_DESC_T("unused"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_STATS_LIVE[] =
    { D_DESC_F(D_MEM_DESC_STATS_BYTES_LIVE), D_DESC_T(" live byte"),
      D_DESC_PL, D_DESC_T(" in "), D_DESC_F(D_MEM_DESC_STATS_LIVE_COUNT),
      D_DESC_T(" allocation"), D_DESC_PL, D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, COST)
static const struct dj_desc_op D_MEM_DESC_OPS_STATS_RESERVED[] =
    { D_DESC_T("It reserved "), D_DESC_F(D_MEM_DESC_STATS_RESERVED),
      D_DESC_T(" byte"), D_DESC_PL, D_DESC_T(" in "),
      D_DESC_F(D_MEM_DESC_STATS_UPSTREAM), D_DESC_T(" upstream request"),
      D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, COST)
static const struct dj_desc_op D_MEM_DESC_OPS_STATS_CALLS[] =
    { D_DESC_T("It counted "), D_DESC_F(D_MEM_DESC_STATS_ACQUIRES),
      D_DESC_T(" acquire"), D_DESC_PL, D_DESC_T(" and "),
      D_DESC_F(D_MEM_DESC_STATS_RELEASES), D_DESC_T(" release"),
      D_DESC_PL, D_DESC_T("."), D_DESC_END };
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
static const struct dj_desc_op D_MEM_DESC_OPS_STATS_PEAK[] =
    { D_DESC_T("At its peak, "), D_DESC_F(D_MEM_DESC_STATS_PEAK_BYTES),
      D_DESC_T(" byte"), D_DESC_PL, D_DESC_T(" "),
      D_DESC_WORDS("was", "were"), D_DESC_T(" live."), D_DESC_END };
#endif
#endif
static const struct dj_desc_op D_MEM_DESC_OPS_TALLY[] =
    { D_DESC_T("allocation tally"), D_DESC_END };

static const struct dj_desc_unit d_internal_mem_desc_stats_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_MEM_DESC_STATS_IF_IDLE, 0u, D_MEM_DESC_OPS_UNUSED),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_MEM_DESC_OPS_TALLY),
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_MEM_DESC_STATS_IF_IDLE, D_MEM_DESC_OPS_STATS_LIVE),
#endif
#if D_DESC_BUILT(SENTENCE, COST)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_COST, D_DESC_SLOT_REMARK,
                0u, D_MEM_DESC_STATS_IF_IDLE, D_MEM_DESC_OPS_STATS_RESERVED),
#endif
#if D_DESC_BUILT(DETAILED, COST)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST, D_DESC_SLOT_REMARK,
                0u, D_MEM_DESC_STATS_IF_IDLE, D_MEM_DESC_OPS_STATS_CALLS),
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST, D_DESC_SLOT_REMARK,
                0u, D_MEM_DESC_STATS_IF_IDLE, D_MEM_DESC_OPS_STATS_PEAK),
#endif
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_mem_desc_stats_fields[] =
{
    D_DESC_FIELD("bytes_live", "live bytes", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("live_count", "allocations", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("bytes_reserved", "bytes reserved", D_DESC_LAYER_COST, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("bytes_committed", "bytes committed", D_DESC_LAYER_COST,
                 2u, D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("acquire_count", "acquires", D_DESC_LAYER_COST, 2u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("release_count", "releases", D_DESC_LAYER_COST, 2u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("upstream_count", "upstream requests", D_DESC_LAYER_COST,
                 2u, D_DESC_TRUTH_OBSERVED)
#if (D_INTERNAL_MEM_STATS_PEAK == 1)
    ,
    D_DESC_FIELD("peak_bytes_live", "peak live bytes", D_DESC_LAYER_COST,
                 3u, D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("peak_live_count", "peak allocations", D_DESC_LAYER_COST,
                 3u, D_DESC_TRUTH_OBSERVED)
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_mem_stats =
{
    "mem_stats",
    "allocation tally",
    D_DESC_UNITS_TABLE(d_internal_mem_desc_stats_units),
    D_DESC_FIELDS_TABLE(d_internal_mem_desc_stats_fields),
    d_internal_mem_desc_stats_plan,
    d_internal_mem_desc_stats_read,
    NULL
};


//==============================================================================
// 3.  THE MEMORY BLOCK
//==============================================================================

#define D_MEM_DESC_BLOCK_SIZE      0u
#define D_MEM_DESC_BLOCK_IF_EMPTY  (1u << 0)

/*
d_internal_mem_desc_block_plan
  Empty: no bytes, or no address.
*/
static uint32_t
d_internal_mem_desc_block_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_mem_block* block = (const struct dj_mem_block*)_object;

    if ( (_ring >= D_DESC_RING_LINE) &&
         ( (block->ptr == NULL) || (block->size == 0u) ) )
    {
        return D_MEM_DESC_BLOCK_IF_EMPTY;
    }

    return 0u;
}

/*
d_internal_mem_desc_block_read
  Reads the size; a block without an address has none.
*/
static struct dj_ostream_value
d_internal_mem_desc_block_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_mem_block* block = (const struct dj_mem_block*)_object;

    if (_field == D_MEM_DESC_BLOCK_SIZE)
    {
        return d_internal_mem_desc_size( (block->ptr != NULL) ? block->size
                                                              : 0u );
    }

    return d_ostream_value_none();
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_BLOCK_BYTES[] =
    { D_DESC_F(D_MEM_DESC_BLOCK_SIZE), D_DESC_T(" byte"), D_DESC_PL,
      D_DESC_END };
#endif
static const struct dj_desc_op D_MEM_DESC_OPS_BLOCK[] =
    { D_DESC_T("memory block"), D_DESC_END };

static const struct dj_desc_unit d_internal_mem_desc_block_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_MEM_DESC_BLOCK_IF_EMPTY, 0u, D_MEM_DESC_OPS_EMPTY),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_MEM_DESC_OPS_BLOCK),
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_MEM_DESC_BLOCK_IF_EMPTY, D_MEM_DESC_OPS_BLOCK_BYTES),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_mem_desc_block_fields[] =
{
    D_DESC_FIELD("size", "bytes", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_mem_block =
{
    "mem_block",
    "memory block",
    D_DESC_UNITS_TABLE(d_internal_mem_desc_block_units),
    D_DESC_FIELDS_TABLE(d_internal_mem_desc_block_fields),
    d_internal_mem_desc_block_plan,
    d_internal_mem_desc_block_read,
    NULL
};


//==============================================================================
// 4.  THE MEMORY SOURCE
//==============================================================================

#define D_MEM_DESC_SOURCE_SET          0u
#define D_MEM_DESC_SOURCE_NAME         1u
#define D_MEM_DESC_SOURCE_REALLOCATES  2u

#define D_MEM_DESC_SOURCE_IF_UNSET     (1u << 0)
#define D_MEM_DESC_SOURCE_IF_NAMED     (1u << 1)
#define D_MEM_DESC_SOURCE_IF_REALLOC   (1u << 2)

/*
d_internal_mem_desc_source_reallocates
  1 when the source grows a block in place itself, rather than by the
emulation: the slot is built and this source fills it.
*/
static int
d_internal_mem_desc_source_reallocates(
    const struct dj_mem_source* _source
)
{
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    return ( (_source->vtable != NULL) &&
             (_source->vtable->reallocate != NULL) ) ? 1 : 0;
#else
    (void)_source;

    return 0;
#endif
}

/*
d_internal_mem_desc_source_plan
  Unset at the line; named and reallocating at the ring that says so.
*/
static uint32_t
d_internal_mem_desc_source_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_mem_source* source = (const struct dj_mem_source*)_object;
    uint32_t                    bits   = 0u;

    if (_ring >= D_DESC_RING_LINE)
    {
        bits |= (source->vtable == NULL) ? D_MEM_DESC_SOURCE_IF_UNSET : 0u;
        bits |= (d_internal_mem_desc_source_name(source) != NULL)
                    ? D_MEM_DESC_SOURCE_IF_NAMED : 0u;
    }

    if ( (_ring >= D_DESC_RING_SENTENCE) &&
         (d_internal_mem_desc_source_reallocates(source) != 0) )
    {
        bits |= D_MEM_DESC_SOURCE_IF_REALLOC;
    }

    return bits;
}

/*
d_internal_mem_desc_source_read
  Reads one field; an unnamed source's name is none.
*/
static struct dj_ostream_value
d_internal_mem_desc_source_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_mem_source* source = (const struct dj_mem_source*)_object;

    switch (_field)
    {
        case D_MEM_DESC_SOURCE_SET:
            return d_ostream_value_bool((source->vtable != NULL) ? 1 : 0);
        case D_MEM_DESC_SOURCE_NAME:
            return d_ostream_value_text(
                       d_internal_mem_desc_source_name(source));
        case D_MEM_DESC_SOURCE_REALLOCATES:
            return d_ostream_value_bool(
                       d_internal_mem_desc_source_reallocates(source));
        default:
            return d_ostream_value_none();
    }
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_UNSET[] =
    { D_DESC_T("unset"), D_DESC_END };
#endif
#if D_DESC_BUILT(LINE, IDENTITY)
static const struct dj_desc_op D_MEM_DESC_OPS_SOURCE_NAMED[] =
    { D_DESC_T("named "), D_DESC_F(D_MEM_DESC_SOURCE_NAME), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_SOURCE_DEFAULT[] =
    { D_DESC_T("The configured default stands in for it."), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
static const struct dj_desc_op D_MEM_DESC_OPS_SOURCE_REALLOC[] =
    { D_DESC_T("It can grow a block in place."), D_DESC_END };
#endif
static const struct dj_desc_op D_MEM_DESC_OPS_SOURCE[] =
    { D_DESC_T("memory source"), D_DESC_END };

static const struct dj_desc_unit d_internal_mem_desc_source_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_MEM_DESC_SOURCE_IF_UNSET, 0u, D_MEM_DESC_OPS_UNSET),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_MEM_DESC_OPS_SOURCE),
#if D_DESC_BUILT(LINE, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_COMPLEMENT, D_MEM_DESC_SOURCE_IF_NAMED, 0u,
                D_MEM_DESC_OPS_SOURCE_NAMED),
#endif
#if D_DESC_BUILT(SENTENCE, STATE)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_STATE,
                D_DESC_SLOT_REMARK, D_MEM_DESC_SOURCE_IF_UNSET, 0u,
                D_MEM_DESC_OPS_SOURCE_DEFAULT),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_MEM_DESC_SOURCE_IF_REALLOC, 0u,
                D_MEM_DESC_OPS_SOURCE_REALLOC),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_mem_desc_source_fields[] =
{
    D_DESC_FIELD("set", "set", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("name", "named", D_DESC_LAYER_IDENTITY, 0u,
                 D_DESC_TRUTH_IMPLEMENTATION),
    D_DESC_FIELD("reallocates", "grows in place", D_DESC_LAYER_POLICY, 2u,
                 D_DESC_TRUTH_OBSERVED)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_mem_source =
{
    "mem_source",
    "memory source",
    D_DESC_UNITS_TABLE(d_internal_mem_desc_source_units),
    D_DESC_FIELDS_TABLE(d_internal_mem_desc_source_fields),
    d_internal_mem_desc_source_plan,
    d_internal_mem_desc_source_read,
    NULL
};


//==============================================================================
// 5.  THE ARENA
//==============================================================================

// the fields, in the field list's order
#define D_MEM_DESC_ARENA_USED         0u
#define D_MEM_DESC_ARENA_CAPACITY     1u
#define D_MEM_DESC_ARENA_REGIONS      2u
#define D_MEM_DESC_ARENA_MAX          3u
#define D_MEM_DESC_ARENA_ALIGN        4u
#define D_MEM_DESC_ARENA_NEXT         5u
#define D_MEM_DESC_ARENA_FIXED        6u
#define D_MEM_DESC_ARENA_FOREIGN      7u
#define D_MEM_DESC_ARENA_GROWTH       8u
#define D_MEM_DESC_ARENA_SOURCE       9u

// the plan's bits
#define D_MEM_DESC_ARENA_IF_EMPTY     (1u << 0)
#define D_MEM_DESC_ARENA_IF_CHAINED   (1u << 1)
#define D_MEM_DESC_ARENA_IF_FIXED     (1u << 2)
#define D_MEM_DESC_ARENA_IF_FOREIGN   (1u << 3)
#define D_MEM_DESC_ARENA_IF_BOUNDED   (1u << 4)
#define D_MEM_DESC_ARENA_IF_NAMED     (1u << 5)

// how a growth is written: enum dj_arena_growth's order
static const char* const D_MEM_DESC_GROWTH_NAMES[] =
{
    "fixed", "linear", "exponential"
};

// the growth's truth: an arena's own where each may differ, else the knob's
#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
    #define D_MEM_DESC_GROWTH_TRUTH  D_DESC_TRUTH_OBSERVED
#else
    #define D_MEM_DESC_GROWTH_TRUTH  D_DESC_TRUTH_BUILD
#endif

/*
d_internal_mem_desc_arena_used
  The payload bytes handed out, every region's: a walk where regions chain.
*/
static dj_mem_size
d_internal_mem_desc_arena_used(
    const struct dj_arena* _arena
)
{
    const struct dj_arena_region* region = _arena->head;
    dj_mem_size                   total  = 0u;

    while (region != NULL)
    {
        total = (dj_mem_size)(total + region->used);
#if (D_INTERNAL_ARENA_CHAIN == 1)
        region = region->next;
#else
        region = NULL;
#endif
    }

    return total;
}

/*
d_internal_mem_desc_arena_regions
  The regions the arena holds: its count where regions chain, else its one
region, or none before it has one.
*/
static dj_mem_size
d_internal_mem_desc_arena_regions(
    const struct dj_arena* _arena
)
{
#if (D_INTERNAL_ARENA_CHAIN == 1)
    return _arena->region_count;
#else
    return (_arena->head != NULL) ? (dj_mem_size)1u : (dj_mem_size)0u;
#endif
}

/*
d_internal_mem_desc_arena_fixed
  1 when the arena will never obtain another region: it says so, or the
build cannot chain.
*/
static int
d_internal_mem_desc_arena_fixed(
    const struct dj_arena* _arena
)
{
#if (D_INTERNAL_ARENA_CHAIN == 1)
    return ((_arena->flags & D_ARENA_FLAG_FIXED) != 0u) ? 1 : 0;
#else
    (void)_arena;

    return 1;
#endif
}

/*
d_internal_mem_desc_arena_growth
  How the arena sizes its next region: its own where arenas may differ,
else the build's.
*/
static struct dj_ostream_value
d_internal_mem_desc_arena_growth(
    const struct dj_arena* _arena
)
{
    unsigned growth;

#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
    growth = (unsigned)_arena->growth;
#else
    (void)_arena;
    growth = (unsigned)D_CFG_ARENA_GROWTH;
#endif

    return (growth < 3u) ? d_ostream_value_text(D_MEM_DESC_GROWTH_NAMES[growth])
                         : d_ostream_value_none();
}

/*
d_internal_mem_desc_arena_plan
  The state at the line; its policies at the sentence; where its regions
come from at the detail, unless its one region is a caller's buffer.
*/
static uint32_t
d_internal_mem_desc_arena_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_arena* arena = (const struct dj_arena*)_object;
    uint32_t               bits  = 0u;

    if (_ring >= D_DESC_RING_LINE)
    {
        bits |= (d_internal_mem_desc_arena_used(arena) == 0u)
                    ? D_MEM_DESC_ARENA_IF_EMPTY : 0u;
        bits |= (d_internal_mem_desc_arena_regions(arena) > 1u)
                    ? D_MEM_DESC_ARENA_IF_CHAINED : 0u;
    }

    if (_ring >= D_DESC_RING_SENTENCE)
    {
        bits |= (d_internal_mem_desc_arena_fixed(arena) != 0)
                    ? D_MEM_DESC_ARENA_IF_FIXED : 0u;
        bits |= ((arena->flags & D_ARENA_FLAG_FOREIGN_HEAD) != 0u)
                    ? D_MEM_DESC_ARENA_IF_FOREIGN : 0u;
        bits |= (arena->max_bytes != 0u) ? D_MEM_DESC_ARENA_IF_BOUNDED : 0u;
    }

    // a caller's buffer is its one region, whatever its source is
    if ( (_ring >= D_DESC_RING_DETAILED)                       &&
         ((arena->flags & D_ARENA_FLAG_FOREIGN_HEAD) == 0u)    &&
         (d_internal_mem_desc_source_name(&arena->source) != NULL) )
    {
        bits |= D_MEM_DESC_ARENA_IF_NAMED;
    }

    return bits;
}

/*
d_internal_mem_desc_arena_read
  Reads one field.
*/
static struct dj_ostream_value
d_internal_mem_desc_arena_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_arena* arena = (const struct dj_arena*)_object;

    switch (_field)
    {
        case D_MEM_DESC_ARENA_USED:
            return d_internal_mem_desc_size(
                       d_internal_mem_desc_arena_used(arena));
        case D_MEM_DESC_ARENA_CAPACITY:
            return d_internal_mem_desc_size(arena->capacity);
        case D_MEM_DESC_ARENA_REGIONS:
            return d_internal_mem_desc_size(
                       d_internal_mem_desc_arena_regions(arena));
        case D_MEM_DESC_ARENA_MAX:
            return d_internal_mem_desc_size(arena->max_bytes);
        case D_MEM_DESC_ARENA_ALIGN:
            return d_internal_mem_desc_size(arena->align);
        case D_MEM_DESC_ARENA_NEXT:
            return d_internal_mem_desc_size(arena->next_bytes);
        case D_MEM_DESC_ARENA_FIXED:
            return d_ostream_value_bool(d_internal_mem_desc_arena_fixed(arena));
        case D_MEM_DESC_ARENA_FOREIGN:
            return d_ostream_value_bool(
                ((arena->flags & D_ARENA_FLAG_FOREIGN_HEAD) != 0u) ? 1 : 0);
        case D_MEM_DESC_ARENA_GROWTH:
            return d_internal_mem_desc_arena_growth(arena);
        case D_MEM_DESC_ARENA_SOURCE:
            return d_ostream_value_text(
                       d_internal_mem_desc_source_name(&arena->source));
        default:
            return d_ostream_value_none();
    }
}

#if (D_INTERNAL_MEM_STATS == 1)
/*
d_internal_mem_desc_arena_contents
  The arena's tally, one level down.
*/
static int
d_internal_mem_desc_arena_contents(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    return d_internal_mem_desc_stats_contents(
               &((const struct dj_arena*)_object)->stats, _sink);
}

    #define D_MEM_DESC_ARENA_CONTENTS  d_internal_mem_desc_arena_contents
#else
    #define D_MEM_DESC_ARENA_CONTENTS  NULL
#endif  // D_INTERNAL_MEM_STATS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_USED[] =
    { D_DESC_F(D_MEM_DESC_ARENA_USED), D_DESC_T(" of "),
      D_DESC_F(D_MEM_DESC_ARENA_CAPACITY), D_DESC_T(" byte"), D_DESC_PL,
      D_DESC_T(" used"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_REGIONS[] =
    { D_DESC_F(D_MEM_DESC_ARENA_REGIONS), D_DESC_T(" region"), D_DESC_PL,
      D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
static const struct dj_desc_op D_MEM_DESC_OPS_FIXED_SIZE[] =
    { D_DESC_T("fixed-size"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_MAX[] =
    { D_DESC_T("It may hold at most "), D_DESC_F(D_MEM_DESC_ARENA_MAX),
      D_DESC_T(" byte"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, PROVENANCE)
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_FOREIGN[] =
    { D_DESC_T("over a caller's buffer"), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, POLICY)
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_NEXT[] =
    { D_DESC_T("Its next region will be "), D_DESC_F(D_MEM_DESC_ARENA_NEXT),
      D_DESC_T(" byte"), D_DESC_PL, D_DESC_T(", by "),
      D_DESC_F(D_MEM_DESC_ARENA_GROWTH), D_DESC_T(" growth."), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_ALIGN[] =
    { D_DESC_T("Its regions are aligned to "),
      D_DESC_F(D_MEM_DESC_ARENA_ALIGN), D_DESC_T(" byte"), D_DESC_PL,
      D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, PROVENANCE)
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA_SOURCE[] =
    { D_DESC_T("Its regions come from "), D_DESC_F(D_MEM_DESC_ARENA_SOURCE),
      D_DESC_T("."), D_DESC_END };
#endif
static const struct dj_desc_op D_MEM_DESC_OPS_ARENA[] =
    { D_DESC_T("arena"), D_DESC_END };

static const struct dj_desc_unit d_internal_mem_desc_arena_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_MEM_DESC_ARENA_IF_EMPTY, 0u, D_MEM_DESC_OPS_EMPTY),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY, D_DESC_SLOT_SIZE,
                D_MEM_DESC_ARENA_IF_FIXED, 0u, D_MEM_DESC_OPS_FIXED_SIZE),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_MEM_DESC_OPS_ARENA),
#if D_DESC_BUILT(SENTENCE, PROVENANCE)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_COMPLEMENT, D_MEM_DESC_ARENA_IF_FOREIGN, 0u,
                D_MEM_DESC_OPS_ARENA_FOREIGN),
#endif
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_MEM_DESC_ARENA_IF_EMPTY, D_MEM_DESC_OPS_ARENA_USED),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                D_MEM_DESC_ARENA_IF_CHAINED, 0u,
                D_MEM_DESC_OPS_ARENA_REGIONS),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_MEM_DESC_ARENA_IF_BOUNDED, 0u,
                D_MEM_DESC_OPS_ARENA_MAX),
#endif
#if D_DESC_BUILT(DETAILED, POLICY)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, 0u, D_MEM_DESC_ARENA_IF_FIXED,
                D_MEM_DESC_OPS_ARENA_NEXT),
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, 0u, 0u, D_MEM_DESC_OPS_ARENA_ALIGN),
#endif
#if D_DESC_BUILT(DETAILED, PROVENANCE)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_REMARK, D_MEM_DESC_ARENA_IF_NAMED, 0u,
                D_MEM_DESC_OPS_ARENA_SOURCE),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_mem_desc_arena_fields[] =
{
    D_DESC_FIELD("used", "bytes used", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("capacity", "bytes", D_DESC_LAYER_COST, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("region_count", "regions", D_DESC_LAYER_STATE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("max_bytes", "at most", D_DESC_LAYER_POLICY, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("align", "alignment", D_DESC_LAYER_POLICY, 2u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("next_bytes", "next region", D_DESC_LAYER_POLICY, 3u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("fixed", "fixed-size", D_DESC_LAYER_POLICY, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("caller_buffer", "over a caller's buffer",
                 D_DESC_LAYER_PROVENANCE, 2u, D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("growth", "growth", D_DESC_LAYER_POLICY, 2u,
                 D_MEM_DESC_GROWTH_TRUTH),
    D_DESC_FIELD("source", "source", D_DESC_LAYER_PROVENANCE, 2u,
                 D_DESC_TRUTH_IMPLEMENTATION)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_arena =
{
    "arena",
    "arena",
    D_DESC_UNITS_TABLE(d_internal_mem_desc_arena_units),
    D_DESC_FIELDS_TABLE(d_internal_mem_desc_arena_fields),
    d_internal_mem_desc_arena_plan,
    d_internal_mem_desc_arena_read,
    D_MEM_DESC_ARENA_CONTENTS
};


//==============================================================================
// 6.  THE POOL
//==============================================================================

// the fields, in the field list's order
#define D_MEM_DESC_POOL_LIVE              0u
#define D_MEM_DESC_POOL_CAPACITY          1u
#define D_MEM_DESC_POOL_SLOT_SIZE         2u
#define D_MEM_DESC_POOL_POLICY            3u
#define D_MEM_DESC_POOL_MAX               4u
#define D_MEM_DESC_POOL_FREE              5u
#define D_MEM_DESC_POOL_BLOCKS            6u
#define D_MEM_DESC_POOL_PER_BLOCK         7u
#define D_MEM_DESC_POOL_STRIDE            8u
#define D_MEM_DESC_POOL_ALIGN             9u
#define D_MEM_DESC_POOL_SOURCE            10u

// the plan's bits
#define D_MEM_DESC_POOL_IF_EMPTY          (1u << 0)
#define D_MEM_DESC_POOL_IF_FULL           (1u << 1)
#define D_MEM_DESC_POOL_IF_MONOTONIC      (1u << 2)
#define D_MEM_DESC_POOL_IF_FREE_LIST      (1u << 3)
#define D_MEM_DESC_POOL_IF_GENERATIONAL   (1u << 4)
#define D_MEM_DESC_POOL_IF_BOUNDED        (1u << 5)
#define D_MEM_DESC_POOL_IF_REUSE          (1u << 6)
#define D_MEM_DESC_POOL_IF_BLOCKS         (1u << 7)
#define D_MEM_DESC_POOL_IF_NAMED          (1u << 8)

// how a policy is written: enum dj_pool_policy's order
static const char* const D_MEM_DESC_POLICY_NAMES[] =
{
    "monotonic", "free_list", "generational"
};

/*
d_internal_mem_desc_pool_free
  The released slots waiting for reuse; none where the build keeps no free
list.
*/
static dj_pool_index
d_internal_mem_desc_pool_free(
    const struct dj_pool* _pool
)
{
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    return _pool->free_count;
#else
    (void)_pool;

    return 0u;
#endif
}

/*
d_internal_mem_desc_pool_plan
  The state at the line; the policy and the ceiling at the sentence; reuse,
blocks and where they come from at the detail.
*/
static uint32_t
d_internal_mem_desc_pool_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_pool* pool = (const struct dj_pool*)_object;
    uint32_t              bits = 0u;

    if (_ring >= D_DESC_RING_LINE)
    {
        bits |= (pool->live == 0u) ? D_MEM_DESC_POOL_IF_EMPTY : 0u;
        bits |= ( (pool->max_slots != 0u) &&
                  (pool->live >= pool->max_slots) )
                    ? D_MEM_DESC_POOL_IF_FULL : 0u;
    }

    if (_ring >= D_DESC_RING_SENTENCE)
    {
        bits |= (pool->policy == (uint8_t)D_POOL_POLICY_MONOTONIC)
                    ? D_MEM_DESC_POOL_IF_MONOTONIC : 0u;
        bits |= (pool->policy == (uint8_t)D_POOL_POLICY_FREE_LIST)
                    ? D_MEM_DESC_POOL_IF_FREE_LIST : 0u;
        bits |= (pool->policy == (uint8_t)D_POOL_POLICY_GENERATIONAL)
                    ? D_MEM_DESC_POOL_IF_GENERATIONAL : 0u;
        bits |= (pool->max_slots != 0u) ? D_MEM_DESC_POOL_IF_BOUNDED : 0u;
    }

    if (_ring >= D_DESC_RING_DETAILED)
    {
        bits |= (d_internal_mem_desc_pool_free(pool) != 0u)
                    ? D_MEM_DESC_POOL_IF_REUSE : 0u;
        bits |= (pool->block_count != 0u) ? D_MEM_DESC_POOL_IF_BLOCKS : 0u;
        bits |= (d_internal_mem_desc_source_name(&pool->source) != NULL)
                    ? D_MEM_DESC_POOL_IF_NAMED : 0u;
    }

    return bits;
}

/*
d_internal_mem_desc_pool_read
  Reads one field; a policy past the enum's is none.
*/
static struct dj_ostream_value
d_internal_mem_desc_pool_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_pool* pool = (const struct dj_pool*)_object;

    switch (_field)
    {
        case D_MEM_DESC_POOL_LIVE:
            return d_internal_mem_desc_index(pool->live);
        case D_MEM_DESC_POOL_CAPACITY:
            return d_internal_mem_desc_index(pool->capacity);
        case D_MEM_DESC_POOL_SLOT_SIZE:
            return d_internal_mem_desc_size(pool->payload);
        case D_MEM_DESC_POOL_POLICY:
            return (pool->policy < 3u)
                       ? d_ostream_value_text(
                             D_MEM_DESC_POLICY_NAMES[pool->policy])
                       : d_ostream_value_none();
        case D_MEM_DESC_POOL_MAX:
            return d_internal_mem_desc_index(pool->max_slots);
        case D_MEM_DESC_POOL_FREE:
            return d_internal_mem_desc_index(
                       d_internal_mem_desc_pool_free(pool));
        case D_MEM_DESC_POOL_BLOCKS:
            return d_internal_mem_desc_size(pool->block_count);
        case D_MEM_DESC_POOL_PER_BLOCK:
            return d_internal_mem_desc_index(pool->slots_per_block);
        case D_MEM_DESC_POOL_STRIDE:
            return d_internal_mem_desc_size(pool->stride);
        case D_MEM_DESC_POOL_ALIGN:
            return d_internal_mem_desc_size(pool->align);
        case D_MEM_DESC_POOL_SOURCE:
            return d_ostream_value_text(
                       d_internal_mem_desc_source_name(&pool->source));
        default:
            return d_ostream_value_none();
    }
}

#if (D_INTERNAL_MEM_STATS == 1)
/*
d_internal_mem_desc_pool_contents
  The pool's tally, one level down.
*/
static int
d_internal_mem_desc_pool_contents(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    return d_internal_mem_desc_stats_contents(
               &((const struct dj_pool*)_object)->stats, _sink);
}

    #define D_MEM_DESC_POOL_CONTENTS  d_internal_mem_desc_pool_contents
#else
    #define D_MEM_DESC_POOL_CONTENTS  NULL
#endif  // D_INTERNAL_MEM_STATS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_FULL[] =
    { D_DESC_T("full"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_POOL_LIVE[] =
    { D_DESC_F(D_MEM_DESC_POOL_LIVE), D_DESC_T(" of "),
      D_DESC_F(D_MEM_DESC_POOL_CAPACITY), D_DESC_T(" slot"), D_DESC_PL,
      D_DESC_T(" in use"), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
static const struct dj_desc_op D_MEM_DESC_OPS_MONOTONIC[] =
    { D_DESC_T("monotonic"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_FREE_LIST[] =
    { D_DESC_T("free-list"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_GENERATIONAL[] =
    { D_DESC_T("generational"), D_DESC_END };
static const struct dj_desc_op D_MEM_DESC_OPS_POOL_MAX[] =
    { D_DESC_T("It may hold at most "), D_DESC_F(D_MEM_DESC_POOL_MAX),
      D_DESC_T(" slot"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, TYPE)
static const struct dj_desc_op D_MEM_DESC_OPS_POOL_OF[] =
    { D_DESC_T("of "), D_DESC_F(D_MEM_DESC_POOL_SLOT_SIZE),
      D_DESC_T("-byte slots"), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, STATE)
static const struct dj_desc_op D_MEM_DESC_OPS_POOL_REUSE[] =
    { D_DESC_T("It has "), D_DESC_F(D_MEM_DESC_POOL_FREE),
      D_DESC_T(" slot"), D_DESC_PL, D_DESC_T(" to reuse."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, COST)
static const struct dj_desc_op D_MEM_DESC_OPS_POOL_BLOCKS[] =
    { D_DESC_T("Its slots are "), D_DESC_F(D_MEM_DESC_POOL_STRIDE),
      D_DESC_T(" byte"), D_DESC_PL, D_DESC_T(" apart, in "),
      D_DESC_F(D_MEM_DESC_POOL_BLOCKS), D_DESC_T(" block"), D_DESC_PL,
      D_DESC_T(" of "), D_DESC_F(D_MEM_DESC_POOL_PER_BLOCK), D_DESC_T("."),
      D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, PROVENANCE)
static const struct dj_desc_op D_MEM_DESC_OPS_POOL_SOURCE[] =
    { D_DESC_T("Its blocks come from "), D_DESC_F(D_MEM_DESC_POOL_SOURCE),
      D_DESC_T("."), D_DESC_END };
#endif
static const struct dj_desc_op D_MEM_DESC_OPS_POOL[] =
    { D_DESC_T("pool"), D_DESC_END };

static const struct dj_desc_unit d_internal_mem_desc_pool_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_MEM_DESC_POOL_IF_EMPTY, 0u, D_MEM_DESC_OPS_EMPTY),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_MEM_DESC_POOL_IF_FULL, 0u, D_MEM_DESC_OPS_FULL),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_QUALITY, D_MEM_DESC_POOL_IF_MONOTONIC, 0u,
                D_MEM_DESC_OPS_MONOTONIC),
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_QUALITY, D_MEM_DESC_POOL_IF_FREE_LIST, 0u,
                D_MEM_DESC_OPS_FREE_LIST),
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_QUALITY, D_MEM_DESC_POOL_IF_GENERATIONAL, 0u,
                D_MEM_DESC_OPS_GENERATIONAL),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_MEM_DESC_OPS_POOL),
#if D_DESC_BUILT(SENTENCE, TYPE)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_TYPE,
                D_DESC_SLOT_COMPLEMENT, 0u, 0u, D_MEM_DESC_OPS_POOL_OF),
#endif
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_MEM_DESC_POOL_IF_EMPTY, D_MEM_DESC_OPS_POOL_LIVE),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_MEM_DESC_POOL_IF_BOUNDED, 0u,
                D_MEM_DESC_OPS_POOL_MAX),
#endif
#if D_DESC_BUILT(DETAILED, STATE)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_STATE,
                D_DESC_SLOT_REMARK, D_MEM_DESC_POOL_IF_REUSE, 0u,
                D_MEM_DESC_OPS_POOL_REUSE),
#endif
#if D_DESC_BUILT(DETAILED, COST)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST,
                D_DESC_SLOT_REMARK, D_MEM_DESC_POOL_IF_BLOCKS, 0u,
                D_MEM_DESC_OPS_POOL_BLOCKS),
#endif
#if D_DESC_BUILT(DETAILED, PROVENANCE)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_REMARK, D_MEM_DESC_POOL_IF_NAMED, 0u,
                D_MEM_DESC_OPS_POOL_SOURCE),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_mem_desc_pool_fields[] =
{
    D_DESC_FIELD("live", "slots in use", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("capacity", "slots", D_DESC_LAYER_COST, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("slot_size", "bytes a slot", D_DESC_LAYER_TYPE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("policy", "policy", D_DESC_LAYER_POLICY, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("max_slots", "at most", D_DESC_LAYER_POLICY, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("free_count", "slots to reuse", D_DESC_LAYER_STATE, 2u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("block_count", "blocks", D_DESC_LAYER_COST, 2u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("slots_per_block", "slots a block", D_DESC_LAYER_COST, 3u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("stride", "bytes between slots", D_DESC_LAYER_COST, 3u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("align", "alignment", D_DESC_LAYER_POLICY, 3u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("source", "source", D_DESC_LAYER_PROVENANCE, 2u,
                 D_DESC_TRUTH_IMPLEMENTATION)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_pool =
{
    "pool",
    "pool",
    D_DESC_UNITS_TABLE(d_internal_mem_desc_pool_units),
    D_DESC_FIELDS_TABLE(d_internal_mem_desc_pool_fields),
    d_internal_mem_desc_pool_plan,
    d_internal_mem_desc_pool_read,
    D_MEM_DESC_POOL_CONTENTS
};


#endif  // D_INTERNAL_DESC_OF_MEMORY


//==============================================================================
// 7.  LAYOUT
//==============================================================================

/*
d_desc_of_mem_layout
  Memory's layout rows as this library was built: the header's table,
compiled with the library's knobs.
*/
size_t
d_desc_of_mem_layout(
    const struct dj_desc_of_layout** _rows
)
{
    return d_desc_of_mem_layout_here(_rows);
}
