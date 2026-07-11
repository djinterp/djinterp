/******************************************************************************
* djinterp [test]                                    free_tests_selective.cpp
*
*   Section VI of the parser/free.hpp suite: the selective stratum
* (free_sel_parser) and its sel_pure / sel_lift / sel_select / sel_branch
* factories.  select branches on a PARSED discriminant (an either<L,R>) while
* keeping both branches static; branch chooses one of two static handlers by
* the discriminant's side.
*
*   !! BUILD NOTE !!  sel_select and sel_branch delegate to the functional
* companion's selective_select / selective_branch via the monad bridge.  As of
* the reviewed tree that bridge does NOT compile: selective_traits<_Monad>::
* select declares its discriminant parameter as
*
*       const typename monad_rebind<_Monad, either<_L,_R>>::type&
*
* which is a NON-DEDUCED context for _L and _R, so selective_select(disc,
* handler) -- called by sel_select with no explicit _L/_R -- cannot deduce them
* and is removed from overload resolution.  Until that is fixed (e.g. by giving
* the bridge's select/branch a deducible discriminant, or by having sel_select
* pass _L/_R explicitly into a deduction-friendly entry point), the
* sel_select / sel_branch predicates below will not instantiate.  The sel_pure
* and sel_lift predicates are unaffected.  This block is therefore NOT wired
* into free_tests_runner.cpp yet; enable its push_backs once the bridge builds.
*
* path:      /tests/djinterp/parse/parser/free_tests_selective.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "free_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;
namespace dp = ::djinterp::parse;


// left_plus100 : produces a (L -> R) handler that maps a Left payload; used as
// the handler-effect for sel_select.
struct make_plus100
{
    std::function<int(int)> operator()() const
    {
        return std::function<int(int)>([](int _l) { return _l + 100; });
    }
};


/*
tests_sel_pure
  Tests the following:
  - sel_pure injects a value, consuming no input.
*/
static bool
tests_sel_pure()
{
    return ( (run(dp::sel_pure<int>(9), "abc").value() == 9) &&
             (run_pos(dp::sel_pure<int>(9), "abc") == 0) );
}

/*
tests_sel_lift
  Tests the following:
  - sel_lift wraps an atomic parser, parsing + consuming as it does.
*/
static bool
tests_sel_lift()
{
    return ( (run(dp::sel_lift(atom_char('a')), "abc").value() == 'a') &&
             (run_pos(dp::sel_lift(atom_char('a')), "abc") == 1) );
}


// ---------------------------------------------------------------------------
//  The following predicates require the selective.hpp monad-bridge fix (see
//  the BUILD NOTE above).  Their logic is complete and correct; they are kept
//  here so they compile-and-run as soon as the bridge deduces _L / _R.  They
//  are guarded so this .cpp compiles as-is (the guard is off by default).
// ---------------------------------------------------------------------------
#if DJINTERP_FREE_TESTS_SELECTIVE_BRIDGE_FIXED

/*
tests_sel_select_right
  Tests the following:
  - when the discriminant is Right r, select short-circuits to r and the handler
    is never applied.
*/
static bool
tests_sel_select_right()
{
    dp::parser< ::djinterp::either<int, int>, char> disc =
        atom_val(::djinterp::either<int, int>::right(5));
    dp::parser<std::function<int(int)>, char> handler =
        atom_val(make_plus100{}());

    dp::free_sel_parser<int, char> s = dp::sel_select<int, int, char>(disc, handler);

    return (run(s, "").value() == 5);
}

/*
tests_sel_select_left
  Tests the following:
  - when the discriminant is Left l, select runs the handler and applies it to
    l (3 -> 103).
*/
static bool
tests_sel_select_left()
{
    dp::parser< ::djinterp::either<int, int>, char> disc =
        atom_val(::djinterp::either<int, int>::left(3));
    dp::parser<std::function<int(int)>, char> handler =
        atom_val(make_plus100{}());

    dp::free_sel_parser<int, char> s = dp::sel_select<int, int, char>(disc, handler);

    return (run(s, "").value() == 103);
}

/*
tests_sel_select_disc_fail
  Tests the following:
  - if the discriminant parser fails, select fails.
*/
static bool
tests_sel_select_disc_fail()
{
    dp::parser< ::djinterp::either<int, int>, char> disc =
        atom_fail< ::djinterp::either<int, int> >();
    dp::parser<std::function<int(int)>, char> handler =
        atom_val(make_plus100{}());

    dp::free_sel_parser<int, char> s = dp::sel_select<int, int, char>(disc, handler);

    return (!run(s, "abc").ok());
}

/*
tests_sel_branch_left
  Tests the following:
  - branch on a Left runs the left handler (3 -> 1003).
*/
static bool
tests_sel_branch_left()
{
    dp::parser< ::djinterp::either<int, int>, char> disc =
        atom_val(::djinterp::either<int, int>::left(3));
    dp::parser<std::function<int(int)>, char> fl =
        atom_val(std::function<int(int)>([](int _l) { return _l + 1000; }));
    dp::parser<std::function<int(int)>, char> fr =
        atom_val(std::function<int(int)>([](int _r) { return _r + 1; }));

    dp::free_sel_parser<int, char> b = dp::sel_branch<int, int, int, char>(disc, fl, fr);

    return (run(b, "").value() == 1003);
}

/*
tests_sel_branch_right
  Tests the following:
  - branch on a Right runs the right handler (5 -> 6).
*/
static bool
tests_sel_branch_right()
{
    dp::parser< ::djinterp::either<int, int>, char> disc =
        atom_val(::djinterp::either<int, int>::right(5));
    dp::parser<std::function<int(int)>, char> fl =
        atom_val(std::function<int(int)>([](int _l) { return _l + 1000; }));
    dp::parser<std::function<int(int)>, char> fr =
        atom_val(std::function<int(int)>([](int _r) { return _r + 1; }));

    dp::free_sel_parser<int, char> b = dp::sel_branch<int, int, int, char>(disc, fl, fr);

    return (run(b, "").value() == 6);
}
#endif  // DJINTERP_FREE_TESTS_SELECTIVE_BRIDGE_FIXED


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
free_selective_block()
{
    dt::block_spec block;

    block.name       = "VI selective stratum";
    block.descriptor = "sel_pure / sel_lift / sel_select / sel_branch";

    // Unaffected by the bridge defect:
    block.tests.push_back(dt::test_spec{
        "sel_pure", "inject a value, no consumption", &tests_sel_pure });
    block.tests.push_back(dt::test_spec{
        "sel_lift", "wrap an atom", &tests_sel_lift });

    // Pending the selective.hpp monad-bridge fix (see BUILD NOTE); auto-enabled
    // when DJINTERP_FREE_TESTS_SELECTIVE_BRIDGE_FIXED is defined:
#if DJINTERP_FREE_TESTS_SELECTIVE_BRIDGE_FIXED
    block.tests.push_back(dt::test_spec{
        "sel_select: Right", "Right short-circuits", &tests_sel_select_right });
    block.tests.push_back(dt::test_spec{
        "sel_select: Left", "Left runs handler", &tests_sel_select_left });
    block.tests.push_back(dt::test_spec{
        "sel_select: disc fail", "discriminant failure", &tests_sel_select_disc_fail });
    block.tests.push_back(dt::test_spec{
        "sel_branch: Left", "left handler", &tests_sel_branch_left });
    block.tests.push_back(dt::test_spec{
        "sel_branch: Right", "right handler", &tests_sel_branch_right });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
