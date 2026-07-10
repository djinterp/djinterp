/******************************************************************************
* djinterp [test]                                    adapter_tests_runner.cpp
*
*   The entry point for the adapter unit-test suite.  Defining DTEST_SPEC_MODE
* selects the suite header's spec-provider face (declarations + adapter_spec,
* no fixture bodies), so this translation unit contributes main() and the
* section TUs contribute the test bodies.  main() renders the module to a PDF
* via run_module.
*
* path:      /build/cmake/config/testing/djinterp/paradigm/adapter/adapter_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.09
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "adapter_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("adapter_tests.pdf"));

    return dt::run_module(
        tt::adapter_spec(),
        "djinterp::paradigm::adapter",
        "Unit tests for the adapter pattern module: ownership policies, "
        "adaptation traits, the object / class / interface adapters, the "
        "function + view adapters, the convenience factories, and the C++20 "
        "concept surface.",
        opts);
}
