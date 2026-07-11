/******************************************************************************
* djinterp [test]                                        free_tests_monad.cpp
*
*   Sections III-IV of the parser/free.hpp suite: the monadic stratum -- the
* parsing_program free-monad construction, lift_to_program / to_parser, the
* free_parser wrapper, and the free_pure / free_lift / free_bind / free_map
* factories.  free_bind is a genuine monadic bind (a later parser may depend on
* an earlier parsed value); to_parser realises the "free monad over a monad
* reduces to the monad" identity by iterative descent.
*
* path:      /tests/djinterp/parse/parser/free_tests_monad.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "free_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;
namespace dp = ::djinterp::parse;


/*
tests_free_pure
  Tests the following:
  - free_pure lifts a value as a Pure leaf: it succeeds with the value and
    consumes no input.
*/
static bool
tests_free_pure()
{
    const parse_result<int> r = run(dp::free_pure<int>(42), "abc");

    return ( r.ok() &&
             (r.value() == 42) &&
             (run_pos(dp::free_pure<int>(42), "abc") == 0) );
}

/*
tests_free_lift_match
  Tests the following:
  - free_lift lifts an atomic parser as one F-layer: it parses and consumes
    exactly as the atom does.
*/
static bool
tests_free_lift_match()
{
    const parse_result<char> r = run(dp::free_lift(atom_char('a')), "abc");

    return ( r.ok() &&
             (r.value() == 'a') &&
             (run_pos(dp::free_lift(atom_char('a')), "abc") == 1) );
}

/*
tests_free_lift_fail
  Tests the following:
  - a lifted atom that does not match fails.
*/
static bool
tests_free_lift_fail()
{
    return (!run(dp::free_lift(atom_char('a')), "xyz").ok());
}

/*
tests_free_bind_sequence
  Tests the following:
  - free_bind sequences two parsers: a-then-b succeeds only when both do, and
    consumes both elements; failure of either arm fails the whole.
*/
static bool
tests_free_bind_sequence()
{
    struct then_b
    {
        dp::free_parser<char, char> operator()(char) const
        {
            return dp::free_lift(atom_char('b'));
        }
    };

    const parse_result<char> ok   = run(dp::free_bind(dp::free_lift(atom_char('a')), then_b()), "ab");
    const bool               both = ok.ok() && (ok.value() == 'b') &&
                                    (run_pos(dp::free_bind(dp::free_lift(atom_char('a')), then_b()), "ab") == 2);

    const bool second_fail = !run(dp::free_bind(dp::free_lift(atom_char('a')), then_b()), "ax").ok();
    const bool first_fail  = !run(dp::free_bind(dp::free_lift(atom_char('a')), then_b()), "xb").ok();

    return (both && second_fail && first_fail);
}

/*
tests_free_bind_threading
  Tests the following:
  - free_bind threads a pure value into the continuation: pure 5 >>= (x -> pure
    2x) yields 10.
*/
static bool
tests_free_bind_threading()
{
    struct dbl
    {
        dp::free_parser<int, char> operator()(int _x) const
        {
            return dp::free_pure<int>(_x * 2);
        }
    };

    return (run(dp::free_bind(dp::free_pure<int>(5), dbl()), "").value() == 10);
}

/*
tests_free_bind_uses_parsed_value
  Tests the following:
  - the continuation may depend on an earlier PARSED value (the essence of
    monadic bind): parse one element, then produce its code.
*/
static bool
tests_free_bind_uses_parsed_value()
{
    struct code_of
    {
        dp::free_parser<int, char> operator()(char _c) const
        {
            return dp::free_pure<int>(static_cast<int>(_c));
        }
    };

    return (run(dp::free_bind(dp::free_lift(atom_any()), code_of()), "A").value() == 65);
}

/*
tests_free_bind_nested
  Tests the following:
  - nested binds compose into a longer program: a-then-b-then-c parses "abc",
    yielding the last value and consuming all three.
*/
static bool
tests_free_bind_nested()
{
    struct then_c
    {
        dp::free_parser<char, char> operator()(char) const
        {
            return dp::free_lift(atom_char('c'));
        }
    };
    struct then_bc
    {
        dp::free_parser<char, char> operator()(char) const
        {
            return dp::free_bind(dp::free_lift(atom_char('b')), then_c());
        }
    };

    const parse_result<char> r =
        run(dp::free_bind(dp::free_lift(atom_char('a')), then_bc()), "abc");

    return ( r.ok() &&
             (r.value() == 'c') &&
             (run_pos(dp::free_bind(dp::free_lift(atom_char('a')), then_bc()), "abc") == 3) );
}

/*
tests_free_map_transform
  Tests the following:
  - free_map transforms the produced value without disturbing parsing: mapping
    an element to its code.
*/
static bool
tests_free_map_transform()
{
    return (run(dp::free_map(dp::free_lift(atom_any()), to_code()), "A").value() == 65);
}

/*
tests_free_map_shortcircuit
  Tests the following:
  - free_map propagates failure: mapping over a failing parser fails.
*/
static bool
tests_free_map_shortcircuit()
{
    return (!run(dp::free_map(dp::free_lift(atom_char('a')), to_code()), "x").ok());
}

/*
tests_lift_to_program_to_parser
  Tests the following:
  - to_parser interprets a lifted program back into a parser that parses exactly
    as the original atom (the free-monad-over-a-monad identity).
*/
static bool
tests_lift_to_program_to_parser()
{
    dp::parser<char, char> p =
        dp::to_parser<char, char>(dp::lift_to_program(atom_char('a')));

    parse_state<char> st("a", 1, 0);
    const parse_result<char> r = p.parse(st);

    return ( r.ok() && (r.value() == 'a') && (st.offset == 1) );
}

/*
tests_free_program_is_pure
  Tests the following:
  - the underlying program is a Pure leaf for free_pure and a Roll layer for
    free_lift (inspectable via program()).
*/
static bool
tests_free_program_is_pure()
{
    return ( dp::free_pure<int>(1).program().is_pure() &&
             (!dp::free_lift(atom_any()).program().is_pure()) );
}

/*
tests_free_default_fails
  Tests the following:
  - a default-constructed free_parser fails when interpreted (its program is a
    fail layer, so it needs no default-constructible result).
*/
static bool
tests_free_default_fails()
{
    dp::free_parser<int, char> fp;

    return (!run(fp, "abc").ok());
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
free_monad_block()
{
    dt::block_spec block;

    block.name       = "III-IV monad stratum";
    block.descriptor =
        "free_pure / free_lift / free_bind / free_map, lift_to_program / to_parser";

    block.tests.push_back(dt::test_spec{
        "free_pure", "Pure leaf: value, no consumption", &tests_free_pure });
    block.tests.push_back(dt::test_spec{
        "free_lift: match", "lifts an atom, parses + consumes", &tests_free_lift_match });
    block.tests.push_back(dt::test_spec{
        "free_lift: fail", "non-matching atom fails", &tests_free_lift_fail });
    block.tests.push_back(dt::test_spec{
        "free_bind: sequence", "a-then-b; both required", &tests_free_bind_sequence });
    block.tests.push_back(dt::test_spec{
        "free_bind: threading", "pure value into continuation", &tests_free_bind_threading });
    block.tests.push_back(dt::test_spec{
        "free_bind: parsed value", "continuation depends on parsed value", &tests_free_bind_uses_parsed_value });
    block.tests.push_back(dt::test_spec{
        "free_bind: nested", "a-then-b-then-c", &tests_free_bind_nested });
    block.tests.push_back(dt::test_spec{
        "free_map: transform", "map the produced value", &tests_free_map_transform });
    block.tests.push_back(dt::test_spec{
        "free_map: short-circuit", "failure propagates", &tests_free_map_shortcircuit });
    block.tests.push_back(dt::test_spec{
        "lift_to_program + to_parser", "free-over-monad identity", &tests_lift_to_program_to_parser });
    block.tests.push_back(dt::test_spec{
        "program(): Pure vs Roll", "inspect the free-monad shape", &tests_free_program_is_pure });
    block.tests.push_back(dt::test_spec{
        "default free_parser fails", "uninitialised program fails", &tests_free_default_fails });

    return block;
}


NS_END  // testing
NS_END  // djinterp
