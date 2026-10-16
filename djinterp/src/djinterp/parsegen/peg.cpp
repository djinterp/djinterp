/*******************************************************************************
* djinterp [parsegen]                                                    peg.cpp
*
*
* path:      /src/djinterp/parsegen/peg.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../inc/djinterp/parse/vparse/peg.hpp"

// std
#include <cstddef>
#include <cstdio>


NS_DJINTERP
NS_PARSEGEN


namespace {

// state
//   struct: this family's private per-run state, reached through machine::ext.
// Holds the program counter, the call/return stack, and the backtrack stack.
// TU-local: the substrate never names it.
struct state
{
    // bt
    //   struct: one backtrack point -- where to resume, the cursor to restore,
    // and the call-stack depth to unwind to.
    struct bt
    {
        int         ip;
        int         sp;
        std::size_t cdepth;
    };

    const program*   prog = nullptr;
    int              ip   = 0;
    std::vector<int> calls;
    std::vector<bt>  back;
    const instr*     cur  = nullptr;
};

// self
//   accessor: recover this family's private state from the machine.
inline state&
self(
    machine& _machine
)
{
    return *static_cast<state*>(_machine.ext);
}

// fail
//   function: the PEG failure convention -- backtrack to the most recent
// choice point, or halt unsuccessfully if there is none.
inline void
fail(
    machine& _machine
)
{
    state& s = self(_machine);

    // no choice point left: the parse fails
    if (s.back.empty())
    {
        _machine.ok     = false;
        _machine.halted = true;

        return;
    }

    state::bt b = s.back.back();
    s.back.pop_back();

    s.ip  = b.ip;
    _machine.sp = b.sp;
    s.calls.resize(b.cdepth);

    return;
}

// op_char
//   voperator: match the current instruction's literal char; advance or fail.
void
op_char(
    machine& _machine
)
{
    state& s = self(_machine);

    if ( (_machine.sp < static_cast<int>(_machine.input->size())) &&
         ((*_machine.input)[_machine.sp] == s.cur->ch) )
    {
        _machine.sp += 1;
        s.ip  += 1;

        return;
    }

    fail(_machine);

    return;
}

// op_any
//   voperator: consume any one char; fail only at end of input.
void
op_any(
    machine& _machine
)
{
    state& s = self(_machine);

    if (_machine.sp < static_cast<int>(_machine.input->size()))
    {
        _machine.sp += 1;
        s.ip  += 1;

        return;
    }

    fail(_machine);

    return;
}

// op_choice
//   voperator: push a backtrack point (the ordered-choice alternative) and
// fall through to the first alternative.
void
op_choice(
    machine& _machine
)
{
    state& s = self(_machine);

    s.back.push_back(state::bt{ s.cur->arg, _machine.sp, s.calls.size() });
    s.ip += 1;

    return;
}

// op_jump
//   voperator: unconditional transfer of control.
void
op_jump(
    machine& _machine
)
{
    state& s = self(_machine);

    s.ip = s.cur->arg;

    return;
}

// op_call
//   voperator: push the return address and enter a sub-rule.
void
op_call(
    machine& _machine
)
{
    state& s = self(_machine);

    s.calls.push_back(s.ip + 1);
    s.ip = s.cur->arg;

    return;
}

// op_return
//   voperator: pop the return address and resume the caller.
void
op_return(
    machine& _machine
)
{
    state& s = self(_machine);

    s.ip = s.calls.back();
    s.calls.pop_back();

    return;
}

// op_commit
//   voperator: discard the most recent backtrack point and jump (the
// alternative succeeded, so its fallback is no longer needed).
void
op_commit(
    machine& _machine
)
{
    state& s = self(_machine);

    s.back.pop_back();
    s.ip = s.cur->arg;

    return;
}

// op_fail
//   voperator: force the failure convention.
void
op_fail(
    machine& _machine
)
{
    fail(_machine);

    return;
}

// op_match
//   voperator: accept -- halt the machine successfully.
void
op_match(
    machine& _machine
)
{
    _machine.ok     = true;
    _machine.halted = true;

    return;
}

// op_set_class
//   voperator: match the current char against the instruction's class set;
// advance or fail.  (Named op_set_class, not op_set -- op_set is the registry
// type.)
void
op_set_class(
    machine& _machine
)
{
    state& s = self(_machine);

    if ( (_machine.sp < static_cast<int>(_machine.input->size())) &&
         (s.cur->set.find((*_machine.input)[_machine.sp]) != std::string::npos) )
    {
        _machine.sp += 1;
        s.ip  += 1;

        return;
    }

    fail(_machine);

    return;
}

// op_name
//   function: display mnemonic for an opcode (disassembly only).
const char*
op_name(
    int _op
)
{
    switch (_op)
    {
        case CHAR:   return "CHAR";
        case ANY:    return "ANY";
        case CHOICE: return "CHOICE";
        case JUMP:   return "JUMP";
        case CALL:   return "CALL";
        case RETURN: return "RETURN";
        case COMMIT: return "COMMIT";
        case FAIL:   return "FAIL";
        case MATCH:  return "MATCH";
        case SET:    return "SET";
    }

    return "?";
}

// has_addr
//   function: whether an opcode carries a jump/address operand.
bool
has_addr(
    int _op
)
{
    return ( (_op == CHOICE) ||
             (_op == JUMP)   ||
             (_op == CALL)   ||
             (_op == COMMIT) );
}

}  // anonymous namespace


// fmt
//   function: disassemble one instruction to a human-readable string.
std::string
fmt(
    const instr& _ins
)
{
    std::string s = op_name(_ins.op);

    // append the operand relevant to this opcode, if any
    if (_ins.op == CHAR)
    {
        s += " '";
        s += _ins.ch;
        s += "'";
    }
    else if (_ins.op == SET)
    {
        s += " [" + _ins.set + "]";
    }
    else if (has_addr(_ins.op))
    {
        s += " " + std::to_string(_ins.arg);
    }

    return s;
}

// make_ops
//   function: build the PEG family's operator registry, binding each opcode to
// its named voperator.
op_set
make_ops()
{
    op_set ops;

    ops.def(CHAR,   "CHAR",   &op_char);
    ops.def(ANY,    "ANY",    &op_any);
    ops.def(CHOICE, "CHOICE", &op_choice);
    ops.def(JUMP,   "JUMP",   &op_jump);
    ops.def(CALL,   "CALL",   &op_call);
    ops.def(RETURN, "RETURN", &op_return);
    ops.def(COMMIT, "COMMIT", &op_commit);
    ops.def(FAIL,   "FAIL",   &op_fail);
    ops.def(MATCH,  "MATCH",  &op_match);
    ops.def(SET,    "SET",    &op_set_class);

    return ops;
}

// run
//   function: the program-counter-driven driver.  Fetches program[ip],
// dispatches the matching voperator, and counts a step, until the machine
// halts (the halting instruction is not counted, for parity with the prior
// versions).
void
run(
    machine&       _machine,
    const program& _prog,
    const op_set&  _ops
)
{
    state st;

    st.prog   = &_prog;
    _machine.ext    = &st;
    _machine.sp     = 0;
    _machine.steps  = 0;
    _machine.halted = false;
    _machine.ok     = false;
    _machine.error.clear();

    // dispatch loop
    while (true)
    {
        // guard against runaway (e.g. left recursion in a hand-built program)
        if (_machine.steps >= _machine.step_limit)
        {
            _machine.ok    = false;
            _machine.error = "step limit hit (left recursion?)";

            break;
        }

        const instr& ins = _prog[st.ip];

        // optional trace of the instruction about to run
        if (_machine.trace)
        {
            char buf[80];
            std::snprintf(buf, sizeof buf,
                          "  step %2ld  ip=%3d  sp=%d  ",
                          _machine.steps, st.ip, _machine.sp);
            _machine.trace->push_back(std::string(buf) + fmt(ins));
        }

        st.cur = &ins;

        const voperator* h = _ops.find(ins.op);

        // unknown opcode: stop rather than misbehave
        if (!h)
        {
            _machine.error  = "unknown opcode";
            _machine.halted = true;

            break;
        }

        (*h)(_machine);

        // the halting instruction (MATCH / terminal FAIL) is not counted
        if (_machine.halted)
        {
            break;
        }

        _machine.steps += 1;
    }

    _machine.ext = nullptr;

    return;
}


NS_END  // vparse
NS_END  // djinterp
