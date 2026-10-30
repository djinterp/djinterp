/******************************************************************************
* djinterp [tests]                                test_archive_tests_runner.cpp
*
*   The runner for the test_archive.hpp suite: a thin main() that selects the
* suite header's provider face, builds a test_option_set, and hands the spec to
* run_module.  It formats nothing itself - run_module drives the console report
* and (per the options below) renders the run's own PDF.
*
*   The module under test is header-only but exercises the archive facade, so
* this target links whatever the framework provides for archive.cpp.  No
* assertion is gated on format_is_writable - see the FINDING in the suite
* header explaining why that query and try_archive answer different questions.
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/test_archive/test_archive_tests_runner.cpp
*                test_archive/test_archive_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#define DTEST_SPEC_MODE   // the suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_archive_tests.hpp"   // resolved via the INCLUDES the
                                    //   CMakeLists adds (the tests dir)

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_archive_tests.pdf"));

    return dt::run_module(
        tt::archive_spec(),
        "test_archive.hpp unit tests",
        "entry construction, slash-tolerant list inspection, payload "
        "comparison, the container sniffers driven by hand-built bytes, and "
        "the round-trip drivers",
        opts);
}
