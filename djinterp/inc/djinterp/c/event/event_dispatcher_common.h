/*******************************************************************************
* djinterp [c]                                         event_dispatcher_common.h
*
* The queue and the facade -- the shared C core (tier 0):
*   Deferred delivery and the unified front end. A queue is a word of pending
* occurrences q in Sigma-hat*; enqueue appends one with its payload copied BY
* VALUE, and process folds dispatch over a prefix of that word against the
* registry read at processing time. The dispatcher composes a registry and a
* queue and is the one type most callers name.
*
* WHY THE PAYLOAD IS COPIED:
*   Deferral outlives the caller's stack frame. C++ captured the payload in a
* shared_ptr; C copies the bytes into the queue's own arena. This is the one
* place the core must know a payload's SIZE rather than treating it as opaque,
* and it is why struct d_event_payload carries one.
*
* RE-ENTRANCY:
*   process() fixes its prefix length before invoking anything. An occurrence
* enqueued by a handler during processing lands after that prefix and is
* deferred to a later call -- it is never dispatched by the call that
* triggered it. The byte arena is a ring that REFUSES to overwrite bytes
* belonging to an occurrence still in flight, returning D_EVENT_ERR_CAPACITY
* instead, so a re-entrant enqueue cannot corrupt the occurrence that made it.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   immediate fire   delta_rho at once      -- d_event_dispatcher_fire
*   queue      q in Sigma-hat*              -- struct d_event_queue
*   enqueue    q . (e,a)  (right concat)    -- d_event_queue_enqueue
*   process    fold of delta_rho over q,    -- d_event_queue_process
*              rho read at processing time
*   merge      rho (+) rho'                 -- d_event_dispatcher_merge
*   fused drive hat-h over a trace          -- d_event_word_drive
*
*   DEFERRAL COHERENCE LAW: for a registry held fixed,
*
*       fire(d, k, a)   ==   queue(d, k, a); process(d, 1)
*
*   in verdict, in invocation count, and in handler side effects and their
*   order. The two paths differ only in WHEN the registry is read, so holding
*   it fixed must collapse the difference. This is a conformance test, not a
*   comment.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/event/event_dispatcher_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_DISPATCHER_COMMON_H
#define DJINTERP_C_EVENT_EVENT_DISPATCHER_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../../config/core/event/cfg_event_dispatcher.h"
#include "./event_common.h"
#include "./event_handler_common.h"
#include "./event_table_common.h"
#include "./event_registry_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///        0.    RESOLVED CONFIGURATION                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Reads only; resolved in cfg_event_dispatcher.h.

// D_EVENT_QUEUE_DEFAULT_CAPACITY / D_EVENT_QUEUE_DEFAULT_BYTES
//   constant: the capacities an allocating queue falls back to when given 0.
#define D_EVENT_QUEUE_DEFAULT_CAPACITY                                        \
    ((size_t)D_INTERNAL_EVENT_QUEUE_DEFAULT_CAPACITY)
#define D_EVENT_QUEUE_DEFAULT_BYTES                                           \
    ((size_t)D_INTERNAL_EVENT_QUEUE_DEFAULT_BYTES)

// D_EVENT_QUEUE_OVERFLOW_REJECT / _DROP_OLDEST
//   constant: the two overflow policies, pinned. D_EVENT_QUEUE_OVERFLOW names
// the one this build uses; enqueue's contract changes with it, so a caller
// that must not lose an occurrence should check for _REJECT rather than
// assume it.
#define D_EVENT_QUEUE_OVERFLOW_REJECT       0
#define D_EVENT_QUEUE_OVERFLOW_DROP_OLDEST  1
#define D_EVENT_QUEUE_OVERFLOW                                                \
    D_INTERNAL_EVENT_QUEUE_OVERFLOW


///////////////////////////////////////////////////////////////////////////////
///        I.    THE QUEUE SLOT                                             ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_QUEUE == 1)

// d_event_slot
//   struct: one pending occurrence. The payload lives in the queue's byte
// arena and is named by an OFFSET, not a pointer, so the whole queue is
// relocatable and writable to disk as-is (AGENT_README.md section 8).
struct d_event_slot
{
    d_event_key key;
    size_t      offset;   // byte offset into the queue arena
    size_t      size;     // payload size in bytes; 0 for an empty payload
    uint32_t    arity;
    uint32_t    flags;    // reserved; must be 0
};


///////////////////////////////////////////////////////////////////////////////
///        II.   THE QUEUE                                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_EVENT_QUEUE_FLAG_OWNS_STORAGE
//   constant: set when the queue allocated its own slot and byte arenas.
#define D_EVENT_QUEUE_FLAG_OWNS_STORAGE     ((uint32_t)0x00000001u)

// D_EVENT_QUEUE_FLAG_FIXED
//   constant: set when the arenas are caller-provided and may not grow.
#define D_EVENT_QUEUE_FLAG_FIXED            ((uint32_t)0x00000002u)

// d_event_queue
//   struct: a ring of pending occurrences over a ring of payload bytes.
// `head` is the next slot to process, `tail` the next to fill.
struct d_event_queue
{
    unsigned char*       bytes;        // payload arena
    struct d_event_slot* slots;        // slot ring
    size_t               bytes_size;   // arena capacity, in bytes
    size_t               bytes_head;   // first live byte
    size_t               bytes_used;   // live bytes
    size_t               capacity;     // slot capacity
    size_t               head;         // next slot to process
    size_t               count;        // pending occurrences
    uint32_t             flags;        // D_EVENT_QUEUE_FLAG_*
    uint32_t             reserved;     // must be 0
};

// d_event_queue_init_fixed
//   function: initializes a queue over caller-provided storage. No
// allocation; enqueue past either capacity returns D_EVENT_ERR_CAPACITY.
int d_event_queue_init_fixed(struct d_event_queue* _queue,
                             struct d_event_slot*  _slots,
                             size_t                _capacity,
                             unsigned char*        _bytes,
                             size_t                _bytes_size);

#if (D_INTERNAL_EVENT_QUEUE_ALLOC == 1)

// d_event_queue_init
//   function: initializes an allocating queue. A zero argument selects the
// corresponding D_EVENT_QUEUE_DEFAULT_*.
int d_event_queue_init(struct d_event_queue* _queue,
                       size_t                _capacity,
                       size_t                _bytes_size);

#endif  // D_INTERNAL_EVENT_QUEUE_ALLOC

// d_event_queue_dispose
//   function: discards every pending occurrence and frees owned storage.
void d_event_queue_dispose(struct d_event_queue* _queue);

// d_event_queue_clear
//   function: discards every pending occurrence WITHOUT dispatching it.
// Storage is retained.
void d_event_queue_clear(struct d_event_queue* _queue);


///////////////////////////////////////////////////////////////////////////////
///        III.  ENQUEUE AND PROCESS                                        ///
///////////////////////////////////////////////////////////////////////////////

// d_event_queue_enqueue
//   function: appends an occurrence of `_key`, COPYING `_payload.size` bytes
// from `_payload.data` into the queue's arena (right concatenation
// q . (e, a)). A payload of size 0 is valid and copies nothing.
//   The registry is not consulted here: binding happens at processing time.
// returns: D_EVENT_OK, D_EVENT_ERR_NULL, or D_EVENT_ERR_CAPACITY when either
// arena is full. On D_EVENT_ERR_CAPACITY nothing is enqueued.
int d_event_queue_enqueue(struct d_event_queue*  _queue,
                          d_event_key            _key,
                          struct d_event_payload _payload);

// d_event_queue_process
//   function: dispatches up to `_max` pending occurrences against
// `_registry`, in order, releasing each as it completes. `_max` of 0 means
// every occurrence pending AT ENTRY.
//   The prefix length is fixed before any handler runs, so occurrences
// enqueued re-entrantly are deferred to a later call.
//   `_result_out` may be NULL; when supplied it receives the aggregate over
// the processed prefix.
// returns: the number of occurrences processed.
size_t d_event_queue_process(struct d_event_queue*    _queue,
                             struct d_event_registry* _registry,
                             size_t                   _max,
                             struct d_run_result*     _result_out);

// d_event_queue_process_all
//   function: d_event_queue_process with `_max` of 0.
size_t d_event_queue_process_all(struct d_event_queue*    _queue,
                                 struct d_event_registry* _registry,
                                 struct d_run_result*     _result_out);


///////////////////////////////////////////////////////////////////////////////
///        IV.   QUEUE STATE                                                ///
///////////////////////////////////////////////////////////////////////////////

// d_event_queue_pending
//   function: the number of occurrences currently queued.
D_STATIC_INLINE size_t
d_event_queue_pending(const struct d_event_queue* _queue)
{
    return (_queue ? _queue->count : 0u);
}

// d_event_queue_is_empty
//   function: true if no occurrence is queued.
D_STATIC_INLINE bool
d_event_queue_is_empty(const struct d_event_queue* _queue)
{
    return (d_event_queue_pending(_queue) == 0u);
}

// d_event_queue_peek
//   function: the slot at the head of the queue, or NULL when empty. The
// payload it names is readable through d_event_queue_payload.
const struct d_event_slot* d_event_queue_peek(
    const struct d_event_queue* _queue);

// d_event_queue_payload
//   function: resolves a slot's offset against the queue's arena, yielding a
// borrowed payload view. Valid until the slot is processed or the queue is
// cleared.
struct d_event_payload d_event_queue_payload(
    const struct d_event_queue* _queue,
    const struct d_event_slot*  _slot);

// d_event_queue_footprint
//   function: the queue's total byte footprint including owned storage.
size_t d_event_queue_footprint(const struct d_event_queue* _queue);


#endif  // D_INTERNAL_EVENT_QUEUE


///////////////////////////////////////////////////////////////////////////////
///        V.    THE DISPATCHER (the facade)                                ///
///////////////////////////////////////////////////////////////////////////////

// d_event_dispatcher
//   struct: the unified front end -- a registry composed with a queue. It
// adds no state of its own beyond the two, which is what makes the C++
// wrapper free.
//   This is the successor to the pre-core `struct d_event_handler`. The
// rename is not cosmetic: "handler" now names the STEP primitive, per the
// note, and the facade needed a different word.
//   LAYOUT NOTE: the queue member is present only when deferral is compiled
// in. This is the one knob in the subframework that changes a shared struct,
// and goals section 4 sanctions it -- "representation is a policy, not a
// type". Both languages read the same knob, so C and C++ never disagree; two
// differently-configured BUILDS do, which is what D_INTERNAL_EVENT_LAYOUT_ID
// in cfg_event.h exists to stamp and detect.
struct d_event_dispatcher
{
    struct d_event_registry registry;

#if (D_INTERNAL_EVENT_QUEUE == 1)
    struct d_event_queue    queue;
#endif
};

// d_event_dispatcher_init_fixed
//   function: initializes a dispatcher over caller-provided storage for both
// the table and the queue. No allocation anywhere.
#if (D_INTERNAL_EVENT_QUEUE == 1)
int d_event_dispatcher_init_fixed(struct d_event_dispatcher* _dispatcher,
                                  struct d_event_entry*      _entries,
                                  size_t                     _capacity,
                                  uint32_t*                  _bucket_head,
                                  uint32_t*                  _bucket_tail,
                                  size_t                     _bucket_count,
                                  struct d_event_slot*       _slots,
                                  size_t                     _slot_capacity,
                                  unsigned char*             _bytes,
                                  size_t                     _bytes_size);
#else
int d_event_dispatcher_init_fixed(struct d_event_dispatcher* _dispatcher,
                                  struct d_event_entry*      _entries,
                                  size_t                     _capacity,
                                  uint32_t*                  _bucket_head,
                                  uint32_t*                  _bucket_tail,
                                  size_t                     _bucket_count);
#endif

// d_event_dispatcher_init
//   function: initializes an allocating dispatcher. A zero argument selects
// the corresponding default.
#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)
int d_event_dispatcher_init(struct d_event_dispatcher* _dispatcher,
                            size_t                     _listener_capacity,
                            size_t                     _bucket_count,
                            size_t                     _queue_capacity,
                            size_t                     _queue_bytes);
#endif

// d_event_dispatcher_dispose
//   function: releases the queue and the registry.
void d_event_dispatcher_dispose(struct d_event_dispatcher* _dispatcher);


// ---- subscription ----

// d_event_dispatcher_bind
//   function: binds a handler for `_key`. Forwards to the registry.
int d_event_dispatcher_bind(struct d_event_dispatcher* _dispatcher,
                            d_event_key                _key,
                            struct d_event_step        _step,
                            fn_free                    _state_free,
                            d_handler_id*              _id_out);

// d_event_dispatcher_unbind
//   function: removes the handler named by `_id`.
int d_event_dispatcher_unbind(struct d_event_dispatcher* _dispatcher,
                              d_handler_id               _id);

// d_event_dispatcher_enable / _disable
//   function: flips the mask for `_id`. Result convention as for
// d_event_table_enable.
int d_event_dispatcher_enable(struct d_event_dispatcher* _dispatcher,
                              d_handler_id               _id);
int d_event_dispatcher_disable(struct d_event_dispatcher* _dispatcher,
                               d_handler_id               _id);

// d_event_dispatcher_is_enabled
//   function: true if the handler exists and its mask is on.
bool d_event_dispatcher_is_enabled(
    const struct d_event_dispatcher* _dispatcher,
    d_handler_id                     _id);

// d_event_dispatcher_contains
//   function: true if a handler with `_id` is bound.
bool d_event_dispatcher_contains(
    const struct d_event_dispatcher* _dispatcher,
    d_handler_id                     _id);


// ---- delivery ----

// d_event_dispatcher_fire
//   function: dispatches one occurrence immediately against the current
// registry.
//   NOTE the changed contract: the pre-core d_event_handler_fire_event
// returned ssize_t -- 1 for "a listener ran", 0 for "none or disabled", -1
// for a null argument -- which conflated a formal outcome with a mechanical
// one. It now returns the enriched (count, verdict) result, and a null
// argument yields d_dispatch_result_empty(); callers needing to distinguish
// a bad argument check their own pointers, which they can do without
// dispatching.
struct d_dispatch_result d_event_dispatcher_fire(
    struct d_event_dispatcher* _dispatcher,
    d_event_key                _key,
    void*                      _payload);

#if (D_INTERNAL_EVENT_QUEUE == 1)

// d_event_dispatcher_queue
//   function: defers one occurrence, copying its payload.
int d_event_dispatcher_queue(struct d_event_dispatcher* _dispatcher,
                             d_event_key                _key,
                             struct d_event_payload     _payload);

// d_event_dispatcher_process
//   function: processes up to `_max` deferred occurrences; 0 means all
// pending at entry.
size_t d_event_dispatcher_process(struct d_event_dispatcher* _dispatcher,
                                  size_t                     _max,
                                  struct d_run_result*       _result_out);

// d_event_dispatcher_process_all
//   function: processes every occurrence pending at entry.
size_t d_event_dispatcher_process_all(struct d_event_dispatcher* _dispatcher,
                                      struct d_run_result*      _result_out);

#endif  // D_INTERNAL_EVENT_QUEUE

#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)

// d_event_dispatcher_run
//   function: folds dispatch over a homogeneous trace. Trace arguments as for
// d_event_registry_run.
struct d_run_result d_event_dispatcher_run(
    struct d_event_dispatcher* _dispatcher,
    d_event_key                _key,
    void*                      _first,
    size_t                     _stride,
    size_t                     _count);

#endif  // D_INTERNAL_EVENT_REGISTRY_RUN

#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)

// d_event_dispatcher_compile
//   function: stages the static word for `_key` into a fused word.
int d_event_dispatcher_compile(
    const struct d_event_dispatcher* _dispatcher,
    d_event_key                      _key,
    struct d_event_word*             _word_out);

#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING

#if (D_INTERNAL_EVENT_DISPATCHER_MERGE == 1)

// d_event_dispatcher_merge
//   function: merges another dispatcher's registry into this one. The other's
// QUEUE is not merged: a pending occurrence belongs to the dispatcher that
// accepted it, and moving one would change when it is delivered.
int d_event_dispatcher_merge(struct d_event_dispatcher*       _dispatcher,
                             const struct d_event_dispatcher* _other,
                             size_t*                          _merged_out);


#endif  // D_INTERNAL_EVENT_DISPATCHER_MERGE


// ---- queries ----

// d_event_dispatcher_handler_count
//   function: total bound handlers.
size_t d_event_dispatcher_handler_count(
    const struct d_event_dispatcher* _dispatcher);

// d_event_dispatcher_enabled_count
//   function: bound handlers whose mask is on.
size_t d_event_dispatcher_enabled_count(
    const struct d_event_dispatcher* _dispatcher);

// d_event_dispatcher_handler_count_for
//   function: the length of the word for one key.
size_t d_event_dispatcher_handler_count_for(
    const struct d_event_dispatcher* _dispatcher,
    d_event_key                      _key);

#if (D_INTERNAL_EVENT_QUEUE == 1)
// d_event_dispatcher_pending
//   function: occurrences awaiting processing.
size_t d_event_dispatcher_pending(
    const struct d_event_dispatcher* _dispatcher);
#endif

// d_event_dispatcher_footprint
//   function: total byte footprint including owned storage.
size_t d_event_dispatcher_footprint(
    const struct d_event_dispatcher* _dispatcher);


///////////////////////////////////////////////////////////////////////////////
///        VI.   LAYOUT ASSERTIONS                                          ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_ASSERT_SIZES == 1)

#if (D_INTERNAL_EVENT_QUEUE == 1)
    D_STATIC_ASSERT(sizeof(struct d_event_slot) == 32,
                    "d_event_slot layout drift");
    D_STATIC_ASSERT(sizeof(struct d_event_queue) == 72,
                    "d_event_queue layout drift");
    D_STATIC_ASSERT(sizeof(struct d_event_dispatcher) == 152,
                    "d_event_dispatcher layout drift");
    D_STATIC_ASSERT(sizeof(struct d_event_dispatcher) ==
                    ( sizeof(struct d_event_registry) +
                      sizeof(struct d_event_queue) ),
                    "the dispatcher must add no state of its own");
#else
    D_STATIC_ASSERT(sizeof(struct d_event_dispatcher) ==
                    sizeof(struct d_event_registry),
                    "without deferral the dispatcher is exactly a registry");
#endif

#endif  // D_INTERNAL_EVENT_ASSERT_SIZES

#if (D_INTERNAL_EVENT_ASSERT_LAYOUT == 1)

#if (D_INTERNAL_EVENT_QUEUE == 1)
D_STATIC_ASSERT(offsetof(struct d_event_slot, key) == 0,
                "d_event_slot field drift");
D_STATIC_ASSERT(offsetof(struct d_event_queue, bytes) == 0,
                "d_event_queue field drift");
#endif
D_STATIC_ASSERT(offsetof(struct d_event_dispatcher, registry) == 0,
                "d_event_dispatcher field drift");

#endif  // D_INTERNAL_EVENT_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_EVENT_EVENT_DISPATCHER_COMMON_H
