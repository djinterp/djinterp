/******************************************************************************
* djinterp [test]                                      consumer_test_runner.cpp
*
*   Standalone entry point for the consumer.hpp unit-test binary, relocated
* into the per-module subdirectory .../functional/consumer/ to match the event
* subsystem's layout (one runner per module, under its own folder).
*
*   consumer_tests.hpp is a DTest run_session suite: its sections are
* `void(test_handler&)` functions scheduled by consumer_module_run_all against a
* test_runner_ctx, and consumer_module_info carries the module's identity. Both
* are declared in the header and DEFINED in the suite's wiring translation unit
* (consumer_tests_runner.cpp); this file only drives them. run_session()
* constructs the engine, walks the one module's sections through the shared
* handler / printer / sink, renders the master-template report, and returns the
* exit code. Because the manifest names exactly one module, run_session()
* suppresses the outer MODULE wrap, so the report reads as a clean single-module
* body.
*
*   SINGLE ENTRY POINT:
*   consumer_tests_runner.cpp (the wiring TU) still carries an inline main() for
* building the suite standalone in place. When this relocated runner is the
* entry point, guard or drop that inline main() so the two do not collide - the
* wiring TU's own header note documents exactly this ("drop the main() below (or
* guard it) and add { consumer_module_info, &consumer_module_run_all } to that
* binary's run_session manifest"). module_info + run_all stay in the wiring TU;
* only the entry point moves here.
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
* / pending / failed / empty) is surfaced in the rendered report's status line,
* so a half-implemented suite reads as PENDING rather than breaking the build.
*
*   PORTABILITY:
*   C++11 minimum, inherited from the DTest framework. All standard-version
* gating, attributes, and noexcept resolution route through the env.h chain
* pulled in transitively via consumer_tests.hpp.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/consumer/consumer_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "consumer_tests.hpp"              // resolved via the test include path


using djinterp::test::run_session;
using djinterp::test::session_descriptor;
using djinterp::testing::consumer_module_info;
using djinterp::testing::consumer_module_run_all;


int
main()
{
    // One module in the manifest -> run_session() suppresses the outer
    // MODULE wrap and renders a clean single-module report. The field order
    // of session_descriptor is: name, description, path, description_short,
    // description_long (see test_runner.hpp section III).
    return run_session(
        session_descriptor{
            "djinterp / functional / consumer",
            "Predicate traits, primitive sinks, adapters, tee broadcast, flow "
            "control, branching, and type erasure for consumer.hpp",
            "/inc/djinterp/core/functional/consumer.hpp",
            "consumer suite",
            "Tests for consumer.hpp across primitives, adapters, tee, flow "
            "control, branching, and boxed type erasure"
        },
        {
            { consumer_module_info, &consumer_module_run_all }
        });
}
