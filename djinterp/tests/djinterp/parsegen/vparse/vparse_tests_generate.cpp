/*******************************************************************************
* djinterp [parsegen]                                  vparse_tests_generate.cpp
*
* vparse tests: the generator -- the byte-for-byte anchor, and a multi-rule
* grammar with back-patched cross-references.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests_generate.cpp
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

// tests_generate_anchor
//   compiling `S = [0-9]+` reproduces the hand-written parser byte-for-byte,
// and the result recognises exactly digit strings.
bool
tests_generate_anchor()
{
    const std::string D = "0123456789";
    vp::ruleset g;
    g.rules.push_back(vp::rule{ "S", { { vp::cls(D), vp::cls(D, true) } } });

    vp::peg::program prog = vp::gen::compile(g);
    if (!same(prog, digits_program())) { return false; }

    if (!accept(prog, "12345"))        { return false; }
    if (!accept(prog, "007"))          { return false; }
    if ( accept(prog, "12a"))          { return false; }
    if ( accept(prog, ""))             { return false; }
    return true;
}

// tests_generate_multirule
//   a multi-rule grammar generates a parser with correctly back-patched cross-
// references that recognises the concatenation.
bool
tests_generate_multirule()
{
    const std::string D = "0123456789";
    const std::string L = "abcdefghijklmnopqrstuvwxyz";

    vp::ruleset g;
    g.rules.push_back(vp::rule{ "S", { { vp::ref("A"), vp::ref("B") } } });
    g.rules.push_back(vp::rule{ "A", { { vp::cls(D), vp::cls(D, true) } } });
    g.rules.push_back(vp::rule{ "B", { { vp::cls(L), vp::cls(L, true) } } });

    vp::peg::program prog = vp::gen::compile(g);
    if (prog.size() != 19)         { return false; }

    if (!accept(prog, "12ab"))     { return false; }
    if (!accept(prog, "1a"))       { return false; }
    if ( accept(prog, "12"))       { return false; }
    if ( accept(prog, "ab"))       { return false; }
    if ( accept(prog, ""))         { return false; }
    return true;
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
