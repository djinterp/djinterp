/******************************************************************************
* djinterp [tests]                       test_render_pdf_table_tests_runner.cpp
*
*   The runner for the test_render_pdf_table.hpp suite: a thin main() that
* selects the suite header's provider face, builds a test_option_set, and
* hands the spec to run_module.  It formats nothing itself - run_module drives
* the console report and (per the options below) renders the run's own PDF.
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/output/test_render_pdf_table_tests_runner.cpp
*                test_render_pdf_table/test_render_pdf_table_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE   // the suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_render_pdf_table_tests.hpp"   // resolved via the INCLUDES
                                             //   the CMakeLists adds (the
                                             //   tests dir)

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_render_pdf_table_tests.pdf"));

    return dt::run_module(
        tt::render_pdf_table_spec(),
        "test_render_pdf_table.hpp unit tests",
        "the default DTest PDF report renderer, painted onto a recording "
        "backend and checked against its own serialized output",
        opts);
}
