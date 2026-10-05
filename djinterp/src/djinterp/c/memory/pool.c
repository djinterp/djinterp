/*******************************************************************************
* djinterp [c]                                                            pool.c
*
*   Implementation unit for pool.h. Carries the slot geometry, the block table
* that makes handle resolution O(1), the index-linked free list, the three
* release policies and the generational handle machinery.
*
*
* path:      /src/djinterp/c/memory/pool.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#include "../../../../inc/djinterp/c/memory/pool.h"


///////////////////////////////////////////////////////////////////////////////
///                      I.   INTERNAL GEOMETRY                             ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_POOL_LINK_SIZE / D_INTERNAL_POOL_LINK_ALIGN
//   macro (internal): the size and alignment of the link a free slot holds.
// An index link is D_CFG_POOL_INDEX_BITS wide; a pointer link is a pointer.
// This IS the pool's minimum slot size, which is why the choice is a knob.
#if (D_INTERNAL_POOL_INDEXED_LINKS == 1)
#   define D_INTERNAL_POOL_LINK_SIZE   ((d_mem_size)sizeof(d_pool_index))
#   define D_INTERNAL_POOL_LINK_ALIGN  ((d_mem_size)sizeof(d_pool_index))
#else
#   define D_INTERNAL_POOL_LINK_SIZE   D_MEM_LINK_SIZE
#   define D_INTERNAL_POOL_LINK_ALIGN  D_MEM_LINK_ALIGN
#endif

// D_INTERNAL_POOL_GEN_ALIGN
//   macro (internal): the alignment the generation counter needs.
#define D_INTERNAL_POOL_GEN_ALIGN                                             \
    ((d_mem_size)sizeof(d_pool_generation))

// D_INTERNAL_POOL_LEAD
//   macro (internal): the bytes between a slot's first byte and the byte the
// CALLER sees -- the leading guard band, or zero when redzones are off.
//   Everything internal to this file addresses slots by their BASE, and every
// pointer crossing the API boundary is a PAYLOAD pointer, base + lead. Keeping
// that distinction explicit is not pedantry: conflating the two wrote the
// leading guard band on top of the preceding block's header, which is a
// corruption that appears as a crash in an unrelated allocation later.
#define D_INTERNAL_POOL_LEAD(_pool)     d_mem_redzone_lead((_pool)->align)

// D_INTERNAL_POOL_PAYLOAD_OF
//   macro (internal): the caller-visible pointer for a slot base.
#define D_INTERNAL_POOL_PAYLOAD_OF(_pool, _base)                              \
    ((void*)((char*)(_base) + (size_t)D_INTERNAL_POOL_LEAD(_pool)))

// D_INTERNAL_POOL_BASE_OF
//   macro (internal): the slot base for a caller-visible pointer.
#define D_INTERNAL_POOL_BASE_OF(_pool, _payload)                              \
    ((void*)((char*)(_payload) - (size_t)D_INTERNAL_POOL_LEAD(_pool)))


/*
d_pool_align_for
  Computes the alignment a pool's slots will actually carry, given a requested
alignment and a release policy.
  Exposed rather than internal because a caller sizing a buffer for a pool --
or a C++ wrapper asserting that it adds nothing -- needs the answer before any
pool exists.

Parameter(s):
  _slot_align: the requested alignment; 0 means the configured default.
  _policy:     the release policy the pool will use.
Return:
  The slot alignment, or 0 when the request is inadmissible. Never weaker than
requested; stricter only where the free list's link demands it.
*/
d_mem_size
d_pool_align_for(
    d_mem_size          _slot_align,
    enum d_pool_policy _policy
)
{
    d_mem_size link_align;

    // a monotonic pool never stores a link, so its slots answer only to the
    // caller's alignment -- which is how it reaches the smallest slot of the
    // three policies
    link_align = (_policy == D_POOL_POLICY_MONOTONIC)
        ? (d_mem_size)0
        : D_INTERNAL_POOL_LINK_ALIGN;

    return d_mem_slot_align(_slot_align, link_align);
}


/*
d_pool_stride_for
  Computes the byte stride between consecutive slots, given a slot size, an
alignment and a release policy.
  The stride is the slot size the pool ACTUALLY uses, and it is what a caller
must multiply by to size a backing buffer. Like d_pool_align_for, it is
exposed because the answer is needed before a pool exists.

Parameter(s):
  _slot_size:  the caller-visible bytes per slot; must be non-zero.
  _slot_align: the requested alignment; 0 means the configured default.
  _policy:     the release policy the pool will use.
Return:
  The stride in bytes, or 0 when the arguments are inadmissible or the
computation would overflow.
*/
d_mem_size
d_pool_stride_for(
    d_mem_size          _slot_size,
    d_mem_size          _slot_align,
    enum d_pool_policy _policy
)
{
    d_mem_size align;
    d_mem_size link_size;
    d_mem_size stride;
    d_mem_size gen_offset;

    align = d_pool_align_for(_slot_align, _policy);

    if (align == 0)
    {
        return 0;
    }

    link_size = (_policy == D_POOL_POLICY_MONOTONIC)
        ? (d_mem_size)0
        : D_INTERNAL_POOL_LINK_SIZE;

    stride = d_mem_slot_size(_slot_size, align, link_size);

    if (stride == 0)
    {
        return 0;
    }

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    // a generational slot carries its counter AFTER the payload rather than
    // before it. Before it, the counter would have to be padded out to the
    // slot alignment to keep the payload aligned -- sixteen wasted bytes on a
    // 16-aligned slot for a two-byte counter. After it, the counter lands in
    // padding the stride was going to contain anyway whenever the payload is
    // not already a multiple of the alignment.
    if (_policy == D_POOL_POLICY_GENERATIONAL)
    {
        gen_offset = d_mem_align_up(stride, D_INTERNAL_POOL_GEN_ALIGN);

        if (d_mem_add_would_overflow(gen_offset,
                                     (d_mem_size)sizeof(d_pool_generation)))
        {
            return 0;
        }

        stride = (d_mem_size)(gen_offset + sizeof(d_pool_generation));

        if (d_mem_align_would_overflow(stride, align))
        {
            return 0;
        }

        stride = d_mem_align_up(stride, align);
    }
#else
    D_MEM_UNUSED(gen_offset);
#endif

    return stride;
}


#if (D_INTERNAL_POOL_GENERATIONAL == 1)

/*
d_internal_pool_generation_offset
  Computes where inside a slot the generation counter sits.

Parameter(s):
  _payload_stride: the stride the slot would have WITHOUT a generation, as
                   returned by d_mem_slot_size.
Return:
  The offset of the counter from the start of the slot.
*/
static d_mem_size
d_internal_pool_generation_offset(
    d_mem_size _payload_stride
)
{
    return d_mem_align_up(_payload_stride, D_INTERNAL_POOL_GEN_ALIGN);
}


/*
d_internal_pool_generation_at
  Locates the generation counter belonging to a slot.

Parameter(s):
  _pool: the pool; must not be NULL and must be generational.
  _slot: the slot's first byte; must not be NULL.
Return:
  A pointer to the slot's counter.
*/
static d_pool_generation*
d_internal_pool_generation_at(
    const struct d_pool* _pool,
    void*                _slot
)
{
    return (d_pool_generation*)(void*)
           ((char*)_slot + (size_t)_pool->generation_offset);
}

#endif  // D_INTERNAL_POOL_GENERATIONAL


/*
d_internal_pool_block_bytes
  Computes the total size of the upstream block backing one block of slots.

Parameter(s):
  _slots:  the slots the block will hold.
  _stride: the byte stride between slots.
  _align:  the slot alignment.
Return:
  The block size in bytes, or 0 if the computation would overflow.
*/
static d_mem_size
d_internal_pool_block_bytes(
    d_pool_index _slots,
    d_mem_size   _stride,
    d_mem_size   _align
)
{
    d_mem_size header;
    d_mem_size body;

    header = d_mem_align_up((d_mem_size)sizeof(struct d_pool_block), _align);

    if (d_mem_mul_would_overflow((d_mem_size)_slots, _stride))
    {
        return 0;
    }

    body = (d_mem_size)((d_mem_size)_slots * _stride);

    if (d_mem_add_would_overflow(header, body))
    {
        return 0;
    }

    return (d_mem_size)(header + body);
}


/*
d_internal_pool_block_slots
  Computes the address of a block's first slot.

Parameter(s):
  _block: the block header; must not be NULL.
  _align: the slot alignment.
Return:
  A pointer to the first slot.
*/
static char*
d_internal_pool_block_slots(
    struct d_pool_block* _block,
    d_mem_size           _align
)
{
    return ( (char*)_block +
             (size_t)d_mem_align_up((d_mem_size)sizeof(struct d_pool_block),
                                    _align) );
}


///////////////////////////////////////////////////////////////////////////////
///                     II.   BLOCK TABLE                                   ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)

/*
d_internal_pool_table_grow
  Makes room in the block table for one more entry, doubling when full.
  The table is what turns "which block holds slot n" into a division and an
array index. Without it the answer costs a walk of the block chain, which is
fine for a diagnostic and useless for handle resolution -- which is why the
generational policy forces this on.

Parameter(s):
  _pool: the pool; must not be NULL.
Return:
  D_MEM_OK on success, or D_MEM_ERR_EXHAUSTED when the source refused.
*/
static enum d_mem_status
d_internal_pool_table_grow(
    struct d_pool* _pool
)
{
    d_mem_size            wanted;
    d_mem_size            old_bytes;
    d_mem_size            new_bytes;
    struct d_pool_block** table;

    if (_pool->block_count < _pool->block_capacity)
    {
        return D_MEM_OK;
    }

    wanted = (_pool->block_capacity == 0)
        ? (d_mem_size)8
        : (d_mem_size)(_pool->block_capacity * 2);

    if (d_mem_mul_would_overflow(wanted,
                                 (d_mem_size)sizeof(struct d_pool_block*)))
    {
        return D_MEM_ERR_OVERFLOW;
    }

    old_bytes = (d_mem_size)(_pool->block_capacity *
                             sizeof(struct d_pool_block*));
    new_bytes = (d_mem_size)(wanted * sizeof(struct d_pool_block*));

    table = (struct d_pool_block**)d_mem_source_reallocate(
                &_pool->source,
                (void*)_pool->blocks,
                old_bytes,
                new_bytes,
                (d_mem_size)sizeof(struct d_pool_block*));

    if (!table)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    _pool->blocks         = table;
    _pool->block_capacity = wanted;

    return D_MEM_OK;
}

#endif  // D_INTERNAL_POOL_BLOCK_TABLE


/*
d_internal_pool_block_for
  Finds the block holding a given slot index.
  With the block table this is a division and an array index. Without it, it
is a walk of the chain -- correct either way, which is what lets the table be
optional for pools that never resolve a handle.

Parameter(s):
  _pool:  the pool; must not be NULL.
  _index: the slot index.
Return:
  The block holding that slot, or NULL when the index names no slot.
*/
static struct d_pool_block*
d_internal_pool_block_for(
    const struct d_pool* _pool,
    d_pool_index         _index
)
{
#if (D_INTERNAL_POOL_BLOCK_TABLE == 0)
    struct d_pool_block* block;
#endif
    d_mem_size           number;

    if (_index >= _pool->capacity)
    {
        return NULL;
    }

    number = (d_mem_size)((d_mem_size)_index /
                          (d_mem_size)_pool->slots_per_block);

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    if ( (_pool->blocks) && (number < _pool->block_count) )
    {
        return _pool->blocks[number];
    }

    return NULL;
#else
    block = _pool->head;

    while ( (block) && (number > 0) )
    {
        block = block->next;
        --number;
    }

    return block;
#endif
}


/*
d_internal_pool_slot_at
  Computes the address of a slot from its index.

Parameter(s):
  _pool:  the pool; must not be NULL.
  _index: the slot index.
Return:
  A pointer to the slot's first byte, or NULL when the index names no slot.
*/
static void*
d_internal_pool_slot_at(
    const struct d_pool* _pool,
    d_pool_index         _index
)
{
    struct d_pool_block* block;
    d_mem_size           offset;

    block = d_internal_pool_block_for(_pool, _index);

    if (!block)
    {
        return NULL;
    }

    offset = (d_mem_size)((d_mem_size)_index -
                          (d_mem_size)block->first_index);

    return (void*)(d_internal_pool_block_slots(block, _pool->align) +
                   (size_t)(offset * _pool->stride));
}


///////////////////////////////////////////////////////////////////////////////
///                     III.   FREE LIST                                    ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_POOL_FREE_LIST == 1)

/*
d_internal_pool_link_read
  Reads the free-list link stored inside a free slot.
  The link lives in the slot's own payload storage, which is why a pool's
minimum slot size is the link's size: a free slot has no other use for those
bytes, so the list costs nothing that was not already there.

Parameter(s):
  _pool: the pool; must not be NULL.
  _slot: a free slot; must not be NULL.
Return:
  The index of the next free slot, or D_POOL_INDEX_NONE at the end.
*/
static d_pool_index
d_internal_pool_link_read(
    const struct d_pool* _pool,
    void*                _slot
)
{
#if (D_INTERNAL_POOL_INDEXED_LINKS == 1)
    d_pool_index next;

    D_MEM_UNUSED(_pool);

    // read through memcpy rather than a cast: the slot is raw storage whose
    // effective type changes with every acquire, and a cast-and-dereference
    // would be an aliasing violation the optimiser is entitled to act on
    memcpy(&next, _slot, sizeof(d_pool_index));

    return next;
#else
    void* next;

    memcpy(&next, _slot, sizeof(void*));

    if (!next)
    {
        return D_POOL_INDEX_NONE;
    }

    //   A stored pointer link is a slot BASE, because that is where the link
    // itself lives; d_pool_index_of speaks the PUBLIC convention and takes a
    // payload pointer. Converting here is what keeps the two conventions from
    // meeting: without it, a build with redzones AND pointer links reads every
    // link back as "no such slot" and silently truncates the free list to one
    // entry -- which shows up only as a pool that grows when it should have
    // recycled.
    return d_pool_index_of(_pool, D_INTERNAL_POOL_PAYLOAD_OF(_pool, next));
#endif
}


/*
d_internal_pool_link_write
  Writes the free-list link into a free slot.

Parameter(s):
  _pool: the pool; must not be NULL.
  _slot: the slot to link; must not be NULL.
  _next: the index of the next free slot, or D_POOL_INDEX_NONE.
Return:
  none.
*/
static void
d_internal_pool_link_write(
    struct d_pool* _pool,
    void*          _slot,
    d_pool_index   _next
)
{
#if (D_INTERNAL_POOL_INDEXED_LINKS == 1)
    D_MEM_UNUSED(_pool);

    memcpy(_slot, &_next, sizeof(d_pool_index));
#else
    void* next;

    next = (_next == D_POOL_INDEX_NONE)
        ? NULL
        : d_internal_pool_slot_at(_pool, _next);

    memcpy(_slot, &next, sizeof(void*));
#endif

    return;
}


#if (D_INTERNAL_POOL_DOUBLE_FREE_CHECK == 1)

/*
d_internal_pool_is_free
  Reports whether a slot index is already on the free list.
  O(free slots), which is why this is compiled only under the testing preset.
Its value is that a double free otherwise threads the list through itself,
after which two callers hold the same slot and the corruption surfaces
arbitrarily far from its cause.

Parameter(s):
  _pool:  the pool; must not be NULL.
  _index: the slot index to look for.
Return:
  A boolean value corresponding to either:
  - true, if the index is already on the free list, or
  - false, otherwise.
*/
static bool
d_internal_pool_is_free(
    const struct d_pool* _pool,
    d_pool_index         _index
)
{
    d_pool_index cursor;
    d_pool_index guard;
    void*        slot;

    cursor = _pool->free_head;
    guard  = 0;

    while (cursor != D_POOL_INDEX_NONE)
    {
        if (cursor == _index)
        {
            return true;
        }

        slot = d_internal_pool_slot_at(_pool, cursor);

        if (!slot)
        {
            return false;
        }

        cursor = d_internal_pool_link_read(_pool, slot);

        // a list already corrupted into a cycle must not hang the check that
        // exists to find corruption
        ++guard;

        if (guard > _pool->capacity)
        {
            return true;
        }
    }

    return false;
}

#endif  // D_INTERNAL_POOL_DOUBLE_FREE_CHECK

#endif  // D_INTERNAL_POOL_FREE_LIST


///////////////////////////////////////////////////////////////////////////////
///                        IV.   GROWTH                                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_internal_pool_grow
  Obtains one more block of slots from the upstream source and links it in.

Parameter(s):
  _pool: the pool; must not be NULL.
Return:
  D_MEM_OK on success, D_MEM_ERR_OVERFLOW when the geometry would not fit, or
D_MEM_ERR_EXHAUSTED when the slot ceiling forbids growth or the source
refused.
*/
static enum d_mem_status
d_internal_pool_grow(
    struct d_pool* _pool
)
{
    d_pool_index         slots;
    d_mem_size           block_bytes;
    void*                memory;
    struct d_pool_block* block;
#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    enum d_mem_status    status;
#endif

    slots = _pool->slots_per_block;

    // the ceiling clamps the block rather than refusing it, so a pool capped
    // at 300 slots with 256-slot blocks gets a 256 and then a 44
    if (_pool->max_slots != 0)
    {
        if (_pool->capacity >= _pool->max_slots)
        {
            return D_MEM_ERR_EXHAUSTED;
        }

        if ((d_mem_size)slots >
            (d_mem_size)(_pool->max_slots - _pool->capacity))
        {
            slots = (d_pool_index)(_pool->max_slots - _pool->capacity);
        }
    }

    // the index space is finite, and running off the end of it must be a
    // reported failure rather than a wrap into slot zero
    if ((d_mem_size)slots >
        (d_mem_size)(D_POOL_INDEX_NONE - (d_mem_size)_pool->capacity))
    {
        return D_MEM_ERR_OVERFLOW;
    }

    if (slots == 0)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    block_bytes = d_internal_pool_block_bytes(slots,
                                              _pool->stride,
                                              _pool->align);

    if (block_bytes == 0)
    {
        return D_MEM_ERR_OVERFLOW;
    }

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    // make room in the table BEFORE taking the block, so a failed table
    // growth does not leave a block that nothing points at
    status = d_internal_pool_table_grow(_pool);

    if (status != D_MEM_OK)
    {
        return status;
    }
#endif

    memory = d_mem_source_allocate(&_pool->source,
                                   block_bytes,
                                   _pool->align);

    if (!memory)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    block              = (struct d_pool_block*)memory;
    block->next        = NULL;
    block->first_index = _pool->capacity;
    block->slot_count  = slots;

    if (_pool->tail)
    {
        _pool->tail->next = block;
    }
    else
    {
        _pool->head = block;
    }

    _pool->tail = block;

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    _pool->blocks[_pool->block_count] = block;
#endif

    _pool->block_count = (d_mem_size)(_pool->block_count + 1);
    _pool->capacity    = (d_pool_index)(_pool->capacity + slots);

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    // every slot starts at generation zero, and a handle never carries zero
    // (acquire advances first), so a handle can never match a slot that has
    // not been handed out
    if (_pool->policy == (uint8_t)D_POOL_POLICY_GENERATIONAL)
    {
        d_mem_zero((void*)d_internal_pool_block_slots(block, _pool->align),
                   (d_mem_size)((d_mem_size)slots * _pool->stride));
    }
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_reserve(&_pool->stats, block_bytes);
#endif

    return D_MEM_OK;
}


///////////////////////////////////////////////////////////////////////////////
///                       V.   LIFECYCLE                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_pool_config_default
  Builds the configuration a pool of the given geometry takes when nothing
else is specified.

Parameter(s):
  _slot_size:  the caller-visible bytes per slot; must be non-zero.
  _slot_align: the alignment each slot must satisfy; 0 means the configured
               default.
Return:
  A configuration naming the free-list policy and the configured defaults
throughout. Free-list rather than monotonic because a caller who has not
thought about the question means "a pool", and a pool that never reuses a slot
would surprise them.
*/
struct d_pool_config
d_pool_config_default(
    d_mem_size _slot_size,
    d_mem_size _slot_align
)
{
    struct d_pool_config config;

    config.slot_size       = _slot_size;
    config.slot_align      = _slot_align;
    config.source          = d_mem_source_none();
    config.slots_per_block = 0;
    config.initial_slots   = 0;
    config.max_slots       = 0;
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    config.policy         = D_POOL_POLICY_FREE_LIST;
#else
    config.policy         = D_POOL_POLICY_MONOTONIC;
#endif

    return config;
}


/*
d_pool_init
  Prepares a pool.
  NO MEMORY IS OBTAINED unless the configuration asks for initial slots, so a
pool that is created and never used costs its struct and nothing else.

Parameter(s):
  _pool:   the pool to prepare; must not be NULL.
  _config: the geometry and policy; must not be NULL, since a pool with no
           slot size is not a pool.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when a pointer argument is NULL,
D_MEM_ERR_INVALID when the geometry is inadmissible or the named release
policy was not compiled, D_MEM_ERR_OVERFLOW when the geometry does not fit, or
D_MEM_ERR_EXHAUSTED when initial slots were requested and could not be taken.
*/
enum d_mem_status
d_pool_init(
    struct d_pool*              _pool,
    const struct d_pool_config* _config
)
{
    d_mem_size          align;
    d_mem_size          stride;
    enum d_pool_policy policy;

    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_config != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_config->slot_size != 0, D_MEM_ERR_INVALID);

    policy = _config->policy;

    // a policy the build did not compile is refused rather than silently
    // exchanged for another: a caller who asked for generational checking and
    // quietly got none is worse off than one who got an error
#if (D_INTERNAL_POOL_FREE_LIST == 0)
    if (policy == D_POOL_POLICY_FREE_LIST)
    {
        return D_MEM_ERR_INVALID;
    }
#endif
#if (D_INTERNAL_POOL_GENERATIONAL == 0)
    if (policy == D_POOL_POLICY_GENERATIONAL)
    {
        return D_MEM_ERR_INVALID;
    }
#endif

    if ( (policy != D_POOL_POLICY_MONOTONIC) &&
         (policy != D_POOL_POLICY_FREE_LIST) &&
         (policy != D_POOL_POLICY_GENERATIONAL) )
    {
        return D_MEM_ERR_INVALID;
    }

    align  = d_pool_align_for(_config->slot_align, policy);
    stride = d_pool_stride_for(_config->slot_size,
                               _config->slot_align,
                               policy);

    if (align == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    if (stride == 0)
    {
        return D_MEM_ERR_OVERFLOW;
    }

    _pool->source  = d_mem_source_resolve(&_config->source);
    _pool->head    = NULL;
    _pool->tail    = NULL;
    _pool->stride  = stride;
    _pool->payload = _config->slot_size;
    _pool->align   = align;

    _pool->capacity    = 0;
    _pool->live        = 0;
    _pool->bump        = 0;
    _pool->max_slots   = _config->max_slots;
    _pool->block_count = 0;
    _pool->policy     = (uint8_t)policy;

    // every block holds the same number of slots, so that index-to-block is a
    // division. A configured block size wins; failing that, the initial
    // reservation is taken as the caller's statement of the right size
    _pool->slots_per_block = _config->slots_per_block;

    if (_pool->slots_per_block == 0)
    {
        _pool->slots_per_block = (_config->initial_slots != 0)
            ? _config->initial_slots
            : (d_pool_index)D_INTERNAL_POOL_SLOTS_PER_BLOCK;
    }

    if (_pool->slots_per_block == 0)
    {
        _pool->slots_per_block = 1;
    }

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    _pool->blocks         = NULL;
    _pool->block_capacity = 0;
#endif

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    _pool->free_head  = D_POOL_INDEX_NONE;
    _pool->free_count = 0;
#endif

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    _pool->generation_offset = d_internal_pool_generation_offset(
        d_mem_slot_size(_config->slot_size,
                        align,
                        (policy == D_POOL_POLICY_MONOTONIC)
                            ? (d_mem_size)0
                            : D_INTERNAL_POOL_LINK_SIZE));
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_clear(&_pool->stats);
#endif

    if (_config->initial_slots != 0)
    {
        return d_pool_reserve(_pool, _config->initial_slots);
    }

    return D_MEM_OK;
}


/*
d_pool_reserve
  Ensures the pool has room for at least _slots slots in total.

Parameter(s):
  _pool:  the pool; must not be NULL.
  _slots: the total capacity wanted, not the additional capacity.
Return:
  D_MEM_OK on success (including when the capacity was already sufficient), or
whatever d_internal_pool_grow reported.
*/
enum d_mem_status
d_pool_reserve(
    struct d_pool* _pool,
    d_pool_index   _slots
)
{
    enum d_mem_status status;

    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

    while (_pool->capacity < _slots)
    {
        status = d_internal_pool_grow(_pool);

        if (status != D_MEM_OK)
        {
            return status;
        }
    }

    return D_MEM_OK;
}


/*
d_pool_reset
  Reclaims every slot, keeping the blocks for reuse.
  NO DESTRUCTOR RUNS -- the pool holds raw storage, and anything with a
lifetime must have been finalized by the caller first.
  A GENERATIONAL POOL ADVANCES EVERY SLOT'S COUNTER, so that every handle
issued before the reset is stale afterwards. That is the property that makes
reset safe on a generational pool: the bulk reclaim invalidates the handles
that named the reclaimed slots, rather than leaving them pointing at whatever
lands there next.

Parameter(s):
  _pool: the pool to reset; may be NULL.
Return:
  none.
*/
void
d_pool_reset(
    struct d_pool* _pool
)
{
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    struct d_pool_block* block;
    char*                slots;
    d_pool_index         i;
    d_pool_generation*   generation;
#endif

    if (!_pool)
    {
        return;
    }

#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    if (_pool->policy == (uint8_t)D_POOL_POLICY_GENERATIONAL)
    {
        block = _pool->head;

        while (block)
        {
            slots = d_internal_pool_block_slots(block, _pool->align);

            for (i = 0; i < block->slot_count; ++i)
            {
                generation = d_internal_pool_generation_at(
                                 _pool,
                                 (void*)(slots + (size_t)((d_mem_size)i *
                                                          _pool->stride)));

                *generation = (d_pool_generation)(*generation + 1);
            }

            block = block->next;
        }
    }
#endif

    _pool->live = 0;
    _pool->bump = 0;

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    _pool->free_head  = D_POOL_INDEX_NONE;
    _pool->free_count = 0;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_reset(&_pool->stats);
#endif

    return;
}


/*
d_pool_release
  Returns every block to the upstream source and leaves the pool in its
initialized-but-empty state, ready to be used again.

Parameter(s):
  _pool: the pool to release; may be NULL.
Return:
  none.
*/
void
d_pool_release(
    struct d_pool* _pool
)
{
    struct d_pool_block* block;
    struct d_pool_block* next;
    d_mem_size           block_bytes;

    if (!_pool)
    {
        return;
    }

    block = _pool->head;

    while (block)
    {
        next        = block->next;
        block_bytes = d_internal_pool_block_bytes(block->slot_count,
                                                  _pool->stride,
                                                  _pool->align);

        d_mem_source_release(&_pool->source,
                             (void*)block,
                             block_bytes,
                             _pool->align);

        block = next;
    }

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    if (_pool->blocks)
    {
        d_mem_source_release(
            &_pool->source,
            (void*)_pool->blocks,
            (d_mem_size)(_pool->block_capacity *
                         sizeof(struct d_pool_block*)),
            (d_mem_size)sizeof(struct d_pool_block*));

        _pool->blocks         = NULL;
        _pool->block_capacity = 0;
    }
#endif

    _pool->head        = NULL;
    _pool->tail        = NULL;
    _pool->capacity    = 0;
    _pool->live        = 0;
    _pool->bump        = 0;
    _pool->block_count = 0;

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    _pool->free_head  = D_POOL_INDEX_NONE;
    _pool->free_count = 0;
#endif

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_clear(&_pool->stats);
#endif

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                  VI.   ACQUIRE AND RELEASE                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_internal_pool_take
  Takes the next slot, from the free list if one is waiting and from the bump
region otherwise, growing when neither can supply one.

Parameter(s):
  _pool:  the pool; must not be NULL.
  _index: receives the index of the slot taken; must not be NULL.
Return:
  A pointer to the slot, or NULL on failure.
*/
static void*
d_internal_pool_take(
    struct d_pool* _pool,
    d_pool_index*  _index
)
{
    void*        slot;
    d_pool_index index;

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    // (1) a recycled slot, if there is one. This is the common path in a
    // steady-state pool and costs one load and one store
    if (_pool->free_head != D_POOL_INDEX_NONE)
    {
        index = _pool->free_head;
        slot  = d_internal_pool_slot_at(_pool, index);

        if (slot)
        {
            _pool->free_head  = d_internal_pool_link_read(_pool, slot);
            _pool->free_count = (d_pool_index)(_pool->free_count - 1);
            *_index           = index;

            return slot;
        }

        // a free list pointing outside the pool is corruption; drop it rather
        // than follow it, and let the bump region answer instead
        _pool->free_head  = D_POOL_INDEX_NONE;
        _pool->free_count = 0;
    }
#endif

    // (2) a slot that has never been used
    if (_pool->bump >= _pool->capacity)
    {
        if (d_internal_pool_grow(_pool) != D_MEM_OK)
        {
            return NULL;
        }
    }

    index = _pool->bump;
    slot  = d_internal_pool_slot_at(_pool, index);

    if (!slot)
    {
        return NULL;
    }

    _pool->bump = (d_pool_index)(_pool->bump + 1);
    *_index     = index;

    return slot;
}


/*
d_pool_acquire
  Takes one slot from the pool.

Parameter(s):
  _pool: the pool; must not be NULL.
Return:
  A pointer to one slot's worth of storage, aligned as the pool was
configured, or NULL when the pool is full and could not grow.
*/
void*
d_pool_acquire(
    struct d_pool* _pool
)
{
    struct d_mem_block block;

    if (d_pool_acquire_ex(_pool, &block) != D_MEM_OK)
    {
        return NULL;
    }

    return block.ptr;
}


/*
d_pool_acquire_ex
  Takes one slot from the pool, reporting why a failure failed.

Parameter(s):
  _pool: the pool; must not be NULL.
  _out:  receives the slot on success, or the empty block on failure; must not
         be NULL.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, or D_MEM_ERR_EXHAUSTED.
*/
enum d_mem_status
d_pool_acquire_ex(
    struct d_pool*      _pool,
    struct d_mem_block* _out
)
{
    void*        base;
    void*        payload;
    d_pool_index index;

    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

    *_out = d_mem_block_empty();
    index = D_POOL_INDEX_NONE;
    base  = d_internal_pool_take(_pool, &index);

    if (!base)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    _pool->live = (d_pool_index)(_pool->live + 1);

    payload = D_INTERNAL_POOL_PAYLOAD_OF(_pool, base);

    d_mem_prepare(payload, _pool->payload);
    d_mem_redzone_write(payload, _pool->payload);

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_acquire(&_pool->stats, _pool->payload);
#endif

    *_out = d_mem_block_make(payload, _pool->payload);

    return D_MEM_OK;
}


/*
d_pool_release_slot
  Returns one slot to the pool.

  What happens depends on the policy, and the differences are the reason the
policy exists:
    MONOTONIC     nothing. The slot returns on reset, and this call reports
                  D_MEM_ERR_UNSUPPORTED so a caller cannot believe otherwise.
    FREE_LIST     the slot goes on the list and the next acquire will hand it
                  out again.
    GENERATIONAL  as free-list, and the slot's counter advances first, which
                  is what makes every handle naming it stale from here on.

Parameter(s):
  _pool: the pool; must not be NULL.
  _slot: the slot to return; may be NULL, which is a no-op reported as OK.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_UNSUPPORTED for a monotonic pool,
D_MEM_ERR_FOREIGN when the pointer names no slot of this pool, or
D_MEM_ERR_CORRUPT when a guard band was overwritten or the slot was already
free.

  ON THE ORDER OF THE CHECKS. Ownership is established BEFORE anything reads
the memory around the pointer, and that ordering is load-bearing rather than
stylistic: verifying a guard band reads the bytes on either side of the
payload, and a foreign pointer is precisely the case where those bytes are not
ours to read. Checking the band first turns a caller's mistake -- passing the
wrong pointer -- into an out-of-bounds read inside the allocator, which is a
worse bug than the one being diagnosed. The guard check still precedes
RECYCLING, which is all its own rationale ever required.
*/
enum d_mem_status
d_pool_release_slot(
    struct d_pool* _pool,
    void*          _slot
)
{
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    d_pool_index index;
    void*        base;
#endif
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    d_pool_generation* generation;
#endif

    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

    if (!_slot)
    {
        return D_MEM_OK;
    }

    // (1) nothing here reads memory, so it is safe on any pointer at all
    if (_pool->policy == (uint8_t)D_POOL_POLICY_MONOTONIC)
    {
        return D_MEM_ERR_UNSUPPORTED;
    }

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    // (2) establish that the pointer is ours BEFORE reading around it
    index = d_pool_index_of(_pool, _slot);

    if (index == D_POOL_INDEX_NONE)
    {
        return D_MEM_ERR_FOREIGN;
    }

    // (3) now the guard bands may safely be read -- and are read before the
    // slot is recycled, so a report names the block that overran rather than
    // whatever lands there next
    if (!d_mem_redzone_verify(_slot, _pool->payload))
    {
        return D_MEM_ERR_CORRUPT;
    }

#   if (D_INTERNAL_POOL_DOUBLE_FREE_CHECK == 1)
    if (d_internal_pool_is_free(_pool, index))
    {
        return D_MEM_ERR_CORRUPT;
    }
#   endif

    base = D_INTERNAL_POOL_BASE_OF(_pool, _slot);

#   if (D_INTERNAL_POOL_GENERATIONAL == 1)
    // the counter advances before the slot is recycled, so that every handle
    // issued against the old occupant fails to resolve from here on
    if (_pool->policy == (uint8_t)D_POOL_POLICY_GENERATIONAL)
    {
        generation  = d_internal_pool_generation_at(_pool, base);
        *generation = (d_pool_generation)(*generation + 1);
    }
#   endif

    d_mem_poison_free(_slot, _pool->payload);

    // the free-list link lives at the slot BASE, whose storage is guaranteed
    // wide enough for it; the payload region alone is not
    d_internal_pool_link_write(_pool, base, _pool->free_head);

    _pool->free_head  = index;
    _pool->free_count = (d_pool_index)(_pool->free_count + 1);

    if (_pool->live > 0)
    {
        _pool->live = (d_pool_index)(_pool->live - 1);
    }

#   if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_release(&_pool->stats, _pool->payload);
#   endif

    return D_MEM_OK;
#else
    return D_MEM_ERR_UNSUPPORTED;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///                        VII.   HANDLES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_POOL_GENERATIONAL == 1)

/*
d_pool_handle_null
  Builds the handle that names no slot.

Parameter(s):
  none.
Return:
  A handle that never resolves.
*/
struct d_pool_handle
d_pool_handle_null(void)
{
    struct d_pool_handle handle;

    handle.index      = D_POOL_INDEX_NONE;
    handle.generation = 0;

    return handle;
}


/*
d_pool_handle_is_null
  Reports whether a handle names no slot.

Parameter(s):
  _handle: the handle to test.
Return:
  A boolean value corresponding to either:
  - true, if the handle is the null handle, or
  - false, otherwise.
*/
bool
d_pool_handle_is_null(
    struct d_pool_handle _handle
)
{
    return (_handle.index == D_POOL_INDEX_NONE);
}


/*
d_pool_acquire_handle
  Takes one slot and returns a generation-checked handle to it.
  UNLIKE A POINTER, THE RESULT IS SAFE TO STORE. Resolving it after the slot
has been released and reused reports D_MEM_ERR_STALE rather than handing back
the new occupant, which is the entire reason the generational policy exists.

Parameter(s):
  _pool: the pool; must not be NULL and must be generational.
  _out:  receives the handle on success, or the null handle on failure; must
         not be NULL.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_UNSUPPORTED when the pool is not
generational, or D_MEM_ERR_EXHAUSTED.
*/
enum d_mem_status
d_pool_acquire_handle(
    struct d_pool*        _pool,
    struct d_pool_handle* _out
)
{
    void*              base;
    void*              payload;
    d_pool_index       index;
    d_pool_generation* generation;

    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

    *_out = d_pool_handle_null();

    if (_pool->policy != (uint8_t)D_POOL_POLICY_GENERATIONAL)
    {
        return D_MEM_ERR_UNSUPPORTED;
    }

    index = D_POOL_INDEX_NONE;
    base  = d_internal_pool_take(_pool, &index);

    if (!base)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    // the counter advances on acquire as well as on release, so that a fresh
    // slot's counter is never zero -- and a zero-initialised handle
    // therefore cannot accidentally resolve to a never-used slot
    generation  = d_internal_pool_generation_at(_pool, base);
    *generation = (d_pool_generation)(*generation + 1);

    _pool->live = (d_pool_index)(_pool->live + 1);

    payload = D_INTERNAL_POOL_PAYLOAD_OF(_pool, base);

    d_mem_prepare(payload, _pool->payload);
    d_mem_redzone_write(payload, _pool->payload);

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_acquire(&_pool->stats, _pool->payload);
#endif

    _out->index      = index;
    _out->generation = *generation;

    return D_MEM_OK;
}


/*
d_pool_resolve_ex
  Turns a handle back into a slot pointer, reporting why a failure failed.

Parameter(s):
  _pool:   the pool; must not be NULL.
  _handle: the handle to resolve.
  _out:    receives the slot on success, or NULL on failure; must not be NULL.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_UNSUPPORTED when the pool is not
generational, D_MEM_ERR_FOREIGN when the index names no slot, or
D_MEM_ERR_STALE when the slot has been released since the handle was issued.
*/
enum d_mem_status
d_pool_resolve_ex(
    const struct d_pool* _pool,
    struct d_pool_handle _handle,
    void**               _out
)
{
    void*              base;
    d_pool_generation* generation;

    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

    *_out = NULL;

    if (_pool->policy != (uint8_t)D_POOL_POLICY_GENERATIONAL)
    {
        return D_MEM_ERR_UNSUPPORTED;
    }

    if (d_pool_handle_is_null(_handle))
    {
        return D_MEM_ERR_FOREIGN;
    }

    base = d_internal_pool_slot_at(_pool, _handle.index);

    if (!base)
    {
        return D_MEM_ERR_FOREIGN;
    }

    generation = d_internal_pool_generation_at(_pool, base);

    // the whole mechanism, in one comparison: the slot remembers how many
    // times it has turned over, and the handle remembers which turn it was
    // issued on
    if (*generation != _handle.generation)
    {
        return D_MEM_ERR_STALE;
    }

    *_out = D_INTERNAL_POOL_PAYLOAD_OF(_pool, base);

    return D_MEM_OK;
}


/*
d_pool_resolve
  Turns a handle back into a slot pointer.

Parameter(s):
  _pool:   the pool; must not be NULL.
  _handle: the handle to resolve.
Return:
  A pointer to the slot, or NULL when the handle is null, foreign, or stale.
*/
void*
d_pool_resolve(
    const struct d_pool* _pool,
    struct d_pool_handle _handle
)
{
    void* slot;

    if (d_pool_resolve_ex(_pool, _handle, &slot) != D_MEM_OK)
    {
        return NULL;
    }

    return slot;
}


/*
d_pool_handle_is_live
  Reports whether a handle still names the slot it was issued against.

Parameter(s):
  _pool:   the pool; may be NULL.
  _handle: the handle to test.
Return:
  A boolean value corresponding to either:
  - true, if the handle resolves, or
  - false, if it is null, foreign, or stale.
*/
bool
d_pool_handle_is_live(
    const struct d_pool* _pool,
    struct d_pool_handle _handle
)
{
    return (d_pool_resolve(_pool, _handle) != NULL);
}


/*
d_pool_release_handle
  Returns the slot a handle names to the pool.
  Preferred over releasing by pointer on a generational pool: the handle
already carries the index, so this is O(1) where the pointer form must first
find which block the address belongs to.

Parameter(s):
  _pool:   the pool; must not be NULL.
  _handle: the handle whose slot should be returned.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_UNSUPPORTED, D_MEM_ERR_FOREIGN, or
D_MEM_ERR_STALE -- the last meaning the slot was already released, which this
detects rather than double-freeing.
*/
enum d_mem_status
d_pool_release_handle(
    struct d_pool*       _pool,
    struct d_pool_handle _handle
)
{
    enum d_mem_status  status;
    void*              payload;
    void*              base;
    d_pool_generation* generation;

    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

    status = d_pool_resolve_ex(_pool, _handle, &payload);

    if (status != D_MEM_OK)
    {
        return status;
    }

    if (!d_mem_redzone_verify(payload, _pool->payload))
    {
        return D_MEM_ERR_CORRUPT;
    }

    base        = D_INTERNAL_POOL_BASE_OF(_pool, payload);
    generation  = d_internal_pool_generation_at(_pool, base);
    *generation = (d_pool_generation)(*generation + 1);

    d_mem_poison_free(payload, _pool->payload);

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    d_internal_pool_link_write(_pool, base, _pool->free_head);

    _pool->free_head  = _handle.index;
    _pool->free_count = (d_pool_index)(_pool->free_count + 1);
#endif

    if (_pool->live > 0)
    {
        _pool->live = (d_pool_index)(_pool->live - 1);
    }

#if (D_INTERNAL_MEM_STATS == 1)
    d_mem_stats_on_release(&_pool->stats, _pool->payload);
#endif

    return D_MEM_OK;
}

#endif  // D_INTERNAL_POOL_GENERATIONAL


///////////////////////////////////////////////////////////////////////////////
///                      VIII.   ADDRESSING                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_pool_at
  Returns the slot with the given index.
  This does NOT say whether the slot is live -- the pool holds raw storage and
does not track occupancy per slot except through the generation counter. On a
generational pool, resolve a handle instead.

Parameter(s):
  _pool:  the pool; may be NULL.
  _index: the slot index.
Return:
  A pointer to the slot, or NULL when the index names no slot.
*/
void*
d_pool_at(
    const struct d_pool* _pool,
    d_pool_index         _index
)
{
    void* base;

    if (!_pool)
    {
        return NULL;
    }

    base = d_internal_pool_slot_at(_pool, _index);

    if (!base)
    {
        return NULL;
    }

    return D_INTERNAL_POOL_PAYLOAD_OF(_pool, base);
}


/*
d_pool_index_of
  Returns the index of the slot a pointer names.

  COST: O(blocks), because the block table indexes by slot number and not by
address, so the block containing an arbitrary address must be found by
scanning. That is why a generational caller should release by handle -- the
handle already carries the index -- and why this is documented rather than
hidden.

Parameter(s):
  _pool: the pool; may be NULL.
  _slot: the address to locate; may be NULL.
Return:
  The slot's index, or D_POOL_INDEX_NONE when the address names no slot of
this pool. An address INSIDE a slot but not at its start also yields NONE:
partial-slot pointers are a caller error, not a rounding case.
*/
d_pool_index
d_pool_index_of(
    const struct d_pool* _pool,
    const void*          _slot
)
{
    struct d_pool_block* block;
    const char*          slots;
    const char*          target;
    d_mem_size           span;
    d_mem_size           offset;

    if ( (!_pool) || (!_slot) )
    {
        return D_POOL_INDEX_NONE;
    }

    // the caller holds a PAYLOAD pointer; slot arithmetic is done on bases
    target = (const char*)_slot - (size_t)D_INTERNAL_POOL_LEAD(_pool);
    block  = _pool->head;

    while (block)
    {
        slots = d_internal_pool_block_slots(block, _pool->align);
        span  = (d_mem_size)((d_mem_size)block->slot_count * _pool->stride);

        if ( ((uintptr_t)target >= (uintptr_t)slots) &&
             ((uintptr_t)target <  (uintptr_t)(slots + (size_t)span)) )
        {
            offset = (d_mem_size)(target - slots);

            // an address partway into a slot is a caller error rather than a
            // slot to round to, and is reported as naming no slot
            if ((offset % _pool->stride) != 0)
            {
                return D_POOL_INDEX_NONE;
            }

            return (d_pool_index)((d_mem_size)block->first_index +
                                  (offset / _pool->stride));
        }

        block = block->next;
    }

    return D_POOL_INDEX_NONE;
}


///////////////////////////////////////////////////////////////////////////////
///                       IX.   ITERATION                                   ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_POOL_ITERATE == 1)

/*
d_pool_first
  Opens a walk over every slot the pool has ever handed out, in index order.

  WHAT THE WALK CANNOT TELL YOU: a pool holds raw storage and does not record
which slots are live, so the walk visits released slots too. A caller that
needs live-only iteration should either keep its own occupancy set or use a
generational pool and check each slot's handle. Saying so here is better than
a walk that quietly lies.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  A cursor at the first slot, or an exhausted cursor when the pool is empty.
*/
struct d_pool_cursor
d_pool_first(
    const struct d_pool* _pool
)
{
    struct d_pool_cursor cursor;

    cursor.block  = NULL;
    cursor.index  = 0;
    cursor.offset = 0;

    if ( (_pool) && (_pool->bump > 0) )
    {
        cursor.block = _pool->head;
    }

    return cursor;
}


/*
d_pool_cursor_valid
  Reports whether a cursor still names a slot.

Parameter(s):
  _pool:   the pool; may be NULL.
  _cursor: the cursor; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the cursor names a slot that has been handed out, or
  - false, otherwise.
*/
bool
d_pool_cursor_valid(
    const struct d_pool*        _pool,
    const struct d_pool_cursor* _cursor
)
{
    if ( (!_pool) || (!_cursor) || (!_cursor->block) )
    {
        return false;
    }

    return (_cursor->index < _pool->bump);
}


/*
d_pool_cursor_slot
  Returns the slot a cursor names.

Parameter(s):
  _pool:   the pool; may be NULL.
  _cursor: the cursor; may be NULL.
Return:
  A pointer to the slot, or NULL when the cursor is exhausted.
*/
void*
d_pool_cursor_slot(
    const struct d_pool*        _pool,
    const struct d_pool_cursor* _cursor
)
{
    if (!d_pool_cursor_valid(_pool, _cursor))
    {
        return NULL;
    }

    return D_INTERNAL_POOL_PAYLOAD_OF(
               _pool,
               d_internal_pool_block_slots(_cursor->block, _pool->align) +
                   (size_t)((d_mem_size)_cursor->offset * _pool->stride));
}


/*
d_pool_next
  Advances a cursor to the following slot.

Parameter(s):
  _pool:   the pool; may be NULL.
  _cursor: the cursor to advance; may be NULL.
Return:
  none.
*/
void
d_pool_next(
    const struct d_pool*  _pool,
    struct d_pool_cursor* _cursor
)
{
    if ( (!_pool) || (!_cursor) || (!_cursor->block) )
    {
        return;
    }

    _cursor->index  = (d_pool_index)(_cursor->index + 1);
    _cursor->offset = (d_pool_index)(_cursor->offset + 1);

    // crossing a block boundary is the only thing this has to get right, and
    // it is a comparison rather than a division because the cursor already
    // knows where it is
    if (_cursor->offset >= _cursor->block->slot_count)
    {
        _cursor->block  = _cursor->block->next;
        _cursor->offset = 0;
    }

    return;
}

#endif  // D_INTERNAL_POOL_ITERATE


///////////////////////////////////////////////////////////////////////////////
///                        X.   QUERIES                                     ///
///////////////////////////////////////////////////////////////////////////////

/*
d_pool_is_valid
  Reports whether a pool has been initialized.

Parameter(s):
  _pool: the pool to test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the pool has a slot geometry, or
  - false, otherwise.
*/
bool
d_pool_is_valid(
    const struct d_pool* _pool
)
{
    if (!_pool)
    {
        return false;
    }

    return ( (_pool->stride != 0) && (_pool->payload != 0) );
}


/*
d_pool_size
  Reports how many slots are currently acquired.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The live slot count, or 0 when _pool is NULL.
*/
d_pool_index
d_pool_size(
    const struct d_pool* _pool
)
{
    if (!_pool)
    {
        return 0;
    }

    return _pool->live;
}


/*
d_pool_capacity
  Reports how many slots exist.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The total slot count across every block, or 0 when _pool is NULL.
*/
d_pool_index
d_pool_capacity(
    const struct d_pool* _pool
)
{
    if (!_pool)
    {
        return 0;
    }

    return _pool->capacity;
}


/*
d_pool_available
  Reports how many slots could be handed out without growing.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The free-list length plus the unused bump region, or 0 when _pool is NULL.
*/
d_pool_index
d_pool_available(
    const struct d_pool* _pool
)
{
    d_mem_size available;

    if (!_pool)
    {
        return 0;
    }

    available = (d_mem_size)((d_mem_size)_pool->capacity -
                             (d_mem_size)_pool->bump);

#if (D_INTERNAL_POOL_FREE_LIST == 1)
    available = (d_mem_size)(available + (d_mem_size)_pool->free_count);
#endif

    return (d_pool_index)available;
}


/*
d_pool_slot_size
  Reports the caller-visible bytes in one slot.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The payload size, or 0 when _pool is NULL. NOTE that this is what the caller
asked for, not the stride -- use d_pool_bytes_allocated to learn what the pool
actually costs.
*/
d_mem_size
d_pool_slot_size(
    const struct d_pool* _pool
)
{
    if (!_pool)
    {
        return 0;
    }

    return _pool->payload;
}


/*
d_pool_slot_align
  Reports the alignment every slot satisfies.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The slot alignment, or 0 when _pool is NULL. May be stricter than what was
requested, where the free list's link demanded more; never weaker.
*/
d_mem_size
d_pool_slot_align(
    const struct d_pool* _pool
)
{
    if (!_pool)
    {
        return 0;
    }

    return _pool->align;
}


/*
d_pool_block_count
  Reports how many blocks the pool holds.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The block count, or 0 when _pool is NULL.
*/
d_mem_size
d_pool_block_count(
    const struct d_pool* _pool
)
{
    if (!_pool)
    {
        return 0;
    }

    return _pool->block_count;
}


/*
d_pool_bytes_allocated
  Reports the total bytes the pool holds from its upstream source, including
per-block headers and the block table.
  This is the honest cost of the pool, as distinct from capacity times slot
size -- which omits exactly the overhead a caller weighing the pool wants to
see.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  The byte total, or 0 when _pool is NULL.
*/
d_mem_size
d_pool_bytes_allocated(
    const struct d_pool* _pool
)
{
    struct d_pool_block* block;
    d_mem_size           total;

    if (!_pool)
    {
        return 0;
    }

    total = 0;
    block = _pool->head;

    while (block)
    {
        total = (d_mem_size)(total +
                             d_internal_pool_block_bytes(block->slot_count,
                                                         _pool->stride,
                                                         _pool->align));
        block = block->next;
    }

#if (D_INTERNAL_POOL_BLOCK_TABLE == 1)
    total = (d_mem_size)(total +
                         (_pool->block_capacity *
                          sizeof(struct d_pool_block*)));
#endif

    return total;
}


/*
d_pool_utilization
  Reports the fraction of the pool's slots that are currently acquired.

Parameter(s):
  _pool: the pool; may be NULL.
Return:
  A value in [0.0, 1.0], or 0.0 when _pool is NULL or has no slots yet.
*/
double
d_pool_utilization(
    const struct d_pool* _pool
)
{
    if ( (!_pool) || (_pool->capacity == 0) )
    {
        return 0.0;
    }

    return ((double)_pool->live / (double)_pool->capacity);
}


/*
d_pool_stats
  Copies out a pool's accounting block.

Parameter(s):
  _pool: the pool to read; must not be NULL.
  _out:  receives a copy of the block; must not be NULL.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when either pointer is NULL, or
D_MEM_ERR_UNSUPPORTED when this build carries no accounting -- in which case
_out is zeroed rather than left undefined.
*/
enum d_mem_status
d_pool_stats(
    const struct d_pool* _pool,
    struct d_mem_stats*  _out
)
{
    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_pool != NULL, D_MEM_ERR_NULL);

#if (D_INTERNAL_MEM_STATS == 1)
    *_out = _pool->stats;

    return D_MEM_OK;
#else
    d_mem_stats_clear(_out);

    return D_MEM_ERR_UNSUPPORTED;
#endif
}


#if (D_INTERNAL_POOL_OWNS == 1)

/*
d_pool_owns
  Reports whether a pointer names a slot of this pool.

Parameter(s):
  _pool: the pool; may be NULL.
  _slot: the address to locate; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if _slot is the first byte of a slot of this pool, or
  - false, otherwise.
*/
bool
d_pool_owns(
    const struct d_pool* _pool,
    const void*          _slot
)
{
    return (d_pool_index_of(_pool, _slot) != D_POOL_INDEX_NONE);
}

#endif  // D_INTERNAL_POOL_OWNS
