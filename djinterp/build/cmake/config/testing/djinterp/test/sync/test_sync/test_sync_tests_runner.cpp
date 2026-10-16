/******************************************************************************
* djinterp [tests]                                  test_sync_tests_runner.cpp
*
*   Thin runner for the test_sync suite: options only, then run_module.
*
* path:      /build/cmake/config/testing/djinterp/test/sync/test_sync_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "test_sync_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_sync_tests.pdf"));

    return dt::run_module(
        tt::sync_spec(),
        "test_sync.hpp unit tests",
        "Latch, barrier, gate, rendezvous, and simultaneous-start test "
        "coordination primitives.",
        opts);
}
