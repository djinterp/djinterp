/******************************************************************************
* djinterp [test]                                       producer_test_runner.cpp
*
*   Standalone entry point for the producer.hpp unit-test binary, relocated into
* the per-module subdirectory .../functional/producer/ to match the event
* subsystem's layout (one runner per module, under its own folder).
*
*   producer_tests.hpp is now a DTest run_session suite (converted from the old
* test_registry harness): its sections are `void(test_handler&)` functions
* scheduled by producer_module_run_all against a test_runner_ctx, recording
* their checks through the D_TEST_CHECK bridge over test::record_assertion.
* producer_module_info carries the module's identity. Both are declared in the
* header and DEFINED in the suite's wiring translation unit
* (producer_tests_runner.cpp); this file only drives them. The
* session_descriptor is derived from producer_module_info so the module's
* identity has a single source of truth. run_session() constructs the engine,
* walks the one module's sections through the shared handler / printer / sink,
* renders the master-template report, and returns the exit code. One module in
* the manifest -> run_session() suppresses the outer MODULE wrap, so the report
* reads as a clean single-module body.
*
*   SINGLE ENTRY POINT:
*   producer_tests_runner.cpp (the wiring TU) still carries an inline main() for
* building the suite standalone in place. When this relocated runner is the
* entry point, guard or drop that inline main() so the two do not collide;
* module_info + run_all stay in the wiring TU, only the entry point moves here.
*
*   PORTABILITY:
*   C++11 minimum, inherited from the DTest framework. The producer sections
* internally guard their C++11/14/20-specific checks (traits, _v aliases,
* concepts) exactly as before; the harness change does not alter that gating.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/producer/producer_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "producer_tests.hpp"             // resolved via the test include path


int
main()
{
    // Derive the single-module session descriptor from the module identity
    // constant (field order: name, description, path, description_short,
    // description_long - see test_runner.hpp section III), so producer's
    // identity is stated once, in producer_module_info.
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
