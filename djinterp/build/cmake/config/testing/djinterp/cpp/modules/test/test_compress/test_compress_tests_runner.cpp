/******************************************************************************
* djinterp [tests]                               test_compress_tests_runner.cpp
*
*   The runner for the test_compress.hpp suite: a thin main() that selects the
* suite header's provider face, builds a test_option_set, and hands the spec to
* run_module.  It formats nothing itself - run_module drives the console report
* and (per the options below) renders the run's own PDF.
*
*   The module under test is header-only but exercises the compression facade,
* so this target links whatever the framework provides for compress.cpp.  Every
* assertion is written to hold whichever codecs this build compiled; see the
* BUILD-AGNOSTIC note in the suite header.
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/test_compress/test_compress_tests_runner.cpp
*                test_compress/test_compress_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#define DTEST_SPEC_MODE   // the suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_compress_tests.hpp"   // resolved via the INCLUDES the
                                     //   CMakeLists adds (the tests dir)

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_compress_tests.pdf"));

    return dt::run_module(
        tt::compress_spec(),
        "test_compress.hpp unit tests",
        "the shared verification surface for compressed output: deterministic "
        "payload corpora, the magic-byte predicates, and the round-trip and "
        "decode checks, asserted on both sides of the availability split",
        opts);
}
