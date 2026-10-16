/*******************************************************************************
* djinterp [vparse]                                             vparse_tests.hpp
*
*   Test header for the vparse module: shared helpers plus declarations of the
* tests_* section functions (defined in the vparse_tests_*.cpp TUs, flat in
* djinterp::testing, and driven by vparse_tests_runner.cpp).
*
*
* path:      /tests/djinterp/parse/vparse/vparse_tests.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_PARSE_VPARSE_TESTS_
#define DJINTERP_PARSE_VPARSE_TESTS_ 1

// std
#include <string>
#include <vector>
// djinterp
#include <djinterp/parse/vparse/adapter.hpp>
#include <djinterp/parse/vparse/gen.hpp>
#include <djinterp/parse/vparse/notation.hpp>
#include <djinterp/parse/vparse/peg.hpp>
#include <djinterp/parse/vparse/ruleset.hpp>

namespace djinterp
{
namespace testing
{

namespace pr = ::djinterp::parse;

// peg_ops
//   helper: the PEG operator registry (built once).
inline const op_set&
peg_ops()
{
    static op_set ops = make_ops();
    return ops;
}

// digits_program
//   helper: the hand-written `S = [0-9]+` parser (with the end-of-input gate),
// the oracle the generator must reproduce byte-for-byte.
inline program
digits_program()
{
    using namespace peg;
    const std::string D = "0123456789";
    program p;
    p.push_back(instr{ CALL,    6,  0, ""  }); p.push_back(instr{ CHOICE,  5, 0, "" });
    p.push_back(instr{ ANY,    -1,  0, ""  }); p.push_back(instr{ COMMIT,  4, 0, "" });
    p.push_back(instr{ FAIL,   -1,  0, ""  }); p.push_back(instr{ MATCH,  -1, 0, "" });
    p.push_back(instr{ SET,    -1,  0, D   }); p.push_back(instr{ CHOICE, 10, 0, "" });
    p.push_back(instr{ SET,    -1,  0, D   }); p.push_back(instr{ COMMIT,  7, 0, "" });
    p.push_back(instr{ RETURN, -1,  0, ""  });
    return p;
}

// run_matcher
//   helper: a prefix matcher for one-or-more of a class -- MATCHes WITHOUT an
// end-of-input gate, so it consumes its run and stops, leaving the rest for a
// following parser.  Used to exercise composition.
inline program
run_matcher(const std::string& _set)
{
    using namespace peg;
    program p;
    p.push_back(instr{ SET,   -1, 0, _set }); p.push_back(instr{ CHOICE, 4, 0, "" });
    p.push_back(instr{ SET,   -1, 0, _set }); p.push_back(instr{ COMMIT, 1, 0, "" });
    p.push_back(instr{ MATCH, -1, 0, ""   });
    return p;
}

// same
//   helper: instruction-level program equality.
inline bool
same(const program& _a, const program& _b)
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
accept(const program& _prog, const std::string& _s)
{
    pr::parse_state<char> st(_s.data(), _s.size(), 0);
    return run(st, _prog, peg_ops());
}

// ---- section functions (defined in the vparse_tests_*.cpp TUs) -------------

bool tests_captures_backtrack();
bool tests_captures_generated();
bool tests_generate_anchor();
bool tests_generate_multirule();
bool tests_notation_anchor();
bool tests_notation_alternation();
bool tests_notation_plus_literal();
bool tests_rebase_handle();
bool tests_rebase_compose();

}  // namespace testing
}  // namespace djinterp

#endif  // DJINTERP_PARSE_VPARSE_TESTS_
