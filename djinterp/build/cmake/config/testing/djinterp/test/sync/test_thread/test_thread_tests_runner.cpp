/******************************************************************************
* djinterp [tests]                              test_thread_tests_runner.cpp
*
*   Thin runner for the test_thread suite.  Spec mode selects the suite
* header's provider face; the runner does nothing but build the option set and
* hand the spec to run_module.
*
* path:      /build/cmake/config/testing/djinterp/test/sync/test_thread_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_thread_tests.hpp"   // resolved via the INCLUDES the CMakeLists adds


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_thread_tests.pdf"));

    return dt::run_module(
        tt::thread_spec(),
        "test_thread.hpp unit tests",
        "Exception-capturing thread wrapper, group, factories, and folded "
        "threadsafe-testable traits/concepts.",
        opts);
}
