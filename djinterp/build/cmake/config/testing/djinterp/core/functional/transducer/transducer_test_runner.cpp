/******************************************************************************
* djinterp [functional]                             transducer_test_runner.cpp
*
*   Entry point for the transducer.hpp unit-test binary. This is the main
* holder, not a reusable module: it pulls in the section runners declared in
* transducer_tests.hpp and drives the whole suite inline from main().
*
*   WHAT main() DOES:
*   It walks a local manifest -- one entry per like-group semantic section
* of filter.hpp -- and for each section: emits a banner, runs the section's
* `bool run_*_tests()` runner while timing it, prints a pass / fail result
* line, and rolls the section's outcome into the suite-wide accumulators.
* After the walk it renders a summary block and returns a process exit code.
*
*   WHY NOT run_all_transducer_tests():
*   transducer_tests.hpp supplies an inline `run_all_transducer_tests()`
* aggregate, but it folds the sections with `&&` and therefore short-circuits
* at the first failing section -- handy for a bare pass/fail answer, but it
* hides which later sections would also have failed and prints nothing. This
* runner instead calls each section runner unconditionally so every section
* reports, then derives the verdict from the full tally.
*
*   VERDICT MODEL:
*   The section runners answer a plain bool (true == all checks in the
* section passed), so a section is simply `passed` or `failed` -- there is
* no check-count or "empty" distinction to surface here. The suite is
* `failed` if any section returned false, else `passed`. Only a failed
* suite yields EXIT_FAILURE.
*
*   PORTABILITY:
*   Tracks transducer_tests.hpp: compiles and runs C++11 through C++20 (the
* fixtures use named functors so trait sections are well-formed pre-C++20).
* The runner itself stays in the C++98 subset -- named types, std::clock for
* timing, plain enums, `%lu` with explicit casts -- so it imposes nothing
* beyond what the suite already requires. Attribute / constexpr / noexcept
* spellings route through the env.h chain pulled in transitively via
* transducer_tests.hpp.
*
*
* path:      /inc/functional/transducer_test_runner.cpp
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
#include "../../../../../../../tests/djinterp/core/functional/transducer_tests.hpp"


using djinterp::testing::run_state_tests;
using djinterp::testing::run_core_tests;
using djinterp::testing::run_core2_tests;
using djinterp::testing::run_composition_tests;
using djinterp::testing::run_drivers_tests;
using djinterp::testing::run_traits_tests;


// ---------------------------------------------------------------------------
//  file-internal verdict + reporting helpers
//   Kept in an unnamed namespace so they carry internal linkage and do not
// pollute the link-time symbol table.
// ---------------------------------------------------------------------------
namespace {

    // verdict
    //   enum: two-way outcome of a section or of the suite. A plain
    // (unscoped) enum keeps the type usable identically from C++11 through
    // C++20; the section runners answer bool, so there is no third state to
    // model. Enumerators are prefixed to avoid collisions.
    enum verdict
    {
        verdict_passed = 0,
        verdict_failed = 1
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
                return "[PASS] ALL TRANSDUCER TESTS PASSED";

            case verdict_failed:
                return "[FAIL] SOME TRANSDUCER TESTS FAILED";

            default:
                return "[?] UNKNOWN";
        }
    }


    // worst_verdict
    //   helper: combines two verdicts into the worse case (failed
    // dominates).
    D_NODISCARD D_CONSTEXPR verdict
    worst_verdict(
        verdict _a,
        verdict _b
    ) D_NOEXCEPT
    {
        return ( (_a == verdict_failed) ||
                 (_b == verdict_failed) )
            ? verdict_failed
            : verdict_passed;
    }


    // safe_cstr
    //   helper: returns the input pointer, or "" if it is null.
    D_NODISCARD const char*
    safe_cstr(
        const char* _s
    ) D_NOEXCEPT
    {
        return (_s != 0) ? _s : "";
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
    // adapts.

    struct section_entry
    {
        const char* name;
        bool      (*fn)();
    };

    const section_entry sections[] =
    {
        { "state       (reduced/reducing_state)", &run_state_tests },
        { "core        (map/filter/take/drop)", &run_core_tests },
        { "core2       (take_while/.../flat_map)", &run_core2_tests },
        { "composition (compose/pipe/into)", &run_composition_tests },
        { "drivers     (transduce/into_*)", &run_drivers_tests },
        { "traits      (is_reducer/concepts)", &run_traits_tests },
    };

    // -------------------------------------------------------------------
    // 1. variable declarations
    // -------------------------------------------------------------------

    const char* suite_name;
    const char* suite_description;
    const char* suite_path;

    std::size_t sections_total;
    std::size_t sections_passed;
    std::size_t sections_failed;
    double      seconds_total;
    verdict     overall;

    const char* first_fail_section;

    double      sections_rate;

    std::size_t section_count;
    std::size_t i;

    // -------------------------------------------------------------------
    // 2. initialization
    // -------------------------------------------------------------------

    suite_name         = "djinterp / functional / transducer";
    suite_description  = "reduced and reducing_state, the core transducers, "
                         "the second core group, composition, the drivers, "
                         "and structural traits for transducer.hpp (a C++14+ "
                         "module; under C++11 the suite compiles to "
                         "trivially-passing stubs)";
    suite_path         = "/inc/functional/transducer.hpp";

    sections_total     = 0;
    sections_passed    = 0;
    sections_failed    = 0;
    seconds_total      = 0.0;
    overall            = verdict_passed;

    first_fail_section = 0;

    sections_rate      = 0.0;

    section_count      = sizeof(sections) / sizeof(sections[0]);

    // -------------------------------------------------------------------
    // 3. run every section
    // -------------------------------------------------------------------

    for (i = 0; i < section_count; ++i)
    {
        std::clock_t t0;
        std::clock_t t1;
        double       seconds;
        bool         ok;
        verdict      v;

        seconds = 0.0;

        // banner so the live output shows which section follows
        std::printf("\n  --- %s ---\n", sections[i].name);
        std::fflush(stdout);

        // time and invoke the section runner
        t0 = std::clock();
        ok = sections[i].fn();
        t1 = std::clock();

        seconds = static_cast<double>(t1 - t0) /
                  static_cast<double>(CLOCKS_PER_SEC);
        v       = ok ? verdict_passed : verdict_failed;

        // emit the per-section result line
        if (v == verdict_failed)
        {
            std::printf("  [FAIL] %s (%.3fs)\n", sections[i].name, seconds);
        }
        else
        {
            std::printf("  [PASS] %s (%.3fs)\n", sections[i].name, seconds);
        }

        std::fflush(stdout);

        // remember the first failing section for the summary, once
        if ( (v == verdict_failed) &&
             (first_fail_section == 0) )
        {
            first_fail_section = sections[i].name;
        }

        // roll the section's outcome into the suite-wide accumulators
        ++sections_total;
        if (v == verdict_passed)
        {
            ++sections_passed;
        }
        else
        {
            ++sections_failed;
        }
        seconds_total += seconds;
        overall        = worst_verdict(overall, v);
    }

    // -------------------------------------------------------------------
    // 4. compute pass-rate value
    // -------------------------------------------------------------------

    if (sections_total > 0)
    {
        sections_rate = ( 100.0 * static_cast<double>(sections_passed) /
                                  static_cast<double>(sections_total) );
    }

    // -------------------------------------------------------------------
    // 5. render the summary block
    // -------------------------------------------------------------------

    // -- header / identity --
    std::printf("\n");
    print_rule();
    std::printf("  TRANSDUCER TEST SUITE SUMMARY\n");
    print_rule();
    std::printf("  suite:        %s\n", safe_cstr(suite_name));
    std::printf("  description:  %s\n", safe_cstr(suite_description));
    std::printf("  path:         %s\n", safe_cstr(suite_path));
    std::printf("  built:        %s %s\n", __DATE__, __TIME__);

    // -- rollups --
    std::printf("\n");
    std::printf("  sections:     %lu/%lu passed (%.2f%%), %lu failed\n",
                static_cast<unsigned long>(sections_passed),
                static_cast<unsigned long>(sections_total),
                sections_rate,
                static_cast<unsigned long>(sections_failed));
    std::printf("  time:         %.3f seconds\n", seconds_total);

    // -- first failure, if any --
    if (first_fail_section != 0)
    {
        std::printf("\n  first failing section: %s\n", first_fail_section);
    }

    // -- overall verdict --
    std::printf("\n  %s\n", verdict_status_line(overall));
    print_rule();
    std::fflush(stdout);

    // -------------------------------------------------------------------
    // 6. exit code
    // -------------------------------------------------------------------

    return (overall == verdict_failed)
        ? EXIT_FAILURE
        : EXIT_SUCCESS;
}
