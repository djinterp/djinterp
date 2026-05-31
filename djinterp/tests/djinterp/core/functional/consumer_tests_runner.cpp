/******************************************************************************
* djinterp [test]                                  consumer_tests_runner.cpp
*
*   Module wiring and entry point for the consumer.hpp unit test suite.
* Defines the module identity constant, the run_all function that
* schedules every section in document order against the runner engine,
* and a main() that drives a single-module session.
*
*   To fold this module into an aggregate binary instead, drop the
* main() below (or guard it) and add `{ consumer_module_info,
* &consumer_module_run_all }` to that binary's run_session manifest.
******************************************************************************/

#include "consumer_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_module_info;
using ::djinterp::test::test_runner_ctx;


///////////////////////////////////////////////////////////////////////////////
///                I.   MODULE IDENTITY                                      ///
///////////////////////////////////////////////////////////////////////////////

// consumer_module_info
//   constant: per-module identity bound at the in-output banner site.
const test_module_info consumer_module_info =
{
    "consumer",
    "djinterp / functional / consumer",
    "/inc/djinterp/core/functional/consumer.hpp",
    "consumer suite",
    "First-class consumer sinks: primitives, adapters, broadcast, "
    "stateful flow, branching, type erasure, and the predicate SFINAE "
    "traits/concepts of consumer.hpp."
};


///////////////////////////////////////////////////////////////////////////////
///                II.  RUN-ALL (SECTION SCHEDULE)                           ///
///////////////////////////////////////////////////////////////////////////////

/*
consumer_module_run_all
  Schedules every consumer.hpp test section in document order against
  the runner engine.  One _ctx.run(name, &fn) call per section; the
  engine resets the handler's counters before each section and rolls
  the per-section deltas into the module total.

Parameter(s):
  _ctx: the runner engine, supplying the shared handler / printer /
        sink and the per-section accounting.
Return:
  none.
*/
void
consumer_module_run_all(
    test_runner_ctx& _ctx
)
{
    _ctx.run("Section 0  - traits & concepts", &consumer_tests_traits);
    _ctx.run("Section II - primitives",        &consumer_tests_primitives);
    _ctx.run("Section II - adapters",          &consumer_tests_adapters);
    _ctx.run("Section II - tee broadcast",     &consumer_tests_tee);
    _ctx.run("Section II - flow",              &consumer_tests_flow);
    _ctx.run("Section II - branching",         &consumer_tests_branching);
    _ctx.run("Section III- type erasure",      &consumer_tests_erasure);

    return;
}


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                III. ENTRY POINT                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
main
  Drives a single-module session over the consumer.hpp suite and returns
  its exit code.

Return:
  EXIT_FAILURE iff any section's verdict resolved to failed;
  EXIT_SUCCESS otherwise.
*/
int
main()
{
    return ::djinterp::test::run_session(
        ::djinterp::test::session_descriptor{
            "djinterp / functional / consumer",
            "Unit tests for consumer.hpp",
            "/inc/djinterp/core/functional/consumer.hpp",
            "consumer suite",
            "First-class consumer sinks: primitives, adapters, broadcast, "
            "stateful flow, branching, type erasure, and the predicate "
            "SFINAE traits/concepts of consumer.hpp."
        },
        {
            { ::djinterp::testing::consumer_module_info,
              &::djinterp::testing::consumer_module_run_all }
        });
}
