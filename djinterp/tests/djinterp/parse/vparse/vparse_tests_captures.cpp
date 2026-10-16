/*******************************************************************************
* djinterp [parse]                                     vparse_tests_captures.cpp
*
*
* path:      /tests/djinterp/parse/vparse/vparse_tests_captures.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "vparse_tests.hpp"

namespace djinterp
{
namespace testing
{

// tests_captures_backtrack
//   MARK / CAP record a tagged span, and a capture on a path that later fails
// is unwound exactly as the cursor is.
bool
tests_captures_backtrack()
{
    using namespace vp::peg;
    const std::string D = "0123456789";

    // MARK ; [0-9]+ ; CAP #7 ; MATCH   (no end-of-input gate)
    program p = { instr{ MARK, -1, 0, "" }, instr{ SET, -1, 0, D },
                  instr{ CHOICE, 5, 0, "" }, instr{ SET, -1, 0, D },
                  instr{ COMMIT, 2, 0, "" }, instr{ CAP, 7, 0, "" },
                  instr{ MATCH, -1, 0, "" } };

    struct expect { std::string in; std::string want; };
    const expect cases[] = { { "12a", "12" }, { "98765x", "98765" } };

    for (const expect& c : cases)
    {
        pr::parse_state<char>     st(c.in.data(), c.in.size(), 0);
        std::vector<capture>      caps;
        if (!run(st, p, peg_ops(), &caps))                         { return false; }
        if ((caps.size() != 1) || (caps[0].tag != 7))              { return false; }
        if (c.in.substr(caps[0].start, caps[0].end - caps[0].start) != c.want)
        {
            return false;
        }
    }
    return true;
}

// tests_captures_generated
//   captures emitted by the generator work end to end, including unwinding
// across an alternation backtrack (the "abc" path falls through to #2 clean).
bool
tests_captures_generated()
{
    const std::string D = "0123456789";
    const std::string L = "abcdefghijklmnopqrstuvwxyz";

    vp::ruleset g;
    g.rules.push_back(vp::rule{ "S",
        { { vp::ref("Digits", false, 1) }, { vp::ref("Lowers", false, 2) } } });
    g.rules.push_back(vp::rule{ "Digits", { { vp::cls(D), vp::cls(D, true) } } });
    g.rules.push_back(vp::rule{ "Lowers", { { vp::cls(L), vp::cls(L, true) } } });

    vp::peg::program prog = vp::gen::compile(g);

    struct expect { std::string in; int tag; std::string want; };
    const expect cases[] = { { "123", 1, "123" }, { "abc", 2, "abc" } };

    for (const expect& c : cases)
    {
        pr::parse_state<char>         st(c.in.data(), c.in.size(), 0);
        std::vector<vp::peg::capture> caps;
        if (!vp::peg::run(st, prog, peg_ops(), &caps))             { return false; }
        if ((caps.size() != 1) || (caps[0].tag != c.tag))         { return false; }
        if (c.in.substr(caps[0].start, caps[0].end - caps[0].start) != c.want)
        {
            return false;
        }
    }
    return true;
}

}  // namespace testing
}  // namespace djinterp
