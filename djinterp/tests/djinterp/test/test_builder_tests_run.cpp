/******************************************************************************
* djinterp [test]                                 test_builder_tests_run.cpp
*
*   The terminal and the read surface.  run() walks the forest, evaluates each
* deferred leaf, and summarize() folds the result into a tally: passed /
* failed / skipped / errored come from the evaluated nodes, while pending is
* corrected to LEAF-ONLY by subtracting the perpetually-pending structural
* interiors (the modules, the blocks, and the conjunctive root).  A thunk that
* throws is caught in evaluate as `error` with its diagnostic stashed under the
* "error" metadata key (a std::exception's what(), or "unknown exception").
* Re-running is stable for pure thunks and picks up newly added tests.  The
* read-only combinators each / count_if / fold traverse every node (the
* conjunctive root included); clear() empties the forest, callables, cursor,
* and bookkeeping and re-installs the default kinds; and tree() / callables()
* expose the live backing stores.
*
* path:      /tests/djinterp/test/test_builder/test_builder_tests_run.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_builder_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_run_summary_tally
//   a mixed forest tallies passed / failed / skipped / pending across named
// tests and inline checks alike.
bool
tests_run_summary_tally()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m")
        .test("p").assert_(true)
        .test("f").assert_(false)
        .test("s").skip()
        .test("pend");            // no clause -> pending
    b.check(true);                // inline pass
    b.check(false);               // inline fail
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 1)       && ok;
    ok = D_TB_CHECK(s.tests == 4)         && ok;
    ok = D_TB_CHECK(s.inline_checks == 2) && ok;
    ok = D_TB_CHECK(s.passed == 2)        && ok;   // p + inline true
    ok = D_TB_CHECK(s.failed == 2)        && ok;   // f + inline false
    ok = D_TB_CHECK(s.skipped == 1)       && ok;   // s
    ok = D_TB_CHECK(s.pending == 1)       && ok;   // pend
    ok = D_TB_CHECK(s.errored == 0)       && ok;

    return ok;
}

// tests_run_pending_is_leaf_only
//   the perpetually-pending structural interiors (modules, blocks, root) are
// subtracted from pending, so it counts only unimplemented leaf tests.
bool
tests_run_pending_is_leaf_only()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m1").test_block("b1");                              // interiors only
    b.test_module("m2").test_block("b2").test("impl").assert_(true);  // one implemented test
    b.test_module("m3");                                              // empty module
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 3) && ok;
    ok = D_TB_CHECK(s.blocks == 2)  && ok;
    ok = D_TB_CHECK(s.tests == 1)   && ok;
    ok = D_TB_CHECK(s.passed == 1)  && ok;
    ok = D_TB_CHECK(s.pending == 0) && ok;   // every pending node here is structural

    // add one unimplemented leaf: now pending counts exactly it
    b.test_module("m4").test("todo");
    dt::test_summary s2 = b.run();
    ok = D_TB_CHECK(s2.pending == 1) && ok;

    return ok;
}

// tests_run_evaluate_throw_error_message
//   a throwing deferred thunk records as error with its diagnostic captured;
// a non-std::exception throw records "unknown exception".
bool
tests_run_evaluate_throw_error_message()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("boom")
        .assert_([]() -> bool { throw std::runtime_error("kaboom"); });
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.errored == 1)                 && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0) && ok;

    auto has_err = [](const dt::basic_test& n){ return n.metadata().get("error") == std::string("kaboom"); };
    ok = D_TB_CHECK(b.count_if(has_err) == std::size_t(1)) && ok;

    // a non-std::exception throw is captured as "unknown exception"
    dt::test_builder<> b2;
    b2.test_module("m").test("boom2")
        .assert_([]() -> bool { throw 42; });
    dt::test_summary s2 = b2.run();
    ok = D_TB_CHECK(s2.errored == 1) && ok;

    auto has_unknown = [](const dt::basic_test& n){
        return n.metadata().get("error") == std::string("unknown exception");
    };
    ok = D_TB_CHECK(b2.count_if(has_unknown) == std::size_t(1)) && ok;

    return ok;
}

// tests_run_idempotent_and_incremental
//   re-running is stable for pure thunks and reflects tests added between
// runs.
bool
tests_run_idempotent_and_incremental()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("t1").assert_(true);

    dt::test_summary s1 = b.run();
    ok = D_TB_CHECK(s1.tests == 1 && s1.passed == 1) && ok;

    dt::test_summary s1b = b.run();                       // same again
    ok = D_TB_CHECK(s1b.tests == 1 && s1b.passed == 1) && ok;

    b.test("t2").assert_(false);                          // add a failing test
    dt::test_summary s2 = b.run();
    ok = D_TB_CHECK(s2.tests == 2)                   && ok;
    ok = D_TB_CHECK(s2.passed == 1 && s2.failed == 1) && ok;

    return ok;
}

// tests_traversal_each_visits_all
//   each visits every node (the conjunctive root included), can mutate, and
// returns *this for chaining.
bool
tests_traversal_each_visits_all()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test_block("bl").test("t").assert_(true);

    // visits root + m + bl + t
    int visited = 0;
    b.each([&visited](dt::basic_test&){ ++visited; });
    ok = D_TB_CHECK(visited == 4) && ok;

    // mutating visitor
    b.each([](dt::basic_test& n){ n.metadata().set("visited", "yes"); });
    auto is_visited = [](const dt::basic_test& n){ return n.metadata().get("visited") == std::string("yes"); };
    ok = D_TB_CHECK(b.count_if(is_visited) == std::size_t(4)) && ok;

    // chainable: two passes over four nodes
    int again = 0;
    b.each([&again](dt::basic_test&){ ++again; }).each([&again](dt::basic_test&){ ++again; });
    ok = D_TB_CHECK(again == 8) && ok;

    return ok;
}

// tests_traversal_count_if
//   count_if counts nodes matching a predicate, over the whole forest.
bool
tests_traversal_count_if()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("p").assert_(true).test("f").assert_(false);
    b.run();

    auto is_test   = [](const dt::basic_test& n){ return n.type_id() == dt::k_kind_test; };
    auto is_passed = [](const dt::basic_test& n){ return n.passed(); };
    auto always    = [](const dt::basic_test&){ return true;  };
    auto never     = [](const dt::basic_test&){ return false; };

    ok = D_TB_CHECK(b.count_if(is_test)   == std::size_t(2)) && ok;   // p, f
    ok = D_TB_CHECK(b.count_if(is_passed) == std::size_t(1)) && ok;   // p
    ok = D_TB_CHECK(b.count_if(always)    == std::size_t(4)) && ok;   // root, m, p, f
    ok = D_TB_CHECK(b.count_if(never)     == std::size_t(0)) && ok;

    return ok;
}

// tests_traversal_fold
//   fold left-folds an accumulator over every node.
bool
tests_traversal_fold()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m")
        .test("p1").assert_(true)
        .test("p2").assert_(true)
        .test("f").assert_(false);
    b.run();

    std::size_t passed = b.fold(std::size_t(0),
        [](std::size_t acc, const dt::basic_test& n){
            return acc + (n.passed() ? std::size_t(1) : std::size_t(0));
        });
    ok = D_TB_CHECK(passed == std::size_t(2)) && ok;

    // sum of type ids: root(0) + m(1000) + p1(1002) + p2(1002) + f(1002)
    long sum = b.fold(0L,
        [](long acc, const dt::basic_test& n){ return acc + static_cast<long>(n.type_id()); });
    ok = D_TB_CHECK(sum == 4006L) && ok;

    return ok;
}

// tests_clear_resets_state
//   clear empties the forest, the callables, the cursor, and the bookkeeping,
// and re-installs the default kinds so a rebuild nests normally.
bool
tests_clear_resets_state()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test_block("bl").test("t").assert_(true);
    b.run();
    ok = D_TB_CHECK(!b.callables().empty()) && ok;

    b.clear();
    dt::test_summary s = b.run();
    ok = D_TB_CHECK(s.modules == 0 && s.blocks == 0 &&
                    s.tests == 0 && s.inline_checks == 0)     && ok;
    ok = D_TB_CHECK(s.passed == 0 && s.failed == 0)          && ok;
    ok = D_TB_CHECK(b.callables().empty())                   && ok;
    ok = D_TB_CHECK(b.tree().size() == std::size_t(0))       && ok;

    // rebuild: rank nesting still works, so the default kinds were re-installed
    b.test_module("m2").test_block("bl2").test("t2").assert_(true);
    dt::test_summary s2 = b.run();
    ok = D_TB_CHECK(s2.modules == 1 && s2.blocks == 1 && s2.tests == 1) && ok;
    ok = D_TB_CHECK(s2.passed == 1) && ok;

    return ok;
}

// tests_accessors_tree_and_callables
//   tree() and callables() (mutable and const) expose the live backing
// stores.
bool
tests_accessors_tree_and_callables()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test("t1").assert_(true).test("t2").assert_(true);

    ok = D_TB_CHECK(b.callables().size() == std::size_t(2)) && ok;   // one row per test
    ok = D_TB_CHECK(b.tree().size() == std::size_t(4))      && ok;   // root + m + t1 + t2

    const dt::test_builder<>& cb = b;
    ok = D_TB_CHECK(cb.callables().size() == std::size_t(2)) && ok;
    ok = D_TB_CHECK(cb.tree().size() == std::size_t(4))      && ok;
    ok = D_TB_CHECK(b.tree().root() != nullptr)              && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
