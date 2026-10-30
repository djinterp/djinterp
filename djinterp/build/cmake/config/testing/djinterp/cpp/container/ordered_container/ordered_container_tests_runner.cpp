/******************************************************************************
* djinterp [testing]                         ordered_container_tests_runner.cpp
*
*   Thin runner for the ordered_container suite.  It defines DTEST_SPEC_MODE so
* the suite header presents its spec provider, builds a default option set, and
* hands the spec to run_module.  It contains no per-test logic and no report
* formatting - swapping the output face is a one-line options change.
*
* path:  /build/cmake/config/testing/djinterp/cpp/container/ordered_container/ordered_container_tests_runner.cpp
*        ordered_container_tests_runner.cpp
******************************************************************************/

#define DTEST_SPEC_MODE            // spec-provider face, not fixtures

// std
#include <string>
// djinterp (resolved through the INCLUDES the CMakeLists adds)
#include "ordered_container_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    // console (txt) by default so the runner builds without the PDF pipeline.
    // To emit a PDF as well, uncomment the two lines below:
    //   opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    //   opts.set<dt::test_option::output_file>(
    //       std::string("ordered_container_tests.pdf"));

    return dt::run_module(
        tt::ordered_container_spec(),
        "ordered_container.hpp unit tests",
        "The ORDER axis: positional identity, ordered_base, order algorithms",
        opts);
}
