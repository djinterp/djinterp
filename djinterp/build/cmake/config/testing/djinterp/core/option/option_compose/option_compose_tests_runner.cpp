/******************************************************************************
* djinterp [test]                             option_compose_tests_runner.cpp
*
*   Entry point for the option_compose.hpp unit suite.  Assembles the four
* section blocks - each contributed by its own translation unit through the
* block-provider declared in option_compose_tests.hpp - into one module_spec
* and hands it to the framework runner.
*
*   Sections:
*     I.   defopt              (option_compose_tests_defopt.cpp)
*     II.  with_option / _as   (option_compose_tests_with_option.cpp)
*     III. with_options / _as  (option_compose_tests_with_options.cpp)
*     IV.  compose_options/_as (option_compose_tests_compose.cpp)
*
*   PDF OUTPUT:
*   The report is configured through a test_option_set (document = pdf,
* output_file = "option_compose_tests.pdf"); with the default whole_run split
* and no packaging that renders one loose PDF for the suite.
*
*   option_compose.hpp is C++20-only (its folds are OverridePolicy-constrained),
* so below C++20 concepts every section is emitted empty and this runner reports
* a clean 0/0 rather than failing to build.  run_module returns 0 iff every
* executed test passed.
*
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_compose_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <string>
// djinterp
#include "option_compose_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_compose.hpp";
    mod.descriptor = "defopt surface, define-and-add, multi-surface folds, and compose-from-empty";

    mod.blocks.push_back(tt::option_compose_defopt_block());        // I.
    mod.blocks.push_back(tt::option_compose_with_option_block());   // II.
    mod.blocks.push_back(tt::option_compose_with_options_block());  // III.
    mod.blocks.push_back(tt::option_compose_compose_block());       // IV.

    // configure the report as a single PDF (whole_run + no packaging).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("option_compose_tests.pdf"));

    return dt::run_module(
        mod,
        "djinterp option_compose unit tests",
        "defopt + with_option(s) + compose_options",
        opts);
}
