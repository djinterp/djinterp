/*******************************************************************************
* djinterp [parsegen]                                  vparse_tests_notation.cpp
*
* vparse tests: the metacircular notation -- the anchor reached through text,
* alternation, and '+' with a character literal.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests_notation.cpp
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

// tests_notation_anchor
//   the text front-end (via '+') reproduces the hand-written parser: the whole
// text -> ruleset -> generator pipeline agrees with the oracle.
bool
tests_notation_anchor()
{
    bool        ok = false;
    vp::ruleset g  = vp::notation::parse_grammar("S = [0-9]+ ;", &ok);
    if (!ok) { return false; }

    vp::peg::program prog = vp::gen::compile(g);
    return same(prog, digits_program());
}

// tests_notation_alternation
//   '|' in the notation compiles to an ordered choice: the parser recognises
// either alternative and nothing mixed.
bool
tests_notation_alternation()
{
    bool        ok = false;
    vp::ruleset g  = vp::notation::parse_grammar("S = [0-9]+ | [a-z]+ ;", &ok);
    if (!ok) { return false; }

    vp::peg::program prog = vp::gen::compile(g);
    if (!accept(prog, "123"))  { return false; }
    if (!accept(prog, "abc"))  { return false; }
    if ( accept(prog, "12a"))  { return false; }
    if ( accept(prog, ""))     { return false; }
    return true;
}

// tests_notation_plus_literal
//   a grammar exercising '|', '+', and a character literal together compiles
// to a working (right-recursive) recognizer.
bool
tests_notation_plus_literal()
{
    bool        ok = false;
    vp::ruleset g  = vp::notation::parse_grammar(
        "Expr = Term '+' Expr | Term ;\n"
        "Term = [0-9]+ ;", &ok);
    if (!ok) { return false; }

    vp::peg::program prog = vp::gen::compile(g);
    if (!accept(prog, "1"))     { return false; }
    if (!accept(prog, "1+2"))   { return false; }
    if (!accept(prog, "1+2+3")) { return false; }
    if ( accept(prog, "1+"))    { return false; }
    if ( accept(prog, "+1"))    { return false; }
    if ( accept(prog, ""))      { return false; }
    return true;
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
