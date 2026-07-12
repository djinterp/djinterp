/******************************************************************************
* djinterp [test]                                         curry_test_runner.cpp
*
*   Standalone entry point for the curry.hpp unit-test binary, relocated into
* the per-module subdirectory .../functional/curry/ to match the event
* subsystem's layout (one runner per module, under its own folder).
*
*   curry_tests.hpp is a DTest run_session suite: its sections are
* `void(test_handler&)` functions scheduled by curry_module_run_all against a
* test_runner_ctx, and curry_module_info carries the module's identity. Both are
* declared in the header and DEFINED in the suite's wiring translation unit
* (curry_tests_runner.cpp); this file only drives them. The session_descriptor
* is derived from curry_module_info so the module's identity has a single source
* of truth - exactly as the wiring TU's own main() builds it. run_session()
* constructs the engine, walks the one module's sections through the shared
* handler / printer / sink, renders the master-template report, and returns the
* exit code. One module in the manifest -> run_session() suppresses the outer
* MODULE wrap, so the report reads as a clean single-module body.
*
*   SINGLE ENTRY POINT:
*   curry_tests_runner.cpp (the wiring TU) still carries an inline main() for
* building the suite standalone in place. When this relocated runner is the
* entry point, guard or drop that inline main() so the two do not collide;
* module_info + run_all stay in the wiring TU, only the entry point moves here.
*
*   ADDING MODULES:
*   To fold this suite into a larger binary, add more
* `{ <mod>_module_info, &<mod>_module_run_all }` entries to the
* initializer_list; run_session() adapts its totals, headers, verdict, and exit
* code off the manifest with no other change to main().
*
*   VERDICT POLICY:
*   run_session() returns EXIT_FAILURE only when a module's verdict resolves to
* failed; pending and empty sessions exit success. The four-way verdict (passed
* / pending / failed / empty) is surfaced in the rendered report's status line.
*
*   PORTABILITY:
*   C++11 minimum, inherited from the DTest framework. All standard-version
* gating, attributes, and noexcept resolution route through the env.h chain
* pulled in transitively via curry_tests.hpp.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/curry/curry_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "curry_tests.hpp"                 // resolved via the test include path


int
main()
{
    // Derive the single-module session descriptor from the module identity
    // constant (field order: name, description, path, description_short,
    // description_long - see test_runner.hpp section III), so curry's identity
    // is stated once, in curry_module_info.
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::curry_module_info.display_name,
            djinterp::testing::curry_module_info.description_short,
            djinterp::testing::curry_module_info.path,
            djinterp::testing::curry_module_info.description_short,
            djinterp::testing::curry_module_info.description_long
        },
        {
            { djinterp::testing::curry_module_info,
              &djinterp::testing::curry_module_run_all }
        });
}
