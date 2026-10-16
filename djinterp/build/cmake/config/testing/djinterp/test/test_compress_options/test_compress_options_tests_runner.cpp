/******************************************************************************
* djinterp [tests]                       test_compress_options_tests_runner.cpp
*
*   The runner for the test_compress_options.hpp suite: a thin main() that
* selects the suite header's provider face, builds a test_option_set, and hands
* the spec to run_module.  It formats nothing itself - run_module drives the
* console report and (per the options below) renders the run's own PDF.
*
*   The module under test is header-only, link-free and needs no compile
* definitions, so this runner carries no options beyond the document request.
*
* path:      /build/cmake/config/testing/djinterp/test/
*                test_compress_options/test_compress_options_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#define DTEST_SPEC_MODE   // the suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_compress_options_tests.hpp"   // resolved via the INCLUDES the
                                             //   CMakeLists adds (tests dir)

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_compress_options_tests.pdf"));

    return dt::run_module(
        tt::compress_options_spec(),
        "test_compress_options.hpp unit tests",
        "deep comparison, pristine-default predicates, the field-level diff "
        "description and the one-knob builders for the fifty-field "
        "compression option vocabulary",
        opts);
}
