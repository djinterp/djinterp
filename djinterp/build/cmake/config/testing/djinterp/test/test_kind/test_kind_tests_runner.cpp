/******************************************************************************
* djinterp [test]                                  test_kind_tests_runner.cpp
*
*   Entry point for the test_kind.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + test_kind_spec(), no
* fixtures); this TU contributes main() and the seven section TUs the bodies.
* main() only configures options and hands test_kind_spec() to run_module -
* all templating lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_kind_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_kind_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_kind_tests.pdf"));

    return dt::run_module(
        tt::test_kind_spec(),
        "test_kind.hpp unit tests",
        "records, the kind set, resolved queries, and structural detection.",
        opts);
}
