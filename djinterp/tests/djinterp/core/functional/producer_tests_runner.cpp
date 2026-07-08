/******************************************************************************
* djinterp [test]                                      producer_tests_runner.cpp
*
*   Module wiring and standalone entry point for the producer.hpp suite,
* converted from the legacy self-contained test_registry harness to the DTest
* run_session model so it reads and reports exactly like every other functional
* module (view, consumer, compose, ...).
*
*   producer_tests.hpp now declares each section with the framework's leaf
* signature `void(test::test_handler&)`; the section .cpp files record their
* checks through D_TEST_CHECK, the thin bridge over test::record_assertion that
* replaced D_TESTING_CHECK (it stringifies the checked expression as the
* assertion label). This translation unit binds the two run_session hooks the
* header declares: producer_module_info (identity) and producer_module_run_all
* (the section schedule).
*
*   The inline main() below runs the suite standalone in place. The relocated
* per-module runner at .../functional/producer/producer_test_runner.cpp is the
* canonical entry point under the config/testing tree; when building through it,
* guard or drop this main() so the two do not collide. module_info + run_all
* stay here with the suite.
*
* path:      /inc/functional/producer_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "./producer_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// producer_module_info
//   constant: per-module identity, reused as the single-module session
// descriptor below.
const test::test_module_info producer_module_info =
{
    "producer",
    "djinterp / functional / producer",
    "/inc/djinterp/core/functional/producer.hpp",
    "producer suite",
    "pull-based producers: the producer_step type with make_step / no_step, the "
    "source generators (iterate / unfold / range / iota / repeat / repeat_n / "
    "cycle / generate / empty / single / from_container), the wrapping adapters "
    "(take_n / drop_n / concat / interleave / transform / filter) with the CRTP "
    "collect terminal, the free collect / for_each / fold terminals, and the "
    "detection traits / concepts"
};


/*
producer_module_run_all
  Schedules every producer.hpp test section against the shared engine, in the
  same order the sections appear in producer.hpp's table of contents.
*/
void
producer_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    // producer_step<T>, make_step, no_step
    _ctx.run("step",        &test_producer_step);

    // source generators
    _ctx.run("generators",  &test_producer_generators);

    // wrapping adapters + CRTP collect
    _ctx.run("adapters",    &test_producer_adapters);

    // free terminals
    _ctx.run("terminals",   &test_producer_terminals);

    // detection traits & concepts
    _ctx.run("traits",      &test_producer_traits);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the producer.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::producer_module_info.display_name,
            djinterp::testing::producer_module_info.description_short,
            djinterp::testing::producer_module_info.path,
            djinterp::testing::producer_module_info.description_short,
            djinterp::testing::producer_module_info.description_long
        },
        {
            { djinterp::testing::producer_module_info,
              &djinterp::testing::producer_module_run_all }
        });
}
