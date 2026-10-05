/*******************************************************************************
* djinterp [parsegen]                                                    gen.cpp
*
* Definitions for `gen.hpp`.
*
*
* path:      /src/djinterp/parsegen/vparse/gen.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.01
*******************************************************************************/
// djinterp
#include "djinterp/parsegen/vparse/gen.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>
#include <unordered_map>

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace gen {

namespace {

// state
//   struct: the generator's per-run state.  Output program, rule-address
// labels, pending reference fixups, the open-star-loop stack, per-rule
// alternative bookkeeping (open CHOICEs + exit COMMITs), and start bookkeeping.
struct state
{
    struct fixup { int addr; std::string name; };

    peg::program                          out;
    std::unordered_map<std::string, int>  labels;
    std::vector<fixup>                    fixups;
    std::vector<int>                      open;
    std::vector<int>                      alt_choice;
    std::vector<int>                      alt_exits;
    std::string                           start;
    int                                   start_call = -1;
    const gen_instr*                      cur = nullptr;
};

inline state& self(machine& _m) { return *static_cast<state*>(_m.ext); }

inline int emit(state& _s, const peg::instr& _ins)
{
    int addr = static_cast<int>(_s.out.size());
    _s.out.push_back(_ins);
    return addr;
}

inline peg::instr
make_instr(int _op, int _arg = -1, char _ch = 0,
           const std::string& _set = std::string())
{
    peg::instr ins; ins.op = _op; ins.arg = _arg; ins.ch = _ch; ins.set = _set;
    return ins;
}

// op_preamble
//   voperator: CALL(start, placeholder) + the !. end-of-input test + MATCH.
void op_preamble(machine& _m)
{
    state& s = self(_m);
    s.start      = s.cur->text;
    s.start_call = emit(s, make_instr(peg::CALL, -1));
    emit(s, make_instr(peg::CHOICE, 5));
    emit(s, make_instr(peg::ANY));
    emit(s, make_instr(peg::COMMIT, 4));
    emit(s, make_instr(peg::FAIL));
    emit(s, make_instr(peg::MATCH));
    return;
}

// op_rule_begin
//   voperator: record the rule start; reset the alternative-exit list.
void op_rule_begin(machine& _m)
{
    state& s = self(_m);
    s.labels[s.cur->text] = static_cast<int>(s.out.size());
    s.alt_exits.clear();
    return;
}

// op_rule_end
//   voperator: resolve alternative exits to here; emit RETURN.
void op_rule_end(machine& _m)
{
    state& s = self(_m);
    int end = static_cast<int>(s.out.size());
    for (int a : s.alt_exits) { s.out[a].arg = end; }
    s.alt_exits.clear();
    emit(s, make_instr(peg::RETURN));
    return;
}

// op_alt_choice
//   voperator: open an alternative -- emit CHOICE(placeholder).
void op_alt_choice(machine& _m)
{
    state& s = self(_m);
    s.alt_choice.push_back(emit(s, make_instr(peg::CHOICE, -1)));
    return;
}

// op_alt_commit
//   voperator: close a non-last alternative -- COMMIT to the rule exit
// (recorded), and patch the opening CHOICE to the next alternative.
void op_alt_commit(machine& _m)
{
    state& s = self(_m);
    int commit = emit(s, make_instr(peg::COMMIT, -1));
    s.alt_exits.push_back(commit);
    int choice = s.alt_choice.back(); s.alt_choice.pop_back();
    s.out[choice].arg = static_cast<int>(s.out.size());
    return;
}

// op_emit_char / op_emit_set / op_emit_ref
void op_emit_char(machine& _m)
{
    state& s = self(_m); emit(s, make_instr(peg::CHAR, -1, s.cur->ch)); return;
}
void op_emit_set(machine& _m)
{
    state& s = self(_m); emit(s, make_instr(peg::SET, -1, 0, s.cur->text)); return;
}
void op_emit_ref(machine& _m)
{
    state& s = self(_m);
    int addr = emit(s, make_instr(peg::CALL, -1));
    s.fixups.push_back(state::fixup{ addr, s.cur->text });
    return;
}

// op_emit_any -- match any one char
void op_emit_any(machine& _m)
{
    state& s = self(_m); emit(s, make_instr(peg::ANY)); return;
}

// op_emit_mark / op_emit_cap -- captures in the generated parser
void op_emit_mark(machine& _m)
{
    state& s = self(_m); emit(s, make_instr(peg::MARK)); return;
}
void op_emit_cap(machine& _m)
{
    state& s = self(_m); emit(s, make_instr(peg::CAP, s.cur->tag)); return;
}

// op_star_begin / op_star_end -- a zero-or-more loop
void op_star_begin(machine& _m)
{
    state& s = self(_m);
    s.open.push_back(emit(s, make_instr(peg::CHOICE, -1)));
    return;
}
void op_star_end(machine& _m)
{
    state& s = self(_m);
    int choice = s.open.back(); s.open.pop_back();
    emit(s, make_instr(peg::COMMIT, choice));
    s.out[choice].arg = static_cast<int>(s.out.size());
    return;
}

// op_finish
//   voperator: resolve the start CALL and every rule-reference fixup.
void op_finish(machine& _m)
{
    state& s = self(_m);
    if (s.start_call >= 0) { s.out[s.start_call].arg = s.labels[s.start]; }
    for (const state::fixup& f : s.fixups) { s.out[f.addr].arg = s.labels[f.name]; }
    _m.halted = true; _m.ok = true;
    return;
}

}  // anonymous namespace

// make_ops
//   function: build the generator's operator registry.
op_set
make_ops()
{
    op_set ops;
    ops.def(G_PREAMBLE,   "PREAMBLE",   &op_preamble);
    ops.def(G_RULE_BEGIN, "RULE_BEGIN", &op_rule_begin);
    ops.def(G_RULE_END,   "RULE_END",   &op_rule_end);
    ops.def(G_ALT_CHOICE, "ALT_CHOICE", &op_alt_choice);
    ops.def(G_ALT_COMMIT, "ALT_COMMIT", &op_alt_commit);
    ops.def(G_EMIT_CHAR,  "EMIT_CHAR",  &op_emit_char);
    ops.def(G_EMIT_SET,   "EMIT_SET",   &op_emit_set);
    ops.def(G_EMIT_REF,   "EMIT_REF",   &op_emit_ref);
    ops.def(G_EMIT_ANY,   "EMIT_ANY",   &op_emit_any);
    ops.def(G_EMIT_MARK,  "EMIT_MARK",  &op_emit_mark);
    ops.def(G_EMIT_CAP,   "EMIT_CAP",   &op_emit_cap);
    ops.def(G_STAR_BEGIN, "STAR_BEGIN", &op_star_begin);
    ops.def(G_STAR_END,   "STAR_END",   &op_star_end);
    ops.def(G_FINISH,     "FINISH",     &op_finish);
    return ops;
}

// emit_term
//   helper (host): directives for one term -- capture brackets outside an
// optional star loop around the recognizer.
static void
emit_term(gen_program& _p, const term& _t)
{
    if (_t.cap != 0) { gen_instr m; m.op = G_EMIT_MARK; _p.push_back(m); }
    if (_t.star)     { gen_instr sb; sb.op = G_STAR_BEGIN; _p.push_back(sb); }

    gen_instr e;
    if (_t.kind == T_CHAR)      { e.op = G_EMIT_CHAR; e.ch = _t.ch; }
    else if (_t.kind == T_SET)  { e.op = G_EMIT_SET;  e.text = _t.set; }
    else if (_t.kind == T_ANY)  { e.op = G_EMIT_ANY; }
    else                        { e.op = G_EMIT_REF;  e.text = _t.ref; }
    _p.push_back(e);

    if (_t.star)     { gen_instr se; se.op = G_STAR_END; _p.push_back(se); }
    if (_t.cap != 0) { gen_instr c; c.op = G_EMIT_CAP; c.tag = _t.cap; _p.push_back(c); }
    return;
}

// plan
//   function: linearise a ruleset into a generation program.
gen_program
plan(const ruleset& _grammar)
{
    gen_program p;

    if (!_grammar.rules.empty())
    {
        gen_instr pre; pre.op = G_PREAMBLE; pre.text = _grammar.rules.front().name;
        p.push_back(pre);
    }

    for (const rule& r : _grammar.rules)
    {
        gen_instr rb; rb.op = G_RULE_BEGIN; rb.text = r.name; p.push_back(rb);

        for (std::size_t ai = 0; ai < r.alts.size(); ++ai)
        {
            const bool last = (ai + 1 == r.alts.size());
            if (!last) { gen_instr ac; ac.op = G_ALT_CHOICE; p.push_back(ac); }
            for (const term& t : r.alts[ai]) { emit_term(p, t); }
            if (!last) { gen_instr acm; acm.op = G_ALT_COMMIT; p.push_back(acm); }
        }

        gen_instr re; re.op = G_RULE_END; p.push_back(re);
    }

    gen_instr fin; fin.op = G_FINISH; p.push_back(fin);
    return p;
}

// generate
//   function: run the generation program on the machine; return the parser.
peg::program
generate(const gen_program& _plan, const op_set& _ops)
{
    machine m; state st;
    m.ext = &st;
    int pc = 0;

    while ((!m.halted) && (pc < static_cast<int>(_plan.size())))
    {
        const gen_instr& d = _plan[pc];
        st.cur = &d;
        const voperator* h = _ops.find(d.op);
        if (!h) { m.halted = true; break; }
        (*h)(m);
        pc += 1;
    }
    return st.out;
}

// compile
//   function: the whole pipeline in one call.
peg::program
compile(const ruleset& _grammar)
{
    op_set ops = make_ops();
    return generate(plan(_grammar), ops);
}

}  // namespace gen

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
