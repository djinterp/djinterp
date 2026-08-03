/******************************************************************************
* djinterp [sync]                                               cow_common.h
*
* Shared core for copy-on-write.
*   Tier 0.  A value held behind a reference count: readers share it and pay
* nothing, a writer that finds the count above one copies first.  BOTH
* languages compile this declaration.
*
* THE SCHEME
*   Readers take a snapshot, which increments the count and pins the value
* for as long as the snapshot lives.  A writer wanting to modify checks the
* count: at one it is the sole owner and may write in place; above one it
* copies, modifies the copy, and publishes it.  Whoever holds the old value
* keeps reading it until they let go, and the last release frees it.
*
*   The consequence worth stating: a reader's view is a SNAPSHOT in the
* strict sense of sync.tex section 5 - one frozen value for the whole of its
* life, not a valid-but-shifting subset.  That is the strongest consistency
* any strategy here offers, and it is why copy-on-write is chosen despite the
* copy.
*
* THE C PROBLEM, AND WHERE THE SEAM IS
*   The C++ block is `{atomic refcount, T value}` - one struct with a typed
* member.  C cannot name T, so the block is a HEADER followed by bytes, and
* the payload's offset is computed from its alignment rather than fixed:
*
*     header: 16 bytes, 8-aligned
*     int         payload at 16, block 20
*     long double payload at 16, block 32
*     cache line  payload at 64, block 128    <- over-aligned, offset moves
*
*   That is the whole difference.  The reference counting, the clone-on-write
* decision and the publication protocol never touch the value; they are
* untyped and they are core.  What the C side needs instead of the type is a
* DESCRIPTOR - size, alignment, how to copy, how to destroy - which is the
* d_value_ops record below.
*
* THE OPS RECORD COMPOSES WITH RECLAMATION
*   d_value_ops contains a d_reclaim_ops as its FIRST member, so a pointer to
* one converts to a pointer to the other.  That is not a trick for its own
* sake: a copy-on-write block whose count reaches zero while readers may
* still hold references is retired into a d_reclaim_list, and the list wants
* exactly the reclaim ops.  One descriptor serves both consumers because the
* two are the same object seen at different widths.
*
* KNOWN DEFECT: THE LOCK-FREE SNAPSHOT IS NOT RACE-FREE.  USE cow_rcu_state.
*   d_cow_state_snapshot takes a reference to a block another agent may be
* freeing, which is the hardest step in any lock-free design.  Three
* corrections went into it, each of them right on its own terms:
*
*     1. the reference taken ATOMICALLY WITH the pointer read, via a
*        double-width compare-and-exchange, which fixed the original
*        use-after-free
*     2. the writer holding a reference before it clones, rather than
*        dereferencing a block it did not own
*     3. the split-count absorption made ONE atomic add rather than N, since
*        another reader could complete a whole acquire-and-release between
*        two of them and drive the count to zero mid-absorption
*
*   ThreadSanitizer still reports a race on recycled storage.  The remaining
* window has not been located, and three failed attempts is enough evidence
* that it will not be found by inspection.
*
* WHAT TO USE INSTEAD
*   cow_rcu_state, in cow_cpp.hpp.  A reader enters an epoch section and
* reads the pointer; a writer publishes and RETIRES the old block, and the
* grace-period scan frees it once every section that could have seen it has
* ended.  There is no reference count on the read path, so there is nothing
* to take a reference to and nothing to race on taking.
*
*   VERIFIED CLEAN under the stress suite's ThreadSanitizer stage, alongside
* hazard pointers, RCU, the stamped pointer and the backoff.  The reader path
* is also cheaper - an epoch publish and a pointer load, against a
* double-width CAS plus two more atomics - so the simpler design is the
* faster one, which is not the usual direction.
*
* WHAT cow_state IS STILL FOR
*   A single agent, or many agents holding a lock.  Its reference counting is
* correct when nothing races: the parity table covers it and passes, and the
* clone-on-write decision is what makes copy-on-write worth having at all.
* Only the LOCK-FREE snapshot is unsound.
*
* THE LESSON, WHICH IS WORTH MORE THAN THE DEFECT
*   Reference counting looks composable and is not.  The four schemes that
* never claimed to be simple - hazard pointers, RCU, the stamped pointer, the
* backoff - came out clean.  The one that looked simplest needed a different
* mechanism underneath, and sync.tex section 7 removed the strategy-to-
* obligation map precisely so that a copy-on-write container could say so:
* cow_rcu_state's obligation is grace_period, which the deleted map forbade.
*
* path:      /inc/djinterp/core/sync/cow_common.h
* link(s):   monograph, chapter Concurrency (sync.tex) section 6
* author(s): Sam 'teer' Neal-Blim                          created: 2026.08.01
******************************************************************************/

#ifndef DJINTERP_SYNC_COW_COMMON_
#define DJINTERP_SYNC_COW_COMMON_ 1

//   THE FEATURE-TEST HEADER COMES BEFORE THE STANDARD ONES, which inverts
// the framework's usual "std, then djinterp" include order and is the one
// place that order is wrong.  glibc's <features.h> reads the feature-test
// macros once, at its first inclusion, and every later standard header
// consults what it decided - so a <stddef.h> above this line would freeze
// the surface before env_feature_test.h could request POSIX, and
// pthread_rwlock_t would be invisible under -std=c11.

#include "../env/env_feature_test.h"

// std
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// djinterp
#include "../../c/djinterp.h"
#include "../../c/datomic.h"
#include "./sync_common.h"
#include "./reclaim_common.h"
// (datomic.h already pulls datomic_dw.h)

//   Absent without lock-free pointer atomics: the published pointer is read
// by every reader on every snapshot, and a lock behind it would serialize
// exactly the readers the scheme exists to keep free.
#if (D_INTERNAL_SYNC_STRATEGY_COW == 1) && (D_ATOMIC_HAVE_DW == 1)

D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///             I.    THE VALUE DESCRIPTOR                                  ///
///////////////////////////////////////////////////////////////////////////////

// d_value_copy_fn
//   type: constructs a copy of a value at _dst.
//   _dst is raw storage of the right size and alignment, NOT an existing
// value - so this is construction, not assignment, and a C++ implementation
// spells it with placement new rather than operator=.
typedef void (*d_value_copy_fn)(void*       _dst,
                                const void* _src,
                                void*       _context);

// struct d_value_ops
//   struct: everything the untyped core needs to know about a value.
//
//   THE RECLAIM OPS COME FIRST, DELIBERATELY.  A pointer to a d_value_ops is
// also a pointer to its d_reclaim_ops - the standard C composition - so a
// block retired for deferred reclamation needs no separate descriptor and no
// cast that the standard does not bless.  The two records are one object
// seen at two widths.
//
//   LIFETIME: static storage duration.  A descriptor must outlive every
// block that names it, and a function-local one would not.
struct d_value_ops
{
    struct d_reclaim_ops reclaim;   /* MUST be first; see above */
    size_t               size;
    size_t               alignment;
    d_value_copy_fn      copy;
};

// D_VALUE_OPS_INIT
//   macro: initializer for a descriptor at declaration.
#define D_VALUE_OPS_INIT(_destroy, _ctx, _size, _align, _copy)                \
    { D_RECLAIM_OPS_INIT((_destroy), (_ctx)), (_size), (_align), (_copy) }

// d_value_ops_as_reclaim
//   function: the descriptor's reclamation half.
//   A named function rather than a cast at the call site, so the dependency
// on the first-member layout is written once and asserted once - see the
// assertion in section VI.
D_INLINE const struct d_reclaim_ops*
d_value_ops_as_reclaim
(
    const struct d_value_ops* _ops
)
{
    return (_ops != 0) ? &_ops->reclaim : (const struct d_reclaim_ops*)0;
}


///////////////////////////////////////////////////////////////////////////////
///             II.   THE BLOCK                                             ///
///////////////////////////////////////////////////////////////////////////////

// struct d_cow_header
//   struct: the reference count and the descriptor.  The VALUE follows this
// in memory, at the offset d_cow_payload_offset computes.
//
//   Not a flexible array member, and not a `char payload[]` field: the
// payload's alignment may exceed the header's, so its offset is not
// sizeof(header) in general.  A cache-line-aligned value starts at 64 where
// the header ends at 16.  Computing the offset rather than declaring it is
// what makes an over-aligned payload work.
struct d_cow_header
{
    d_atomic_size_t           refcount;
    const struct d_value_ops* ops;
};

// d_cow_payload_offset
//   function: where the value begins, relative to the header.
//   The header size rounded up to the payload's alignment.
D_INLINE size_t
d_cow_payload_offset
(
    size_t _alignment
)
{
    const size_t base = sizeof(struct d_cow_header);
    const size_t a    = (_alignment != 0) ? _alignment : 1u;

    return (base + a - 1u) & ~(a - 1u);
}

// d_cow_block_size
//   function: total bytes a block for this descriptor occupies.
//   Use this to size an allocation or to carve one out of an arena; the
// framework does not allocate behind the caller's back, so the caller
// supplies the storage.
D_INLINE size_t
d_cow_block_size
(
    const struct d_value_ops* _ops
)
{
    if (!_ops)
    {
        return 0;
    }

    return d_cow_payload_offset(_ops->alignment) + _ops->size;
}

// d_cow_value
//   function: the value inside a block.
D_INLINE void*
d_cow_value
(
    struct d_cow_header* _block
)
{
    if (!_block || !_block->ops)
    {
        return (void*)0;
    }

    return (void*)((unsigned char*)_block
                       + d_cow_payload_offset(_block->ops->alignment));
}

// d_cow_value_const
//   function: the value inside a block, read-only.
//   THE ONLY LEGITIMATE ACCESS FOR A READER.  A snapshot holds a reference
// to a block that a writer may be about to replace - it will not be
// destroyed while the snapshot lives, but it is shared, and writing through
// it would be visible to every other holder.
D_INLINE const void*
d_cow_value_const
(
    const struct d_cow_header* _block
)
{
    if (!_block || !_block->ops)
    {
        return (const void*)0;
    }

    return (const void*)((const unsigned char*)_block
                             + d_cow_payload_offset(_block->ops->alignment));
}

// d_cow_block_init
//   function: prepares caller-supplied storage as a block with a reference
// count of one.  Does NOT construct the value - the caller does that, into
// d_cow_value(_block), because only the caller knows how.
//
//   The count starts at one rather than zero: the block is born owned by
// whoever created it, and a count of zero would mean a concurrent scan could
// reclaim it before its creator published it.
D_INLINE void
d_cow_block_init
(
    struct d_cow_header*      _block,
    const struct d_value_ops* _ops
)
{
    if (!_block)
    {
        return;
    }

    d_atomic_init_size(&_block->refcount, 1);
    _block->ops = _ops;
}

// d_cow_block_acquire
//   function: takes a reference.
//   RELAXED, and that is not an oversight.  Acquiring a reference to a block
// the caller already reached means the pointer to it was already
// synchronized by whatever published it; the increment orders nothing
// further.  This is the same argument shared_ptr's copy constructor makes,
// and it matters: the increment is on every snapshot, which is the reader
// fast path.
D_INLINE void
d_cow_block_acquire
(
    struct d_cow_header* _block
)
{
    if (!_block)
    {
        return;
    }

    (void)d_atomic_fetch_add_size_explicit(&_block->refcount, 1,
                                           D_MEMORY_ORDER_RELAXED);
}

// d_cow_block_acquire_many
//   function: takes _n references in ONE atomic step.
//
//   NOT A LOOP OF ACQUIRES, and the difference is a use-after-free.  The
// split-count publish below grants one reference per in-flight reader, and
// an earlier version did so with N separate increments.  Between two of
// them another reader can complete a whole acquire-and-release cycle and
// drive the count to zero - so the block is destroyed halfway through the
// loop that exists to keep it alive.
//
//   ThreadSanitizer reported it as a race on recycled storage, which is what
// a use-after-free looks like once the allocator has handed the memory to
// somebody else.  The window is invisible to inspection: every individual
// increment is correct, and the defect is only in their not being one.
D_INLINE void
d_cow_block_acquire_many
(
    struct d_cow_header* _block,
    size_t               _n
)
{
    if (!_block || (_n == 0))
    {
        return;
    }

    (void)d_atomic_fetch_add_size_explicit(&_block->refcount, _n,
                                           D_MEMORY_ORDER_RELAXED);
}

// d_cow_block_release
//   function: drops a reference.  Returns 1 when the caller took the last
// one and must destroy the block.
//
//   ACQ_REL, and here the ordering IS load-bearing.  The release half
// publishes everything this agent did with the value before the last holder
// destroys it; the acquire half means the destroying agent sees every other
// agent's writes.  A relaxed decrement would let the destructor race the
// last reader's final read.
D_INLINE int
d_cow_block_release
(
    struct d_cow_header* _block
)
{
    if (!_block)
    {
        return 0;
    }

    return (d_atomic_fetch_sub_size_explicit(&_block->refcount, 1,
                                             D_MEMORY_ORDER_ACQ_REL) == 1)
               ? 1 : 0;
}

// d_cow_block_use_count
//   function: how many references exist.
//   Stale on return; useful for diagnostics and for the unique check below,
// where staleness is safe in one direction only.
D_INLINE size_t
d_cow_block_use_count
(
    const struct d_cow_header* _block
)
{
    if (!_block)
    {
        return 0;
    }

    return (size_t)d_atomic_load_size_explicit(&_block->refcount,
                                               D_MEMORY_ORDER_ACQUIRE);
}

// d_cow_block_is_unique
//   function: whether the caller holds the only reference.
//
//   SAFE IN ONE DIRECTION ONLY, and the asymmetry is the whole basis of the
// write path.  If this returns true it is TRUE and stays true: no other
// agent holds a reference, so none can take a second one - a reference can
// only be created from an existing one.  If it returns false it may already
// be stale, and the writer copies unnecessarily, which costs a copy and
// nothing else.
//
//   Wrong in the other direction would be a disaster: writing in place to a
// block another agent is reading.  The asymmetry is what makes an unsynchron-
// ized read of the count sound here.
D_INLINE int
d_cow_block_is_unique
(
    const struct d_cow_header* _block
)
{
    return (d_cow_block_use_count(_block) == 1u) ? 1 : 0;
}

// d_cow_block_destroy
//   function: destroys the value and returns the storage to the caller's
// discipline via the descriptor's destroy hook.
//   Call ONLY when release returned 1.  Calling it otherwise destroys a
// value other agents are still reading.
D_INLINE void
d_cow_block_destroy
(
    struct d_cow_header* _block
)
{
    if (!_block || !_block->ops)
    {
        return;
    }

    if (_block->ops->reclaim.destroy)
    {
        _block->ops->reclaim.destroy((void*)_block,
                                     _block->ops->reclaim.context);
    }
}

// d_cow_block_copy_into
//   function: constructs a copy of _src's value into _dst's payload, and
// initializes _dst's header with a count of one.
//   The clone half of clone-on-write.  _dst must be storage of at least
// d_cow_block_size(ops) bytes, aligned to at least the payload's alignment.
D_INLINE int
d_cow_block_copy_into
(
    struct d_cow_header*       _dst,
    const struct d_cow_header* _src
)
{
    const struct d_value_ops* ops;

    if (!_dst || !_src || !_src->ops)
    {
        return 0;
    }

    ops = _src->ops;

    d_cow_block_init(_dst, ops);

    if (ops->copy)
    {
        ops->copy(d_cow_value(_dst), d_cow_value_const(_src),
                  ops->reclaim.context);
    }
    else
    {
        //   No copy hook means the value is trivially copyable and the
        // descriptor said so by leaving the hook null.  memcpy is then
        // correct and is what a C caller of a plain-old-data type expects.
        memcpy(d_cow_value(_dst), d_cow_value_const(_src), ops->size);
    }

    return 1;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  THE PUBLISHED POINTER                                 ///
///////////////////////////////////////////////////////////////////////////////

// struct d_cow_state
//   struct: the currently published block, together with an EXTERNAL count
// of readers currently taking a reference to it.
//
//   THE EXTERNAL COUNT IS NOT AN OPTIMIZATION.  An earlier version held only
// the pointer and took the reference in two steps - load the pointer, then
// increment the block's own count - with a re-read afterwards to confirm the
// pointer had not moved.  That loop closes one race and not the other:
//
//     reader                      writer
//     ------                      ------
//     block = load(&current)
//                                 publish(replacement)
//                                 release(block) -> 0
//                                 destroy(block)          <- freed here
//     acquire(block)                                      <- USE AFTER FREE
//     confirm...                                          <- never reached
//
//   The confirm cannot help: the damage is done by the increment, before the
// re-read runs.  ThreadSanitizer found exactly this - a heap-use-after-free
// in d_atomic_fetch_add_size_explicit, from a reader's acquire, against a
// delete in the writer's destroy.
//
//   Reference counting alone cannot make a lock-free snapshot safe, and no
// arrangement of re-reads fixes it.  The reference must be taken ATOMICALLY
// WITH the pointer read, which is what the double-width compare-and-exchange
// below does: the pair {block, external} moves as one unit, so a reader that
// obtained the pointer has already been counted, and the writer cannot see
// zero while a reader is arriving.
//
//   ABSENT WITHOUT A DOUBLE-WIDTH CAS.  The tier law again: a copy-on-write
// state that cannot take its references safely is not a degraded one.
struct d_cow_state
{
    struct d_atomic_dw pair;    /* lo = block pointer, hi = external count */
};

// d_cow_state_init
//   function: publishes an initial block without synchronization, for a
// state no other agent can yet reach.
D_INLINE void
d_cow_state_init
(
    struct d_cow_state*  _state,
    struct d_cow_header* _block
)
{
    if (!_state)
    {
        return;
    }

    d_atomic_dw_init(&_state->pair, (D_ATOMIC_DW_WORD)(uintptr_t)_block, 0);
}

// d_cow_state_snapshot
//   function: takes a reference to the current block and returns it, or null
// when nothing is published.
//
//   THE LOOP IS NOT OPTIONAL, and it is the same shape as the hazard confirm
// for the same reason.  Between loading the pointer and incrementing its
// count, a writer may publish a replacement and release the old block to
// zero - so the increment could land on a block already being destroyed.
// Re-reading afterwards and retrying if the pointer moved closes it: if the
// published pointer is unchanged after the increment, the count was raised
// while the block was still published, and no releaser can have taken it to
// zero.
//
//   This is the one place the reader path is more than a load, and it is
// unavoidable without a hazard pointer or an epoch underneath - which is
// exactly what a COW-over-RCU container does, and why the reclamation
// obligation is DECLARED rather than derived from the strategy.
D_INLINE struct d_cow_header*
d_cow_state_snapshot
(
    struct d_cow_state* _state
)
{
    struct d_backoff   bo = D_BACKOFF_INIT;
    struct d_atomic_dw seen;

    if (!_state)
    {
        return (struct d_cow_header*)0;
    }

    seen = d_atomic_dw_load(&_state->pair);

    for (;;)
    {
        struct d_atomic_dw wanted;

        if (seen.lo == 0)
        {
            return (struct d_cow_header*)0;
        }

        //   Claim the reference IN THE PAIR, so obtaining the pointer and
        // being counted are one operation.  Until this succeeds the reader
        // has touched nothing but the state itself, which the writer never
        // frees.
        wanted.lo = seen.lo;
        wanted.hi = seen.hi + 1u;

        if (d_atomic_dw_compare_exchange(&_state->pair, &seen, wanted))
        {
            //   Counted.  The block cannot have been destroyed, because the
            // writer's release path (below) folds the external count into
            // the block before dropping its own reference - so a nonzero
            // external count keeps the block alive.
            struct d_cow_header* block =
                (struct d_cow_header*)(uintptr_t)wanted.lo;

            //   Move the claim from the state to the block, then give the
            // state's external count back.  After this the reader holds an
            // ordinary reference and the state is as it was.
            d_cow_block_acquire(block);

            for (;;)
            {
                struct d_atomic_dw give_back;

                //   Only decrement if the pointer is still the same one; if
                // a writer replaced it, the writer already absorbed this
                // reader's external count into the old block, and
                // decrementing here would drop it twice.
                if (seen.lo != wanted.lo)
                {
                    //   The writer took the external count with it.  Release
                    // the extra reference it granted on our behalf.
                    if (d_cow_block_release(block))
                    {
                        d_cow_block_destroy(block);
                        return (struct d_cow_header*)0;
                    }

                    break;
                }

                give_back.lo = seen.lo;
                give_back.hi = seen.hi - 1u;

                if (d_atomic_dw_compare_exchange(&_state->pair, &seen,
                                                 give_back))
                {
                    break;
                }

                d_backoff_pause(&bo);
            }

            return block;
        }

        //   Lost the claim; `seen` was refreshed by the exchange.
        d_backoff_pause(&bo);
    }
}

// d_cow_state_publish
//   function: publishes _block and returns the block it replaced, WITHOUT
// releasing it - the caller decides what happens to the old one, because
// only the caller knows whether it is safe to destroy now or must be
// retired.
//
//   Returning rather than releasing is deliberate.  A container using plain
// reference counting releases immediately; one using COW over RCU or hazard
// pointers retires instead.  Deciding here would bake one reclamation
// obligation into the strategy, which sync.tex section 7 removed the map for.
D_INLINE struct d_cow_header*
d_cow_state_publish
(
    struct d_cow_state*  _state,
    struct d_cow_header* _block
)
{
    struct d_backoff   bo = D_BACKOFF_INIT;
    struct d_atomic_dw seen;

    if (!_state)
    {
        return (struct d_cow_header*)0;
    }

    seen = d_atomic_dw_load(&_state->pair);

    for (;;)
    {
        struct d_atomic_dw wanted;

        wanted.lo = (D_ATOMIC_DW_WORD)(uintptr_t)_block;
        wanted.hi = 0;

        if (d_atomic_dw_compare_exchange(&_state->pair, &seen, wanted))
        {
            struct d_cow_header* old =
                (struct d_cow_header*)(uintptr_t)seen.lo;

            //   ABSORB THE EXTERNAL COUNT INTO THE OLD BLOCK.  Readers that
            // claimed a reference in the pair but have not yet moved it to
            // the block are counted in seen.hi; granting those references
            // here is what keeps the block alive until they arrive.
            //
            //   Without this the writer could see a count of one - its own -
            // and destroy a block several readers are about to touch, which
            // is the use-after-free this design exists to prevent.
            d_cow_block_acquire_many(old, (size_t)seen.hi);

            return old;
        }

        d_backoff_pause(&bo);
    }
}

// d_cow_state_compare_publish
//   function: publishes _block only if the current pointer is still
// _expected.  Returns 1 on success; on failure writes the current pointer
// into *_expected so a retry needs no separate read.
//
//   For a writer that copied from a particular block and must not overwrite
// a newer one - the read-copy-update shape, where losing the race means
// discarding the copy and starting again from what is there now.
D_INLINE int
d_cow_state_compare_publish
(
    struct d_cow_state*   _state,
    struct d_cow_header** _expected,
    struct d_cow_header*  _block
)
{
    struct d_atomic_dw seen;
    struct d_atomic_dw wanted;

    if (!_state || !_expected)
    {
        return 0;
    }

    seen = d_atomic_dw_load(&_state->pair);

    if ((struct d_cow_header*)(uintptr_t)seen.lo != *_expected)
    {
        *_expected = (struct d_cow_header*)(uintptr_t)seen.lo;
        return 0;
    }

    wanted.lo = (D_ATOMIC_DW_WORD)(uintptr_t)_block;
    wanted.hi = 0;

    if (d_atomic_dw_compare_exchange(&_state->pair, &seen, wanted))
    {
        //   Same absorption as publish; see there for why it is required.
        struct d_cow_header* old = (struct d_cow_header*)(uintptr_t)seen.lo;

        d_cow_block_acquire_many(old, (size_t)seen.hi);

        return 1;
    }

    *_expected = (struct d_cow_header*)(uintptr_t)seen.lo;

    return 0;
}

// d_cow_state_current
//   function: the published block WITHOUT taking a reference.
//
//   THIS IS THE WRITER'S ENTRY POINT, and using the snapshot instead is the
// mistake that makes copy-on-write copy every time.
//
//   The published state itself holds one reference.  A block with no readers
// therefore has a count of exactly one, and d_cow_needs_clone says false -
// the writer may modify in place, which is the whole optimization.  A writer
// that took a SNAPSHOT first would raise the count to two and see "shared"
// even when it is alone, so it would clone unconditionally and the scheme
// would degrade to copy-always without any symptom other than being slow.
//
//   The writer protocol is therefore:
//
//       block = d_cow_state_current(&state);
//       if (d_cow_needs_clone(block)) { copy, modify, publish }
//       else                          { modify in place }
//
//   A READER MUST NOT USE THIS.  Dereferencing it has no protection against
// a concurrent publication; readers call d_cow_state_snapshot.
D_INLINE struct d_cow_header*
d_cow_state_current
(
    const struct d_cow_state* _state
)
{
    if (!_state)
    {
        return (struct d_cow_header*)0;
    }

    {
        const struct d_atomic_dw seen = d_atomic_dw_load(
            (const struct d_atomic_dw*)&_state->pair);

        return (struct d_cow_header*)(uintptr_t)seen.lo;
    }
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   THE WRITE DECISION                                    ///
///////////////////////////////////////////////////////////////////////////////

// d_cow_needs_clone
//   function: whether a writer holding a reference to _block must copy
// before modifying it.
//
//   THIS IS THE ENTIRE COPY-ON-WRITE DECISION, and it is one comparison.
// Everything else in this file is the bookkeeping that makes the comparison
// meaningful.  See d_cow_block_is_unique for why the answer is trustworthy
// in the direction that matters and merely conservative in the other.
D_INLINE int
d_cow_needs_clone
(
    const struct d_cow_header* _block
)
{
    return (d_cow_block_is_unique(_block) == 0) ? 1 : 0;
}

// d_cow_retire_into
//   function: hands a block to a reclamation list rather than destroying it.
//   For a container whose readers are protected by epochs or hazard pointers
// rather than by the reference count alone - the count reaching zero means
// no SNAPSHOT holds it, which is not the same as no agent being able to
// reach it.
//
//   The descriptor's reclaim half is what the list needs, which is why the
// two records compose.
D_INLINE int
d_cow_retire_into
(
    struct d_reclaim_list* _list,
    struct d_cow_header*   _block,
    uint64_t               _tag
)
{
    if (!_list || !_block || !_block->ops)
    {
        return 0;
    }

    return d_reclaim_list_retire(_list, (void*)_block,
                                 d_value_ops_as_reclaim(_block->ops),
                                 _tag);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    STORAGE HELPERS                                       ///
///////////////////////////////////////////////////////////////////////////////

// D_COW_BLOCK_BYTES
//   macro: bytes a block for a payload of _size and _align occupies.
//   The compile-time form of d_cow_block_size, for declaring storage.
#define D_COW_BLOCK_BYTES(_size, _align)                                      \
    ((((sizeof(struct d_cow_header) + (_align) - 1u)                          \
       & ~((size_t)(_align) - 1u)) + (_size)))

// D_COW_STORAGE
//   macro: declares suitably-aligned storage for one block.
//     D_COW_STORAGE(buf, sizeof(struct thing), D_ALIGNOF(struct thing));
//     struct d_cow_header* block = (struct d_cow_header*)buf;
//   The union forces the storage to the header's alignment; a payload
// needing more than that must be declared by the caller with its own
// alignment attribute, because C has no portable way to align a declaration
// to a value computed here.
#define D_COW_STORAGE(_name, _size, _align)                                   \
    union                                                                     \
    {                                                                         \
        struct d_cow_header hdr_;                                             \
        unsigned char       bytes_[D_COW_BLOCK_BYTES((_size), (_align))];     \
    } _name


///////////////////////////////////////////////////////////////////////////////
///             VI.   CONFORMANCE ASSERTIONS                                ///
///////////////////////////////////////////////////////////////////////////////

//   THE COMPOSITION THE OPS RECORD DEPENDS ON.  d_value_ops_as_reclaim takes
// the address of the first member and hands it out as a d_reclaim_ops*; that
// is well-defined precisely because the member is first, and this asserts it
// rather than trusting the declaration order to survive an edit.
D_STATIC_ASSERT(offsetof(struct d_value_ops, reclaim) == 0,
                "d_value_ops's reclaim half must be its first member; "
                "d_value_ops_as_reclaim depends on it");

#if (D_INTERNAL_SYNC_ASSERT_LAYOUT == 1)

//   The header is a count and a descriptor pointer.  It is the fixed part of
// every block, so growth changes the offset of every payload and the size of
// every allocation the caller made.
D_STATIC_ASSERT(sizeof(struct d_cow_header) >=
                    sizeof(d_atomic_size_t) + sizeof(void*),
                "d_cow_header is smaller than its own members");

#if (D_HAS_ALIGNOF == 1)
D_STATIC_ASSERT(D_ALIGNOF(struct d_cow_header) >= D_ALIGNOF(void*),
                "d_cow_header is aligned below its pointer member");
#endif

D_STATIC_ASSERT(sizeof(struct d_cow_state) == sizeof(struct d_atomic_dw),
                "d_cow_state carries state beyond its {block, count} pair");

#endif  // D_INTERNAL_SYNC_ASSERT_LAYOUT

#if (D_INTERNAL_DATOMIC_REQUIRE_LOCK_FREE_SHARED == 1)
#if defined(ATOMIC_POINTER_LOCK_FREE)
//   The published pointer is read by every reader on every snapshot.  A lock
// behind it would serialize exactly the readers the scheme exists to keep
// free, and - as everywhere in this subframework - the C and C++ runtimes do
// not choose their locks from the same table, so a shared state would not
// even be correct.
D_STATIC_ASSERT(ATOMIC_POINTER_LOCK_FREE == 2,
                "copy-on-write needs lock-free pointer atomics");
#endif
#endif


D_EXTERN_C_END

#endif  // D_INTERNAL_SYNC_STRATEGY_COW

#endif  // DJINTERP_SYNC_COW_COMMON_
