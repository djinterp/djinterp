/*******************************************************************************
* djinterp [parse]                                                       peg.cpp
*
*
* path:      /src/djinterp/parse/vparse/peg.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/parse/vparse/peg.hpp"

// std
#include <cstdio>

namespace peg {

// ---- family-private machine state (attached to Machine::ext by run) ----
namespace {
struct State {
    const Program* prog = nullptr;
    int ip = 0;
    std::vector<int> calls;                 // return addresses
    struct BT { int ip, sp; size_t cdepth; };
    std::vector<BT> back;                    // backtrack points
    const Instr* cur = nullptr;              // current instruction
};
inline State& self(Machine& M) { return *static_cast<State*>(M.ext); }

inline void fail(Machine& M) {              // PEG failure / backtrack convention
    State& P = self(M);
    if (P.back.empty()) { M.ok = false; M.halted = true; return; }
    auto b = P.back.back(); P.back.pop_back();
    P.ip = b.ip; M.sp = b.sp; P.calls.resize(b.cdepth);
}

const char* opName(int op) {
    switch (op) {
        case CHAR: return "CHAR"; case ANY: return "ANY"; case CHOICE: return "CHOICE";
        case JUMP: return "JUMP"; case CALL: return "CALL"; case RETURN: return "RETURN";
        case COMMIT: return "COMMIT"; case FAIL: return "FAIL"; case MATCH: return "MATCH";
        case SET: return "SET";
    }
    return "?";
}
inline bool hasAddr(int op) { return op == CHOICE || op == JUMP || op == CALL || op == COMMIT; }
} // anonymous namespace

std::string fmt(const Instr& ins) {
    std::string s = opName(ins.op);
    if (ins.op == CHAR)       { s += " '"; s += ins.ch; s += "'"; }
    else if (ins.op == SET)   { s += " [" + ins.set + "]"; }
    else if (hasAddr(ins.op))  s += " " + std::to_string(ins.arg);
    return s;
}

OpSet make_ops() {
    OpSet S;
    S.def(CHAR, "CHAR", [](Machine& M){
        State& P = self(M);
        if (M.sp < (int)M.input->size() && (*M.input)[M.sp] == P.cur->ch) { M.sp++; P.ip++; }
        else fail(M);
    });
    S.def(ANY, "ANY", [](Machine& M){
        State& P = self(M);
        if (M.sp < (int)M.input->size()) { M.sp++; P.ip++; } else fail(M);
    });
    S.def(CHOICE, "CHOICE", [](Machine& M){ State& P = self(M); P.back.push_back({P.cur->arg, M.sp, P.calls.size()}); P.ip++; });
    S.def(JUMP,   "JUMP",   [](Machine& M){ State& P = self(M); P.ip = P.cur->arg; });
    S.def(CALL,   "CALL",   [](Machine& M){ State& P = self(M); P.calls.push_back(P.ip + 1); P.ip = P.cur->arg; });
    S.def(RETURN, "RETURN", [](Machine& M){ State& P = self(M); P.ip = P.calls.back(); P.calls.pop_back(); });
    S.def(COMMIT, "COMMIT", [](Machine& M){ State& P = self(M); P.back.pop_back(); P.ip = P.cur->arg; });
    S.def(FAIL,   "FAIL",   [](Machine& M){ fail(M); });
    S.def(MATCH,  "MATCH",  [](Machine& M){ M.ok = true; M.halted = true; });
    S.def(SET,    "SET",    [](Machine& M){
        State& P = self(M);
        if (M.sp < (int)M.input->size() && P.cur->set.find((*M.input)[M.sp]) != std::string::npos) { M.sp++; P.ip++; }
        else fail(M);
    });
    return S;
}

void run(Machine& M, const Program& prog, const OpSet& ops) {
    State st; st.prog = &prog;
    M.ext = &st;
    M.sp = 0; M.steps = 0; M.halted = false; M.ok = false; M.error.clear();
    while (true) {
        if (M.steps >= M.stepLimit) { M.ok = false; M.error = "step limit hit (left recursion?)"; break; }
        const Instr& ins = prog[st.ip];
        if (M.trace) {
            char buf[80];
            std::snprintf(buf, sizeof buf, "  step %2ld  ip=%3d  sp=%d  ", M.steps, st.ip, M.sp);
            M.trace->push_back(std::string(buf) + fmt(ins));
        }
        st.cur = &ins;
        const VOp* h = ops.find(ins.op);
        if (!h) { M.error = "unknown opcode"; M.halted = true; break; }
        (*h)(M);
        if (M.halted) break;            // halting instruction not counted (parity)
        M.steps++;
    }
    M.ext = nullptr;
}

} // namespace peg
