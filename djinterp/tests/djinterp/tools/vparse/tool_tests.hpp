/*******************************************************************************
* vparse [tests]                                                  tool_tests.hpp
*
* What the tool's suites share: the grammars they run, the few helpers they
* read runs with, and the loop that runs a table of sections.
*   Three suites stand on it: the model's (workbench_core_tests.cpp), which
* needs nothing but djinterp; the window's face (workbench_tests.cpp), which
* needs uxoxo; and the command line's (cli_tests.cpp), which needs the
* argument parser. Each is a table of sections, as djinterp's suites are,
* whose main() is run_sections() over the table, printing the line the
* build's test holds it to.
*   Everything here is inline, or a constant, so that each suite takes what it
* uses and no suite links another's.
*   LEVELS. The model's floor is C++17, so this file's is: below it the file
* is empty rather than an error.
*
*
* path:      /tests/djinterp/tools/vparse/tool_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  GRAMMARS
    --------
    1.  SUMS
    2.  NESTED / NESTED_INPUT
    3.  ARITHMETIC
    4.  chain
2.  HELPERS
    -------
    1.  has
    2.  request
    3.  disassembly
3.  THE SUITE
    ---------
    1.  section
    2.  run_sections
*/

#ifndef VPARSE_TESTS_TOOL_TESTS_HPP
#define VPARSE_TESTS_TOOL_TESTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as
// the model it tests is.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <string>   // std::string, std::to_string
// djinterp
#include "djinterp/cpp/modules/parsegen/vparse/peg.hpp"  // peg::program, peg::fmt
// vparse
#include "../../../../inc/djinterp/tools/vparse/run.hpp"  // run_request


namespace vparse
{
namespace tests
{

namespace vp = ::djinterp::parsegen::vparse;
namespace wb = ::vparse::workbench;


//==============================================================================
// 1.  GRAMMARS
//==============================================================================


// 1.1    SUMS
//------------------------------------------------------------------------------
// 1.1.1
// SUMS
//   constant: a grammar in the notation that factors its repetition -- the
// window's own example.
const char* const SUMS =
    "Sum  = Term More* ;\n"
    "More = '+' Term | '-' Term ;\n"
    "Term = [0-9]+ ;\n";

// 1.2    NESTED / NESTED_INPUT
//------------------------------------------------------------------------------
// 1.2.1
// NESTED
//   constant: a grammar in the notation that re-parses: every alternative of
// Sum and Prod begins with the same rule, so a failure at the operator throws
// the work away and does it again.
const char* const NESTED =
    "Expr = Sum ;\n"
    "Sum  = Prod '+' Sum | Prod '-' Sum | Prod ;\n"
    "Prod = Atom '*' Prod | Atom '/' Prod | Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// 1.2.2
// NESTED_INPUT
//   constant: what NESTED's heat was first measured over.
const char* const NESTED_INPUT = "((1+2)*(3-4))/5";

// 1.3    ARITHMETIC
//------------------------------------------------------------------------------
// 1.3.1
// ARITHMETIC
//   constant: the same language in EBNF, with grouping.
const char* const ARITHMETIC =
    "Expr   = Term ((\"+\" | \"-\") Term)* ;\n"
    "Term   = Factor ((\"*\" | \"/\") Factor)* ;\n"
    "Factor = \"(\" Expr \")\" | Digit+ ;\n"
    "Digit  = [0-9] ;\n";

// 1.4    chain
//------------------------------------------------------------------------------
// 1.4.1
// chain
//   function: a grammar in the notation of `_rules` rules, at least one, each
// of which may defer to the next -- `R0 = [a-z] R1 | [0-9] ;` and on, the
// last a digit alone -- which takes up to `_rules - 1` letters and then a
// digit. At a few hundred rules, reading it takes more steps than a
// request's default limit.
inline
std::string chain(
    std::size_t _rules
)
{
    std::string out;

    // every rule but the last defers to the next
    for (std::size_t at = 0u; (at + 1u) < _rules; ++at)
    {
        out += "R" + std::to_string(at) + " = [a-z] R" +
               std::to_string(at + 1u) + " | [0-9] ;\n";
    }

    out += "R" + std::to_string(_rules - 1u) + " = [0-9] ;\n";

    return out;
}


//==============================================================================
// 2.  HELPERS
//==============================================================================


// 2.1    has
//------------------------------------------------------------------------------
// 2.1.1
// has
//   function: whether a text contains another.
inline
bool has(
    const std::string& _text,
    const std::string& _part
)
{
    return (_text.find(_part) != std::string::npos);
}

// 2.2    request
//------------------------------------------------------------------------------
// 2.2.1
// request
//   function: a run of a grammar over an input, in a notation.
inline
wb::run_request request(
    const std::string& _frontend,
    const std::string& _grammar,
    const std::string& _input
)
{
    wb::run_request out;

    out.frontend = _frontend;
    out.grammar  = _grammar;
    out.input    = _input;

    return out;
}

// 2.3    disassembly
//------------------------------------------------------------------------------
// 2.3.1
// disassembly
//   function: a program listed the way session::disassembly() lists one --
// addresses right-aligned to four columns, two spaces, the instruction.
inline
std::string disassembly(
    const vp::peg::program& _program
)
{
    std::string out;

    // an instruction a line
    for (std::size_t at = 0u; at < _program.size(); ++at)
    {
        const std::string address = std::to_string(at);

        out.append((address.size() < 4u) ? (4u - address.size()) : 0u,
                   ' ');
        out += address + "  " + vp::peg::fmt(_program[at]) + "\n";
    }

    return out;
}


//==============================================================================
// 3.  THE SUITE
//==============================================================================


// 3.1    section
//------------------------------------------------------------------------------
// 3.1.1
// section
//   struct: one section of a suite -- its name, and the function that runs
// it.
struct section
{
    const char* name;
    bool      (*run)();
};

// 3.2    run_sections
//------------------------------------------------------------------------------
// 3.2.1
// run_sections
//   function: runs every section of a suite, in order, printing a line for
// each and then the summary line the build's test matches; returns main's
// status, 0 when every section passed.
template<std::size_t Count>
int run_sections(
    const char* _title,
    const section (&_sections)[Count]
)
{
    int passed = 0;
    int failed = 0;

    std::printf("%s\n",
                _title);

    // every section, in order
    for (const section& each : _sections)
    {
        const bool ok = each.run();

        std::printf("  [%s] %s\n",
                    ok ? "PASS" : "FAIL",
                    each.name);

        // tally for the summary line
        if (ok)
        {
            passed++;
        }
        else
        {
            failed++;
        }
    }

    std::printf("passed: %d   failed: %d\n",
                passed,
                failed);

    return (failed == 0) ? 0 : 1;
}

}  // namespace tests
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_TESTS_TOOL_TESTS_HPP
