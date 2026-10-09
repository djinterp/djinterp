/*******************************************************************************
* djinterp [parse]                                     parse_substrate_tests.cpp
*
* The execution-substrate suite's sections: the diagnostic channel, the machine
* and its operator registry, and the two faces being one set of objects.
*   A REGISTRATION SHAPE HAS A SECTION FROM THE LEVEL IT EXISTS AT. An empty
* functor, a caller-owned object and a raw C-ABI function register at C++11.
* A function passed by address is a template<auto> parameter, which is C++17.
* A captureless lambda is default-constructible, which the stateless
* trampoline needs of it, only from C++20. Each of the three is its own
* section, absent below its level rather than present and empty, so the count
* a runner prints is the count of things that were checked.
*
*
* path:      /tests/djinterp/parse/parse_substrate_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parse_substrate_tests.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstring>  // std::strcmp
// djinterp
#include "../../../inc/djinterp/parse/diagnostic.hpp"  // the sink's C++ face
#include "../../../inc/djinterp/parse/machine.hpp"     // machine, op_set


namespace
{

using namespace djinterp::parse;

// test opcodes -- a throwaway family's private space, dense from zero, which
// is what every real family's opcode enum looks like
enum
{
    OP_TICK = 0,
    OP_ADD,
    OP_STOP,
    OP_COUNT
};

// the family identifier the test's opcode space belongs to
enum
{
    FAMILY_TEST = 7
};

// PROGRAM
//   constant: two ticks, an add and a stop -- every opcode of the family once
// at least, ending on the one that halts.
const int PROGRAM[] = { OP_TICK, OP_TICK, OP_ADD, OP_STOP };

// counter
//   struct: the throwaway family's private state, reached through machine.ext.
struct counter
{
    int ticks;
    int total;
};

// tick_op
//   struct: a stateless operator -- an empty functor, the callable def() takes
// at every level. It reaches its family's state the way every family does,
// through ext.
struct tick_op
{
    void
    operator()(
        machine& _machine
    ) const
    {
        _machine.extension<counter>()->ticks++;

        return;
    }
};

// add_op
//   struct: an operator that owns state. Its caller keeps it alive, and the
// registry reaches it through the context slot.
struct add_op
{
    int amount;

    void
    operator()(
        machine& _machine
    ) const
    {
        _machine.extension<counter>()->total += amount;

        return;
    }
};

// nop_op
//   struct: an operator that does nothing, for a registry that exists only to
// be compared with another.
struct nop_op
{
    void
    operator()(
        machine& _machine
    ) const
    {
        (void)_machine;

        return;
    }
};

// stop_raw
//   function: a raw C-ABI operator, the form a C family registers.
void
stop_raw(
    d_parse_machine* _machine,
    void*            _ctx
)
{
    (void)_ctx;

    d_parse_machine_halt(_machine, 1);

    return;
}

// drive
//   function: the whole of a driver's inner loop -- fetch, dispatch, repeat --
// over PROGRAM, and whether the run ended as it must: halted successfully, two
// ticks counted, seven added, and nothing reported.
bool
drive(
    const op_set& _ops,
    counter&      _state
)
{
    fixed_diagnostics<4u, 128u> diag;
    machine                     vm(diag);

    vm.bind(_state);

    // fetch, dispatch, and stop when an operator halts the run
    for (const int code : PROGRAM)
    {
        if (!_ops.dispatch(vm, code))
        {
            break;
        }
    }

    return ( (vm.succeeded())     &&
             (_state.ticks == 2)  &&
             (_state.total == 7)  &&
             (!diag.failed())     );
}

// fill
//   function: a warning, a note under it, and an error the two-slot sink has
// no room to store -- the state the storage and identity sections both read.
void
fill(
    fixed_diagnostics<2u, 128u>& _sink
)
{
    (void)_sink.warning(D_PARSE_DIAG_DOMAIN_CORE, 2u, span(3u, 4u), "careful");
    (void)_sink.note(D_PARSE_DIAG_DOMAIN_CORE, 3u, span(3u, 4u), "because");
    (void)_sink.error(D_PARSE_DIAG_DOMAIN_CORE, 4u, span(), "too many");

    return;
}

/*
d_tests_parse_diagnostics_tally
  What a sink counts, with and without storage.
  Tests the following:
  - a sink with no storage still tallies and still answers failed()
  - nothing is stored where there is nowhere to store it
  - severity filtering rejects below the threshold and counts nothing
*/
bool
d_tests_parse_diagnostics_tally(void)
{
    // a counting-only sink: no storage at all
    diagnostics counting;

    if (counting.failed())
    {
        return false;
    }

    (void)counting.error(D_PARSE_DIAG_DOMAIN_CORE, 1u, span(0u, 1u), "boom");

    if ( (!counting.failed())                        ||
         (counting.size() != 0u)                     ||
         (counting.tally_of(severity::error) != 1u) )
    {
        return false;
    }

    // filtering: a rejected diagnostic is neither stored nor counted
    fixed_diagnostics<4u, 128u> filtered;

    filtered.filter(severity::error);

    (void)filtered.warning(D_PARSE_DIAG_DOMAIN_CORE, 5u, span(), "ignored");

    return ( (filtered.size() == 0u)                          &&
             (filtered.tally_of(severity::warning) == 0u) );
}

/*
d_tests_parse_diagnostics_storage
  What a sink with embedded storage keeps.
  Tests the following:
  - message text round-trips through the arena by offset
  - continuation notes are distinguishable from standalone diagnostics
  - a full sink keeps tallying, marks itself truncated, and reports dropped
  - iteration yields every stored record, in order
*/
bool
d_tests_parse_diagnostics_storage(void)
{
    fixed_diagnostics<2u, 128u> fixed;

    fill(fixed);

    if ( (std::strcmp(fixed[0].text(), "careful") != 0)  ||
         (std::strcmp(fixed[1].text(), "because") != 0)  ||
         (fixed[0].continuation())                       ||
         (!fixed[1].continuation())                      ||
         (fixed[0].where().offset != 3u)                 )
    {
        return false;
    }

    // overflow: the third diagnostic is counted but not stored
    if ( (fixed.size() != 2u)                      ||
         (!fixed.truncated())                      ||
         (fixed.dropped != 1u)                     ||
         (!fixed.failed())                         ||
         (fixed.tally_of(severity::error) != 1u)   )
    {
        return false;
    }

    int seen = 0;

    // every stored record is reachable, and is a valid view
    for (const diagnostic_view entry : fixed)
    {
        if (!entry.valid())
        {
            return false;
        }

        seen++;
    }

    return (seen == 2);
}

/*
d_tests_parse_diagnostics_identity
  A diagnostic is identified by its domain and its code together.
  Tests the following:
  - a stored condition is found by the pair
  - a dropped one is not, though the tally still counts it
  - the same code under another domain is a different condition
*/
bool
d_tests_parse_diagnostics_identity(void)
{
    fixed_diagnostics<2u, 128u> fixed;

    fill(fixed);

    return ( (fixed.contains(D_PARSE_DIAG_DOMAIN_CORE, 2u))        &&
             (!fixed.contains(D_PARSE_DIAG_DOMAIN_CORE, 4u))       &&
             (!fixed.contains(D_PARSE_DIAG_DOMAIN_MACHINE, 2u))    &&
             (fixed[0].is(D_PARSE_DIAG_DOMAIN_CORE, 2u))           &&
             (!fixed[0].is(D_PARSE_DIAG_DOMAIN_MACHINE, 2u))       );
}

/*
d_tests_parse_machine_dispatch
  The registration shapes every level has, and the dispatch through them.
  Tests the following:
  - the machine carries no input cursor and no element type
  - dispatch routes through the registry with no switch on opcode
  - an empty functor, a caller-owned object and a raw C-ABI function each
    reach their handler
  - family state travels through ext and through a context pointer
  - a fixed registry allocates nothing
*/
bool
d_tests_parse_machine_dispatch(void)
{
    fixed_op_set<OP_COUNT> ops(FAMILY_TEST);
    counter                state = { 0, 0 };
    add_op                 adder = { 7 };

    if ( (!ops.def(OP_TICK, "TICK", tick_op()))             ||
         (!ops.def_object(OP_ADD, "ADD", adder))            ||
         (!ops.def_raw(OP_STOP, "STOP", &stop_raw))         ||
         (std::strcmp(ops.name_of(OP_ADD), "ADD") != 0)     )
    {
        return false;
    }

    return drive(ops, state);
}

/*
d_tests_parse_machine_faults
  What stops a run that cannot go on.
  Tests the following:
  - an unregistered opcode halts the run and reports it, never a silent no-op
  - the step budget halts a runaway program and reports it
  - each is identified by the machine's domain and its own code
*/
bool
d_tests_parse_machine_faults(void)
{
    fixed_diagnostics<4u, 256u> diag;
    fixed_op_set<OP_COUNT>      ops(FAMILY_TEST);
    counter                     state = { 0, 0 };

    if (!ops.def(OP_TICK, "TICK", tick_op()))
    {
        return false;
    }

    machine unknown(diag);

    // an opcode nobody registered is a diagnosed error
    if ( (ops.dispatch(unknown, OP_COUNT + 3))                         ||
         (unknown.succeeded())                                        ||
         (!diag.contains(D_PARSE_DIAG_DOMAIN_MACHINE,
                         D_PARSE_DIAG_UNKNOWN_OP))                    )
    {
        return false;
    }

    diag.clear();

    machine runaway(diag);

    runaway.bind(state);
    runaway.step_limit = 16L;

    // the handler never halts; the budget must
    while (ops.dispatch(runaway, OP_TICK))
    {
        continue;
    }

    return ( (!runaway.succeeded())                               &&
             (diag.size() == 1u)                                  &&
             (diag[0].is(D_PARSE_DIAG_DOMAIN_MACHINE,
                         D_PARSE_DIAG_STEP_LIMIT))                );
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
// tick_fn
//   function: a free operator taking only the machine, registered by address.
void
tick_fn(
    machine& _machine
)
{
    _machine.extension<counter>()->ticks++;

    return;
}

// add_fn
//   function: a free operator over the machine and its family's state, bound
// to that state at registration.
void
add_fn(
    machine& _machine,
    counter& _state
)
{
    (void)_machine;

    _state.total += 7;

    return;
}

/*
d_tests_parse_machine_by_address
  The registration shapes that take a function as a template argument.
  Tests the following:
  - a free function over the machine registers by its address
  - a free function over the machine and its family's state registers by its
    address, with the state bound at registration
  - both dispatch to the same result the level-independent shapes reach
*/
bool
d_tests_parse_machine_by_address(void)
{
    fixed_op_set<OP_COUNT> ops(FAMILY_TEST);
    counter                state = { 0, 0 };

    if ( (!ops.def<&tick_fn>(OP_TICK, "TICK"))              ||
         (!ops.def_state<&add_fn>(OP_ADD, "ADD", state))    ||
         (!ops.def_raw(OP_STOP, "STOP", &stop_raw))         )
    {
        return false;
    }

    return drive(ops, state);
}
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
/*
d_tests_parse_machine_lambda
  The registration shape that takes a captureless lambda.
  Tests the following:
  - a captureless lambda registers as a stateless operator
  - a captureless lambda converts to the raw C-ABI operator type
  - both dispatch to the same result the level-independent shapes reach
*/
bool
d_tests_parse_machine_lambda(void)
{
    fixed_op_set<OP_COUNT> ops(FAMILY_TEST);
    counter                state = { 0, 0 };
    add_op                 adder = { 7 };

    const auto tick = [](machine& _machine)
    {
        _machine.extension<counter>()->ticks++;
    };

    const voperator stop = [](d_parse_machine* _machine, void*)
    {
        d_parse_machine_halt(_machine, 1);
    };

    if ( (!ops.def(OP_TICK, "TICK", tick))           ||
         (!ops.def_object(OP_ADD, "ADD", adder))     ||
         (!ops.def_raw(OP_STOP, "STOP", stop))       )
    {
        return false;
    }

    return drive(ops, state);
}
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

/*
d_tests_parse_machine_coverage
  Whether one registry implements every opcode another one defines.
  Tests the following:
  - a registry covers one that defines no more than it does
  - a registry missing an opcode does not cover, and the lowest opcode it is
    missing is reported
*/
bool
d_tests_parse_machine_coverage(void)
{
    fixed_op_set<OP_COUNT> full(FAMILY_TEST);
    fixed_op_set<OP_COUNT> partial(FAMILY_TEST);

    if ( (!full.def(OP_TICK, "TICK", nop_op()))      ||
         (!full.def(OP_ADD,  "ADD",  nop_op()))      ||
         (!full.def(OP_STOP, "STOP", nop_op()))      ||
         (!partial.def(OP_TICK, "TICK", nop_op()))   ||
         (!partial.def(OP_STOP, "STOP", nop_op()))   )
    {
        return false;
    }

    int missing = 0;

    if (!full.covers(partial, &missing))
    {
        return false;
    }

    return ( (!partial.covers(full, &missing))  &&
             (missing == OP_ADD)                );
}

/*
d_tests_parse_interop
  The claim that the C++ faces and the C types are the same objects.
  Tests the following:
  - a C++ machine is accepted by a C entry point with no conversion
  - a C++ sink is accepted by a C entry point with no conversion
  - what a C entry point reports is read back through the C++ face
*/
bool
d_tests_parse_interop(void)
{
    fixed_diagnostics<4u, 128u> diag;
    machine                     vm(diag);

    // the C entry points take the C++ objects directly -- this compiling at
    // all is most of what the section asserts
    d_parse_machine_init(&vm, &diag);
    d_parse_machine_fail(&vm,
                         D_PARSE_DIAG_DOMAIN_MACHINE,
                         D_PARSE_DIAG_BAD_ARGUMENT,
                         d_parse_span_make(1u, 2u),
                         "from C");

    return ( (!vm.running())                                  &&
             (diag.failed())                                  &&
             (std::strcmp(diag[0].text(), "from C") == 0)     &&
             (diag[0].where().length == 2u)                   );
}

}  // namespace

// d_tests_parse_substrate
//   constant: the suite -- every section this build has, the most fundamental
// first, so that the first failure reported is the one to look at.
const d_tests_section d_tests_parse_substrate[] =
{
    { "parse_diagnostics_tally",    &d_tests_parse_diagnostics_tally    },
    { "parse_diagnostics_storage",  &d_tests_parse_diagnostics_storage  },
    { "parse_diagnostics_identity", &d_tests_parse_diagnostics_identity },
    { "parse_machine_dispatch",     &d_tests_parse_machine_dispatch     },
    { "parse_machine_faults",       &d_tests_parse_machine_faults       },
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    { "parse_machine_by_address",   &d_tests_parse_machine_by_address   },
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    { "parse_machine_lambda",       &d_tests_parse_machine_lambda       },
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
    { "parse_machine_coverage",     &d_tests_parse_machine_coverage     },
    { "parse_interop",              &d_tests_parse_interop              }
};

// d_tests_parse_substrate_count
//   constant: how many sections the table above holds in this build.
const std::size_t d_tests_parse_substrate_count =
    sizeof(d_tests_parse_substrate) / sizeof(d_tests_parse_substrate[0]);

#endif  // floor, for now
