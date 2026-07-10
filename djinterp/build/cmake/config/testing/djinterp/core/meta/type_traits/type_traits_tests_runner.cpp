/******************************************************************************
* djinterp [test]                                 type_traits_tests_runner.cpp
*
*   Standalone runner for the type_traits suite.  It includes the single suite
* header with DTEST_SPEC_MODE defined - which drops the header's legacy
* whole-suite void() driver and leaves type_traits_spec() as the entry point -
* collects that one spec, and hands it to run_module.  run_module lowers the
* spec into the six-kind test tree (structure + per-node metadata) and projects
* it onto a report driving the console report and, here, one PDF.  The fifteen
* section translation units (detection / macros_* / logical / callable / cpp20
* / cpp23 / evaluate / rules / traits / arity / template / tuple), compiled
* WITHOUT DTEST_SPEC_MODE, supply the void(test_handler&) worker definitions the
* spec's function pointers resolve to; this file is only the spec collection and
* the entry point.
*
*   WHY THIS REPLACES THE OLD HAND-ROLLED main():
*   The previous runner pre-dated the spec bridge and open-coded everything the
* framework now does for free: a per-module timing/verdict lambda, suite-wide
* accumulators, and a hand-bound master-suite template.  All of that is now
* run_module's job.  Authoring the suite as a module_spec (type_traits_spec(),
* in the suite header) and projecting it into both framework views is the whole
* point, so this file collapses to option setup plus one call.
*
*   NOTE ON THE SPEC PROVIDER'S NAMESPACE:
*   Unlike the test_handler suite (whose provider lives in ::djinterp::testing),
* the type_traits suite lives entirely in ::djinterp::test, so its provider is
* dt::type_traits_spec() and a single namespace alias suffices.
*
*   OUTPUT VIA TEST_OPTIONS - ONE PDF:
*   The document side rides one option set (test_options.hpp):
*     document    -> pdf                     emit PDF (not the txt default)
*     output_file -> "type_traits_tests.pdf" the document's path
*   Leave `document` at its txt default (or drop these two lines) for a
*   console-only run.  Being a SINGLE module, there is nothing to split or
*   bundle, so no split / pack / archive_format is set.
*
*   FINDING THE FILE:
*   output_file is a RELATIVE name, so the .pdf lands in the process's current
* working directory - under Visual Studio the Debugging "Working Directory",
* NOT the folder holding the .exe.  Set output_file to an absolute path to pin
* it, or watch the console for the "wrote report: ..." line.
*
*   VERDICT / EXIT CODE:
*   run_module returns the report's process exit code (0 = every leaf passed),
* handed straight back from main().  A section whose worker records a failure
* or error turns its unit test red and the exit code non-zero.
*
* path:      /build/cmake/config/testing/djinterp/core/meta/type_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not the aggregate driver

// std
#include <string>
// djinterp
#include "../../../../../../../tests/djinterp/core/meta/type_traits_tests.hpp"  // resolved via the test include path

int
main()
{
    namespace dt = ::djinterp::test;

    // configure the report through the option set: console plus one PDF.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("type_traits_tests.pdf"));

    return dt::run_module(
        dt::type_traits_spec(),                           // the suite, as data
        "djinterp type_traits unit tests",                // report title
        "The compile-time metafunction library: the detection idiom, the "
        "D_TYPE_TRAIT_* macro toolkit, the portable standard-library traits, "
        "and the djinterp custom trait surface.",
        opts);                                            // options: one PDF
}
