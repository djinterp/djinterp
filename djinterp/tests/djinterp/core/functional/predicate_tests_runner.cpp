/******************************************************************************
* djinterp [test]                                     predicate_tests_runner.cpp
*
*   Module wiring and standalone entry point for the predicate.hpp suite,
* converted from the legacy self-contained test_registry harness to the DTest
* run_session model so it reads and reports exactly like every other functional
* module (view, consumer, compose, ...).
*
*   predicate_tests.hpp now declares each section with the framework's leaf
* signature `void(test::test_handler&)`; the section .cpp files record their
* checks through D_TEST_CHECK, the thin bridge over test::record_assertion that
* replaced D_TESTING_CHECK (it stringifies the checked expression as the
* assertion label). This translation unit binds the two run_session hooks the
* header declares: predicate_module_info (identity) and predicate_module_run_all
* (the section schedule).
*
*   The inline main() below runs the suite standalone in place. The relocated
* per-module runner at .../functional/predicate/predicate_test_runner.cpp is the
* canonical entry point under the config/testing tree; when building through it,
* guard or drop this main() so the two do not collide. module_info + run_all
* stay here with the suite.
*
* path:      /inc/functional/predicate_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "./predicate_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// predicate_module_info
//   constant: per-module identity, reused as the single-module session
// descriptor below.
const test::test_module_info predicate_module_info =
{
    "predicate",
    "djinterp / functional / predicate",
    "/inc/djinterp/core/functional/predicate.hpp",
    "predicate suite",
    "logical predicate combinators: and / or / xor / not / nand / nor, the "
    "variadic all_of / any_of / none_of folds, the structural is_predicate_* "
    "traits, and the behavioral is_predicate trait with its C++20 concepts"
};


/*
predicate_module_run_all
  Schedules every predicate.hpp test section against the shared engine, in the
  same order the sections appear in predicate.hpp's table of contents.
*/
void
predicate_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    // III / IV. combinators + factories
    _ctx.run("binary",     &test_predicate_binary);
    _ctx.run("not",        &test_predicate_not);
    _ctx.run("nand_nor",   &test_predicate_nand_nor);

    // V. variadic folds
    _ctx.run("variadic",   &test_predicate_variadic);

    // VI. traits & concepts
    _ctx.run("traits",     &test_predicate_traits);
    _ctx.run("behavioral", &test_predicate_behavioral);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the predicate.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::predicate_module_info.display_name,
            djinterp::testing::predicate_module_info.description_short,
            djinterp::testing::predicate_module_info.path,
            djinterp::testing::predicate_module_info.description_short,
            djinterp::testing::predicate_module_info.description_long
        },
        {
            { djinterp::testing::predicate_module_info,
              &djinterp::testing::predicate_module_run_all }
        });
}
