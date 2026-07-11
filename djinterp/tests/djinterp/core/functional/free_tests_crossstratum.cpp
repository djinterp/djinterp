/******************************************************************************
* djinterp [test]                                free_tests_crossstratum.cpp
*
*   Section VII of the parser/free.hpp suite: the cross-stratum lifts along the
* chain  FreeAp F A  <=  FreeSel F A  <=  Free F A.  ap_to_sel rewraps the same
* underlying parser one stratum up; sel_to_monad / ap_to_monad lift the
* underlying handle into one F-layer of the canonical free-monad construction.
* Each lift must preserve parsing behaviour (same value, same residual) while
* changing the static stratum (the result type).
*
* path:      /tests/djinterp/parse/parser/free_tests_crossstratum.cpp
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
tests_ap_to_sel_preserves
  Tests the following:
  - ap_to_sel promotes a FreeAp parser to a FreeSel parser that parses
    identically (same value, same consumption).
*/
static bool
tests_ap_to_sel_preserves()
{
    dp::free_sel_parser<char, char> s = dp::ap_to_sel(dp::ap_lift(atom_char('a')));

    return ( (run(s, "abc").value() == 'a') &&
             (run_pos(s, "abc") == 1) &&
             (!run(dp::ap_to_sel(dp::ap_lift(atom_char('a'))), "xyz").ok()) );
}

/*
tests_ap_to_sel_type
  Tests the following:
  - ap_to_sel yields a free_sel_parser of the same R, E.
*/
static bool
tests_ap_to_sel_type()
{
    dp::free_ap_parser<char, char>  a = dp::ap_lift(atom_any());
    const bool same = std::is_same<
        decltype(dp::ap_to_sel(a)),
        dp::free_sel_parser<char, char> >::value;

    return same;
}

/*
tests_sel_to_monad_preserves
  Tests the following:
  - sel_to_monad promotes a FreeSel parser to a Free (monadic) parser that
    parses identically.
*/
static bool
tests_sel_to_monad_preserves()
{
    dp::free_parser<char, char> m = dp::sel_to_monad(dp::sel_lift(atom_char('a')));

    return ( (run(m, "abc").value() == 'a') &&
             (run_pos(m, "abc") == 1) );
}

/*
tests_sel_to_monad_type
  Tests the following:
  - sel_to_monad yields a free_parser of the same R, E.
*/
static bool
tests_sel_to_monad_type()
{
    dp::free_sel_parser<char, char> s = dp::sel_lift(atom_any());
    const bool same = std::is_same<
        decltype(dp::sel_to_monad(s)),
        dp::free_parser<char, char> >::value;

    return same;
}

/*
tests_ap_to_monad_preserves
  Tests the following:
  - ap_to_monad promotes a FreeAp parser directly to a Free parser that parses
    identically.
*/
static bool
tests_ap_to_monad_preserves()
{
    dp::free_parser<char, char> m = dp::ap_to_monad(dp::ap_lift(atom_char('a')));

    return ( (run(m, "abc").value() == 'a') &&
             (run_pos(m, "abc") == 1) );
}

/*
tests_ap_to_monad_type
  Tests the following:
  - ap_to_monad yields a free_parser of the same R, E.
*/
static bool
tests_ap_to_monad_type()
{
    dp::free_ap_parser<char, char> a = dp::ap_lift(atom_any());
    const bool same = std::is_same<
        decltype(dp::ap_to_monad(a)),
        dp::free_parser<char, char> >::value;

    return same;
}

/*
tests_ap_to_monad_failure
  Tests the following:
  - the lift preserves failure: promoting a failing applicative parser yields a
    monadic parser that also fails.
*/
static bool
tests_ap_to_monad_failure()
{
    return (!run(dp::ap_to_monad(dp::ap_lift(atom_char('a'))), "xyz").ok());
}

/*
tests_ap_to_monad_equals_composition
  Tests the following:
  - ap_to_monad agrees with the two-step composition sel_to_monad . ap_to_sel
    (same value, same consumption).
*/
static bool
tests_ap_to_monad_equals_composition()
{
    dp::free_parser<char, char> direct = dp::ap_to_monad(dp::ap_lift(atom_char('a')));
    dp::free_parser<char, char> viaSel = dp::sel_to_monad(dp::ap_to_sel(dp::ap_lift(atom_char('a'))));

    return ( (run(direct, "abc").value() == run(viaSel, "abc").value()) &&
             (run_pos(direct, "abc") == run_pos(viaSel, "abc")) );
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
free_crossstratum_block()
{
    dt::block_spec block;

    block.name       = "VII cross-stratum lifts";
    block.descriptor = "ap_to_sel / sel_to_monad / ap_to_monad";

    block.tests.push_back(dt::test_spec{
        "ap_to_sel: preserves", "same parse, one stratum up", &tests_ap_to_sel_preserves });
    block.tests.push_back(dt::test_spec{
        "ap_to_sel: type", "yields free_sel_parser", &tests_ap_to_sel_type });
    block.tests.push_back(dt::test_spec{
        "sel_to_monad: preserves", "same parse, into Free", &tests_sel_to_monad_preserves });
    block.tests.push_back(dt::test_spec{
        "sel_to_monad: type", "yields free_parser", &tests_sel_to_monad_type });
    block.tests.push_back(dt::test_spec{
        "ap_to_monad: preserves", "same parse, App into Free", &tests_ap_to_monad_preserves });
    block.tests.push_back(dt::test_spec{
        "ap_to_monad: type", "yields free_parser", &tests_ap_to_monad_type });
    block.tests.push_back(dt::test_spec{
        "ap_to_monad: failure", "failure preserved", &tests_ap_to_monad_failure });
    block.tests.push_back(dt::test_spec{
        "ap_to_monad == sel_to_monad . ap_to_sel", "composition agrees", &tests_ap_to_monad_equals_composition });

    return block;
}


NS_END  // testing
NS_END  // djinterp
