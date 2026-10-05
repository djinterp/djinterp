/*******************************************************************************
* djinterp [parsegen]                                                     lr.cpp
*
*   Definitions for lr.hpp: the LR family's operators and its table-driven
* driver.
*   Consolidated from the standalone prototype. Behaviour on a well-formed table
* is unchanged; what changed is what a malformed one does. The prototype would
* dereference a missing handler, throw from a missing goto, and index past the
* stacks on a reduce deeper than they are. Each of those now halts the run with
* an error, which is how the rest of the system treats an opcode or a state it
* cannot act on.
*
*
* path:      /src/djinterp/parsegen/vparse/lr.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.01
*******************************************************************************/
#include "djinterp/parsegen/vparse/lr.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has
// std
#include <cstddef>                          // std::size_t
#include <string>                           // std::string, std::to_string
#include <utility>                          // std::move
#include <vector>                           // std::vector


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace lr
{

namespace
{

// state
//   struct: this family's private run state, reached through machine::ext for
// the duration of one run. The value stack runs one shorter than the state
// stack, since the initial state has no value beneath it.
struct state
{
    const table*      tab      = nullptr;
    const tokens*     toks     = nullptr;
    std::vector<int>  states;
    std::vector<long> values;
    std::size_t       pos      = 0;
    int               act_kind = 0;
    long              act_arg  = 0;
    long              value    = 0;
};

// self
//   function: this family's state, as the driver installed it.
state&
self(
    machine& _machine
)
{
    return *static_cast<state*>(_machine.ext);
}

// halt_error
//   function: stops the run as failed, recording why.
void
halt_error(
    machine&    _machine,
    std::string _why
)
{
    _machine.ok     = false;
    _machine.error  = std::move(_why);
    _machine.halted = true;

    return;
}

// op_shift
//   operator: pushes the action's state and the lookahead's value, and
// advances past the lookahead.
void
op_shift(
    machine& _machine
)
{
    state& s = self(_machine);

    // shifting the end-of-input token would advance past the input
    if (s.pos >= s.toks->size())
    {
        halt_error(_machine, "shift past the end of the input");

        return;
    }

    s.states.push_back(static_cast<int>(s.act_arg));
    s.values.push_back((*s.toks)[s.pos].second);
    s.pos++;

    return;
}

// op_reduce
//   operator: pops a production's right-hand side, pushes the value its
// semantic action computes, and follows the goto for its left-hand side.
void
op_reduce(
    machine& _machine
)
{
    state& s = self(_machine);

    // the action must name a production the table has
    if ( (s.act_arg < 0) ||
         (static_cast<std::size_t>(s.act_arg) >= s.tab->productions.size()) )
    {
        halt_error(_machine, "reduce by a production the table does not have");

        return;
    }

    const std::pair<int, int> production =
        s.tab->productions[static_cast<std::size_t>(s.act_arg)];
    const std::size_t length = static_cast<std::size_t>(production.second);

    // the stacks must hold the right-hand side, plus the state beneath it
    if ( (length >= s.states.size()) ||
         (length > s.values.size())  )
    {
        halt_error(_machine, "reduce deeper than the stack");

        return;
    }

    const std::vector<long> popped(s.values.end() - static_cast<long>(length),
                                   s.values.end());

    s.values.resize(s.values.size() - length);
    s.states.resize(s.states.size() - length);

    // a table with no semantic actions is a recognizer; its value is 0
    s.values.push_back(s.tab->semantic
                       ? s.tab->semantic(static_cast<int>(s.act_arg), popped)
                       : 0L);

    const auto next = s.tab->gotos.find({ s.states.back(), production.first });

    if (next == s.tab->gotos.end())
    {
        halt_error(_machine, "no goto for the reduced nonterminal");

        return;
    }

    s.states.push_back(next->second);

    return;
}

// op_accept
//   operator: halts the run as successful, with the value on top of the stack.
void
op_accept(
    machine& _machine
)
{
    state& s = self(_machine);

    _machine.ok     = true;
    s.value         = s.values.empty() ? 0L : s.values.back();
    _machine.halted = true;

    return;
}

// op_error
//   operator: halts the run as failed. The table routes every (state, token)
// pair it has no action for here.
void
op_error(
    machine& _machine
)
{
    halt_error(_machine, "parse error");

    return;
}

// describe
//   function: one trace line for an action about to run.
std::string
describe(
    const table&  _table,
    const op_set& _ops,
    int           _state,
    int           _look,
    const action& _action
)
{
    const std::string look = _table.token_name
                             ? _table.token_name(_look)
                             : std::to_string(_look);

    std::string line = "  state " + std::to_string(_state) +
                       "  look=" + look + "  -> " + _ops.name(_action.kind);

    if (_action.kind == SHIFT)
    {
        line += " s" + std::to_string(_action.arg);
    }
    else if (_action.kind == REDUCE)
    {
        line += " (" + (_table.production_name
                        ? _table.production_name(static_cast<int>(_action.arg))
                        : std::to_string(_action.arg)) + ")";
    }

    return line;
}

}  // namespace


/*
make_ops
  Builds this family's operator registry.

Return:
  The registry, with SHIFT, REDUCE, ACCEPT and ERROR defined.
*/
op_set
make_ops()
{
    op_set ops;

    ops.def(SHIFT,  "SHIFT",  &op_shift);
    ops.def(REDUCE, "REDUCE", &op_reduce);
    ops.def(ACCEPT, "ACCEPT", &op_accept);
    ops.def(ERROR,  "ERROR",  &op_error);

    return ops;
}


/*
run
  Drives the LR family over a token stream: looks up the action for the current
state and lookahead, and dispatches it through the registry until an operator
halts the run.
NOTE:
  The driver never branches on an opcode. It resolves an action from the table
and hands it to whatever the registry holds, so a registry missing an operator
is reported rather than dereferenced.

Parameter(s):
  _machine: the run state; its step_limit and trace are honoured.
  _table:   the parse table.
  _tokens:  the input, ending with the end-of-input token.
  _ops:     this family's registry, normally from make_ops().
Return:
  Whether the input was accepted, the value it reduced to, the reason for a
failure, and the number of steps taken.
*/
result
run(
    machine&      _machine,
    const table&  _table,
    const tokens& _tokens,
    const op_set& _ops
)
{
    state s;

    s.tab  = &_table;
    s.toks = &_tokens;
    s.states.assign(1, 0);

    _machine.ext    = &s;
    _machine.steps  = 0;
    _machine.halted = false;
    _machine.ok     = false;
    _machine.error.clear();

    while (!_machine.halted)
    {
        // a runaway table is stopped, not left to spin
        if (_machine.steps >= _machine.step_limit)
        {
            halt_error(_machine, "step limit");

            break;
        }

        // an input that does not end with the accepted end token runs out here
        if (s.pos >= _tokens.size())
        {
            halt_error(_machine, "ran past the end of the input");

            break;
        }

        const int  current = s.states.back();
        const int  look    = _tokens[s.pos].first;
        const auto found   = _table.actions.find({ current, look });

        const action act = (found == _table.actions.end())
                           ? action{ ERROR, 0 }
                           : found->second;

        if (_machine.trace)
        {
            _machine.trace->push_back(describe(_table,
                                               _ops,
                                               current,
                                               look,
                                               act));
        }

        s.act_kind = act.kind;
        s.act_arg  = act.arg;

        const voperator* handler = _ops.find(act.kind);

        // an action with no registered operator is a hard error, not a no-op
        if (!handler)
        {
            halt_error(_machine,
                       "no operator registered for action " +
                       std::to_string(act.kind));

            break;
        }

        (*handler)(_machine);

        if (_machine.halted)
        {
            break;
        }

        _machine.steps++;
    }

    _machine.ext = nullptr;

    return result{ _machine.ok, s.value, _machine.error, _machine.steps };
}

}  // namespace lr

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
