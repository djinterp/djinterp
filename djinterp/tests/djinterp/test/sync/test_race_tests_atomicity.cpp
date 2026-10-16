/*******************************************************************************
* djinterp [tests]                                 test_race_tests_atomicity.cpp
*
*   Section TU: atomicity_observer_report and atomicity_observer<T>.
*
*   The observer counts snapshots and classifies each as allowed or
* forbidden.  The tests drive it with deterministic readers and classifiers so
* the counts are exact rather than probabilistic.
*
*
* path:      /tests/djinterp/test/sync/test_race_tests_atomicity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <string>
// djinterp
#include "test_race_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_ratomicity_report_defaults()
{
    dt::atomicity_observer_report rep;

    D_HR_CHECK(rep.total_observations == 0u);
    D_HR_CHECK(rep.allowed_state_observations == 0u);
    D_HR_CHECK(rep.forbidden_state_observations == 0u);
    D_HR_CHECK(rep.success());          // nothing forbidden seen

    return true;
}

bool tests_ratomicity_report_success_predicate()
{
    dt::atomicity_observer_report rep;
    rep.total_observations         = 100;
    rep.allowed_state_observations = 100;
    D_HR_CHECK(rep.success());

    rep.forbidden_state_observations = 1;   // a single sighting is fatal
    D_HR_CHECK(!rep.success());

    return true;
}

bool tests_ratomicity_report_to_test_object()
{
    dt::atomicity_observer_report clean;
    dt::basic_test a = clean.to_test_object(hr_type_id());

    D_HR_CHECK(a.result());
    D_HR_CHECK(a.type_id() == hr_type_id());
    D_HR_CHECK(a.metadata().get("name") == std::string("atomicity"));

    dt::atomicity_observer_report dirty;
    dirty.forbidden_state_observations = 2;
    dt::basic_test b = dirty.to_test_object(hr_type_id(), "torn_read");

    D_HR_CHECK(!b.result());
    D_HR_CHECK(b.metadata().get("name") == std::string("torn_read"));
    // the two verdicts carry distinct messages
    D_HR_CHECK(a.metadata().get("message") != b.metadata().get("message"));

    return true;
}

bool tests_ratomicity_run_zero_observers()
{
    dt::atomicity_observer<int> obs;
    obs.set_observer_count(0);
    obs.set_observation_count(10);
    obs.set_reader([]() { return 1; });
    obs.set_allowed([](const int&) { return true; });

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 0u);

    return true;
}

bool tests_ratomicity_run_zero_observations()
{
    dt::atomicity_observer<int> obs;
    obs.set_observer_count(2);
    obs.set_observation_count(0);
    obs.set_reader([]() { return 1; });
    obs.set_allowed([](const int&) { return true; });

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 0u);

    return true;
}

bool tests_ratomicity_run_without_reader()
{
    dt::atomicity_observer<int> obs;
    obs.set_observer_count(2);
    obs.set_observation_count(10);
    obs.set_allowed([](const int&) { return true; });
    // no reader

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 0u);

    return true;
}

bool tests_ratomicity_run_without_allowed()
{
    dt::atomicity_observer<int> obs;
    obs.set_observer_count(2);
    obs.set_observation_count(10);
    obs.set_reader([]() { return 1; });
    // no classifier

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 0u);

    return true;
}

bool tests_ratomicity_run_all_allowed()
{
    dt::atomicity_observer<int> obs;
    obs.set_observer_count(3);
    obs.set_observation_count(40);
    obs.set_reader([]() { return 7; });
    obs.set_allowed([](const int& _v) { return _v == 7; });

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 120u);              // 3 x 40
    D_HR_CHECK(rep.allowed_state_observations == 120u);
    D_HR_CHECK(rep.forbidden_state_observations == 0u);
    D_HR_CHECK(rep.success());

    return true;
}

bool tests_ratomicity_run_all_forbidden()
{
    dt::atomicity_observer<int> obs;
    obs.set_observer_count(2);
    obs.set_observation_count(25);
    obs.set_reader([]() { return 1; });
    obs.set_allowed([](const int&) { return false; });

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 50u);
    D_HR_CHECK(rep.allowed_state_observations == 0u);
    D_HR_CHECK(rep.forbidden_state_observations == 50u);
    D_HR_CHECK(!rep.success());

    return true;
}

bool tests_ratomicity_run_counts_partition()
{
    // classify by parity of a per-observation counter: allowed + forbidden
    // must always add up to the total, whatever the split turns out to be.
    std::atomic<long> tick(0);

    dt::atomicity_observer<long> obs;
    obs.set_observer_count(2);
    obs.set_observation_count(30);
    obs.set_reader(
        [&tick]()
        {
            return tick.fetch_add(1, std::memory_order_relaxed);
        });
    obs.set_allowed(
        [](const long& _v)
        {
            return (_v % 2) == 0;
        });

    const dt::atomicity_observer_report rep = obs.run_observers();

    D_HR_CHECK(rep.total_observations == 60u);
    D_HR_CHECK(rep.allowed_state_observations +
               rep.forbidden_state_observations == rep.total_observations);
    // both classes were exercised
    D_HR_CHECK(rep.allowed_state_observations > 0u);
    D_HR_CHECK(rep.forbidden_state_observations > 0u);

    return true;
}

bool tests_ratomicity_run_reader_invoked()
{
    std::atomic<long> reads(0);

    dt::atomicity_observer<int> obs;
    obs.set_observer_count(4);
    obs.set_observation_count(15);
    obs.set_reader(
        [&reads]()
        {
            reads.fetch_add(1, std::memory_order_relaxed);
            return 0;
        });
    obs.set_allowed([](const int&) { return true; });

    const dt::atomicity_observer_report rep = obs.run_observers();

    // one reader call per observation per observer
    D_HR_CHECK(reads.load(std::memory_order_relaxed) == 60);
    D_HR_CHECK(rep.total_observations == 60u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
