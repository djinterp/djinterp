/*******************************************************************************
* djinterp [cpp][test]                           test_race_tests_consistency.cpp
*
*   Section TU: DConsistencyCheckReport and DConsistencyCheck - the
* structural verifier over a DLinearizationLog.
*
*   The checker verifies two properties: interval validity (completed_at >=
* invoked_at) and per-thread program order (an event may not begin before the
* previous event on the same thread completed).  Both violations are
* constructed here deterministically rather than raced into existence.
*
*
* path:      /tests/djinterp/cpp/modules/test/sync/test_race_tests_consistency.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <chrono>
#include <string>
#include <thread>
// djinterp
#include "test_race_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace {

// hr_settle
//   local helper: a pause long enough that the steady clock is guaranteed to
// advance, so a constructed overlap really is an overlap.
inline
void hr_settle()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

}  // anonymous namespace


bool tests_rconsistency_report_defaults()
{
    dt::DConsistencyCheckReport rep;

    D_HR_CHECK(rep.events_examined == 0u);
    D_HR_CHECK(rep.ordering_violations == 0u);
    D_HR_CHECK(rep.interval_violations == 0u);
    D_HR_CHECK(rep.consistent);

    return true;
}

bool tests_rconsistency_report_to_test_object()
{
    dt::DConsistencyCheckReport good;
    dt::basic_test a = good.to_test_object(hr_type_id());

    D_HR_CHECK(a.result());
    D_HR_CHECK(a.type_id() == hr_type_id());
    D_HR_CHECK(a.metadata().get("name") == std::string("consistency"));

    dt::DConsistencyCheckReport bad;
    bad.consistent = false;
    dt::basic_test b = bad.to_test_object(hr_type_id(), "queue_lin");

    D_HR_CHECK(!b.result());
    D_HR_CHECK(b.metadata().get("name") == std::string("queue_lin"));
    D_HR_CHECK(a.metadata().get("message") != b.metadata().get("message"));

    return true;
}

bool tests_rconsistency_empty_log()
{
    dt::DLinearizationLog  log;
    dt::DConsistencyCheck  checker;

    const dt::DConsistencyCheckReport rep = checker.check(log);

    D_HR_CHECK(rep.events_examined == 0u);
    D_HR_CHECK(rep.consistent);
    D_HR_CHECK(rep.ordering_violations == 0u);
    D_HR_CHECK(rep.interval_violations == 0u);

    return true;
}

bool tests_rconsistency_well_formed_log()
{
    // instantaneous events on one thread, recorded in order: intervals are
    // degenerate-but-valid and program order is respected
    dt::DLinearizationLog log;
    hr_fill_log(log, 0, 6);

    dt::DConsistencyCheck checker;
    const dt::DConsistencyCheckReport rep = checker.check(log);

    D_HR_CHECK(rep.events_examined == 6u);
    D_HR_CHECK(rep.interval_violations == 0u);
    D_HR_CHECK(rep.ordering_violations == 0u);
    D_HR_CHECK(rep.consistent);

    return true;
}

bool tests_rconsistency_counts_events()
{
    dt::DLinearizationLog log;
    hr_fill_log(log, 0, 3);
    hr_fill_log(log, 1, 4);

    dt::DConsistencyCheck checker;
    const dt::DConsistencyCheckReport rep = checker.check(log);

    D_HR_CHECK(rep.events_examined == 7u);
    D_HR_CHECK(rep.consistent);

    return true;
}

bool tests_rconsistency_interval_violation()
{
    // an event that is begun but never ended keeps the epoch as its
    // completion stamp, so it completes "before" it was invoked
    dt::DLinearizationLog log;
    log.begin_event(0, std::string("never_closed"));

    dt::DConsistencyCheck checker;
    const dt::DConsistencyCheckReport rep = checker.check(log);

    D_HR_CHECK(rep.events_examined == 1u);
    D_HR_CHECK(rep.interval_violations == 1u);
    D_HR_CHECK(!rep.consistent);

    return true;
}

bool tests_rconsistency_ordering_violation()
{
    // two operations on the SAME thread deliberately overlap: the second is
    // begun while the first is still open
    dt::DLinearizationLog log;

    dt::DLinearizationLog::DEventHandle first =
        log.begin_event(0, std::string("outer"));

    hr_settle();

    dt::DLinearizationLog::DEventHandle second =
        log.begin_event(0, std::string("inner"));

    hr_settle();

    log.end_event(first,  std::string("ok"));   // closes AFTER inner began
    log.end_event(second, std::string("ok"));

    dt::DConsistencyCheck checker;
    const dt::DConsistencyCheckReport rep = checker.check(log);

    D_HR_CHECK(rep.events_examined == 2u);
    D_HR_CHECK(rep.interval_violations == 0u);   // both intervals run forward
    D_HR_CHECK(rep.ordering_violations == 1u);   // but they overlap
    D_HR_CHECK(!rep.consistent);

    return true;
}

bool tests_rconsistency_threads_independent()
{
    // the same overlap across DIFFERENT threads is legal: program order is a
    // per-thread property
    dt::DLinearizationLog log;

    dt::DLinearizationLog::DEventHandle a =
        log.begin_event(0, std::string("thread_a"));

    hr_settle();

    dt::DLinearizationLog::DEventHandle b =
        log.begin_event(1, std::string("thread_b"));

    hr_settle();

    log.end_event(a, std::string("ok"));
    log.end_event(b, std::string("ok"));

    dt::DConsistencyCheck checker;
    const dt::DConsistencyCheckReport rep = checker.check(log);

    D_HR_CHECK(rep.events_examined == 2u);
    D_HR_CHECK(rep.ordering_violations == 0u);
    D_HR_CHECK(rep.interval_violations == 0u);
    D_HR_CHECK(rep.consistent);

    return true;
}


NS_END  // testing
NS_END  // djinterp
