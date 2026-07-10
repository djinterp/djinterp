/******************************************************************************
* djinterp [test]                                      predicate_test_runner.cpp
*
*   Standalone entry point for the predicate.hpp unit-test binary, relocated
* into the per-module subdirectory .../functional/predicate/ to match the event
* subsystem's layout (one runner per module, under its own folder).
*
*   predicate_tests.hpp is now a DTest run_session suite (converted from the old
* test_registry harness): its sections are `void(test_handler&)` functions
* scheduled by predicate_module_run_all against a test_runner_ctx, recording
* their checks through the D_TEST_CHECK bridge over test::record_assertion.
* predicate_module_info carries the module's identity. Both are declared in the
* header and DEFINED in the suite's wiring translation unit
* (predicate_tests_runner.cpp); this file only drives them. The
* session_descriptor is derived from predicate_module_info so the module's
* identity has a single source of truth. run_session() constructs the engine,
* walks the one module's sections through the shared handler / printer / sink,
* renders the master-template report, and returns the exit code. One module in
* the manifest -> run_session() suppresses the outer MODULE wrap, so the report
* reads as a clean single-module body.
*
*   SINGLE ENTRY POINT:
*   predicate_tests_runner.cpp (the wiring TU) still carries an inline main() for
* building the suite standalone in place. When this relocated runner is the
* entry point, guard or drop that inline main() so the two do not collide;
* module_info + run_all stay in the wiring TU, only the entry point moves here.
*
*   PORTABILITY:
*   C++11 minimum, inherited from the DTest framework. The predicate sections
* internally guard their C++11/14/20-specific checks (traits, _v aliases,
* concepts) exactly as before; the harness change does not alter that gating.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/predicate/predicate_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "predicate_tests.hpp"            // resolved via the test include path


int
main()
{
    // Derive the single-module session descriptor from the module identity
    // constant (field order: name, description, path, description_short,
    // description_long - see test_runner.hpp section III), so predicate's
    // identity is stated once, in predicate_module_info.
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
