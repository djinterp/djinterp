/*******************************************************************************
* djinterp [parsegen]                                                    peg.cpp
*
* Definitions for `peg.hpp`.
*
*
* path:      /src/djinterp/parsegen/vparse/peg.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.02
*******************************************************************************/
// djinterp
#include "djinterp/parsegen/vparse/peg.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>
#include <string>

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace peg {

namespace {

// state
//   struct: this family's per-run state, reached through machine::ext.  The
// cursor (data / len / off) is seeded from the parse_state; the rest is the
// PEG machinery -- call/return stack, backtrack stack, and the mark/capture
// stacks.  TU-local.
struct state
{
    // bt
    //   struct: a backtrack point -- resume ip, cursor, and the call / mark /
    // capture depths to unwind to.
    struct bt
    {
        int         ip;
        int         off;
        std::size_t cdepth;
        std::size_t mdepth;
        std::size_t capdepth;
    };

    const program*       prog = nullptr;
    const char*          data = nullptr;   // the input (seeded from parse_state)
    int                  len  = 0;
    int                  off  = 0;         // the cursor
    int                  ip   = 0;
    std::vector<int>     calls;
    std::vector<bt>      back;
    std::vector<int>     marks;
    std::vector<capture> caps;
    const instr*         cur  = nullptr;
};

inline state& self(machine& _m) { return *static_cast<state*>(_m.ext); }

// fail
//   the PEG failure convention -- backtrack (restoring cursor, call depth, and
// the mark/capture stacks), or halt unsuccessfully.
inline void
fail(machine& _m)
{
    state& s = self(_m);

    if (s.back.empty())
    {
        _m.ok     = false;
        _m.halted = true;
        return;
    }

    state::bt b = s.back.back();
    s.back.pop_back();

    s.ip  = b.ip;
    s.off = b.off;
    s.calls.resize(b.cdepth);
    s.marks.resize(b.mdepth);
    s.caps.resize(b.capdepth);
    return;
}

// op_char
//   voperator: match the current literal char; advance or fail.
void op_char(machine& _m)
{
    state& s = self(_m);
    if ( (s.off < s.len) && (s.data[s.off] == s.cur->ch) )
    {
        s.off += 1; s.ip += 1; return;
    }
    fail(_m); return;
}

// op_any
//   voperator: consume any one char; fail only at end of input.
void op_any(machine& _m)
{
    state& s = self(_m);
    if (s.off < s.len) { s.off += 1; s.ip += 1; return; }
    fail(_m); return;
}

// op_choice
//   voperator: push a backtrack point (cursor + depths) and fall through.
void op_choice(machine& _m)
{
    state& s = self(_m);
    s.back.push_back(state::bt{ s.cur->arg, s.off, s.calls.size(),
                                s.marks.size(), s.caps.size() });
    s.ip += 1; return;
}

// op_jump
//   voperator: unconditional transfer of control.
void op_jump(machine& _m) { state& s = self(_m); s.ip = s.cur->arg; return; }

// op_call
//   voperator: push the return address and enter a sub-rule.
void op_call(machine& _m)
{
    state& s = self(_m);
    s.calls.push_back(s.ip + 1); s.ip = s.cur->arg; return;
}

// op_return
//   voperator: pop the return address and resume the caller.
void op_return(machine& _m)
{
    state& s = self(_m);
    s.ip = s.calls.back(); s.calls.pop_back(); return;
}

// op_commit
//   voperator: discard the most recent backtrack point and jump.
void op_commit(machine& _m)
{
    state& s = self(_m);
    s.back.pop_back(); s.ip = s.cur->arg; return;
}

// op_fail
//   voperator: force the failure convention.
void op_fail(machine& _m) { fail(_m); return; }

// op_match
//   voperator: accept -- halt the machine successfully.
void op_match(machine& _m) { _m.ok = true; _m.halted = true; return; }

// op_set_class
//   voperator: match the current char against the class set; advance or fail.
void op_set_class(machine& _m)
{
    state& s = self(_m);
    if ( (s.off < s.len) &&
         (s.cur->set.find(s.data[s.off]) != std::string::npos) )
    {
        s.off += 1; s.ip += 1; return;
    }
    fail(_m); return;
}

// op_mark
//   voperator: record the cursor as the start of a capture.
void op_mark(machine& _m)
{
    state& s = self(_m); s.marks.push_back(s.off); s.ip += 1; return;
}

// op_cap
//   voperator: pop the mark and record a tagged span [mark, cursor).
void op_cap(machine& _m)
{
    state& s = self(_m);
    int start = s.marks.back(); s.marks.pop_back();
    s.caps.push_back(capture{ s.cur->arg, start, s.off });
    s.ip += 1; return;
}

// op_name
//   display mnemonic for an opcode.
const char* op_name(int _op)
{
    switch (_op)
    {
        case CHAR:   return "CHAR";   case ANY:    return "ANY";
        case CHOICE: return "CHOICE"; case JUMP:   return "JUMP";
        case CALL:   return "CALL";   case RETURN: return "RETURN";
        case COMMIT: return "COMMIT"; case FAIL:   return "FAIL";
        case MATCH:  return "MATCH";  case SET:    return "SET";
        case MARK:   return "MARK";   case CAP:    return "CAP";
    }
    return "?";
}

// has_addr
//   whether an opcode carries a jump/address operand.
bool has_addr(int _op)
{
    return ( (_op == CHOICE) || (_op == JUMP) ||
             (_op == CALL)   || (_op == COMMIT) );
}

// describe
//   one trace line for the instruction about to run (format in peg.hpp).
std::string
describe(const state& _s, const instr& _ins)
{
    return "ip="     + std::to_string(_s.ip)           +
           " off="   + std::to_string(_s.off)          +
           " calls=" + std::to_string(_s.calls.size()) +
           " back="  + std::to_string(_s.back.size())  +
           " marks=" + std::to_string(_s.marks.size()) +
           " caps="  + std::to_string(_s.caps.size())  +
           " | "     + fmt(_ins);
}

}  // anonymous namespace

// fmt
//   function: disassemble one instruction.
std::string
fmt(const instr& _ins)
{
    std::string s = op_name(_ins.op);
    if (_ins.op == CHAR)      { s += " '"; s += _ins.ch; s += "'"; }
    else if (_ins.op == SET)  { s += " [" + _ins.set + "]"; }
    else if (_ins.op == CAP)  { s += " #" + std::to_string(_ins.arg); }
    else if (has_addr(_ins.op)) { s += " " + std::to_string(_ins.arg); }
    return s;
}

// make_ops
//   function: build the PEG operator registry.
op_set
make_ops()
{
    op_set ops;
    ops.def(CHAR,   "CHAR",   &op_char);   ops.def(ANY,    "ANY",    &op_any);
    ops.def(CHOICE, "CHOICE", &op_choice); ops.def(JUMP,   "JUMP",   &op_jump);
    ops.def(CALL,   "CALL",   &op_call);   ops.def(RETURN, "RETURN", &op_return);
    ops.def(COMMIT, "COMMIT", &op_commit); ops.def(FAIL,   "FAIL",   &op_fail);
    ops.def(MATCH,  "MATCH",  &op_match);  ops.def(SET,    "SET",    &op_set_class);
    ops.def(MARK,   "MARK",   &op_mark);   ops.def(CAP,    "CAP",    &op_cap);
    return ops;
}

// run
//   function: the rebased driver.  Parses a parse_state<char> from its offset;
// on success advances offset to the match end and hands back captures, on
// failure leaves offset unchanged (match-or-restore).  Returns success.
bool
run(parse_state<char>&    _state,
    const program&        _prog,
    const op_set&         _ops,
    std::vector<capture>* _caps)
{
    machine m;

    return run(m, _state, _prog, _ops, _caps);
}

// run (caller's machine)
//   function: the driver loop.  The machine is the caller's, so its step
// limit and trace sink apply; the family state is local and is detached from
// the machine again before returning.
bool
run(machine&              _machine,
    parse_state<char>&    _state,
    const program&        _prog,
    const op_set&         _ops,
    std::vector<capture>* _caps)
{
    state s;

    s.prog = &_prog;
    s.data = _state.data;
    s.len  = static_cast<int>(_state.length);
    s.off  = static_cast<int>(_state.offset);

    _machine.ext    = &s;
    _machine.steps  = 0;
    _machine.halted = false;
    _machine.ok     = false;
    _machine.error.clear();

    while (true)
    {
        if (_machine.steps >= _machine.step_limit)
        {
            _machine.ok    = false;
            _machine.error = "step limit";
            break;
        }

        // an ip with no instruction -- the first of an empty program, which
        // ebnf::compile returns for a grammar it cannot parse -- is an
        // ordinary failure, not a read past the end of the program
        if ( (s.ip < 0) ||
             (static_cast<std::size_t>(s.ip) >= _prog.size()) )
        {
            _machine.ok    = false;
            _machine.error = _prog.empty() ? "empty program" : "ip out of range";
            break;
        }

        const instr& ins = _prog[s.ip];
        s.cur = &ins;

        if (_machine.trace != nullptr)
        {
            _machine.trace->push_back(describe(s, ins));
        }

        const voperator* h = _ops.find(ins.op);
        if (!h)
        {
            _machine.halted = true;
            _machine.error  = "no operator for opcode " +
                              std::to_string(ins.op);
            break;
        }

        (*h)(_machine);
        if (_machine.halted) { break; }
        _machine.steps += 1;
    }

    _machine.ext = nullptr;

    // match-or-restore: advance + publish captures on success, else no change
    if (_machine.ok)
    {
        _state.offset = static_cast<std::size_t>(s.off);
        if (_caps != nullptr) { *_caps = s.caps; }
    }

    return _machine.ok;
}

}  // namespace peg

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
