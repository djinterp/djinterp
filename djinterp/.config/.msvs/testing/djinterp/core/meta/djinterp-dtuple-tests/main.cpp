/******************************************************************************
* djinterp [test]                                                     main.cpp
*
*   Entry point for the dtuple compile-time trait test suite.  Configures
* a suite_printer with the project-standard banner format (matching the
* buffer suite's textual layout exactly), runs the dtuple test module,
* and returns EXIT_SUCCESS or EXIT_FAILURE to the host environment.
*
*   The suite infrastructure (suite_printer / suite_results /
* module_result) is the same one used by the buffer suite — this file
* is deliberately structured so adding a second or third test module
* (e.g. for djinterp::core::meta/other headers) is a matter of
* appending another run_*_tests(...) call and accumulating its
* outcome into the results object; no printer changes are needed.
*
*   NOTE (2026.04.19): the dtuple test module has been expanded to
* split its tests across semantic groups (structural, composition,
* indexing, transforms, classification, flatten, meta) and to
* demonstrate additional framework features (test_tree overlay,
* trait_suite_object runtime adapter, mid-run listener enable /
* disable, queued custom-event dispatch).  The orchestrator's
* signature is unchanged, so main.cpp needs no structural edits —
* only the suite name / description reflect the expansion.
*
*
* path:      /.config/.msvs/testing/core/meta/djinterp-dtuple-tests/main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.18
* revised:                                                 2026.04.19
******************************************************************************/

// std
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
// djinterp
#include "../../../../../../inc/djinterp/test/test_handler.hpp"
#include "../../../../../../inc/djinterp/test/test_suite_printer.hpp"
#include "../../../../../../tests/djinterp/core/meta/dtuple_tests.hpp"


// =========================================================================
// EXIT CODES
// =========================================================================
//   File-scope constants rather than macros so they pick up the
// right type (int) and appear under their own name in a debugger.

namespace {

    // exit_success_all_passed
    //   constant: every recorded outcome passed.
    constexpr int exit_success_all_passed        = 0;

    // exit_failure_any_failed
    //   constant: at least one outcome failed or errored.
    constexpr int exit_failure_any_failed        = 1;

    // exit_failure_empty_session
    //   constant: session ran but produced no outcomes.  Treated
    // as an internal error rather than a trivial pass.
    constexpr int exit_failure_empty_session     = 2;

    // exit_failure_exception
    //   constant: a std::exception escaped the test session.
    // Should be unreachable in well-formed listeners; kept as a
    // safety net.
    constexpr int exit_failure_exception         = 3;

    // exit_failure_unknown_exception
    //   constant: a non-std::exception escaped the test session.
    constexpr int exit_failure_unknown_exception = 4;

}  // anonymous namespace


// =========================================================================
// MODULE RUNNER
// =========================================================================
//   Wrapper that invokes the dtuple test module through its orchestrator
// and packages the resulting session_result into a module_result suitable
// for feeding suite_printer.  Separated from main() so adding a second
// module is one more call at the same indentation level as this one.

namespace {

    // run_dtuple_module
    //   helper: constructs a fresh test_handler, runs the dtuple
    // test module into it, and converts the session_result into a
    // populated module_result.
    djinterp::test::module_result
    run_dtuple_module()
    {
        djinterp::test::test_handler   handler;
        djinterp::test::session_result session =
            djinterp::testing::run_dtuple_tests(handler);

        djinterp::test::module_result mod;

        mod.name              = "dtuple.hpp";
        mod.description       = "dtuple module: compile-time "
                                "tuple metaprogramming traits "
                                "(structural / composition / "
                                "indexing / transforms / "
                                "classification / flatten / meta)";
        mod.passed            = ( !session.any_failed() &&
                                  (session.total > 0) );
        mod.assertions_total  = session.total;
        mod.assertions_passed = session.passed;
        mod.tests_total       = session.total;
        mod.tests_passed      = session.passed;
        mod.elapsed_seconds   = 0.0;

        return mod;
    }

}  // anonymous namespace


// =========================================================================
// ENTRY POINT
// =========================================================================

int
main(
    int    /*_argc*/,
    char** /*_argv*/
)
{
    int exit_code = exit_failure_unknown_exception;

    // wrap the session in try/catch so a stray listener exception
    // produces a clean exit code rather than a core dump.  the
    // framework's own listeners are noexcept by construction;
    // this guards against user-supplied additions.
    try
    {
        // ---- configure the suite printer ----

        djinterp::test::suite_printer sp;

        sp.set_suite_name(
            "djinterp dtuple Test Suite");
        sp.set_suite_description(
            "Compile-time trait validation for "
            "core/meta/dtuple.hpp  "
            "(expanded suite with hierarchical test tree, "
            "full framework-feature demonstration)");
        sp.set_sink_stdout();

        // ---- render the opening banner ----

        sp.print_suite_header();

        // ---- run the module ----
        //   print_module_header and print_module_footer both take
        // a const module_result&, so we build the record first,
        // then pass it to both methods bracketing the actual run.

        djinterp::test::module_result dtuple_mod = run_dtuple_module();

        sp.print_module_header(dtuple_mod);
        sp.print_module_footer(dtuple_mod);

        // ---- accumulate into suite_results ----
        //   suite_printer owns a suite_results instance that its
        // bindings draw from; add_module updates the module-level
        // aggregate counters and appends the per-module record.
        // Assertion- and test-level totals live on suite_results
        // directly and are added explicitly here — add_module
        // handles only the module count and the per-module append.

        sp.results().add_module(dtuple_mod);

        sp.results().assertions.total  += dtuple_mod.assertions_total;
        sp.results().assertions.passed += dtuple_mod.assertions_passed;
        sp.results().assertions.failed += ( dtuple_mod.assertions_total -
                                            dtuple_mod.assertions_passed );
        sp.results().tests_total       += dtuple_mod.tests_total;
        sp.results().tests_passed      += dtuple_mod.tests_passed;

        // ---- render comprehensive + final status ----

        sp.print_comprehensive();
        sp.print_final_status();

        // ---- translate to exit code ----

        if (dtuple_mod.assertions_total == 0)
        {
            exit_code = exit_failure_empty_session;
        }
        else if (!dtuple_mod.passed)
        {
            exit_code = exit_failure_any_failed;
        }
        else
        {
            exit_code = exit_success_all_passed;
        }
    }
    catch (const std::exception& _ex)
    {
        std::fprintf(stderr,
                     "\xE2\x80\xBC test session aborted: %s\n",
                     _ex.what());
        exit_code = exit_failure_exception;
    }
    catch (...)
    {
        std::fputs(
            "\xE2\x80\xBC test session aborted: "
            "unknown exception\n",
            stderr);
        exit_code = exit_failure_unknown_exception;
    }

    return exit_code;
}