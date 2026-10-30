/******************************************************************************
* djinterp [testing]                      sequential_container_tests_runner.cpp
*
*   Thin runner for the sequential_container suite: defines DTEST_SPEC_MODE,
* includes the suite header, builds a default option set, and hands the spec to
* run_module.  No per-test logic, no report formatting.
*
* path:  /build/cmake/config/testing/djinterp/cpp/container/sequential_container/sequential_container_tests_runner.cpp
*        sequential_container/sequential_container_tests_runner.cpp
******************************************************************************/

#define DTEST_SPEC_MODE            // spec-provider face, not fixtures

// std
#include <string>
// djinterp (resolved through the INCLUDES the CMakeLists adds)
#include "sequential_container_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    // console (txt) by default so the runner builds without the PDF pipeline.
    // To emit a PDF as well, uncomment the two lines below:
    //   opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    //   opts.set<dt::test_option::output_file>(
    //       std::string("sequential_container_tests.pdf"));

    return dt::run_module(
        tt::sequential_container_spec(),
        "sequential_container.hpp unit tests",
        "The legacy order-preserving base: verdict, kind, base, algorithms",
        opts);
}
