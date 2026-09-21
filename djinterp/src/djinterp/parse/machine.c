/******************************************************************************
* djinterp [parse]                                                   machine.c
*
*   Definitions for the non-inline declarations in machine.h.
*
*
* path:      /src/djinterp/parse/machine.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parse/machine.h"  // corresponding header
// std
#include <string.h>  // memset
// djinterp
#include "../../../inc/djinterp/parse/storage.h"  // d_parse_grow, the shared
                                                  // growth policy
#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP


/*
d_parse_machine_fail
  Halts a run as failed and reports why.
NOTE:
  This is the only way a family should stop on an error. The machine carries no
message field of its own: the reason belongs in the shared diagnostic channel,
where every other stage's findings already are, in order.

Parameter(s):
  _machine: the machine to halt; ignored if NULL.
  _domain:  the emitting family's diagnostic domain.
  _code:    the condition, in that domain's private code space.
  _span:    where in the subject the failure occurred.
  _message: human-readable text; may be NULL.
Return:
  none.
*/
void
d_parse_machine_fail(
    struct d_parse_machine* _machine,
    uint16_t                _domain,
    uint16_t                _code,
    struct d_parse_span     _span,
    const char*             _message
)
{
    if (!_machine)
    {
        return;
    }

    d_parse_machine_halt(_machine, 0);

    // a machine with no sink still halts; it just reports nothing
    if (_machine->diag)
    {
        (void)d_parse_diag_emit(_machine->diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                _domain,
                                _code,
                                _span,
                                _message);
    }

    return;
}


/*
d_parse_op_set_init
  Initialises a registry over a caller-supplied table.
NOTE:
  The table must be zeroed or the registry cannot tell a hole from a handler;
this zeroes it.

Parameter(s):
  _set:      the registry to initialise; ignored if NULL.
  _family:   which private opcode space this registry reads.
  _ops:      storage for the opcode table; may be NULL for an empty registry.
  _capacity: how many opcodes _ops holds, which caps the opcode space.
Return:
  none.
*/
void
d_parse_op_set_init(
    struct d_parse_op_set* _set,
    uint16_t               _family,
    struct d_parse_op*     _ops,
    uint32_t               _capacity
)
{
    if (!_set)
    {
        return;
    }

    memset(_set, 0, sizeof(*_set));

    _set->ops      = _ops;
    _set->capacity = (_ops != NULL) ? _capacity : 0u;
    _set->family   = _family;

    // clear the table, so every slot reads as a hole until it is defined
    if ( (_ops != NULL) &&
         (_capacity > 0u) )
    {
        memset(_ops, 0, (size_t)_capacity * sizeof(*_ops));
    }

    return;
}


#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)

/*
d_parse_op_set_init_heap
  Initialises a registry over a table it allocates and owns.
NOTE:
  The capacity is a starting point, not a cap: defining an opcode beyond it
grows the table.

Parameter(s):
  _set:      the registry to initialise; ignored if NULL.
  _family:   which private opcode space this registry reads.
  _capacity: opcodes to reserve room for; 0 reserves nothing and defers the
             first allocation to the first definition.
Return:
  0 on success; -1 if _set is NULL or the allocation was refused.
*/
int
d_parse_op_set_init_heap(
    struct d_parse_op_set* _set,
    uint16_t               _family,
    uint32_t               _capacity
)
{
    if (!_set)
    {
        return -1;
    }

    d_parse_op_set_init(_set, _family, NULL, 0u);

    // an empty reservation is valid; the first def will allocate
    if (_capacity == 0u)
    {
        _set->flags = (uint8_t)D_PARSE_OP_SET_OWNS_OPS;

        return 0;
    }

    struct d_parse_op* table =
        (struct d_parse_op*)malloc((size_t)_capacity * sizeof(*table));

    // check if memory allocation was successful
    if (!table)
    {
        return -1;
    }

    memset(table, 0, (size_t)_capacity * sizeof(*table));

    _set->ops      = table;
    _set->capacity = _capacity;
    _set->flags    = (uint8_t)D_PARSE_OP_SET_OWNS_OPS;

    return 0;
}


/*
d_parse_op_set_internal_grow
  Grows an owned table to hold at least one more opcode.
NOTE:
  The shared policy zeroes what it adds, which is what keeps an undefined
opcode reading as a hole rather than as whatever the allocator returned.

Parameter(s):
  _set:      the registry to grow.
  _required: the number of slots the table must hold afterwards.
Return:
  0 on success; -1 if the registry does not own its table or the reallocation
was refused.
*/
static int
d_parse_op_set_internal_grow(
    struct d_parse_op_set* _set,
    uint32_t               _required
)
{
    void* const grown = d_parse_grow(_set->ops,
                                     &_set->capacity,
                                     _required,
                                     (uint32_t)sizeof(*_set->ops),
                                     (_set->flags &
                                      D_PARSE_OP_SET_OWNS_OPS) != 0u);

    // check if memory allocation was successful
    if (!grown)
    {
        return -1;
    }

    _set->ops = (struct d_parse_op*)grown;

    return 0;
}

#endif  // D_INTERNAL_PARSE_OP_SET_HEAP


/*
d_parse_op_set_release
  Releases any table the registry owns and leaves it empty.

Parameter(s):
  _set: the registry to release; ignored if NULL.
Return:
  none.
*/
void
d_parse_op_set_release(
    struct d_parse_op_set* _set
)
{
    if (!_set)
    {
        return;
    }

#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
    // free only what this registry allocated; caller storage is never touched
    if ( (_set->ops != NULL) &&
         ((_set->flags & D_PARSE_OP_SET_OWNS_OPS) != 0u) )
    {
        free(_set->ops);
    }
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP

    memset(_set, 0, sizeof(*_set));

    return;
}


/*
d_parse_op_set_def
  Registers an operator under an opcode.
NOTE:
  Redefining an opcode replaces it rather than failing, which is what lets a
family layer an instrumented or specialised handler over a base registry
without rebuilding it.

Parameter(s):
  _set:  the registry to define into; may be NULL.
  _code: the opcode, in this family's private space; must not be negative.
  _name: display name for traces and disassembly; may be NULL.
  _fn:   the operator to call; must not be NULL.
  _ctx:  passed to the operator unchanged; may be NULL.
Return:
  0 on success; -1 on an invalid argument, or when the opcode lies beyond a
fixed table that cannot grow.
*/
int
d_parse_op_set_def(
    struct d_parse_op_set* _set,
    int                    _code,
    const char*            _name,
    d_parse_voperator      _fn,
    void*                  _ctx
)
{
    // reject a missing registry, a negative opcode, or an absent operator
    if ( (!_set)      ||
         (_code < 0)  ||
         (!_fn)       )
    {
        return -1;
    }

    const uint32_t slot = (uint32_t)_code;

    // grow, or refuse, when the opcode lies past the table
    if (slot >= _set->capacity)
    {
#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
        if (d_parse_op_set_internal_grow(_set, slot + 1u) != 0)
        {
            return -1;
        }
#else
        return -1;
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP
    }

    _set->ops[slot].fn   = _fn;
    _set->ops[slot].ctx  = _ctx;
    _set->ops[slot].name = _name;

    // count is one past the highest opcode ever defined, which bounds a find
    if (slot >= _set->count)
    {
        _set->count = slot + 1u;
    }

    return 0;
}


/*
d_parse_op_set_covers
  Whether a registry implements every opcode another one defines.
NOTE:
  This is the gate that keeps a second reading of one opcode space honest. A
family that grows an opcode and teaches only its interpreter about it fails
this check against its other backends, which turns a silent loss of coverage
into a test failure.

Parameter(s):
  _set:       the registry under test; may be NULL.
  _reference: the registry whose opcodes must all be present; may be NULL.
  _missing:   receives the lowest uncovered opcode, or -1 when none; optional.
Return:
  A boolean value corresponding to either:
  - 1, if every opcode defined in _reference is defined in _set, or
  - 0, otherwise.
*/
int
d_parse_op_set_covers(
    const struct d_parse_op_set* _set,
    const struct d_parse_op_set* _reference,
    int*                         _missing
)
{
    // report "nothing missing" before any early return, so the out-parameter
    // is never left holding a stale opcode
    if (_missing)
    {
        *_missing = -1;
    }

    // an empty reference is covered by anything, including a missing registry
    if ( (!_reference)        ||
         (!_reference->ops)   ||
         (_reference->count == 0u) )
    {
        return 1;
    }

    // walk the reference's opcodes in order, so the first gap is the lowest
    for (uint32_t code = 0u; code < _reference->count; code++)
    {
        if (!_reference->ops[code].fn)
        {
            continue;
        }

        if (!d_parse_op_set_find(_set, (int)code))
        {
            if (_missing)
            {
                *_missing = (int)code;
            }

            return 0;
        }
    }

    return 1;
}
