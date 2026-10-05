/*******************************************************************************
* djinterp [c]                                              event_table_common.c
*
* The erased store -- tier 0 definitions.
*   Implements event_table_common.h: an entry arena chained by 32-bit index,
* a free list threaded through the `next` field of free slots, and bucket
* chains that are only ever appended to, so that the entries of one key stay
* in bind order -- the word order.
*   Also implements the private dispatch bracket of event_table_internal.h:
* while a dispatch runs, unbind turns a letter into a zombie instead of
* releasing it, and the outermost dispatch reaps the zombies on its way out.
*
* path:      /src/djinterp/c/event/event_table_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/event/event_table_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, realloc, free
#include <string.h>   // memset
// djinterp
#include "./event_table_internal.h"  // the dispatch bracket and its markers
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t,
                                                     // SIZE_MAX


// D_INTERNAL_EVENT_TABLE_MAX_ENTRIES
//   constant: the most entries a table can index; D_EVENT_INDEX_NONE itself
// is the null index, so it cannot name a slot.
#define D_INTERNAL_EVENT_TABLE_MAX_ENTRIES ((size_t)D_EVENT_INDEX_NONE)


/*
d_internal_event_entry_live
  A live entry is bound and not a zombie. Every query and count goes through
this test, so a zombie is invisible everywhere except to the dispatch that
must still fold it.
*/
static bool
d_internal_event_entry_live(
    const struct d_event_entry* _entry
)
{
    return ( (_entry->id != D_HANDLER_ID_NULL) &&
             ((_entry->enabled & D_INTERNAL_EVENT_ENTRY_ZOMBIE) == 0u) );
}

/*
d_internal_event_table_ready
  A table with no bucket arrays was never initialized (or was disposed);
every operation treats it as empty.
*/
static bool
d_internal_event_table_ready(
    const struct d_event_table* _table
)
{
    return ( (_table != NULL)               &&
             (_table->entries != NULL)      &&
             (_table->bucket_head != NULL)  &&
             (_table->bucket_tail != NULL)  &&
             (_table->bucket_count != 0u) );
}

/*
d_internal_event_table_index_of
  Handler ids are not hashed, so finding one is a scan of the arena up to its
high-water mark. Unbind and the mask are not on the dispatch path, which is
what keeps this acceptable.
*/
static uint32_t
d_internal_event_table_index_of(
    const struct d_event_table* _table,
    d_handler_id                _id
)
{
    // the null id names nothing
    if ( (_id == D_HANDLER_ID_NULL) ||
         (!_table->entries) )
    {
        return D_EVENT_INDEX_NONE;
    }

    // scan the slots ever used for the live entry holding _id
    for (uint32_t i = 0u; i < _table->used; ++i)
    {
        if ( (_table->entries[i].id == _id) &&
             (d_internal_event_entry_live(&_table->entries[i])) )
        {
            return i;
        }
    }

    return D_EVENT_INDEX_NONE;
}

/*
d_internal_event_table_append
  Links a slot at the tail of its key's bucket chain. Appending is the only
way a chain grows, which is what makes chain order bind order.
*/
static void
d_internal_event_table_append(
    struct d_event_table* _table,
    uint32_t              _index
)
{
    const size_t bucket = d_event_key_hash(_table->entries[_index].key,
                                           _table->bucket_count);

    _table->entries[_index].next = D_EVENT_INDEX_NONE;

    // an empty chain starts at this slot; otherwise it follows the tail
    if (_table->bucket_tail[bucket] == D_EVENT_INDEX_NONE)
    {
        _table->bucket_head[bucket] = _index;
    }
    else
    {
        _table->entries[_table->bucket_tail[bucket]].next = _index;
    }

    _table->bucket_tail[bucket] = _index;

    return;
}

/*
d_internal_event_table_detach
  Unlinks a slot from its chain, clears it onto the free list, and hands back
its state and hook without calling the hook: the caller calls it last, after
the table is consistent, because a hook may re-enter the table.
*/
static void
d_internal_event_table_detach(
    struct d_event_table* _table,
    uint32_t              _index,
    void**                _state_out,
    fn_free*              _free_out
)
{
    struct d_event_entry* const entry  = &_table->entries[_index];
    const size_t                bucket = d_event_key_hash(entry->key,
                                                          _table->bucket_count);
    uint32_t                    prev   = D_EVENT_INDEX_NONE;
    uint32_t                    cur    = _table->bucket_head[bucket];

    *_state_out = entry->step.state;
    *_free_out  = entry->state_free;

    // find the slot and its predecessor in the chain
    while ( (cur != D_EVENT_INDEX_NONE) &&
            (cur != _index) )
    {
        prev = cur;
        cur  = _table->entries[cur].next;
    }

    // unlink it, repairing the head or the tail when it held either
    if (cur == _index)
    {
        if (prev == D_EVENT_INDEX_NONE)
        {
            _table->bucket_head[bucket] = entry->next;
        }
        else
        {
            _table->entries[prev].next = entry->next;
        }

        if (_table->bucket_tail[bucket] == _index)
        {
            _table->bucket_tail[bucket] = prev;
        }
    }

    memset(entry, 0, sizeof(*entry));
    entry->next       = _table->free_head;
    _table->free_head = _index;

    return;
}

/*
d_internal_event_table_drop
  Removes one live entry. Outside a dispatch the slot is released at once and
its hook runs last; during one the entry becomes a zombie, leaving the chain
the running fold is walking unchanged and the state alive until the reap.
Either way the counts change now, because the letter is unbound now.
*/
static void
d_internal_event_table_drop(
    struct d_event_table* _table,
    uint32_t              _index
)
{
    struct d_event_entry* const entry = &_table->entries[_index];
    void*                       state = NULL;
    fn_free                     hook  = NULL;

    --_table->count;

    // the mask counts only while the letter is bound
    if ((entry->enabled & D_INTERNAL_EVENT_ENTRY_ON) != 0u)
    {
        --_table->enabled_count;
    }

    // mid-dispatch: defer the release to the outermost dispatch's end
    if ((_table->flags & D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING) != 0u)
    {
        entry->enabled = (uint8_t)(entry->enabled |
                                   D_INTERNAL_EVENT_ENTRY_ZOMBIE);
        _table->flags |= D_INTERNAL_EVENT_TABLE_FLAG_ZOMBIES;

        return;
    }

    d_internal_event_table_detach(_table, _index, &state, &hook);

    // the hook runs last: it may re-enter the table
    if (hook)
    {
        hook(state);
    }

    return;
}

/*
d_internal_event_table_find_zombie
  The reap restarts its search after every hook, because a hook may bind or
unbind and so restructure the chains under a cursor; a restart costs a scan,
and zombies are rare.
*/
static uint32_t
d_internal_event_table_find_zombie(
    const struct d_event_table* _table
)
{
    // scan every chain for an entry still marked as a zombie
    for (size_t bucket = 0u; bucket < _table->bucket_count; ++bucket)
    {
        uint32_t cur = _table->bucket_head[bucket];

        while (cur != D_EVENT_INDEX_NONE)
        {
            if ((_table->entries[cur].enabled &
                 D_INTERNAL_EVENT_ENTRY_ZOMBIE) != 0u)
            {
                return cur;
            }

            cur = _table->entries[cur].next;
        }
    }

    return D_EVENT_INDEX_NONE;
}

/*
d_internal_event_table_monotone
  True when the live entries' ids rise with their slot index, which holds
until an unbind frees a slot that a later bind reuses. When it holds, bind
order is slot order and an ordered walk is a single pass.
*/
static bool
d_internal_event_table_monotone(
    const struct d_event_table* _table
)
{
    d_handler_id last = D_HANDLER_ID_NULL;

    // any live id not above its predecessor breaks the order
    for (uint32_t i = 0u; i < _table->used; ++i)
    {
        if (!d_internal_event_entry_live(&_table->entries[i]))
        {
            continue;
        }

        if (_table->entries[i].id <= last)
        {
            return false;
        }

        last = _table->entries[i].id;
    }

    return true;
}

/*
d_internal_event_table_next_in_order
  The next live entry in bind order: the one with the smallest id in
(_last, _bound]. In a monotone arena that is the next live slot at or after
*_cursor; otherwise it is found by a scan. Both forms re-read the arena on
every call and never visit an id above _bound, so a visitor or hook that
binds, unbinds or grows the table mid-walk can neither derail the walk nor
lengthen it.
*/
static uint32_t
d_internal_event_table_next_in_order(
    const struct d_event_table* _table,
    bool                        _monotone,
    uint32_t*                   _cursor,
    d_handler_id                _last,
    d_handler_id                _bound
)
{
    uint32_t     best    = D_EVENT_INDEX_NONE;
    d_handler_id best_id = D_HANDLER_ID_NULL;

    // monotone arena: the next qualifying slot is the answer
    if (_monotone)
    {
        for (uint32_t i = *_cursor; i < _table->used; ++i)
        {
            const struct d_event_entry* const entry = &_table->entries[i];

            if ( (d_internal_event_entry_live(entry)) &&
                 (entry->id > _last)                  &&
                 (entry->id <= _bound) )
            {
                *_cursor = i + 1u;

                return i;
            }
        }

        *_cursor = _table->used;

        return D_EVENT_INDEX_NONE;
    }

    // otherwise select the smallest qualifying id
    for (uint32_t i = 0u; i < _table->used; ++i)
    {
        const struct d_event_entry* const entry = &_table->entries[i];

        if ( (d_internal_event_entry_live(entry)) &&
             (entry->id > _last)                  &&
             (entry->id <= _bound)                &&
             ( (best == D_EVENT_INDEX_NONE) ||
               (entry->id < best_id) ) )
        {
            best    = i;
            best_id = entry->id;
        }
    }

    return best;
}

/*
d_internal_event_table_next_of_key
  The next live letter of one word in bind order after id _last, found by
walking the key's chain afresh: robust to any edit a visitor makes, and a
word is short.
*/
static uint32_t
d_internal_event_table_next_of_key(
    const struct d_event_table* _table,
    d_event_key                 _key,
    d_handler_id                _last,
    d_handler_id                _bound
)
{
    const size_t bucket = d_event_key_hash(_key, _table->bucket_count);
    uint32_t     best   = D_EVENT_INDEX_NONE;
    uint32_t     cur    = _table->bucket_head[bucket];

    // the smallest qualifying id of this key in the chain
    while (cur != D_EVENT_INDEX_NONE)
    {
        const struct d_event_entry* const entry = &_table->entries[cur];

        if ( (entry->key == _key)                 &&
             (d_internal_event_entry_live(entry)) &&
             (entry->id > _last)                  &&
             (entry->id <= _bound)                &&
             ( (best == D_EVENT_INDEX_NONE) ||
               (entry->id < _table->entries[best].id) ) )
        {
            best = cur;
        }

        cur = entry->next;
    }

    return best;
}

/*
d_internal_event_table_bind_at
  bind, also reporting the slot, which merge needs in order to carry the
mask across without a second lookup. Slots come from the free list first,
then from the high-water mark, then -- for an owning table -- from growing
the arena. The load check runs after the insert; a failed rehash leaves a
correct if denser table, so it does not fail the bind.
*/
static int
d_internal_event_table_bind_at(
    struct d_event_table* _table,
    d_event_key           _key,
    struct d_event_step   _step,
    fn_free               _state_free,
    d_handler_id*         _id_out,
    uint32_t*             _index_out
)
{
    uint32_t index = D_EVENT_INDEX_NONE;

    // an uninitialized or disposed table cannot take a letter
    if (!d_internal_event_table_ready(_table))
    {
        return D_EVENT_ERR_STATE;
    }

    // a freed slot first, then a fresh one
    if (_table->free_head != D_EVENT_INDEX_NONE)
    {
        index             = _table->free_head;
        _table->free_head = _table->entries[index].next;
    }
    else
    {
        // the arena is full: a fixed or borrowed table cannot grow
        if ((size_t)_table->used >= _table->capacity)
        {
            if ( ((_table->flags & D_EVENT_TABLE_FLAG_FIXED) != 0u)        ||
                 ((_table->flags & D_EVENT_TABLE_FLAG_OWNS_STORAGE) == 0u) ||
                 (_table->capacity >= D_INTERNAL_EVENT_TABLE_MAX_ENTRIES) )
            {
                return D_EVENT_ERR_CAPACITY;
            }

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)
            {
                const int status = d_event_table_reserve(
                    _table,
                    (_table->capacity > 0u) ? (_table->capacity * 2u) : 1u);

                if (status != D_EVENT_OK)
                {
                    return status;
                }
            }
#else
            return D_EVENT_ERR_CAPACITY;
#endif  // D_INTERNAL_EVENT_TABLE_ALLOC
        }

        index = _table->used;
        ++_table->used;
    }

    // a table whose counter was never started starts it now
    if (_table->next_id == D_HANDLER_ID_NULL)
    {
        _table->next_id = 1u;
    }

    {
        struct d_event_entry* const entry = &_table->entries[index];

        entry->id          = _table->next_id;
        entry->key         = _key;
        entry->step        = _step;
        entry->state_free  = _state_free;
        entry->enabled     = D_INTERNAL_EVENT_ENTRY_ON;
        entry->reserved[0] = 0u;
        entry->reserved[1] = 0u;
        entry->reserved[2] = 0u;
    }

    ++_table->next_id;
    d_internal_event_table_append(_table, index);
    ++_table->count;
    ++_table->enabled_count;

    if (_id_out)
    {
        *_id_out = _table->entries[index].id;
    }

    if (_index_out)
    {
        *_index_out = index;
    }

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)
    // past the load threshold an owning table rehashes; failure is harmless
    if ( ((_table->flags & D_EVENT_TABLE_FLAG_OWNS_STORAGE) != 0u) &&
         ( (_table->count * (size_t)D_EVENT_TABLE_LOAD_FACTOR_DEN) >
           (_table->bucket_count * (size_t)D_EVENT_TABLE_LOAD_FACTOR_NUM) ) )
    {
        (void)d_event_table_rehash(
            _table,
            _table->bucket_count * (size_t)D_EVENT_TABLE_GROWTH_FACTOR);
    }
#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

    return D_EVENT_OK;
}


/*
d_internal_event_table_dispatch_begin
  The mark is a single bit, not a depth: a nested dispatch finds it set and
leaves it set, and only the dispatch that set it clears it.
*/
bool
d_internal_event_table_dispatch_begin(
    struct d_event_table* _table
)
{
    const bool outermost =
        ((_table->flags & D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING) == 0u);

    _table->flags |= D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING;

    return outermost;
}

/*
d_internal_event_table_dispatch_end
  The mark is cleared before the reap so that an unbind a hook performs
releases immediately instead of making a new zombie.
*/
void
d_internal_event_table_dispatch_end(
    struct d_event_table* _table,
    bool                  _outermost
)
{
    // only the dispatch that set the mark ends it
    if (!_outermost)
    {
        return;
    }

    _table->flags &= ~D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING;

    // reap: release every zombie, restarting after each hook
    if ((_table->flags & D_INTERNAL_EVENT_TABLE_FLAG_ZOMBIES) != 0u)
    {
        _table->flags &= ~D_INTERNAL_EVENT_TABLE_FLAG_ZOMBIES;

        for (;;)
        {
            const uint32_t index = d_internal_event_table_find_zombie(_table);
            void*          state = NULL;
            fn_free        hook  = NULL;

            if (index == D_EVENT_INDEX_NONE)
            {
                break;
            }

            d_internal_event_table_detach(_table, index, &state, &hook);

            if (hook)
            {
                hook(state);
            }
        }
    }

    // a handler asked for the table to be disposed
    if ((_table->flags & D_INTERNAL_EVENT_TABLE_FLAG_DISPOSE) != 0u)
    {
        _table->flags &= ~D_INTERNAL_EVENT_TABLE_FLAG_DISPOSE;
        d_event_table_dispose(_table);
    }

    return;
}


/*
d_event_table_init_fixed
  A zero bucket count is refused as missing storage: there would be nowhere
to hash to. A capacity beyond the 32-bit index range is clamped to it, since
no chain could name the slots above it.
*/
int
d_event_table_init_fixed(
    struct d_event_table* _table,
    struct d_event_entry* _entries,
    size_t                _capacity,
    uint32_t*             _bucket_head,
    uint32_t*             _bucket_tail,
    size_t                _bucket_count
)
{
    // every piece of storage is required
    if ( (!_table)       ||
         (!_entries)     ||
         (!_bucket_head) ||
         (!_bucket_tail) ||
         (_bucket_count == 0u) )
    {
        return D_EVENT_ERR_NULL;
    }

    memset(_table, 0, sizeof(*_table));
    _table->entries      = _entries;
    _table->bucket_head  = _bucket_head;
    _table->bucket_tail  = _bucket_tail;
    _table->capacity     = (_capacity > D_INTERNAL_EVENT_TABLE_MAX_ENTRIES)
                               ? D_INTERNAL_EVENT_TABLE_MAX_ENTRIES
                               : _capacity;
    _table->bucket_count = _bucket_count;
    _table->next_id      = 1u;
    _table->free_head    = D_EVENT_INDEX_NONE;
    _table->flags        = D_EVENT_TABLE_FLAG_FIXED;

    // every chain starts empty
    for (size_t i = 0u; i < _bucket_count; ++i)
    {
        _bucket_head[i] = D_EVENT_INDEX_NONE;
        _bucket_tail[i] = D_EVENT_INDEX_NONE;
    }

    return D_EVENT_OK;
}

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

/*
d_event_table_init
  All three blocks are allocated up front, so a failure leaves the table
zeroed rather than half-built.
*/
int
d_event_table_init(
    struct d_event_table* _table,
    size_t                _capacity,
    size_t                _bucket_count
)
{
    // the table itself is required
    if (!_table)
    {
        return D_EVENT_ERR_NULL;
    }

    memset(_table, 0, sizeof(*_table));

    size_t capacity = (_capacity != 0u) ? _capacity
                                        : D_EVENT_TABLE_DEFAULT_CAPACITY;
    const size_t buckets = d_event_next_prime(
        (_bucket_count != 0u) ? _bucket_count : D_EVENT_TABLE_DEFAULT_BUCKETS);

    // clamp the arena to the index range
    if (capacity > D_INTERNAL_EVENT_TABLE_MAX_ENTRIES)
    {
        capacity = D_INTERNAL_EVENT_TABLE_MAX_ENTRIES;
    }

    // no prime fits, or a block size would overflow
    if ( (buckets == 0u)                                         ||
         (capacity > (SIZE_MAX / sizeof(struct d_event_entry))) ||
         (buckets > (SIZE_MAX / sizeof(uint32_t))) )
    {
        return D_EVENT_ERR_ALLOC;
    }

    struct d_event_entry* entries = malloc(capacity *
                                           sizeof(struct d_event_entry));
    uint32_t*             head    = malloc(buckets * sizeof(uint32_t));
    uint32_t*             tail    = malloc(buckets * sizeof(uint32_t));

    // all or nothing
    if ( (!entries) ||
         (!head)    ||
         (!tail) )
    {
        free(entries);
        free(head);
        free(tail);

        return D_EVENT_ERR_ALLOC;
    }

    // every chain starts empty
    for (size_t i = 0u; i < buckets; ++i)
    {
        head[i] = D_EVENT_INDEX_NONE;
        tail[i] = D_EVENT_INDEX_NONE;
    }

    _table->entries      = entries;
    _table->bucket_head  = head;
    _table->bucket_tail  = tail;
    _table->capacity     = capacity;
    _table->bucket_count = buckets;
    _table->next_id      = 1u;
    _table->free_head    = D_EVENT_INDEX_NONE;
    _table->flags        = D_EVENT_TABLE_FLAG_OWNS_STORAGE;

    return D_EVENT_OK;
}

#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

/*
d_event_table_dispose
  Clears first, so every hook runs, then frees what the table owns and zeroes
it: a zeroed table is what makes a second call, or a call on a table never
initialized, a no-op. Called from a handler of this table's own dispatch, it
unbinds everything now and leaves the freeing to the outermost dispatch.
*/
void
d_event_table_dispose(
    struct d_event_table* _table
)
{
    // nothing to release
    if (!_table)
    {
        return;
    }

    d_event_table_clear(_table);

    // mid-dispatch: the running fold still needs the arena
    if ((_table->flags & D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING) != 0u)
    {
        _table->flags |= D_INTERNAL_EVENT_TABLE_FLAG_DISPOSE;

        return;
    }

    // free what the table allocated itself
    if ((_table->flags & D_EVENT_TABLE_FLAG_OWNS_STORAGE) != 0u)
    {
        free(_table->entries);
        free(_table->bucket_head);
        free(_table->bucket_tail);
    }

    memset(_table, 0, sizeof(*_table));

    return;
}

/*
d_event_table_clear
  Walks the arena once, dropping every live entry bound before the clear
began; an entry a hook binds meanwhile has a higher id and survives, so a
hook that binds cannot keep the walk going. The handler id counter is NOT
reset: a handle held from before the clear must never come to name an entry
bound after it. An emptied table outside a dispatch is compacted back to its
initial layout.
*/
void
d_event_table_clear(
    struct d_event_table* _table
)
{
    // nothing bound
    if (!d_internal_event_table_ready(_table))
    {
        return;
    }

    const d_handler_id bound = _table->next_id - 1u;

    // drop every live entry that existed when the clear began
    for (uint32_t i = 0u; i < _table->used; ++i)
    {
        const struct d_event_entry* const entry = &_table->entries[i];

        if ( (d_internal_event_entry_live(entry)) &&
             (entry->id <= bound) )
        {
            d_internal_event_table_drop(_table, i);
        }
    }

    // an empty table outside a dispatch returns to its initial layout
    if ( (_table->count == 0u) &&
         ((_table->flags & D_INTERNAL_EVENT_TABLE_FLAG_DISPATCHING) == 0u) )
    {
        _table->used      = 0u;
        _table->free_head = D_EVENT_INDEX_NONE;

        for (size_t i = 0u; i < _table->bucket_count; ++i)
        {
            _table->bucket_head[i] = D_EVENT_INDEX_NONE;
            _table->bucket_tail[i] = D_EVENT_INDEX_NONE;
        }
    }

    return;
}

/*
d_event_table_clear_key
  Drops the key's first live letter until none remains that predates the
call, so the search always starts from a consistent chain whatever a hook
did in between.
*/
size_t
d_event_table_clear_key(
    struct d_event_table* _table,
    d_event_key           _key
)
{
    size_t removed = 0u;

    // nothing bound
    if (!d_internal_event_table_ready(_table))
    {
        return 0u;
    }

    const d_handler_id bound = _table->next_id - 1u;

    // drop the word's letters one at a time, first to last
    for (;;)
    {
        const uint32_t index = d_internal_event_table_next_of_key(
            _table, _key, D_HANDLER_ID_NULL, bound);

        if (index == D_EVENT_INDEX_NONE)
        {
            break;
        }

        d_internal_event_table_drop(_table, index);
        ++removed;
    }

    return removed;
}

/*
d_event_table_bind
  The public form of d_internal_event_table_bind_at.
*/
int
d_event_table_bind(
    struct d_event_table* _table,
    d_event_key           _key,
    struct d_event_step   _step,
    fn_free               _state_free,
    d_handler_id*         _id_out
)
{
    // the table is required
    if (!_table)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_internal_event_table_bind_at(_table,
                                          _key,
                                          _step,
                                          _state_free,
                                          _id_out,
                                          NULL);
}

/*
d_event_table_unbind
  See d_internal_event_table_drop for what happens mid-dispatch.
*/
int
d_event_table_unbind(
    struct d_event_table* _table,
    d_handler_id          _id
)
{
    // nothing is bound in a missing or empty table
    if (!d_internal_event_table_ready(_table))
    {
        return (_table != NULL) ? D_EVENT_ERR_NOT_FOUND : D_EVENT_ERR_NULL;
    }

    const uint32_t index = d_internal_event_table_index_of(_table, _id);

    if (index == D_EVENT_INDEX_NONE)
    {
        return D_EVENT_ERR_NOT_FOUND;
    }

    d_internal_event_table_drop(_table, index);

    return D_EVENT_OK;
}

/*
d_internal_event_table_set_mask
  enable and disable differ only in the target state; the three-way result
is shared.
*/
static int
d_internal_event_table_set_mask(
    struct d_event_table* _table,
    d_handler_id          _id,
    bool                  _on
)
{
    // nothing is bound in a missing or empty table
    if (!d_internal_event_table_ready(_table))
    {
        return D_EVENT_ERR_NOT_FOUND;
    }

    const uint32_t index = d_internal_event_table_index_of(_table, _id);

    if (index == D_EVENT_INDEX_NONE)
    {
        return D_EVENT_ERR_NOT_FOUND;
    }

    struct d_event_entry* const entry = &_table->entries[index];
    const bool is_on = ((entry->enabled & D_INTERNAL_EVENT_ENTRY_ON) != 0u);

    // already in the requested state
    if (is_on == _on)
    {
        return D_EVENT_ERR_STATE;
    }

    // flip the mask and its count together
    if (_on)
    {
        entry->enabled = D_INTERNAL_EVENT_ENTRY_ON;
        ++_table->enabled_count;
    }
    else
    {
        entry->enabled = 0u;
        --_table->enabled_count;
    }

    return D_EVENT_OK;
}

int
d_event_table_enable(
    struct d_event_table* _table,
    d_handler_id          _id
)
{
    return d_internal_event_table_set_mask(_table, _id, true);
}

int
d_event_table_disable(
    struct d_event_table* _table,
    d_handler_id          _id
)
{
    return d_internal_event_table_set_mask(_table, _id, false);
}

bool
d_event_table_is_enabled(
    const struct d_event_table* _table,
    d_handler_id                _id
)
{
    const struct d_event_entry* const entry = d_event_table_find(_table, _id);

    return ( (entry != NULL) &&
             ((entry->enabled & D_INTERNAL_EVENT_ENTRY_ON) != 0u) );
}

bool
d_event_table_contains(
    const struct d_event_table* _table,
    d_handler_id                _id
)
{
    return (d_event_table_find(_table, _id) != NULL);
}

/*
d_event_table_first
  Walks the key's bucket chain to its first live letter of that key.
*/
uint32_t
d_event_table_first(
    const struct d_event_table* _table,
    d_event_key                 _key
)
{
    // nothing bound
    if (!d_internal_event_table_ready(_table))
    {
        return D_EVENT_INDEX_NONE;
    }

    uint32_t cur = _table->bucket_head[d_event_key_hash(_key,
                                                        _table->bucket_count)];

    // skip the other keys sharing the bucket
    while (cur != D_EVENT_INDEX_NONE)
    {
        if ( (_table->entries[cur].key == _key) &&
             (d_internal_event_entry_live(&_table->entries[cur])) )
        {
            return cur;
        }

        cur = _table->entries[cur].next;
    }

    return D_EVENT_INDEX_NONE;
}

/*
d_event_table_next
  A free slot is on no chain, so it has no successor. A zombie still is, so
a cursor resting on one keeps working.
*/
uint32_t
d_event_table_next(
    const struct d_event_table* _table,
    d_event_key                 _key,
    uint32_t                    _index
)
{
    // out of range, or a slot on no chain
    if ( (!d_internal_event_table_ready(_table)) ||
         (_index >= _table->used)                ||
         (_table->entries[_index].id == D_HANDLER_ID_NULL) )
    {
        return D_EVENT_INDEX_NONE;
    }

    uint32_t cur = _table->entries[_index].next;

    // skip the other keys sharing the bucket
    while (cur != D_EVENT_INDEX_NONE)
    {
        if ( (_table->entries[cur].key == _key) &&
             (d_internal_event_entry_live(&_table->entries[cur])) )
        {
            return cur;
        }

        cur = _table->entries[cur].next;
    }

    return D_EVENT_INDEX_NONE;
}

const struct d_event_entry*
d_event_table_at(
    const struct d_event_table* _table,
    uint32_t                    _index
)
{
    // out of range, free, or a zombie
    if ( (!d_internal_event_table_ready(_table)) ||
         (_index >= _table->used)                ||
         (!d_internal_event_entry_live(&_table->entries[_index])) )
    {
        return NULL;
    }

    return &_table->entries[_index];
}

const struct d_event_entry*
d_event_table_find(
    const struct d_event_table* _table,
    d_handler_id                _id
)
{
    // nothing bound
    if (!d_internal_event_table_ready(_table))
    {
        return NULL;
    }

    const uint32_t index = d_internal_event_table_index_of(_table, _id);

    return (index == D_EVENT_INDEX_NONE) ? NULL : &_table->entries[index];
}

size_t
d_event_table_count(
    const struct d_event_table* _table
)
{
    return (_table != NULL) ? _table->count : 0u;
}

size_t
d_event_table_enabled_count(
    const struct d_event_table* _table
)
{
    return (_table != NULL) ? _table->enabled_count : 0u;
}

size_t
d_event_table_count_for(
    const struct d_event_table* _table,
    d_event_key                 _key
)
{
    size_t   count = 0u;
    uint32_t index = d_event_table_first(_table, _key);

    // walk the word
    while (index != D_EVENT_INDEX_NONE)
    {
        ++count;
        index = d_event_table_next(_table, _key, index);
    }

    return count;
}

bool
d_event_table_has_entries_for(
    const struct d_event_table* _table,
    d_event_key                 _key
)
{
    return (d_event_table_first(_table, _key) != D_EVENT_INDEX_NONE);
}

/*
d_internal_event_table_first_in_chain
  True when no live entry before _index in its chain holds the same key,
i.e. _index opens its word's run within the bucket. Counting distinct keys
this way needs no storage.
*/
static bool
d_internal_event_table_first_in_chain(
    const struct d_event_table* _table,
    size_t                      _bucket,
    uint32_t                    _index
)
{
    uint32_t cur = _table->bucket_head[_bucket];

    // look for an earlier live letter of the same key
    while ( (cur != D_EVENT_INDEX_NONE) &&
            (cur != _index) )
    {
        if ( (_table->entries[cur].key == _table->entries[_index].key) &&
             (d_internal_event_entry_live(&_table->entries[cur])) )
        {
            return false;
        }

        cur = _table->entries[cur].next;
    }

    return true;
}

size_t
d_event_table_key_count(
    const struct d_event_table* _table
)
{
    size_t keys = 0u;

    // nothing bound
    if (!d_internal_event_table_ready(_table))
    {
        return 0u;
    }

    // count each key at its first live letter in its chain
    for (size_t bucket = 0u; bucket < _table->bucket_count; ++bucket)
    {
        for (uint32_t cur = _table->bucket_head[bucket];
             cur != D_EVENT_INDEX_NONE;
             cur = _table->entries[cur].next)
        {
            if ( (d_internal_event_entry_live(&_table->entries[cur])) &&
                 (d_internal_event_table_first_in_chain(_table,
                                                        bucket,
                                                        cur)) )
            {
                ++keys;
            }
        }
    }

    return keys;
}

/*
d_event_table_for_each
  Ascending handler id, bounded by the id counter at entry: see
d_internal_event_table_next_in_order for why the walk survives edits.
*/
size_t
d_event_table_for_each(
    const struct d_event_table* _table,
    fn_event_entry_visitor      _visitor,
    void*                       _context
)
{
    size_t visited = 0u;

    // nothing to visit, or nobody to visit it
    if ( (!d_internal_event_table_ready(_table)) ||
         (!_visitor) )
    {
        return 0u;
    }

    const bool         monotone = d_internal_event_table_monotone(_table);
    const d_handler_id bound    = _table->next_id - 1u;
    d_handler_id       last     = D_HANDLER_ID_NULL;
    uint32_t           cursor   = 0u;

    // visit in bind order until the entries or the visitor run out
    for (;;)
    {
        const uint32_t index = d_internal_event_table_next_in_order(
            _table, monotone, &cursor, last, bound);

        if (index == D_EVENT_INDEX_NONE)
        {
            break;
        }

        last = _table->entries[index].id;
        ++visited;

        if (!_visitor(&_table->entries[index], _context))
        {
            break;
        }
    }

    return visited;
}

size_t
d_event_table_for_each_key(
    const struct d_event_table* _table,
    d_event_key                 _key,
    fn_event_entry_visitor      _visitor,
    void*                       _context
)
{
    size_t visited = 0u;

    // nothing to visit, or nobody to visit it
    if ( (!d_internal_event_table_ready(_table)) ||
         (!_visitor) )
    {
        return 0u;
    }

    const d_handler_id bound = _table->next_id - 1u;
    d_handler_id       last  = D_HANDLER_ID_NULL;

    // visit the word in word order until it or the visitor runs out
    for (;;)
    {
        const uint32_t index = d_internal_event_table_next_of_key(
            _table, _key, last, bound);

        if (index == D_EVENT_INDEX_NONE)
        {
            break;
        }

        last = _table->entries[index].id;
        ++visited;

        if (!_visitor(&_table->entries[index], _context))
        {
            break;
        }
    }

    return visited;
}

#if (D_INTERNAL_EVENT_TABLE_MERGE == 1)

/*
d_event_table_merge
  Appends `_other`'s live letters in its bind order, which keeps each word's
letters in word order; the fresh ids therefore follow `_other`'s bind order.
The walk is bounded by `_other`'s id counter at entry, so merging a table
into itself doubles each word once instead of chasing its own appends.
*/
int
d_event_table_merge(
    struct d_event_table*       _table,
    const struct d_event_table* _other,
    size_t*                     _merged_out
)
{
    size_t merged = 0u;

    if (_merged_out)
    {
        *_merged_out = 0u;
    }

    // both tables are required
    if ( (!_table) ||
         (!_other) )
    {
        return D_EVENT_ERR_NULL;
    }

    // merging the empty table is the identity
    if (!d_internal_event_table_ready(_other))
    {
        return D_EVENT_OK;
    }

    const bool         monotone = d_internal_event_table_monotone(_other);
    const d_handler_id bound    = _other->next_id - 1u;
    d_handler_id       last     = D_HANDLER_ID_NULL;
    uint32_t           cursor   = 0u;
    int                status   = D_EVENT_OK;

    // append each letter, borrowing its state and carrying its mask
    for (;;)
    {
        const uint32_t from = d_internal_event_table_next_in_order(
            _other, monotone, &cursor, last, bound);
        uint32_t       to   = D_EVENT_INDEX_NONE;

        if (from == D_EVENT_INDEX_NONE)
        {
            break;
        }

        // copy out first: binding into the same table may move its arena
        const struct d_event_entry letter = _other->entries[from];

        last   = letter.id;
        status = d_internal_event_table_bind_at(_table,
                                                letter.key,
                                                letter.step,
                                                NULL,
                                                NULL,
                                                &to);

        if (status != D_EVENT_OK)
        {
            break;
        }

        // the mask travels with the letter
        if ((letter.enabled & D_INTERNAL_EVENT_ENTRY_ON) == 0u)
        {
            _table->entries[to].enabled = 0u;
            --_table->enabled_count;
        }

        ++merged;
    }

    if (_merged_out)
    {
        *_merged_out = merged;
    }

    return status;
}

#endif  // D_INTERNAL_EVENT_TABLE_MERGE

#if (D_INTERNAL_EVENT_TABLE_STATS == 1)

/*
d_event_table_get_stats
  One pass over the chains, counting live letters only: a zombie is unbound.
*/
struct d_event_table_stats
d_event_table_get_stats(
    const struct d_event_table* _table
)
{
    struct d_event_table_stats stats;

    stats.total_buckets           = 0u;
    stats.used_buckets            = 0u;
    stats.total_entries           = 0u;
    stats.enabled_entries         = 0u;
    stats.key_count               = 0u;
    stats.max_entries_per_key     = 0u;
    stats.average_entries_per_key = 0.0;
    stats.load_factor             = 0.0;

    // a zeroed table yields the zero record
    if (!d_internal_event_table_ready(_table))
    {
        return stats;
    }

    stats.total_buckets   = _table->bucket_count;
    stats.total_entries   = _table->count;
    stats.enabled_entries = _table->enabled_count;
    stats.load_factor     = d_event_table_load_factor(_table);

    // per bucket: is it used, and how long is each word that opens in it
    for (size_t bucket = 0u; bucket < _table->bucket_count; ++bucket)
    {
        bool used = false;

        for (uint32_t cur = _table->bucket_head[bucket];
             cur != D_EVENT_INDEX_NONE;
             cur = _table->entries[cur].next)
        {
            if (!d_internal_event_entry_live(&_table->entries[cur]))
            {
                continue;
            }

            used = true;

            // a word is measured once, at its first letter
            if (d_internal_event_table_first_in_chain(_table, bucket, cur))
            {
                const size_t length = d_event_table_count_for(
                    _table, _table->entries[cur].key);

                ++stats.key_count;

                if (length > stats.max_entries_per_key)
                {
                    stats.max_entries_per_key = length;
                }
            }
        }

        if (used)
        {
            ++stats.used_buckets;
        }
    }

    stats.average_entries_per_key =
        (stats.key_count > 0u)
            ? ((double)stats.total_entries / (double)stats.key_count)
            : 0.0;

    return stats;
}

#endif  // D_INTERNAL_EVENT_TABLE_STATS

double
d_event_table_load_factor(
    const struct d_event_table* _table
)
{
    // no buckets, no load
    if ( (!_table) ||
         (_table->bucket_count == 0u) )
    {
        return 0.0;
    }

    return (double)_table->count / (double)_table->bucket_count;
}

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

/*
d_event_table_rehash
  Rebuilds the chains by walking the old ones in order and appending each
entry, zombies included, to its new chain. The entries of one key share an
old chain and a new one, so their relative order -- the word -- survives;
only the interleaving of different keys in a bucket can change. Slot indices
do not move, so a running dispatch's cursor stays valid.
*/
int
d_event_table_rehash(
    struct d_event_table* _table,
    size_t                _bucket_count
)
{
    // the table is required
    if (!_table)
    {
        return D_EVENT_ERR_NULL;
    }

    // a fixed or borrowed table has no arrays of its own to replace
    if ( ((_table->flags & D_EVENT_TABLE_FLAG_FIXED) != 0u)        ||
         ((_table->flags & D_EVENT_TABLE_FLAG_OWNS_STORAGE) == 0u) ||
         (!d_internal_event_table_ready(_table)) )
    {
        return D_EVENT_ERR_STATE;
    }

    const size_t buckets = d_event_next_prime(_bucket_count);

    // no prime fits, or the arrays would overflow
    if ( (buckets == 0u) ||
         (buckets > (SIZE_MAX / sizeof(uint32_t))) )
    {
        return D_EVENT_ERR_ALLOC;
    }

    uint32_t* head = malloc(buckets * sizeof(uint32_t));
    uint32_t* tail = malloc(buckets * sizeof(uint32_t));

    // all or nothing
    if ( (!head) ||
         (!tail) )
    {
        free(head);
        free(tail);

        return D_EVENT_ERR_ALLOC;
    }

    // every new chain starts empty
    for (size_t i = 0u; i < buckets; ++i)
    {
        head[i] = D_EVENT_INDEX_NONE;
        tail[i] = D_EVENT_INDEX_NONE;
    }

    // re-append every linked entry, walking each old chain in order
    for (size_t bucket = 0u; bucket < _table->bucket_count; ++bucket)
    {
        uint32_t cur = _table->bucket_head[bucket];

        while (cur != D_EVENT_INDEX_NONE)
        {
            const uint32_t next = _table->entries[cur].next;
            const size_t   to   = d_event_key_hash(_table->entries[cur].key,
                                                   buckets);

            _table->entries[cur].next = D_EVENT_INDEX_NONE;

            if (tail[to] == D_EVENT_INDEX_NONE)
            {
                head[to] = cur;
            }
            else
            {
                _table->entries[tail[to]].next = cur;
            }

            tail[to] = cur;
            cur      = next;
        }
    }

    free(_table->bucket_head);
    free(_table->bucket_tail);
    _table->bucket_head  = head;
    _table->bucket_tail  = tail;
    _table->bucket_count = buckets;

    return D_EVENT_OK;
}

/*
d_event_table_reserve
  Grows the arena with realloc, which may move it: that is why the table and
every cursor into it speak in indices. A request beyond the index range is
clamped to it, and is refused only if even that would not grow the arena.
*/
int
d_event_table_reserve(
    struct d_event_table* _table,
    size_t                _capacity
)
{
    // the table is required
    if (!_table)
    {
        return D_EVENT_ERR_NULL;
    }

    // a fixed or borrowed table cannot grow
    if ( ((_table->flags & D_EVENT_TABLE_FLAG_FIXED) != 0u) ||
         ((_table->flags & D_EVENT_TABLE_FLAG_OWNS_STORAGE) == 0u) )
    {
        return D_EVENT_ERR_STATE;
    }

    // already large enough
    if (_capacity <= _table->capacity)
    {
        return D_EVENT_OK;
    }

    const size_t capacity = (_capacity > D_INTERNAL_EVENT_TABLE_MAX_ENTRIES)
                                ? D_INTERNAL_EVENT_TABLE_MAX_ENTRIES
                                : _capacity;

    // clamped to the index range, the request no longer grows the arena
    if (capacity <= _table->capacity)
    {
        return D_EVENT_ERR_CAPACITY;
    }

    // the block size would overflow
    if (capacity > (SIZE_MAX / sizeof(struct d_event_entry)))
    {
        return D_EVENT_ERR_ALLOC;
    }

    struct d_event_entry* const entries =
        realloc(_table->entries, capacity * sizeof(struct d_event_entry));

    if (!entries)
    {
        return D_EVENT_ERR_ALLOC;
    }

    _table->entries  = entries;
    _table->capacity = capacity;

    return D_EVENT_OK;
}

#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

size_t
d_event_table_footprint(
    const struct d_event_table* _table
)
{
    // no table, no bytes
    if (!_table)
    {
        return 0u;
    }

    size_t bytes = sizeof(struct d_event_table);

    // owned storage counts; borrowed storage is the caller's
    if ((_table->flags & D_EVENT_TABLE_FLAG_OWNS_STORAGE) != 0u)
    {
        bytes += _table->capacity * sizeof(struct d_event_entry);
        bytes += 2u * _table->bucket_count * sizeof(uint32_t);
    }

    return bytes;
}
