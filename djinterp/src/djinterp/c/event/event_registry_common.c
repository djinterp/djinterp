/*******************************************************************************
* djinterp [c]                                           event_registry_common.c
*
* The registry and its folds -- tier 0 definitions.
*   Implements event_registry_common.h. The registry is one table, so most of
* this file forwards; the substance is dispatch, which walks the word for a
* key and folds it letter by letter through d_event_step_fold -- the same fold
* the fused word runs over its snapshot, so the two paths cannot diverge in
* how a step is invoked, how the unit passes, or where consume cuts off.
*
* path:      /src/djinterp/c/event/event_registry_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/event/event_registry_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memset
// djinterp
#include "./event_table_internal.h"  // the dispatch bracket and its markers
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint32_t,
                                                     // SIZE_MAX


int
d_event_registry_init_fixed(
    struct d_event_registry* _registry,
    struct d_event_entry*    _entries,
    size_t                   _capacity,
    uint32_t*                _bucket_head,
    uint32_t*                _bucket_tail,
    size_t                   _bucket_count
)
{
    // the registry is required
    if (!_registry)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_table_init_fixed(&_registry->table,
                                    _entries,
                                    _capacity,
                                    _bucket_head,
                                    _bucket_tail,
                                    _bucket_count);
}

/*
d_event_registry_init
  Declared whatever the configuration, so a build without allocation still
links; there it leaves the registry zeroed and reports the missing
capability as a state error rather than an allocation failure.
*/
int
d_event_registry_init(
    struct d_event_registry* _registry,
    size_t                   _capacity,
    size_t                   _bucket_count
)
{
    // the registry is required
    if (!_registry)
    {
        return D_EVENT_ERR_NULL;
    }

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)
    return d_event_table_init(&_registry->table, _capacity, _bucket_count);
#else
    (void)_capacity;
    (void)_bucket_count;
    memset(&_registry->table, 0, sizeof(_registry->table));

    return D_EVENT_ERR_STATE;
#endif  // D_INTERNAL_EVENT_TABLE_ALLOC
}

void
d_event_registry_dispose(
    struct d_event_registry* _registry
)
{
    // nothing to release
    if (_registry)
    {
        d_event_table_dispose(&_registry->table);
    }

    return;
}

int
d_event_registry_bind(
    struct d_event_registry* _registry,
    d_event_key              _key,
    struct d_event_step      _step,
    fn_free                  _state_free,
    d_handler_id*            _id_out
)
{
    // the registry is required
    if (!_registry)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_table_bind(&_registry->table,
                              _key,
                              _step,
                              _state_free,
                              _id_out);
}

int
d_event_registry_unbind(
    struct d_event_registry* _registry,
    d_handler_id             _id
)
{
    // the registry is required
    if (!_registry)
    {
        return D_EVENT_ERR_NULL;
    }

    return d_event_table_unbind(&_registry->table, _id);
}

#if (D_INTERNAL_EVENT_REGISTRY_MERGE == 1)

int
d_event_registry_merge(
    struct d_event_registry*       _registry,
    const struct d_event_registry* _other,
    size_t*                        _merged_out
)
{
    // both registries are required
    if ( (!_registry) ||
         (!_other) )
    {
        if (_merged_out)
        {
            *_merged_out = 0u;
        }

        return D_EVENT_ERR_NULL;
    }

    return d_event_table_merge(&_registry->table, &_other->table, _merged_out);
}

#endif  // D_INTERNAL_EVENT_REGISTRY_MERGE

/*
d_event_registry_dispatch
  Folds the word in force when the occurrence began. Two bounds make that
hold while handlers edit the registry:
  - a letter bound during the dispatch has an id above the counter read at
entry, and is skipped;
  - a letter unbound during it is a zombie (see event_table_internal.h): still
linked, state alive, so the outermost dispatch still folds it. A NESTED
dispatch skips zombies, which is exact for letters unbound before it began;
the one approximation left is a letter unbound during a nested dispatch by a
handler of that same nested dispatch, which the nested fold then skips.
  The mask is read as each letter is reached. Every read goes through the
table afresh after a handler returns, because a handler may grow the arena
(realloc) or rehash the chains; slot indices survive both, and a rehash keeps
the letters of one key in word order, so following `next` from the current
slot stays on the word.
*/
struct d_dispatch_result
d_event_registry_dispatch(
    struct d_event_registry* _registry,
    d_event_key              _key,
    void*                    _payload
)
{
    struct d_dispatch_result result = d_dispatch_result_empty();

    // no registry, or an empty one: the empty word
    if ( (!_registry)                                 ||
         (!_registry->table.entries)                  ||
         (!_registry->table.bucket_head)              ||
         (_registry->table.bucket_count == 0u) )
    {
        return result;
    }

    struct d_event_table* const table = &_registry->table;
    const d_handler_id          bound = table->next_id - 1u;
    uint32_t                    cursor =
        table->bucket_head[d_event_key_hash(_key, table->bucket_count)];

    // an empty bucket is an empty word
    if (cursor == D_EVENT_INDEX_NONE)
    {
        return result;
    }

    const bool outermost = d_internal_event_table_dispatch_begin(table);

    // fold the word in chain order, which for one key is bind order
    while (cursor != D_EVENT_INDEX_NONE)
    {
        const struct d_event_entry* const entry  = &table->entries[cursor];
        const bool                        zombie =
            ((entry->enabled & D_INTERNAL_EVENT_ENTRY_ZOMBIE) != 0u);

        // a letter of this word, bound before the occurrence, mask on
        if ( (entry->key == _key)                                      &&
             (entry->id != D_HANDLER_ID_NULL)                          &&
             (entry->id <= bound)                                      &&
             ((entry->enabled & D_INTERNAL_EVENT_ENTRY_ON) != 0u)      &&
             ( (!zombie) ||
               (outermost) ) )
        {
            // copy the step: the arena may move while it runs
            const struct d_event_step step    = entry->step;
            size_t                    invoked = 0u;
            const int32_t             verdict =
                d_event_step_fold(&step, 1u, _payload, &invoked);

            result.invoked += invoked;

            // consume is the left zero: the remainder is cut off
            if (D_VERDICT_IS_CONSUMED(verdict))
            {
                result.outcome = (int32_t)D_VERDICT_CONSUME;

                break;
            }
        }

        cursor = table->entries[cursor].next;
    }

    d_internal_event_table_dispatch_end(table, outermost);

    return result;
}

struct d_dispatch_result
d_event_registry_dispatch_occurrence(
    struct d_event_registry*         _registry,
    const struct d_event_occurrence* _occurrence
)
{
    // no occurrence: the empty word's result
    if (!_occurrence)
    {
        return d_dispatch_result_empty();
    }

    return d_event_registry_dispatch(_registry,
                                     _occurrence->key,
                                     _occurrence->payload.data);
}

#if ( (D_INTERNAL_EVENT_REGISTRY_RUN == 1) ||                                  \
      (D_INTERNAL_EVENT_REGISTRY_STAGING == 1) )

/*
d_internal_event_block
  The payload block of trace element _i: NULL throughout for a trace of
empty payloads (arity 0), else _first advanced by _i strides.
*/
static void*
d_internal_event_block(
    void*  _first,
    size_t _stride,
    size_t _i
)
{
    return (_first != NULL)
        ? (void*)((unsigned char*)_first + (_i * _stride))
        : NULL;
}

#endif  // D_INTERNAL_EVENT_REGISTRY_RUN || D_INTERNAL_EVENT_REGISTRY_STAGING

#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)

/*
d_event_registry_run
  Each element is a separate occurrence, so each dispatch takes its own
snapshot: an edit a handler makes during element i is in force for i + 1.
*/
struct d_run_result
d_event_registry_run(
    struct d_event_registry* _registry,
    d_event_key              _key,
    void*                    _first,
    size_t                   _stride,
    size_t                   _count
)
{
    struct d_run_result run;

    run.occurrences      = 0u;
    run.handlers_invoked = 0u;
    run.consumed_count   = 0u;

    // no registry: nothing runs
    if (!_registry)
    {
        return run;
    }

    // dispatch each block in place, in trace order
    for (size_t i = 0u; i < _count; ++i)
    {
        const struct d_dispatch_result result = d_event_registry_dispatch(
            _registry, _key, d_internal_event_block(_first, _stride, i));

        ++run.occurrences;
        run.handlers_invoked += result.invoked;

        if (D_VERDICT_IS_CONSUMED(result.outcome))
        {
            ++run.consumed_count;
        }
    }

    return run;
}

#endif  // D_INTERNAL_EVENT_REGISTRY_RUN

#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)

/*
d_event_registry_compile_into
  The snapshot is the live, enabled letters in word order -- the masked word
exactly as a dispatch starting now would fold it.
*/
int
d_event_registry_compile_into(
    const struct d_event_registry* _registry,
    d_event_key                    _key,
    struct d_event_step*           _steps,
    size_t                         _capacity,
    struct d_event_word*           _word_out
)
{
    // the registry and the word are required, and storage if any fits
    if ( (!_registry)                     ||
         (!_word_out)                     ||
         ( (!_steps) && (_capacity > 0u) ) )
    {
        return D_EVENT_ERR_NULL;
    }

    const struct d_event_table* const table = &_registry->table;
    uint32_t index = d_event_table_first(table, _key);

    _word_out->steps    = _steps;
    _word_out->count    = 0u;
    _word_out->capacity = _capacity;
    _word_out->key      = _key;
    _word_out->flags    = 0u;
    _word_out->reserved = 0u;

    // copy the enabled letters in word order while they fit
    while (index != D_EVENT_INDEX_NONE)
    {
        if ((table->entries[index].enabled &
             D_INTERNAL_EVENT_ENTRY_ON) != 0u)
        {
            // a truncated word is a different word: report it
            if (_word_out->count >= _capacity)
            {
                return D_EVENT_ERR_CAPACITY;
            }

            _steps[_word_out->count] = table->entries[index].step;
            ++_word_out->count;
        }

        index = d_event_table_next(table, _key, index);
    }

    return D_EVENT_OK;
}

#if (D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC == 1)

/*
d_event_registry_compile
  Measures the enabled word first and allocates exactly that many steps; an
empty word allocates nothing and owns nothing.
*/
int
d_event_registry_compile(
    const struct d_event_registry* _registry,
    d_event_key                    _key,
    struct d_event_word*           _word_out
)
{
    // the registry and the word are required
    if ( (!_registry) ||
         (!_word_out) )
    {
        return D_EVENT_ERR_NULL;
    }

    const struct d_event_table* const table = &_registry->table;
    size_t   letters = 0u;
    uint32_t index   = d_event_table_first(table, _key);

    // measure the enabled word
    while (index != D_EVENT_INDEX_NONE)
    {
        if ((table->entries[index].enabled &
             D_INTERNAL_EVENT_ENTRY_ON) != 0u)
        {
            ++letters;
        }

        index = d_event_table_next(table, _key, index);
    }

    // an empty word needs no storage
    if (letters == 0u)
    {
        return d_event_registry_compile_into(_registry,
                                             _key,
                                             NULL,
                                             0u,
                                             _word_out);
    }

    // the block size would overflow
    if (letters > (SIZE_MAX / sizeof(struct d_event_step)))
    {
        memset(_word_out, 0, sizeof(*_word_out));

        return D_EVENT_ERR_ALLOC;
    }

    struct d_event_step* const steps =
        malloc(letters * sizeof(struct d_event_step));

    if (!steps)
    {
        memset(_word_out, 0, sizeof(*_word_out));

        return D_EVENT_ERR_ALLOC;
    }

    const int status = d_event_registry_compile_into(_registry,
                                                     _key,
                                                     steps,
                                                     letters,
                                                     _word_out);

    _word_out->flags = D_EVENT_WORD_FLAG_OWNS_STORAGE;

    return status;
}

#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC

void
d_event_word_dispose(
    struct d_event_word* _word
)
{
    // nothing to release
    if (!_word)
    {
        return;
    }

    // free the step array only if the word allocated it
    if ((_word->flags & D_EVENT_WORD_FLAG_OWNS_STORAGE) != 0u)
    {
        free(_word->steps);
    }

    memset(_word, 0, sizeof(*_word));

    return;
}

int32_t
d_event_word_run_one(
    const struct d_event_word* _word,
    void*                      _payload,
    size_t*                    _invoked_out
)
{
    // no word: the empty word, which passes
    if (!_word)
    {
        if (_invoked_out)
        {
            *_invoked_out = 0u;
        }

        return (int32_t)D_VERDICT_PASS;
    }

    return d_event_step_fold(_word->steps,
                             _word->count,
                             _payload,
                             _invoked_out);
}

/*
d_event_word_drive
  The fused outer fold: the same blocks, in the same order, through the same
d_event_step_fold that dispatch uses, which is what the coherence law with
d_event_registry_run rests on.
*/
struct d_drive_result
d_event_word_drive(
    const struct d_event_word* _word,
    void*                      _first,
    size_t                     _stride,
    size_t                     _count
)
{
    struct d_drive_result drive;

    drive.occurrences    = 0u;
    drive.consumed_count = 0u;

    // no word: nothing runs
    if (!_word)
    {
        return drive;
    }

    // fold the word over each block, in trace order
    for (size_t i = 0u; i < _count; ++i)
    {
        ++drive.occurrences;

        if (D_VERDICT_IS_CONSUMED(d_event_word_run_one(
                _word, d_internal_event_block(_first, _stride, i), NULL)))
        {
            ++drive.consumed_count;
        }
    }

    return drive;
}

#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING
