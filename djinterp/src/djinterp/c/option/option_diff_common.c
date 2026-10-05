/*******************************************************************************
* djinterp [c]                                              option_diff_common.c
*
* The difference algebra over option sets: the key-set delta, changed and
* unchanged keys, the full difference with and without exemption, its
* count, and the per-key verdict record.
*   Values are compared by d_option_eq, the relation d_option_set_agrees
* uses, so `changed` is empty exactly when the sets agree.
*   THE TWO-CALL PROTOCOL, AS THE CARRIER ALLOWS IT. option_diff_common.h asks
* a short buffer to fail with D_OPTION_STATUS_BUFFER_TOO_SMALL AND the count,
* but `struct d_option_result` holds a status or a count, never both. So:
* `_out == NULL` returns the count; a short buffer returns BUFFER_TOO_SMALL
* alone, leaving the buffer's contents unspecified; and the count for a retry
* comes from a NULL-buffer call. This is option (a) of the question put to
* the owner on 2026.09.30; widening the carrier is option (b).
*
*
* path:      /src/djinterp/c/option/option_diff_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_diff_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint32_t,
                                                     // uint64_t, UINT32_MAX


/*
d_internal_option_diff_sink
  Where an enumeration writes: keys or verdict entries, the capacity, and
how many have been produced so far -- counted past the capacity, so one walk
both measures and produces.
*/
struct d_internal_option_diff_sink
{
    uint64_t*                   keys;
    struct d_option_diff_entry* entries;
    uint32_t                    capacity;
    uint64_t                    produced;
};

static void
d_internal_option_diff_emit(
    struct d_internal_option_diff_sink* _sink,
    uint64_t                            _key,
    int32_t                             _verdict
)
{
    // write while there is room; count regardless
    if (_sink->produced < _sink->capacity)
    {
        if (_sink->keys)
        {
            _sink->keys[_sink->produced] = _key;
        }

        if (_sink->entries)
        {
            _sink->entries[_sink->produced].key      = _key;
            _sink->entries[_sink->produced].verdict  = _verdict;
            _sink->entries[_sink->produced].reserved = 0u;
        }
    }

    ++_sink->produced;

    return;
}

/*
d_internal_option_diff_finish
  The protocol's answer for a finished walk: the count when measuring or when
everything fit, BUFFER_TOO_SMALL when it did not, and CAPACITY for a count a
uint32_t cannot carry.
*/
static struct d_option_result
d_internal_option_diff_finish(
    const struct d_internal_option_diff_sink* _sink,
    bool                                      _measuring
)
{
    if (_sink->produced > (uint64_t)UINT32_MAX)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    if ( (!_measuring) &&
         (_sink->produced > _sink->capacity) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);
    }

    return d_option_ok((uint32_t)_sink->produced);
}

static struct d_internal_option_diff_sink
d_internal_option_diff_keys_sink(
    uint64_t* _out,
    uint32_t  _capacity
)
{
    struct d_internal_option_diff_sink sink;

    sink.keys     = _out;
    sink.entries  = NULL;
    sink.capacity = (_out != NULL) ? _capacity : 0u;
    sink.produced = 0u;

    return sink;
}

/*
d_internal_option_diff_only_in
  Emits the keys of `_from` absent from `_other`, in `_from`'s order: added
when `_from` is the delta, removed when it is the base.
*/
static void
d_internal_option_diff_only_in(
    const struct d_option_set*          _from,
    const struct d_option_set*          _other,
    struct d_internal_option_diff_sink* _sink,
    int32_t                             _verdict
)
{
    uint32_t at = 0u;

    for (uint32_t i = 0u; i < _from->count; ++i)
    {
        if (!d_option_set_find(_other, _from->options[i].key, &at))
        {
            d_internal_option_diff_emit(_sink,
                                        _from->options[i].key,
                                        _verdict);
        }
    }

    return;
}

struct d_option_result
d_option_set_keys(
    const struct d_option_set* _set,
    uint64_t*                  _out,
    uint32_t                   _capacity
)
{
    // a set is required
    if (!_set)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink =
        d_internal_option_diff_keys_sink(_out, _capacity);

    for (uint32_t i = 0u; i < _set->count; ++i)
    {
        d_internal_option_diff_emit(&sink,
                                    _set->options[i].key,
                                    D_OPTION_DIFF_UNCHANGED);
    }

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

struct d_option_result
d_option_set_added_keys(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    uint64_t*                  _out,
    uint32_t                   _capacity
)
{
    // both sets are required
    if ( (!_base) ||
         (!_delta) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink =
        d_internal_option_diff_keys_sink(_out, _capacity);

    d_internal_option_diff_only_in(_delta, _base, &sink, D_OPTION_DIFF_ADDED);

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

struct d_option_result
d_option_set_removed_keys(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    uint64_t*                  _out,
    uint32_t                   _capacity
)
{
    // both sets are required
    if ( (!_base) ||
         (!_delta) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink =
        d_internal_option_diff_keys_sink(_out, _capacity);

    d_internal_option_diff_only_in(_base,
                                   _delta,
                                   &sink,
                                   D_OPTION_DIFF_REMOVED);

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

/*
d_internal_option_diff_walk
  The one walk every value-reading function is a view of. Over the base, in
its order: a key the delta lacks is REMOVED, a shared key is CHANGED or
UNCHANGED by d_option_eq. Then over the delta, in its order: a key the base
lacks is ADDED. A verdict reaches the sink only if `_report` holds it and
`_exempt` (when given) does not omit the key -- so exemption filters the
result and never touches the comparison.
*/
static void
d_internal_option_diff_walk(
    const struct d_option_set*          _base,
    const struct d_option_set*          _delta,
    fn_option_project                   _project,
    fn_binary_predicate                 _compare,
    fn_option_exempt                    _exempt,
    void*                               _context,
    const bool                          _report[4],
    struct d_internal_option_diff_sink* _sink
)
{
    uint32_t at = 0u;

    // removed, changed and unchanged, in the base's order
    for (uint32_t i = 0u; i < _base->count; ++i)
    {
        const struct d_option* const base_option = &_base->options[i];
        const struct d_option*       delta_option = NULL;
        int32_t                      verdict      = D_OPTION_DIFF_REMOVED;

        if (d_option_set_find(_delta, base_option->key, &at))
        {
            delta_option = &_delta->options[at];
            verdict      = d_option_eq(base_option, _base->values,
                                       delta_option, _delta->values,
                                       _project, _compare, _context)
                               ? D_OPTION_DIFF_UNCHANGED
                               : D_OPTION_DIFF_CHANGED;
        }

        if ( (_report[verdict]) &&
             ( (!_exempt) ||
               (!_exempt(base_option->key, base_option, delta_option,
                         _context)) ) )
        {
            d_internal_option_diff_emit(_sink, base_option->key, verdict);
        }
    }

    // added, in the delta's order
    for (uint32_t i = 0u; (_report[D_OPTION_DIFF_ADDED]) &&
                          (i < _delta->count); ++i)
    {
        const struct d_option* const delta_option = &_delta->options[i];

        if ( (!d_option_set_find(_base, delta_option->key, &at)) &&
             ( (!_exempt) ||
               (!_exempt(delta_option->key, NULL, delta_option,
                         _context)) ) )
        {
            d_internal_option_diff_emit(_sink,
                                        delta_option->key,
                                        D_OPTION_DIFF_ADDED);
        }
    }

    return;
}

struct d_option_result
d_option_set_changed_keys(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    uint64_t*                  _out,
    uint32_t                   _capacity,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    static const bool report[4] = { false, true, false, false };

    // both sets are required
    if ( (!_base) ||
         (!_delta) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink =
        d_internal_option_diff_keys_sink(_out, _capacity);

    d_internal_option_diff_walk(_base, _delta, _project, _compare, NULL,
                                _context, report, &sink);

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

struct d_option_result
d_option_set_unchanged_keys(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    uint64_t*                  _out,
    uint32_t                   _capacity,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    static const bool report[4] = { true, false, false, false };

    // both sets are required
    if ( (!_base) ||
         (!_delta) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink =
        d_internal_option_diff_keys_sink(_out, _capacity);

    d_internal_option_diff_walk(_base, _delta, _project, _compare, NULL,
                                _context, report, &sink);

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

/*
d_option_set_diff_keys_exempt
  added, changed and removed, filtered by `_exempt`; the order the header
states falls out of the walk.
*/
struct d_option_result
d_option_set_diff_keys_exempt(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    uint64_t*                  _out,
    uint32_t                   _capacity,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    fn_option_exempt           _exempt,
    void*                      _context
)
{
    static const bool report[4] = { false, true, true, true };

    // both sets are required
    if ( (!_base) ||
         (!_delta) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink =
        d_internal_option_diff_keys_sink(_out, _capacity);

    d_internal_option_diff_walk(_base, _delta, _project, _compare, _exempt,
                                _context, report, &sink);

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

struct d_option_result
d_option_set_diff_keys(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    uint64_t*                  _out,
    uint32_t                   _capacity,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    return d_option_set_diff_keys_exempt(_base, _delta, _out, _capacity,
                                         _project, _compare, NULL, _context);
}

struct d_option_result
d_option_set_diff_count(
    const struct d_option_set* _base,
    const struct d_option_set* _delta,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    return d_option_set_diff_keys(_base, _delta, NULL, 0u, _project,
                                  _compare, _context);
}

struct d_option_result
d_option_set_diff(
    const struct d_option_set*  _base,
    const struct d_option_set*  _delta,
    struct d_option_diff_entry* _out,
    uint32_t                    _capacity,
    fn_option_project           _project,
    fn_binary_predicate         _compare,
    void*                       _context
)
{
    static const bool report[4] = { true, true, true, true };

    // both sets are required
    if ( (!_base) ||
         (!_delta) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct d_internal_option_diff_sink sink;

    sink.keys     = NULL;
    sink.entries  = _out;
    sink.capacity = (_out != NULL) ? _capacity : 0u;
    sink.produced = 0u;

    d_internal_option_diff_walk(_base, _delta, _project, _compare, NULL,
                                _context, report, &sink);

    return d_internal_option_diff_finish(&sink, (_out == NULL));
}

const char*
d_option_diff_verdict_name(
    int32_t _verdict
)
{
    switch (_verdict)
    {
        case D_OPTION_DIFF_UNCHANGED: return "unchanged";
        case D_OPTION_DIFF_CHANGED:   return "changed";
        case D_OPTION_DIFF_ADDED:     return "added";
        case D_OPTION_DIFF_REMOVED:   return "removed";
        default:                      return "unknown";
    }
}
