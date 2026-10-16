/******************************************************************************
* djinterp [tests]                              test_deadlock_tests_runner.cpp
*
*   Thin runner for the test_deadlock suite: options only, then run_module.
*
* path:      /build/cmake/config/testing/djinterp/test/sync/test_deadlock_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "test_deadlock_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_deadlock_tests.pdf"));

    return dt::run_module(
        tt::deadlock_spec(),
        "test_deadlock.hpp unit tests",
        "Deadlock events and reports, the watchdog timer, the lock-order "
        "cycle tracker, and the RAII watchdog scope.",
        opts);
}
