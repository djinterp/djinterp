/*******************************************************************************
* djinterp [tests]                                     test_race_tests_probe.cpp
*
*   Section TU: race_probe_report and race_probe.
*
*   The probe's job is to detect races, which are probabilistic.  These tests
* therefore never require that a race IS observed; they pin the harness's own
* contract - a correct workload yields no evidence, a deliberately false
* invariant yields evidence on every repetition, and the counters aggregate
* exactly.
*
*
* path:      /tests/djinterp/test/sync/test_race_tests_probe.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <stdexcept>
#include <string>
// djinterp
#include "test_race_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_rprobe_report_defaults()
{
    dt::race_probe_report rep;

    D_HR_CHECK(rep.runs_performed == 0u);
    D_HR_CHECK(rep.runs_with_race_evidence == 0u);
    D_HR_CHECK(rep.total_thread_iterations == 0u);
    D_HR_CHECK(!rep.any_evidence);
    D_HR_CHECK(rep.success());          // no evidence == pass

    return true;
}

bool tests_rprobe_report_probability_zero_runs()
{
    dt::race_probe_report rep;
    rep.runs_with_race_evidence = 3;    // nonsensical without runs

    // guards against divide-by-zero: no runs means no estimate
    D_HR_CHECK(rep.probability_estimate() == 0.0);

    return true;
}

bool tests_rprobe_report_probability_estimate()
{
    dt::race_probe_report rep;
    rep.runs_performed          = 8;
    rep.runs_with_race_evidence = 2;

    const double p = rep.probability_estimate();

    D_HR_CHECK(p > 0.2);
    D_HR_CHECK(p < 0.3);                // 2/8 == 0.25

    return true;
}

bool tests_rprobe_report_success_predicate()
{
    dt::race_probe_report rep;
    D_HR_CHECK(rep.success());

    rep.any_evidence = true;
    D_HR_CHECK(!rep.success());

    return true;
}

bool tests_rprobe_report_to_test_object()
{
    dt::race_probe_report clean;
    dt::basic_test a = clean.to_test_object(hr_type_id());

    D_HR_CHECK(a.result());
    D_HR_CHECK(a.type_id() == hr_type_id());
    D_HR_CHECK(a.metadata().contains("name"));

    dt::race_probe_report dirty;
    dirty.any_evidence = true;
    dt::basic_test b = dirty.to_test_object(hr_type_id(), "my_probe");

    D_HR_CHECK(!b.result());
    D_HR_CHECK(b.metadata().get("name") == std::string("my_probe"));

    return true;
}

bool tests_rprobe_run_zero_threads()
{
    hr_shared shared;
    dt::race_probe probe;
    probe.set_thread_count(0);
    probe.set_iterations_per_thread(5);
    probe.set_runs(2);
    probe.set_operation(
        dt::race_probe::operation_fn(
            [&shared](std::size_t, std::size_t)
            {
                shared.counter.fetch_add(1, std::memory_order_relaxed);
            }));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 0u);
    D_HR_CHECK(shared.counter.load(std::memory_order_relaxed) == 0);

    return true;
}

bool tests_rprobe_run_zero_iterations()
{
    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(0);
    probe.set_runs(2);
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 0u);

    return true;
}

bool tests_rprobe_run_zero_runs()
{
    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(5);
    probe.set_runs(0);
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 0u);

    return true;
}

bool tests_rprobe_run_without_operation()
{
    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(5);
    probe.set_runs(2);
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));
    // no operation

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 0u);

    return true;
}

bool tests_rprobe_run_without_invariant()
{
    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(5);
    probe.set_runs(2);
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    // no invariant

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 0u);

    return true;
}

bool tests_rprobe_run_clean_workload()
{
    // a properly synchronised workload: the invariant holds every time, so
    // the probe must report no evidence.
    hr_shared shared;
    const std::size_t threads = 4;
    const std::size_t iters   = 50;
    const std::size_t runs    = 3;

    dt::race_probe probe;
    probe.set_thread_count(threads);
    probe.set_iterations_per_thread(iters);
    probe.set_runs(runs);
    probe.set_setup(
        dt::race_probe::setup_fn(
            [&shared]()
            {
                shared.reset();
            }));
    probe.set_operation(
        dt::race_probe::operation_fn(
            [&shared](std::size_t, std::size_t)
            {
                shared.counter.fetch_add(1, std::memory_order_relaxed);
            }));
    probe.set_invariant(
        dt::race_probe::invariant_fn(
            [&shared, threads, iters]()
            {
                return shared.counter.load(std::memory_order_relaxed) ==
                       static_cast<long>(threads * iters);
            }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == runs);
    D_HR_CHECK(rep.runs_with_race_evidence == 0u);
    D_HR_CHECK(!rep.any_evidence);
    D_HR_CHECK(rep.success());
    D_HR_CHECK(rep.probability_estimate() == 0.0);

    return true;
}

bool tests_rprobe_run_counts_runs()
{
    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(3);
    probe.set_runs(5);
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 5u);

    return true;
}

bool tests_rprobe_run_total_iterations()
{
    const std::size_t threads = 3;
    const std::size_t iters   = 7;
    const std::size_t runs    = 4;

    std::atomic<long> calls(0);

    dt::race_probe probe;
    probe.set_thread_count(threads);
    probe.set_iterations_per_thread(iters);
    probe.set_runs(runs);
    probe.set_operation(
        dt::race_probe::operation_fn(
            [&calls](std::size_t, std::size_t)
            {
                calls.fetch_add(1, std::memory_order_relaxed);
            }));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));

    const dt::race_probe_report rep = probe.run();

    // 4 runs x 3 threads x 7 iterations
    D_HR_CHECK(rep.total_thread_iterations == 84u);
    D_HR_CHECK(calls.load(std::memory_order_relaxed) == 84);

    return true;
}

bool tests_rprobe_run_setup_per_run()
{
    std::atomic<int> setups(0);

    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(2);
    probe.set_runs(5);
    probe.set_setup(
        dt::race_probe::setup_fn(
            [&setups]()
            {
                setups.fetch_add(1, std::memory_order_acq_rel);
            }));
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return true; }));

    probe.run();

    // setup runs once before each repetition, not once per thread
    D_HR_CHECK(setups.load(std::memory_order_acquire) == 5);

    return true;
}

bool tests_rprobe_run_failing_invariant()
{
    // a deliberately false invariant: every run must be counted as evidence
    const std::size_t runs = 4;

    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(2);
    probe.set_runs(runs);
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    probe.set_invariant(
        dt::race_probe::invariant_fn([]() { return false; }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == runs);
    D_HR_CHECK(rep.runs_with_race_evidence == runs);
    D_HR_CHECK(rep.any_evidence);
    D_HR_CHECK(!rep.success());
    D_HR_CHECK(rep.probability_estimate() == 1.0);

    return true;
}

bool tests_rprobe_run_throwing_operation()
{
    // a thread that throws is itself treated as evidence, and the invariant
    // is skipped for that run
    std::atomic<int> invariant_calls(0);

    dt::race_probe probe;
    probe.set_thread_count(2);
    probe.set_iterations_per_thread(2);
    probe.set_runs(3);
    probe.set_operation(
        dt::race_probe::operation_fn(
            [](std::size_t, std::size_t)
            {
                throw std::runtime_error("op threw");
            }));
    probe.set_invariant(
        dt::race_probe::invariant_fn(
            [&invariant_calls]()
            {
                invariant_calls.fetch_add(1, std::memory_order_acq_rel);
                return true;
            }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 3u);
    D_HR_CHECK(rep.runs_with_race_evidence == 3u);
    D_HR_CHECK(rep.any_evidence);
    // the invariant is never reached on a failed run
    D_HR_CHECK(invariant_calls.load(std::memory_order_acquire) == 0);

    return true;
}

bool tests_rprobe_run_throwing_invariant()
{
    // an invariant that throws is caught and treated as a failed invariant
    dt::race_probe probe;
    probe.set_thread_count(1);
    probe.set_iterations_per_thread(1);
    probe.set_runs(2);
    probe.set_operation(
        dt::race_probe::operation_fn([](std::size_t, std::size_t) {}));
    probe.set_invariant(
        dt::race_probe::invariant_fn(
            []() -> bool
            {
                throw std::runtime_error("invariant threw");
            }));

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 2u);
    D_HR_CHECK(rep.runs_with_race_evidence == 2u);
    D_HR_CHECK(!rep.success());

    return true;
}


NS_END  // testing
NS_END  // djinterp
