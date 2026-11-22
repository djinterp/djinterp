/*******************************************************************************
* djinterp [cpp][container]                               array_tests_runner.cpp
*
*   Runner for the array suite (array_tests.hpp): defines DTEST_SPEC_MODE,
* takes the suite's spec and hands it to DTest.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/array/array_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.25
*                                                            revised: 2026.11.14
*******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>  // std::string
// djinterp
#include "array_tests.hpp"  // relative to the tests dir via INCLUDES

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("array_tests.pdf"));

    return dt::run_module(
        tt::array_spec(),
        "array family unit tests",
        "array<Options...>, its options, traits, concepts and iterators, and "
        "the atomic, copy-on-write, RCU and lock-protected wrappers.",
        opts);
}
