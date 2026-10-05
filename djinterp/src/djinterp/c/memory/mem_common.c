/*******************************************************************************
* djinterp [c]                                                      mem_common.c
*
*   Implementation unit for mem_common.h. Carries the out-of-line vocabulary --
* block views, checked arithmetic, slot geometry, accounting, the hardening
* fills and the status names -- and re-declares the header's `inline` kernels
* so exactly one external definition of each is emitted for the compiled C
* library (no effect under D_CFG_MEM_HEADER_ONLY).
*
*
* path:      /src/djinterp/c/memory/mem_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#include "../../../../inc/djinterp/c/memory/mem_common.h"


/*   C99 out-of-line emission. The kernels in mem_common.h are declared
* `inline` with their bodies in the header, so a C++ TU and an inlining C
* caller use them directly. Under C99 inline rules that emits NO external
* definition, which a non-inlined call (at -O0) or a taken address still
* needs -- so exactly one translation unit must re-declare each of them with
* `extern inline`, and this is that unit.
*   Under D_CFG_MEM_HEADER_ONLY the kernels are `static inline` instead,
* every TU carries its own copy, and re-declaring here would emit a second
* unused static definition; hence the guard.
*/
#if (D_INTERNAL_MEM_HEADER_ONLY == 0)

D_MEM_EXTERN_INLINE bool       d_mem_is_pow2(d_mem_size _value);
D_MEM_EXTERN_INLINE d_mem_size d_mem_align_normalize(d_mem_size _align);
D_MEM_EXTERN_INLINE bool       d_mem_align_is_valid(d_mem_size _align);
D_MEM_EXTERN_INLINE d_mem_size d_mem_align_accept(d_mem_size _align);
D_MEM_EXTERN_INLINE d_mem_size d_mem_align_up(d_mem_size _value,
                                              d_mem_size _align);
D_MEM_EXTERN_INLINE d_mem_size d_mem_align_padding(d_mem_size _value,
                                                   d_mem_size _align);
D_MEM_EXTERN_INLINE bool       d_mem_align_would_overflow(d_mem_size _value,
                                                          d_mem_size _align);
D_MEM_EXTERN_INLINE bool       d_mem_is_aligned(const void* _ptr,
                                                d_mem_size  _align);
D_MEM_EXTERN_INLINE bool       d_mem_add_would_overflow(d_mem_size _a,
                                                        d_mem_size _b);
D_MEM_EXTERN_INLINE bool       d_mem_mul_would_overflow(d_mem_size _a,
                                                        d_mem_size _b);
D_MEM_EXTERN_INLINE d_mem_size d_mem_redzone_lead(d_mem_size _align);

#endif  // D_INTERNAL_MEM_HEADER_ONLY


/*
d_mem_block_make
  Builds a block view over an existing byte range. The view borrows: it does
not copy, own, or validate the bytes, and it is valid only for as long as the
allocator that produced them says it is.

Parameter(s):
  _ptr:  the first byte, or NULL.
  _size: the length in bytes.
Return:
  A block view of (_ptr, _size). A null _ptr yields the empty block regardless
of _size, so that "empty" has one representation rather than two.
*/
struct d_mem_block
d_mem_block_make(
    void*      _ptr,
    d_mem_size _size
)
{
    struct d_mem_block block;

    block.ptr  = NULL;
    block.size = 0;

    // a null base is the empty block whatever size was claimed for it
    if (_ptr)
    {
        block.ptr  = _ptr;
        block.size = _size;
    }

    return block;
}


/*
d_mem_block_empty
  Builds the canonical empty block.

Parameter(s):
  none.
Return:
  A block view whose pointer is NULL and whose size is zero.
*/
struct d_mem_block
d_mem_block_empty(void)
{
    struct d_mem_block block;

    block.ptr  = NULL;
    block.size = 0;

    return block;
}


/*
d_mem_block_is_empty
  Reports whether a block names any bytes.

Parameter(s):
  _block: the block to test; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if any of the following were true:
    - _block is NULL,
    - the block's pointer is NULL,
    - the block's size is zero, or
  - false, otherwise.
*/
bool
d_mem_block_is_empty(
    const struct d_mem_block* _block
)
{
    if (!_block)
    {
        return true;
    }

    return ( (!_block->ptr) || (_block->size == 0) );
}


/*
d_mem_block_end
  Computes the one-past-the-end address of a block.

Parameter(s):
  _block: the block to measure; may be NULL.
Return:
  A pointer one past the block's last byte, or NULL when the block is empty.
The result is a valid comparison operand but must not be dereferenced.
*/
void*
d_mem_block_end(
    const struct d_mem_block* _block
)
{
    if (d_mem_block_is_empty(_block))
    {
        return NULL;
    }

    return (void*)((char*)_block->ptr + _block->size);
}


/*
d_mem_block_contains
  Reports whether an address falls inside a block.

  NOTE ON PORTABILITY: this compares pointers that are not necessarily part of
the same array object, which the standard leaves unspecified. Every allocator
in this subframework needs the question answered anyway, so the comparison is
done ONCE, here, through uintptr_t -- where the ordering is defined by the
implementation's address mapping rather than left to the language.

Parameter(s):
  _block: the block to test against; may be NULL.
  _ptr:   the address to locate; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the block is non-empty and _ptr lies within [begin, end), or
  - false, otherwise.
*/
bool
d_mem_block_contains(
    const struct d_mem_block* _block,
    const void*               _ptr
)
{
    uintptr_t begin;
    uintptr_t end;
    uintptr_t target;

    if ( (d_mem_block_is_empty(_block)) ||
         (!_ptr) )
    {
        return false;
    }

    begin  = (uintptr_t)_block->ptr;
    end    = begin + (uintptr_t)_block->size;
    target = (uintptr_t)_ptr;

    return ( (target >= begin) && (target < end) );
}


/*
d_mem_align_up_checked
  Rounds a value up to the next multiple of an alignment, reporting rather
than wrapping when the rounded value would exceed D_MEM_SIZE_MAX.
  This is the routine every module calls on a caller-supplied magnitude. The
unchecked d_mem_align_up exists for the internal cases where the magnitude is
a compile-time-bounded slot or header size and the comparison would be dead.

Parameter(s):
  _value: the magnitude to round.
  _align: the alignment; normalized and validated here, so 0 is accepted and
          means "the configured default".
  _out:   receives the rounded value on success; untouched on failure.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when _out is NULL, D_MEM_ERR_INVALID
when the normalized alignment is not an admissible power of two, or
D_MEM_ERR_OVERFLOW when the rounded value would not fit.
*/
enum d_mem_status
d_mem_align_up_checked(
    d_mem_size  _value,
    d_mem_size  _align,
    d_mem_size* _out
)
{
    d_mem_size align;

    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);

    align = d_mem_align_accept(_align);

    // reject an alignment no allocator in this subframework will honour
    if (align == 0)
    {
        return D_MEM_ERR_INVALID;
    }

    // the rounded value must still be representable
    if (d_mem_align_would_overflow(_value, align))
    {
        return D_MEM_ERR_OVERFLOW;
    }

    *_out = d_mem_align_up(_value, align);

    return D_MEM_OK;
}


/*
d_mem_add_checked
  Adds two magnitudes, reporting rather than wrapping on overflow.

Parameter(s):
  _a:   the first addend.
  _b:   the second addend.
  _out: receives the sum on success; untouched on failure.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when _out is NULL, or
D_MEM_ERR_OVERFLOW when the sum would exceed D_MEM_SIZE_MAX.
*/
enum d_mem_status
d_mem_add_checked(
    d_mem_size  _a,
    d_mem_size  _b,
    d_mem_size* _out
)
{
    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);

    if (d_mem_add_would_overflow(_a, _b))
    {
        return D_MEM_ERR_OVERFLOW;
    }

    *_out = (d_mem_size)(_a + _b);

    return D_MEM_OK;
}


/*
d_mem_mul_checked
  Multiplies two magnitudes, reporting rather than wrapping on overflow. This
is the guard on every "count times element size" computation in the
subframework, which is the classic place an attacker-supplied count turns into
an under-sized allocation.

Parameter(s):
  _a:   the first factor.
  _b:   the second factor.
  _out: receives the product on success; untouched on failure.
Return:
  D_MEM_OK on success, D_MEM_ERR_NULL when _out is NULL, or
D_MEM_ERR_OVERFLOW when the product would exceed D_MEM_SIZE_MAX.
*/
enum d_mem_status
d_mem_mul_checked(
    d_mem_size  _a,
    d_mem_size  _b,
    d_mem_size* _out
)
{
    D_MEM_REQUIRE(_out != NULL, D_MEM_ERR_NULL);

    if (d_mem_mul_would_overflow(_a, _b))
    {
        return D_MEM_ERR_OVERFLOW;
    }

    *_out = (d_mem_size)(_a * _b);

    return D_MEM_OK;
}


/*
d_mem_slot_align
  Computes the alignment a fixed-size slot must carry.
  When the slot will hold an intrusive link while unused, it must also satisfy
that link's alignment -- which is the whole reason slot geometry is a
computation rather than a copy of the caller's request.
  The link's alignment is a PARAMETER rather than a flag, because the link is
not always a pointer: a pool threading its free list by 32-bit index needs
four-byte alignment where one threading it by pointer needs eight, and the
difference is the pool's minimum slot size. Pass D_MEM_LINK_ALIGN for a
pointer link, or 0 for no link at all.

Parameter(s):
  _align:      the caller's required alignment; 0 means the configured
               default.
  _link_align: the alignment of the link stored in an unused slot; 0 when
               unused slots hold nothing.
Return:
  The normalized alignment, raised to the link's alignment where a link shares
the storage. The result is a power of two, or 0 when the normalized alignment
was inadmissible.
*/
d_mem_size
d_mem_slot_align(
    d_mem_size _align,
    d_mem_size _link_align
)
{
    d_mem_size align;

    align = d_mem_align_accept(_align);

    if (align == 0)
    {
        return 0;
    }

    // a slot that stores a link while free must satisfy the link's alignment
    if (align < _link_align)
    {
        align = _link_align;
    }

    return align;
}


/*
d_mem_slot_size
  Computes the byte stride of a fixed-size slot: the object widened to hold an
intrusive link when one will share the storage, widened by the guard bands
when redzones are configured, and rounded up to the slot alignment.

Parameter(s):
  _object_size: the caller's object size in bytes; must be non-zero.
  _slot_align:  the slot alignment, as returned by d_mem_slot_align.
  _link_size:   the size of the link stored in an unused slot; 0 when unused
                slots hold nothing.
Return:
  The slot stride in bytes, or 0 when the arguments are inadmissible or the
computation would overflow.
*/
d_mem_size
d_mem_slot_size(
    d_mem_size _object_size,
    d_mem_size _slot_align,
    d_mem_size _link_size
)
{
    d_mem_size size;

    if ( (_object_size == 0) ||
         (!d_mem_align_is_valid(_slot_align)) )
    {
        return 0;
    }

    size = _object_size;

    //   Guard bands sit on both sides of the payload, but they are NOT the
    // same width: the leading one is rounded up to the slot alignment so that
    // the payload, which starts after it, stays aligned. That asymmetry is
    // why this is d_mem_redzone_lead plus D_MEM_REDZONE_BYTES rather than
    // D_MEM_REDZONE_TOTAL -- and getting it wrong put the leading band on top
    // of the preceding block's header.
    if (D_MEM_REDZONE_BYTES != 0)
    {
        d_mem_size guards;

        guards = (d_mem_size)(d_mem_redzone_lead(_slot_align) +
                              D_MEM_REDZONE_BYTES);

        if (d_mem_add_would_overflow(size, guards))
        {
            return 0;
        }

        size = (d_mem_size)(size + guards);
    }

    // a slot that stores a link while free must be wide enough to hold one
    if (size < _link_size)
    {
        size = _link_size;
    }

    // the stride must keep every slot after the first correctly aligned
    if (d_mem_align_would_overflow(size, _slot_align))
    {
        return 0;
    }

    return d_mem_align_up(size, _slot_align);
}


/*
d_mem_stats_clear
  Returns an accounting block to its zero state.

Parameter(s):
  _stats: the block to clear; may be NULL, in which case nothing happens.
Return:
  none.
*/
void
d_mem_stats_clear(
    struct d_mem_stats* _stats
)
{
    if (_stats)
    {
        memset(_stats, 0, sizeof(struct d_mem_stats));
    }

    return;
}


/*
d_mem_stats_on_reserve
  Records bytes obtained from the upstream source.

Parameter(s):
  _stats: the block to update; may be NULL.
  _bytes: the number of bytes obtained.
Return:
  none.
*/
void
d_mem_stats_on_reserve(
    struct d_mem_stats* _stats,
    d_mem_size          _bytes
)
{
    if (_stats)
    {
        _stats->bytes_reserved =
            (d_mem_size)(_stats->bytes_reserved + _bytes);
        _stats->upstream_count =
            (d_mem_size)(_stats->upstream_count + 1);
    }

    return;
}


/*
d_mem_stats_on_unreserve
  Records bytes returned to the upstream source.

Parameter(s):
  _stats: the block to update; may be NULL.
  _bytes: the number of bytes returned.
Return:
  none.
*/
void
d_mem_stats_on_unreserve(
    struct d_mem_stats* _stats,
    d_mem_size          _bytes
)
{
    if (_stats)
    {
        // clamp rather than wrap: an under-count is a wrong number, an
        // under-flowed count is a wrong number that looks like a leak
        if (_stats->bytes_reserved >= _bytes)
        {
            _stats->bytes_reserved =
                (d_mem_size)(_stats->bytes_reserved - _bytes);
        }
        else
        {
            _stats->bytes_reserved = 0;
        }
    }

    return;
}


/*
d_mem_stats_on_acquire
  Records bytes handed to a caller, updating the running and (when configured)
high-water totals.

Parameter(s):
  _stats: the block to update; may be NULL.
  _bytes: the number of bytes handed out, including padding and guard bands.
Return:
  none.
*/
void
d_mem_stats_on_acquire(
    struct d_mem_stats* _stats,
    d_mem_size          _bytes
)
{
    if (_stats)
    {
        _stats->bytes_committed =
            (d_mem_size)(_stats->bytes_committed + _bytes);
        _stats->bytes_live =
            (d_mem_size)(_stats->bytes_live + _bytes);
        _stats->acquire_count =
            (d_mem_size)(_stats->acquire_count + 1);

#if (D_INTERNAL_MEM_STATS_PEAK == 1)
        // high-water marks: a compare and a conditional store, which is why
        // they are gated separately from the running totals above
        if (_stats->bytes_live > _stats->peak_bytes_live)
        {
            _stats->peak_bytes_live = _stats->bytes_live;
        }

        if ((_stats->acquire_count - _stats->release_count) >
            _stats->peak_live_count)
        {
            _stats->peak_live_count =
                (d_mem_size)(_stats->acquire_count - _stats->release_count);
        }
#endif
    }

    return;
}


/*
d_mem_stats_on_release
  Records bytes returned by a caller.

Parameter(s):
  _stats: the block to update; may be NULL.
  _bytes: the number of bytes returned.
Return:
  none.
*/
void
d_mem_stats_on_release(
    struct d_mem_stats* _stats,
    d_mem_size          _bytes
)
{
    if (_stats)
    {
        // clamp rather than wrap, for the reason given in on_unreserve
        if (_stats->bytes_live >= _bytes)
        {
            _stats->bytes_live =
                (d_mem_size)(_stats->bytes_live - _bytes);
        }
        else
        {
            _stats->bytes_live = 0;
        }

        _stats->release_count =
            (d_mem_size)(_stats->release_count + 1);
    }

    return;
}


/*
d_mem_stats_on_reset
  Records a bulk reclamation: every live byte becomes free at once, but the
backing storage is retained, so the reserved total survives and the live total
does not.

Parameter(s):
  _stats: the block to update; may be NULL.
Return:
  none.
*/
void
d_mem_stats_on_reset(
    struct d_mem_stats* _stats
)
{
    if (_stats)
    {
        _stats->bytes_live    = 0;
        _stats->release_count = _stats->acquire_count;
    }

    return;
}


/*
d_mem_stats_utilization
  Computes the fraction of reserved bytes that are currently live.

Parameter(s):
  _stats: the block to read; may be NULL.
Return:
  A value in [0.0, 1.0], or 0.0 when the block is NULL or nothing has been
reserved. Values above 1.0 are impossible by construction and are clamped, so
a caller may use the result as a ratio without checking it.
*/
double
d_mem_stats_utilization(
    const struct d_mem_stats* _stats
)
{
    double live;
    double reserved;

    if ( (!_stats) || (_stats->bytes_reserved == 0) )
    {
        return 0.0;
    }

    live     = (double)_stats->bytes_live;
    reserved = (double)_stats->bytes_reserved;

    if (live >= reserved)
    {
        return 1.0;
    }

    return (live / reserved);
}


/*
d_mem_poison
  Fills a range with the allocation poison byte, so that a read of memory that
was never written produces a recognisable value rather than a plausible one.
Compiles to nothing when poisoning is off.

Parameter(s):
  _ptr:  the first byte; may be NULL.
  _size: the number of bytes to fill.
Return:
  none.
*/
void
d_mem_poison(
    void*      _ptr,
    d_mem_size _size
)
{
#if (D_INTERNAL_MEM_POISON_ALLOC == 1)
    if ( (_ptr) && (_size > 0) )
    {
        memset(_ptr, D_MEM_POISON_ALLOC, (size_t)_size);
    }
#else
    D_MEM_UNUSED(_ptr);
    D_MEM_UNUSED(_size);
#endif

    return;
}


/*
d_mem_poison_free
  Fills a range with the release poison byte. Distinct from d_mem_poison so
that a dump distinguishes "never written" from "written, then freed" -- which
is the difference between an initialisation bug and a use-after-free.

Parameter(s):
  _ptr:  the first byte; may be NULL.
  _size: the number of bytes to fill.
Return:
  none.
*/
void
d_mem_poison_free(
    void*      _ptr,
    d_mem_size _size
)
{
#if (D_INTERNAL_MEM_POISON_FREE == 1)
    if ( (_ptr) && (_size > 0) )
    {
        memset(_ptr, D_MEM_POISON_FREE, (size_t)_size);
    }
#else
    D_MEM_UNUSED(_ptr);
    D_MEM_UNUSED(_size);
#endif

    return;
}


/*
d_mem_zero
  Fills a range with zero. Unconditional -- this is the routine a caller asks
for explicitly, as distinct from d_mem_prepare, which honours the configured
policy.

Parameter(s):
  _ptr:  the first byte; may be NULL.
  _size: the number of bytes to fill.
Return:
  none.
*/
void
d_mem_zero(
    void*      _ptr,
    d_mem_size _size
)
{
    if ( (_ptr) && (_size > 0) )
    {
        memset(_ptr, 0, (size_t)_size);
    }

    return;
}


/*
d_mem_prepare
  Applies the configured allocate-edge treatment to a freshly vended range:
zeroing when D_CFG_MEM_ZERO_ON_ALLOC is on, poisoning when it is off and
D_CFG_MEM_POISON is on, and nothing otherwise.
  The two are resolved against each other in cfg_mem_common.h rather than
here, so this function has one branch per configuration and every allocator in
the subframework calls exactly one routine on its vend path.

Parameter(s):
  _ptr:  the first byte; may be NULL.
  _size: the number of bytes to treat.
Return:
  none.
*/
void
d_mem_prepare(
    void*      _ptr,
    d_mem_size _size
)
{
#if (D_INTERNAL_MEM_ZERO == 1)
    d_mem_zero(_ptr, _size);
#elif (D_INTERNAL_MEM_POISON_ALLOC == 1)
    d_mem_poison(_ptr, _size);
#else
    D_MEM_UNUSED(_ptr);
    D_MEM_UNUSED(_size);
#endif

    return;
}


/*
d_mem_redzone_write
  Stamps the guard bands around a payload. _payload points at the first byte
the CALLER will see; the bands sit immediately before and after it, which is
why the pointer handed to the caller is always D_MEM_REDZONE_BYTES past the
start of the underlying block.
  Compiles to nothing when redzones are off.

Parameter(s):
  _payload:      the first byte of the caller-visible range; may be NULL.
  _payload_size: the length of the caller-visible range.
Return:
  none.
*/
void
d_mem_redzone_write(
    void*      _payload,
    d_mem_size _payload_size
)
{
#if (D_INTERNAL_MEM_REDZONE == 1)
    char* base;

    if (!_payload)
    {
        return;
    }

    base = (char*)_payload;

    memset(base - (size_t)D_MEM_REDZONE_BYTES,
           D_MEM_REDZONE_FILL,
           (size_t)D_MEM_REDZONE_BYTES);
    memset(base + (size_t)_payload_size,
           D_MEM_REDZONE_FILL,
           (size_t)D_MEM_REDZONE_BYTES);
#else
    D_MEM_UNUSED(_payload);
    D_MEM_UNUSED(_payload_size);
#endif

    return;
}


/*
d_mem_redzone_verify
  Checks that the guard bands around a payload are intact.

Parameter(s):
  _payload:      the first byte of the caller-visible range; may be NULL.
  _payload_size: the length of the caller-visible range.
Return:
  A boolean value corresponding to either:
  - true, if any of the following were true:
    - redzones are disabled (there is nothing to violate),
    - both guard bands still hold the fill byte, or
  - false, if either band has been overwritten.
*/
bool
d_mem_redzone_verify(
    const void* _payload,
    d_mem_size  _payload_size
)
{
#if (D_INTERNAL_MEM_REDZONE == 1)
    const unsigned char* base;
    d_mem_size           index;

    if (!_payload)
    {
        return true;
    }

    base = (const unsigned char*)_payload;

    // the band that precedes the payload catches an under-run
    for (index = 0; index < D_MEM_REDZONE_BYTES; ++index)
    {
        if (base[-(ptrdiff_t)(index + 1)] !=
            (unsigned char)D_MEM_REDZONE_FILL)
        {
            return false;
        }
    }

    // the band that follows it catches the far more common over-run
    for (index = 0; index < D_MEM_REDZONE_BYTES; ++index)
    {
        if (base[(size_t)_payload_size + (size_t)index] !=
            (unsigned char)D_MEM_REDZONE_FILL)
        {
            return false;
        }
    }

    return true;
#else
    D_MEM_UNUSED(_payload);
    D_MEM_UNUSED(_payload_size);

    return true;
#endif
}


/*
d_mem_status_string
  Names a status value.
  The strings are stable and are part of the reporting contract: a differential
test that diffs two text streams compares these, so changing one is a wire-
format change and not a cosmetic edit.

Parameter(s):
  _status: the status to name.
Return:
  A pointer to a static, null-terminated name. Never NULL -- an unrecognized
value yields "unknown", so a caller may print the result without checking it.
*/
const char*
d_mem_status_string(
    enum d_mem_status _status
)
{
    switch (_status)
    {
        case D_MEM_OK:              return "ok";
        case D_MEM_ERR_NULL:        return "null-argument";
        case D_MEM_ERR_INVALID:     return "invalid-argument";
        case D_MEM_ERR_OVERFLOW:    return "size-overflow";
        case D_MEM_ERR_EXHAUSTED:   return "exhausted";
        case D_MEM_ERR_UNSUPPORTED: return "unsupported";
        case D_MEM_ERR_FOREIGN:     return "foreign-pointer";
        case D_MEM_ERR_STALE:       return "stale-handle";
        case D_MEM_ERR_CORRUPT:     return "corrupt";
        default:                    break;
    }

    return "unknown";
}


/*
d_mem_status_is_ok
  Reports whether a status names success.

Parameter(s):
  _status: the status to test.
Return:
  A boolean value corresponding to either:
  - true, if _status is D_MEM_OK, or
  - false, otherwise.
*/
bool
d_mem_status_is_ok(
    enum d_mem_status _status
)
{
    return (_status == D_MEM_OK);
}
