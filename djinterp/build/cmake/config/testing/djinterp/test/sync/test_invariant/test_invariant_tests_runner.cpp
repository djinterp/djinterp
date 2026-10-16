/******************************************************************************
* djinterp [tests]                             test_invariant_tests_runner.cpp
*
*   Thin runner for the test_invariant suite: options only, then run_module.
*
* path:      /build/cmake/config/testing/djinterp/test/sync/test_invariant_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "test_invariant_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_invariant_tests.pdf"));

    return dt::run_module(
        tt::invariant_spec(),
        "test_invariant.hpp unit tests",
        "Invariant reporting, the polling monitor, monotonic and bounded "
        "guards, the RAII monitoring scope, and the monitor factory.",
        opts);
}
