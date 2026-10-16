/******************************************************************************
* djinterp [test]                             event_dispatcher_tests_runner.cpp
*
*   Enriched runner for the event_dispatcher unit tests.  The suite describes
* itself: event_dispatcher_tests.hpp, compiled with DTEST_SPEC_MODE, exposes
* event_dispatcher_spec() - the suite's blocks as data, each unit test paired with
* a name and a one-line descriptor.  run_module (test_defaults.hpp) lowers that
* spec into the six-kind tree, describes it, and drives the report to the
* console and - as configured by a test_option_set - a PDF.  This file is only
* the entry point.
*
*   OUTPUT VIA TEST_OPTIONS:
*   Rather than the bare-filename convenience, this runner builds a
* test_option_set (test_options.hpp) and selects the PDF face on it:
*     opts.set<test_option::document>(test_doc_type::pdf);        // emit a PDF
*     opts.set<test_option::output_file>("event_dispatcher_tests.pdf");
* then hands the set to run_module's option-set overload.  document /
* output_file / split / show now ride the one configuration surface, so tuning
* any of them (split -> per_module, show -> failures_only, ...) is a one-line
* change here and needs no runner-shape edit.  Leave `document` at its txt
* default for a console-only run.
*
*   DTEST_SPEC_MODE swaps the header's fixtures for its spec provider, so the
* runner pulls in the authoring surface without the section-file fixtures.
*
* path:      /build/cmake/config/testing/djinterp/core/event/event_dispatcher/event_dispatcher_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.01
******************************************************************************/

#define DTEST_SPEC_MODE                    // expose event_dispatcher_spec(); omit fixtures
#include "event_dispatcher_tests.hpp"     // resolved via the test include path

#include <string>


int
main()
{
    namespace dt = ::djinterp::test;

    // configure the report through the option set: PDF, to a named file.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("event_dispatcher_tests.pdf"));

    return dt::run_module(
        ::djinterp::testing::event_dispatcher_spec(),
        "event_dispatcher.hpp unit tests",
        "enriched: six-kind vocabulary, per-node metadata",
        opts);
}
