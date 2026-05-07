/******************************************************************************
* djinterp [test]                                                      main.cpp
*
*   Entry point for the DTest array test suite.  Configures a
* test_printer, attaches it to a default_test_handler via the
* listener-bundle wiring, runs both Part A (base array) and Part B
* (threadsafe wrappers) of the suite, and returns EXIT_SUCCESS or
* EXIT_FAILURE to the host environment.
*
*   ARCHITECTURE NOTES:
*   The runner takes a test_handler reference rather than a
* test_printer reference.  The handler owns the printer wiring
* internally: set_printer() installs a bundle of listeners on
* the handler's event_handler whose bodies forward lifecycle
* events into the printer.  Counters are read via
* handler.result() after each walk completes.
*
*   MULTI-MODULE PROTOCOL:
*   We drive two independent runs against the same printer/sink:
*     1. run_array_suite           - Part A (base array)
*     2. run_threadsafe_array_suite - Part B (threadsafe wrappers)
*   Per-run totals are captured between calls so that the
* rendered report's MODULE SUMMARY block can report a real
* count (Modules Tested: 2) rather than a constant.  Assertion
* totals are summed across runs; unit-test counters are
* maintained by lifecycle listeners that already accumulate
* across multiple handler.run() invocations naturally.
*   To run only Part A or only Part B, comment out the
* corresponding block below; everything else (totals, headers,
* exit code) keys off `verdict`/`totals` so it adapts.
*
*   VERDICT POLICY:
*   The runner returns a four-way session_verdict
* distinguishing passed / pending / failed / empty.  This
* main.cpp uses the verdict in two places:
*     - the status line in the rendered report (so a
*       half-implemented suite reads as "PENDING" rather
*       than "FAIL")
*     - the process exit code, which is EXIT_FAILURE only
*       when a real failure or error was observed.  Pending
*       leaves do NOT trigger EXIT_FAILURE - an unfinished
*       suite hasn't broken anything, it's just not done.
*
*   The historical distinction between "assertion totals" and
* "unit-test totals" (formerly two separate output structs) is
* not maintained inside default_test_handler itself: every
* leaf is counted as a node in session_result regardless of
* kind.  This main.cpp drives the per-suite unit-test count
* externally via on_module_start / on_test_failed /
* on_test_error / on_session_end listeners - one
* test_block-rank interior node is counted as one unit test,
* with the block flagged as failed if any assertion under it
* failed or errored.  The same on_module_start listener also
* emits a section header line into the printer's sink so the
* rendered report shows which block each group of assertions
* belongs to.
*
*   PORTABILITY:
*   C++11 minimum.  All standard-version gating, attributes,
* and noexcept resolution route through the env.h chain
* (env.h -> env_cpp_features.h -> env_attributes.h), pulled
* in transitively via djinterp.hpp and the test / * headers.
* Helper functions are decorated with D_NODISCARD / D_NOEXCEPT
* / D_CONSTEXPR so the macros expand to the strongest
* spelling supported by the detected toolchain and degrade
* cleanly on older standards.
*
*
* path:      /.config/.msvs/testing/core/container/array/main
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

// std
#include <cstdio>
#include <cstdlib>
#include <string>
// djinterp
#include "../../../../../../../inc/djinterp/core/djinterp.hpp"
#include "../../../../../../../inc/djinterp/test/test_suite_printer.hpp"
#include "../../../../../../../inc/djinterp/test/test_defaults.hpp"
#include "../../../../../../../inc/djinterp/test/test_handler.hpp"
#include "../../../../../../../inc/djinterp/test/test_printer.hpp"
#include "../../../../../../../tests/djinterp/core/container/array/array_tests.hpp"


// feature gates
//   the test framework itself enforces C++11 in test_handler.hpp,
// but we re-assert here so a misconfigured build of just the main
// translation unit produces a clean diagnostic at this site
// rather than deep inside the framework expansion.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "main.cpp requires C++11 or higher"
#endif


using djinterp::test::D_TEST_KIND_MODULE;
using djinterp::test::D_TEST_KIND_TEST_BLOCK;
using djinterp::test::D_TEST_TPL_MASTER_SUITE;
using djinterp::test::default_test_handler;
using djinterp::test::print_context;
using djinterp::test::session_result;
using djinterp::test::session_verdict;
using djinterp::test::test_printer;
using djinterp::testing::run_array_suite;
using djinterp::testing::run_threadsafe_array_suite;


// ---------------------------------------------------------------------------
//  verdict helpers
//   File-internal mappings from session_verdict to the
// strings the master-suite template binds.  Kept in an
// unnamed namespace so they don't pollute the link-time
// symbol table.
// ---------------------------------------------------------------------------
namespace {

    // verdict_status_line
    //   helper: human-readable verdict tag bound to
    // %has_passed% in the rendered report.
    //   NOTE: NOT D_CONSTEXPR.  A switch with multiple
    // return statements is not a valid constexpr function
    // body until C++14's relaxed constexpr; tagging this
    // would break the C++11 baseline.
    D_NODISCARD const char*
    verdict_status_line(
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
    D_NODISCARD D_CONSTEXPR std::size_t
    verdict_passes(
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
    // is the same as session_result::verdict()'s decision
    // order: failed dominates, then pending, then passed,
    // empty is the lowest.
    D_NODISCARD D_CONSTEXPR session_verdict
    worst_verdict(
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

}  // unnamed namespace


int
main()
{
    // -------------------------------------------------------------------
    // 0. variable declarations
    // -------------------------------------------------------------------

    std::string             output;
    test_printer            printer;
    default_test_handler    handler;
    djinterp::text_template tpl;
    std::string             rendered;

    // per-run state
    double                  seconds_a;
    double                  seconds_b;
    session_verdict         verdict_a;
    session_verdict         verdict_b;
    session_result          totals_a;
    session_result          totals_b;

    // suite-wide accumulators
    session_verdict         verdict_overall;
    std::size_t             modules_total;
    std::size_t             modules_passed_count;
    std::size_t             asserts_total;
    std::size_t             asserts_passed;
    std::size_t             asserts_failed;
    double                  seconds_total;
    double                  asserts_rate;
    char                    asserts_rate_buf[16];
    char                    modules_rate_buf[16];
    double                  modules_rate;
    char                    time_buf[24];

    seconds_a            = 0.0;
    seconds_b            = 0.0;
    seconds_total        = 0.0;
    asserts_rate         = 0.0;
    modules_rate         = 0.0;
    modules_total        = 0;
    modules_passed_count = 0;

    // -------------------------------------------------------------------
    // 1. wire up printer + handler
    // -------------------------------------------------------------------

    printer.set_sink_string(output);
    handler.set_printer(&printer);

    // -------------------------------------------------------------------
    // 1a. unit-test tracking + section-header rendering
    // -------------------------------------------------------------------
    //   Listeners are bound once and persist across both
    // handler.run() invocations below.  on_session_end
    // fires after each run, so unit_test_state::total /
    // passed naturally accumulate across both modules
    // without us having to do anything special.

    struct unit_test_state
    {
        std::size_t total;
        std::size_t passed;
        bool        current_open;
        bool        current_failed;

        unit_test_state() D_NOEXCEPT
            : total(0),
              passed(0),
              current_open(false),
              current_failed(false)
        {}

        // finalize_current
        //   helper: closes the currently-open block and
        // updates the running tally.  Idempotent when no
        // block is open (no-op).
        void finalize_current() D_NOEXCEPT
        {
            if (!current_open)
            {
                return;
            }

            ++total;
            if (!current_failed)
            {
                ++passed;
            }

            current_open   = false;
            current_failed = false;

            return;
        }
    };

    unit_test_state ut;

    handler.on<djinterp::test::events::on_module_start>(
        [&output, &ut](djinterp::event_context& /*_ctx*/,
                       const djinterp::test::basic_test* _obj) D_NOEXCEPT
        {
            if (_obj == nullptr)
            {
                return;
            }

            // we count test_blocks as unit tests; modules
            // (the outer wrapper) and other interior kinds
            // are ignored.
            if (_obj->type_id() != D_TEST_KIND_TEST_BLOCK)
            {
                return;
            }

            // close the prior block, if any
            ut.finalize_current();

            // open the new one
            ut.current_open   = true;
            ut.current_failed = false;

            // emit a section header into the printer's sink
            output.append("\n  --- ");
            output.append(_obj->name() ? _obj->name() : "");
            output.append(" ---\n");

            return;
        });

    handler.on<djinterp::test::events::on_test_failed>(
        [&ut](djinterp::event_context& /*_ctx*/,
              const djinterp::test::basic_test* /*_obj*/) D_NOEXCEPT
        {
            if (ut.current_open)
            {
                ut.current_failed = true;
            }

            return;
        });

    handler.on<djinterp::test::events::on_test_error>(
        [&ut](djinterp::event_context& /*_ctx*/,
              const djinterp::test::basic_test* /*_obj*/,
              const char*                       /*_what*/) D_NOEXCEPT
        {
            if (ut.current_open)
            {
                ut.current_failed = true;
            }

            return;
        });

    handler.on<djinterp::test::events::on_session_end>(
        [&ut](djinterp::event_context& /*_ctx*/,
              std::size_t /*_passed*/,
              std::size_t /*_failed*/) D_NOEXCEPT
        {
            ut.finalize_current();

            return;
        });

    // -------------------------------------------------------------------
    // 2. run Part A:  base array
    // -------------------------------------------------------------------
    //   Inject a per-module banner into the sink so the
    // rendered report shows which module each block of
    // assertions belongs to.  The handler then walks the
    // base-array tree and emits each leaf through the
    // printer (whose sink is `output`).
    //
    //   PROGRESS REPORTING:
    //   The printer sink is a string, so nothing reaches
    // the console until the final render at the bottom of
    // main().  In MSVC Debug-x64, the threadsafe / atomic
    // / cow concurrent tests can take many seconds because
    // _ITERATOR_DEBUG_LEVEL=2 makes every locked op
    // dramatically slower.  To make the run observable as
    // it progresses, we emit a short status line to stdout
    // before each module starts and dump that module's
    // portion of `output` to stdout immediately when its
    // run() returns.  The full report still renders at the
    // end via the master template; the stdout dumps are
    // additive, not a replacement.

    std::printf("[BEGIN] module: array.hpp  (base container)\n");
    std::fflush(stdout);

    output.append("\n========================================"
                  "========================================\n");
    output.append("  MODULE: array.hpp  (base container)\n");
    output.append("========================================"
                  "========================================\n");

    std::size_t mod_a_start = output.size();

    verdict_a = run_array_suite(handler,
                                /*_kind=*/        D_TEST_KIND_MODULE,
                                /*_out_seconds=*/ &seconds_a);

    totals_a = handler.result();

    // dump just this module's portion to stdout for live
    // progress; the full report still renders at the end
    std::fwrite(output.data() + mod_a_start,
                1,
                output.size() - mod_a_start,
                stdout);
    std::fflush(stdout);

    std::printf("[END]   module: array.hpp  verdict=%d  elapsed=%.3fs  "
                "leaves=%zu/%zu\n",
                static_cast<int>(verdict_a),
                seconds_a,
                static_cast<size_t>(totals_a.passed),
                static_cast<size_t>(totals_a.total));
    std::fflush(stdout);

    ++modules_total;
    modules_passed_count += verdict_passes(verdict_a);

    // -------------------------------------------------------------------
    // 3. run Part B:  threadsafe wrappers
    // -------------------------------------------------------------------
    //   Same handler, same printer: the next handler.run()
    // call resets the handler's session_result up front
    // (start_session reassigns it), so totals_b reflects
    // ONLY this run.  We accumulate into our own counters
    // before reading totals_b's snapshot.

    std::printf("[BEGIN] module: threadsafe wrappers  "
                "(threadsafe_array, atomic_array, cow_array)\n");
    std::fflush(stdout);

    output.append("\n========================================"
                  "========================================\n");
    output.append("  MODULE: threadsafe wrappers  "
                  "(threadsafe_array, atomic_array, cow_array)\n");
    output.append("========================================"
                  "========================================\n");

    std::size_t mod_b_start = output.size();

    verdict_b = run_threadsafe_array_suite(handler,
                                           /*_kind=*/        D_TEST_KIND_MODULE,
                                           /*_out_seconds=*/ &seconds_b);

    totals_b = handler.result();

    std::fwrite(output.data() + mod_b_start,
                1,
                output.size() - mod_b_start,
                stdout);
    std::fflush(stdout);

    std::printf("[END]   module: threadsafe wrappers  "
                "verdict=%d  elapsed=%.3fs  leaves=%zu/%zu\n",
                static_cast<int>(verdict_b),
                seconds_b,
                static_cast<size_t>(totals_b.passed),
                static_cast<size_t>(totals_b.total));
    std::fflush(stdout);

    ++modules_total;
    modules_passed_count += verdict_passes(verdict_b);

    // -------------------------------------------------------------------
    // 4. roll up suite-wide totals
    // -------------------------------------------------------------------

    seconds_total   = seconds_a + seconds_b;
    asserts_total   = totals_a.total  + totals_b.total;
    asserts_passed  = totals_a.passed + totals_b.passed;
    asserts_failed  = totals_a.failed + totals_b.failed;
    verdict_overall = worst_verdict(verdict_a, verdict_b);

    // -------------------------------------------------------------------
    // 5. format pass-rate strings
    // -------------------------------------------------------------------

    if (asserts_total > 0)
    {
        asserts_rate = ( 100.0 * static_cast<double>(asserts_passed) /
                                 static_cast<double>(asserts_total) );
    }

    if (modules_total > 0)
    {
        modules_rate = ( 100.0 * static_cast<double>(modules_passed_count) /
                                 static_cast<double>(modules_total) );
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
    // 6. bind the master-suite template
    // -------------------------------------------------------------------
    //   The "module" identity at the top of the report is
    // the SUITE name (the whole test binary), not any one
    // module within it.  The per-module banners and bodies
    // already live inside `output` (bound to %test_modules%).

    // -- header / identity --
    tpl.bind("module_name",         "djinterp / array");
    tpl.bind("module_description",
             "Combined test suite: array.hpp container plus the "
             "threadsafe wrappers (threadsafe_array, atomic_array, "
             "cow_array)");
    tpl.bind("module_path",
             "/inc/djinterp/core/container/array/");
    tpl.bind("timestamp_start",     __DATE__ " " __TIME__);
    tpl.bind("description_short",   "array suite");
    tpl.bind("description_long",
             "Tests for the array container family across all 8 "
             "lifetime x iterability cells, plus the three "
             "concurrency-strategy wrappers: locked, atomic, cow");
    tpl.bind("test_modules",        output);

    // -- module rollup (now real numbers, not "1") --
    tpl.bind("modules_tested",      print_context::size_to_string(modules_total));
    tpl.bind("modules_passed",      print_context::size_to_string(modules_passed_count));
    tpl.bind("modules_percent",     std::string(modules_rate_buf));

    // -- assertion totals (summed across runs) --
    tpl.bind("asserts_total",       print_context::size_to_string(asserts_total));
    tpl.bind("asserts_passed",      print_context::size_to_string(asserts_passed));
    tpl.bind("asserts_failed",      print_context::size_to_string(asserts_failed));
    tpl.bind("asserts_percent",     std::string(asserts_rate_buf));

    // -- unit-test totals (driven by the bound listeners
    //    above; both runs accumulate into the same `ut`
    //    state because the listeners are bound once and
    //    fired by both handler.run() calls) --
    char                tests_rate_buf[16];
    double              tests_rate;
    const std::size_t   tests_failed = ut.total - ut.passed;

    tests_rate = (ut.total > 0)
        ? ( 100.0 * static_cast<double>(ut.passed) /
                    static_cast<double>(ut.total) )
        : 0.0;

    std::snprintf(tests_rate_buf,
                  sizeof(tests_rate_buf),
                  "%.2f%%",
                  tests_rate);

    tpl.bind("tests_total",         print_context::size_to_string(ut.total));
    tpl.bind("tests_passed",        print_context::size_to_string(ut.passed));
    tpl.bind("tests_failed",        print_context::size_to_string(tests_failed));
    tpl.bind("tests_percent",       std::string(tests_rate_buf));

    // -- timing --
    tpl.bind("time_total",          std::string(time_buf));

    // -- overall verdict --
    tpl.bind("has_passed",          verdict_status_line(verdict_overall));

    // -------------------------------------------------------------------
    // 7. render and emit
    // -------------------------------------------------------------------

    rendered = tpl.render(D_TEST_TPL_MASTER_SUITE);

    std::fwrite(rendered.data(),
                1,
                rendered.size(),
                stdout);

    // -------------------------------------------------------------------
    // 8. exit code
    // -------------------------------------------------------------------
    //   EXIT_FAILURE only on real failure.  Pending suites
    // exit success because nothing actually broke; the
    // "incomplete" state is communicated through the
    // rendered report's status line, not the exit code.
    //   Flip the predicate if your CI policy is stricter
    // (e.g. require verdict_overall == passed for success).

    return (verdict_overall == session_verdict::failed)
        ? EXIT_FAILURE
        : EXIT_SUCCESS;
}