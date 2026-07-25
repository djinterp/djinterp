/******************************************************************************
* djinterp [test]                          test_builder_tests_inline_ops.cpp
*
*   The immediate and imperative surface.  check / expect add an ANONYMOUS leaf
* evaluated RIGHT NOW - folded into the tree's counts (and the inline_checks
* tally) without a callable row and without becoming the current test, so a
* following assert_ still binds onto the real open test; a throwing inline
* thunk records as error.  skip marks the current test skipped and tag writes
* metadata onto it, both no-ops with no test open.  add splices a body against
* the LIVE cursor (no save / restore, unlike the scoped forms), and add_if does
* so only when its condition - value or predicate, evaluated now - holds.
*
* path:      /tests/djinterp/test/test_builder/test_builder_tests_inline_ops.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_builder_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_inline_check_immediate
//   check evaluates its condition immediately and folds pass / fail into the
// counts and the inline_checks tally, without adding a named test.
bool
tests_inline_check_immediate()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m");
    b.check(true);
    b.check(false);
    b.check([]{ return true; });   // a predicate is run now too
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.inline_checks == 3) && ok;
    ok = D_TB_CHECK(s.passed == 2)        && ok;   // the two trues
    ok = D_TB_CHECK(s.failed == 1)        && ok;   // the one false
    ok = D_TB_CHECK(s.tests == 0)         && ok;   // inline checks are not named tests

    return ok;
}

// tests_inline_expect_named
//   expect is a named inline check: the outcome is recorded under the name.
bool
tests_inline_expect_named()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m");
    b.expect("named check", true);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.inline_checks == 1) && ok;
    ok = D_TB_CHECK(s.passed == 1)        && ok;

    auto is_named = [](const dt::basic_test& n){
        return n.metadata().get("name") == std::string("named check");
    };
    ok = D_TB_CHECK(b.count_if(is_named) == std::size_t(1)) && ok;

    return ok;
}

// tests_inline_check_not_current_test
//   check does NOT become the current test: a later assert_ still binds onto
// the real open test, which therefore does not stay pending.
bool
tests_inline_check_not_current_test()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test("real");        // open "real" (pending, current)
    b.check(true);         // inline; must NOT capture the cursor
    b.assert_(true);       // binds onto "real"
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.inline_checks == 1) && ok;
    ok = D_TB_CHECK(s.tests == 1)         && ok;
    ok = D_TB_CHECK(s.passed == 2)        && ok;   // "real" (bound) + inline check
    ok = D_TB_CHECK(s.pending == 0)       && ok;   // "real" got its clause

    return ok;
}

// tests_inline_check_throw_is_error
//   a throwing inline thunk is caught and recorded as error.
bool
tests_inline_check_throw_is_error()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m");
    b.check([]() -> bool { throw std::runtime_error("inline boom"); });
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.inline_checks == 1)            && ok;
    ok = D_TB_CHECK(s.errored == 1)                  && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0)  && ok;

    return ok;
}

// tests_nodeops_skip
//   skip marks the current test skipped, and (with no clause) it stays
// skipped through run().
bool
tests_nodeops_skip()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("s").skip();
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.tests == 1)    && ok;
    ok = D_TB_CHECK(s.skipped == 1)  && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0 && s.pending == 0) && ok;

    return ok;
}

// tests_nodeops_tag
//   tag writes key/value metadata onto the current test.
bool
tests_nodeops_tag()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("t")
        .tag("owner", "alice")
        .tag("ticket", "T-42")
        .assert_(true);
    dt::test_summary s = b.run();
    ok = D_TB_CHECK(s.passed == 1) && ok;

    auto has_owner  = [](const dt::basic_test& n){ return n.metadata().get("owner")  == std::string("alice"); };
    auto has_ticket = [](const dt::basic_test& n){ return n.metadata().get("ticket") == std::string("T-42");  };
    ok = D_TB_CHECK(b.count_if(has_owner)  == std::size_t(1)) && ok;
    ok = D_TB_CHECK(b.count_if(has_ticket) == std::size_t(1)) && ok;

    return ok;
}

// tests_nodeops_skip_tag_no_test_noop
//   skip / tag with no test open are safe no-ops.
bool
tests_nodeops_skip_tag_no_test_noop()
{
    bool ok = true;

    dt::test_builder<> b;
    b.skip();
    b.tag("k", "v");
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.tests == 0)   && ok;
    ok = D_TB_CHECK(s.skipped == 0) && ok;

    auto has_k = [](const dt::basic_test& n){ return n.metadata().contains("k"); };
    ok = D_TB_CHECK(b.count_if(has_k) == std::size_t(0)) && ok;

    return ok;
}

// tests_compose_add_runs_body
//   add runs a body against the live cursor and does NOT restore it, so the
// scope the body opens persists.  An empty body is a no-op.
bool
tests_compose_add_runs_body()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m");
    b.add([](dt::test_builder<>& s){
        s.test_block("added");
        s.test("t").assert_(true);
    });
    dt::test_summary s = b.run();
    ok = D_TB_CHECK(s.modules == 1) && ok;
    ok = D_TB_CHECK(s.blocks == 1)  && ok;
    ok = D_TB_CHECK(s.tests == 1)   && ok;
    ok = D_TB_CHECK(s.passed == 1)  && ok;

    // the cursor persisted at "added" -> a further test nests there (no new block)
    b.test("t2").assert_(true);
    dt::test_summary s2 = b.run();
    ok = D_TB_CHECK(s2.tests == 2)  && ok;
    ok = D_TB_CHECK(s2.blocks == 1) && ok;

    // an empty body is a no-op
    dt::test_builder<>::body_type empty;
    dt::test_builder<> b3;
    b3.add(empty);
    dt::test_summary s3 = b3.run();
    ok = D_TB_CHECK(s3.tests == 0 && s3.modules == 0) && ok;

    return ok;
}

// tests_compose_add_if_conditional
//   add_if runs the body only when its condition holds (value or predicate,
// evaluated now); an empty body is a no-op even when the condition holds.
bool
tests_compose_add_if_conditional()
{
    bool ok = true;

    auto add_a_test = [](dt::test_builder<>& s){ s.test("t").assert_(true); };

    // value true -> runs
    dt::test_builder<> bt;
    bt.test_module("m");
    bt.add_if(true, add_a_test);
    ok = D_TB_CHECK(bt.run().tests == 1) && ok;

    // value false -> skipped
    dt::test_builder<> bf;
    bf.test_module("m");
    bf.add_if(false, add_a_test);
    ok = D_TB_CHECK(bf.run().tests == 0) && ok;

    // predicate condition -> evaluated now
    bool want = true;
    dt::test_builder<> bp;
    bp.test_module("m");
    bp.add_if([&want]{ return want; }, add_a_test);
    ok = D_TB_CHECK(bp.run().tests == 1) && ok;

    // empty body -> no-op even when the condition is true
    dt::test_builder<>::body_type empty;
    dt::test_builder<> be;
    be.add_if(true, empty);
    ok = D_TB_CHECK(be.run().tests == 0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
