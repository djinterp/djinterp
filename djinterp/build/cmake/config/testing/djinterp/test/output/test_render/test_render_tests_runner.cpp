/******************************************************************************
* djinterp [test]                                 test_render_tests_runner.cpp
*
*   Entry point for the test_render.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + render_spec(), no fixtures);
* this TU contributes main() and the five section TUs the bodies.  main() only
* configures options and hands render_spec() to run_module - all templating
* lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/output/test_render_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp   (suite header lives under tests/djinterp/test/output/)
#include "test_render_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_render_tests.pdf"));

    return dt::run_module(
        tt::render_spec(),
        "test_render.hpp unit tests",
        "The report renderer: projections, resolver, sinks, the walk, and "
        "the default layouts.",
        opts);
}
