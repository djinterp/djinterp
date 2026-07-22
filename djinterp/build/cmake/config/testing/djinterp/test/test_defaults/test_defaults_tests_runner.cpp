/******************************************************************************
* djinterp [test]                              test_defaults_tests_runner.cpp
*
*   Entry point for the test_defaults.hpp suite.  Defining DTEST_SPEC_MODE
* selects the header's spec-provider face (declarations + defaults_spec(), no
* check helpers); this TU contributes main() and the five section TUs the
* bodies.  main() only configures options and hands defaults_spec() to
* run_module - all templating lives in test_defaults.hpp (the very header
* under test).
*
* path:      /build/cmake/config/testing/djinterp/test/test_defaults_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not helpers

// std
#include <string>
// djinterp
#include "test_defaults_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_defaults_tests.pdf"));

    return dt::run_module(
        tt::defaults_spec(),
        "test_defaults.hpp unit tests",
        "Authoring surface: metadata helpers, kinds, factories, aggregates, "
        "and the integration tier.",
        opts);
}
