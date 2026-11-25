/*******************************************************************************
* djinterp [cpp][meta]                          math_vocabulary_tests_runner.cpp
*
*   The runner for math_vocabulary_tests.hpp: its spec, run as one module.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/meta/math_vocabulary/math_vocabulary_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite's spec provider, not its fixtures

// std
#include <string>  // std::string
// djinterp
#include "math_vocabulary_tests.hpp"  // the suite, through INCLUDES


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("math_vocabulary_tests.pdf"));

    return dt::run_module(
        tt::math_vocabulary_spec(),
        "math_vocabulary.hpp unit tests",
        "The math vocabulary's values, its traits, and its old names in "
        "djinterp::math.",
        opts);
}
