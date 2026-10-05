/*******************************************************************************
* djinterp [c]                                               option_set_common.c
*
*   Implementation of the flat option set and the relation algebra of
* body-options.tex. See option_set_common.h for the rationale.
*
*
* path:      /src/djinterp/c/option/option_set_common.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_set_common.h"


// std
#include <string.h>


// ===========================================================================
// I.   CONSTRUCTION
// ===========================================================================

struct d_option_set
d_option_set_view(
    struct d_option* _options,
    unsigned char*   _values,
    uint32_t         _capacity,
    uint32_t         _value_capacity
)
{
    struct d_option_set set;

    set.options        = _options;
    set.values         = _values;
    set.count          = 0u;
    set.capacity       = _capacity;
    set.value_used     = 0u;
    set.value_capacity = _value_capacity;

    return set;
}

void
d_option_set_clear(
    struct d_option_set* _set
)
{
    if (_set == NULL)
    {
        return;
    }

    _set->count      = 0u;
    _set->value_used = 0u;

    return;
}

bool
d_option_set_is_valid(
    const struct d_option_set* _set
)
{
    uint32_t i;
    uint32_t j;

    if (_set == NULL)
    {
        return false;
    }

    if ((_set->count > _set->capacity) ||
        (_set->value_used > _set->value_capacity))
    {
        return false;
    }

    if ((_set->count > 0u) && (_set->options == NULL))
    {
        return false;
    }

    for (i = 0u; i < _set->count; ++i)
    {
        const struct d_option* option = &_set->options[i];

        if (!d_option_is_valid(option))
        {
            return false;
        }

        //   the slot must lie inside the block it claims to address.
        if (option->value_size > 0u)
        {
            if (((uint64_t)option->value_offset +
                 (uint64_t)option->value_size) > (uint64_t)_set->value_used)
            {
                return false;
            }
        }

        //   MULTIPLICITY 1, checked rather than assumed. This is the set's
        // central invariant and the run-time counterpart of the C++ face's
        // value_pack_unique static_assert.
        for (j = (i + 1u); j < _set->count; ++j)
        {
            if (_set->options[j].key == option->key)
            {
                return false;
            }
        }

        //   one key type across the set -- the counterpart of all_same_type.
        if (_set->options[0].key_type != option->key_type)
        {
            return false;
        }
    }

    return true;
}


// ===========================================================================
// II.  POPULATION
// ===========================================================================

struct d_option_result
d_option_set_add(
    struct d_option_set* _set,
    uint64_t             _key,
    d_type_info16        _key_type,
    d_type_info16        _value_type,
    uint32_t             _value_size,
    uint32_t             _value_align
)
{
    uint32_t align;
    uint32_t offset;
    uint32_t index;

    if (_set == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (d_option_set_contains(_set, _key))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_DUPLICATE);
    }

    if ((_set->count > 0u) && (_set->options[0].key_type != _key_type))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_TYPE_MISMATCH);
    }

    if (_set->count >= _set->capacity)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    //   the offset is assigned HERE, from the running high-water mark, under
    // the slot's own alignment. A caller never computes one, which is the only
    // way the offsets-not-pointers decision survives editing.
    align  = ((_value_align == 0u) ? 1u : _value_align);
    offset = (((_set->value_used + align) - 1u) / align) * align;

    if (((uint64_t)offset + (uint64_t)_value_size) >
        (uint64_t)_set->value_capacity)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_VALUE_CAPACITY);
    }

    index = _set->count;

    _set->options[index] = d_option_make(_key, _key_type, _value_type,
                                         offset, _value_size,
                                         D_OPTION_FLAG_NONE);

    if (_value_size > 0u)
    {
        memset(_set->values + offset, 0, (size_t)_value_size);
    }

    _set->value_used = (offset + _value_size);
    _set->count      = (index + 1u);

    return d_option_ok(index);
}

struct d_option_result
d_option_set_add_unary(
    struct d_option_set* _set,
    uint64_t             _key,
    d_type_info16        _key_type
)
{
    uint32_t index;

    if (_set == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (d_option_set_contains(_set, _key))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_DUPLICATE);
    }

    if ((_set->count > 0u) && (_set->options[0].key_type != _key_type))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_TYPE_MISMATCH);
    }

    if (_set->count >= _set->capacity)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    index                = _set->count;
    _set->options[index] = d_option_make_unary(_key, _key_type);
    _set->count          = (index + 1u);

    return d_option_ok(index);
}

struct d_option_result
d_option_set_add_cell(
    struct d_option_set*   _set,
    const struct d_option* _option,
    const void*            _value
)
{
    struct d_option_result result;
    uint32_t               align;

    if ((_set == NULL) || (_option == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ((_option->flags & D_OPTION_FLAG_UNARY) != 0u)
    {
        return d_option_set_add_unary(_set, _option->key, _option->key_type);
    }

    //   the lowered cell carries no alignment, so the slot's own width is used
    // as its alignment requirement, capped at eight. That is conservative and
    // exact for every scalar; an over-aligned type would need the alignment
    // passed, which is what d_option_set_add is for.
    align = _option->value_size;

    if (align > 8u)
    {
        align = 8u;
    }

    result = d_option_set_add(_set, _option->key, _option->key_type,
                              _option->value_type, _option->value_size,
                              align);

    if (!result.is_ok)
    {
        return result;
    }

    if (_value != NULL)
    {
        struct d_option* placed = &_set->options[result.payload.value];

        return d_option_write(placed, _set->values, _value,
                              (size_t)_option->value_size);
    }

    return result;
}


// ===========================================================================
// III. QUERIES
// ===========================================================================

uint32_t
d_option_set_size(
    const struct d_option_set* _set
)
{
    return ((_set != NULL) ? _set->count : 0u);
}

bool
d_option_set_empty(
    const struct d_option_set* _set
)
{
    return (d_option_set_size(_set) == 0u);
}

bool
d_option_set_find(
    const struct d_option_set* _set,
    uint64_t                   _key,
    uint32_t*                  _out_index
)
{
    uint32_t i;

    if ((_set == NULL) || (_set->options == NULL))
    {
        return false;
    }

    for (i = 0u; i < _set->count; ++i)
    {
        if (_set->options[i].key == _key)
        {
            //   _out_index is left untouched on a miss, so a miss cannot be
            // mistaken for index 0.
            if (_out_index != NULL)
            {
                *_out_index = i;
            }

            return true;
        }
    }

    return false;
}

bool
d_option_set_contains(
    const struct d_option_set* _set,
    uint64_t                   _key
)
{
    return d_option_set_find(_set, _key, NULL);
}

struct d_option*
d_option_set_at(
    struct d_option_set* _set,
    uint32_t             _index
)
{
    if ((_set == NULL) || (_index >= _set->count))
    {
        return NULL;
    }

    return &_set->options[_index];
}

const struct d_option*
d_option_set_at_const(
    const struct d_option_set* _set,
    uint32_t                   _index
)
{
    if ((_set == NULL) || (_index >= _set->count))
    {
        return NULL;
    }

    return &_set->options[_index];
}

bool
d_option_set_key_at(
    const struct d_option_set* _set,
    uint32_t                   _index,
    uint64_t*                  _out_key
)
{
    const struct d_option* option = d_option_set_at_const(_set, _index);

    if ((option == NULL) || (_out_key == NULL))
    {
        return false;
    }

    *_out_key = option->key;

    return true;
}

d_type_info16
d_option_set_key_type(
    const struct d_option_set* _set
)
{
    if ((_set == NULL) || (_set->count == 0u))
    {
        return (d_type_info16)0;
    }

    return _set->options[0].key_type;
}


// ===========================================================================
// IV.  ACCESS
// ===========================================================================

struct d_option_result
d_option_set_get(
    const struct d_option_set* _set,
    uint64_t                   _key,
    void*                      _out,
    size_t                     _out_size
)
{
    uint32_t index;

    if (_set == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (!d_option_set_find(_set, _key, &index))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);
    }

    return d_option_read(&_set->options[index], _set->values, _out, _out_size);
}

struct d_option_result
d_option_set_set(
    struct d_option_set* _set,
    uint64_t             _key,
    const void*          _value,
    size_t               _value_size
)
{
    uint32_t index;

    if (_set == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (!d_option_set_find(_set, _key, &index))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);
    }

    return d_option_write(&_set->options[index], _set->values,
                          _value, _value_size);
}

bool
d_option_set_is_assigned(
    const struct d_option_set* _set,
    uint64_t                   _key
)
{
    uint32_t index;

    if (!d_option_set_find(_set, _key, &index))
    {
        return false;
    }

    return d_option_is_assigned(&_set->options[index]);
}


// ===========================================================================
// V.   THE RELATION ALGEBRA
// ===========================================================================
//
//   Four relations, and the quantifier is what distinguishes them. Each is
// written to make its own quantifier visible rather than sharing a helper that
// would hide it.

bool
d_option_set_key_equal(
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs
)
{
    uint32_t i;

    if ((_lhs == NULL) || (_rhs == NULL))
    {
        return false;
    }

    //   equal cardinality plus one-way containment gives set equality, because
    // keys are unique by invariant.
    if (_lhs->count != _rhs->count)
    {
        return false;
    }

    for (i = 0u; i < _lhs->count; ++i)
    {
        if (!d_option_set_contains(_rhs, _lhs->options[i].key))
        {
            return false;
        }
    }

    return true;
}

bool
d_option_set_agrees(
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    uint32_t i;
    uint32_t j;

    if ((_lhs == NULL) || (_rhs == NULL))
    {
        return false;
    }

    //   AGREEMENT QUANTIFIES OVER THE OVERLAP ONLY. Keys present in just one
    // set are not consulted at all, which is why disjoint sets agree
    // vacuously -- and why this relation is NOT transitive.
    for (i = 0u; i < _lhs->count; ++i)
    {
        if (!d_option_set_find(_rhs, _lhs->options[i].key, &j))
        {
            continue;
        }

        if (!d_option_eq(&_lhs->options[i], _lhs->values,
                         &_rhs->options[j], _rhs->values,
                         _project, _compare, _context))
        {
            return false;
        }
    }

    return true;
}

bool
d_option_set_conflicts(
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context,
    uint64_t*                  _out_key
)
{
    uint32_t i;
    uint32_t j;

    if ((_lhs == NULL) || (_rhs == NULL))
    {
        return false;
    }

    for (i = 0u; i < _lhs->count; ++i)
    {
        if (!d_option_set_find(_rhs, _lhs->options[i].key, &j))
        {
            continue;
        }

        if (!d_option_eq(&_lhs->options[i], _lhs->values,
                         &_rhs->options[j], _rhs->values,
                         _project, _compare, _context))
        {
            //   the witness, so a caller can act on the diagnosis rather than
            // only receive it.
            if (_out_key != NULL)
            {
                *_out_key = _lhs->options[i].key;
            }

            return true;
        }
    }

    return false;
}

bool
d_option_set_equal(
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    //   EQUALITY QUANTIFIES OVER THE UNION: same key set, then identical
    // options at every key. Given equal key sets, "identical on the overlap"
    // and "identical everywhere" coincide, so agreement finishes the job --
    // which is the .tex's own decomposition, not a shortcut.
    if (!d_option_set_key_equal(_lhs, _rhs))
    {
        return false;
    }

    return d_option_set_agrees(_lhs, _rhs, _project, _compare, _context);
}


// ===========================================================================
// VI.  UNION AND INTERSECTION
// ===========================================================================

struct d_option_result
d_option_set_key_intersection(
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    uint64_t*                  _out,
    uint32_t                   _capacity
)
{
    uint32_t i;
    uint32_t n;

    if ((_lhs == NULL) || (_rhs == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    n = 0u;

    for (i = 0u; i < _lhs->count; ++i)
    {
        if (!d_option_set_contains(_rhs, _lhs->options[i].key))
        {
            continue;
        }

        //   the two-call protocol: measure when _out is NULL, and still count
        // when the buffer is short, so the failure carries its own remedy.
        if ((_out != NULL) && (n < _capacity))
        {
            _out[n] = _lhs->options[i].key;
        }

        ++n;
    }

    if ((_out != NULL) && (n > _capacity))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);
    }

    return d_option_ok(n);
}

struct d_option_result
d_option_set_intersection(
    struct d_option_set*       _out,
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    uint32_t i;
    uint32_t j;

    if ((_out == NULL) || (_lhs == NULL) || (_rhs == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ((_out == _lhs) || (_out == _rhs))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    d_option_set_clear(_out);

    for (i = 0u; i < _lhs->count; ++i)
    {
        struct d_option_result added;

        if (!d_option_set_find(_rhs, _lhs->options[i].key, &j))
        {
            continue;
        }

        //   IDENTITY-FILTERED, not key-filtered: a shared key carrying
        // different options belongs to neither set's intersection.
        if (!d_option_eq(&_lhs->options[i], _lhs->values,
                         &_rhs->options[j], _rhs->values,
                         _project, _compare, _context))
        {
            continue;
        }

        added = d_option_set_add_cell(_out, &_lhs->options[i],
                                      d_option_slot_const(&_lhs->options[i],
                                                          _lhs->values));

        if (!added.is_ok)
        {
            return added;
        }
    }

    return d_option_ok(_out->count);
}

struct d_option_result
d_option_set_union(
    struct d_option_set*       _out,
    const struct d_option_set* _lhs,
    const struct d_option_set* _rhs,
    fn_option_project          _project,
    fn_binary_predicate        _compare,
    void*                      _context
)
{
    uint32_t i;

    if ((_out == NULL) || (_lhs == NULL) || (_rhs == NULL))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ((_out == _lhs) || (_out == _rhs))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    //   THE UNION IS PARTIAL. body-options.tex: valid "if, and only if,
    // O1 ~ O2". A conflicting union does not exist, so this is FORMAL -- no
    // amount of extra buffer would make it exist, and a mechanical status
    // would tell the caller to retry.
    if (!d_option_set_agrees(_lhs, _rhs, _project, _compare, _context))
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CONFLICT);
    }

    d_option_set_clear(_out);

    for (i = 0u; i < _lhs->count; ++i)
    {
        struct d_option_result added =
            d_option_set_add_cell(_out, &_lhs->options[i],
                                  d_option_slot_const(&_lhs->options[i],
                                                      _lhs->values));

        if (!added.is_ok)
        {
            return added;
        }
    }

    for (i = 0u; i < _rhs->count; ++i)
    {
        struct d_option_result added;

        //   shared keys are already present, and the sets agree, so either
        // copy would do. Skipping keeps the left set's order.
        if (d_option_set_contains(_lhs, _rhs->options[i].key))
        {
            continue;
        }

        added = d_option_set_add_cell(_out, &_rhs->options[i],
                                      d_option_slot_const(&_rhs->options[i],
                                                          _rhs->values));

        if (!added.is_ok)
        {
            return added;
        }
    }

    return d_option_ok(_out->count);
}


// ===========================================================================
// VII. TRAVERSAL
// ===========================================================================

uint32_t
d_option_set_for_each(
    const struct d_option_set* _set,
    fn_option_visit            _visit,
    void*                      _context
)
{
    uint32_t i;

    if ((_set == NULL) || (_visit == NULL))
    {
        return 0u;
    }

    for (i = 0u; i < _set->count; ++i)
    {
        if (!_visit(&_set->options[i], _set->values, i, _context))
        {
            //   a stopped walk reports how far it got, which is what gives
            // quantifiers and early-exit searches the same shape as a full
            // traversal.
            return (i + 1u);
        }
    }

    return _set->count;
}

bool
d_option_set_fold(
    const struct d_option_set* _set,
    void*                      _accumulator,
    fn_accumulator             _step,
    void*                      _context
)
{
    uint32_t i;

    if ((_set == NULL) || (_step == NULL))
    {
        return false;
    }

    for (i = 0u; i < _set->count; ++i)
    {
        if (!_step(_accumulator, &_set->options[i], _context))
        {
            return false;
        }
    }

    return true;
}

bool
d_option_set_is_canon(
    const struct d_option_set* _set
)
{
    uint32_t i;

    if (_set == NULL)
    {
        return false;
    }

    for (i = 1u; i < _set->count; ++i)
    {
        if (_set->options[i - 1u].key > _set->options[i].key)
        {
            return false;
        }
    }

    return true;
}

struct d_option_result
d_option_set_canon(
    struct d_option_set* _set
)
{
    uint32_t i;
    uint32_t j;

    if (_set == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    //   CELLS ARE PERMUTED; THE VALUE BLOCK IS NOT TOUCHED.
    //   Each cell carries its own offset, so reordering the schema does not
    // move a single byte of value. That is the payoff of offsets over
    // pointers, and it is what makes canonicalisation allocation-free: an
    // in-place repack would need either scratch space or a cycle-following
    // permutation with an unbounded temporary, and neither is available on a
    // fixed-storage tier.
    //   Insertion sort: the arrays are small, it is stable, and stability
    // matters because it makes canon idempotent on an already-sorted set.
    for (i = 1u; i < _set->count; ++i)
    {
        struct d_option pivot = _set->options[i];

        j = i;

        while ((j > 0u) && (_set->options[j - 1u].key > pivot.key))
        {
            _set->options[j] = _set->options[j - 1u];
            --j;
        }

        _set->options[j] = pivot;
    }

    return d_option_ok(_set->count);
}

size_t
d_option_set_byte_size(
    const struct d_option_set* _set
)
{
    if (_set == NULL)
    {
        return 0u;
    }

    //   arithmetic, not a traversal -- goal 4's runtime sizeof for a flat set.
    return ( (sizeof(struct d_option) * (size_t)_set->count) +
             (size_t)_set->value_used );
}
