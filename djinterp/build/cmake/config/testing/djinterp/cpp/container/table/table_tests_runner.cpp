/*******************************************************************************
* djinterp [cpp][container]                               table_tests_runner.cpp
*
* Runner for the table suite (table_tests.hpp): defines DTEST_SPEC_MODE,
* takes the suite's spec and hands it to DTest.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/table/table_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// suite exposes its spec provider, not fixtures
#define DTEST_SPEC_MODE

// std
#include <string>  // std::string
// djinterp
#include "table_tests.hpp"  // relative to the tests dir via INCLUDES

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("table_tests.pdf"));

    return dt::run_module(
        tt::table_spec(),
        "container/table unit tests",
        "The table base; the dynamic, fixed, static, keyed, sorted and "
        "constrained tables; table metadata; and the declaration vocabulary: "
        "shape, layout, builder, options, model, parser and template.",
        opts);
}
