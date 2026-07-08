/******************************************************************************
* djinterp [functional]                                   view_test_runner.cpp
*
*   Entry point for the view.hpp unit-test binary. This is the main holder,
* not a reusable module: it pulls in the DTest section functions declared in
* view_tests.hpp and drives the whole suite inline from main().
*
*   WHAT main() DOES:
*   It walks a local manifest -- one entry per like-group semantic section
* of view.hpp -- and for each section: emits a banner, runs the section
* against its OWN fresh test_handler while timing it, reads the handler's
* per-section result delta, derives a verdict from the assertion / failure
* tally, prints a result line, captures the first failing section, and rolls
* the section's contribution into the suite-wide accumulators. After the walk
* it renders a summary block and returns a process exit code.
*
*   PER-SECTION ISOLATION:
*   Each section gets a brand-new test_handler rather than sharing one across
* the suite, so a section's assertion count and first failing assertion are
* its own. The suite totals are the integer sum of those per-section deltas
* -- no shadow accounting. This is the framework's documented "section
* delta" model (a fresh handler per section, read result() after), driven
* here directly rather than through run_session so the rendered report
* matches the predicate suite's shape.
*
*   RELATIONSHIP TO view_module_run_all / run_session:
*   view_tests.hpp also exposes the module_info + module_run_all pair for
* the framework's one-statement run_session() path. This runner deliberately
* does NOT use it: to mirror the lightweight predicate runner, it lists the
* sections in a local manifest and drives each one itself. The tradeoff is
* that the section list lives here as well as in view_module_run_all -- keep
* the two in step when sections are added or reordered. Note also that the
* framework's four-way verdict (passed / pending / failed / empty) collapses
* to the predicate suite's three-way model here: a pending-only section reads
* as passed.
*
*   VERDICT POLICY:
*   A section is `failed` if it observed any failing or errored assertion,
* `passed` if it observed at least one assertion and none failed, and `empty`
* if it observed no assertions at all. The suite verdict is the worst across
* its sections. Only a real `failed` verdict yields EXIT_FAILURE; `empty`
* sections do not break the build -- an unexercised section hasn't failed, it
* just had nothing to run -- so they exit success.
*
*   PORTABILITY:
*   C++11 minimum, inherited from the DTest framework. Timing uses std::clock
* so a single code path serves every standard. Attribute / constexpr /
* noexcept spellings route through the env.h chain pulled in transitively via
* view_tests.hpp.
*
*
* path:      /inc/functional/view_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.01
******************************************************************************/

// std
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
// djinterp
#include <djinterp/core/djinterp.hpp>
#include "../../../../../../../tests/djinterp/core/functional/view_tests.hpp"


using djinterp::test::test_handler;
using djinterp::test::session_result;
using djinterp::testing::test_core_traits;
using djinterp::testing::test_adapter_terminal_traits;
using djinterp::testing::test_ref_view;
using djinterp::testing::test_owning_view;
using djinterp::testing::test_iterator_pair_view;
using djinterp::testing::test_iota;
using djinterp::testing::test_repeat;
using djinterp::testing::test_generate;
using djinterp::testing::test_empty;
using djinterp::testing::test_single;
using djinterp::testing::test_transform;
using djinterp::testing::test_filter;
using djinterp::testing::test_take;
using djinterp::testing::test_drop;
using djinterp::testing::test_take_while;
using djinterp::testing::test_drop_while;
using djinterp::testing::test_enumerate;
using djinterp::testing::test_zip;
using djinterp::testing::test_concat;
using djinterp::testing::test_reverse;
using djinterp::testing::test_chunk;
using djinterp::testing::test_stride;
using djinterp::testing::test_pipeline_view_adapter;
using djinterp::testing::test_pipeline_container_lift;
using djinterp::testing::test_pipeline_chain;
using djinterp::testing::test_to_vector;
using djinterp::testing::test_to_container;
using djinterp::testing::test_count;
using djinterp::testing::test_fold;
using djinterp::testing::test_for_each;
using djinterp::testing::test_any_all_none;


// ---------------------------------------------------------------------------
//  file-internal verdict + reporting helpers
//   Kept in an unnamed namespace so they carry internal linkage and do not
// pollute the link-time symbol table. These mirror the verdict helpers the
// predicate runner spells out in its own main(), reading the framework
// handler's pass / fail / empty signal in place of the test_registry tally.
// ---------------------------------------------------------------------------
namespace {

    // verdict
    //   enum: three-way outcome of a section or of the suite as a whole. A
    // plain (unscoped) enum keeps the type usable identically from C++11
    // through C++20; the enumerators are prefixed to avoid collisions.
    enum verdict
    {
        verdict_empty  = 0,
        verdict_passed = 1,
        verdict_failed = 2
    };


    // verdict_status_line
    //   helper: human-readable tag for a verdict.
    //   NOT D_CONSTEXPR: a switch with multiple return statements is not a
    // valid constexpr body until C++14's relaxed rules, which the C++11
    // baseline rules out. D_NOEXCEPT is still safe.
    D_NODISCARD const char*
    verdict_status_line(
        verdict _verdict
    ) D_NOEXCEPT
    {
        switch (_verdict)
        {
            case verdict_passed:
                return "[PASS] ALL VIEW TESTS PASSED";

            case verdict_failed:
                return "[FAIL] SOME VIEW TESTS FAILED";

            case verdict_empty:
                return "[EMPTY] NO VIEW CHECKS OBSERVED";

            default:
                return "[?] UNKNOWN";
        }
    }


    // verdict_passes
    //   helper: 1 if _verdict counts as a pass for the sections-passed
    // rollup, else 0. `empty` is NOT a pass -- the passed tally is a strict
    // count of passed verdicts.
    D_NODISCARD D_CONSTEXPR std::size_t
    verdict_passes(
        verdict _verdict
    ) D_NOEXCEPT
    {
        return (_verdict == verdict_passed)
            ? static_cast<std::size_t>(1)
            : static_cast<std::size_t>(0);
    }


    // worst_verdict
    //   helper: combines two verdicts into the worse case. Decision order:
    //   failed  >  passed  >  empty
    D_NODISCARD D_CONSTEXPR verdict
    worst_verdict(
        verdict _a,
        verdict _b
    ) D_NOEXCEPT
    {
        return ( (_a == verdict_failed) ||
                 (_b == verdict_failed) )
            ? verdict_failed
            : ( ( (_a == verdict_passed) ||
                  (_b == verdict_passed) )
                ? verdict_passed
                : verdict_empty );
    }


    // verdict_from_counts
    //   helper: derives a section's verdict from its assertion / failure
    // tally. Any failure means failed; otherwise at least one assertion
    // means passed; no assertions at all means empty.
    D_NODISCARD D_CONSTEXPR verdict
    verdict_from_counts(
        std::size_t _checks,
        std::size_t _failures
    ) D_NOEXCEPT
    {
        return (_failures > 0)
            ? verdict_failed
            : ( (_checks > 0)
                ? verdict_passed
                : verdict_empty );
    }


    // safe_cstr
    //   helper: returns the input pointer, or "" if it is null. Used at
    // printf sites where handler / section string fields are not guaranteed
    // non-null.
    D_NODISCARD const char*
    safe_cstr(
        const char* _s
    ) D_NOEXCEPT
    {
        return (_s != 0) ? _s : "";
    }


    // first_failing_name
    //   helper: returns the name of the first non-passing assertion recorded
    // in _handler's sink this section, or 0 if every assertion passed. The
    // framework records assertion NAMES (not source locations), so this is
    // the handler-side analogue of the predicate registry's first-failure
    // expression.
    D_NODISCARD const char*
    first_failing_name(
        const test_handler& _handler
    )
    {
        std::size_t k;

        for (k = 0; k < _handler.sink().size(); ++k)
        {
            if (!_handler.sink()[k].passed())
            {
                return _handler.sink()[k].name();
            }
        }

        return 0;
    }


    // print_rule
    //   helper: writes one horizontal rule line to stdout. Built with a
    // loop rather than a long string literal so the source stays within
    // the line-width limit.
    void
    print_rule() D_NOEXCEPT
    {
        int i;

        for (i = 0; i < 78; ++i)
        {
            std::putchar('=');
        }

        std::putchar('\n');

        return;
    }

}  // unnamed namespace


int
main()
{
    // -------------------------------------------------------------------
    // 0. the section manifest (one entry per semantic section)
    // -------------------------------------------------------------------
    //   A local struct holds a (name, fn) pair; the array below is the
    // whole suite in document order. To run a subset, comment a line; to
    // reorder, swap lines; everything downstream keys off the array so it
    // adapts. Keep this list in step with view_module_run_all.

    struct section_entry
    {
        const char* name;
        void      (*fn)(test_handler&);
    };

    const section_entry sections[] =
    {
        { "core_traits", &test_core_traits },
        { "adapter/terminal_traits", &test_adapter_terminal_traits },
        { "ref_view", &test_ref_view },
        { "owning_view", &test_owning_view },
        { "iterator_pair_view", &test_iterator_pair_view },
        { "iota", &test_iota },
        { "repeat", &test_repeat },
        { "generate", &test_generate },
        { "empty", &test_empty },
        { "single", &test_single },
        { "transform", &test_transform },
        { "filter", &test_filter },
        { "take", &test_take },
        { "drop", &test_drop },
        { "take_while", &test_take_while },
        { "drop_while", &test_drop_while },
        { "enumerate", &test_enumerate },
        { "zip", &test_zip },
        { "concat", &test_concat },
        { "reverse", &test_reverse },
        { "chunk", &test_chunk },
        { "stride", &test_stride },
        { "pipeline_view_adapter", &test_pipeline_view_adapter },
        { "pipeline_container_lift", &test_pipeline_container_lift },
        { "pipeline_chain", &test_pipeline_chain },
        { "to_vector", &test_to_vector },
        { "to_container", &test_to_container },
        { "count", &test_count },
        { "fold", &test_fold },
        { "for_each", &test_for_each },
        { "any_all_none", &test_any_all_none },
    };

    // -------------------------------------------------------------------
    // 1. variable declarations
    // -------------------------------------------------------------------

    const char* suite_name;
    const char* suite_description;
    const char* suite_path;

    std::size_t sections_total;
    std::size_t sections_passed;
    std::size_t checks_total;
    std::size_t checks_failed;
    std::size_t checks_passed;
    double      seconds_total;
    verdict     overall;

    const char* first_fail_section;
    const char* first_fail_assert;

    double      sections_rate;
    double      checks_rate;

    std::size_t section_count;
    std::size_t i;

    // -------------------------------------------------------------------
    // 2. initialization
    // -------------------------------------------------------------------

    suite_name         = "djinterp / functional / view";
    suite_description  = "Core and adapter/terminal traits, the fundamental "
                         "views, the source views, the basic and combining "
                         "adapters, the pipeline operators, and the terminal "
                         "operators for view.hpp";
    suite_path         = "/inc/djinterp/core/functional/view.hpp";

    sections_total     = 0;
    sections_passed    = 0;
    checks_total       = 0;
    checks_failed      = 0;
    checks_passed      = 0;
    seconds_total      = 0.0;
    overall            = verdict_empty;

    first_fail_section = 0;
    first_fail_assert  = 0;

    sections_rate      = 0.0;
    checks_rate        = 0.0;

    section_count      = sizeof(sections) / sizeof(sections[0]);

    // -------------------------------------------------------------------
    // 3. run every section
    // -------------------------------------------------------------------

    for (i = 0; i < section_count; ++i)
    {
        test_handler   handler;
        session_result delta;
        std::clock_t   t0;
        std::clock_t   t1;
        double         seconds;
        std::size_t    checks;
        std::size_t    failures;
        const char*    fail_name;
        verdict        v;

        fail_name = 0;

        // banner so the live output shows which section follows
        std::printf("\n  --- %s ---\n", sections[i].name);
        std::fflush(stdout);

        // time and invoke the section against its own fresh handler; an
        // isolated handler gives this section its own assertion tally and
        // first failing assertion rather than sharing one across the suite
        t0 = std::clock();
        sections[i].fn(handler);
        t1 = std::clock();

        delta    = handler.result();
        seconds  = static_cast<double>(t1 - t0) /
                   static_cast<double>(CLOCKS_PER_SEC);
        checks   = delta.total;
        failures = delta.failed + delta.errors;
        v        = verdict_from_counts(checks, failures);

        // emit the per-section result line
        if (v == verdict_failed)
        {
            fail_name = first_failing_name(handler);

            std::printf("  [FAIL] %s: %lu checks, %lu failures (%.3fs)\n",
                        sections[i].name,
                        static_cast<unsigned long>(checks),
                        static_cast<unsigned long>(failures),
                        seconds);

            // show which assertion first went wrong
            std::printf("         first failing assertion: %s\n",
                        safe_cstr(fail_name));
        }
        else if (v == verdict_passed)
        {
            std::printf("  [PASS] %s: %lu checks, %lu failures (%.3fs)\n",
                        sections[i].name,
                        static_cast<unsigned long>(checks),
                        static_cast<unsigned long>(failures),
                        seconds);
        }
        else  // empty
        {
            std::printf("  [EMPTY] %s: no checks observed (%.3fs)\n",
                        sections[i].name,
                        seconds);
        }

        std::fflush(stdout);

        // capture the first failing section's first failing assertion, once
        if ( (v == verdict_failed) &&
             (first_fail_section == 0) )
        {
            first_fail_section = sections[i].name;
            first_fail_assert  = fail_name;
        }

        // roll the section's delta into the suite-wide accumulators
        ++sections_total;
        sections_passed += verdict_passes(v);
        checks_total    += checks;
        checks_failed   += failures;
        seconds_total   += seconds;
        overall          = worst_verdict(overall, v);
    }

    // -------------------------------------------------------------------
    // 4. compute pass-rate values
    // -------------------------------------------------------------------

    checks_passed = checks_total - checks_failed;

    if (sections_total > 0)
    {
        sections_rate = ( 100.0 * static_cast<double>(sections_passed) /
                                  static_cast<double>(sections_total) );
    }

    if (checks_total > 0)
    {
        checks_rate = ( 100.0 * static_cast<double>(checks_passed) /
                                static_cast<double>(checks_total) );
    }

    // -------------------------------------------------------------------
    // 5. render the summary block
    // -------------------------------------------------------------------

    // -- header / identity --
    std::printf("\n");
    print_rule();
    std::printf("  VIEW TEST SUITE SUMMARY\n");
    print_rule();
    std::printf("  suite:        %s\n", safe_cstr(suite_name));
    std::printf("  description:  %s\n", safe_cstr(suite_description));
    std::printf("  path:         %s\n", safe_cstr(suite_path));
    std::printf("  built:        %s %s\n", __DATE__, __TIME__);

    // -- rollups --
    std::printf("\n");
    std::printf("  sections:     %lu/%lu passed (%.2f%%)\n",
                static_cast<unsigned long>(sections_passed),
                static_cast<unsigned long>(sections_total),
                sections_rate);
    std::printf("  checks:       %lu/%lu passed (%.2f%%), %lu failed\n",
                static_cast<unsigned long>(checks_passed),
                static_cast<unsigned long>(checks_total),
                checks_rate,
                static_cast<unsigned long>(checks_failed));
    std::printf("  time:         %.3f seconds\n", seconds_total);

    // -- first failure, if any --
    if (first_fail_section != 0)
    {
        std::printf("\n  first failure in section '%s':\n",
                    first_fail_section);
        std::printf("    assertion: %s\n", safe_cstr(first_fail_assert));
    }

    // -- overall verdict --
    std::printf("\n  %s\n", verdict_status_line(overall));
    print_rule();
    std::fflush(stdout);

    // -------------------------------------------------------------------
    // 6. exit code
    // -------------------------------------------------------------------
    //   EXIT_FAILURE only on a real failure. An empty suite exits success
    // because nothing actually broke; the "no checks" state is communicated
    // through the summary's status line, not the exit code.

    return (overall == verdict_failed)
        ? EXIT_FAILURE
        : EXIT_SUCCESS;
}
