/*******************************************************************************
* djinterp [c]                                                    t_event_core.c
*
*   Conformance harness for the tier-0 event core: every contract the five
* event_*_common.h headers state, checked against their implementations --
* the monoid laws (including that consume leaves its right operand
* uninvoked), the table's word order under slot reuse and rehashing, the
* three-way mask results, merge, dispatch and its snapshot semantics, run,
* the fused word and its coherence law with run, and the queue with its
* deferral coherence law, re-entrancy guarantees and ring wrap-around.
*   Sections compile away with the configuration knobs that remove what they
* test, so the harness builds in every configuration.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                  \
*        src/djinterp/c/event/event_common.c                                   \
*        src/djinterp/c/event/event_handler_common.c                           \
*        src/djinterp/c/event/event_table_common.c                             \
*        src/djinterp/c/event/event_registry_common.c                          \
*        src/djinterp/c/event/event_dispatcher_common.c                        \
*        tests/djinterp/c/event/t_event_core.c -o t_event_core
*
*
* path:      /tests/djinterp/c/event/t_event_core.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <string.h>   // memcmp, memcpy, memset, strcmp
// djinterp
#include "../../../../inc/djinterp/c/event/event_dispatcher_common.h"  // unit under test
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint32_t,
                                                     // SIZE_MAX


static int g_checks   = 0;
static int g_failures = 0;

/*
d_tests_event_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_event_expect(
    bool        _ok,
    const char* _what,
    int         _line
)
{
    ++g_checks;

    // report a failure with what was being checked, and where
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL line %d: %s\n", _line, _what);
    }

    return;
}

// D_TESTS_EVENT_CHECK
//   macro: records one check with its source text and line, which only the
// preprocessor can supply.
#define D_TESTS_EVENT_CHECK(_cond)                                            \
    d_tests_event_expect((_cond), #_cond, __LINE__)


///////////////////////////////////////////////////////////////////////////////
///        SIDE-EFFECT LOG AND HANDLERS                                     ///
///////////////////////////////////////////////////////////////////////////////

static char   g_log[256];
static size_t g_log_length = 0u;

static void
d_tests_event_log_reset(void)
{
    g_log_length = 0u;
    g_log[0]     = '\0';

    return;
}

static void
d_tests_event_log(
    char _c
)
{
    // keep the terminator
    if ((g_log_length + 1u) < sizeof(g_log))
    {
        g_log[g_log_length] = _c;
        ++g_log_length;
        g_log[g_log_length] = '\0';
    }

    return;
}

static bool
d_tests_event_logged(
    const char* _expected
)
{
    return (strcmp(g_log, _expected) == 0);
}

// hook bookkeeping: how many states were released, and the last one
static int   g_freed      = 0;
static void* g_freed_last = NULL;

static void
d_tests_event_free(
    void* _state
)
{
    ++g_freed;
    g_freed_last = _state;

    return;
}

// d_tests_event_tag
//   struct: a step that logs its tag, counts its calls, and returns a fixed
// verdict.
struct d_tests_event_tag
{
    char    tag;
    int32_t verdict;
    int     calls;
};

static int32_t
d_tests_event_step_tag(
    void* _payload,
    void* _state
)
{
    struct d_tests_event_tag* const tag = _state;

    (void)_payload;
    ++tag->calls;
    d_tests_event_log(tag->tag);

    return tag->verdict;
}

static void
d_tests_event_tag_init(
    struct d_tests_event_tag* _tag,
    char                      _c,
    int32_t                   _verdict
)
{
    _tag->tag     = _c;
    _tag->verdict = _verdict;
    _tag->calls   = 0;

    return;
}

static struct d_event_step
d_tests_event_tagged(
    struct d_tests_event_tag* _tag
)
{
    return d_event_step_make(&d_tests_event_step_tag, _tag);
}

// d_tests_event_num
//   struct: a step over an int payload: logs its tag and the value's last
// digit ('-' for no payload), adds `add`, and consumes when asked to and the
// result is odd.
struct d_tests_event_num
{
    char tag;
    int  add;
    bool consume_odd;
};

static int32_t
d_tests_event_step_num(
    void* _payload,
    void* _state
)
{
    const struct d_tests_event_num* const num   = _state;
    int* const                            value = _payload;

    d_tests_event_log(num->tag);

    // an empty payload
    if (!value)
    {
        d_tests_event_log('-');

        return (int32_t)D_VERDICT_PASS;
    }

    d_tests_event_log((char)('0' + (((*value % 10) + 10) % 10)));
    *value += num->add;

    return ( (num->consume_odd) &&
             ((*value % 2) != 0) )
        ? (int32_t)D_VERDICT_CONSUME
        : (int32_t)D_VERDICT_PASS;
}

static struct d_event_step
d_tests_event_numbered(
    struct d_tests_event_num* _num
)
{
    return d_event_step_make(&d_tests_event_step_num, _num);
}

// d_tests_event_actor
//   struct: a step that edits the registry it runs from, to probe dispatch's
// snapshot semantics. It records what it could observe at the moment it ran.
struct d_tests_event_actor
{
    char                     tag;
    int                      action;       // D_TESTS_EVENT_ACT_*
    struct d_event_registry* registry;
    d_event_key              key;          // what BIND binds to, FIRE fires
    d_handler_id             target;       // what UNBIND unbinds
    struct d_event_step      bind_step;    // what BIND binds
    bool                     target_gone;  // contains(target) failed after
    int                      freed_seen;   // g_freed when this step ran
};

enum
{
    D_TESTS_EVENT_ACT_NONE    = 0,
    D_TESTS_EVENT_ACT_BIND    = 1,
    D_TESTS_EVENT_ACT_UNBIND  = 2,
    D_TESTS_EVENT_ACT_CLEAR   = 3,
    D_TESTS_EVENT_ACT_DISPOSE = 4,
    D_TESTS_EVENT_ACT_FIRE    = 5
};

static int32_t
d_tests_event_step_actor(
    void* _payload,
    void* _state
)
{
    struct d_tests_event_actor* const actor = _state;

    d_tests_event_log(actor->tag);
    actor->freed_seen = g_freed;

    // perform the edit this actor was built for
    switch (actor->action)
    {
        case D_TESTS_EVENT_ACT_BIND:
            (void)d_event_registry_bind(actor->registry,
                                        actor->key,
                                        actor->bind_step,
                                        NULL,
                                        NULL);
            break;

        case D_TESTS_EVENT_ACT_UNBIND:
            (void)d_event_registry_unbind(actor->registry, actor->target);
            actor->target_gone =
                !d_event_table_contains(&actor->registry->table,
                                        actor->target);
            break;

        case D_TESTS_EVENT_ACT_CLEAR:
            d_event_table_clear(&actor->registry->table);
            break;

        case D_TESTS_EVENT_ACT_DISPOSE:
            d_event_registry_dispose(actor->registry);
            break;

        case D_TESTS_EVENT_ACT_FIRE:
            (void)d_event_registry_dispatch(actor->registry,
                                            actor->key,
                                            _payload);
            break;

        default:
            break;
    }

    return (int32_t)D_VERDICT_PASS;
}

static void
d_tests_event_actor_init(
    struct d_tests_event_actor* _actor,
    char                        _c,
    int                         _action,
    struct d_event_registry*    _registry
)
{
    memset(_actor, 0, sizeof(*_actor));
    _actor->tag      = _c;
    _actor->action   = _action;
    _actor->registry = _registry;

    return;
}

static struct d_event_step
d_tests_event_acting(
    struct d_tests_event_actor* _actor
)
{
    return d_event_step_make(&d_tests_event_step_actor, _actor);
}

// d_tests_event_ids
//   struct: collects the ids a visitor sees, optionally stopping early.
struct d_tests_event_ids
{
    d_handler_id ids[16];
    size_t       count;
    size_t       stop_after;   // 0: never stop
};

static bool
d_tests_event_collect(
    const struct d_event_entry* _entry,
    void*                       _context
)
{
    struct d_tests_event_ids* const ids = _context;

    if (ids->count < 16u)
    {
        ids->ids[ids->count] = _entry->id;
    }

    ++ids->count;

    return ( (ids->stop_after == 0u) ||
             (ids->count < ids->stop_after) );
}

/*
d_tests_event_word_is
  True when the word for _key, walked with first/next, holds exactly the ids
given, in that order.
*/
static bool
d_tests_event_word_is(
    const struct d_event_table* _table,
    d_event_key                 _key,
    const d_handler_id*         _ids,
    size_t                      _count
)
{
    size_t   seen  = 0u;
    uint32_t index = d_event_table_first(_table, _key);

    // compare letter by letter
    while (index != D_EVENT_INDEX_NONE)
    {
        if ( (seen >= _count) ||
             (_table->entries[index].id != _ids[seen]) )
        {
            return false;
        }

        ++seen;
        index = d_event_table_next(_table, _key, index);
    }

    return (seen == _count);
}

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

/*
d_tests_event_word_ordered
  True when the word for _key has _expected letters in strictly ascending id
order: bind order, whatever slots and buckets they occupy.
*/
static bool
d_tests_event_word_ordered(
    const struct d_event_table* _table,
    d_event_key                 _key,
    size_t                      _expected
)
{
    size_t       seen  = 0u;
    d_handler_id last  = D_HANDLER_ID_NULL;
    uint32_t     index = d_event_table_first(_table, _key);

    // every letter must follow the one before it in bind order
    while (index != D_EVENT_INDEX_NONE)
    {
        if (_table->entries[index].id <= last)
        {
            return false;
        }

        last = _table->entries[index].id;
        ++seen;
        index = d_event_table_next(_table, _key, index);
    }

    return (seen == _expected);
}


#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

#if (D_INTERNAL_EVENT_TABLE_MERGE == 1)

static bool
d_tests_event_log_entry(
    const struct d_event_entry* _entry,
    void*                       _context
)
{
    (void)_context;
    const struct d_tests_event_tag* const tag = _entry->step.state;

    d_tests_event_log(tag->tag);

    return true;
}


#endif  // D_INTERNAL_EVENT_TABLE_MERGE


///////////////////////////////////////////////////////////////////////////////
///        I.    FOUNDATIONS AND THE HANDLER MONOID                         ///
///////////////////////////////////////////////////////////////////////////////

static void
d_tests_event_prime(void)
{
    D_TESTS_EVENT_CHECK(d_event_next_prime(0u) == 2u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(1u) == 2u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(2u) == 2u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(3u) == 3u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(4u) == 5u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(14u) == 17u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(100u) == 101u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(7920u) == 7927u);

    // no prime at or above the top of size_t fits in one
    D_TESTS_EVENT_CHECK(d_event_next_prime(SIZE_MAX) == 0u);
    D_TESTS_EVENT_CHECK(d_event_next_prime(SIZE_MAX - 1u) == 0u);

    return;
}

#if (D_INTERNAL_EVENT_HANDLER_ADAPTER == 1)

static int   g_callback_calls   = 0;
static void* g_callback_context = NULL;

static void
d_tests_event_callback(
    void* _context
)
{
    ++g_callback_calls;
    g_callback_context = _context;

    return;
}


#endif  // D_INTERNAL_EVENT_HANDLER_ADAPTER

static void
d_tests_event_monoid(void)
{
    struct d_tests_event_tag  a;
    struct d_tests_event_tag  b;
    struct d_tests_event_tag  c;
    const struct d_event_step skip = d_event_step_skip();

    // the unit passes without a call, and a NULL step is the unit
    D_TESTS_EVENT_CHECK(d_event_step_is_skip(&skip));
    D_TESTS_EVENT_CHECK(d_event_step_is_skip(NULL));
    D_TESTS_EVENT_CHECK(d_event_step_invoke(&skip, NULL) ==
                        (int32_t)D_VERDICT_PASS);

#if (D_INTERNAL_EVENT_HANDLER_SEQ == 1)
    {
        struct d_event_seq_state n1;
        struct d_event_seq_state n2;
        struct d_event_seq_state n3;
        struct d_event_seq_state n4;
        struct d_event_step      s;
        char                     alone[8];

        // left zero: seq(consume, b) is consume and b is NOT invoked
        d_tests_event_tag_init(&a, 'a', (int32_t)D_VERDICT_CONSUME);
        d_tests_event_tag_init(&b, 'b', (int32_t)D_VERDICT_PASS);
        d_tests_event_log_reset();
        s = d_event_step_seq(&n1, d_tests_event_tagged(&a),
                             d_tests_event_tagged(&b));
        D_TESTS_EVENT_CHECK(d_event_step_invoke(&s, NULL) ==
                            (int32_t)D_VERDICT_CONSUME);
        D_TESTS_EVENT_CHECK(b.calls == 0);
        D_TESTS_EVENT_CHECK(d_tests_event_logged("a"));

        // both stages run, in order, when the first passes
        d_tests_event_tag_init(&a, 'a', (int32_t)D_VERDICT_PASS);
        d_tests_event_tag_init(&b, 'b', (int32_t)D_VERDICT_CONSUME);
        d_tests_event_log_reset();
        s = d_event_step_seq(&n1, d_tests_event_tagged(&a),
                             d_tests_event_tagged(&b));
        D_TESTS_EVENT_CHECK(d_event_step_invoke(&s, NULL) ==
                            (int32_t)D_VERDICT_CONSUME);
        D_TESTS_EVENT_CHECK(d_tests_event_logged("ab"));

        // skip is a unit on both sides, for either verdict
        for (int32_t v = 0; v <= 1; ++v)
        {
            const struct d_event_step ta = d_event_step_make(
                &d_tests_event_step_tag, &a);
            int32_t                   verdict;

            d_tests_event_tag_init(&a, 'a', v);
            d_tests_event_log_reset();
            verdict = d_event_step_invoke(&ta, NULL);
            memcpy(alone, g_log, sizeof(alone) - 1u);
            alone[sizeof(alone) - 1u] = '\0';

            d_tests_event_log_reset();
            s = d_event_step_seq(&n1, skip, ta);
            D_TESTS_EVENT_CHECK( (d_event_step_invoke(&s, NULL) == verdict) &&
                                 (d_tests_event_logged(alone)) );

            d_tests_event_log_reset();
            s = d_event_step_seq(&n1, ta, skip);
            D_TESTS_EVENT_CHECK( (d_event_step_invoke(&s, NULL) == verdict) &&
                                 (d_tests_event_logged(alone)) );
        }

        // associativity, over all eight verdict combinations
        for (int m = 0; m < 8; ++m)
        {
            struct d_event_step left;
            struct d_event_step right;
            char                left_log[8];
            int32_t             left_verdict;
            int32_t             right_verdict;

            d_tests_event_tag_init(&a, 'a', (int32_t)(m & 1));
            d_tests_event_tag_init(&b, 'b', (int32_t)((m >> 1) & 1));
            d_tests_event_tag_init(&c, 'c', (int32_t)((m >> 2) & 1));

            d_tests_event_log_reset();
            left = d_event_step_seq(&n2,
                                    d_event_step_seq(&n1,
                                                     d_tests_event_tagged(&a),
                                                     d_tests_event_tagged(&b)),
                                    d_tests_event_tagged(&c));
            left_verdict = d_event_step_invoke(&left, NULL);
            memcpy(left_log, g_log, sizeof(left_log) - 1u);
            left_log[sizeof(left_log) - 1u] = '\0';

            d_tests_event_log_reset();
            right = d_event_step_seq(&n3,
                                     d_tests_event_tagged(&b),
                                     d_tests_event_tagged(&c));
            right = d_event_step_seq(&n4, d_tests_event_tagged(&a), right);
            right_verdict = d_event_step_invoke(&right, NULL);

            D_TESTS_EVENT_CHECK( (left_verdict == right_verdict) &&
                                 (d_tests_event_logged(left_log)) );
        }

        // a NULL node degrades to the unit
        s = d_event_step_seq(NULL, d_tests_event_tagged(&a),
                             d_tests_event_tagged(&b));
        D_TESTS_EVENT_CHECK(d_event_step_is_skip(&s));
    }
#endif  // D_INTERNAL_EVENT_HANDLER_SEQ

    // fold: stops at the first consume; the unit is neither called nor counted
    {
        struct d_event_step word[4];
        struct d_event_step plain[2];
        struct d_event_step padded[5];
        size_t              invoked        = 99u;
        size_t              invoked_plain  = 0u;
        size_t              invoked_padded = 0u;

        d_tests_event_tag_init(&a, 'a', (int32_t)D_VERDICT_PASS);
        d_tests_event_tag_init(&b, 'b', (int32_t)D_VERDICT_CONSUME);
        d_tests_event_tag_init(&c, 'c', (int32_t)D_VERDICT_PASS);
        word[0] = d_tests_event_tagged(&a);
        word[1] = skip;
        word[2] = d_tests_event_tagged(&b);
        word[3] = d_tests_event_tagged(&c);

        d_tests_event_log_reset();
        D_TESTS_EVENT_CHECK(d_event_step_fold(word, 4u, NULL, &invoked) ==
                            (int32_t)D_VERDICT_CONSUME);
        D_TESTS_EVENT_CHECK(invoked == 2u);
        D_TESTS_EVENT_CHECK(d_tests_event_logged("ab"));
        D_TESTS_EVENT_CHECK(c.calls == 0);

        // inserting the unit changes neither the verdict nor the count
        plain[0]  = d_tests_event_tagged(&a);
        plain[1]  = d_tests_event_tagged(&c);
        padded[0] = skip;
        padded[1] = d_tests_event_tagged(&a);
        padded[2] = skip;
        padded[3] = d_tests_event_tagged(&c);
        padded[4] = skip;
        D_TESTS_EVENT_CHECK(
            d_event_step_fold(plain, 2u, NULL, &invoked_plain) ==
            d_event_step_fold(padded, 5u, NULL, &invoked_padded));
        D_TESTS_EVENT_CHECK( (invoked_plain == 2u) &&
                             (invoked_padded == 2u) );

        // a missing array folds as the empty word, and the count is written
        invoked = 99u;
        D_TESTS_EVENT_CHECK( (d_event_step_fold(NULL, 3u, NULL, &invoked) ==
                              (int32_t)D_VERDICT_PASS) &&
                             (invoked == 0u) );
    }

#if (D_INTERNAL_EVENT_HANDLER_ADAPTER == 1)
    // the adapter calls back with its own context, never the payload
    {
        struct d_event_callback_state cb;
        struct d_event_step           cs;
        int                           target  = 0;
        int                           payload = 7;

        d_event_callback_state_init(&cb, &d_tests_event_callback, &target);
        cs                 = d_event_step_from_callback(&cb);
        g_callback_calls   = 0;
        g_callback_context = NULL;
        D_TESTS_EVENT_CHECK(d_event_step_invoke(&cs, &payload) ==
                            (int32_t)D_VERDICT_PASS);
        D_TESTS_EVENT_CHECK( (g_callback_calls == 1) &&
                             (g_callback_context == &target) );

        // no state, or no callback, is the unit
        cs = d_event_step_from_callback(NULL);
        D_TESTS_EVENT_CHECK(d_event_step_is_skip(&cs));
        d_event_callback_state_init(&cb, NULL, &target);
        cs = d_event_step_from_callback(&cb);
        D_TESTS_EVENT_CHECK(d_event_step_is_skip(&cs));
        D_TESTS_EVENT_CHECK(d_event_callback_step_fn(&payload, NULL) ==
                            (int32_t)D_VERDICT_PASS);
    }
#endif  // D_INTERNAL_EVENT_HANDLER_ADAPTER

    return;
}


///////////////////////////////////////////////////////////////////////////////
///        II.   THE TABLE                                                  ///
///////////////////////////////////////////////////////////////////////////////

static void
d_tests_event_table_fixed(void)
{
    struct d_event_entry     entries[5];
    uint32_t                 head[1];
    uint32_t                 tail[1];
    struct d_event_table     table;
    struct d_event_table     zero;
    struct d_tests_event_tag t[6];
    struct d_tests_event_ids ids;
    d_handler_id             id    = D_HANDLER_ID_NULL;
    const d_event_key        k1    = d_event_key_of_name("k1");
    const d_event_key        k2    = d_event_key_of_name("k2");

    // every piece of storage is required
    D_TESTS_EVENT_CHECK(d_event_table_init_fixed(NULL, entries, 5u, head,
                                                 tail, 1u) ==
                        D_EVENT_ERR_NULL);
    D_TESTS_EVENT_CHECK(d_event_table_init_fixed(&table, NULL, 5u, head,
                                                 tail, 1u) ==
                        D_EVENT_ERR_NULL);
    D_TESTS_EVENT_CHECK(d_event_table_init_fixed(&table, entries, 5u, NULL,
                                                 tail, 1u) ==
                        D_EVENT_ERR_NULL);
    D_TESTS_EVENT_CHECK(d_event_table_init_fixed(&table, entries, 5u, head,
                                                 tail, 0u) ==
                        D_EVENT_ERR_NULL);
    D_TESTS_EVENT_CHECK(d_event_table_init_fixed(&table, entries, 5u, head,
                                                 tail, 1u) == D_EVENT_OK);

    for (int i = 0; i < 6; ++i)
    {
        d_tests_event_tag_init(&t[i], (char)('a' + i),
                               (int32_t)D_VERDICT_PASS);
    }

    // one bucket, two interleaved words; ids count up from 1 in bind order
    D_TESTS_EVENT_CHECK( (d_event_table_bind(&table, k1,
                                             d_tests_event_tagged(&t[0]),
                                             &d_tests_event_free,
                                             &id) == D_EVENT_OK) &&
                         (id == 1u) );
    (void)d_event_table_bind(&table, k2, d_tests_event_tagged(&t[1]), NULL,
                             &id);
    (void)d_event_table_bind(&table, k1, d_tests_event_tagged(&t[2]),
                             &d_tests_event_free, &id);
    (void)d_event_table_bind(&table, k2, d_tests_event_tagged(&t[3]),
                             &d_tests_event_free, &id);
    D_TESTS_EVENT_CHECK( (d_event_table_bind(&table, k1,
                                             d_tests_event_tagged(&t[4]),
                                             NULL, &id) == D_EVENT_OK) &&
                         (id == 5u) );
    D_TESTS_EVENT_CHECK( (d_event_table_count(&table) == 5u)         &&
                         (d_event_table_enabled_count(&table) == 5u) &&
                         (d_event_table_count_for(&table, k1) == 3u) &&
                         (d_event_table_count_for(&table, k2) == 2u) &&
                         (d_event_table_key_count(&table) == 2u) );
    D_TESTS_EVENT_CHECK(d_event_table_has_entries_for(&table, k1));
    D_TESTS_EVENT_CHECK(!d_event_table_has_entries_for(
                            &table, d_event_key_of_name("none")));

    // a full fixed arena refuses rather than grows
    D_TESTS_EVENT_CHECK(d_event_table_bind(&table, k1,
                                           d_tests_event_tagged(&t[5]),
                                           NULL, NULL) ==
                        D_EVENT_ERR_CAPACITY);
    D_TESTS_EVENT_CHECK(d_event_table_count(&table) == 5u);

    // word access skips the other key sharing the bucket
    {
        const d_handler_id word[3] = { 1u, 3u, 5u };

        D_TESTS_EVENT_CHECK(d_tests_event_word_is(&table, k1, word, 3u));
    }

    // unbind runs the hook once, and the id is gone for good
    g_freed = 0;
    D_TESTS_EVENT_CHECK(d_event_table_unbind(&table, 3u) == D_EVENT_OK);
    D_TESTS_EVENT_CHECK( (g_freed == 1) &&
                         (g_freed_last == &t[2]) );
    D_TESTS_EVENT_CHECK( (!d_event_table_contains(&table, 3u)) &&
                         (d_event_table_find(&table, 3u) == NULL) );
    D_TESTS_EVENT_CHECK(d_event_table_unbind(&table, 3u) ==
                        D_EVENT_ERR_NOT_FOUND);
    D_TESTS_EVENT_CHECK(d_event_table_unbind(&table, D_HANDLER_ID_NULL) ==
                        D_EVENT_ERR_NOT_FOUND);

    // a reused slot keeps bind order: the new letter ends its word
    D_TESTS_EVENT_CHECK( (d_event_table_bind(&table, k1,
                                             d_tests_event_tagged(&t[5]),
                                             NULL, &id) == D_EVENT_OK) &&
                         (id == 6u) );
    {
        const d_handler_id word[3] = { 1u, 5u, 6u };

        D_TESTS_EVENT_CHECK(d_tests_event_word_is(&table, k1, word, 3u));
    }

    // global iteration is ascending id, not slot order
    memset(&ids, 0, sizeof(ids));
    D_TESTS_EVENT_CHECK(d_event_table_for_each(&table, &d_tests_event_collect,
                                               &ids) == 5u);
    D_TESTS_EVENT_CHECK( (ids.ids[0] == 1u) && (ids.ids[1] == 2u) &&
                         (ids.ids[2] == 4u) && (ids.ids[3] == 5u) &&
                         (ids.ids[4] == 6u) );

    // a visitor returning false stops the walk after that entry
    memset(&ids, 0, sizeof(ids));
    ids.stop_after = 2u;
    D_TESTS_EVENT_CHECK( (d_event_table_for_each(&table,
                                                 &d_tests_event_collect,
                                                 &ids) == 2u) &&
                         (ids.ids[1] == 2u) );

    // one word, in word order
    memset(&ids, 0, sizeof(ids));
    D_TESTS_EVENT_CHECK( (d_event_table_for_each_key(&table, k1,
                                                     &d_tests_event_collect,
                                                     &ids) == 3u) &&
                         (ids.ids[0] == 1u) && (ids.ids[1] == 5u) &&
                         (ids.ids[2] == 6u) );

    // the mask's three-way results, with the count following the mask
    D_TESTS_EVENT_CHECK(d_event_table_disable(&table, 1u) == D_EVENT_OK);
    D_TESTS_EVENT_CHECK(d_event_table_disable(&table, 1u) ==
                        D_EVENT_ERR_STATE);
    D_TESTS_EVENT_CHECK( (!d_event_table_is_enabled(&table, 1u)) &&
                         (d_event_table_enabled_count(&table) == 4u) );
    D_TESTS_EVENT_CHECK(d_event_table_count_for(&table, k1) == 3u);
    D_TESTS_EVENT_CHECK(d_event_table_enable(&table, 1u) == D_EVENT_OK);
    D_TESTS_EVENT_CHECK(d_event_table_enable(&table, 1u) ==
                        D_EVENT_ERR_STATE);
    D_TESTS_EVENT_CHECK(d_event_table_enable(&table, 99u) ==
                        D_EVENT_ERR_NOT_FOUND);
    D_TESTS_EVENT_CHECK(d_event_table_disable(&table, 99u) ==
                        D_EVENT_ERR_NOT_FOUND);
    D_TESTS_EVENT_CHECK( (d_event_table_is_enabled(&table, 1u)) &&
                         (d_event_table_enabled_count(&table) == 5u) );

    // at() hides out-of-range indices; slot 0 still holds id 1
    D_TESTS_EVENT_CHECK(d_event_table_at(&table, 5u) == NULL);
    D_TESTS_EVENT_CHECK( (d_event_table_at(&table, 0u) != NULL) &&
                         (d_event_table_at(&table, 0u)->id == 1u) );

    D_TESTS_EVENT_CHECK(d_event_table_load_factor(&table) == 5.0);
    D_TESTS_EVENT_CHECK(d_event_table_footprint(&table) ==
                        sizeof(struct d_event_table));

    // clear_key drops one word, running its hooks
    g_freed = 0;
    D_TESTS_EVENT_CHECK(d_event_table_clear_key(&table, k2) == 2u);
    D_TESTS_EVENT_CHECK( (d_event_table_count_for(&table, k2) == 0u) &&
                         (g_freed == 1) );

    // clear runs every remaining hook once; handler ids are never reused
    g_freed = 0;
    d_event_table_clear(&table);
    D_TESTS_EVENT_CHECK( (d_event_table_count(&table) == 0u)         &&
                         (d_event_table_enabled_count(&table) == 0u) &&
                         (g_freed == 1) );
    D_TESTS_EVENT_CHECK( (d_event_table_bind(&table, k1,
                                             d_tests_event_tagged(&t[0]),
                                             NULL, &id) == D_EVENT_OK) &&
                         (id == 7u) );

    // dispose twice, and on a zeroed table, is safe
    d_event_table_dispose(&table);
    d_event_table_dispose(&table);
    D_TESTS_EVENT_CHECK(d_event_table_count(&table) == 0u);
    memset(&zero, 0, sizeof(zero));
    d_event_table_dispose(&zero);
    d_event_table_dispose(NULL);

    // a zeroed table refuses a bind and reports empty everywhere
    D_TESTS_EVENT_CHECK(d_event_table_bind(&zero, k1,
                                           d_tests_event_tagged(&t[0]),
                                           NULL, NULL) == D_EVENT_ERR_STATE);
    D_TESTS_EVENT_CHECK(d_event_table_first(&zero, k1) == D_EVENT_INDEX_NONE);
    D_TESTS_EVENT_CHECK(d_event_table_for_each(&zero, &d_tests_event_collect,
                                               &ids) == 0u);
    D_TESTS_EVENT_CHECK(d_event_table_bind(NULL, k1, d_event_step_skip(),
                                           NULL, NULL) == D_EVENT_ERR_NULL);

    return;
}

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

static void
d_tests_event_table_alloc(void)
{
    struct d_event_table     table;
    struct d_event_table     other;
    struct d_event_entry     fixed_entries[2];
    uint32_t                 fixed_head[1];
    uint32_t                 fixed_tail[1];
    struct d_tests_event_tag t;
    bool                     ok     = true;
    const d_event_key        base   = (d_event_key)1000u;

    d_tests_event_tag_init(&t, 'x', (int32_t)D_VERDICT_PASS);
    D_TESTS_EVENT_CHECK(d_event_table_init(NULL, 0u, 0u) == D_EVENT_ERR_NULL);

    // zero arguments select the defaults, rounded to a prime
    D_TESTS_EVENT_CHECK(d_event_table_init(&other, 0u, 0u) == D_EVENT_OK);
    D_TESTS_EVENT_CHECK( (other.capacity == D_EVENT_TABLE_DEFAULT_CAPACITY) &&
                         (other.bucket_count ==
                          d_event_next_prime(D_EVENT_TABLE_DEFAULT_BUCKETS)) );
    d_event_table_dispose(&other);

    // binding far past capacity grows the arena and rehashes
    D_TESTS_EVENT_CHECK(d_event_table_init(&table, 2u, 3u) == D_EVENT_OK);
    D_TESTS_EVENT_CHECK( (table.capacity == 2u) &&
                         (table.bucket_count == 3u) );

    for (unsigned i = 0u; i < 200u; ++i)
    {
        ok = ok && (d_event_table_bind(&table, base + (d_event_key)(i % 7u),
                                       d_tests_event_tagged(&t), NULL,
                                       NULL) == D_EVENT_OK);
    }

    D_TESTS_EVENT_CHECK(ok);
    D_TESTS_EVENT_CHECK( (d_event_table_count(&table) == 200u) &&
                         (table.capacity >= 200u) );
    D_TESTS_EVENT_CHECK( (table.bucket_count > 3u) &&
                         (d_event_next_prime(table.bucket_count) ==
                          table.bucket_count) );
    D_TESTS_EVENT_CHECK(
        (d_event_table_count(&table) * (size_t)D_EVENT_TABLE_LOAD_FACTOR_DEN) <=
        (table.bucket_count * (size_t)D_EVENT_TABLE_LOAD_FACTOR_NUM));

    // every word is still in bind order
    for (unsigned k = 0u; k < 7u; ++k)
    {
        D_TESTS_EVENT_CHECK(d_tests_event_word_ordered(
            &table, base + (d_event_key)k, (k < 4u) ? 29u : 28u));
    }

    // churn: free every third letter, bind as many again, order holds
    for (d_handler_id i = 1u; i <= 200u; i += 3u)
    {
        ok = ok && (d_event_table_unbind(&table, i) == D_EVENT_OK);
    }

    for (unsigned i = 0u; i < 67u; ++i)
    {
        ok = ok && (d_event_table_bind(&table, base + (d_event_key)(i % 7u),
                                       d_tests_event_tagged(&t), NULL,
                                       NULL) == D_EVENT_OK);
    }

    D_TESTS_EVENT_CHECK( (ok) &&
                         (d_event_table_count(&table) == 200u) );

    // an explicit rehash rounds up to a prime and keeps every word
    D_TESTS_EVENT_CHECK( (d_event_table_rehash(&table, 50u) == D_EVENT_OK) &&
                         (table.bucket_count == 53u) );

    for (unsigned k = 0u; k < 7u; ++k)
    {
        D_TESTS_EVENT_CHECK(d_tests_event_word_ordered(
            &table, base + (d_event_key)k,
            d_event_table_count_for(&table, base + (d_event_key)k)));
    }

    // reserve grows and never shrinks
    D_TESTS_EVENT_CHECK( (d_event_table_reserve(&table, 10u) == D_EVENT_OK) &&
                         (table.capacity >= 200u) );
    D_TESTS_EVENT_CHECK( (d_event_table_reserve(&table, 1000u) ==
                          D_EVENT_OK) &&
                         (table.capacity == 1000u) );
    D_TESTS_EVENT_CHECK(d_event_table_footprint(&table) ==
                        ( sizeof(struct d_event_table) +
                          (1000u * sizeof(struct d_event_entry)) +
                          (2u * 53u * sizeof(uint32_t)) ));

#if (D_INTERNAL_EVENT_TABLE_STATS == 1)
    {
        const struct d_event_table_stats stats =
            d_event_table_get_stats(&table);
        size_t                           longest = 0u;
        struct d_event_table_stats       none;

        for (unsigned k = 0u; k < 7u; ++k)
        {
            const size_t length = d_event_table_count_for(
                &table, base + (d_event_key)k);

            longest = (length > longest) ? length : longest;
        }

        D_TESTS_EVENT_CHECK( (stats.total_buckets == 53u)            &&
                             (stats.total_entries == 200u)           &&
                             (stats.enabled_entries == 200u)         &&
                             (stats.key_count == 7u)                 &&
                             (stats.max_entries_per_key == longest)  &&
                             (stats.used_buckets >= 1u)              &&
                             (stats.used_buckets <= 7u) );
        D_TESTS_EVENT_CHECK(stats.average_entries_per_key ==
                            ((double)200u / (double)7u));
        D_TESTS_EVENT_CHECK(stats.load_factor ==
                            d_event_table_load_factor(&table));

        // a zeroed table yields the zero record
        memset(&other, 0, sizeof(other));
        none = d_event_table_get_stats(&other);
        D_TESTS_EVENT_CHECK( (none.total_buckets == 0u) &&
                             (none.key_count == 0u)     &&
                             (none.load_factor == 0.0) );
    }
#endif  // D_INTERNAL_EVENT_TABLE_STATS

    d_event_table_dispose(&table);

    // a fixed table has nothing of its own to rehash or grow
    (void)d_event_table_init_fixed(&other, fixed_entries, 2u, fixed_head,
                                   fixed_tail, 1u);
    D_TESTS_EVENT_CHECK(d_event_table_rehash(&other, 7u) == D_EVENT_ERR_STATE);
    D_TESTS_EVENT_CHECK(d_event_table_reserve(&other, 7u) ==
                        D_EVENT_ERR_STATE);
    d_event_table_dispose(&other);

    return;
}

#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

#if (D_INTERNAL_EVENT_TABLE_MERGE == 1)

static void
d_tests_event_merge(void)
{
    struct d_event_entry     ea[8];
    struct d_event_entry     eb[8];
    struct d_event_entry     ec[8];
    struct d_event_entry     ed[2];
    uint32_t                 ha[3], ta[3], hb[3], tb[3], hc[3], tc[3];
    uint32_t                 hd[1], td[1];
    struct d_event_table     a;
    struct d_event_table     b;
    struct d_event_table     c;
    struct d_event_table     d;
    struct d_tests_event_tag p, q, r, s;
    size_t                   merged = 99u;
    const d_event_key        k1     = d_event_key_of_name("m1");
    const d_event_key        k2     = d_event_key_of_name("m2");

    (void)d_event_table_init_fixed(&a, ea, 8u, ha, ta, 3u);
    (void)d_event_table_init_fixed(&b, eb, 8u, hb, tb, 3u);
    (void)d_event_table_init_fixed(&c, ec, 8u, hc, tc, 3u);
    (void)d_event_table_init_fixed(&d, ed, 2u, hd, td, 1u);
    d_tests_event_tag_init(&p, 'p', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&q, 'q', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&r, 'r', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&s, 's', (int32_t)D_VERDICT_PASS);

    (void)d_event_table_bind(&a, k1, d_tests_event_tagged(&p), NULL, NULL);
    (void)d_event_table_bind(&a, k1, d_tests_event_tagged(&q), NULL, NULL);
    (void)d_event_table_bind(&b, k1, d_tests_event_tagged(&r),
                             &d_tests_event_free, NULL);
    (void)d_event_table_bind(&b, k2, d_tests_event_tagged(&s),
                             &d_tests_event_free, NULL);
    (void)d_event_table_disable(&b, 2u);

    // this table's letters first, then the other's; fresh ids; mask travels
    D_TESTS_EVENT_CHECK( (d_event_table_merge(&a, &b, &merged) ==
                          D_EVENT_OK) &&
                         (merged == 2u) );
    d_tests_event_log_reset();
    (void)d_event_table_for_each_key(&a, k1, &d_tests_event_log_entry, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("pqr"));
    D_TESTS_EVENT_CHECK( (d_event_table_contains(&a, 3u))    &&
                         (d_event_table_contains(&a, 4u))    &&
                         (!d_event_table_is_enabled(&a, 4u)) &&
                         (d_event_table_enabled_count(&a) == 3u) );

    // merged letters borrow their state: unbinding one runs no hook
    g_freed = 0;
    D_TESTS_EVENT_CHECK( (d_event_table_unbind(&a, 3u) == D_EVENT_OK) &&
                         (g_freed == 0) );

    // the other table is unchanged
    D_TESTS_EVENT_CHECK( (d_event_table_count(&b) == 2u)    &&
                         (d_event_table_contains(&b, 1u))   &&
                         (!d_event_table_is_enabled(&b, 2u)) );

    // the empty table is the identity, on either side
    D_TESTS_EVENT_CHECK( (d_event_table_merge(&a, &c, &merged) ==
                          D_EVENT_OK) &&
                         (merged == 0u) &&
                         (d_event_table_count(&a) == 3u) );
    D_TESTS_EVENT_CHECK( (d_event_table_merge(&c, &b, &merged) ==
                          D_EVENT_OK) &&
                         (merged == 2u) );
    d_tests_event_log_reset();
    (void)d_event_table_for_each(&c, &d_tests_event_log_entry, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("rs"));

    // merging a table into itself doubles each word, once
    D_TESTS_EVENT_CHECK( (d_event_table_merge(&c, &c, &merged) ==
                          D_EVENT_OK) &&
                         (merged == 2u) &&
                         (d_event_table_count_for(&c, k1) == 2u) &&
                         (d_event_table_count_for(&c, k2) == 2u) );

    // a full fixed table stops part-way and says how far it got
    (void)d_event_table_bind(&d, k1, d_tests_event_tagged(&p), NULL, NULL);
    D_TESTS_EVENT_CHECK( (d_event_table_merge(&d, &b, &merged) ==
                          D_EVENT_ERR_CAPACITY) &&
                         (merged == 1u) &&
                         (d_event_table_count(&d) == 2u) );
    D_TESTS_EVENT_CHECK( (d_event_table_merge(NULL, &b, &merged) ==
                          D_EVENT_ERR_NULL) &&
                         (merged == 0u) );

    d_event_table_dispose(&a);
    d_event_table_dispose(&c);
    d_event_table_dispose(&d);
    g_freed = 0;
    d_event_table_dispose(&b);
    D_TESTS_EVENT_CHECK(g_freed == 2);

    return;
}

#endif  // D_INTERNAL_EVENT_TABLE_MERGE


///////////////////////////////////////////////////////////////////////////////
///        III.  DISPATCH, SNAPSHOTS, RUN, AND THE FUSED WORD               ///
///////////////////////////////////////////////////////////////////////////////

static void
d_tests_event_dispatch(void)
{
    struct d_event_entry      entries[16];
    uint32_t                  head[5];
    uint32_t                  tail[5];
    struct d_event_registry   r;
    struct d_tests_event_tag  a, b, c, x, y, z;
    struct d_tests_event_num  n1 = { 'a', 1, false };
    struct d_tests_event_num  n2 = { 'b', 10, false };
    struct d_dispatch_result  result;
    struct d_dispatch_result  again;
    struct d_event_occurrence occurrence;
    d_handler_id              middle = D_HANDLER_ID_NULL;
    int                       value  = 5;
    const d_event_key         k      = d_event_key_of_name("on_tick");
    const d_event_key         kmask  = d_event_key_of_name("on_mask");
    const d_event_key         knum   = d_event_key_of_name("on_num");

    D_TESTS_EVENT_CHECK(d_event_registry_init_fixed(&r, entries, 16u, head,
                                                    tail, 5u) == D_EVENT_OK);

    // the empty word: no invocation, pass, and not an error
    result = d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK( (result.invoked == 0u) &&
                         (result.outcome == (int32_t)D_VERDICT_PASS) &&
                         (!d_dispatch_result_consumed(&result)) );
    result = d_event_registry_dispatch(NULL, k, NULL);
    D_TESTS_EVENT_CHECK(result.invoked == 0u);

    // word order, and the first consume cuts off the rest
    d_tests_event_tag_init(&a, 'a', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&b, 'b', (int32_t)D_VERDICT_CONSUME);
    d_tests_event_tag_init(&c, 'c', (int32_t)D_VERDICT_PASS);
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&a), NULL, NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&b), NULL, NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&c), NULL, NULL);
    d_tests_event_log_reset();
    result = d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("ab"))          &&
                         (result.invoked == 2u)                &&
                         (d_dispatch_result_consumed(&result)) &&
                         (c.calls == 0) );

    // the occurrence form is the same fold
    occurrence = d_event_occurrence_make(k, d_event_payload_make(NULL, 0u,
                                                                 0u));
    d_tests_event_log_reset();
    again = d_event_registry_dispatch_occurrence(&r, &occurrence);
    D_TESTS_EVENT_CHECK( (again.invoked == result.invoked) &&
                         (again.outcome == result.outcome) &&
                         (d_tests_event_logged("ab")) );
    again = d_event_registry_dispatch_occurrence(&r, NULL);
    D_TESTS_EVENT_CHECK(again.invoked == 0u);

    // the mask law: masking a letter of a passing word keeps its verdict
    d_tests_event_tag_init(&x, 'x', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&y, 'y', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&z, 'z', (int32_t)D_VERDICT_PASS);
    (void)d_event_registry_bind(&r, kmask, d_tests_event_tagged(&x), NULL,
                                NULL);
    (void)d_event_registry_bind(&r, kmask, d_tests_event_tagged(&y), NULL,
                                &middle);
    (void)d_event_registry_bind(&r, kmask, d_tests_event_tagged(&z), NULL,
                                NULL);
    (void)d_event_table_disable(&r.table, middle);
    d_tests_event_log_reset();
    result = d_event_registry_dispatch(&r, kmask, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("xz"))                   &&
                         (result.invoked == 2u)                         &&
                         (result.outcome == (int32_t)D_VERDICT_PASS)    &&
                         (d_event_table_count_for(&r.table, kmask) == 3u) );
    (void)d_event_table_enable(&r.table, middle);
    d_tests_event_log_reset();
    result = d_event_registry_dispatch(&r, kmask, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("xyz")) &&
                         (result.invoked == 3u)        &&
                         (result.outcome == (int32_t)D_VERDICT_PASS) );

    // the payload reaches every letter, untouched by the core
    (void)d_event_registry_bind(&r, knum, d_tests_event_numbered(&n1), NULL,
                                NULL);
    (void)d_event_registry_bind(&r, knum, d_tests_event_numbered(&n2), NULL,
                                NULL);
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, knum, &value);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("a5b6")) &&
                         (value == 16) );

    d_event_registry_dispose(&r);

    return;
}

/*
d_tests_event_snapshot
  "A handler that binds or unbinds during its own dispatch does not change
the word being folded; the edit is visible to the next occurrence."
*/
static void
d_tests_event_snapshot(void)
{
    struct d_event_entry       entries[16];
    uint32_t                   head[5];
    uint32_t                   tail[5];
    struct d_event_registry    r;
    struct d_tests_event_actor u, l, o, f, g, h, w;
    struct d_tests_event_tag   n, p, e;
    d_handler_id               id  = D_HANDLER_ID_NULL;
    const d_event_key          k   = d_event_key_of_name("snap");
    const d_event_key          kk  = d_event_key_of_name("outer");
    const d_event_key          kj  = d_event_key_of_name("inner");

    // a letter bound mid-dispatch joins the NEXT occurrence
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_tag_init(&n, 'n', (int32_t)D_VERDICT_PASS);
    d_tests_event_actor_init(&u, 'a', D_TESTS_EVENT_ACT_BIND, &r);
    u.key       = k;
    u.bind_step = d_tests_event_tagged(&n);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&u), NULL, NULL);
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("a")) &&
                         (d_event_table_count_for(&r.table, k) == 2u) );
    u.action = D_TESTS_EVENT_ACT_NONE;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("an"));
    d_event_registry_dispose(&r);

    // a later letter unbound mid-dispatch still runs, with its state alive;
    // it is gone from every query at once, and its hook runs afterwards
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_actor_init(&u, 'u', D_TESTS_EVENT_ACT_UNBIND, &r);
    d_tests_event_actor_init(&l, 'l', D_TESTS_EVENT_ACT_NONE, &r);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&u), NULL, NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&l),
                                &d_tests_event_free, &u.target);
    g_freed      = 0;
    g_freed_last = NULL;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("ul"));
    D_TESTS_EVENT_CHECK( (u.target_gone) &&
                         (l.freed_seen == 0) );
    D_TESTS_EVENT_CHECK( (g_freed == 1) &&
                         (g_freed_last == &l) &&
                         (d_event_table_count_for(&r.table, k) == 1u) );
    u.action = D_TESTS_EVENT_ACT_NONE;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("u"));
    d_event_registry_dispose(&r);

    // a one-shot letter unbinds itself: it runs once, its hook after
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_actor_init(&o, 'o', D_TESTS_EVENT_ACT_UNBIND, &r);
    d_tests_event_tag_init(&p, 'p', (int32_t)D_VERDICT_PASS);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&o),
                                &d_tests_event_free, &o.target);
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&p), NULL, NULL);
    g_freed = 0;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("op")) &&
                         (g_freed == 1) );
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("p"));
    d_event_registry_dispose(&r);

    // a clear mid-dispatch leaves the running word intact
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_actor_init(&u, 'c', D_TESTS_EVENT_ACT_CLEAR, &r);
    d_tests_event_actor_init(&l, 'd', D_TESTS_EVENT_ACT_NONE, &r);
    d_tests_event_tag_init(&e, 'e', (int32_t)D_VERDICT_PASS);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&u), NULL, NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&l),
                                &d_tests_event_free, NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&e), NULL, NULL);
    g_freed = 0;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("cde"))             &&
                         (d_event_table_count(&r.table) == 0u)     &&
                         (g_freed == 1)                            &&
                         (l.freed_seen == 0) );

    // after the reap the table is whole again
    D_TESTS_EVENT_CHECK( (d_event_registry_bind(&r, k,
                                                d_tests_event_tagged(&e),
                                                NULL, &id) == D_EVENT_OK) &&
                         (d_event_table_count(&r.table) == 1u) );
    d_event_registry_dispose(&r);

    // a dispose mid-dispatch completes the word, then disposes
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_actor_init(&u, 'x', D_TESTS_EVENT_ACT_DISPOSE, &r);
    d_tests_event_actor_init(&l, 'y', D_TESTS_EVENT_ACT_NONE, &r);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&u), NULL, NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_acting(&l),
                                &d_tests_event_free, NULL);
    g_freed = 0;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, k, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("xy"))    &&
                         (g_freed == 1)                  &&
                         (r.table.entries == NULL)       &&
                         (r.table.bucket_count == 0u) );

    // nested: a letter unbound inside a nested dispatch is still folded by
    // the outer one, which began before the unbind
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_actor_init(&f, 'f', D_TESTS_EVENT_ACT_FIRE, &r);
    d_tests_event_actor_init(&g, 'g', D_TESTS_EVENT_ACT_NONE, &r);
    d_tests_event_actor_init(&h, 'h', D_TESTS_EVENT_ACT_UNBIND, &r);
    f.key = kj;
    (void)d_event_registry_bind(&r, kk, d_tests_event_acting(&f), NULL, NULL);
    (void)d_event_registry_bind(&r, kk, d_tests_event_acting(&g),
                                &d_tests_event_free, &h.target);
    (void)d_event_registry_bind(&r, kj, d_tests_event_acting(&h), NULL, NULL);
    g_freed = 0;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, kk, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("fhg")) &&
                         (g.freed_seen == 0)           &&
                         (g_freed == 1)                &&
                         (d_event_table_count_for(&r.table, kk) == 1u) );
    d_event_registry_dispose(&r);

    // nested: a letter unbound BEFORE a nested dispatch began is not in it
    (void)d_event_registry_init_fixed(&r, entries, 16u, head, tail, 5u);
    d_tests_event_actor_init(&u, 'u', D_TESTS_EVENT_ACT_UNBIND, &r);
    d_tests_event_actor_init(&f, 'f', D_TESTS_EVENT_ACT_FIRE, &r);
    d_tests_event_actor_init(&w, 'w', D_TESTS_EVENT_ACT_NONE, &r);
    f.key = kj;
    (void)d_event_registry_bind(&r, kk, d_tests_event_acting(&u), NULL, NULL);
    (void)d_event_registry_bind(&r, kk, d_tests_event_acting(&f), NULL, NULL);
    (void)d_event_registry_bind(&r, kj, d_tests_event_acting(&w),
                                &d_tests_event_free, &u.target);
    g_freed = 0;
    d_tests_event_log_reset();
    (void)d_event_registry_dispatch(&r, kk, NULL);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("uf")) &&
                         (g_freed == 1) );
    d_event_registry_dispose(&r);

    return;
}

#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)

static void
d_tests_event_run(void)
{
    struct d_event_entry     entries[8];
    uint32_t                 head[3];
    uint32_t                 tail[3];
    struct d_event_registry  r;
    struct d_tests_event_num p     = { 'p', 1, true };
    struct d_tests_event_num q     = { 'q', 100, false };
    struct d_tests_event_num z     = { 'z', 0, false };
    int                      v[3]  = { 0, 1, 2 };
    struct d_run_result      run;
    const d_event_key        k     = d_event_key_of_name("trace");
    const d_event_key        k0    = d_event_key_of_name("empty");

    (void)d_event_registry_init_fixed(&r, entries, 8u, head, tail, 3u);
    (void)d_event_registry_bind(&r, k, d_tests_event_numbered(&p), NULL,
                                NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_numbered(&q), NULL,
                                NULL);

    // each block is an occurrence, dispatched in place, in trace order
    d_tests_event_log_reset();
    run = d_event_registry_run(&r, k, v, sizeof(int), 3u);
    D_TESTS_EVENT_CHECK( (run.occurrences == 3u)      &&
                         (run.handlers_invoked == 4u) &&
                         (run.consumed_count == 2u) );
    D_TESTS_EVENT_CHECK(d_tests_event_logged("p0p1q2p2"));
    D_TESTS_EVENT_CHECK( (v[0] == 1) && (v[1] == 102) && (v[2] == 3) );

    // a trace of empty payloads passes NULL throughout
    (void)d_event_registry_bind(&r, k0, d_tests_event_numbered(&z), NULL,
                                NULL);
    d_tests_event_log_reset();
    run = d_event_registry_run(&r, k0, NULL, 0u, 2u);
    D_TESTS_EVENT_CHECK( (run.occurrences == 2u) &&
                         (d_tests_event_logged("z-z-")) );

    run = d_event_registry_run(NULL, k, v, sizeof(int), 3u);
    D_TESTS_EVENT_CHECK(run.occurrences == 0u);
    d_event_registry_dispose(&r);

    return;
}

#endif  // D_INTERNAL_EVENT_REGISTRY_RUN

#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)

static void
d_tests_event_staging(void)
{
    struct d_event_entry     entries[8];
    uint32_t                 head[3];
    uint32_t                 tail[3];
    struct d_event_registry  r;
    struct d_tests_event_num p = { 'p', 1, true };
    struct d_tests_event_num q = { 'q', 100, false };
    struct d_tests_event_tag m;
    struct d_tests_event_tag extra;
    struct d_event_step      steps[4];
    struct d_event_word      word;
    struct d_event_word      cut;
    d_handler_id             masked  = D_HANDLER_ID_NULL;
    d_handler_id             q_id    = D_HANDLER_ID_NULL;
    size_t                   invoked = 0u;
    int                      x       = 1;
    const d_event_key        k       = d_event_key_of_name("fused");

    (void)d_event_registry_init_fixed(&r, entries, 8u, head, tail, 3u);
    d_tests_event_tag_init(&m, 'm', (int32_t)D_VERDICT_PASS);
    d_tests_event_tag_init(&extra, 'n', (int32_t)D_VERDICT_PASS);
    (void)d_event_registry_bind(&r, k, d_tests_event_numbered(&p), NULL,
                                NULL);
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&m), NULL,
                                &masked);
    (void)d_event_registry_bind(&r, k, d_tests_event_numbered(&q), NULL,
                                &q_id);
    (void)d_event_table_disable(&r.table, masked);

    // the snapshot is the enabled letters in word order
    D_TESTS_EVENT_CHECK(d_event_registry_compile_into(&r, k, steps, 4u,
                                                      &word) == D_EVENT_OK);
    D_TESTS_EVENT_CHECK( (d_event_word_size(&word) == 2u) &&
                         (word.flags == 0u)               &&
                         (word.steps[0].state == &p)      &&
                         (word.steps[1].state == &q) );

    // a truncated word is reported, holding the prefix that fitted
    D_TESTS_EVENT_CHECK( (d_event_registry_compile_into(&r, k, steps, 1u,
                                                        &cut) ==
                          D_EVENT_ERR_CAPACITY) &&
                         (cut.count == 1u) &&
                         (cut.steps[0].state == &p) );
    D_TESTS_EVENT_CHECK(d_event_registry_compile_into(NULL, k, steps, 4u,
                                                      &cut) ==
                        D_EVENT_ERR_NULL);
    D_TESTS_EVENT_CHECK(d_event_registry_compile_into(&r, k, NULL, 1u,
                                                      &cut) ==
                        D_EVENT_ERR_NULL);

    // recompile into our own array for the coherence check
    (void)d_event_registry_compile_into(&r, k, steps, 4u, &word);

#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)
    // COHERENCE LAW: run and drive agree on a registry held fixed -- in
    // occurrences, consumes, side effects and their order
    {
        int                   first[4]  = { 0, 1, 2, 3 };
        int                   second[4] = { 0, 1, 2, 3 };
        char                  run_log[64];
        struct d_run_result   run;
        struct d_drive_result drive;

        d_tests_event_log_reset();
        run = d_event_registry_run(&r, k, first, sizeof(int), 4u);
        memcpy(run_log, g_log, sizeof(run_log) - 1u);
        run_log[sizeof(run_log) - 1u] = '\0';

        d_tests_event_log_reset();
        drive = d_event_word_drive(&word, second, sizeof(int), 4u);

        D_TESTS_EVENT_CHECK( (run.occurrences == drive.occurrences)       &&
                             (run.consumed_count == drive.consumed_count) &&
                             (d_tests_event_logged(run_log))              &&
                             (memcmp(first, second, sizeof(first)) == 0) );
    }
#endif  // D_INTERNAL_EVENT_REGISTRY_RUN

    // the fused word ignores every later edit of the registry
    (void)d_event_registry_bind(&r, k, d_tests_event_tagged(&extra), NULL,
                                NULL);
    (void)d_event_table_disable(&r.table, q_id);
    d_tests_event_log_reset();
    D_TESTS_EVENT_CHECK( (d_event_word_run_one(&word, &x, &invoked) ==
                          (int32_t)D_VERDICT_PASS) &&
                         (invoked == 2u) &&
                         (d_tests_event_logged("p1q2")) );
    D_TESTS_EVENT_CHECK(d_event_word_run_one(NULL, &x, &invoked) ==
                        (int32_t)D_VERDICT_PASS);
    D_TESTS_EVENT_CHECK(invoked == 0u);

#if (D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC == 1)
    {
        struct d_event_word owned;
        struct d_event_word empty;

        // the allocating form owns its steps: p and the new letter now
        D_TESTS_EVENT_CHECK( (d_event_registry_compile(&r, k, &owned) ==
                              D_EVENT_OK) &&
                             (owned.count == 2u) &&
                             ((owned.flags & D_EVENT_WORD_FLAG_OWNS_STORAGE)
                              != 0u) );
        d_event_word_dispose(&owned);
        d_event_word_dispose(&owned);
        D_TESTS_EVENT_CHECK( (owned.steps == NULL) &&
                             (owned.count == 0u) );

        // an empty word allocates nothing and owns nothing
        D_TESTS_EVENT_CHECK( (d_event_registry_compile(
                                  &r, d_event_key_of_name("nobody"),
                                  &empty) == D_EVENT_OK) &&
                             (empty.count == 0u)        &&
                             (empty.steps == NULL)      &&
                             (empty.flags == 0u) );
        d_event_word_dispose(&empty);
    }
#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC

    d_event_word_dispose(&word);
    d_event_registry_dispose(&r);

    return;
}

#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING


///////////////////////////////////////////////////////////////////////////////
///        IV.   THE QUEUE AND THE FACADE                                   ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_QUEUE == 1)

// ring wrap-around bookkeeping: occurrences seen, in order, intact
static int      g_wrap_seen = 0;
static int      g_wrap_bad  = 0;
static uint32_t g_wrap_next = 0u;

static int32_t
d_tests_event_step_wrap(
    void* _payload,
    void* _state
)
{
    const unsigned char* const bytes = _payload;
    uint32_t                   n     = 0u;

    (void)_state;
    memcpy(&n, bytes, sizeof(n));
    ++g_wrap_seen;

    // FIFO: occurrences arrive in the order they were queued
    if (n != g_wrap_next)
    {
        ++g_wrap_bad;
    }

    g_wrap_next = n + 1u;

    // the bytes arrive exactly as they were queued
    for (uint32_t j = 0u; j < (n % 37u); ++j)
    {
        if (bytes[4u + j] != (unsigned char)((n * 7u + j) & 0xFFu))
        {
            ++g_wrap_bad;

            break;
        }
    }

    return (int32_t)D_VERDICT_PASS;
}

// d_tests_event_reenter
//   struct: a step that enqueues a follow-up and tries a nested process.
struct d_tests_event_reenter
{
    struct d_event_dispatcher* dispatcher;
    d_event_key                then;
    size_t                     nested;
};

static int32_t
d_tests_event_step_reenter(
    void* _payload,
    void* _state
)
{
    struct d_tests_event_reenter* const reenter = _state;
    int                                 value   = 7;

    (void)_payload;
    d_tests_event_log('r');
    (void)d_event_dispatcher_queue(reenter->dispatcher,
                                   reenter->then,
                                   d_event_payload_make(&value,
                                                        sizeof(value),
                                                        1u));
    reenter->nested = d_event_dispatcher_process_all(reenter->dispatcher,
                                                     NULL);

    return (int32_t)D_VERDICT_PASS;
}

static int32_t
d_tests_event_step_clear_queue(
    void* _payload,
    void* _state
)
{
    struct d_event_dispatcher* const dispatcher = _state;

    (void)_payload;
    d_tests_event_log('c');
    d_event_queue_clear(&dispatcher->queue);

    // refused while this queue is processing: the arena is in use
    d_event_queue_dispose(&dispatcher->queue);

    return (int32_t)D_VERDICT_PASS;
}

static void
d_tests_event_queue(void)
{
    struct d_event_entry         entries[8];
    uint32_t                     head[3];
    uint32_t                     tail[3];
    struct d_event_slot          slots[4];
    union
    {
        unsigned char b[64];
        long double   ld;
        void*         p;
        long long     ll;
    }                            arena;
    struct d_event_dispatcher    d;
    struct d_tests_event_num     p      = { 'p', 1, true };
    struct d_tests_event_num     q      = { 'q', 100, false };
    struct d_tests_event_tag     then;
    struct d_tests_event_reenter reenter;
    struct d_run_result          run;
    struct d_event_payload       view;
    const struct d_event_slot*   slot   = NULL;
    const d_event_key            k      = d_event_key_of_name("on_value");
    const d_event_key            kw     = d_event_key_of_name("on_wrap");
    const d_event_key            kr     = d_event_key_of_name("on_reenter");
    const d_event_key            kt     = d_event_key_of_name("on_then");
    const d_event_key            kc     = d_event_key_of_name("on_clear");
    int                          v      = 5;
    int                          w      = 42;
    unsigned char                big[100];

    D_TESTS_EVENT_CHECK(d_event_dispatcher_init_fixed(&d, entries, 8u, head,
                                                      tail, 3u, slots, 4u,
                                                      arena.b, 64u) ==
                        D_EVENT_OK);
    (void)d_event_dispatcher_bind(&d, k, d_tests_event_numbered(&p), NULL,
                                  NULL);
    (void)d_event_dispatcher_bind(&d, k, d_tests_event_numbered(&q), NULL,
                                  NULL);

    // DEFERRAL COHERENCE: fire(d, k, a) == queue(d, k, a); process(d, 1)
    for (int start = 0; start < 4; ++start)
    {
        int                      x = start;
        int                      y = start;
        char                     fire_log[16];
        struct d_dispatch_result fired;

        d_tests_event_log_reset();
        fired = d_event_dispatcher_fire(&d, k, &x);
        memcpy(fire_log, g_log, sizeof(fire_log) - 1u);
        fire_log[sizeof(fire_log) - 1u] = '\0';

        d_tests_event_log_reset();
        D_TESTS_EVENT_CHECK(d_event_dispatcher_queue(
                                &d, k, d_event_payload_make(&y, sizeof(y),
                                                            1u)) ==
                            D_EVENT_OK);
        D_TESTS_EVENT_CHECK(d_event_dispatcher_process(&d, 1u, &run) == 1u);
        D_TESTS_EVENT_CHECK( (run.occurrences == 1u)                  &&
                             (run.handlers_invoked == fired.invoked)  &&
                             ((run.consumed_count == 1u) ==
                              d_dispatch_result_consumed(&fired))     &&
                             (d_tests_event_logged(fire_log)) );
    }

    // the payload is copied at enqueue: later writes to it are invisible
    (void)d_event_dispatcher_queue(&d, k, d_event_payload_make(&v, sizeof(v),
                                                               1u));
    v = 8;
    d_tests_event_log_reset();
    (void)d_event_dispatcher_process_all(&d, NULL);
    D_TESTS_EVENT_CHECK(d_tests_event_logged("p5q6"));

    // an empty payload reaches handlers as NULL
    {
        const struct d_event_payload empty = D_EVENT_PAYLOAD_EMPTY;

        (void)d_event_dispatcher_queue(&d, k, empty);
        d_tests_event_log_reset();
        (void)d_event_dispatcher_process_all(&d, NULL);
        D_TESTS_EVENT_CHECK(d_tests_event_logged("p-q-"));

        // enqueue refuses when the slots are full, and enqueues nothing
        for (int i = 0; i < 4; ++i)
        {
            D_TESTS_EVENT_CHECK(d_event_dispatcher_queue(&d, k, empty) ==
                                D_EVENT_OK);
        }

#if (D_INTERNAL_EVENT_QUEUE_OVERFLOW == D_EVENT_QUEUE_OVERFLOW_REJECT)
        D_TESTS_EVENT_CHECK( (d_event_dispatcher_queue(&d, k, empty) ==
                              D_EVENT_ERR_CAPACITY) &&
                             (d_event_dispatcher_pending(&d) == 4u) );
#endif  // D_INTERNAL_EVENT_QUEUE_OVERFLOW
        d_event_queue_clear(&d.queue);
    }

    // ... or when the bytes are, and a payload larger than the arena
    memset(big, 0, sizeof(big));
    D_TESTS_EVENT_CHECK(d_event_dispatcher_queue(
                            &d, k, d_event_payload_make(big, 48u, 1u)) ==
                        D_EVENT_OK);
#if (D_INTERNAL_EVENT_QUEUE_OVERFLOW == D_EVENT_QUEUE_OVERFLOW_REJECT)
    D_TESTS_EVENT_CHECK( (d_event_dispatcher_queue(
                              &d, k, d_event_payload_make(big, 32u, 1u)) ==
                          D_EVENT_ERR_CAPACITY) &&
                         (d_event_dispatcher_pending(&d) == 1u) );
#endif  // D_INTERNAL_EVENT_QUEUE_OVERFLOW
    d_event_queue_clear(&d.queue);
    D_TESTS_EVENT_CHECK(d_event_dispatcher_queue(
                            &d, k, d_event_payload_make(big, 100u, 1u)) ==
                        D_EVENT_ERR_CAPACITY);
    D_TESTS_EVENT_CHECK(d_event_dispatcher_queue(
                            &d, k, d_event_payload_make(NULL, 4u, 1u)) ==
                        D_EVENT_ERR_NULL);

    // peek and the payload view
    D_TESTS_EVENT_CHECK(d_event_queue_peek(&d.queue) == NULL);
    (void)d_event_dispatcher_queue(&d, k, d_event_payload_make(&w, sizeof(w),
                                                               1u));
    slot = d_event_queue_peek(&d.queue);
    D_TESTS_EVENT_CHECK( (slot != NULL)              &&
                         (slot->key == k)            &&
                         (slot->size == sizeof(int)) &&
                         (slot->arity == 1u) );
    view = d_event_queue_payload(&d.queue, slot);
    D_TESTS_EVENT_CHECK( (view.size == sizeof(int)) &&
                         (view.data != NULL)        &&
                         (*(const int*)view.data == 42) );
    d_event_queue_clear(&d.queue);

    // the ring wraps: varied sizes, each checked when it is dispatched
    (void)d_event_dispatcher_bind(&d, kw,
                                  d_event_step_make(&d_tests_event_step_wrap,
                                                    NULL),
                                  NULL, NULL);
    g_wrap_seen = 0;
    g_wrap_bad  = 0;
    g_wrap_next = 0u;

    for (uint32_t n = 0u; n < 200u; ++n)
    {
        unsigned char item[44];
        const size_t  size = 4u + (n % 37u);

        memcpy(item, &n, sizeof(n));

        for (uint32_t j = 0u; j < (n % 37u); ++j)
        {
            item[4u + j] = (unsigned char)((n * 7u + j) & 0xFFu);
        }

#if (D_INTERNAL_EVENT_QUEUE_OVERFLOW == D_EVENT_QUEUE_OVERFLOW_DROP_OLDEST)
        // drop-oldest never refuses: drain first so that nothing is dropped
        (void)d_event_dispatcher_process_all(&d, NULL);
#endif  // D_INTERNAL_EVENT_QUEUE_OVERFLOW

        // make room by processing the oldest until the item fits
        while (d_event_dispatcher_queue(&d, kw,
                                        d_event_payload_make(item, size,
                                                             1u)) !=
               D_EVENT_OK)
        {
            if (d_event_dispatcher_process(&d, 1u, NULL) != 1u)
            {
                ++g_wrap_bad;

                break;
            }
        }
    }

    (void)d_event_dispatcher_process_all(&d, NULL);
    D_TESTS_EVENT_CHECK( (g_wrap_seen == 200) &&
                         (g_wrap_bad == 0)    &&
                         (d.queue.count == 0u) &&
                         (d.queue.bytes_used == 0u) );

    // re-entrancy: a handler's enqueue waits for the next call, and a
    // nested process is refused
    d_tests_event_tag_init(&then, 't', (int32_t)D_VERDICT_PASS);
    reenter.dispatcher = &d;
    reenter.then       = kt;
    reenter.nested     = 99u;
    (void)d_event_dispatcher_bind(&d, kr,
                                  d_event_step_make(&d_tests_event_step_reenter,
                                                    &reenter),
                                  NULL, NULL);
    (void)d_event_dispatcher_bind(&d, kt, d_tests_event_tagged(&then), NULL,
                                  NULL);
    (void)d_event_dispatcher_queue(&d, kr, d_event_payload_make(&w, sizeof(w),
                                                                1u));
    d_tests_event_log_reset();
    D_TESTS_EVENT_CHECK(d_event_dispatcher_process_all(&d, NULL) == 1u);
    D_TESTS_EVENT_CHECK( (d_tests_event_logged("r"))       &&
                         (reenter.nested == 0u)            &&
                         (d_event_dispatcher_pending(&d) == 1u) );
    d_tests_event_log_reset();
    D_TESTS_EVENT_CHECK( (d_event_dispatcher_process_all(&d, NULL) == 1u) &&
                         (d_tests_event_logged("t")) );

    // a clear from a handler keeps the occurrence in flight, drops the rest
    (void)d_event_dispatcher_bind(&d, kc,
                                  d_event_step_make(
                                      &d_tests_event_step_clear_queue, &d),
                                  NULL, NULL);
    (void)d_event_dispatcher_queue(&d, kc, d_event_payload_make(&w, sizeof(w),
                                                                1u));
    (void)d_event_dispatcher_queue(&d, k, d_event_payload_make(&w, sizeof(w),
                                                               1u));
    (void)d_event_dispatcher_queue(&d, k, d_event_payload_make(&w, sizeof(w),
                                                               1u));
    d_tests_event_log_reset();
    D_TESTS_EVENT_CHECK( (d_event_dispatcher_process_all(&d, NULL) == 1u) &&
                         (d_tests_event_logged("c"))                      &&
                         (d_event_dispatcher_pending(&d) == 0u) );

    // ... and its dispose was refused: the queue still works
    D_TESTS_EVENT_CHECK( (d_event_dispatcher_queue(
                              &d, k, d_event_payload_make(&w, sizeof(w),
                                                          1u)) ==
                          D_EVENT_OK) &&
                         (d_event_dispatcher_process_all(&d, NULL) == 1u) );

    // counts through the facade
    D_TESTS_EVENT_CHECK( (d_event_dispatcher_handler_count(&d) == 6u)     &&
                         (d_event_dispatcher_enabled_count(&d) == 6u)     &&
                         (d_event_dispatcher_handler_count_for(&d, k) ==
                          2u)                                             &&
                         (d_event_dispatcher_footprint(&d) ==
                          sizeof(struct d_event_dispatcher)) );

#if (D_INTERNAL_EVENT_DISPATCHER_MERGE == 1)
    // merge takes the other's registry, never its queue
    {
        struct d_event_entry      oentries[4];
        uint32_t                  ohead[1];
        uint32_t                  otail[1];
        struct d_event_slot       oslots[2];
        union
        {
            unsigned char b[32];
            long double   ld;
        }                         oarena;
        struct d_event_dispatcher other;
        size_t                    merged = 0u;

        (void)d_event_dispatcher_init_fixed(&other, oentries, 4u, ohead,
                                            otail, 1u, oslots, 2u, oarena.b,
                                            32u);
        (void)d_event_dispatcher_bind(&other, k, d_tests_event_tagged(&then),
                                      NULL, NULL);
        (void)d_event_dispatcher_queue(&other, k,
                                       d_event_payload_make(&w, sizeof(w),
                                                            1u));
        D_TESTS_EVENT_CHECK( (d_event_dispatcher_merge(&d, &other,
                                                       &merged) ==
                              D_EVENT_OK) &&
                             (merged == 1u) );
        D_TESTS_EVENT_CHECK( (d_event_dispatcher_handler_count_for(&d, k) ==
                              3u) &&
                             (d_event_dispatcher_pending(&d) == 0u) &&
                             (d_event_dispatcher_pending(&other) == 1u) );
        d_event_dispatcher_dispose(&other);
    }
#endif  // D_INTERNAL_EVENT_DISPATCHER_MERGE

    d_event_dispatcher_dispose(&d);

#if (D_INTERNAL_EVENT_QUEUE_OVERFLOW == D_EVENT_QUEUE_OVERFLOW_DROP_OLDEST)
    // drop-oldest: a full queue releases its oldest occurrence to make room
    {
        (void)d_event_dispatcher_init_fixed(&d, entries, 8u, head, tail, 3u,
                                            slots, 4u, arena.b, 64u);
        (void)d_event_dispatcher_bind(&d, k, d_tests_event_numbered(&q),
                                      NULL, NULL);

        for (int i = 1; i <= 5; ++i)
        {
            D_TESTS_EVENT_CHECK(d_event_dispatcher_queue(
                                    &d, k, d_event_payload_make(&i,
                                                                sizeof(i),
                                                                1u)) ==
                                D_EVENT_OK);
        }

        d_tests_event_log_reset();
        (void)d_event_dispatcher_process_all(&d, NULL);
        D_TESTS_EVENT_CHECK(d_tests_event_logged("q2q3q4q5"));
        d_event_dispatcher_dispose(&d);
    }
#endif  // D_INTERNAL_EVENT_QUEUE_OVERFLOW

#if ( (D_INTERNAL_EVENT_TABLE_ALLOC == 1) &&                                  \
      (D_INTERNAL_EVENT_QUEUE_ALLOC == 1) )
    // the allocating dispatcher owns its storage end to end
    {
        struct d_event_dispatcher owning;

        D_TESTS_EVENT_CHECK(d_event_dispatcher_init(&owning, 0u, 0u, 0u,
                                                    0u) == D_EVENT_OK);
        D_TESTS_EVENT_CHECK(d_event_dispatcher_footprint(&owning) >
                            sizeof(struct d_event_dispatcher));
        (void)d_event_dispatcher_bind(&owning, k, d_tests_event_numbered(&q),
                                      NULL, NULL);
        (void)d_event_dispatcher_queue(&owning, k,
                                       d_event_payload_make(&w, sizeof(w),
                                                            1u));
        d_tests_event_log_reset();
        D_TESTS_EVENT_CHECK( (d_event_dispatcher_process_all(&owning,
                                                             NULL) == 1u) &&
                             (d_tests_event_logged("q2")) );

#if ( (D_INTERNAL_EVENT_REGISTRY_STAGING == 1) &&                             \
      (D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC == 1) )
        {
            struct d_event_word word;

            D_TESTS_EVENT_CHECK( (d_event_dispatcher_compile(&owning, k,
                                                             &word) ==
                                  D_EVENT_OK) &&
                                 (word.count == 1u) );
            d_event_word_dispose(&word);
        }
#endif  // STAGING && STAGING_ALLOC

        d_event_dispatcher_dispose(&owning);
        d_event_dispatcher_dispose(&owning);
    }
#endif  // TABLE_ALLOC && QUEUE_ALLOC

    return;
}

#endif  // D_INTERNAL_EVENT_QUEUE


int
main(void)
{
    d_tests_event_prime();
    d_tests_event_monoid();
    d_tests_event_table_fixed();
#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)
    d_tests_event_table_alloc();
#endif
#if (D_INTERNAL_EVENT_TABLE_MERGE == 1)
    d_tests_event_merge();
#endif
    d_tests_event_dispatch();
    d_tests_event_snapshot();
#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)
    d_tests_event_run();
#endif
#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)
    d_tests_event_staging();
#endif
#if (D_INTERNAL_EVENT_QUEUE == 1)
    d_tests_event_queue();
#endif

    printf("t_event_core: %d checks, %d failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
