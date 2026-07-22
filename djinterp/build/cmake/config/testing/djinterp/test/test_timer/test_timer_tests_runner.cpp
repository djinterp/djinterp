/******************************************************************************
* djinterp [test]                                test_timer_tests_runner.cpp
*
*   Entry point for the test_timer.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + timer_spec(), no fixtures);
* this TU contributes main() and the six section TUs the bodies.  main() only
* configures options and hands timer_spec() to run_module - all templating
* lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_timer_tests_runner.cpp
* author(s): TBA
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_timer_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_timer_tests.pdf"));

    return dt::run_module(
        tt::timer_spec(),
        "test_timer.hpp unit tests",
        "Event-emitting test timer over a deterministic clock.",
        opts);
}
