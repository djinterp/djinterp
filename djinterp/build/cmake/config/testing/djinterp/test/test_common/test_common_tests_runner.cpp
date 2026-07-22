/******************************************************************************
* djinterp [test]                                 test_common_tests_runner.cpp
*
*   Entry point for the test_common.hpp suite.  Defining DTEST_SPEC_MODE
* selects the header's spec-provider face (declarations + common_spec(), no
* fixtures); this TU contributes main() and the four section TUs the bodies.
* main() only configures options and hands common_spec() to run_module - all
* templating lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_common_tests_runner.cpp
* author(s): DTest contributors
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_common_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // console plus one PDF; leave `document` at its txt default for console-only.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_common_tests.pdf"));

    return dt::run_module(
        tt::common_spec(),
        "test_common.hpp unit tests",
        "Foundational test types, status classification, and events.",
        opts);
}
