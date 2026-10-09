/*******************************************************************************
* djinterp [parsegen]                                    vparse_tests_rebase.cpp
*
* vparse tests: the carrier adapter -- a compiled program as a parser handle,
* and two of them threading one parse_state.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests_rebase.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

namespace djinterp
{
namespace testing
{

// tests_rebase_handle
//   a compiled program wears the parser<std::string, char> handle: it consumes
// a parse_state and returns a parse_result, advancing the offset on a match and
// leaving it (and reporting failure) otherwise.
bool
tests_rebase_handle()
{
    const std::string D = "0123456789";
    vp::ruleset g;
    g.rules.push_back(vp::rule{ "S", { { vp::cls(D), vp::cls(D, true) } } });

    pr::parser<std::string, char> p = vp::as_parser(vp::gen::compile(g));
    if (!p.ok()) { return false; }

    {   // whole-input match advances the offset and yields the text
        std::string           s = "12345";
        pr::parse_state<char> st(s.data(), s.size(), 0);
        pr::parse_result<std::string> r = p(st);
        if (!r.ok() || (r.value() != "12345") || (st.offset != 5)) { return false; }
    }
    {   // failure restores the offset and reports the status
        std::string           s = "12a";
        pr::parse_state<char> st(s.data(), s.size(), 0);
        pr::parse_result<std::string> r = p(st);
        if (r.ok() || (st.offset != 0) ||
            (r.error().status() != pr::DParseStatusFailure))       { return false; }
    }
    return true;
}

// tests_rebase_compose
//   two vparse parsers thread one parse_state: the second continues where the
// first stopped -- the contract seq / bind rely on.
bool
tests_rebase_compose()
{
    const std::string D = "0123456789";
    const std::string L = "abcdefghijklmnopqrstuvwxyz";

    pr::parser<std::string, char> p_dig = vp::as_parser(run_matcher(D));
    pr::parser<std::string, char> p_low = vp::as_parser(run_matcher(L));

    {   // digits then letters over "42abc"
        std::string           s = "42abc";
        pr::parse_state<char> st(s.data(), s.size(), 0);

        pr::parse_result<std::string> r1 = p_dig(st);
        if (!r1.ok() || (r1.value() != "42") || (st.offset != 2)) { return false; }

        pr::parse_result<std::string> r2 = p_low(st);
        if (!r2.ok() || (r2.value() != "abc") || (st.offset != 5)) { return false; }
    }
    {   // letters-first fails at '4' and leaves the offset put
        std::string           s = "42abc";
        pr::parse_state<char> st(s.data(), s.size(), 0);
        pr::parse_result<std::string> r = p_low(st);
        if (r.ok() || (st.offset != 0)) { return false; }
    }
    return true;
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
