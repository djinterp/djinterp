/******************************************************************************
* djinterp [test]                                      monad_tests_runner.cpp
*
*   Module wiring and entry point for the monad.hpp unit test suite.
* Defines the module identity constant, the run_all function that
* schedules every section in document order against the runner engine,
* and a main() that drives a single-module session.
*
*   To fold this module into an aggregate binary instead, drop the
* main() below (or guard it) and add `{ monad_module_info,
* &monad_module_run_all }` to that binary's run_session manifest.
******************************************************************************/
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_module_info;
using ::djinterp::test::test_runner_ctx;


///////////////////////////////////////////////////////////////////////////////
///                I.   MODULE IDENTITY                                      ///
///////////////////////////////////////////////////////////////////////////////

// monad_module_info
//   constant: per-module identity bound at the in-output banner site.
const test_module_info monad_module_info =
{
    "monad",
    "djinterp / functional / monad",
    "/inc/functional/monad.hpp",
    "monad suite",
    "Monad protocol and generic operations: detection traits, unit / "
    "bind / map / join, then / kleisli_compose / lift_m2, the pipeline "
    "combinators, and the predicate SFINAE traits/concepts of monad.hpp."
};


///////////////////////////////////////////////////////////////////////////////
///                II.  RUN-ALL (SECTION SCHEDULE)                           ///
///////////////////////////////////////////////////////////////////////////////

/*
monad_module_run_all
  Schedules every monad.hpp test section in document order against the
  runner engine.  One _ctx.run(name, &fn) call per section; the engine
  resets the handler's counters before each section and rolls the
  per-section deltas into the module total.

Parameter(s):
  _ctx: the runner engine, supplying the shared handler / printer /
        sink and the per-section accounting.
Return:
  none.
*/
void
monad_module_run_all(
    test_runner_ctx& _ctx
)
{
    _ctx.run("Section 0  - traits & concepts", &monad_tests_traits);
    _ctx.run("Section I  - protocol",          &monad_tests_protocol);
    _ctx.run("Section II - core operations",   &monad_tests_operations);
    _ctx.run("Section II - composition",       &monad_tests_composition);
    _ctx.run("Section III/IV - pipeline",      &monad_tests_pipeline);

    return;
}


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                III. ENTRY POINT                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
main
  Drives a single-module session over the monad.hpp suite and returns
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
            "djinterp / functional / monad",
            "Unit tests for monad.hpp",
            "/inc/functional/monad.hpp",
            "monad suite",
            "Monad protocol and generic operations: detection traits, "
            "unit / bind / map / join, then / kleisli_compose / lift_m2, "
            "the pipeline combinators, and the predicate SFINAE "
            "traits/concepts of monad.hpp."
        },
        {
            { ::djinterp::testing::monad_module_info,
              &::djinterp::testing::monad_module_run_all }
        });
}
