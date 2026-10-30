/******************************************************************************
* djinterp [test]                  option_override_tests_runner.cpp
*
*   Entry point for the option_override.hpp suite.  Defining
* DTEST_SPEC_MODE selects the header's spec-provider face (block-provider
* declarations + option_override_spec(), no fixtures); this TU contributes
* main() and the section TUs the bodies.  main() only configures options
* and hands option_override_spec() to run_module - all templating lives in
* test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/cpp/option/option_override/option_override_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "option_override_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("option_override_tests.pdf"));

    return dt::run_module(
        tt::option_override_spec(),
        "djinterp option_override<> unit tests",
        "engine + merge + lazy SFINAE + policies (C++20)",
        opts);
}
