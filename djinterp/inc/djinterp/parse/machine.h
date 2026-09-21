/******************************************************************************
* djinterp [parse]                                                   machine.h
*
* The operator-agnostic execution substrate.
*   A machine is run-state with no opinion about what is being run: a step
* budget, a halt/result pair, a diagnostic channel, and `ext`, an opaque
* pointer a driver aims at its family's private state for the duration of a
* run. An op_set maps an opcode to the operator that implements it. A driver
* fetches, looks up, and calls. There is no switch on opcode anywhere in this
* file, and adding capability to a family means adding a handler, never
* editing a dispatch loop.
*
*   WHAT IS DELIBERATELY ABSENT. The substrate holds no input cursor and names
* no element type. It formerly carried `const char* input` and an `sp`, which
* silently made every family a byte-at-a-time recogniser over one contiguous
* string. A family that reads tokens, or one that reads nothing at all and
* only emits -- a generator -- paid for a cursor it could not use, and a driver
* over a token stream had nowhere to put its own. Both now live behind `ext`,
* where every other piece of family-private state already lived.
*
*   WHY A TABLE AND NOT A LOOKUP. An opcode space is family-private and dense
* from zero by construction, so the registry is an array indexed by opcode and
* a find is a bounds check plus a load. meta/lookup.h is the right module for
* records whose keys are sparse or opaque; an opcode is neither.
*
*   Requires: c/djinterp.h (qualifier kit), config/parse/cfg_parse.h, and
*             parse/diagnostic.h (the channel a halted run reports through).
*
* path:      /inc/djinterp/parse/machine.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE MACHINE
    -----------
    1.  Tracing
         1.  d_parse_trace_hook
    2.  Run-state
         1.  d_parse_machine
    3.  Run control
         1.  d_parse_machine_init
         2.  d_parse_machine_halt
         3.  d_parse_machine_step
         4.  d_parse_machine_fail
2.  THE OPERATOR REGISTRY
    ---------------------
    1.  The operator
         1.  d_parse_voperator
         2.  d_parse_op
    2.  The registry
         1.  d_parse_op_set
         2.  Registry flags
              a. D_PARSE_OP_SET_OWNS_OPS
    3.  Registry lifetime
    4.  Registration and dispatch
    5.  Coverage
*/

#ifndef DJINTERP_PARSE_MACHINE_
#define DJINTERP_PARSE_MACHINE_ 1

// std
#include <stdint.h>                         // uint8_t, uint16_t, uint32_t
// djinterp
#include "../c/djinterp.h"                  // framework root
#include "../config/parse/cfg_parse.h"      // D_INTERNAL_PARSE_* knobs
#include "./diagnostic.h"                   // the channel a halted run reports
                                            // through, and d_parse_span


//==============================================================================
// 1.  THE MACHINE
//==============================================================================


// 1.1    Tracing
//------------------------------------------------------------------------------
// 1.1.1
// d_parse_machine (forward)
//   struct: declared ahead of the hook so the tag the hook names is the file's
// tag and not one scoped to that prototype.
struct d_parse_machine;

// d_parse_trace_hook
//   type: called once per dispatched operator when tracing is compiled in,
// with the opcode about to run and the name the registry holds for it. Present
// only when D_INTERNAL_PARSE_MACHINE_TRACE is 1.
typedef void (*d_parse_trace_hook)(void*                         _ctx,
                                   const struct d_parse_machine* _machine,
                                   int                           _code,
                                   const char*                   _name);


// 1.2    Run-state
//------------------------------------------------------------------------------
// 1.2.1
// d_parse_machine
//   struct: the shared run-state. Public-payload layout -- the substrate IS
// the protocol the operators manipulate -- so these members are unprefixed and
// written directly by handlers.
//   `ok` is meaningful only once `halted` is set. `step_limit` of 0 disables
// the runaway guard. `diag` may be NULL, in which case a halted run reports
// nothing and the caller learns only `ok`.
//   NOTE: D_INTERNAL_PARSE_MACHINE_TRACE changes this layout, so it must hold
// the same value in every translation unit that touches a machine.
struct d_parse_machine
{
    long                      steps;
    long                      step_limit;
    int                       halted;
    int                       ok;
    void*                     ext;
    struct d_parse_diag_sink* diag;
#if (D_INTERNAL_PARSE_MACHINE_TRACE == 1)
    d_parse_trace_hook        trace;
    void*                     trace_ctx;
#endif  // D_INTERNAL_PARSE_MACHINE_TRACE
};


// 1.3    Run control
//------------------------------------------------------------------------------
D_EXTERN_C_BEGIN

/*
d_parse_machine_init
  Initialises a machine to a fresh, un-halted run against a diagnostic sink.
NOTE:
  The step budget starts at D_PARSE_MACHINE_STEP_LIMIT; a driver that wants a
different guard assigns step_limit after this call.

Parameter(s):
  _machine: the machine to initialise; ignored if NULL.
  _diag:    the sink a halted run reports through; may be NULL.
Return:
  none.
*/
D_INLINE void
d_parse_machine_init(
    struct d_parse_machine*   _machine,
    struct d_parse_diag_sink* _diag
)
{
    if (!_machine)
    {
        return;
    }

    _machine->steps      = 0L;
    _machine->step_limit = D_PARSE_MACHINE_STEP_LIMIT;
    _machine->halted     = 0;
    _machine->ok         = 0;
    _machine->ext        = NULL;
    _machine->diag       = _diag;
#if (D_INTERNAL_PARSE_MACHINE_TRACE == 1)
    _machine->trace      = NULL;
    _machine->trace_ctx  = NULL;
#endif  // D_INTERNAL_PARSE_MACHINE_TRACE

    return;
}

/*
d_parse_machine_halt
  Stops the run and records its outcome.

Parameter(s):
  _machine: the machine to halt; ignored if NULL.
  _ok:      non-zero if the run succeeded.
Return:
  none.
*/
D_INLINE void
d_parse_machine_halt(
    struct d_parse_machine* _machine,
    int                     _ok
)
{
    if (!_machine)
    {
        return;
    }

    _machine->halted = 1;
    _machine->ok     = (_ok != 0) ? 1 : 0;

    return;
}

void d_parse_machine_fail(struct d_parse_machine* _machine,
                          uint16_t                _domain,
                          uint16_t                _code,
                          struct d_parse_span     _span,
                          const char*             _message);

/*
d_parse_machine_step
  Charges one dispatch against the step budget.
NOTE:
  A driver calls this once per operator, before dispatching. Exceeding the
budget halts the run and reports D_PARSE_DIAG_STEP_LIMIT, so a family whose
program loops forever fails as a diagnosed error rather than as a hang.

Parameter(s):
  _machine: the machine being run; a NULL machine cannot continue.
Return:
  A boolean value corresponding to either:
  - 1, if the run may continue, or
  - 0, if it is halted or has just exhausted its budget.
*/
D_INLINE int
d_parse_machine_step(
    struct d_parse_machine* _machine
)
{
    // a missing or already-stopped machine dispatches nothing further
    if ( (!_machine) ||
         (_machine->halted) )
    {
        return 0;
    }

    _machine->steps++;

    // check the runaway guard, which a step_limit of 0 disables
    if ( (_machine->step_limit > 0L) &&
         (_machine->steps > _machine->step_limit) )
    {
        d_parse_machine_fail(_machine,
                             (uint16_t)D_PARSE_DIAG_DOMAIN_MACHINE,
                             (uint16_t)D_PARSE_DIAG_STEP_LIMIT,
                             d_parse_span_unknown(),
                             "step limit exceeded");

        return 0;
    }

    return 1;
}

D_EXTERN_C_END


//==============================================================================
// 2.  THE OPERATOR REGISTRY
//==============================================================================


// 2.1    The operator
//------------------------------------------------------------------------------
// 2.1.1
// d_parse_voperator
//   type: a parsing operator -- an effect on the machine. A plain function
// pointer plus a context pointer rather than a closure object, so the
// indirection is one call through a known signature, the registry is a POD,
// and a handler written in either language is callable from the other.
typedef void (*d_parse_voperator)(struct d_parse_machine* _machine,
                                  void*                   _ctx);

// 2.1.2
// d_parse_op
//   struct: one registered operator: what to call, what to pass it, and what
// to call it in a trace or a disassembly.
struct d_parse_op
{
    d_parse_voperator fn;
    void*             ctx;
    const char*       name;
};


// 2.2    The registry
//------------------------------------------------------------------------------
// 2.2.1
// d_parse_op_set
//   struct: a family's opcode-to-operator table. `count` is one past the
// highest opcode ever defined, so it bounds a find; entries below it with a
// NULL `fn` are holes and find reports them as absent. A driver dispatches
// only through this table and never names an opcode, which is what keeps the
// machine operator-agnostic and opcode spaces family-private.
//   `family` names WHICH private space, so a program written for one family
// and a registry reading another can be caught rather than merely documented
// -- see d_parse_program_verify. Two registries over one opcode space, such as
// an interpreter and a code generator, share a family and differ only in what
// their handlers do.
struct d_parse_op_set
{
    struct d_parse_op* ops;
    uint32_t           count;
    uint32_t           capacity;
    uint16_t           family;
    uint8_t            flags;
    uint8_t            reserved;
};

// 2.2.2
// D_PARSE_OP_SET_OWNS_OPS
//   constant: the table was allocated by the registry and is freed by
// d_parse_op_set_release.
#define D_PARSE_OP_SET_OWNS_OPS         0x01u


// 2.3    Registry lifetime
//------------------------------------------------------------------------------
D_EXTERN_C_BEGIN

void            d_parse_op_set_init(struct d_parse_op_set* _set,
                                    uint16_t               _family,
                                    struct d_parse_op*     _ops,
                                    uint32_t               _capacity);
#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
D_NODISCARD int d_parse_op_set_init_heap(struct d_parse_op_set* _set,
                                         uint16_t               _family,
                                         uint32_t               _capacity);
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP
void            d_parse_op_set_release(struct d_parse_op_set* _set);


// 2.4    Registration and dispatch
//------------------------------------------------------------------------------
D_NODISCARD int d_parse_op_set_def(struct d_parse_op_set* _set,
                                   int                    _code,
                                   const char*            _name,
                                   d_parse_voperator      _fn,
                                   void*                  _ctx);

/*
d_parse_op_set_find
  The operator registered for an opcode.

Parameter(s):
  _set:  the registry to search; may be NULL.
  _code: the opcode, in this family's private space.
Return:
  A pointer to the registered operator, or NULL when the opcode is out of
range or is a hole in the table.
*/
D_INLINE const struct d_parse_op*
d_parse_op_set_find(
    const struct d_parse_op_set* _set,
    int                          _code
)
{
    // reject a missing registry, a negative or out-of-range opcode, and a hole
    if ( (!_set)                            ||
         (!_set->ops)                       ||
         (_code < 0)                        ||
         ((uint32_t)_code >= _set->count)   ||
         (!_set->ops[_code].fn)             )
    {
        return NULL;
    }

    return &_set->ops[_code];
}

/*
d_parse_op_set_name
  The display name registered for an opcode.

Parameter(s):
  _set:  the registry to search; may be NULL.
  _code: the opcode, in this family's private space.
Return:
  The name, or "?" when the opcode is not registered. Never NULL.
*/
D_INLINE const char*
d_parse_op_set_name(
    const struct d_parse_op_set* _set,
    int                          _code
)
{
    const struct d_parse_op* op = d_parse_op_set_find(_set, _code);

    if ( (!op) ||
         (!op->name) )
    {
        return "?";
    }

    return op->name;
}

/*
d_parse_op_set_dispatch
  Charges a step, then runs the operator registered for an opcode.
NOTE:
  This is the whole of a driver's inner loop besides the fetch. An unregistered
opcode halts the run and reports D_PARSE_DIAG_UNKNOWN_OP rather than being
ignored, because a program holding an opcode its family does not implement is
a generator bug and silence would bury it.

Parameter(s):
  _machine: the machine to run the operator against; ignored if NULL.
  _set:     the family's registry.
  _code:    the opcode to dispatch.
Return:
  A boolean value corresponding to either:
  - 1, if an operator ran and the machine may continue, or
  - 0, if the run is halted, out of budget, or the opcode is unregistered.
*/
D_INLINE int
d_parse_op_set_dispatch(
    struct d_parse_machine*      _machine,
    const struct d_parse_op_set* _set,
    int                          _code
)
{
    if (!d_parse_machine_step(_machine))
    {
        return 0;
    }

    const struct d_parse_op* op = d_parse_op_set_find(_set, _code);

    // an opcode with no handler is a hard error, not a no-op
    if (!op)
    {
        d_parse_machine_fail(_machine,
                             (uint16_t)D_PARSE_DIAG_DOMAIN_MACHINE,
                             (uint16_t)D_PARSE_DIAG_UNKNOWN_OP,
                             d_parse_span_unknown(),
                             "no operator registered for opcode");

        return 0;
    }

#if (D_INTERNAL_PARSE_MACHINE_TRACE == 1)
    // report the dispatch before it happens, so a trace shows the step that
    // faulted rather than stopping one short of it
    if (_machine->trace)
    {
        _machine->trace(_machine->trace_ctx, _machine, _code, op->name);
    }
#endif  // D_INTERNAL_PARSE_MACHINE_TRACE

    op->fn(_machine, op->ctx);

    return (_machine->halted) ? 0 : 1;
}


// 2.5    Coverage
//------------------------------------------------------------------------------
D_NODISCARD int d_parse_op_set_covers(const struct d_parse_op_set* _set,
                                      const struct d_parse_op_set* _reference,
                                      int*                         _missing);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_MACHINE_
