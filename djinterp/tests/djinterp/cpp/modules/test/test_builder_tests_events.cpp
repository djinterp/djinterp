/*******************************************************************************
* djinterp [cpp][test]                             test_builder_tests_events.cpp
*
*   The event lifecycle run() fires as it walks, and the subscription sugar.
* on_passed / on_failed / on_skipped / on_error fire once per matching leaf
* (on_error carrying the diagnostic); DOnStatusChange fires only when a
* deferred leaf actually resolves; DOnModuleStart fires once per interior
* node; DOnSessionStart / DOnSessionEnd bracket the walk, the latter carrying
* the pass / fail counts.  verdict::consume from a handler halts the rest of
* that event's word.  A handler that throws is caught and re-reported as
* DOnListenerThrew (named by the offending event) rather than aborting the
* run.  The generic on<> / fire<> surface and direct events() access work for a
* custom event, and subscriptions survive a clear().
*
*
* path:      /tests/djinterp/cpp/modules/test/test_builder_tests_events.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_builder_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_events_on_passed_failed_counts
//   on_passed / on_failed fire once for each passing / failing leaf.
bool tests_events_on_passed_failed_counts()
{
    bool ok = true;

    int passed_calls = 0;
    int failed_calls = 0;
    const dt::basic_test* lastp = nullptr;

    dt::test_builder<> b;
    b.on_passed(DNodeCounter{&passed_calls, &lastp});
    b.on_failed(DNodeCounter{&failed_calls, nullptr});
    b.test_module("m")
        .test("p1").assert_(true)
        .test("p2").assert_(true)
        .test("f1").assert_(false);
    dt::DTestSummary s = b.run();

    ok = D_TB_CHECK(s.passed == 2 && s.failed == 1) && ok;
    ok = D_TB_CHECK(passed_calls == 2)              && ok;
    ok = D_TB_CHECK(failed_calls == 1)              && ok;
    ok = D_TB_CHECK(lastp != nullptr)               && ok;

    return ok;
}

// tests_events_on_skipped_and_error
//   on_skipped fires for a skipped leaf; on_error fires for a leaf whose
// evaluation threw, carrying the captured diagnostic.
bool tests_events_on_skipped_and_error()
{
    bool ok = true;

    int         skip_calls = 0;
    int         err_calls  = 0;
    std::string err_msg;

    dt::test_builder<> b;
    b.on_skipped(DNodeCounter{&skip_calls, nullptr});
    b.on_error(DErrorRecorder{&err_calls, &err_msg});
    b.test_module("m")
        .test("s").skip()
        .test("e").assert_([]() -> bool { throw std::runtime_error("evaluate boom"); });
    dt::DTestSummary s = b.run();

    ok = D_TB_CHECK(s.skipped == 1 && s.errored == 1)    && ok;
    ok = D_TB_CHECK(skip_calls == 1)                     && ok;
    ok = D_TB_CHECK(err_calls == 1)                      && ok;
    ok = D_TB_CHECK(err_msg == std::string("evaluate boom")) && ok;

    return ok;
}

// tests_events_status_change_fired
//   DOnStatusChange fires exactly when a deferred leaf's status changes on
// evaluation - not for pending or inline leaves that never re-evaluate.
bool tests_events_status_change_fired()
{
    bool ok = true;

    int change_calls = 0;

    dt::test_builder<> b;
    b.on<dt::DOnStatusChange>(
        [&change_calls](const dt::basic_test*, dt::test_status, dt::test_status){
            ++change_calls;
        });
    b.test_module("m")
        .test("deferred").assert_(true)   // pending -> passed : one change
        .test("pending2");                // no clause : no change
    b.check(true);                        // inline : already resolved, no change
    b.run();

    ok = D_TB_CHECK(change_calls == 1) && ok;

    return ok;
}

// tests_events_module_start_per_interior
//   DOnModuleStart fires once for every interior node (module or block).
bool tests_events_module_start_per_interior()
{
    bool ok = true;

    int module_starts = 0;

    dt::test_builder<> b;
    b.on<dt::DOnModuleStart>([&module_starts](const dt::basic_test*){ ++module_starts; });
    b.test_module("m1")
        .test_block("b1").test("t1").assert_(true)
        .test_block("b2").test("t2").assert_(true);
    b.test_module("m2");
    b.run();

    // interior nodes: m1, b1, b2, m2
    ok = D_TB_CHECK(module_starts == 4) && ok;

    return ok;
}

// tests_events_session_end_counts
//   DOnSessionStart fires once up front; DOnSessionEnd carries the final
// pass / fail counts.
bool tests_events_session_end_counts()
{
    bool ok = true;

    int         start_calls = 0;
    std::size_t sp = 999;
    std::size_t sf = 999;

    dt::test_builder<> b;
    b.on<dt::DOnSessionStart>([&start_calls]{ ++start_calls; });
    b.on<dt::DOnSessionEnd>(DSessionEndRecorder{&sp, &sf});
    b.test_module("m").test("p").assert_(true).test("f").assert_(false);
    b.run();

    ok = D_TB_CHECK(start_calls == 1)  && ok;
    ok = D_TB_CHECK(sp == 1 && sf == 1) && ok;

    return ok;
}

// tests_events_verdict_consume_halts
//   a handler returning verdict::consume halts the rest of that event's word:
// a second handler bound after it does not run.
bool tests_events_verdict_consume_halts()
{
    bool ok = true;

    int first  = 0;
    int second = 0;

    dt::test_builder<> b;
    b.on<dt::DOnTestPassed>(DConsumingCounter{&first});        // returns consume
    b.on<dt::DOnTestPassed>(DNodeCounter{&second, nullptr});  // must be halted
    b.test_module("m").test("p").assert_(true);
    dt::DTestSummary s = b.run();

    ok = D_TB_CHECK(s.passed == 1) && ok;
    ok = D_TB_CHECK(first == 1)    && ok;
    ok = D_TB_CHECK(second == 0)   && ok;

    return ok;
}

// tests_events_listener_exception_captured
//   a throwing listener is caught and re-reported as DOnListenerThrew (named
// by the offending event); the run completes and the summary is correct.
bool tests_events_listener_exception_captured()
{
    bool ok = true;

    int         threw = 0;
    std::string ev;

    dt::test_builder<> b;
    b.on<dt::DOnTestPassed>(DThrowingHandler{});
    b.on<dt::DOnListenerThrew>(DThrewRecorder{&threw, &ev});
    b.test_module("m").test("p").assert_(true);
    dt::DTestSummary s = b.run();

    ok = D_TB_CHECK(s.passed == 1) && ok;                             // run not aborted
    ok = D_TB_CHECK(threw == 1)    && ok;                             // exception captured
    ok = D_TB_CHECK(ev == std::string(dt::DOnTestPassed::name())) && ok;  // and named

    return ok;
}

// tests_events_fire_custom_and_on
//   on<> subscribes and fire<> emits, for a custom event; fire is chainable
// and events() gives direct dispatcher access.
bool tests_events_fire_custom_and_on()
{
    bool ok = true;

    int probe_calls = 0;
    const dt::basic_test* seen = nullptr;

    dt::test_builder<> b;
    b.on<DOnCustomProbe>(DNodeCounter{&probe_calls, &seen});

    dt::basic_test node(dt::k_kind_test);
    b.fire<DOnCustomProbe>(&node);
    ok = D_TB_CHECK(probe_calls == 1) && ok;
    ok = D_TB_CHECK(seen == &node)    && ok;

    // fire returns *this (chainable)
    b.fire<DOnCustomProbe>(&node).fire<DOnCustomProbe>(&node);
    ok = D_TB_CHECK(probe_calls == 3) && ok;

    // events() gives direct dispatcher access; a handler bound through it fires too
    int direct = 0;
    b.events().bind<DOnCustomProbe>(DNodeCounter{&direct, nullptr});
    b.fire<DOnCustomProbe>(&node);
    ok = D_TB_CHECK(direct == 1)      && ok;
    ok = D_TB_CHECK(probe_calls == 4) && ok;

    return ok;
}

// tests_events_survive_clear
//   event subscriptions are deliberately left intact across clear(), so
// reporting wired up before a rebuild survives it.
bool tests_events_survive_clear()
{
    bool ok = true;

    int passed_calls = 0;

    dt::test_builder<> b;
    b.on_passed(DNodeCounter{&passed_calls, nullptr});
    b.test_module("m").test("p").assert_(true);
    b.run();
    ok = D_TB_CHECK(passed_calls == 1) && ok;

    b.clear();
    b.test_module("m2").test("p2").assert_(true);
    b.run();
    ok = D_TB_CHECK(passed_calls == 2) && ok;   // handler still bound after clear

    return ok;
}


NS_END  // testing
NS_END  // djinterp
