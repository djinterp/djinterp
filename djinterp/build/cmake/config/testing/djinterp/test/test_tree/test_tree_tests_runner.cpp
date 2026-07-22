/******************************************************************************
* djinterp [test]                                  test_tree_tests_runner.cpp
*
*   Entry point for the test_tree.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + tree_spec(), no fixtures);
* this TU contributes main() and the seven section TUs the bodies.  main() only
* configures options and hands tree_spec() to run_module - all templating lives
* in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_tree_tests_runner.cpp
* author(s): djinterp test-suite
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_tree_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_tree_tests.pdf"));

    return dt::run_module(
        tt::tree_spec(),
        "test_tree.hpp unit tests",
        "Default test container: kinds paired with a node forest.",
        opts);
}
