/*******************************************************************************
* djinterp [parsegen]                                           vparse_tests.hpp
*
* Test header for the vparse module: shared helpers plus declarations of the
* tests_* section functions (defined in the vparse_tests_*.cpp TUs, flat in
* djinterp::testing, and driven by vparse_tests_runner.cpp).
*   Every include is a path relative to this file, so a section unit compiles
* with no include directory beyond the one the ladder gives every unit.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.06
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_VPARSE_TESTS_HPP
#define DJINTERP_PARSEGEN_VPARSE_VPARSE_TESTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the vparse headers it tests are. The owner's ruling: compile at every level
// first; port to C++98 only where something needs it.
#include "../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../inc/djinterp/parsegen/vparse/adapter.hpp"   // as_parser
#include "../../../../inc/djinterp/parsegen/vparse/ebnf.hpp"      // ebnf::parse, compile
#include "../../../../inc/djinterp/parsegen/vparse/gen.hpp"       // gen::compile
#include "../../../../inc/djinterp/parsegen/vparse/lr.hpp"        // the LR family
#include "../../../../inc/djinterp/parsegen/vparse/notation.hpp"  // parse_grammar
#include "../../../../inc/djinterp/parsegen/vparse/peg.hpp"       // the PEG family
#include "../../../../inc/djinterp/parsegen/vparse/ruleset.hpp"   // ruleset, rule, term

namespace djinterp
{
namespace testing
{

namespace pr = ::djinterp::parse;
namespace vp = ::djinterp::parsegen::vparse;

// peg_ops
//   helper: the PEG operator registry (built once).
inline const vp::op_set&
peg_ops()
{
    static vp::op_set ops = vp::peg::make_ops();
    return ops;
}

// ins
//   helper: one instruction.  peg::instr has default member initializers, so
// at C++11 it is not an aggregate and `instr{ ... }` does not compile there;
// assigning the fields is valid at every level, and is how gen.cpp builds one.
inline vp::peg::instr
ins(
    int                _op,
    int                _arg = -1,
    char               _ch  = 0,
    const std::string& _set = std::string()
)
{
    vp::peg::instr i;

    i.op  = _op;
    i.arg = _arg;
    i.ch  = _ch;
    i.set = _set;

    return i;
}

// digits_program
//   helper: the hand-written `S = [0-9]+` parser (with the end-of-input gate),
// the oracle the generator must reproduce byte-for-byte.
inline vp::peg::program
digits_program()
{
    using namespace vp::peg;
    const std::string D = "0123456789";
    program p;
    p.push_back(ins(CALL,    6));        p.push_back(ins(CHOICE,  5));
    p.push_back(ins(ANY));               p.push_back(ins(COMMIT,  4));
    p.push_back(ins(FAIL));              p.push_back(ins(MATCH));
    p.push_back(ins(SET,    -1, 0, D));  p.push_back(ins(CHOICE, 10));
    p.push_back(ins(SET,    -1, 0, D));  p.push_back(ins(COMMIT,  7));
    p.push_back(ins(RETURN));
    return p;
}

// run_matcher
//   helper: a prefix matcher for one-or-more of a class -- MATCHes WITHOUT an
// end-of-input gate, so it consumes its run and stops, leaving the rest for a
// following parser.  Used to exercise composition.
inline vp::peg::program
run_matcher(const std::string& _set)
{
    using namespace vp::peg;
    program p;
    p.push_back(ins(SET,   -1, 0, _set)); p.push_back(ins(CHOICE, 4));
    p.push_back(ins(SET,   -1, 0, _set)); p.push_back(ins(COMMIT, 1));
    p.push_back(ins(MATCH));
    return p;
}

// same
//   helper: instruction-level program equality.
inline bool
same(const vp::peg::program& _a, const vp::peg::program& _b)
{
    if (_a.size() != _b.size()) { return false; }
    for (std::size_t i = 0; i < _a.size(); ++i)
    {
        if ( (_a[i].op  != _b[i].op)  || (_a[i].arg != _b[i].arg) ||
             (_a[i].ch  != _b[i].ch)  || (_a[i].set != _b[i].set) )
        {
            return false;
        }
    }
    return true;
}

// accept
//   helper: does the (end-of-input-gated) program accept the whole string?
inline bool
accept(const vp::peg::program& _prog, const std::string& _s)
{
    pr::parse_state<char> st(_s.data(), _s.size(), 0);
    return vp::peg::run(st, _prog, peg_ops());
}

// ---- section functions (defined in the vparse_tests_*.cpp TUs) -------------

bool tests_captures_backtrack();
bool tests_captures_generated();
bool tests_captures_trace();
bool tests_generate_anchor();
bool tests_generate_multirule();
bool tests_notation_anchor();
bool tests_notation_alternation();
bool tests_notation_plus_literal();
bool tests_rebase_handle();
bool tests_rebase_compose();
bool tests_lr_left_recursive();
bool tests_lr_trace_and_limits();
bool tests_ebnf_arithmetic();
bool tests_ebnf_grouping();
bool tests_ebnf_errors();

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_VPARSE_TESTS_HPP
