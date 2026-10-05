/*******************************************************************************
* djinterp [c]                                                      mem_source.c
*
*   Implementation unit for mem_source.h. Carries the four built-in sources --
* system, null, buffer and counting -- their operation tables, and the generic
* protocol operations every allocator calls through, including the
* allocate/copy/release emulation that makes the reallocate slot optional.
*
*
* path:      /src/djinterp/c/memory/mem_source.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#include "../../../../inc/djinterp/c/memory/mem_source.h"

#if ( (D_INTERNAL_MEM_SOURCE_SYSTEM == 1) ||                                  \
      (D_INTERNAL_MEM_SOURCE_REALLOC == 1) )
#   include <stdlib.h>
#endif
#if ( (D_INTERNAL_MEM_SOURCE_SYSTEM == 1) &&                                  \
      (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_WIN32) )
#   include <malloc.h>
#endif


///////////////////////////////////////////////////////////////////////////////
///                    I.   THE SYSTEM SOURCE                               ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_SOURCE_SYSTEM == 1)

/*
d_internal_mem_system_needs_aligned_path
  Reports whether a request needs more than malloc guarantees.
  This is the branch that keeps the common case free: malloc already returns
memory suitable for any fundamental type, so a request at or below that
alignment takes plain malloc and pays none of the over-alignment machinery --
no size rounding, no stored raw pointer, no second free routine.

Parameter(s):
  _align: the normalized alignment.
Return:
  A boolean value corresponding to either:
  - true, if _align exceeds the platform's fundamental alignment, or
  - false, otherwise.
*/
static bool
d_internal_mem_system_needs_aligned_path(
    d_mem_size _align
)
{
    return (_align > D_MEM_MAX_ALIGN);
}


#if (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_MANUAL)

/*
d_internal_mem_manual_aligned_alloc
  Satisfies an over-aligned request on a platform with no aligned allocator.
  Over-allocates by (align - 1) plus one pointer, aligns the payload inside
that range, and stores the raw allocation immediately before the payload so
that release can find it again.

Parameter(s):
  _bytes: the payload size; must be non-zero.
  _align: the required alignment; must be a power of two.
Return:
  A pointer to _bytes aligned bytes, or NULL on failure or overflow.
*/
static void*
d_internal_mem_manual_aligned_alloc(
    d_mem_size _bytes,
    d_mem_size _align
)
{
    d_mem_size overhead;
    d_mem_size total;
    void*      raw;
    char*      payload;

    overhead = 0;
    total    = 0;
    raw      = NULL;
    payload  = NULL;

    // the payload may start anywhere in the first _align bytes, and one
    // pointer must fit behind wherever it starts
    overhead = (d_mem_size)(_align + (d_mem_size)sizeof(void*));

    if (d_mem_add_would_overflow(_bytes, overhead))
    {
        return NULL;
    }

    total = (d_mem_size)(_bytes + overhead);
    raw   = malloc((size_t)total);

    if (!raw)
    {
        return NULL;
    }

    // align forward from the first byte that leaves room for the back-pointer
    payload = (char*)raw + sizeof(void*);
    payload = payload +
              (size_t)d_mem_align_padding((d_mem_size)(uintptr_t)payload,
                                          _align);

    ((void**)payload)[-1] = raw;

    return (void*)payload;
}


/*
d_internal_mem_manual_aligned_free
  Releases a block produced by d_internal_mem_manual_aligned_alloc.

Parameter(s):
  _ptr: the payload pointer; must not be NULL.
Return:
  none.
*/
static void
d_internal_mem_manual_aligned_free(
    void* _ptr
)
{
    free(((void**)_ptr)[-1]);

    return;
}

#endif  // D_CFG_MEM_SOURCE_ALIGN_MANUAL


/*
d_internal_mem_system_allocate
  The system source's allocate slot: malloc for ordinary alignments, and the
configured over-alignment path above that.

Parameter(s):
  _context: unused; the system source is stateless.
  _bytes:   the request size.
  _align:   the required alignment, already normalized by the caller.
Return:
  A pointer to _bytes bytes aligned to _align, or NULL.
*/
static void*
d_internal_mem_system_allocate(
    void*      _context,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    D_MEM_UNUSED(_context);

    if (_bytes == 0)
    {
        return NULL;
    }

    // the fast path: malloc already meets any fundamental alignment
    if (!d_internal_mem_system_needs_aligned_path(_align))
    {
        return malloc((size_t)_bytes);
    }

#if (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_C11)
    {
        d_mem_size rounded;

        // C11 aligned_alloc requires the size to be a multiple of the
        // alignment; rounding up is the caller's job, not the library's
        if (d_mem_align_would_overflow(_bytes, _align))
        {
            return NULL;
        }

        rounded = d_mem_align_up(_bytes, _align);

        return aligned_alloc((size_t)_align, (size_t)rounded);
    }
#elif (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_POSIX)
    {
        void*      result;
        d_mem_size align;

        result = NULL;

        // posix_memalign additionally requires a multiple of sizeof(void*)
        align = _align;

        if (align < (d_mem_size)sizeof(void*))
        {
            align = (d_mem_size)sizeof(void*);
        }

        if (posix_memalign(&result, (size_t)align, (size_t)_bytes) != 0)
        {
            return NULL;
        }

        return result;
    }
#elif (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_WIN32)
    return _aligned_malloc((size_t)_bytes, (size_t)_align);
#else
    return d_internal_mem_manual_aligned_alloc(_bytes, _align);
#endif
}


/*
d_internal_mem_system_release
  The system source's release slot. Chooses the same branch allocate did,
which is why the contract requires the caller to hand back the alignment it
was given.

Parameter(s):
  _context: unused.
  _ptr:     the block to release; may be NULL.
  _bytes:   the size it was allocated with; unused by this source.
  _align:   the alignment it was allocated with.
Return:
  none.
*/
static void
d_internal_mem_system_release(
    void*      _context,
    void*      _ptr,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    D_MEM_UNUSED(_context);
    D_MEM_UNUSED(_bytes);

    if (!_ptr)
    {
        return;
    }

    if (!d_internal_mem_system_needs_aligned_path(_align))
    {
        free(_ptr);

        return;
    }

#if (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_WIN32)
    _aligned_free(_ptr);
#elif (D_INTERNAL_MEM_SOURCE_ALIGNED_API == D_CFG_MEM_SOURCE_ALIGN_MANUAL)
    d_internal_mem_manual_aligned_free(_ptr);
#else
    free(_ptr);
#endif

    return;
}


#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)

/*
d_internal_mem_system_reallocate
  The system source's reallocate slot. Only the malloc path can grow in place;
every over-aligned path must move, so this returns NULL there and lets the
generic emulation in d_mem_source_reallocate do the copy.

Parameter(s):
  _context:   unused.
  _ptr:       the block to resize; may be NULL, which means allocate.
  _old_bytes: its current size; unused by this source.
  _new_bytes: the requested size.
  _align:     the alignment it was allocated with.
Return:
  A pointer to the resized block, or NULL when the resize failed or when this
source cannot resize a block of that alignment in place. In the latter case
the ORIGINAL BLOCK IS UNTOUCHED, which is what makes the fallback safe.
*/
static void*
d_internal_mem_system_reallocate(
    void*      _context,
    void*      _ptr,
    d_mem_size _old_bytes,
    d_mem_size _new_bytes,
    d_mem_size _align
)
{
    D_MEM_UNUSED(_context);
    D_MEM_UNUSED(_old_bytes);

    // an over-aligned block cannot be grown in place: realloc makes no
    // alignment promise about the block it returns
    if (d_internal_mem_system_needs_aligned_path(_align))
    {
        return NULL;
    }

    if (_new_bytes == 0)
    {
        return NULL;
    }

    return realloc(_ptr, (size_t)_new_bytes);
}

#endif  // D_INTERNAL_MEM_SOURCE_REALLOC


#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)

/*
d_internal_mem_system_max_size
  The system source's max_size slot.

Parameter(s):
  _context: unused.
Return:
  D_MEM_SIZE_MAX. The system heap's real ceiling is not knowable without
asking it, and a guess would be worse than an honest "unbounded".
*/
static d_mem_size
d_internal_mem_system_max_size(
    const void* _context
)
{
    D_MEM_UNUSED(_context);

    return D_MEM_SIZE_MAX;
}

#endif  // D_INTERNAL_MEM_SOURCE_MAX_SIZE


// d_internal_mem_system_vtable
//   constant: the system source's operation table. One instance for the
// process, since the source is stateless.
static const struct d_mem_source_vtable d_internal_mem_system_vtable =
{
    d_internal_mem_system_allocate,
    d_internal_mem_system_release
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    , d_internal_mem_system_reallocate
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    , d_internal_mem_system_max_size
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    , "system"
#endif
};


/*
d_mem_source_system
  Builds the malloc-backed source.

Parameter(s):
  none.
Return:
  A source that draws from the C library heap. Stateless, so every copy is
interchangeable and none of them owns anything.
*/
struct d_mem_source
d_mem_source_system(void)
{
    struct d_mem_source source;

    source.vtable  = &d_internal_mem_system_vtable;
    source.context = NULL;

    return source;
}

#endif  // D_INTERNAL_MEM_SOURCE_SYSTEM


///////////////////////////////////////////////////////////////////////////////
///                     II.   THE NULL SOURCE                               ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_SOURCE_NULL == 1)

/*
d_internal_mem_null_allocate
  The null source's allocate slot: refuses everything.

Parameter(s):
  _context: unused.
  _bytes:   unused.
  _align:   unused.
Return:
  NULL, always.
*/
static void*
d_internal_mem_null_allocate(
    void*      _context,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    D_MEM_UNUSED(_context);
    D_MEM_UNUSED(_bytes);
    D_MEM_UNUSED(_align);

    return NULL;
}


/*
d_internal_mem_null_release
  The null source's release slot: a no-op, since it never vended anything.

Parameter(s):
  _context: unused.
  _ptr:     unused.
  _bytes:   unused.
  _align:   unused.
Return:
  none.
*/
static void
d_internal_mem_null_release(
    void*      _context,
    void*      _ptr,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    D_MEM_UNUSED(_context);
    D_MEM_UNUSED(_ptr);
    D_MEM_UNUSED(_bytes);
    D_MEM_UNUSED(_align);

    return;
}


#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)

/*
d_internal_mem_null_max_size
  The null source's max_size slot.

Parameter(s):
  _context: unused.
Return:
  Zero -- which lets an allocator discover it can never grow BEFORE it tries,
and is the reason this slot is worth a pointer.
*/
static d_mem_size
d_internal_mem_null_max_size(
    const void* _context
)
{
    D_MEM_UNUSED(_context);

    return 0;
}

#endif  // D_INTERNAL_MEM_SOURCE_MAX_SIZE


// d_internal_mem_null_vtable
//   constant: the null source's operation table.
static const struct d_mem_source_vtable d_internal_mem_null_vtable =
{
    d_internal_mem_null_allocate,
    d_internal_mem_null_release
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    , NULL
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    , d_internal_mem_null_max_size
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    , "null"
#endif
};


/*
d_mem_source_null
  Builds the source that refuses every request.

Parameter(s):
  none.
Return:
  A source whose allocate always fails. Pointing an allocator at it turns
"this subsystem does not allocate" from a claim into a test.
*/
struct d_mem_source
d_mem_source_null(void)
{
    struct d_mem_source source;

    source.vtable  = &d_internal_mem_null_vtable;
    source.context = NULL;

    return source;
}

#endif  // D_INTERNAL_MEM_SOURCE_NULL


///////////////////////////////////////////////////////////////////////////////
///                    III.   THE BUFFER SOURCE                             ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_SOURCE_BUFFER == 1)

/*
d_internal_mem_buffer_allocate
  The buffer source's allocate slot: a bump pointer over the caller's memory.

Parameter(s):
  _context: the struct d_mem_buffer_source; must not be NULL.
  _bytes:   the request size.
  _align:   the required alignment.
Return:
  A pointer into the caller's buffer, or NULL when the buffer cannot satisfy
the request.
*/
static void*
d_internal_mem_buffer_allocate(
    void*      _context,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    struct d_mem_buffer_source* state;
    d_mem_size                  padding;
    d_mem_size                  start;
    d_mem_size                  end;

    if ( (!_context) || (_bytes == 0) )
    {
        return NULL;
    }

    state = (struct d_mem_buffer_source*)_context;

    // align relative to the real address, not to the offset: the caller's
    // buffer is not itself guaranteed to be aligned to _align
    padding = d_mem_align_padding(
                  (d_mem_size)(uintptr_t)(state->base + state->used),
                  _align);

    if (d_mem_add_would_overflow(state->used, padding))
    {
        return NULL;
    }

    start = (d_mem_size)(state->used + padding);

    if (d_mem_add_would_overflow(start, _bytes))
    {
        return NULL;
    }

    end = (d_mem_size)(start + _bytes);

    // a fixed buffer reports exhaustion; it never grows
    if (end > state->capacity)
    {
        return NULL;
    }

    state->used        = end;
    state->last_offset = start;
    state->last_size   = _bytes;

    return (void*)(state->base + start);
}


/*
d_internal_mem_buffer_release
  The buffer source's release slot.
  Reclaims the block only when it is the most recent one, which is the case
that actually arises -- an allocator that takes a region, finds it unusable,
and hands it straight back. Any earlier block is retained until reset, because
a free list for the handful of large regions this source vends would cost more
than it recovers.

Parameter(s):
  _context: the struct d_mem_buffer_source; may be NULL.
  _ptr:     the block to release; may be NULL.
  _bytes:   the size it was allocated with.
  _align:   unused; the block's position already encodes its alignment.
Return:
  none.
*/
static void
d_internal_mem_buffer_release(
    void*      _context,
    void*      _ptr,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    struct d_mem_buffer_source* state;

    D_MEM_UNUSED(_align);

    if ( (!_context) || (!_ptr) )
    {
        return;
    }

    state = (struct d_mem_buffer_source*)_context;

    // pop only when this is exactly the block the bump pointer last vended
    if ( (state->last_size == _bytes) &&
         (state->last_size != 0) &&
         ((char*)_ptr == (state->base + state->last_offset)) )
    {
        state->used      = state->last_offset;
        state->last_size = 0;
    }

    return;
}


#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)

/*
d_internal_mem_buffer_max_size
  The buffer source's max_size slot.

Parameter(s):
  _context: the struct d_mem_buffer_source; may be NULL.
Return:
  The bytes still unused, ignoring alignment padding -- an upper bound, which
is what a growth policy wants when deciding how large a step to attempt.
*/
static d_mem_size
d_internal_mem_buffer_max_size(
    const void* _context
)
{
    const struct d_mem_buffer_source* state;

    if (!_context)
    {
        return 0;
    }

    state = (const struct d_mem_buffer_source*)_context;

    return (d_mem_size)(state->capacity - state->used);
}

#endif  // D_INTERNAL_MEM_SOURCE_MAX_SIZE


// d_internal_mem_buffer_vtable
//   constant: the buffer source's operation table. Its reallocate slot is
// deliberately null: growing in place would only ever succeed for the most
// recent block, and the emulation handles that case correctly anyway.
static const struct d_mem_source_vtable d_internal_mem_buffer_vtable =
{
    d_internal_mem_buffer_allocate,
    d_internal_mem_buffer_release
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    , NULL
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    , d_internal_mem_buffer_max_size
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    , "buffer"
#endif
};


/*
d_mem_buffer_source_init
  Prepares a buffer source over memory the caller owns.
  The buffer is NOT copied and NOT owned. It must outlive every allocator that
draws from the resulting source, which is the caller's obligation and the
whole point: this is how a static array becomes an allocator.

Parameter(s):
  _state:  the state block to prepare; must not be NULL.
  _buffer: the caller's memory; must not be NULL.
  _bytes:  its length; must be non-zero.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when a pointer argument is NULL, or
D_MEM_ERR_INVALID when the length is zero.
*/
enum d_mem_status
d_mem_buffer_source_init(
    struct d_mem_buffer_source* _state,
    void*                       _buffer,
    d_mem_size                  _bytes
)
{
    D_MEM_REQUIRE(_state != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_buffer != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_bytes != 0, D_MEM_ERR_INVALID);

    _state->base        = (char*)_buffer;
    _state->capacity    = _bytes;
    _state->used        = 0;
    _state->last_offset = 0;
    _state->last_size   = 0;

    return D_MEM_OK;
}


/*
d_mem_source_buffer
  Builds a source over a prepared buffer state.

Parameter(s):
  _state: a state block prepared by d_mem_buffer_source_init; may be NULL, in
          which case the unset source is returned rather than a source that
          would fault on first use.
Return:
  A source that vends from the caller's buffer.
*/
struct d_mem_source
d_mem_source_buffer(
    struct d_mem_buffer_source* _state
)
{
    struct d_mem_source source;

    source.vtable  = NULL;
    source.context = NULL;

    if (_state)
    {
        source.vtable  = &d_internal_mem_buffer_vtable;
        source.context = (void*)_state;
    }

    return source;
}


/*
d_mem_buffer_source_used
  Reports how many bytes of a buffer source have been handed out.

Parameter(s):
  _state: the state block to read; may be NULL.
Return:
  The bytes consumed, including alignment padding, or 0 when _state is NULL.
*/
d_mem_size
d_mem_buffer_source_used(
    const struct d_mem_buffer_source* _state
)
{
    if (!_state)
    {
        return 0;
    }

    return _state->used;
}


/*
d_mem_buffer_source_remaining
  Reports how many bytes of a buffer source are still unclaimed.

Parameter(s):
  _state: the state block to read; may be NULL.
Return:
  The bytes remaining, ignoring the alignment padding a future request may
need, or 0 when _state is NULL.
*/
d_mem_size
d_mem_buffer_source_remaining(
    const struct d_mem_buffer_source* _state
)
{
    if (!_state)
    {
        return 0;
    }

    return (d_mem_size)(_state->capacity - _state->used);
}


/*
d_mem_buffer_source_reset
  Reclaims every byte of a buffer source at once.
  EVERY BLOCK IT EVER VENDED IS INVALIDATED. Reset it only when every
allocator drawing from it has itself been released.

Parameter(s):
  _state: the state block to reset; may be NULL.
Return:
  none.
*/
void
d_mem_buffer_source_reset(
    struct d_mem_buffer_source* _state
)
{
    if (_state)
    {
        d_mem_poison_free((void*)_state->base, _state->used);

        _state->used        = 0;
        _state->last_offset = 0;
        _state->last_size   = 0;
    }

    return;
}

#endif  // D_INTERNAL_MEM_SOURCE_BUFFER


///////////////////////////////////////////////////////////////////////////////
///                   IV.   THE COUNTING SOURCE                             ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)

/*
d_internal_mem_counting_allocate
  The counting source's allocate slot: forwards, then records.

Parameter(s):
  _context: the struct d_mem_counting_source; must not be NULL.
  _bytes:   the request size.
  _align:   the required alignment.
Return:
  Whatever the wrapped source returned.
*/
static void*
d_internal_mem_counting_allocate(
    void*      _context,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    struct d_mem_counting_source* state;
    void*                         result;

    if (!_context)
    {
        return NULL;
    }

    state  = (struct d_mem_counting_source*)_context;
    result = d_mem_source_allocate(&state->upstream, _bytes, _align);

    // a refused request is not reserved memory, so only success is recorded
    if (result)
    {
        d_mem_stats_on_reserve(&state->stats, _bytes);
        d_mem_stats_on_acquire(&state->stats, _bytes);
    }

    return result;
}


/*
d_internal_mem_counting_release
  The counting source's release slot: records, then forwards.

Parameter(s):
  _context: the struct d_mem_counting_source; must not be NULL.
  _ptr:     the block to release; may be NULL.
  _bytes:   the size it was allocated with.
  _align:   the alignment it was allocated with.
Return:
  none.
*/
static void
d_internal_mem_counting_release(
    void*      _context,
    void*      _ptr,
    d_mem_size _bytes,
    d_mem_size _align
)
{
    struct d_mem_counting_source* state;

    if ( (!_context) || (!_ptr) )
    {
        return;
    }

    state = (struct d_mem_counting_source*)_context;

    d_mem_stats_on_release(&state->stats, _bytes);
    d_mem_stats_on_unreserve(&state->stats, _bytes);
    d_mem_source_release(&state->upstream, _ptr, _bytes, _align);

    return;
}


#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)

/*
d_internal_mem_counting_max_size
  The counting source's max_size slot: forwards unchanged.

Parameter(s):
  _context: the struct d_mem_counting_source; may be NULL.
Return:
  The wrapped source's ceiling, or 0 when _context is NULL.
*/
static d_mem_size
d_internal_mem_counting_max_size(
    const void* _context
)
{
    const struct d_mem_counting_source* state;

    if (!_context)
    {
        return 0;
    }

    state = (const struct d_mem_counting_source*)_context;

    return d_mem_source_max_size(&state->upstream);
}

#endif  // D_INTERNAL_MEM_SOURCE_MAX_SIZE


// d_internal_mem_counting_vtable
//   constant: the counting source's operation table. Its reallocate slot is
// null on purpose: the emulation routes through allocate and release, which
// are the two slots that count, so a forwarded reallocate would be invisible
// to the very accounting this source exists to provide.
static const struct d_mem_source_vtable d_internal_mem_counting_vtable =
{
    d_internal_mem_counting_allocate,
    d_internal_mem_counting_release
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    , NULL
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    , d_internal_mem_counting_max_size
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    , "counting"
#endif
};


/*
d_mem_counting_source_init
  Prepares a counting decorator over another source.

Parameter(s):
  _state:    the state block to prepare; must not be NULL.
  _upstream: the source to wrap. An unset source is resolved to the configured
             default here, so wrapping "whatever the default is" needs no
             special spelling at the call site.
Return:
  D_MEM_OK on success, or D_MEM_ERR_NULL when _state is NULL.
*/
enum d_mem_status
d_mem_counting_source_init(
    struct d_mem_counting_source* _state,
    struct d_mem_source           _upstream
)
{
    D_MEM_REQUIRE(_state != NULL, D_MEM_ERR_NULL);

    _state->upstream = d_mem_source_resolve(&_upstream);

    d_mem_stats_clear(&_state->stats);

    return D_MEM_OK;
}


/*
d_mem_source_counting
  Builds a source over a prepared counting state.

Parameter(s):
  _state: a state block prepared by d_mem_counting_source_init; may be NULL,
          in which case the unset source is returned.
Return:
  A source that forwards to the wrapped one and accounts for what passes.
*/
struct d_mem_source
d_mem_source_counting(
    struct d_mem_counting_source* _state
)
{
    struct d_mem_source source;

    source.vtable  = NULL;
    source.context = NULL;

    if (_state)
    {
        source.vtable  = &d_internal_mem_counting_vtable;
        source.context = (void*)_state;
    }

    return source;
}


/*
d_mem_counting_source_stats
  Copies out a counting source's accounting block.

Parameter(s):
  _state: the state block to read; must not be NULL.
  _out:   receives a copy of the accounting block; must not be NULL.
Return:
  D_MEM_OK on success, or D_MEM_ERR_NULL when either pointer is NULL.
*/
enum d_mem_status
d_mem_counting_source_stats(
    const struct d_mem_counting_source* _state,
    struct d_mem_stats*                 _out
)
{
    D_MEM_REQUIRE(_state != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);

    *_out = _state->stats;

    return D_MEM_OK;
}

#endif  // D_INTERNAL_MEM_SOURCE_COUNTER


///////////////////////////////////////////////////////////////////////////////
///                    V.   SOURCE OPERATIONS                               ///
///////////////////////////////////////////////////////////////////////////////

/*
d_mem_source_none
  Builds the unset source.
  A zeroed d_mem_source is not an error state; it is the one every
configuration struct starts in, and d_mem_source_resolve turns it into the
configured default. This function exists so a caller can say so explicitly.

Parameter(s):
  none.
Return:
  A source with a null vtable and a null context.
*/
struct d_mem_source
d_mem_source_none(void)
{
    struct d_mem_source source;

    source.vtable  = NULL;
    source.context = NULL;

    return source;
}


/*
d_mem_source_default
  Builds the source an allocator takes when its configuration names none.
  Which one that is comes from D_CFG_MEM_SOURCE_DEFAULT, and it is the null
source on a build with no system heap -- so a freestanding build that forgets
to configure an allocator fails at that allocator's first request, with a
status that says exhausted, rather than at link time with an undefined malloc.

Parameter(s):
  none.
Return:
  The configured default source.
*/
struct d_mem_source
d_mem_source_default(void)
{
#if (D_INTERNAL_MEM_SOURCE_DEFAULT == D_CFG_MEM_SOURCE_DEFAULT_SYSTEM)
    return d_mem_source_system();
#elif (D_INTERNAL_MEM_SOURCE_NULL == 1)
    return d_mem_source_null();
#else
    return d_mem_source_none();
#endif
}


/*
d_mem_source_is_valid
  Reports whether a source can be used.

Parameter(s):
  _source: the source to test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the source has a vtable with both mandatory slots filled, or
  - false, otherwise.
*/
bool
d_mem_source_is_valid(
    const struct d_mem_source* _source
)
{
    if ( (!_source) || (!_source->vtable) )
    {
        return false;
    }

    return ( (_source->vtable->allocate != NULL) &&
             (_source->vtable->release != NULL) );
}


/*
d_mem_source_resolve
  Turns an unset source into the configured default and leaves any other
source alone.
  Every allocator in the subframework calls this exactly once, at init, and
stores the result -- so the "is it set" question is asked one time per
allocator rather than one time per allocation.

Parameter(s):
  _source: the source to resolve; may be NULL, which is treated as unset.
Return:
  _source when it is usable, or the configured default when it is not.
*/
struct d_mem_source
d_mem_source_resolve(
    const struct d_mem_source* _source
)
{
    if (d_mem_source_is_valid(_source))
    {
        return *_source;
    }

    return d_mem_source_default();
}


/*
d_mem_source_name
  Names a source, for diagnostics.

Parameter(s):
  _source: the source to name; may be NULL.
Return:
  A pointer to a static, null-terminated name. Never NULL: an unnamed or
invalid source yields "unknown", so the result may be printed unchecked. When
D_CFG_MEM_SOURCE_NAMED is off, every source reports "unnamed" -- the slot is
absent, not the function.
*/
const char*
d_mem_source_name(
    const struct d_mem_source* _source
)
{
    if (!d_mem_source_is_valid(_source))
    {
        return "unknown";
    }

#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    if (_source->vtable->name)
    {
        return _source->vtable->name;
    }
#endif

    return "unnamed";
}


/*
d_mem_source_allocate
  Requests bytes from a source.
  Normalizes and validates the alignment here, once, so that no source has to
-- which is what keeps the obligation on a caller-written source down to "hand
back N bytes at that alignment".

Parameter(s):
  _source: the source to draw from; must be valid.
  _bytes:  the request size. Zero returns NULL and is not an error.
  _align:  the required alignment; 0 means the configured default.
Return:
  A pointer to _bytes bytes aligned to the normalized alignment, or NULL.
*/
void*
d_mem_source_allocate(
    const struct d_mem_source* _source,
    d_mem_size                 _bytes,
    d_mem_size                 _align
)
{
    d_mem_size align;

    if ( (!d_mem_source_is_valid(_source)) || (_bytes == 0) )
    {
        return NULL;
    }

    // an inadmissible alignment is refused, never rounded: a silently
    // adjusted alignment is a corruption the caller gets blamed for
    align = d_mem_align_accept(_align);

    if (align == 0)
    {
        return NULL;
    }

    return _source->vtable->allocate(_source->context, _bytes, align);
}


/*
d_mem_source_allocate_ex
  Requests bytes from a source, reporting why a failure failed.

Parameter(s):
  _source: the source to draw from; must be valid.
  _bytes:  the request size; must be non-zero for this form, since a caller
           asking for a status wants a zero request called out rather than
           silently answered with an empty block.
  _align:  the required alignment; 0 means the configured default.
  _out:    receives the block on success, or the empty block on failure; must
           not be NULL.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_INVALID when the size is zero or the
alignment inadmissible, or D_MEM_ERR_EXHAUSTED when the source refused.
*/
enum d_mem_status
d_mem_source_allocate_ex(
    const struct d_mem_source* _source,
    d_mem_size                 _bytes,
    d_mem_size                 _align,
    struct d_mem_block*        _out
)
{
    d_mem_size align;
    void*      result;

    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_source != NULL, D_MEM_ERR_NULL);

    *_out = d_mem_block_empty();

    if (!d_mem_source_is_valid(_source))
    {
        return D_MEM_ERR_NULL;
    }

    if (_bytes == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    align = d_mem_align_accept(_align);

    if (align == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    result = _source->vtable->allocate(_source->context, _bytes, align);

    if (!result)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    *_out = d_mem_block_make(result, _bytes);

    return D_MEM_OK;
}


/*
d_mem_source_release
  Returns bytes to the source that vended them.
  _bytes and _align MUST be the values that produced the block; see the
contract in the header. This function does not and cannot check that.

Parameter(s):
  _source: the source that vended the block; must be valid.
  _ptr:    the block; may be NULL, in which case nothing happens.
  _bytes:  the size the block was allocated with.
  _align:  the alignment the block was allocated with.
Return:
  none.
*/
void
d_mem_source_release(
    const struct d_mem_source* _source,
    void*                      _ptr,
    d_mem_size                 _bytes,
    d_mem_size                 _align
)
{
    d_mem_size align;

    if ( (!d_mem_source_is_valid(_source)) || (!_ptr) )
    {
        return;
    }

    //   NORMALIZE, not accept. This must reproduce exactly the alignment that
    // allocate passed down, and allocate normalized -- so normalizing again
    // is the only way to arrive at the same number. Validation would be
    // pointless here anyway: allocate already refused every inadmissible
    // value, so a block that exists was allocated at an admissible one.
    align = d_mem_align_normalize(_align);

    _source->vtable->release(_source->context, _ptr, _bytes, align);

    return;
}


/*
d_mem_source_release_block
  Returns a block view to the source that vended it, and empties the view.
  Emptying is the point: a released block whose view still names its former
address is the most direct route to a use-after-free this subframework has, so
the one function that releases a view also invalidates it.

Parameter(s):
  _source: the source that vended the block; must be valid.
  _block:  the view to release and empty; may be NULL.
  _align:  the alignment the block was allocated with.
Return:
  none.
*/
void
d_mem_source_release_block(
    const struct d_mem_source* _source,
    struct d_mem_block*        _block,
    d_mem_size                 _align
)
{
    if ( (!_block) || (d_mem_block_is_empty(_block)) )
    {
        return;
    }

    d_mem_source_release(_source, _block->ptr, _block->size, _align);

    _block->ptr  = NULL;
    _block->size = 0;

    return;
}


/*
d_mem_source_reallocate
  Resizes a block, in place where the source can and by copying where it
cannot.
  THE EMULATION IS THE POINT. A source that leaves its reallocate slot null --
or a build that removed the slot entirely -- still supports resizing, because
allocate, memcpy and release compose into it. That is what lets
D_CFG_MEM_SOURCE_REALLOC be a knob about a copy rather than about a
capability, and what keeps the obligation on a caller-written source to two
functions.

Parameter(s):
  _source:    the source that vended the block; must be valid.
  _ptr:       the block to resize; NULL means allocate.
  _old_bytes: its current size.
  _new_bytes: the requested size; zero releases and returns NULL.
  _align:     the alignment the block was allocated with.
Return:
  A pointer to the resized block, or NULL on failure. ON FAILURE THE ORIGINAL
BLOCK IS UNTOUCHED and remains the caller's to release.
*/
void*
d_mem_source_reallocate(
    const struct d_mem_source* _source,
    void*                      _ptr,
    d_mem_size                 _old_bytes,
    d_mem_size                 _new_bytes,
    d_mem_size                 _align
)
{
    d_mem_size align;
    void*      result;
    d_mem_size copy_bytes;

    if (!d_mem_source_is_valid(_source))
    {
        return NULL;
    }

    align  = d_mem_align_accept(_align);
    result = NULL;

    if (align == 0)
    {
        return NULL;
    }

    // a resize to nothing is a release
    if (_new_bytes == 0)
    {
        d_mem_source_release(_source, _ptr, _old_bytes, align);

        return NULL;
    }

    // a resize from nothing is an allocation
    if (!_ptr)
    {
        return d_mem_source_allocate(_source, _new_bytes, align);
    }

#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    // give the source its chance to grow in place first
    if (_source->vtable->reallocate)
    {
        result = _source->vtable->reallocate(_source->context,
                                             _ptr,
                                             _old_bytes,
                                             _new_bytes,
                                             align);

        if (result)
        {
            return result;
        }
    }
#endif

    // the emulation: allocate, copy the overlap, release the original
    result = d_mem_source_allocate(_source, _new_bytes, align);

    if (!result)
    {
        return NULL;
    }

    copy_bytes = (_old_bytes < _new_bytes)
        ? _old_bytes
        : _new_bytes;

    if (copy_bytes > 0)
    {
        memcpy(result, _ptr, (size_t)copy_bytes);
    }

    d_mem_source_release(_source, _ptr, _old_bytes, align);

    return result;
}


/*
d_mem_source_reallocate_ex
  Resizes a block view in place, reporting why a failure failed and leaving
the view untouched when it does.

Parameter(s):
  _source:    the source that vended the block; must be valid.
  _new_bytes: the requested size; zero releases the block and empties the view.
  _align:     the alignment the block was allocated with.
  _block:     the view to resize; must not be NULL. Updated only on success.
Return:
  D_MEM_OK, D_MEM_ERR_NULL, D_MEM_ERR_INVALID, or D_MEM_ERR_EXHAUSTED.
*/
enum d_mem_status
d_mem_source_reallocate_ex(
    const struct d_mem_source* _source,
    d_mem_size                 _new_bytes,
    d_mem_size                 _align,
    struct d_mem_block*        _block
)
{
    void* result;

    D_MEM_REQUIRE(_block != NULL, D_MEM_ERR_NULL);
    D_MEM_REQUIRE(_source != NULL, D_MEM_ERR_NULL);

    if (!d_mem_source_is_valid(_source))
    {
        return D_MEM_ERR_NULL;
    }

    if (_new_bytes == 0)
    {
        d_mem_source_release_block(_source, _block, _align);

        return D_MEM_OK;
    }

    result = d_mem_source_reallocate(_source,
                                     _block->ptr,
                                     _block->size,
                                     _new_bytes,
                                     _align);

    if (!result)
    {
        return D_MEM_ERR_EXHAUSTED;
    }

    _block->ptr  = result;
    _block->size = _new_bytes;

    return D_MEM_OK;
}


/*
d_mem_source_max_size
  Reports the largest single request a source could satisfy.

Parameter(s):
  _source: the source to ask; may be NULL.
Return:
  The source's ceiling, or D_MEM_SIZE_MAX when it declares none. An invalid
source reports 0, which is honest: it can satisfy nothing.
*/
d_mem_size
d_mem_source_max_size(
    const struct d_mem_source* _source
)
{
    if (!d_mem_source_is_valid(_source))
    {
        return 0;
    }

#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    if (_source->vtable->max_size)
    {
        return _source->vtable->max_size(_source->context);
    }
#endif

    return D_MEM_SIZE_MAX;
}
