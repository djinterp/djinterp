/*******************************************************************************
* djinterp [c]                                          option_override_common.c
*
*   Implementation of the merge engine and the named policies. See
* option_override_common.h for the rationale and for the algebraic laws.
*
*
* path:      /src/djinterp/c/option/option_override_common.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_override_common.h"


// std
#include <string.h>


// ===========================================================================
// I.   THE NAMED POLICY HOOKS
// ===========================================================================
//
//   Each hook copies a cell and, where the cell carries a value, its bytes.
// The engine has already sized `_out_value` to the cell's own width, so the
// hooks never check it.

static int32_t
elect(
    const struct d_option* _from,
    const unsigned char*   _from_values,
    struct d_option*       _out,
    const void**           _out_bytes
)
{
    *_out = *_from;

    //   the engine assigns the real offset when the cell joins the output set,
    // so a hook must not carry the source's.
    _out->value_offset = 0u;

    if (_out_bytes != NULL)
    {
        *_out_bytes = d_option_slot_const(_from, _from_values);
    }

    return D_OPTION_KEEP;
}

static int32_t
take_delta(
    const struct d_option* _base,
    const unsigned char*   _base_values,
    const struct d_option* _delta,
    const unsigned char*   _delta_values,
    struct d_option*       _out,
    const void**           _out_bytes,
    void*                  _context
)
{
    (void)_base; (void)_base_values; (void)_context;
    return elect(_delta, _delta_values, _out, _out_bytes);
}

static int32_t
take_base(
    const struct d_option* _base,
    const unsigned char*   _base_values,
    const struct d_option* _delta,
    const unsigned char*   _delta_values,
    struct d_option*       _out,
    const void**           _out_bytes,
    void*                  _context
)
{
    (void)_delta; (void)_delta_values; (void)_context;
    return elect(_base, _base_values, _out, _out_bytes);
}

static int32_t
keep_single(
    const struct d_option* _option,
    const unsigned char*   _values,
    struct d_option*       _out,
    const void**           _out_bytes,
    void*                  _context
)
{
    (void)_context;
    return elect(_option, _values, _out, _out_bytes);
}

static int32_t
drop_single(
    const struct d_option* _option,
    const unsigned char*   _values,
    struct d_option*       _out,
    const void**           _out_bytes,
    void*                  _context
)
{
    (void)_option; (void)_values; (void)_out; (void)_out_bytes; (void)_context;
    return D_OPTION_DROP;
}

static int32_t
reject_single(
    const struct d_option* _option,
    const unsigned char*   _values,
    struct d_option*       _out,
    const void**           _out_bytes,
    void*                  _context
)
{
    (void)_option; (void)_values; (void)_out; (void)_out_bytes; (void)_context;

    //   C++ `strict_subset` fires a static_assert here. C reports a FORMAL
    // status, so a caller inspecting the range learns the merge was ill-formed
    // rather than under-provisioned. Same invariant, different enforcement
    // tier -- goal 2 permits exactly this.
    return -((int32_t)D_OPTION_STATUS_POLICY_REJECTED);
}


// ===========================================================================
// II.  THE NAMED POLICIES
// ===========================================================================

bool
d_option_policy_is_valid(
    const struct d_option_policy* _policy
)
{
    if (_policy == NULL)
    {
        return false;
    }

    //   an all-NULL policy drops everything, which is almost certainly a
    // mistake rather than an intention, so it is rejected here rather than
    // silently returning the empty set.
    return ( (_policy->on_both       != NULL) ||
             (_policy->on_base_only  != NULL) ||
             (_policy->on_delta_only != NULL) );
}

struct d_option_policy
d_option_policy_replace(void)
{
    struct d_option_policy policy;

    policy.on_both       = &take_delta;
    policy.on_base_only  = &keep_single;
    policy.on_delta_only = &keep_single;
    policy.context       = NULL;

    return policy;
}

struct d_option_policy
d_option_policy_keep(void)
{
    struct d_option_policy policy;

    policy.on_both       = &take_base;
    policy.on_base_only  = &keep_single;
    policy.on_delta_only = &keep_single;
    policy.context       = NULL;

    return policy;
}

struct d_option_policy
d_option_policy_subset(void)
{
    struct d_option_policy policy;

    policy.on_both       = &take_delta;
    policy.on_base_only  = &keep_single;
    policy.on_delta_only = &drop_single;
    policy.context       = NULL;

    return policy;
}

struct d_option_policy
d_option_policy_strict(void)
{
    struct d_option_policy policy;

    policy.on_both       = &take_delta;
    policy.on_base_only  = &keep_single;
    policy.on_delta_only = &reject_single;
    policy.context       = NULL;

    return policy;
}

struct d_option_policy
d_option_policy_filter(void)
{
    struct d_option_policy policy;

    policy.on_both       = &take_delta;
    policy.on_base_only  = &drop_single;
    policy.on_delta_only = &keep_single;
    policy.context       = NULL;

    return policy;
}

struct d_option_policy
d_option_merge_mode_policy(
    int32_t _mode
)
{
    if (_mode == D_OPTION_MERGE_OVERWRITE)
    {
        return d_option_policy_replace();
    }

    //   ADD_NEW_ONLY and KEEP_EXISTING share a value, so they cannot disagree
    // about being the same mode.
    return d_option_policy_keep();
}


// ===========================================================================
// III. THE ALGEBRAIC LAWS
// ===========================================================================

bool
d_option_set_is_identity(
    const struct d_option_set* _set
)
{
    //   the monoid identity is the EMPTY SET. Named rather than open-coded as
    // `count == 0` so the identity law is asserted against the document's
    // notion and not against an implementation fact that happens to coincide.
    return d_option_set_empty(_set);
}

bool
d_option_set_commutes(
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    //   body-options.tex: A (+) B == B (+) A if and only if A ~ B. Implemented
    // AS the agreement test rather than by computing both folds and comparing
    // them: the proposition says the two questions are the same question, so
    // answering it twice would create the possibility of two answers.
    return d_option_set_agrees(_lhs, _rhs, _project, _compare, _context);
}


// ===========================================================================
// IV.  THE ENGINE
// ===========================================================================

//   emit: run one verdict into the output set. The cell says what to add; the
// borrowed pointer says where its bytes are.
static struct d_option_result
emit(
    struct d_option_set*   _out,
    int32_t                _verdict,
    const struct d_option* _cell,
    const void*            _bytes
)
{
    if (D_OPTION_VERDICT_IS_ERROR(_verdict))
    {
        return d_option_fail(D_OPTION_VERDICT_STATUS(_verdict));
    }

    if (_verdict == D_OPTION_DROP)
    {
        return d_option_ok(0u);
    }

    return d_option_set_add_cell(_out, _cell, _bytes);
}

struct d_option_result
d_option_set_override(
    struct d_option_set*          _out,
    const struct d_option_set*    _base,
    const struct d_option_set*    _delta,
    const struct d_option_policy* _policy
)
{
    uint32_t        i;
    uint32_t        j;
    struct d_option scratch;

    if ((_out == NULL) || (_base == NULL) || (_delta == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    //   aliasing is rejected rather than diagnosed later: an in-place merge
    // would have the walk reading cells it has already rewritten.
    if ((_out == _base) || (_out == _delta))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (!d_option_policy_is_valid(_policy))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    d_option_set_clear(_out);

    //   pass one: every key the base has.
    for (i = 0u; i < _base->count; ++i)
    {
        struct d_option_result written;
        int32_t                verdict;
        const void*            bytes;

        bytes = NULL;

        if (d_option_set_find(_delta, _base->options[i].key, &j))
        {
            if (_policy->on_both == NULL)
            {
                continue;
            }

            verdict = _policy->on_both(&_base->options[i], _base->values,
                                       &_delta->options[j], _delta->values,
                                       &scratch, &bytes, _policy->context);
        }
        else
        {
            if (_policy->on_base_only == NULL)
            {
                continue;
            }

            verdict = _policy->on_base_only(&_base->options[i], _base->values,
                                            &scratch, &bytes,
                                            _policy->context);
        }

        written = emit(_out, verdict, &scratch, bytes);

        if (!written.is_ok)
        {
            return written;
        }
    }

    //   pass two: extensions -- keys the delta has and the base does not.
    // Reached only for keys that genuinely are extensions, which is where the
    // C++ engine needs its SFINAE laziness wrapper and C needs nothing.
    for (i = 0u; i < _delta->count; ++i)
    {
        struct d_option_result written;
        int32_t                verdict;
        const void*            bytes;

        if (d_option_set_contains(_base, _delta->options[i].key))
        {
            continue;
        }

        if (_policy->on_delta_only == NULL)
        {
            continue;
        }

        bytes   = NULL;
        verdict = _policy->on_delta_only(&_delta->options[i], _delta->values,
                                         &scratch, &bytes, _policy->context);

        written = emit(_out, verdict, &scratch, bytes);

        if (!written.is_ok)
        {
            return written;
        }
    }

    return d_option_ok(_out->count);
}

struct d_option_result
d_option_set_merge(
    struct d_option_set*       _target,
    const struct d_option_set* _delta,
    int32_t                    _mode
)
{
    uint32_t i;
    uint32_t j;
    uint32_t touched;

    if ((_target == NULL) || (_delta == NULL) || (_target == _delta))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    touched = 0u;

    for (i = 0u; i < _delta->count; ++i)
    {
        const struct d_option* incoming = &_delta->options[i];

        if (d_option_set_find(_target, incoming->key, &j))
        {
            const void* slot;

            //   KEEP_EXISTING leaves a present key alone; OVERWRITE assigns.
            if (_mode != D_OPTION_MERGE_OVERWRITE)
            {
                continue;
            }

            slot = d_option_slot_const(incoming, _delta->values);

            if (slot != NULL)
            {
                struct d_option_result written =
                    d_option_write(&_target->options[j], _target->values,
                                   slot, (size_t)incoming->value_size);

                if (!written.is_ok)
                {
                    return written;
                }
            }

            ++touched;
        }
        else
        {
            //   in EVERY mode a new key is inserted, which is what makes this
            // the in-place shape of the same engine rather than a variant of
            // it.
            struct d_option_result added =
                d_option_set_add_cell(_target, incoming,
                                      d_option_slot_const(incoming,
                                                          _delta->values));

            if (!added.is_ok)
            {
                return added;
            }

            ++touched;
        }
    }

    return d_option_ok(touched);
}

struct d_option_result
d_option_set_compose(
    struct d_option_set*          _out,
    struct d_option_set*          _scratch,
    const struct d_option_set*    _base,
    const struct d_option_set**   _deltas,
    uint32_t                      _count,
    const struct d_option_policy* _policy
)
{
    uint32_t             i;
    struct d_option_set* target;
    struct d_option_set* spare;

    if ((_out == NULL) || (_scratch == NULL) || (_base == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ((_count > 0u) && (_deltas == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    //   COMPOSITION IS REPEATED OVERRIDE -- a LEFT FOLD, which is well defined
    // precisely because (+) is associative (see the header, section V). This
    // is a loop and not a second engine.
    target = _out;
    spare  = _scratch;

    {
        struct d_option_result seeded =
            d_option_set_override(target, _base, _base,
                                  _policy);

        if (!seeded.is_ok)
        {
            return seeded;
        }
    }

    for (i = 0u; i < _count; ++i)
    {
        struct d_option_result stepped;
        struct d_option_set*   swap;

        if (_deltas[i] == NULL)
        {
            continue;
        }

        stepped = d_option_set_override(spare, target, _deltas[i], _policy);

        if (!stepped.is_ok)
        {
            return stepped;
        }

        //   each step's output is the next step's base, so the two sets swap
        // rather than one being copied into the other.
        swap   = target;
        target = spare;
        spare  = swap;
    }

    //   the fold may have finished in the scratch set; the caller asked for
    // the answer in _out, so copy it back only when it is not already there.
    if (target != _out)
    {
        struct d_option_result copied =
            d_option_set_override(_out, target, target, _policy);

        if (!copied.is_ok)
        {
            return copied;
        }
    }

    return d_option_ok(_out->count);
}
