/******************************************************************************
* djinterp [testing]                                   dtuple_tests_runner.cpp
*
*   Entry point for the DTest dtuple test suite.  Configures a
* DTestPrinter, attaches it to a DDefaultTestHandler via the
* listener-bundle wiring, runs every per-section module of the
* dtuple suite in order, and returns EXIT_SUCCESS or EXIT_FAILURE
* to the host environment.
*
*   ARCHITECTURE NOTES:
*   The runner takes a test_handler reference (via DDefaultTestHandler).
* The handler owns the printer wiring internally: set_printer() installs
* a bundle of listeners on the handler's event_handler whose bodies
* forward lifecycle events into the printer.  Counters are read via
* handler.result() after each module run completes.
*
*   The per-section test functions in dtuple_tests.hpp have the
* uniform signature `void(test_handler&)` (or the variadic-options
* wrapper `void(test_handler&, _Options&&...)`).  They record their
* assertions directly into the handler via record_assertion(), which
* in turn updates DSessionResult counters and pushes basic_test nodes
* into the handler's internal sink.  Because they don't go through
* handler.run(tree), the per-module isolation provided by start_session
* in the array suite doesn't apply automatically here -- we call
* handler.reset_counters() before each module instead, then snapshot
* handler.result() after to capture that module's totals.
*
*   MULTI-MODULE PROTOCOL:
*   We drive twelve independent runs against the same printer / sink,
* one per semantic section of dtuple.hpp:
*     1.  pack             - parameter-pack utilities + tuple detection
*     2.  construction     - to_tuple, make_tuple_of
*     3.  modifiers        - DWrapAll, ref / pointer / type modifiers,
*                            tuple_join
*     4.  transformation   - tuple_apply_all
*     5.  access           - DTupleTypeAt, tuple_type_at_value,
*                            tuple_concat
*     6.  counting         - tuple_count_type, DTupleCountAndRemove,
*                            tuple_consolidate_types
*     7.  splitting        - DTupleSplit, tuple_subsequence
*     8.  utilities        - tuple_to_pack
*     9.  selection        - DTypeCase, DTypeSelector, type_select_t
*     10. homogeneity      - is_tuple_homogeneous, is_homogeneous
*     11. 2d               - is_2d_tuple, tuple_inner_sizes,
*                            tuple_flatten_types, is_uniform_2d_tuple,
*                            and friends
*     12. relations        - normalize_tuple, tuple_all_elements_same_as
*
*   Per-module totals are captured between calls so that the rendered
* report's MODULE SUMMARY block can report a real count (Modules Tested:
* 12) rather than a constant.  Assertion totals are summed across runs.
*   To run only a subset of modules, comment out the corresponding
* `run_module(...)` line below; everything else (totals, headers,
* exit code) keys off `verdict_overall` / accumulators so it adapts.
*
*   VERDICT POLICY:
*   The runner returns a four-way session_verdict
* distinguishing passed / pending / failed / empty.  This main.cpp
* uses the verdict in two places:
*     - the status line in the rendered report (so a half-implemented
*       suite reads as "PENDING" rather than "FAIL")
*     - the process exit code, which is EXIT_FAILURE only when a real
*       failure or error was observed.  Pending leaves do NOT trigger
*       EXIT_FAILURE -- an unfinished suite hasn't broken anything,
*       it's just not done.
*
*   The historical distinction between "assertion totals" and "unit-test
* totals" is collapsed here: each per-section module IS the unit-test
* unit.  modules_total == tests_total; modules_passed_count ==
* tests_passed.  No on_module_start / on_test_failed listeners are
* needed because record_assertion already drives both the counter
* (DSessionResult) and the sink (basic_test nodes) without going
* through lifecycle events.
*
*   PORTABILITY:
*   C++11 minimum.  All standard-version gating, attributes, and
* noexcept resolution route through the env.h chain (env.h ->
* env_cpp_features.h -> env_attributes.h), pulled in transitively via
* djinterp.hpp and the test / * headers.  Helper functions are
* decorated with D_NODISCARD / D_NOEXCEPT / D_CONSTEXPR so the macros
* expand to the strongest spelling supported by the detected toolchain
* and degrade cleanly on older standards.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/meta/dtuple/dtuple_tests_runner.cpp
*                dtuple_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

// std
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
// djinterp
#include <djinterp/core/djinterp.hpp>
#include <djinterp/test/test_suite_printer.hpp>
#include <djinterp/cpp/modules/test/test_defaults.hpp>
#include <djinterp/cpp/modules/test/test_handler.hpp>
#include <djinterp/cpp/modules/test/test_printer.hpp>
#include "../../../../../../../tests/djinterp/core/meta/dtuple_tests.hpp"


// feature gates
//   the test framework itself enforces C++11 in test_handler.hpp,
// but we re-assert here so a misconfigured build of just the main
// translation unit produces a clean diagnostic at this site
// rather than deep inside the framework expansion.
//#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
//    #error "main.cpp requires C++11 or higher"
//#endif
using djinterp::test::D_TEST_TPL_MASTER_SUITE;
using djinterp::test::DDefaultTestHandler;
using djinterp::test::print_context;
using djinterp::test::DSessionResult;
using djinterp::test::session_verdict;
using djinterp::test::test_handler;
using djinterp::test::DTestPrinter;
using namespace djinterp::testing;


// ---------------------------------------------------------------------------
//  verdict helpers
//   File-internal mappings from session_verdict to the
// strings the master-suite template binds, plus a small
// helper that derives a per-module verdict from a captured
// DSessionResult snapshot (since our per-section functions
// return void).  Kept in an unnamed namespace so they don't
// pollute the link-time symbol table.
// ---------------------------------------------------------------------------
namespace {

    // verdict_status_line
    //   helper: human-readable verdict tag bound to
    // %has_passed% in the rendered report.
    //   NOTE: NOT D_CONSTEXPR.  A switch with multiple
    // return statements is not a valid constexpr function
    // body until C++14's relaxed constexpr; tagging this
    // would break the C++11 baseline.
    D_NODISCARD const char* verdict_status_line(
        session_verdict _verdict
    ) D_NOEXCEPT
    {
        switch (_verdict)
        {
            case session_verdict::passed:
                return "[PASS] ALL TESTS PASSED";

            case session_verdict::pending:
                return "[PENDING] TESTS NOT YET IMPLEMENTED";

            case session_verdict::failed:
                return "[FAIL] SOME TESTS FAILED";

            case session_verdict::empty:
                return "[EMPTY] NO TESTS OBSERVED";

            default:
                return "[?] UNKNOWN";
        }
    }


    // verdict_passes
    //   helper: did this run pass?  A run that ends in
    // session_verdict::passed counts as 1 passing module
    // for the MODULE SUMMARY rollup; anything else counts
    // as 0.  Pending and empty don't count as passes
    // because the user-facing "Modules Passed" tally
    // matches the strict pass criterion.
    D_NODISCARD D_CONSTEXPR std::size_t verdict_passes(
        session_verdict _verdict
    ) D_NOEXCEPT
    {
        return (_verdict == session_verdict::passed)
            ? std::size_t{1}
            : std::size_t{0};
    }


    // worst_verdict
    //   helper: combines two verdicts into the worst case.
    // Used to roll up the per-module verdicts into a single
    // suite-level verdict for the status line.  The order
    // is the same as DSessionResult::verdict()'s decision
    // order: failed dominates, then pending, then passed,
    // empty is the lowest.
    //   Three- or higher-way rollups compose pairwise:
    //   worst_verdict(worst_verdict(a, b), c).
    D_NODISCARD D_CONSTEXPR session_verdict worst_verdict(
        session_verdict _a,
        session_verdict _b
    ) D_NOEXCEPT
    {
        return (_a == session_verdict::failed ||
                _b == session_verdict::failed)
            ? session_verdict::failed
            : ((_a == session_verdict::pending ||
                _b == session_verdict::pending)
                ? session_verdict::pending
                : ((_a == session_verdict::passed ||
                    _b == session_verdict::passed)
                    ? session_verdict::passed
                    : session_verdict::empty));
    }


    // verdict_from_result
    //   helper: derives a session_verdict from a DSessionResult
    // snapshot.  Mirrors the decision order used by
    // DSessionResult::verdict() inside the test_handler:
    // failed > pending > passed > empty.
    //   We need this here because the dtuple per-section
    // functions return void, not a verdict.  We snapshot
    // handler.result() after each call and feed it through
    // this helper to recover the same answer.
    D_NODISCARD D_CONSTEXPR session_verdict verdict_from_result(
        const DSessionResult& _r
    ) D_NOEXCEPT
    {
        return (_r.failed > 0 || _r.errors > 0)
            ? session_verdict::failed
            : (_r.pending > 0
                ? session_verdict::pending
                : (_r.total > 0
                    ? session_verdict::passed
                    : session_verdict::empty));
    }

}  // unnamed namespace


int main()
{
    // -------------------------------------------------------------------
    // 0. variable declarations
    // -------------------------------------------------------------------

    std::string             output;
    DTestPrinter            printer;
    DDefaultTestHandler    handler;
    djinterp::DTextTemplate tpl;
    std::string             rendered;

    // suite-wide accumulators
    session_verdict         verdict_overall;
    std::size_t             modules_total;
    std::size_t             modules_passed_count;
    std::size_t             asserts_total;
    std::size_t             asserts_passed;
    std::size_t             asserts_failed;
    double                  seconds_total;
    double                  asserts_rate;
    double                  modules_rate;
    char                    asserts_rate_buf[16];
    char                    modules_rate_buf[16];
    char                    time_buf[24];

    verdict_overall      = session_verdict::empty;
    modules_total        = 0;
    modules_passed_count = 0;
    asserts_total        = 0;
    asserts_passed       = 0;
    asserts_failed       = 0;
    seconds_total        = 0.0;
    asserts_rate         = 0.0;
    modules_rate         = 0.0;

    // -------------------------------------------------------------------
    // 1. wire up printer + handler
    // -------------------------------------------------------------------

    printer.set_sink_string(output);
    handler.set_printer(&printer);

    // -------------------------------------------------------------------
    // 2. per-module helper
    // -------------------------------------------------------------------
    //   run_module: encapsulates the boilerplate that the array suite
    // gets for free from run_array_suite's `_kind` + `_out_seconds`
    // parameters.  Each per-section dtuple function is `void(handler&)`,
    // so we time it externally, snapshot handler.result() before /
    // after, and derive a per-module verdict from the delta.
    //
    //   We use handler.reset_counters() to zero DSessionResult before
    // each run, so handler.result() AFTER the call already reports
    // only this module's totals -- mirroring how handler.run(tree)
    // resets via start_session in the array example.
    //
    //   Progress is reported the same way as the array runner: a
    // [BEGIN] / [END] pair on stdout plus the module's section of
    // `output` dumped immediately when the run returns.  The full
    // master-template report still renders at the bottom of main().

    auto run_module = [&](
        const char* _name,
        void      (*_fn)(test_handler&)
    )
    {
        std::printf("[BEGIN] module: %s\n", _name);
        std::fflush(stdout);

        output.append("\n========================================"
                      "========================================\n");
        output.append("  MODULE: ");
        output.append(_name);
        output.append("\n========================================"
                      "========================================\n");

        const std::size_t mod_start = output.size();

        handler.reset_counters();

        const auto t0 = std::chrono::steady_clock::now();
        _fn(handler);
        const auto t1 = std::chrono::steady_clock::now();

        const double          seconds = std::chrono::duration<double>(t1 - t0).count();
        const DSessionResult  totals  = handler.result();
        const session_verdict v       = verdict_from_result(totals);

        // append a per-module summary line into the rendered output
        // so the master-template body contains something legible for
        // each section even though we don't fire per-leaf events.
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "  assertions:   %zu (passed %zu, failed %zu)\n"
                      "  time:         %.3fs\n"
                      "  verdict:      %s\n",
                      static_cast<std::size_t>(totals.total),
                      static_cast<std::size_t>(totals.passed),
                      static_cast<std::size_t>(totals.failed),
                      seconds,
                      verdict_status_line(v));
        output.append(buf);

        // dump just this module's portion to stdout for live progress;
        // the full report still renders at the end
        std::fwrite(output.data() + mod_start,
                    1,
                    output.size() - mod_start,
                    stdout);
        std::fflush(stdout);

        std::printf("[END]   module: %s  verdict=%d  elapsed=%.3fs  "
                    "leaves=%zu/%zu\n",
                    _name,
                    static_cast<int>(v),
                    seconds,
                    static_cast<size_t>(totals.passed),
                    static_cast<size_t>(totals.total));
        std::fflush(stdout);

        // roll up suite-wide accumulators
        ++modules_total;
        modules_passed_count += verdict_passes(v);
        asserts_total        += static_cast<std::size_t>(totals.total);
        asserts_passed       += static_cast<std::size_t>(totals.passed);
        asserts_failed       += static_cast<std::size_t>(totals.failed);
        seconds_total        += seconds;
        verdict_overall       = worst_verdict(verdict_overall, v);
    };

    // -------------------------------------------------------------------
    // 3. run each module
    // -------------------------------------------------------------------
    //   Order matches dtuple.hpp's section layout.  Comment out any
    // line to skip that section; nothing else needs to change.
    run_module("pack",           dtuple_tests_pack_all);
    run_module("construction",   dtuple_tests_construction_all);
    run_module("modifiers",      dtuple_tests_modifiers_all);
    run_module("transformation", dtuple_tests_transformation_all);
    run_module("access",         dtuple_tests_access_all);
    run_module("counting",       dtuple_tests_counting_all);
    run_module("splitting",      dtuple_tests_splitting_all);
    run_module("utilities",      dtuple_tests_utilities_all);
    run_module("selection",      dtuple_tests_selection_all);
    run_module("homogeneity",    dtuple_tests_homogeneity_all);
    run_module("2d",             dtuple_tests_2d_all);
    run_module("relations",      dtuple_tests_relations_all);

    // -------------------------------------------------------------------
    // 4. format pass-rate strings
    // -------------------------------------------------------------------

    if (asserts_total > 0)
    {
        asserts_rate = (100.0 * static_cast<double>(asserts_passed) /
                                static_cast<double>(asserts_total));
    }

    if (modules_total > 0)
    {
        modules_rate = (100.0 * static_cast<double>(modules_passed_count) /
                                static_cast<double>(modules_total));
    }

    std::snprintf(asserts_rate_buf,
                  sizeof(asserts_rate_buf),
                  "%.2f%%",
                  asserts_rate);

    std::snprintf(modules_rate_buf,
                  sizeof(modules_rate_buf),
                  "%.2f%%",
                  modules_rate);

    std::snprintf(time_buf,
                  sizeof(time_buf),
                  "%.3f seconds",
                  seconds_total);

    // -------------------------------------------------------------------
    // 5. bind the master-suite template
    // -------------------------------------------------------------------
    //   The "module" identity at the top of the report is the SUITE
    // name (the whole test binary), not any one section within it.
    // The per-section banners and bodies already live inside `output`
    // (bound to %test_modules%).

    // -- header / identity --
    tpl.bind("module_name",         "djinterp / dtuple");
    tpl.bind("module_description",
             "Compile-time and runtime tests for the dtuple metafunction "
             "module: parameter-pack utilities, construction, modifiers, "
             "transformations, element access, counting / filtering, "
             "splitting, runtime utilities, selection, homogeneity, "
             "2D / jagged tuples, and tuple-of-tuples relation traits");
    tpl.bind("module_path",
             "/inc/djinterp/cpp/meta/dtuple.hpp");
    tpl.bind("timestamp_start",     __DATE__ " " __TIME__);
    tpl.bind("description_short",   "dtuple suite");
    tpl.bind("description_long",
             "Tests for the dtuple compile-time tuple metafunction library, "
             "covering twelve semantic sections from parameter-pack "
             "utilities through 2D tuple relations.  The bulk of dtuple is "
             "compile-time machinery, so each section's static_assert "
             "block validates the trait surface and the runtime portion "
             "exercises tuple_concat, tuple_type_at_value, tuple_to_pack, "
             "and is_homogeneous");

    // -- per-module body --
    tpl.bind("test_modules",        output);

    // -- module rollup --
    tpl.bind("modules_tested",      print_context::size_to_string(modules_total));
    tpl.bind("modules_passed",      print_context::size_to_string(modules_passed_count));
    tpl.bind("modules_percent",     std::string(modules_rate_buf));

    // -- assertion totals (summed across modules) --
    tpl.bind("asserts_total",       print_context::size_to_string(asserts_total));
    tpl.bind("asserts_passed",      print_context::size_to_string(asserts_passed));
    tpl.bind("asserts_failed",      print_context::size_to_string(asserts_failed));
    tpl.bind("asserts_percent",     std::string(asserts_rate_buf));

    // -- unit-test totals --
    //   For dtuple we treat each section as one unit test; assertion-
    // level rollup happens via the counter delta on handler.result().
    // The array runner derives this from event listeners; here we
    // can lift the numbers directly from the module accumulators.
    {
        char              tests_rate_buf[16];
        double            tests_rate;
        const std::size_t tests_failed = modules_total - modules_passed_count;

        tests_rate = (modules_total > 0)
            ? ( 100.0 * static_cast<double>(modules_passed_count) /
                        static_cast<double>(modules_total) )
            : 0.0;

        std::snprintf(tests_rate_buf,
                      sizeof(tests_rate_buf),
                      "%.2f%%",
                      tests_rate);

        tpl.bind("tests_total",     print_context::size_to_string(modules_total));
        tpl.bind("tests_passed",    print_context::size_to_string(modules_passed_count));
        tpl.bind("tests_failed",    print_context::size_to_string(tests_failed));
        tpl.bind("tests_percent",   std::string(tests_rate_buf));
    }

    // -- timing --
    tpl.bind("time_total",          std::string(time_buf));

    // -- overall verdict --
    tpl.bind("has_passed",          verdict_status_line(verdict_overall));

    // -------------------------------------------------------------------
    // 6. render and emit
    // -------------------------------------------------------------------

    rendered = tpl.render(D_TEST_TPL_MASTER_SUITE);

    std::fwrite(rendered.data(),
                1,
                rendered.size(),
                stdout);

    // -------------------------------------------------------------------
    // 7. exit code
    // -------------------------------------------------------------------
    //   EXIT_FAILURE only on real failure.  Pending suites exit success
    // because nothing actually broke; the "incomplete" state is
    // communicated through the rendered report's status line, not the
    // exit code.
    //   Flip the predicate if your CI policy is stricter (e.g. require
    // verdict_overall == passed for success).

    return (verdict_overall == session_verdict::failed)
        ? EXIT_FAILURE
        : EXIT_SUCCESS;
}
