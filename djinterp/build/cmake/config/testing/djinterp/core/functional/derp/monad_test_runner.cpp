/******************************************************************************
* djinterp [test]                                         monad_test_runner.cpp
*
*   Standalone entry point for the monad.hpp unit-test binary, relocated into
* the per-module subdirectory .../functional/monad/ to match the event
* subsystem's layout (one runner per module, under its own folder).
*
*   monad_tests.hpp is a DTest run_session suite: its sections are
* `void(test_handler&)` functions scheduled by monad_module_run_all against a
* test_runner_ctx, and monad_module_info carries the module's identity. Both are
* declared in the header and DEFINED in the suite's wiring translation unit
* (monad_tests_runner.cpp); this file only drives them. monad.hpp is a protocol
* header, so monad_tests.hpp also defines the self-contained test_maybe<T> monad
* and its monad_traits specialization that the sections fold over. run_session()
* constructs the engine, walks the one module's sections through the shared
* handler / printer / sink, renders the master-template report, and returns the
* exit code. One module in the manifest -> run_session() suppresses the outer
* MODULE wrap, so the report reads as a clean single-module body.
*
*   SINGLE ENTRY POINT:
*   monad_tests_runner.cpp (the wiring TU) still carries an inline main() for
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
* pulled in transitively via monad_tests.hpp.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/monad/monad_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "monad_tests.hpp"                 // resolved via the test include path


int
main()
{
    // One module in the manifest -> run_session() suppresses the outer MODULE
    // wrap and renders a clean single-module report. Field order of
    // session_descriptor: name, description, path, description_short,
    // description_long (see test_runner.hpp section III).
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
