/******************************************************************************
* djinterp [test]                                    concepts_tests_runner.cpp
*
*   Entry point for the DTest concepts test suite.  Configures a
* test_printer, attaches it to a default_test_handler via the
* listener-bundle wiring, runs every per-section module of the
* concepts suite in order, and returns EXIT_SUCCESS or EXIT_FAILURE
* to the host environment.
*
*   ARCHITECTURE NOTES:
*   The runner takes a test_handler reference (via default_test_handler).
* The handler owns the printer wiring internally: set_printer() installs
* a bundle of listeners on the handler's event_handler whose bodies
* forward lifecycle events into the printer.  Counters are read via
* handler.result() after each module run completes.
*
*   The per-section test functions in concepts_tests.hpp have the
* uniform signature `void(test_handler&)`.  They record their
* assertions directly into the handler via record_assertion(), which
* in turn updates session_result counters and pushes basic_test nodes
* into the handler's internal sink.  Because they don't go through
* handler.run(tree), the per-module isolation provided by start_session
* in the array suite doesn't apply automatically here -- we call
* handler.reset_counters() before each module instead, then snapshot
* handler.result() after to capture that module's totals.
*
*   MULTI-MODULE PROTOCOL:
*   We drive thirteen independent runs against the same printer / sink,
* one per semantic section of concepts.hpp:
*     1.  macros        - section 0: D_CONCEPT_HAS_* macro family
*                         (METHOD, METHOD_ARGS, METHOD_TYPED,
*                         METHOD_ARGS_TYPED, TYPE, STATIC_MEMBER)
*     2.  std_reexports - sections I.1 - I.4: standard library
*                         concept re-exports (same_as, derived_from,
*                         integral / floating_point / signed_integral /
*                         unsigned_integral, comparison, object, and
*                         callable concepts)
*     3.  fundamental   - section II.1: is_void_c, is_null_pointer_c,
*                         nonvoid, is_array_c, bounded_array,
*                         unbounded_array, is_enum_c, scoped_enum,
*                         unscoped_enum, is_pointer_c,
*                         is_member_pointer_c, is_lvalue_reference_c,
*                         is_rvalue_reference_c, is_reference_c,
*                         is_class_c, is_union_c, is_function_c
*     4.  composite     - section II.2: is_arithmetic_c,
*                         is_fundamental_c, is_scalar_c, is_object_c,
*                         is_compound_c
*     5.  properties    - section II.3: is_const_c, is_volatile_c,
*                         is_trivial_c, is_trivially_copyable_c,
*                         is_standard_layout_c, pod_type, is_empty_c,
*                         is_polymorphic_c, is_abstract_c, is_final_c,
*                         is_aggregate_c
*     6.  tuple         - section II.4: is_tuple_c, tuple_like,
*                         homogeneous_tuple, empty_tuple,
*                         nonempty_tuple, single_element_tuple
*     7.  rules         - section II.5: follows_rule_of_zero_c,
*                         follows_rule_of_three_c,
*                         follows_rule_of_five_c
*     8.  container     - section II.6: has_value_type, has_size_type,
*                         has_iterator, has_const_iterator, sizeable,
*                         has_max_size_c, bounded_c, allocator_c
*     9.  templates     - section II.7: has_nested_template_type_c,
*                         has_variadic_constructor_c,
*                         template_parameter_base_of, is_template_c,
*                         is_template_with_args_c
*     10. logical       - section II.8: all_of, any_of, none_of,
*                         exactly_one_of, xor_of
*     11. invocable     - section II.9: invocable_r, nothrow_invocable,
*                         nothrow_invocable_r
*     12. numeric       - section II.10: valid_size_type, nonzero_size,
*                         zero_size
*     13. pack          - section II.11: single_type, empty_pack,
*                         nonempty_pack, all_same, all_convertible_to,
*                         all_derived_from, single_type_arg,
*                         single_tuple_arg, nonvoid_pack
*
*   Per-module totals are captured between calls so that the rendered
* report's MODULE SUMMARY block can report a real count (Modules Tested:
* 13) rather than a constant.  Assertion totals are summed across runs.
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
* (session_result) and the sink (basic_test nodes) without going
* through lifecycle events.
*
*   PORTABILITY:
*   C++20 minimum -- concepts are a C++20 feature, so the entire suite
* is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS.  All standard-version
* gating, attributes, and noexcept resolution route through the env.h
* chain (env.h -> env_cpp_features.h -> env_attributes.h), pulled in
* transitively via djinterp.hpp and the test headers.  Helper
* functions are decorated with D_NODISCARD / D_NOEXCEPT / D_CONSTEXPR
* so the macros expand to the strongest spelling supported by the
* detected toolchain.
*
*
* path:      /.config/.msvs/testing/core/meta/concepts/main
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
#include <djinterp/test/test_defaults.hpp>
#include <djinterp/test/test_handler.hpp>
#include <djinterp/test/test_printer.hpp>
#include "../../../../../../../tests/djinterp/core/meta/concepts_tests.hpp"

// feature gates
//   the test framework itself enforces C++11 in test_handler.hpp,
// but the concepts SUITE itself requires C++20.  We re-assert here
// so a misconfigured build of just the main translation unit produces
// a clean diagnostic at this site rather than deep inside the
// concepts_tests.hpp expansion.
//#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
//    #error "concepts_tests_runner.cpp requires C++20 concepts support"
//#endif


using namespace djinterp::test;

// ---------------------------------------------------------------------------
//  verdict helpers
//   File-internal mappings from session_verdict to the
// strings the master-suite template binds, plus a small
// helper that derives a per-module verdict from a captured
// session_result snapshot (since our per-section functions
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
    //   Three- or higher-way rollups compose pairwise:
    //   worst_verdict(worst_verdict(a, b), c).
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


    // verdict_from_result
    //   helper: derives a session_verdict from a session_result
    // snapshot.  Mirrors the decision order used by
    // session_result::verdict() inside the test_handler:
    // failed > pending > passed > empty.
    //   We need this here because the concepts per-section
    // functions return void, not a verdict.  We snapshot
    // handler.result() after each call and feed it through
    // this helper to recover the same answer.
    D_NODISCARD D_CONSTEXPR session_verdict
    verdict_from_result(
        const session_result& _r
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
    // parameters.  Each per-section concepts function is
    // `void(handler&)`, so we time it externally, snapshot
    // handler.result() before / after, and derive a per-module verdict
    // from the delta.
    //
    //   We use handler.reset_counters() to zero session_result before
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
        const session_result  totals  = handler.result();
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
    //   Order matches concepts.hpp's section layout.  Comment out any
    // line to skip that section; nothing else needs to change.
    run_module("macros",          concepts_tests_macros);
    run_module("std_reexports",   concepts_tests_std_reexports);
    run_module("fundamental",     concepts_tests_fundamental);
    run_module("composite",       concepts_tests_composite);
    run_module("properties",      concepts_tests_properties);
    run_module("tuple",           concepts_tests_tuple);
    run_module("rules",           concepts_tests_rules);
    run_module("container",       concepts_tests_container);
    run_module("templates",       concepts_tests_templates);
    run_module("logical",         concepts_tests_logical);
    run_module("invocable",       concepts_tests_invocable);
    run_module("numeric",         concepts_tests_numeric);
    run_module("pack",            concepts_tests_pack);

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
    tpl.bind("module_name",         "djinterp / concepts");
    tpl.bind("module_description",
             "Tests for the concepts metafunction module: the "
             "D_CONCEPT_HAS_* macro family (section 0), every "
             "standard-library concept re-export (sections I.1 - I.4), "
             "and the eleven djinterp custom-concept categories in "
             "section II - fundamental, composite, property, tuple, "
             "class-definition-rule, container, template, logical, "
             "invocable, numeric, and pack concepts");
    tpl.bind("module_path",
             "/inc/djinterp/core/meta/concepts.hpp");
    tpl.bind("timestamp_start",     __DATE__ " " __TIME__);
    tpl.bind("description_short",   "concepts suite");
    tpl.bind("description_long",
             "Tests for the concepts compile-time metafunction "
             "library, covering thirteen semantic sections from the "
             "macro family through the parameter-pack concepts.  The "
             "entire concepts surface is compile-time machinery, so "
             "each section's static_assert block validates the "
             "concept satisfaction and is mirrored by a runtime "
             "record_assertion of the same predicate for inclusion "
             "in the framework's report.  Special attention is paid "
             "to xor_of and single_tuple_arg, the two concepts added "
             "during the type_traits / concepts surface audit");

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
    //   For concepts we treat each section as one unit test;
    // assertion-level rollup happens via the counter delta on
    // handler.result().  The array runner derives this from event
    // listeners; here we can lift the numbers directly from the
    // module accumulators.
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
    tpl.bind("time_total", std::string(time_buf));

    // -- overall verdict --
    tpl.bind("has_passed", verdict_status_line(verdict_overall));

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