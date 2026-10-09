/*******************************************************************************
* djinterp [parsegen]                                      vparse_tests_ebnf.cpp
*
* vparse tests: the EBNF frontend -- the standalone prototype's arithmetic
* demo, the constructs the metacircular notation cannot express, and how a
* malformed grammar is reported.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests_ebnf.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <initializer_list>  // std::initializer_list
#include <string>            // std::string


namespace djinterp
{
namespace testing
{

namespace
{

// build
//   function: parses and compiles grammar text, reporting whether both
// succeeded.
vp::peg::program
build(
    const std::string& _text,
    bool&              _ok
)
{
    bool       parsed  = false;
    const auto grammar = vp::ebnf::parse(_text, &parsed);

    if (!parsed)
    {
        _ok = false;

        return vp::peg::program();
    }

    return vp::ebnf::compile(grammar.first, grammar.second, &_ok);
}

// agrees
//   function: whether a program accepts every input of one list and none of
// another.
bool
agrees(
    const vp::peg::program&            _program,
    std::initializer_list<const char*> _accepted,
    std::initializer_list<const char*> _refused
)
{
    for (const char* text : _accepted)
    {
        if (!accept(_program, text))
        {
            return false;
        }
    }

    for (const char* text : _refused)
    {
        if (accept(_program, text))
        {
            return false;
        }
    }

    return true;
}

// compiles
//   function: whether grammar text parses and then compiles, with the reason
// it did not compile where it parsed.
bool
compiles(
    const std::string& _text,
    bool&              _parsed,
    std::string&       _why
)
{
    bool       compiled = true;
    const auto grammar  = vp::ebnf::parse(_text, &_parsed);

    if (!_parsed)
    {
        return false;
    }

    (void)vp::ebnf::compile(grammar.first, grammar.second, &compiled, &_why);

    return compiled;
}

}  // namespace

/*
tests_ebnf_arithmetic
  The prototype's demo: one arithmetic grammar, its digits written twice.
  Tests the following:
  - digits as ten literals and digits as one class both parse and compile
  - the two programs agree on every case, accepted and refused
  - the class form compiles to the smaller program
*/
bool
tests_ebnf_arithmetic()
{
    const std::string literals =
        "Expr   = Term (\"+\" Term)* ;\n"
        "Term   = Factor (\"*\" Factor)* ;\n"
        "Factor = \"(\" Expr \")\" | Digit ;\n"
        "Digit  = \"0\"|\"1\"|\"2\"|\"3\"|\"4\""
                "|\"5\"|\"6\"|\"7\"|\"8\"|\"9\" ;\n";
    const std::string klass =
        "Expr   = Term (\"+\" Term)* ;\n"
        "Term   = Factor (\"*\" Factor)* ;\n"
        "Factor = \"(\" Expr \")\" | Digit ;\n"
        "Digit  = [0-9] ;\n";

    bool ok_literals = false;
    bool ok_klass    = false;

    const vp::peg::program by_literals = build(literals, ok_literals);
    const vp::peg::program by_klass    = build(klass, ok_klass);

    if ( (!ok_literals)                             ||
         (!ok_klass)                                ||
         (by_klass.size() >= by_literals.size())    )
    {
        return false;
    }

    return ( (agrees(by_literals,
                     { "2*(3+4)", "2+3*4", "((1))" },
                     { "2*", "2+(3", "1+2)", "" }))     &&
             (agrees(by_klass,
                     { "2*(3+4)", "2+3*4", "((1))" },
                     { "2*", "2+(3", "1+2)", "" }))     );
}

/*
tests_ebnf_grouping
  What the metacircular notation cannot express.
  Tests the following:
  - a group, and `?` over it
  - `+` over a multi-character literal
  - `.`, any one symbol
  - a `#` comment after a rule
*/
bool
tests_ebnf_grouping()
{
    bool ok_optional = false;
    bool ok_repeated = false;
    bool ok_any      = false;

    const vp::peg::program optional =
        build("S = \"a\" (\"b\" | \"c\")? \"d\" ;", ok_optional);
    const vp::peg::program repeated =
        build("S = (\"ab\")+ ;", ok_repeated);
    const vp::peg::program any =
        build("S = \"<\" . \">\" ;  # any one symbol", ok_any);

    if ( (!ok_optional)  ||
         (!ok_repeated)  ||
         (!ok_any)       )
    {
        return false;
    }

    return ( (agrees(optional,
                     { "ad", "abd", "acd" },
                     { "abcd", "abbd", "a", "d", "" }))     &&
             (agrees(repeated,
                     { "ab", "abab" },
                     { "aba", "", "ba" }))                  &&
             (agrees(any,
                     { "<x>" },
                     { "<>", "<xy>" }))                     );
}

/*
tests_ebnf_errors
  A malformed grammar is reported through `ok`, never thrown.
  Tests the following:
  - text that is not a grammar fails to parse
  - an undefined reference parses, and does not compile
  - a duplicated rule parses, and does not compile
  - a grammar that does not compile says why
*/
bool
tests_ebnf_errors()
{
    // none of these is a grammar
    for (const char* text : { "S = \"a\"",  "S = ( \"a\" ;", "= \"a\" ;",
                              "S = \"a ;",  "S = [a-z ;",    "S = @ ;" })
    {
        bool ok = true;

        (void)vp::ebnf::parse(text, &ok);

        if (ok)
        {
            return false;
        }
    }

    // an undefined reference and a duplicated rule parse, and do not compile
    for (const char* text : { "S = T ;", "S = \"a\" ; S = \"b\" ;" })
    {
        bool        parsed = false;
        std::string why;

        if ( (compiles(text, parsed, why))  ||
             (!parsed)                      ||
             (why.empty())                  )
        {
            return false;
        }
    }

    return true;
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
