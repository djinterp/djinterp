/******************************************************************************
* djinterp [test]                           test_builder_tests_assertions.cpp
*
*   The predicate algebra conjoined into the current test.  assert_ (and its
* intent-named twin test_fn) tells a deferred PREDICATE - evaluated at run() -
* from a bare boolean VALUE captured now, wrapping each into a thunk; the
* timing difference is the crisp observable, so a predicate reading a variable
* mutated after authoring sees the run()-time value while a captured value does
* not.  Repeated assertions conjoin under short-circuit AND; assert_all /
* assert_any fold a whole clause list under AND / OR (vacuously true / false
* when empty).  With no test open, every assertion is a no-op that binds
* nothing; a test never given a clause stays pending.
*
* path:      /tests/djinterp/test/test_builder/test_builder_tests_assertions.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_builder_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_assert_value_true_false
//   a bare boolean value drives the leaf's verdict directly.
bool
tests_assert_value_true_false()
{
    bool ok = true;

    dt::test_summary sp = dt::make_suite().test("t").assert_(true).run();
    ok = D_TB_CHECK(sp.passed == 1 && sp.failed == 0) && ok;

    dt::test_summary sf = dt::make_suite().test("t").assert_(false).run();
    ok = D_TB_CHECK(sf.failed == 1 && sf.passed == 0) && ok;

    // a boolean-valued expression is a value too (not a callable)
    dt::test_summary sc = dt::make_suite().test("t").assert_(1 + 1 == 2).run();
    ok = D_TB_CHECK(sc.passed == 1) && ok;

    return ok;
}

// tests_assert_predicate_deferred_timing
//   a predicate is evaluated at run() (so it sees a later mutation); a value
// is captured at authoring time (so it does not).
bool
tests_assert_predicate_deferred_timing()
{
    bool ok = true;

    // predicate: deferred, sees x == 1 at run() time
    int x = 0;
    dt::test_builder<> b;
    b.test("t").assert_([&x]{ return x == 1; });
    x = 1;
    dt::test_summary s = b.run();
    ok = D_TB_CHECK(s.passed == 1 && s.failed == 0) && ok;

    // value: captured now as (0 == 1) == false, unaffected by the later y = 1
    int y = 0;
    dt::test_builder<> b2;
    b2.test("t").assert_(y == 1);
    y = 1;
    dt::test_summary s2 = b2.run();
    ok = D_TB_CHECK(s2.failed == 1 && s2.passed == 0) && ok;

    return ok;
}

// tests_assert_conjunction_and
//   repeated assert_ / test_fn conjoin under AND (the first allocates the
// row, later ones compose onto it).
bool
tests_assert_conjunction_and()
{
    bool ok = true;

    ok = D_TB_CHECK(dt::make_suite().test("t").assert_(true).assert_(true).run().passed  == 1) && ok;
    ok = D_TB_CHECK(dt::make_suite().test("t").assert_(true).assert_(false).run().failed == 1) && ok;
    ok = D_TB_CHECK(dt::make_suite().test("t").assert_(false).assert_(true).run().failed == 1) && ok;

    // test_fn shares the mechanism
    ok = D_TB_CHECK(dt::make_suite().test("t").assert_(true).test_fn([]{ return true; }).run().passed  == 1) && ok;
    ok = D_TB_CHECK(dt::make_suite().test("t").test_fn([]{ return false; }).run().failed == 1) && ok;

    return ok;
}

// tests_assert_all_and_semantics
//   assert_all conjoins the AND of every clause: passes only if all pass.
bool
tests_assert_all_and_semantics()
{
    bool ok = true;

    dt::test_summary sp = dt::make_suite().test("t")
                              .assert_all(true, []{ return true; }, 1 == 1).run();
    ok = D_TB_CHECK(sp.passed == 1) && ok;

    dt::test_summary sf = dt::make_suite().test("t")
                              .assert_all(true, false, true).run();
    ok = D_TB_CHECK(sf.failed == 1) && ok;

    return ok;
}

// tests_assert_any_or_semantics
//   assert_any conjoins the OR of every clause: passes if any passes.
bool
tests_assert_any_or_semantics()
{
    bool ok = true;

    dt::test_summary sp = dt::make_suite().test("t")
                              .assert_any(false, []{ return false; }, true).run();
    ok = D_TB_CHECK(sp.passed == 1) && ok;

    dt::test_summary sf = dt::make_suite().test("t")
                              .assert_any(false, false, 1 == 2).run();
    ok = D_TB_CHECK(sf.failed == 1) && ok;

    return ok;
}

// tests_assert_all_any_vacuous
//   the empty folds: assert_all() is vacuously true, assert_any() vacuously
// false.
bool
tests_assert_all_any_vacuous()
{
    bool ok = true;

    dt::test_summary sa = dt::make_suite().test("t").assert_all().run();
    ok = D_TB_CHECK(sa.passed == 1 && sa.failed == 0) && ok;

    dt::test_summary so = dt::make_suite().test("t").assert_any().run();
    ok = D_TB_CHECK(so.failed == 1 && so.passed == 0) && ok;

    return ok;
}

// tests_assert_no_open_test_is_noop
//   with no test open, every assertion form binds nothing and allocates no
// callable row.
bool
tests_assert_no_open_test_is_noop()
{
    bool ok = true;

    dt::test_builder<> b;
    b.assert_(false);
    b.test_fn([]{ return false; });
    b.assert_all(false);
    b.assert_any(false);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.tests == 0)                    && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0)  && ok;
    ok = D_TB_CHECK(b.callables().empty())           && ok;

    return ok;
}

// tests_assert_pending_without_clause
//   a leaf never given a clause stays pending - a not-yet-implemented test,
// counted only in pending.
bool
tests_assert_pending_without_clause()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("unimpl");
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.tests == 1)                   && ok;
    ok = D_TB_CHECK(s.pending == 1)                 && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
