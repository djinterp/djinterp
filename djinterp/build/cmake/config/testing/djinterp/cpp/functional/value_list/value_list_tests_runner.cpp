/*******************************************************************************
* djinterp [test]                                    value_list_tests_runner.cpp
*
*   The runner for value_list.hpp's suite: it selects the suite header's spec
* face (DTEST_SPEC_MODE) and hands the spec to run_module, which runs the
* tests and reports them (docs/djinterp_testing.md, section 5).
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/functional/value_list/value_list_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.11.09
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header shows its spec provider

// std
#include <string>
// djinterp
#include "value_list_tests.hpp"  // via INCLUDES (tests dir)


/*
main
  The runner: the default options, a PDF report, and the module.
*/
int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("value_list_tests.pdf"));

    return dt::run_module(
        tt::value_list_spec(),
        "value_list.hpp unit tests",
        "DValueList - the NTTP sequence (value-domain counterpart of the type "
        "list): dtuple.hpp treats...",
        opts);
}
