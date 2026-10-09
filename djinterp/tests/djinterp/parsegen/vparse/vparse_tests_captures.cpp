/*******************************************************************************
* djinterp [parsegen]                                  vparse_tests_captures.cpp
*
* vparse tests: MARK / CAP captures -- backtrack-safe unwinding, captures the
* generator emits, and the traced run on a caller's machine.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests_captures.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

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
    program p = { ins(MARK), ins(SET, -1, 0, D),
                  ins(CHOICE, 5), ins(SET, -1, 0, D),
                  ins(COMMIT, 2), ins(CAP, 7),
                  ins(MATCH) };

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

// tests_captures_trace
//   run() on a caller's machine traces one line per dispatch -- the last is
// the MATCH -- shows a backtrack restoring the cursor, honours the step limit
// with the offset left put, detaches its state, and agrees with the default
// run() on result and captures.
bool
tests_captures_trace()
{
    const std::string D = "0123456789";
    const std::string L = "abcdefghijklmnopqrstuvwxyz";

    vp::ruleset g;
    g.rules.push_back(vp::rule{ "S",
        { { vp::ref("Digits", false, 1) },
          { vp::ref("Lowers", false, 2) } } });
    g.rules.push_back(vp::rule{ "Digits",
        { { vp::cls(D), vp::cls(D, true) } } });
    g.rules.push_back(vp::rule{ "Lowers",
        { { vp::cls(L), vp::cls(L, true) } } });

    const vp::peg::program prog = vp::gen::compile(g);
    const std::string      in   = "abc";

    // a traced run: same answer as the default run, one line per dispatch
    pr::parse_state<char>         st(in.data(), in.size(), 0);
    std::vector<vp::peg::capture> caps;
    std::vector<std::string>      trace;
    vp::machine                   m;
    m.trace = &trace;

    if (!vp::peg::run(m, st, prog, peg_ops(), &caps))
    {
        return false;
    }

    if ( (static_cast<long>(trace.size()) != (m.steps + 1))    ||
         (trace.back().find("| MATCH") == std::string::npos) ||
         (st.offset != in.size())                            ||
         (m.ext != nullptr) )
    {
        return false;
    }

    pr::parse_state<char>         plain(in.data(), in.size(), 0);
    std::vector<vp::peg::capture> plain_caps;

    if (!vp::peg::run(plain, prog, peg_ops(), &plain_caps))
    {
        return false;
    }

    if ( (plain_caps.size() != caps.size())     ||
         (plain_caps[0].tag != caps[0].tag)     ||
         (plain_caps[0].start != caps[0].start) ||
         (plain_caps[0].end != caps[0].end) )
    {
        return false;
    }

    // "abc" fails the Digits alternative at off=0, so some later line shows
    // the cursor back at 0 after a line at 0 with a backtrack point pushed
    bool saw_backtrack_point = false;

    for (const std::string& line : trace)
    {
        saw_backtrack_point = ( (saw_backtrack_point) ||
                                (line.find(" back=1 ") != std::string::npos) );
    }

    if (!saw_backtrack_point)
    {
        return false;
    }

    // the step limit stops the run, fails it, and leaves the offset put
    pr::parse_state<char> limited(in.data(), in.size(), 0);
    vp::machine           short_machine;
    short_machine.step_limit = 3;

    if (vp::peg::run(short_machine, limited, prog, peg_ops()))
    {
        return false;
    }

    return ( (limited.offset == 0)                  &&
             (short_machine.error == "step limit") &&
             (short_machine.steps == 3) );
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
