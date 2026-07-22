/******************************************************************************
* djinterp [test]                               test_session_tests_runner.cpp
*
*   Entry point for the test_session.hpp suite.  Defining DTEST_SPEC_MODE
* selects the header's spec-provider face (declarations + session_spec(), no
* fixtures); this TU contributes main() and the five section TUs the bodies.
* main() only configures options and hands session_spec() to run_module - all
* templating lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_session_tests_runner.cpp
* author(s): djinterp test-suite
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_session_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_session_tests.pdf"));

    return dt::run_module(
        tt::session_spec(),
        "test_session.hpp unit tests",
        "Top-level run context: tree, counters, timer, lifecycle, and verdict.",
        opts);
}
