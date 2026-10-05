/*******************************************************************************
* djinterp [c]                                                           arena.c
*
*   Implementation unit for arena.h. Carries the region geometry, the growth
* policy, the bump path, mark and rewind, and the operation table that presents
* an arena as a d_mem_source.
*
*
* path:      /src/djinterp/c/memory/arena.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#include "../../../../inc/djinterp/c/memory/arena.h"


///////////////////////////////////////////////////////////////////////////////
///                       I.   INTERNAL GEOMETRY                            ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_ARENA_CURRENT
//   macro (internal): the region the bump cursor is in. With chaining that is
// a distinct field; without it, the head is the only region there is. One
// macro, so every bump, query and traversal below is written once rather than
// twice.
#if (D_INTERNAL_ARENA_CHAIN == 1)
#   define D_INTERNAL_ARENA_CURRENT(_a)   ((_a)->current)
#else
#   define D_INTERNAL_ARENA_CURRENT(_a)   ((_a)->head)
#endif

// D_INTERNAL_ARENA_LEAD
//   macro (internal): the bytes reserved BEFORE a payload for its leading
// guard band. Defers to mem_common's kernel, which owns the rounding rule --
// the pool needs the identical answer, and two copies of it disagreed once.
#define D_INTERNAL_ARENA_LEAD(_align)   d_mem_redzone_lead(_align)


/*
d_internal_arena_payload_offset
  Computes where a region's payload begins, measured from the region header.

Parameter(s):
  _align: the arena's region alignment.
Return:
  The header size rounded up to _align. Because the region block itself is
requested at _align, this offset lands the payload on an _align boundary
without any further arithmetic at allocation time.
*/
static d_mem_size
d_internal_arena_payload_offset(
    d_mem_size _align
)
{
    return d_mem_align_up((d_mem_size)sizeof(struct d_arena_region), _align);
}


/*
d_internal_arena_payload
  Computes the address of a region's payload.

Parameter(s):
  _region: the region header; must not be NULL.
  _align:  the arena's region alignment.
Return:
  A pointer to the first payload byte.
*/
static char*
d_internal_arena_payload(
    struct d_arena_region* _region,
    d_mem_size             _align
)
{
    return ( (char*)_region +
             (size_t)d_internal_arena_payload_offset(_align) );
}


/*
d_internal_arena_block_bytes
  Computes the total size of the upstream block backing a region of the given
payload capacity.
  This is recomputed rather than stored, which is why the region header is
three fields instead of four: the formula is deterministic given the arena's
alignment, and that alignment is fixed for the arena's lifetime.

Parameter(s):
  _capacity: the region's payload capacity.
  _align:    the arena's region alignment.
Return:
  The block size in bytes, or 0 if the computation would overflow.
*/
static d_mem_size
d_internal_arena_block_bytes(
    d_mem_size _capacity,
    d_mem_size _align
)
{
    d_mem_size offset;

    offset = d_internal_arena_payload_offset(_align);

    if (d_mem_add_would_overflow(offset, _capacity))
    {
        return 0;
    }

    return (d_mem_size)(offset + _capacity);
}


/*
d_internal_arena_can_grow
  Reports whether this arena is permitted to obtain another region.

Parameter(s):
  _arena: the arena; must not be NULL.
Return:
  A boolean value corresponding to either:
  - true, if all of the following are true:
    - the build compiled chaining,
    - the instance is not flagged fixed, and
    - the instance has a first region already, or
  - false, otherwise.
*/
static bool
d_internal_arena_can_grow(
    const struct d_arena* _arena
)
{
#if (D_INTERNAL_ARENA_CHAIN == 1)
    return ((_arena->flags & D_ARENA_FLAG_FIXED) == 0);
#else
    // an unchained arena may still take its FIRST region; it simply may not
    // take a second
    return ( ((_arena->flags & D_ARENA_FLAG_FIXED) == 0) &&
             (_arena->head == NULL) );
#endif
}


#if (D_INTERNAL_ARENA_CHAIN == 1)

/*
d_internal_arena_growth_policy
  Reports the growth policy in force for an arena.

Parameter(s):
  _arena: the arena; must not be NULL.
Return:
  The instance's policy where the policy is a field, or the compile-time
policy where it is pinned.
*/
static enum d_arena_growth
d_internal_arena_growth_policy(
    const struct d_arena* _arena
)
{
#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
    return _arena->growth;
#else
    D_MEM_UNUSED(_arena);

    return (enum d_arena_growth)D_INTERNAL_ARENA_GROWTH;
#endif
}

#endif  // D_INTERNAL_ARENA_CHAIN


/*
d_internal_arena_next_capacity
  Chooses the payload capacity of the next region.

  Three constraints meet here, in this order:
    1. the growth policy proposes a speculative size,
    2. the per-region ceiling caps it -- so that a doubling arena's twentieth
       region is not measured in gigabytes,
    3. the actual need overrides both -- a request larger than the ceiling is
       still satisfied, because the ceiling bounds SPECULATION, not need,
    4. the arena's total budget clamps the result, and a clamp that lands
       below the need means the arena is full.

Parameter(s):
  _arena:  the arena; must not be NULL.
  _needed: the payload bytes this region must be able to satisfy immediately.
Return:
  The payload capacity to request, or 0 when the budget cannot cover _needed.
*/
static d_mem_size
d_internal_arena_next_capacity(
    const struct d_arena* _arena,
    d_mem_size            _needed
)
{
    d_mem_size proposed;
    d_mem_size budget;

    proposed = _arena->next_bytes;
    budget   = 0;

    if (proposed == 0)
    {
        proposed = D_INTERNAL_ARENA_REGION_BYTES;
    }

    // (2) the per-region ceiling bounds speculative growth
    if ( (D_INTERNAL_ARENA_MAX_REGION != 0) &&
         (proposed > D_INTERNAL_ARENA_MAX_REGION) )
    {
        proposed = D_INTERNAL_ARENA_MAX_REGION;
    }

    // (3) need overrides the ceiling; a caller asking for more than a whole
    // region gets a region sized to the ask
    if (proposed < _needed)
    {
        proposed = _needed;
    }

    // (4) the arena's total budget is the last word
    if (_arena->max_bytes != 0)
    {
        if (_arena->capacity >= _arena->max_bytes)
        {
            return 0;
        }

        budget = (d_mem_size)(_arena->max_bytes - _arena->capacity);

        if (proposed > budget)
        {
            proposed = budget;
        }

        // a budget that cannot cover the need means the arena is full, and
        // saying so here avoids an upstream call that could only fail
        if (proposed < _needed)
        {
            return 0;
        }
    }

    return proposed;
}


#if (D_INTERNAL_ARENA_CHAIN == 1)

/*
d_internal_arena_advance_next_bytes
  Advances the speculative region size according to the growth policy, after a
region of _just_taken bytes has been added.
  The exponential step is an integer ratio rather than a float multiply, so
that two builds and two languages compute the same sizes -- which the parity
law requires and which a float would not deliver across architectures.

Parameter(s):
  _arena:      the arena; must not be NULL.
  _just_taken: the payload capacity of the region just added.
Return:
  none.
*/
static void
d_internal_arena_advance_next_bytes(
    struct d_arena* _arena,
    d_mem_size      _just_taken
)
{
    d_mem_size next;

    next = _just_taken;

    switch (d_internal_arena_growth_policy(_arena))
    {
        case D_ARENA_GROWTH_FIXED:
            // the size the arena was configured with, forever
            next = _arena->next_bytes;
            break;

        case D_ARENA_GROWTH_LINEAR:
            if (!d_mem_add_would_overflow(next,
                                          D_INTERNAL_ARENA_GROWTH_STEP))
            {
                next = (d_mem_size)(next + D_INTERNAL_ARENA_GROWTH_STEP);
            }
            break;

        case D_ARENA_GROWTH_EXPONENTIAL:
        default:
            if (!d_mem_mul_would_overflow(next,
                                          D_INTERNAL_ARENA_GROWTH_NUM))
            {
                next = (d_mem_size)((next * D_INTERNAL_ARENA_GROWTH_NUM) /
                                    D_INTERNAL_ARENA_GROWTH_DEN);
            }
            break;
    }

    // a ratio that rounds down to no growth would stall the arena at one
    // region size forever, so the step is floored at "no smaller than before"
    if (next < _just_taken)
    {
        next = _just_taken;
    }

    if ( (D_INTERNAL_ARENA_MAX_REGION != 0) &&
         (next > D_INTERNAL_ARENA_MAX_REGION) )
    {
        next = D_INTERNAL_ARENA_MAX_REGION;
    }

    _arena->next_bytes = next;

    return;
}

#endif  // D_INTERNAL_ARENA_CHAIN


/*
d_internal_arena_grow
  Obtains one more region from the upstream source and links it in.

Parameter(s):
  _arena:  the arena; must not be NULL.
  _needed: the payload bytes the new region must be able to satisfy.
Return:
  D_MEM_OK on success, D_MEM_ERR_OVERFLOW when the geometry would not fit in a
d_mem_size, or D_MEM_ERR_EXHAUSTED when this arena may not grow, when its
budget forbids it, or when the upstream refused.
  NOTE that an arena which may not grow reports EXHAUSTED rather than
UNSUPPORTED. The caller asked for an allocation, not for growth, and "there is
no more room" is exhaustion however the room came to run out --
D_MEM_ERR_UNSUPPORTED is reserved for an OPERATION the configuration forbids,
such as releasing an individual block back to an arena.
*/
static enum d_mem_status
d_internal_arena_grow(
    struct d_arena* _arena,
    d_mem_size      _needed
)
{
    d_mem_size             capacity;
    d_mem_size             block_bytes;
    void*                  block;
    struct d_arena_region* region;

    if (!d_internal_arena_can_grow(_arena))
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    capacity = d_internal_arena_next_capacity(_arena, _needed);

    if (capacity == 0)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    block_bytes = d_internal_arena_block_bytes(capacity, _arena->align);

    if (block_bytes == 0)
    {
        return D_MEM_ERR_OVERFLOW;
    }

    block = d_mem_source_allocate(&_arena->source,
                                  block_bytes,
                                  _arena->align);

    if (!block)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    region           = (struct d_arena_region*)block;
    region->capacity = capacity;
    region->used     = 0;

#if (D_INTERNAL_ARENA_CHAIN == 1)
    region->next = NULL;

    // link at the tail so that the chain stays in allocation order, which is
    // what makes reset's traversal and owns's traversal both predictable
    if (_arena->tail)
    {
        _arena->tail->next = region;
    }
    else
    {
        _arena->head = region;
    }

    _arena->tail    = region;
    _arena->current = region;
    _arena->region_count =
        (d_mem_size)(_arena->region_count + 1);

    d_internal_arena_advance_next_bytes(_arena, capacity);
#else
    _arena->head = region;
#endif

    _arena->capacity = (d_mem_size)(_arena->capacity + capacity);

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_reserve(&_arena->stats, block_bytes);
#endif

    return D_MEM_OK;
}


/*
d_internal_arena_bump
  Attempts to satisfy a request from the region the cursor is currently in.
  This is the hot path, and it is deliberately branch-light: one alignment
computation, two overflow guards, one comparison, one add.

Parameter(s):
  _arena: the arena; must not be NULL.
  _bytes: the payload size.
  _align: the normalized alignment.
Return:
  A pointer to the payload, or NULL when the current region cannot satisfy the
request (which is not an error -- the caller then grows).
*/
static void*
d_internal_arena_bump(
    struct d_arena* _arena,
    d_mem_size      _bytes,
    d_mem_size      _align
)
{
    struct d_arena_region* region;
    char*                  base;
    d_mem_size             lead;
    d_mem_size             padding;
    d_mem_size             start;
    d_mem_size             span;
    char*                  payload;

    region = D_INTERNAL_ARENA_CURRENT(_arena);

    if (!region)
    {
        return NULL;
    }

    base    = d_internal_arena_payload(region, _arena->align);
    lead    = D_INTERNAL_ARENA_LEAD(_align);
    padding = d_mem_align_padding(
                  (d_mem_size)(uintptr_t)(base + region->used),
                  _align);

    if (d_mem_add_would_overflow(region->used, padding))
    {
        return NULL;
    }

    start = (d_mem_size)(region->used + padding);

    // the span is the leading guard, the payload, and the trailing guard.
    // Both guards are zero-width when redzones are off, so this arithmetic
    // is the same expression in every configuration
    if (d_mem_add_would_overflow(_bytes, lead) ||
        d_mem_add_would_overflow((d_mem_size)(_bytes + lead),
                                 D_MEM_REDZONE_BYTES))
    {
        return NULL;
    }

    span = (d_mem_size)(_bytes + lead + D_MEM_REDZONE_BYTES);

    if (d_mem_add_would_overflow(start, span))
    {
        return NULL;
    }

    // the only comparison on the hot path
    if ((d_mem_size)(start + span) > region->capacity)
    {
        return NULL;
    }

    payload      = base + (size_t)start + (size_t)lead;
    region->used = (d_mem_size)(start + span);

    return (void*)payload;
}


///////////////////////////////////////////////////////////////////////////////
///                        II.   LIFECYCLE                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_arena_config_default
  Builds the configuration an arena takes when none is supplied.
  Every field is the zero that d_arena_init already interprets as "default",
so this function exists to be READ rather than to be necessary: a caller can
take it, change one field, and pass it, without having to know which zeros
mean what.

Parameter(s):
  none.
Return:
  A zeroed configuration, which names the configured defaults throughout.
*/
struct d_arena_config
d_arena_config_default(void)
{
    struct d_arena_config config;

    config.source        = d_mem_source_none();
    config.initial_bytes = 0;
    config.max_bytes     = 0;
    config.align         = 0;
    config.growth        = (enum d_arena_growth)D_INTERNAL_ARENA_GROWTH;

    return config;
}


/*
d_arena_init
  Prepares an arena.
  NO MEMORY IS OBTAINED HERE. The first region is taken on the first
allocation, so an arena that is created and never used costs its struct and
nothing else -- which is what makes it reasonable to give one to every object
that might need one.

Parameter(s):
  _arena:  the arena to prepare; must not be NULL.
  _config: how it differs from the defaults; may be NULL, meaning "no
           difference".
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when _arena is NULL, or
D_MEM_ERR_INVALID when the configuration names an inadmissible alignment.
*/
enum d_mem_status
d_arena_init(
    struct d_arena*              _arena,
    const struct d_arena_config* _config
)
{
    struct d_arena_config config;
    d_mem_size            align;

    D_MEM_REQUIRE(_arena != NULL, D_MEM_ERR_NULL);

    config = (_config)
        ? *_config
        : d_arena_config_default();

    // the configured region alignment stands in for a config that names none
    align = (config.align != 0)
        ? config.align
        : D_INTERNAL_ARENA_REGION_ALIGN;

    align = d_mem_align_accept(align);

    if (align == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    // a region header must itself be correctly aligned inside the block, and
    // the block is requested at `align` -- so `align` must cover the header
    if (align < D_MEM_LINK_ALIGN)
    {
        align = D_MEM_LINK_ALIGN;
    }

    _arena->source    = d_mem_source_resolve(&config.source);
    _arena->head      = NULL;
    _arena->capacity  = 0;
    _arena->max_bytes = config.max_bytes;
    _arena->align     = align;
    _arena->flags     = 0;

    _arena->next_bytes = (config.initial_bytes != 0)
        ? config.initial_bytes
        : D_INTERNAL_ARENA_REGION_BYTES;

#if (D_INTERNAL_ARENA_CHAIN == 1)
    _arena->current      = NULL;
    _arena->tail         = NULL;
    _arena->region_count = 0;
#endif

#if (D_INTERNAL_ARENA_GROWTH_RUNTIME == 1)
    _arena->growth = config.growth;
#endif

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    _arena->last_ptr  = NULL;
    _arena->last_size = 0;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_clear(&_arena->stats);
#endif

    return D_MEM_OK;
}


/*
d_arena_init_buffer
  Prepares an arena over memory the caller already owns, with no upstream
source at all.
  This is the freestanding shape: a static array becomes a complete allocator,
and nothing in the program calls malloc. The arena places its region header at
the front of the buffer and vends the remainder; d_arena_release will NOT hand
the buffer to any source, because it did not come from one.

Parameter(s):
  _arena:  the arena to prepare; must not be NULL.
  _buffer: the caller's memory; must not be NULL and must outlive the arena.
  _bytes:  its length; must be large enough for the region header plus at
           least one payload byte.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when a pointer argument is NULL, or
D_MEM_ERR_INVALID when the buffer is too small to hold a region.
*/
enum d_mem_status
d_arena_init_buffer(
    struct d_arena* _arena,
    void*           _buffer,
    d_mem_size      _bytes
)
{
    enum d_mem_status      status;
    struct d_arena_config  config;
    char*                  start;
    char*                  end;
    d_mem_size             header_pad;
    d_mem_size             offset;
    struct d_arena_region* region;

    D_MEM_REQUIRE(_arena != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_buffer != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_bytes != 0, D_MEM_ERR_INVALID);

    config        = d_arena_config_default();
    config.source = d_mem_source_none();

    status = d_arena_init(_arena, &config);

    if (status != D_MEM_OK)
    {
        return status;
    }

    start = (char*)_buffer;
    end   = start + (size_t)_bytes;

    // the caller's buffer carries no alignment promise, so the header is
    // placed at the first suitably aligned address inside it
    header_pad = d_mem_align_padding((d_mem_size)(uintptr_t)start,
                                     _arena->align);
    offset     = d_internal_arena_payload_offset(_arena->align);

    if (d_mem_add_would_overflow(header_pad, offset))
    {
        return D_MEM_ERR_INVALID;
    }

    // the buffer must hold the alignment padding, the header, and a payload
    if ((d_mem_size)(header_pad + offset) >= _bytes)
    {
        return D_MEM_ERR_INVALID;
    }

    region           = (struct d_arena_region*)(start + (size_t)header_pad);
    region->capacity = (d_mem_size)(end -
                                    (start + (size_t)header_pad +
                                     (size_t)offset));
    region->used     = 0;

#if (D_INTERNAL_ARENA_CHAIN == 1)
    region->next         = NULL;
    _arena->current      = region;
    _arena->tail         = region;
    _arena->region_count = 1;
#endif

    _arena->head     = region;
    _arena->capacity = region->capacity;

    // the head is the caller's memory and the arena may never take a second
    // region: it has no source to take one from
    _arena->flags = (uint32_t)(D_ARENA_FLAG_FOREIGN_HEAD |
                               D_ARENA_FLAG_FIXED);

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_reserve(&_arena->stats, _bytes);
#endif

    return D_MEM_OK;
}


/*
d_arena_reset
  Reclaims every byte the arena has handed out, keeping the regions for reuse.
  NO DESTRUCTOR RUNS. The arena deals in raw storage; anything with a lifetime
must have been finalized by the caller before this point. That is the arena's
contract everywhere, and the reason it is fast.

Parameter(s):
  _arena: the arena to reset; may be NULL.
Return:
  none.
*/
void
d_arena_reset(
    struct d_arena* _arena
)
{
    struct d_arena_region* region;

    if (!_arena)
    {
        return;
    }

    region = _arena->head;

    while (region)
    {
        d_mem_poison_free((void*)d_internal_arena_payload(region,
                                                          _arena->align),
                          region->used);
        region->used = 0;

#if (D_INTERNAL_ARENA_CHAIN == 1)
        region = region->next;
#else
        region = NULL;
#endif
    }

#if (D_INTERNAL_ARENA_CHAIN == 1)
    // the cursor returns to the first region; every later region stays in the
    // chain and will be reused in order, so a reset arena performs exactly as
    // a fresh one without paying for the regions again
    _arena->current = _arena->head;
#endif

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    _arena->last_ptr  = NULL;
    _arena->last_size = 0;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_reset(&_arena->stats);
#endif

    return;
}


/*
d_arena_trim
  Releases every region after the first, and rewinds the first.
  The middle ground between reset (keep everything) and release (give
everything back): an arena that spiked once and will not spike again gives the
spike back but keeps the region it uses every time.

Parameter(s):
  _arena: the arena to trim; may be NULL.
Return:
  none.
*/
void
d_arena_trim(
    struct d_arena* _arena
)
{
#if (D_INTERNAL_ARENA_CHAIN == 1)
    struct d_arena_region* region;
    struct d_arena_region* next;
    d_mem_size             block_bytes;

    if ( (!_arena) || (!_arena->head) )
    {
        return;
    }

    region = _arena->head->next;

    while (region)
    {
        next        = region->next;
        block_bytes = d_internal_arena_block_bytes(region->capacity,
                                                   _arena->align);

        _arena->capacity =
            (d_mem_size)(_arena->capacity - region->capacity);

#if (D_INTERNAL_MEM_STATS == 1)
        d_mem_stats_on_unreserve(&_arena->stats, block_bytes);
#endif

        d_mem_source_release(&_arena->source,
                             (void*)region,
                             block_bytes,
                             _arena->align);

        region = next;
    }

    _arena->head->next   = NULL;
    _arena->tail         = _arena->head;
    _arena->current      = _arena->head;
    _arena->region_count = 1;
#endif  // D_INTERNAL_ARENA_CHAIN

    if (_arena)
    {
        d_arena_reset(_arena);
    }

    return;
}


/*
d_arena_release
  Returns every region to the upstream source and leaves the arena in its
initialized-but-empty state, ready to be used again.
  A region that came from the caller (d_arena_init_buffer) is NOT released --
it was never the source's to take back.

Parameter(s):
  _arena: the arena to release; may be NULL.
Return:
  none.
*/
void
d_arena_release(
    struct d_arena* _arena
)
{
    struct d_arena_region* region;
    struct d_arena_region* next;
    d_mem_size             block_bytes;
    bool                   foreign_head;

    if (!_arena)
    {
        return;
    }

    region       = _arena->head;
    foreign_head = ((_arena->flags & D_ARENA_FLAG_FOREIGN_HEAD) != 0);

    while (region)
    {
#if (D_INTERNAL_ARENA_CHAIN == 1)
        next = region->next;
#else
        next = NULL;
#endif

        // the caller's own buffer is left alone, but every region the arena
        // obtained is handed back
        if ( (!foreign_head) || (region != _arena->head) )
        {
            block_bytes = d_internal_arena_block_bytes(region->capacity,
                                                       _arena->align);

            d_mem_source_release(&_arena->source,
                                 (void*)region,
                                 block_bytes,
                                 _arena->align);
        }

        region = next;
    }

    _arena->head     = NULL;
    _arena->capacity = 0;
    _arena->flags    = 0;

#if (D_INTERNAL_ARENA_CHAIN == 1)
    _arena->current      = NULL;
    _arena->tail         = NULL;
    _arena->region_count = 0;
#endif

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    _arena->last_ptr  = NULL;
    _arena->last_size = 0;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_clear(&_arena->stats);
#endif

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                       III.   ALLOCATION                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_arena_allocate
  Obtains bytes from the arena.

Parameter(s):
  _arena: the arena to draw from; must not be NULL.
  _bytes: the request size. Zero returns NULL and is not an error.
  _align: the required alignment; 0 means the configured default.
Return:
  A pointer to _bytes uninitialized (or poisoned, or zeroed -- see
D_CFG_MEM_ZERO_ON_ALLOC) bytes, or NULL if the arena could neither satisfy the
request nor grow to satisfy it.
*/
void*
d_arena_allocate(
    struct d_arena* _arena,
    d_mem_size      _bytes,
    d_mem_size      _align
)
{
    struct d_mem_block block;

    if (d_arena_allocate_ex(_arena, _bytes, _align, &block) != D_MEM_OK)
    {
        return NULL;
    }

    return block.ptr;
}


/*
d_arena_allocate_ex
  Obtains bytes from the arena, reporting why a failure failed.
  The two-step shape -- try the current region, grow, try once more -- is the
whole algorithm. There is no third attempt, because a grow sized to the need
either produced a region that can hold it or reported that it could not.

Parameter(s):
  _arena: the arena to draw from; must not be NULL.
  _bytes: the request size; must be non-zero for this form.
  _align: the required alignment; 0 means the configured default.
  _out:   receives the block on success, or the empty block on failure; must
          not be NULL.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_INVALID for a zero size or an
inadmissible alignment, D_MEM_ERR_OVERFLOW, or D_MEM_ERR_EXHAUSTED when the
arena is full and may not (or could not) grow.
*/
enum d_mem_status
d_arena_allocate_ex(
    struct d_arena*     _arena,
    d_mem_size          _bytes,
    d_mem_size          _align,
    struct d_mem_block* _out
)
{
    d_mem_size        align;
    d_mem_size        needed;
    void*             payload;
    enum d_mem_status status;

    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_arena != NULL, D_MEM_ERR_NULL);

    *_out = d_mem_block_empty();

    if (_bytes == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    align = d_mem_align_accept(_align);

    if (align == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    // (1) the fast path: the current region has room
    payload = d_internal_arena_bump(_arena, _bytes, align);

    if (!payload)
    {
        // (2) size the new region so the retry cannot fail for want of room:
        // the payload, both guard bands, and the worst-case alignment padding
        needed = _bytes;

        if (d_mem_add_would_overflow(needed, D_INTERNAL_ARENA_LEAD(align)) ||
            d_mem_add_would_overflow(
                (d_mem_size)(needed + D_INTERNAL_ARENA_LEAD(align)),
                (d_mem_size)(D_MEM_REDZONE_BYTES + align)))
        {
            return D_MEM_ERR_OVERFLOW;
        }

        needed = (d_mem_size)(needed +
                              D_INTERNAL_ARENA_LEAD(align) +
                              D_MEM_REDZONE_BYTES +
                              align);

        status = d_internal_arena_grow(_arena, needed);

        if (status != D_MEM_OK)
        {
            return status;
        }

        // (3) the retry. There is no fourth step: a region sized to the need
        // either holds it or the grow already said it could not
        payload = d_internal_arena_bump(_arena, _bytes, align);

        if (!payload)
        {
            return D_MEM_ERR_EXHAUSTED;
        }
    }

    d_mem_prepare(payload, _bytes);
    d_mem_redzone_write(payload, _bytes);

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    _arena->last_ptr  = payload;
    _arena->last_size = _bytes;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_acquire(&_arena->stats, _bytes);
#endif

    *_out = d_mem_block_make(payload, _bytes);

    return D_MEM_OK;
}


/*
d_arena_allocate_array
  Obtains storage for _count elements of _element_size bytes.
  The multiplication is guarded, which is the reason this function exists
rather than leaving callers to write the product themselves: a count that
overflows the product produces an allocation smaller than the caller believes
it received, and every write into it is then out of bounds.

Parameter(s):
  _arena:        the arena to draw from; must not be NULL.
  _count:        the element count.
  _element_size: the size of one element.
  _align:        the required alignment; 0 means the configured default.
Return:
  A pointer to the array storage, or NULL on overflow or exhaustion.
*/
void*
d_arena_allocate_array(
    struct d_arena* _arena,
    d_mem_size      _count,
    d_mem_size      _element_size,
    d_mem_size      _align
)
{
    d_mem_size total;

    if (d_mem_mul_checked(_count, _element_size, &total) != D_MEM_OK)
    {
        return NULL;
    }

    return d_arena_allocate(_arena, total, _align);
}


/*
d_arena_duplicate
  Copies a byte range into the arena.

Parameter(s):
  _arena:  the arena to draw from; must not be NULL.
  _source: the bytes to copy; must not be NULL.
  _bytes:  how many; must be non-zero.
  _align:  the required alignment; 0 means the configured default.
Return:
  A pointer to the copy, or NULL on failure.
*/
void*
d_arena_duplicate(
    struct d_arena* _arena,
    const void*     _source,
    d_mem_size      _bytes,
    d_mem_size      _align
)
{
    void* copy;

    if ( (!_source) || (_bytes == 0) )
    {
        return NULL;
    }

    copy = d_arena_allocate(_arena, _bytes, _align);

    if (copy)
    {
        memcpy(copy, _source, (size_t)_bytes);
    }

    return copy;
}


/*
d_arena_duplicate_string
  Copies a null-terminated string into the arena, terminator included.
  Present because it is the single most common thing an arena is asked to do
-- a parser, a command line, a configuration reader all intern strings -- and
because writing it out at each call site is where the terminator gets
forgotten.

Parameter(s):
  _arena: the arena to draw from; must not be NULL.
  _text:  the string to copy; must not be NULL.
Return:
  A pointer to the copy, or NULL on failure. The copy is aligned to 1, since a
character array needs nothing more and a stricter alignment would waste
padding on the commonest allocation an arena sees.
*/
char*
d_arena_duplicate_string(
    struct d_arena* _arena,
    const char*     _text
)
{
    size_t     length;
    d_mem_size bytes;
    void*      copy;

    if (!_text)
    {
        return NULL;
    }

    length = strlen(_text);

    if ((d_mem_size)length > (d_mem_size)(D_MEM_SIZE_MAX - 1))
    {
        return NULL;
    }

    bytes = (d_mem_size)(length + 1);
    copy  = d_arena_allocate(_arena, bytes, 1);

    if (copy)
    {
        memcpy(copy, _text, (size_t)bytes);
    }

    return (char*)copy;
}


#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)

/*
d_arena_extend
  Grows the most recent allocation in place when it is still at the top of the
bump cursor, and copies it forward when it is not.
  THIS IS WHAT MAKES A GROWABLE BUFFER CHEAP ON AN ARENA. A loop that appends
and re-extends touches no other allocation in between, so every extension is
the in-place case: a comparison and an add, with no copy at all.

Parameter(s):
  _arena:     the arena; must not be NULL.
  _ptr:       the allocation to grow; must not be NULL.
  _old_bytes: its current size.
  _new_bytes: the requested size; must be at least _old_bytes.
  _align:     the alignment it was allocated with.
Return:
  A pointer to the enlarged allocation, or NULL on failure. On failure the
original is untouched.
*/
void*
d_arena_extend(
    struct d_arena* _arena,
    void*           _ptr,
    d_mem_size      _old_bytes,
    d_mem_size      _new_bytes,
    d_mem_size      _align
)
{
    struct d_arena_region* region;
    char*                  base;
    d_mem_size             offset;
    d_mem_size             delta;
    void*                  moved;

    if ( (!_arena) || (!_ptr) || (_new_bytes < _old_bytes) )
    {
        return NULL;
    }

    if (_new_bytes == _old_bytes)
    {
        return _ptr;
    }

    region = D_INTERNAL_ARENA_CURRENT(_arena);

    // the in-place case: this is the most recent allocation, it lives in the
    // current region, and the region has room for the difference
    if ( (region) &&
         (_arena->last_ptr == _ptr) &&
         (_arena->last_size == _old_bytes) )
    {
        base   = d_internal_arena_payload(region, _arena->align);
        offset = (d_mem_size)((char*)_ptr - base);
        delta  = (d_mem_size)(_new_bytes - _old_bytes);

        if ( (offset + _old_bytes + D_MEM_REDZONE_BYTES == region->used) &&
             (!d_mem_add_would_overflow(region->used, delta)) &&
             ((d_mem_size)(region->used + delta) <= region->capacity) )
        {
            region->used = (d_mem_size)(region->used + delta);

            d_mem_prepare((void*)((char*)_ptr + (size_t)_old_bytes), delta);
            d_mem_redzone_write(_ptr, _new_bytes);

            _arena->last_size = _new_bytes;

#if (D_INTERNAL_MEM_STATS == 1)
            d_mem_stats_on_acquire(&_arena->stats, delta);
#endif

            return _ptr;
        }
    }

    // the moving case: an ordinary allocation plus a copy
    moved = d_arena_allocate(_arena, _new_bytes, _align);

    if (moved)
    {
        memcpy(moved, _ptr, (size_t)_old_bytes);
    }

    return moved;
}


/*
d_arena_pop_last
  Returns the most recent allocation to the arena.
  The one form of individual release an arena can honour, and only for the
allocation still at the top of the cursor. Anything else is refused rather
than silently ignored, so a caller cannot mistake this for a general free.

Parameter(s):
  _arena: the arena; must not be NULL.
  _ptr:   the allocation to pop; must not be NULL.
  _bytes: its size.
Return:
  A boolean value corresponding to either:
  - true, if the allocation was the most recent and has been reclaimed, or
  - false, if it was not, in which case nothing happened.
*/
bool
d_arena_pop_last(
    struct d_arena* _arena,
    void*           _ptr,
    d_mem_size      _bytes
)
{
    struct d_arena_region* region;
    char*                  base;
    d_mem_size             offset;
    d_mem_size             lead;

    if ( (!_arena) || (!_ptr) )
    {
        return false;
    }

    region = D_INTERNAL_ARENA_CURRENT(_arena);

    if ( (!region) ||
         (_arena->last_ptr != _ptr) ||
         (_arena->last_size != _bytes) )
    {
        return false;
    }

    base   = d_internal_arena_payload(region, _arena->align);
    offset = (d_mem_size)((char*)_ptr - base);
    lead   = D_INTERNAL_ARENA_LEAD(_arena->align);

    // the allocation must still be the last thing in the region
    if (offset + _bytes + D_MEM_REDZONE_BYTES != region->used)
    {
        return false;
    }

    d_mem_poison_free(_ptr, _bytes);

    // the cursor rewinds past the payload and its leading guard; the
    // alignment padding before it is not recovered, which is correct -- the
    // padding belongs to whatever preceded the allocation
    region->used = (offset >= lead)
        ? (d_mem_size)(offset - lead)
        : 0;

    _arena->last_ptr  = NULL;
    _arena->last_size = 0;

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_release(&_arena->stats, _bytes);
#endif

    return true;
}

#endif  // D_INTERNAL_ARENA_LAST_ALLOC


///////////////////////////////////////////////////////////////////////////////
///                     IV.   MARK AND REWIND                               ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_ARENA_MARKS == 1)

/*
d_arena_mark_get
  Takes a mark naming the arena's current position.

Parameter(s):
  _arena: the arena to mark; may be NULL, which yields an empty mark that
          rewind will refuse.
Return:
  A mark. Marks nest: taking several and rewinding to them in reverse order
gives a stack discipline with no per-allocation bookkeeping at all.
*/
struct d_arena_mark
d_arena_mark_get(
    const struct d_arena* _arena
)
{
    struct d_arena_mark mark;

    mark.region = NULL;
    mark.used   = 0;

    if (_arena)
    {
        mark.region = D_INTERNAL_ARENA_CURRENT(_arena);
        mark.used   = (mark.region)
            ? mark.region->used
            : 0;
    }

    return mark;
}


/*
d_arena_rewind
  Restores the arena to a previously marked position.
  Every region after the marked one is rewound to empty but KEPT, so the
memory stays available and a subsequent burst does not pay the upstream again.
That is the difference between rewind and trim, and it is why rewind is the
cheap operation of the two.

Parameter(s):
  _arena: the arena to rewind; must not be NULL.
  _mark:  the mark to restore; must have come from this arena.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when _arena is NULL, or
D_MEM_ERR_FOREIGN when validation is on and the mark does not name a region
this arena owns. An empty mark (from a never-allocated arena) rewinds to
empty, which is the correct reading of it.
*/
enum d_mem_status
d_arena_rewind(
    struct d_arena*     _arena,
    struct d_arena_mark _mark
)
{
#if ( (D_INTERNAL_ARENA_VALIDATE_MARKS == 1) ||                               \
      (D_INTERNAL_ARENA_CHAIN == 1) )
    struct d_arena_region* region;
#endif
#if (D_INTERNAL_ARENA_VALIDATE_MARKS == 1)
    bool                   found;
#endif

    D_MEM_REQUIRE(_arena != NULL, D_MEM_ERR_NULL);

    // an empty mark names the state before any region existed
    if (!_mark.region)
    {
        d_arena_reset(_arena);

        return D_MEM_OK;
    }

#if (D_INTERNAL_ARENA_VALIDATE_MARKS == 1)
    found  = false;
    region = _arena->head;

    // walking the chain is O(regions), which is why this is a testing-mode
    // check: it catches rewinding one arena with another's mark, a mistake
    // that otherwise corrupts both silently
    while (region)
    {
        if (region == _mark.region)
        {
            found = true;
            break;
        }

#   if (D_INTERNAL_ARENA_CHAIN == 1)
        region = region->next;
#   else
        region = NULL;
#   endif
    }

    if ( (!found) || (_mark.used > _mark.region->capacity) )
    {
        return D_MEM_ERR_FOREIGN;
    }
#endif  // D_INTERNAL_ARENA_VALIDATE_MARKS

    // the marked region rewinds to the marked offset
    if (_mark.region->used > _mark.used)
    {
        d_mem_poison_free(
            (void*)(d_internal_arena_payload(_mark.region, _arena->align) +
                    (size_t)_mark.used),
            (d_mem_size)(_mark.region->used - _mark.used));
    }

    _mark.region->used = _mark.used;

#if (D_INTERNAL_ARENA_CHAIN == 1)
    // every later region rewinds to empty and is retained for reuse
    region = _mark.region->next;

    while (region)
    {
        d_mem_poison_free((void*)d_internal_arena_payload(region,
                                                          _arena->align),
                          region->used);
        region->used = 0;
        region       = region->next;
    }

    _arena->current = _mark.region;
#endif

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    _arena->last_ptr  = NULL;
    _arena->last_size = 0;
#endif

    return D_MEM_OK;
}

#endif  // D_INTERNAL_ARENA_MARKS


///////////////////////////////////////////////////////////////////////////////
///                         V.   QUERIES                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_arena_is_valid
  Reports whether an arena has been initialized.

Parameter(s):
  _arena: the arena to test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the arena has a usable source or a region already in hand, or
  - false, otherwise. Note that a freshly initialized arena with no region yet
is valid: it has simply not been asked for anything.
*/
bool
d_arena_is_valid(
    const struct d_arena* _arena
)
{
    if (!_arena)
    {
        return false;
    }

    return ( (d_mem_source_is_valid(&_arena->source)) ||
             (_arena->head != NULL) );
}


/*
d_arena_used
  Reports how many payload bytes the arena has handed out, including
alignment padding and guard bands.

Parameter(s):
  _arena: the arena to measure; may be NULL.
Return:
  The consumed byte count, or 0 when _arena is NULL.
*/
d_mem_size
d_arena_used(
    const struct d_arena* _arena
)
{
    struct d_arena_region* region;
    d_mem_size             total;

    if (!_arena)
    {
        return 0;
    }

    total  = 0;
    region = _arena->head;

    while (region)
    {
        total = (d_mem_size)(total + region->used);

#if (D_INTERNAL_ARENA_CHAIN == 1)
        region = region->next;
#else
        region = NULL;
#endif
    }

    return total;
}


/*
d_arena_capacity
  Reports the total payload capacity across every region the arena holds.

Parameter(s):
  _arena: the arena to measure; may be NULL.
Return:
  The capacity in bytes, or 0 when _arena is NULL. This does NOT include the
per-region headers, which are overhead rather than capacity.
*/
d_mem_size
d_arena_capacity(
    const struct d_arena* _arena
)
{
    if (!_arena)
    {
        return 0;
    }

    return _arena->capacity;
}


/*
d_arena_remaining
  Reports how many bytes the arena could still hand out without growing.
  The answer concerns the CURRENT region only, since that is the one the
cursor is in and an earlier region's leftovers are not reachable. That makes
this an exact statement about the next allocation rather than an optimistic
one about the arena.

Parameter(s):
  _arena: the arena to measure; may be NULL.
Return:
  The bytes left in the current region, ignoring any alignment padding a
future request may need, or 0 when _arena is NULL or has no region.
*/
d_mem_size
d_arena_remaining(
    const struct d_arena* _arena
)
{
    struct d_arena_region* region;

    if (!_arena)
    {
        return 0;
    }

    region = D_INTERNAL_ARENA_CURRENT(_arena);

    if (!region)
    {
        return 0;
    }

    return (d_mem_size)(region->capacity - region->used);
}


/*
d_arena_region_count
  Reports how many regions the arena holds.

Parameter(s):
  _arena: the arena to measure; may be NULL.
Return:
  The region count, or 0 when _arena is NULL. An unchained arena reports 0 or
1, which is the honest answer rather than a special case.
*/
d_mem_size
d_arena_region_count(
    const struct d_arena* _arena
)
{
    if (!_arena)
    {
        return 0;
    }

#if (D_INTERNAL_ARENA_CHAIN == 1)
    return _arena->region_count;
#else
    return (_arena->head)
        ? (d_mem_size)1
        : (d_mem_size)0;
#endif
}


/*
d_arena_stats
  Copies out an arena's accounting block.

Parameter(s):
  _arena: the arena to read; must not be NULL.
  _out:   receives a copy of the block; must not be NULL.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when either pointer is NULL, or
D_MEM_ERR_UNSUPPORTED when this build carries no accounting -- in which case
_out is zeroed rather than left undefined, so a caller may print it either
way.
*/
enum d_mem_status
d_arena_stats(
    const struct d_arena* _arena,
    struct d_mem_stats*   _out
)
{
    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_arena != NULL, D_MEM_ERR_NULL);

#if (D_INTERNAL_MEM_STATS == 1)
    *_out = _arena->stats;

    return D_MEM_OK;
#else
    d_mem_stats_clear(_out);

    return D_MEM_ERR_UNSUPPORTED;
#endif
}


#if (D_INTERNAL_ARENA_OWNS == 1)

/*
d_arena_owns
  Reports whether a pointer names memory this arena vended.
  O(regions), so this is a diagnostic and an assertion aid rather than
something a hot path should call. It is exact, not heuristic: a pointer into a
region's payload counts, and a pointer into a region's header does not.

Parameter(s):
  _arena: the arena to search; may be NULL.
  _ptr:   the pointer to locate; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if _ptr lies within the payload of some region of this arena, or
  - false, otherwise.
*/
bool
d_arena_owns(
    const struct d_arena* _arena,
    const void*           _ptr
)
{
    struct d_arena_region* region;
    struct d_mem_block     block;

    if ( (!_arena) || (!_ptr) )
    {
        return false;
    }

    region = _arena->head;

    while (region)
    {
        block = d_mem_block_make(
                    (void*)d_internal_arena_payload(region, _arena->align),
                    region->capacity);

        if (d_mem_block_contains(&block, _ptr))
        {
            return true;
        }

#if (D_INTERNAL_ARENA_CHAIN == 1)
        region = region->next;
#else
        region = NULL;
#endif
    }

    return false;
}

#endif  // D_INTERNAL_ARENA_OWNS


///////////////////////////////////////////////////////////////////////////////
///                      VI.   COMPOSITION                                  ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_ARENA_AS_SOURCE == 1)

/*
d_internal_arena_source_allocate
  The arena-as-source allocate slot.

Parameter(s):
  _context: the struct d_arena; must not be NULL.
  _bytes:   the request size.
  _align:   the required alignment.
Return:
  A pointer into the arena, or NULL.
*/
static void*
d_internal_arena_source_allocate(
    void*      _context,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    if (!_context)
    {
        return NULL;
    }

    return d_arena_allocate((struct d_arena*)_context, _bytes, _align);
}


/*
d_internal_arena_source_release
  The arena-as-source release slot.
  An arena cannot free an individual block, so this is a no-op -- EXCEPT for
the most recent allocation when last-allocation tracking is compiled, where
popping is exact and free. That covers the case an allocator drawing from an
arena actually hits: taking a region, finding it unusable, handing it back.

Parameter(s):
  _context: the struct d_arena; may be NULL.
  _ptr:     the block; may be NULL.
  _bytes:   the size it was allocated with.
  _align:   unused.
Return:
  none.
*/
static void
d_internal_arena_source_release(
    void*      _context,
    void*      _ptr,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    D_MEM_UNUSED(_align);

#if (D_INTERNAL_ARENA_LAST_ALLOC == 1)
    if ( (_context) && (_ptr) )
    {
        (void)d_arena_pop_last((struct d_arena*)_context, _ptr, _bytes);
    }
#else
    D_MEM_UNUSED(_context);
    D_MEM_UNUSED(_ptr);
    D_MEM_UNUSED(_bytes);
#endif

    return;
}


#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)

/*
d_internal_arena_source_max_size
  The arena-as-source max_size slot.

Parameter(s):
  _context: the struct d_arena; may be NULL.
Return:
  An UPPER BOUND on the next satisfiable request: what is left in the current
region for an arena that cannot grow, and the remaining budget (or the
upstream's own ceiling) for one that can. Upper bounds are what a growth
policy wants -- it is deciding how large a step to attempt, and an exact
answer would cost a walk.
*/
static d_mem_size
d_internal_arena_source_max_size(
    const void* _context
)
{
    const struct d_arena* arena;

    if (!_context)
    {
        return 0;
    }

    arena = (const struct d_arena*)_context;

    if (!d_internal_arena_can_grow(arena))
    {
        return d_arena_remaining(arena);
    }

    if (arena->max_bytes != 0)
    {
        if (arena->capacity >= arena->max_bytes)
        {
            return d_arena_remaining(arena);
        }

        return (d_mem_size)(arena->max_bytes - arena->capacity);
    }

    return d_mem_source_max_size(&arena->source);
}

#endif  // D_INTERNAL_MEM_SOURCE_MAX_SIZE


// d_internal_arena_source_vtable
//   constant: the operation table that presents an arena as a byte source.
static const struct d_mem_source_vtable d_internal_arena_source_vtable =
{
    d_internal_arena_source_allocate,
    d_internal_arena_source_release
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    , NULL
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    , d_internal_arena_source_max_size
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    , "arena"
#endif
};


/*
d_arena_as_source
  Presents an arena as a byte source, so that a pool, a slab, or another arena
can draw its backing blocks from it.
  THE COMPOSITION POINT OF THE SUBFRAMEWORK. One upstream request at startup,
an arena over it, and every other allocator carved out of that -- with no
allocator in the chain aware of the arrangement, and with the whole graph
freed by one d_arena_release.
  The arena must outlive every allocator drawing from it, and none of them may
release blocks back except through pop-last. Both follow from what an arena
is, and neither can be checked here.

Parameter(s):
  _arena: the arena to present; may be NULL, in which case the unset source is
          returned rather than one that would fault on first use.
Return:
  A source backed by the arena.
*/
struct d_mem_source
d_arena_as_source(
    struct d_arena* _arena
)
{
    struct d_mem_source source;

    source.vtable  = NULL;
    source.context = NULL;

    if (_arena)
    {
        source.vtable  = &d_internal_arena_source_vtable;
        source.context = (void*)_arena;
    }

    return source;
}

#endif  // D_INTERNAL_ARENA_AS_SOURCE
