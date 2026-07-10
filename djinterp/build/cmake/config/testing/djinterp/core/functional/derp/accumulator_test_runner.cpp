/******************************************************************************
* djinterp [test]                                    accumulator_test_runner.cpp
*
*   Enriched runner for the accumulator.hpp unit tests.  The suite describes
* itself: accumulator_tests.hpp, compiled with DTEST_SPEC_MODE, exposes
* accumulator_spec() - the suite's blocks as data, each unit test paired with
* a name and a one-line descriptor.  run_module (test_defaults.hpp) lowers that
* spec into the six-kind tree, describes it, and drives the report to the
* console and - as configured by a test_option_set - a PDF.  This file is only
* the entry point.
*
*   WHY THE SPEC MODEL (vs the old &&-folded manifest walk):
*   The previous runner walked a hand-rolled (name, fn) array and printed a
* summary by hand.  The framework now takes that same idea further: the suite
* is authored once as a module_spec (block -> unit -> predicate, with a name +
* descriptor on every node) and projected into BOTH the six-kind tree (view 1,
* structure + per-node metadata) and the report / PDF (view 2, the roll-up the
* document renders).  Each of the ten sections becomes one block; each
* `bool test_*()` predicate becomes one unit_test wrapping a test_fn leaf, run
* exactly once for its verdict.  The section runners (run_*_tests) and
* run_all_accumulator_tests() are retained in the header for the C++98-subset
* build and for a bare pass/fail check, but the spec is the primary surface.
*
*   OUTPUT VIA TEST_OPTIONS:
*   Rather than the bare-filename convenience, this runner builds a
* test_option_set (test_options.hpp) and selects the PDF face on it:
*     opts.set<test_option::document>(test_doc_type::pdf);        // emit a PDF
*     opts.set<test_option::output_file>("accumulator_tests.pdf");
* then hands the set to run_module's option-set overload.  document /
* output_file / split / show now ride the one configuration surface, so tuning
* any of them (split -> per_module, show -> failures_only, ...) is a one-line
* change here and needs no runner-shape edit.  Leave `document` at its txt
* default for a console-only run.
*
*   DTEST_SPEC_MODE swaps the header's fixtures for its spec provider, so the
* runner pulls in the authoring surface without the section-file fixtures; the
* `bool test_*()` definitions are linked from the compiled section .cpp files.
*
*   PORTABILITY:
*   C++11 minimum, inherited from the DTest framework (the module_spec /
* run_module / test_option_set surface is C++11+).  The accumulator module and
* its section tests otherwise track their own baselines; only this driver
* requires the framework floor.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/accumulator/accumulator_test_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#define DTEST_SPEC_MODE                    // expose accumulator_spec(); omit fixtures
#include "accumulator_tests.hpp"           // resolved via the test include path

#include <string>


int
main()
{
    namespace dt = ::djinterp::test;

    // configure the report through the option set: PDF, to a named file.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("accumulator_tests.pdf"));

    return dt::run_module(
        ::djinterp::testing::accumulator_spec(),
        "accumulator.hpp unit tests",
        "enriched: six-kind vocabulary, per-node metadata",
        opts);
}
