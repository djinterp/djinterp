/*******************************************************************************
* djinterp [c]                                         event_dispatcher_common.c
*
* The queue and the facade -- tier 0 definitions.
*   Implements event_dispatcher_common.h. The queue is two rings: slots, and
* the payload bytes they name by offset. A payload is never split across the
* end of the byte ring -- a handler receives one contiguous block -- so when
* it does not fit before the end it goes to the start and the gap it skips is
* charged to it until it is released. Payloads are placed on
* D_INTERNAL_EVENT_QUEUE_ALIGN boundaries relative to the arena, which an
* allocating queue gets from malloc and a fixed queue from its caller.
*   The occurrence being dispatched stays in both rings until its handlers
* return, so a re-entrant enqueue can never overwrite it: that is the whole
* of the re-entrancy guarantee, plus two refusals while processing -- a
* nested process, and discarding the occurrence in flight.
*
* path:      /src/djinterp/c/event/event_dispatcher_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/event/event_dispatcher_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memset
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, SIZE_MAX


#if (D_INTERNAL_EVENT_QUEUE == 1)

// D_INTERNAL_EVENT_QUEUE_ALIGN
//   constant: the boundary every payload starts on, relative to the arena. A
// power of two no smaller than the strictest fundamental alignment of the
// supported ABIs (C99 has no max_align_t to ask).
#define D_INTERNAL_EVENT_QUEUE_ALIGN ((size_t)16u)

// D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING
//   constant: set while process runs; unassigned in the public flag space.
#define D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING ((uint32_t)0x80000000u)


/*
d_internal_event_queue_padded
  The bytes a payload occupies in the ring: its size rounded up to the
alignment. Returns 0 for a size so large that rounding would overflow, which
the caller refuses.
*/
static size_t
d_internal_event_queue_padded(
    size_t _size
)
{
    // rounding up would wrap
    if (_size > (SIZE_MAX - (D_INTERNAL_EVENT_QUEUE_ALIGN - 1u)))
    {
        return 0u;
    }

    return (_size + (D_INTERNAL_EVENT_QUEUE_ALIGN - 1u)) &
           ~(D_INTERNAL_EVENT_QUEUE_ALIGN - 1u);
}

/*
d_internal_event_queue_place
  Finds room for _padded bytes. The live bytes run from bytes_head for
bytes_used bytes, possibly wrapping; free space is after them up to the end
and before bytes_head. Placing at the start charges the skipped gap at the end
too, so that releasing in order always returns exactly what was charged.
*/
static bool
d_internal_event_queue_place(
    const struct d_event_queue* _queue,
    size_t                      _padded,
    size_t*                     _offset_out,
    size_t*                     _charge_out
)
{
    const size_t end = _queue->bytes_head + _queue->bytes_used;

    // an empty ring is laid out from the start
    if (_queue->bytes_used == 0u)
    {
        *_offset_out = 0u;
        *_charge_out = _padded;

        return (_padded <= _queue->bytes_size);
    }

    // unwrapped: room after the live bytes, else before them
    if (end <= _queue->bytes_size)
    {
        if (_padded <= (_queue->bytes_size - end))
        {
            *_offset_out = end;
            *_charge_out = _padded;

            return true;
        }

        if (_padded <= _queue->bytes_head)
        {
            *_offset_out = 0u;
            *_charge_out = (_queue->bytes_size - end) + _padded;

            return true;
        }

        return false;
    }

    // wrapped: the only room is between the wrapped tail and bytes_head
    if (_padded <= (_queue->bytes_head - (end - _queue->bytes_size)))
    {
        *_offset_out = end - _queue->bytes_size;
        *_charge_out = _padded;

        return true;
    }

    return false;
}

/*
d_internal_event_queue_release_head
  Pops the head slot and returns its bytes -- everything from bytes_head to
the end of its payload, which includes any gap charged to it. An empty
payload holds no bytes and returns none.
*/
static void
d_internal_event_queue_release_head(
    struct d_event_queue* _queue
)
{
    const struct d_event_slot* const slot = &_queue->slots[_queue->head];

    // return the payload's bytes and any gap charged ahead of them
    if (slot->size > 0u)
    {
        const size_t stop     = slot->offset +
                                d_internal_event_queue_padded(slot->size);
        const size_t returned = (stop >= _queue->bytes_head)
            ? (stop - _queue->bytes_head)
            : ((_queue->bytes_size - _queue->bytes_head) + stop);

        _queue->bytes_used -= returned;
        _queue->bytes_head  = (stop == _queue->bytes_size) ? 0u : stop;

        if (_queue->bytes_used == 0u)
        {
            _queue->bytes_head = 0u;
        }
    }

    _queue->head = (_queue->head + 1u) % _queue->capacity;
    --_queue->count;

    // an empty queue restarts both rings at the beginning
    if (_queue->count == 0u)
    {
        _queue->head       = 0u;
        _queue->bytes_head = 0u;
        _queue->bytes_used = 0u;
    }

    return;
}

int
d_event_queue_init_fixed(
    struct d_event_queue* _queue,
    struct d_event_slot*  _slots,
    size_t                _capacity,
    unsigned char*        _bytes,
    size_t                _bytes_size
)
{
    // the queue, and any storage it is given room in, are required
    if ( (!_queue)                            ||
         ( (!_slots) && (_capacity > 0u) )    ||
         ( (!_bytes) && (_bytes_size > 0u) ) )
    {
        return D_EVENT_ERR_NULL;
    }

    memset(_queue, 0, sizeof(*_queue));
    _queue->slots      = _slots;
    _queue->capacity   = _capacity;
    _queue->bytes      = _bytes;
    _queue->bytes_size = _bytes_size;
    _queue->flags      = D_EVENT_QUEUE_FLAG_FIXED;

    return D_EVENT_OK;
}

#if (D_INTERNAL_EVENT_QUEUE_ALLOC == 1)

int
d_event_queue_init(
    struct d_event_queue* _queue,
    size_t                _capacity,
    size_t                _bytes_size
)
{
    // the queue is required
    if (!_queue)
    {
        return D_EVENT_ERR_NULL;
    }

    memset(_queue, 0, sizeof(*_queue));

    const size_t capacity   = (_capacity != 0u)
                                  ? _capacity
                                  : D_EVENT_QUEUE_DEFAULT_CAPACITY;
    const size_t bytes_size = (_bytes_size != 0u)
                                  ? _bytes_size
                                  : D_EVENT_QUEUE_DEFAULT_BYTES;

    // the slot ring's size would overflow
    if (capacity > (SIZE_MAX / sizeof(struct d_event_slot)))
    {
        return D_EVENT_ERR_ALLOC;
    }

    struct d_event_slot* slots = malloc(capacity * sizeof(struct d_event_slot));
    unsigned char*       bytes = malloc(bytes_size);

    // all or nothing
    if ( (!slots) ||
         (!bytes) )
    {
        free(slots);
        free(bytes);

        return D_EVENT_ERR_ALLOC;
    }

    _queue->slots      = slots;
    _queue->capacity   = capacity;
    _queue->bytes      = bytes;
    _queue->bytes_size = bytes_size;
    _queue->flags      = D_EVENT_QUEUE_FLAG_OWNS_STORAGE;

    return D_EVENT_OK;
}

#endif  // D_INTERNAL_EVENT_QUEUE_ALLOC

/*
d_event_queue_dispose
  Refused while the queue is processing: the occurrence in flight lives in
the arena being freed. A handler that must drop the queue clears it instead.
*/
void
d_event_queue_dispose(
    struct d_event_queue* _queue
)
{
    // nothing to release, or a handler of this queue asking
    if ( (!_queue) ||
         ((_queue->flags & D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING) != 0u) )
    {
        return;
    }

    // free what the queue allocated itself
    if ((_queue->flags & D_EVENT_QUEUE_FLAG_OWNS_STORAGE) != 0u)
    {
        free(_queue->slots);
        free(_queue->bytes);
    }

    memset(_queue, 0, sizeof(*_queue));

    return;
}

/*
d_event_queue_clear
  While processing, the occurrence in flight is kept -- its handlers are
still reading its payload -- and everything behind it is discarded, so the
running process stops after it.
*/
void
d_event_queue_clear(
    struct d_event_queue* _queue
)
{
    // nothing queued
    if ( (!_queue) ||
         (_queue->count == 0u) )
    {
        return;
    }

    // keep only the occurrence in flight and the bytes charged to it
    if ((_queue->flags & D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING) != 0u)
    {
        const struct d_event_slot* const slot = &_queue->slots[_queue->head];

        _queue->count = 1u;

        if (slot->size > 0u)
        {
            const size_t stop = slot->offset +
                                d_internal_event_queue_padded(slot->size);

            _queue->bytes_used = (stop >= _queue->bytes_head)
                ? (stop - _queue->bytes_head)
                : ((_queue->bytes_size - _queue->bytes_head) + stop);
        }
        else
        {
            _queue->bytes_used = 0u;
        }

        return;
    }

    _queue->head       = 0u;
    _queue->count      = 0u;
    _queue->bytes_head = 0u;
    _queue->bytes_used = 0u;

    return;
}

/*
d_event_queue_enqueue
  All room is found before anything is written, so a refusal leaves the
queue untouched. Under the drop-oldest policy the oldest occurrences are
released unprocessed to make room -- but never while processing, when the
oldest is the one in flight.
*/
int
d_event_queue_enqueue(
    struct d_event_queue*  _queue,
    d_event_key            _key,
    struct d_event_payload _payload
)
{
    // the queue is required, and so are the bytes of a non-empty payload
    if ( (!_queue) ||
         ( (_payload.size > 0u) && (!_payload.data) ) )
    {
        return D_EVENT_ERR_NULL;
    }

    const size_t padded = (_payload.size > 0u)
                              ? d_internal_event_queue_padded(_payload.size)
                              : 0u;
    size_t       offset = 0u;
    size_t       charge = 0u;

    // a payload too large to pad can never fit
    if ( (_payload.size > 0u) &&
         (padded == 0u) )
    {
        return D_EVENT_ERR_CAPACITY;
    }

    // find a free slot and, for a non-empty payload, room for its bytes
    for (;;)
    {
        const bool slot_free = (_queue->count < _queue->capacity);
        const bool room      = (padded == 0u) ||
                               d_internal_event_queue_place(_queue,
                                                            padded,
                                                            &offset,
                                                            &charge);

        if ( (slot_free) &&
             (room) )
        {
            break;
        }

#if (D_INTERNAL_EVENT_QUEUE_OVERFLOW == D_EVENT_QUEUE_OVERFLOW_DROP_OLDEST)
        // drop the oldest to make room, unless it is in flight
        if ( (_queue->count > 0u) &&
             ((_queue->flags & D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING) == 0u) )
        {
            d_internal_event_queue_release_head(_queue);

            continue;
        }
#endif  // D_INTERNAL_EVENT_QUEUE_OVERFLOW

        return D_EVENT_ERR_CAPACITY;
    }

    struct d_event_slot* const slot =
        &_queue->slots[(_queue->head + _queue->count) % _queue->capacity];

    slot->key    = _key;
    slot->offset = offset;
    slot->size   = _payload.size;
    slot->arity  = _payload.arity;
    slot->flags  = 0u;

    // copy the payload by value: deferral outlives the caller's frame
    if (_payload.size > 0u)
    {
        memcpy(_queue->bytes + offset, _payload.data, _payload.size);
        _queue->bytes_used += charge;
    }

    ++_queue->count;

    return D_EVENT_OK;
}

/*
d_event_queue_process
  The prefix is fixed at entry, so an occurrence a handler enqueues waits for
a later call. Each occurrence is released only after its dispatch returns,
and only if a handler has not already cleared it away. A nested process is
refused, because it would dispatch the occurrence in flight a second time.
*/
size_t
d_event_queue_process(
    struct d_event_queue*    _queue,
    struct d_event_registry* _registry,
    size_t                   _max,
    struct d_run_result*     _result_out
)
{
    struct d_run_result run;
    size_t              processed = 0u;

    run.occurrences      = 0u;
    run.handlers_invoked = 0u;
    run.consumed_count   = 0u;

    if (_result_out)
    {
        *_result_out = run;
    }

    // both are required, and a handler of this queue may not re-enter it
    if ( (!_queue)    ||
         (!_registry) ||
         ((_queue->flags & D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING) != 0u) )
    {
        return 0u;
    }

    const size_t prefix = ( (_max == 0u) || (_max > _queue->count) )
                              ? _queue->count
                              : _max;

    _queue->flags |= D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING;

    // dispatch the prefix in order, releasing each occurrence after it runs
    while ( (processed < prefix) &&
            (_queue->count > 0u) )
    {
        const struct d_event_slot slot    = _queue->slots[_queue->head];
        void* const               payload = (slot.size > 0u)
                                                ? (void*)(_queue->bytes +
                                                          slot.offset)
                                                : NULL;
        const struct d_dispatch_result result =
            d_event_registry_dispatch(_registry, slot.key, payload);

        ++processed;
        ++run.occurrences;
        run.handlers_invoked += result.invoked;

        if (D_VERDICT_IS_CONSUMED(result.outcome))
        {
            ++run.consumed_count;
        }

        // release it, unless a handler cleared the queue out from under it
        if (_queue->count > 0u)
        {
            d_internal_event_queue_release_head(_queue);
        }
    }

    _queue->flags &= ~D_INTERNAL_EVENT_QUEUE_FLAG_PROCESSING;

    if (_result_out)
    {
        *_result_out = run;
    }

    return processed;
}

size_t
d_event_queue_process_all(
    struct d_event_queue*    _queue,
    struct d_event_registry* _registry,
    struct d_run_result*     _result_out
)
{
    return d_event_queue_process(_queue, _registry, 0u, _result_out);
}

const struct d_event_slot*
d_event_queue_peek(
    const struct d_event_queue* _queue
)
{
    // nothing queued
    if ( (!_queue) ||
         (_queue->count == 0u) )
    {
        return NULL;
    }

    return &_queue->slots[_queue->head];
}

struct d_event_payload
d_event_queue_payload(
    const struct d_event_queue* _queue,
    const struct d_event_slot*  _slot
)
{
    // no slot, or an empty payload: the empty view
    if ( (!_queue) ||
         (!_slot)  ||
         (_slot->size == 0u) )
    {
        return d_event_payload_make(NULL,
                                    0u,
                                    (_slot != NULL) ? _slot->arity : 0u);
    }

    return d_event_payload_make((void*)(_queue->bytes + _slot->offset),
                                _slot->size,
                                _slot->arity);
}

size_t
d_event_queue_footprint(
    const struct d_event_queue* _queue
)
{
    // no queue, no bytes
    if (!_queue)
    {
        return 0u;
    }

    size_t bytes = sizeof(struct d_event_queue);

    // owned storage counts; borrowed storage is the caller's
    if ((_queue->flags & D_EVENT_QUEUE_FLAG_OWNS_STORAGE) != 0u)
    {
        bytes += _queue->capacity * sizeof(struct d_event_slot);
        bytes += _queue->bytes_size;
    }

    return bytes;
}

#endif  // D_INTERNAL_EVENT_QUEUE


#if (D_INTERNAL_EVENT_QUEUE == 1)

int
d_event_dispatcher_init_fixed(
    struct d_event_dispatcher* _dispatcher,
    struct d_event_entry*      _entries,
    size_t                     _capacity,
    uint32_t*                  _bucket_head,
    uint32_t*                  _bucket_tail,
    size_t                     _bucket_count,
    struct d_event_slot*       _slots,
    size_t                     _slot_capacity,
    unsigned char*             _bytes,
    size_t                     _bytes_size
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    int status = d_event_registry_init_fixed(&_dispatcher->registry,
                                             _entries,
                                             _capacity,
                                             _bucket_head,
                                             _bucket_tail,
                                             _bucket_count);

    if (status != D_EVENT_OK)
    {
        return status;
    }

    status = d_event_queue_init_fixed(&_dispatcher->queue,
                                      _slots,
                                      _slot_capacity,
                                      _bytes,
                                      _bytes_size);

    // leave nothing half-initialized
    if (status != D_EVENT_OK)
    {
        d_event_registry_dispose(&_dispatcher->registry);
    }

    return status;
}

#else

int
d_event_dispatcher_init_fixed(
    struct d_event_dispatcher* _dispatcher,
    struct d_event_entry*      _entries,
    size_t                     _capacity,
    uint32_t*                  _bucket_head,
    uint32_t*                  _bucket_tail,
    size_t                     _bucket_count
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_registry_init_fixed(&_dispatcher->registry,
                                       _entries,
                                       _capacity,
                                       _bucket_head,
                                       _bucket_tail,
                                       _bucket_count);
}

#endif  // D_INTERNAL_EVENT_QUEUE

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

/*
d_event_dispatcher_init
  With deferral compiled in but queue allocation compiled out there is no
way to give the queue storage, so that configuration reports a state error
and leaves nothing allocated.
*/
int
d_event_dispatcher_init(
    struct d_event_dispatcher* _dispatcher,
    size_t                     _listener_capacity,
    size_t                     _bucket_count,
    size_t                     _queue_capacity,
    size_t                     _queue_bytes
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    int status = d_event_registry_init(&_dispatcher->registry,
                                       _listener_capacity,
                                       _bucket_count);

    if (status != D_EVENT_OK)
    {
        return status;
    }

#if (D_INTERNAL_EVENT_QUEUE == 1)
#if (D_INTERNAL_EVENT_QUEUE_ALLOC == 1)
    status = d_event_queue_init(&_dispatcher->queue,
                                _queue_capacity,
                                _queue_bytes);
#else
    (void)_queue_capacity;
    (void)_queue_bytes;
    memset(&_dispatcher->queue, 0, sizeof(_dispatcher->queue));
    status = D_EVENT_ERR_STATE;
#endif  // D_INTERNAL_EVENT_QUEUE_ALLOC

    // leave nothing half-initialized
    if (status != D_EVENT_OK)
    {
        d_event_registry_dispose(&_dispatcher->registry);
    }
#else
    (void)_queue_capacity;
    (void)_queue_bytes;
#endif  // D_INTERNAL_EVENT_QUEUE

    return status;
}

#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

void
d_event_dispatcher_dispose(
    struct d_event_dispatcher* _dispatcher
)
{
    // nothing to release
    if (!_dispatcher)
    {
        return;
    }

#if (D_INTERNAL_EVENT_QUEUE == 1)
    d_event_queue_dispose(&_dispatcher->queue);
#endif  // D_INTERNAL_EVENT_QUEUE
    d_event_registry_dispose(&_dispatcher->registry);

    return;
}

int
d_event_dispatcher_bind(
    struct d_event_dispatcher* _dispatcher,
    d_event_key                _key,
    struct d_event_step        _step,
    fn_free                    _state_free,
    d_handler_id*              _id_out
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_registry_bind(&_dispatcher->registry,
                                 _key,
                                 _step,
                                 _state_free,
                                 _id_out);
}

int
d_event_dispatcher_unbind(
    struct d_event_dispatcher* _dispatcher,
    d_handler_id               _id
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_registry_unbind(&_dispatcher->registry, _id);
}

int
d_event_dispatcher_enable(
    struct d_event_dispatcher* _dispatcher,
    d_handler_id               _id
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_table_enable(&_dispatcher->registry.table, _id);
}

int
d_event_dispatcher_disable(
    struct d_event_dispatcher* _dispatcher,
    d_handler_id               _id
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_table_disable(&_dispatcher->registry.table, _id);
}

bool
d_event_dispatcher_is_enabled(
    const struct d_event_dispatcher* _dispatcher,
    d_handler_id                     _id
)
{
    return ( (_dispatcher != NULL) &&
             (d_event_table_is_enabled(&_dispatcher->registry.table, _id)) );
}

bool
d_event_dispatcher_contains(
    const struct d_event_dispatcher* _dispatcher,
    d_handler_id                     _id
)
{
    return ( (_dispatcher != NULL) &&
             (d_event_table_contains(&_dispatcher->registry.table, _id)) );
}

struct d_dispatch_result
d_event_dispatcher_fire(
    struct d_event_dispatcher* _dispatcher,
    d_event_key                _key,
    void*                      _payload
)
{
    // no dispatcher: the empty word's result
    if (!_dispatcher)
    {
        return d_dispatch_result_empty();
    }

    return d_event_registry_dispatch(&_dispatcher->registry, _key, _payload);
}

#if (D_INTERNAL_EVENT_QUEUE == 1)

int
d_event_dispatcher_queue(
    struct d_event_dispatcher* _dispatcher,
    d_event_key                _key,
    struct d_event_payload     _payload
)
{
    // the dispatcher is required
    if (!_dispatcher)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_queue_enqueue(&_dispatcher->queue, _key, _payload);
}

size_t
d_event_dispatcher_process(
    struct d_event_dispatcher* _dispatcher,
    size_t                     _max,
    struct d_run_result*       _result_out
)
{
    // no dispatcher: nothing processed, and a zero result
    if (!_dispatcher)
    {
        return d_event_queue_process(NULL, NULL, _max, _result_out);
    }

    return d_event_queue_process(&_dispatcher->queue,
                                 &_dispatcher->registry,
                                 _max,
                                 _result_out);
}

size_t
d_event_dispatcher_process_all(
    struct d_event_dispatcher* _dispatcher,
    struct d_run_result*       _result_out
)
{
    return d_event_dispatcher_process(_dispatcher, 0u, _result_out);
}

#endif  // D_INTERNAL_EVENT_QUEUE

#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)

struct d_run_result
d_event_dispatcher_run(
    struct d_event_dispatcher* _dispatcher,
    d_event_key                _key,
    void*                      _first,
    size_t                     _stride,
    size_t                     _count
)
{
    return d_event_registry_run((_dispatcher != NULL)
                                    ? &_dispatcher->registry
                                    : NULL,
                                _key,
                                _first,
                                _stride,
                                _count);
}

#endif  // D_INTERNAL_EVENT_REGISTRY_RUN

#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)

/*
d_event_dispatcher_compile
  The facade has no parameter for caller storage, so it stages through the
allocating compile; a build without staging allocation reports a state error
and leaves the word zeroed.
*/
int
d_event_dispatcher_compile(
    const struct d_event_dispatcher* _dispatcher,
    d_event_key                      _key,
    struct d_event_word*             _word_out
)
{
    // the dispatcher and the word are required
    if ( (!_dispatcher) ||
         (!_word_out) )
    {
        return D_EVENT_ERR_NULL;
    }

#if (D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC == 1)
    return d_event_registry_compile(&_dispatcher->registry, _key, _word_out);
#else
    (void)_key;
    memset(_word_out, 0, sizeof(*_word_out));

    return D_EVENT_ERR_STATE;
#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC
}

#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING

#if (D_INTERNAL_EVENT_DISPATCHER_MERGE == 1)

int
d_event_dispatcher_merge(
    struct d_event_dispatcher*       _dispatcher,
    const struct d_event_dispatcher* _other,
    size_t*                          _merged_out
)
{
    // both dispatchers are required
    if ( (!_dispatcher) ||
         (!_other) )
    {
        if (_merged_out)
        {
            *_merged_out = 0u;
        }

        return D_EVENT_ERR_NULL;
    }

    return d_event_table_merge(&_dispatcher->registry.table,
                               &_other->registry.table,
                               _merged_out);
}

#endif  // D_INTERNAL_EVENT_DISPATCHER_MERGE

size_t
d_event_dispatcher_handler_count(
    const struct d_event_dispatcher* _dispatcher
)
{
    return (_dispatcher != NULL)
        ? d_event_table_count(&_dispatcher->registry.table)
        : 0u;
}

size_t
d_event_dispatcher_enabled_count(
    const struct d_event_dispatcher* _dispatcher
)
{
    return (_dispatcher != NULL)
        ? d_event_table_enabled_count(&_dispatcher->registry.table)
        : 0u;
}

size_t
d_event_dispatcher_handler_count_for(
    const struct d_event_dispatcher* _dispatcher,
    d_event_key                      _key
)
{
    return (_dispatcher != NULL)
        ? d_event_table_count_for(&_dispatcher->registry.table, _key)
        : 0u;
}

#if (D_INTERNAL_EVENT_QUEUE == 1)

size_t
d_event_dispatcher_pending(
    const struct d_event_dispatcher* _dispatcher
)
{
    return (_dispatcher != NULL)
        ? d_event_queue_pending(&_dispatcher->queue)
        : 0u;
}

#endif  // D_INTERNAL_EVENT_QUEUE

/*
d_event_dispatcher_footprint
  The members' own sizes are already inside sizeof the dispatcher, so only
their owned storage is added.
*/
size_t
d_event_dispatcher_footprint(
    const struct d_event_dispatcher* _dispatcher
)
{
    // no dispatcher, no bytes
    if (!_dispatcher)
    {
        return 0u;
    }

    size_t bytes = sizeof(struct d_event_dispatcher);

    bytes += d_event_table_footprint(&_dispatcher->registry.table) -
             sizeof(struct d_event_table);
#if (D_INTERNAL_EVENT_QUEUE == 1)
    bytes += d_event_queue_footprint(&_dispatcher->queue) -
             sizeof(struct d_event_queue);
#endif  // D_INTERNAL_EVENT_QUEUE

    return bytes;
}
