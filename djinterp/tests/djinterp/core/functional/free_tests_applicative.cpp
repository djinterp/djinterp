/******************************************************************************
* djinterp [test]                                  free_tests_applicative.cpp
*
*   Section V of the parser/free.hpp suite: the applicative stratum
* (free_ap_parser) and its ap_pure / ap_lift / ap_apply / ap_map factories.
* ap_apply runs the function-parser then the argument-parser in sequence,
* threading the residual and short-circuiting on the first failure; ap_map is
* the constant-function specialisation.  Unlike the monadic stratum there is no
* value-dependency between the two effects -- the shape is static.
*
* path:      /tests/djinterp/parse/parser/free_tests_applicative.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "free_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;
namespace dp = ::djinterp::parse;


// makes a (char -> code) function; used to build a function-producing parser
// that also CONSUMES input, so ap_apply's residual threading can be observed.
struct code_fn_of
{
    std::function<int(char)> operator()(char) const
    {
        return std::function<int(char)>([](char _c) { return static_cast<int>(_c); });
    }
};


/*
tests_ap_pure
  Tests the following:
  - ap_pure injects a value, consuming no input.
*/
static bool
tests_ap_pure()
{
    return ( (run(dp::ap_pure<int>(7), "abc").value() == 7) &&
             (run_pos(dp::ap_pure<int>(7), "abc") == 0) );
}

/*
tests_ap_lift
  Tests the following:
  - ap_lift wraps an atomic parser, parsing + consuming as it does.
*/
static bool
tests_ap_lift()
{
    return ( (run(dp::ap_lift(atom_char('a')), "abc").value() == 'a') &&
             (run_pos(dp::ap_lift(atom_char('a')), "abc") == 1) );
}

/*
tests_ap_apply_pure
  Tests the following:
  - ap_apply of two pure effects applies the function to the argument.
*/
static bool
tests_ap_apply_pure()
{
    std::function<int(int)> f(times2{});

    dp::free_ap_parser<std::function<int(int)>, char> pf = dp::ap_pure<std::function<int(int)> >(f);
    dp::free_ap_parser<int, char>                     pa = dp::ap_pure<int>(21);

    return (run(dp::ap_apply(pf, pa), "").value() == 42);
}

/*
tests_ap_apply_over_parsed_arg
  Tests the following:
  - ap_apply applies a pure function to a PARSED argument.
*/
static bool
tests_ap_apply_over_parsed_arg()
{
    std::function<int(char)> f([](char _c) { return static_cast<int>(_c) + 1; });

    dp::free_ap_parser<std::function<int(char)>, char> pf = dp::ap_pure<std::function<int(char)> >(f);

    return (run(dp::ap_apply(pf, dp::ap_lift(atom_any())), "A").value() == 66);
}

/*
tests_ap_apply_sequence_consumes
  Tests the following:
  - ap_apply runs the function-parser THEN the argument-parser, threading the
    residual: a function-producing parser that consumes 'a', then an argument
    parser that consumes 'b', consumes both and applies.
*/
static bool
tests_ap_apply_sequence_consumes()
{
    // pf : consumes 'a', yields a (char -> code) function
    dp::free_ap_parser<std::function<int(char)>, char> pf =
        dp::ap_map(dp::ap_lift(atom_char('a')), code_fn_of{});
    // pa : consumes 'b'
    dp::free_ap_parser<char, char> pa = dp::ap_lift(atom_char('b'));

    const parse_result<int> r = run(dp::ap_apply(pf, pa), "ab");

    return ( r.ok() &&
             (r.value() == static_cast<int>('b')) &&
             (run_pos(dp::ap_apply(pf, pa), "ab") == 2) );
}

/*
tests_ap_apply_shortcircuit_pf
  Tests the following:
  - if the function-parser fails, ap_apply fails and the argument-parser is not
    consulted.
*/
static bool
tests_ap_apply_shortcircuit_pf()
{
    dp::free_ap_parser<std::function<int(int)>, char> pf =
        dp::ap_lift(atom_fail<std::function<int(int)> >());
    dp::free_ap_parser<int, char> pa = dp::ap_pure<int>(1);

    return (!run(dp::ap_apply(pf, pa), "abc").ok());
}

/*
tests_ap_apply_shortcircuit_pa
  Tests the following:
  - if the argument-parser fails, ap_apply fails.
*/
static bool
tests_ap_apply_shortcircuit_pa()
{
    std::function<int(char)> f([](char _c) { return static_cast<int>(_c); });

    dp::free_ap_parser<std::function<int(char)>, char> pf = dp::ap_pure<std::function<int(char)> >(f);

    return (!run(dp::ap_apply(pf, dp::ap_lift(atom_char('a'))), "xyz").ok());
}

/*
tests_ap_map_transform
  Tests the following:
  - ap_map transforms the produced value (element to its code).
*/
static bool
tests_ap_map_transform()
{
    return (run(dp::ap_map(dp::ap_lift(atom_any()), to_code{}), "A").value() == 65);
}

/*
tests_ap_map_shortcircuit
  Tests the following:
  - ap_map propagates failure.
*/
static bool
tests_ap_map_shortcircuit()
{
    return (!run(dp::ap_map(dp::ap_lift(atom_char('a')), to_code{}), "x").ok());
}

/*
tests_ap_map_type_change
  Tests the following:
  - ap_map changes the result type (char -> int), both at compile time and in
    the produced value.
*/
static bool
tests_ap_map_type_change()
{
    dp::free_ap_parser<int, char> mapped = dp::ap_map(dp::ap_lift(atom_any()), to_code{});

    return (run(mapped, "B").value() == 66);
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
free_applicative_block()
{
    dt::block_spec block;

    block.name       = "V applicative stratum";
    block.descriptor = "ap_pure / ap_lift / ap_apply / ap_map";

    block.tests.push_back(dt::test_spec{
        "ap_pure", "inject a value, no consumption", &tests_ap_pure });
    block.tests.push_back(dt::test_spec{
        "ap_lift", "wrap an atom", &tests_ap_lift });
    block.tests.push_back(dt::test_spec{
        "ap_apply: pure", "apply f to arg", &tests_ap_apply_pure });
    block.tests.push_back(dt::test_spec{
        "ap_apply: parsed arg", "apply f to a parsed value", &tests_ap_apply_over_parsed_arg });
    block.tests.push_back(dt::test_spec{
        "ap_apply: sequence", "run pf then pa, thread residual", &tests_ap_apply_sequence_consumes });
    block.tests.push_back(dt::test_spec{
        "ap_apply: short-circuit pf", "function-parser failure", &tests_ap_apply_shortcircuit_pf });
    block.tests.push_back(dt::test_spec{
        "ap_apply: short-circuit pa", "argument-parser failure", &tests_ap_apply_shortcircuit_pa });
    block.tests.push_back(dt::test_spec{
        "ap_map: transform", "map the produced value", &tests_ap_map_transform });
    block.tests.push_back(dt::test_spec{
        "ap_map: short-circuit", "failure propagates", &tests_ap_map_shortcircuit });
    block.tests.push_back(dt::test_spec{
        "ap_map: type change", "char -> int", &tests_ap_map_type_change });

    return block;
}


NS_END  // testing
NS_END  // djinterp
