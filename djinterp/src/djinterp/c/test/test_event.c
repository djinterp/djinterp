/******************************************************************************
* djinterp [test]                                                test_event.c
*
*   Implementation of the DTest event module -- the observation record and
* the dispatch kernel.  Compiled by both languages from this one source.
*
* path:      /src/djinterp/c/test/test_event.c
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.31
******************************************************************************/

#include "djinterp/c/test/test_event.h"

D_EXTERN_C_BEGIN


/* =============================================================================
   I.   THE OBSERVATION RECORD
   =============================================================================
     One constructor, and it is the whole of the record's arithmetic.  It was
   D_STATIC_INLINE in the header while this module did not exist; the header
   said so and said it would move here when the module was done.  It is done.

     The coercion rule -- an out-of-range status is stored as `error` -- is
   stated in the header at VII.a and applied in exactly two places: here, and
   in d_test_event_status_change's two status slots below. */

struct d_test_event
d_test_event_make(
    d_test_event_id _event,
    int32_t         _status,
    const char*     _message,
    const void*     _context
)
{
    struct d_test_event e;

    e.event   = _event;
    e.status  = d_test_status_or_error(_status);
    e.message = _message;
    e.context = _context;

    return e;
}


/* =============================================================================
   II.  THE SIGNATURE TABLE
   =============================================================================
     One row per built-in summand.  This is the single place the alphabet's
   shape is written down; d_test_event_arity and d_test_event_domain are both
   reads of it, and the static assertions below tie it to the macro half.

     The table is indexed by kind, so the row order IS the enumerator order and
   a renumbering that forgot the table would shift every signature.  The
   assertions at the bottom of this section catch that. */

struct d_test_event_signature_
{
    const char* name;
    int         arity;
};

static const struct d_test_event_signature_
d_test_event_table_[D_TEST_EVENT_KIND_COUNT] =
{
    { "on_session_start",  D_TEST_EVENT_ARITY_SESSION_START  },
    { "on_session_end",    D_TEST_EVENT_ARITY_SESSION_END    },
    { "on_module_start",   D_TEST_EVENT_ARITY_MODULE_START   },
    { "on_module_end",     D_TEST_EVENT_ARITY_MODULE_END     },
    { "on_test_start",     D_TEST_EVENT_ARITY_TEST_START     },
    { "on_test_end",       D_TEST_EVENT_ARITY_TEST_END       },
    { "on_test_passed",    D_TEST_EVENT_ARITY_TEST_PASSED    },
    { "on_test_failed",    D_TEST_EVENT_ARITY_TEST_FAILED    },
    { "on_test_skipped",   D_TEST_EVENT_ARITY_TEST_SKIPPED   },
    { "on_test_error",     D_TEST_EVENT_ARITY_TEST_ERROR     },
    { "on_status_change",  D_TEST_EVENT_ARITY_STATUS_CHANGE  },
    { "on_listener_threw", D_TEST_EVENT_ARITY_LISTENER_THREW }
};

/*   THE DOMAIN COLUMN IS GONE with the slots it described.  `arity` stays
   because the C++ face asserts each tag's std::tuple_size against the matching
   D_TEST_EVENT_ARITY_ macro, and this is the runtime half of that same claim --
   the one pair of mechanisms the payload change did not cost.

     The table is still indexed by kind, so the row order IS the enumerator
   order and a renumbering that forgot the table would shift every name.  The
   assertions below catch that. */

D_STATIC_ASSERT(D_TEST_EVENT_LISTENER_THREW + 1 == D_TEST_EVENT_KIND_COUNT,
                "test_event: the alphabet is not dense");
D_STATIC_ASSERT(D_TEST_EVENT_ARITY_STATUS_CHANGE == 3,
                "test_event: on_status_change's arity has drifted");


/* =============================================================================
   III. THE ALPHABET
   ============================================================================= */

const char*
d_test_event_kind_name(
    enum d_test_event_kind _kind
)
{
    if (!d_test_event_kind_is_builtin((int32_t)_kind))
    {
        /*   A user tag's printable name lives with the user tag.  Returning a
           constant rather than null keeps every caller's format string total;
           a caller that needs the real name asks the face that declared it. */
        return "custom";
    }

    return d_test_event_table_[(int)_kind].name;
}


int
d_test_event_arity(
    enum d_test_event_kind _kind
)
{
    if (!d_test_event_kind_is_builtin((int32_t)_kind))
    {
        return -1;
    }

    return d_test_event_table_[(int)_kind].arity;
}


int
d_test_event_kind_is_builtin(
    int32_t _kind
)
{
    return ((_kind >= 0) && (_kind < D_TEST_EVENT_KIND_COUNT)) ? 1 : 0;
}


/* =============================================================================
   IV.  CONSTRUCTION
   =============================================================================
     ONE function where there were six plus a validator.  A payload is a kind
   and a borrowed pointer, so there is nothing to stamp and nothing to check;
   the receiver knows the shape from the kind.  See test_event.h §III for what
   that traded away, and §II for where the shapes are written down.

     d_test_event_status_change is not here either.  It returned one of the
   four argument structs and went with them; the coercion it applied is
   d_test_status_or_error in test_common.h, which d_test_event_make above now
   calls rather than open-coding. */

struct d_test_event_payload
d_test_event_payload_make(
    enum d_test_event_kind _kind,
    const void*            _context
)
{
    struct d_test_event_payload p;

    p.kind    = (int32_t)_kind;
    p.context = _context;

    return p;
}


/* =============================================================================
   V.   DISPATCH
   ============================================================================= */

enum d_test_dispatch_result
d_test_dispatch_init(
    struct d_test_dispatch* _dispatch
)
{
    if (_dispatch == (struct d_test_dispatch*)0)
    {
        return D_TEST_DISPATCH_INVALID;
    }

    _dispatch->listeners  = (struct d_test_listener*)0;
    _dispatch->count      = (size_t)0;
    _dispatch->capacity   = (size_t)0;
    _dispatch->fired      = (size_t)0;
    _dispatch->delivered  = (size_t)0;
    _dispatch->rejected   = (size_t)0;
    _dispatch->consumed   = (size_t)0;
    _dispatch->last_kind  = -1;
    _dispatch->next_order = 0;

    return D_TEST_DISPATCH_OK;
}


enum d_test_dispatch_result
d_test_dispatch_bind_listeners(
    struct d_test_dispatch* _dispatch,
    struct d_test_listener* _storage,
    size_t                  _capacity
)
{
    if ((_dispatch == (struct d_test_dispatch*)0) ||
        ((_storage == (struct d_test_listener*)0) && (_capacity != (size_t)0)))
    {
        return D_TEST_DISPATCH_INVALID;
    }

    _dispatch->listeners = _storage;
    _dispatch->capacity  = _capacity;
    _dispatch->count     = (size_t)0;

    return D_TEST_DISPATCH_OK;
}


enum d_test_dispatch_result
d_test_dispatch_on(
    struct d_test_dispatch* _dispatch,
    enum d_test_event_kind  _kind,
    d_test_listener_fn      _notify,
    void*                   _context
)
{
    struct d_test_listener* slot;

    if ((_dispatch == (struct d_test_dispatch*)0) ||
        (_notify == (d_test_listener_fn)0) ||
        ((int32_t)_kind < 0))
    {
        return D_TEST_DISPATCH_INVALID;
    }

    if (_dispatch->count >= _dispatch->capacity)
    {
        return D_TEST_DISPATCH_FULL;
    }

    slot = &_dispatch->listeners[_dispatch->count];

    slot->notify  = _notify;
    slot->context = _context;
    slot->kind    = (int32_t)_kind;
    slot->order   = _dispatch->next_order;

    _dispatch->count      += (size_t)1;
    _dispatch->next_order += 1;

    return D_TEST_DISPATCH_OK;
}


enum d_test_dispatch_result
d_test_dispatch_fire(
    struct d_test_dispatch*            _dispatch,
    const struct d_test_event_payload* _payload
)
{
    size_t i;

    if ((_dispatch == (struct d_test_dispatch*)0) ||
        (_payload == (const struct d_test_event_payload*)0))
    {
        return D_TEST_DISPATCH_INVALID;
    }

    if (_payload->kind < 0)
    {
        return D_TEST_DISPATCH_INVALID;
    }

    /*   NOTHING VALIDATES THE CONTEXT, and there is no knob that pretends to.
       An opaque pointer has no shape to check against the alphabet, so the
       rejected counter below only ever moves on a bad KIND.  The old branch
       here compared a payload's slots to a signature table under
       D_INTERNAL_TEST_EVENT_VALIDATE; both went with the slots. */

    /*   Ascending index IS bind order: d_test_dispatch_on only ever appends,
       and `order` is issued from the same counter, so the two agree by
       construction.  The field is carried anyway because a later table that
       compacts or reorders would break that agreement silently, and the
       parity body records both so the day it stops holding is a diff. */
    /*   THE FOLD.  Verdicts combine under D_TEST_VERDICT_COMBINE with `consume` as
       the left zero, so the first listener to consume halts the rest -- and
       the accumulator can never return to `pass` afterwards, which is what
       "left zero" means and what a plain boolean flag would have got wrong the
       first time someone reordered the loop. */
    {
        int verdict = (int)D_TEST_VERDICT_PASS;

        for (i = (size_t)0; i < _dispatch->count; ++i)
        {
            if (_dispatch->listeners[i].kind != _payload->kind)
            {
                continue;
            }

            if (D_TEST_VERDICT_CONSUMED(verdict))
            {
                break;
            }

            verdict = D_TEST_VERDICT_COMBINE(
                verdict,
                _dispatch->listeners[i].notify(
                    _dispatch->listeners[i].context, _payload));

            _dispatch->delivered += (size_t)1;
        }

        if (D_TEST_VERDICT_CONSUMED(verdict))
        {
            _dispatch->consumed += (size_t)1;
        }
    }

    _dispatch->fired    += (size_t)1;
    _dispatch->last_kind = _payload->kind;

    return D_TEST_DISPATCH_OK;
}


size_t
d_test_dispatch_count_for(
    const struct d_test_dispatch* _dispatch,
    enum d_test_event_kind        _kind
)
{
    size_t i;
    size_t n = (size_t)0;

    if (_dispatch == (const struct d_test_dispatch*)0)
    {
        return (size_t)0;
    }

    for (i = (size_t)0; i < _dispatch->count; ++i)
    {
        if (_dispatch->listeners[i].kind == (int32_t)_kind)
        {
            n += (size_t)1;
        }
    }

    return n;
}


const char*
d_test_dispatch_result_name(
    enum d_test_dispatch_result _r
)
{
    switch (_r)
    {
        case D_TEST_DISPATCH_OK:        return "ok";
        case D_TEST_DISPATCH_FULL:      return "full";
        case D_TEST_DISPATCH_INVALID:   return "invalid";
        default:                        break;
    }

    return "unknown";
}


D_EXTERN_C_END
