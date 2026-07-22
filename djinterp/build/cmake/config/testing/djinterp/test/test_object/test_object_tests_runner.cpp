/******************************************************************************
* djinterp [test]                                test_object_tests_runner.cpp
*
*   Entry point for the test_object.hpp suite.  Defining DTEST_SPEC_MODE
* selects the header's spec-provider face (declarations + object_spec(), no
* fixtures); this TU contributes main() and the eight section TUs the bodies.
* main() only configures options and hands object_spec() to run_module - all
* templating lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_object_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_object_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_object_tests.pdf"));

    return dt::run_module(
        tt::object_spec(),
        "test_object.hpp unit tests",
        "Unified test object: type identity, result, status, and metadata.",
        opts);
}
