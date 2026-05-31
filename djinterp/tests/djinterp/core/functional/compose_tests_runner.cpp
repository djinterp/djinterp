/******************************************************************************
* djinterp [test]                                   compose_tests_runner.cpp
*
*   Module wiring and entry point for the compose.hpp unit test suite.
* Defines the module identity constant, the run_all function that
* schedules every section in document order against the runner engine,
* and a main() that drives a single-module session.
*
*   To fold this module into an aggregate binary instead, drop the
* main() below (or guard it) and add `{ compose_module_info,
* &compose_module_run_all }` to that binary's run_session manifest.
******************************************************************************/

#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_module_info;
using ::djinterp::test::test_runner_ctx;


///////////////////////////////////////////////////////////////////////////////
///                I.   MODULE IDENTITY                                      ///
///////////////////////////////////////////////////////////////////////////////

// compose_module_info
//   constant: per-module identity bound at the in-output banner site.
const test_module_info compose_module_info =
{
    "compose",
    "djinterp / functional / compose",
    "/inc/functional/compose.hpp",
    "compose suite",
    "Composition, partial application, tap, memoize, fix, and the "
    "predicate SFINAE traits/concepts of compose.hpp."
};


///////////////////////////////////////////////////////////////////////////////
///                II.  RUN-ALL (SECTION SCHEDULE)                           ///
///////////////////////////////////////////////////////////////////////////////

/*
compose_module_run_all
  Schedules every compose.hpp test section in document order against
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
compose_module_run_all(
    test_runner_ctx& _ctx
)
{
    _ctx.run("Section 0  - traits & concepts", &compose_tests_traits);
    _ctx.run("Section II - compose / pipe",    &compose_tests_composition);
    _ctx.run("Section III- variadic",          &compose_tests_variadic);
    _ctx.run("Section IV - partial_back",      &compose_tests_partial);
    _ctx.run("Section V  - tap",               &compose_tests_tap);
    _ctx.run("Section VI - memoize",           &compose_tests_memoize);
    _ctx.run("Section VII- fix",               &compose_tests_fix);

    return;
}


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                III. ENTRY POINT                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
main
  Drives a single-module session over the compose.hpp suite and returns
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
            "djinterp / functional / compose",
            "Unit tests for compose.hpp",
            "/inc/functional/compose.hpp",
            "compose suite",
            "Composition, partial application, tap, memoize, fix, and the "
            "predicate SFINAE traits/concepts of compose.hpp."
        },
        {
            { ::djinterp::testing::compose_module_info,
              &::djinterp::testing::compose_module_run_all }
        });
}
