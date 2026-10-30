/******************************************************************************
* djinterp [tests]                            test_threadsafe_tests_runner.cpp
*
*   Thin runner for the test_threadsafe umbrella suite: options only, then
* run_module.
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/sync/test_threadsafe/test_threadsafe_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "test_threadsafe_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_threadsafe_tests.pdf"));

    return dt::run_module(
        tt::threadsafe_spec(),
        "test_threadsafe.hpp unit tests",
        "Umbrella reachability, the folded trait surface, the production "
        "threadsafe types, and cross-module integration.",
        opts);
}
