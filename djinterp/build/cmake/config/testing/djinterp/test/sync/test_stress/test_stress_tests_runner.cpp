/******************************************************************************
* djinterp [tests]                                test_stress_tests_runner.cpp
*
*   Thin runner for the test_stress suite: options only, then run_module.
*
* path:      /build/cmake/config/testing/djinterp/test/sync/test_stress_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "test_stress_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_stress_tests.pdf"));

    return dt::run_module(
        tt::stress_spec(),
        "test_stress.hpp unit tests",
        "Stress reporting, fixed-iteration and timed runners, the operation "
        "descriptor, and the weighted chaos runner.",
        opts);
}
