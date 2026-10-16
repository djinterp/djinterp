/******************************************************************************
* djinterp [tests]                                  test_race_tests_runner.cpp
*
*   Thin runner for the test_race suite: options only, then run_module.
*
* path:      /build/cmake/config/testing/djinterp/test/sync/test_race_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "test_race_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_race_tests.pdf"));

    return dt::run_module(
        tt::race_spec(),
        "test_race.hpp unit tests",
        "Race probe, atomicity observer, linearization log, and the "
        "sequential-consistency checker.",
        opts);
}
