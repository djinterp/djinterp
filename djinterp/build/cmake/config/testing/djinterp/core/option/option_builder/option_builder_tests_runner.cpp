/******************************************************************************
* djinterp [test]                              option_builder_tests_runner.cpp
*
*   Entry point for the option_builder.hpp unit suite.  Assembles the three
* section blocks - each contributed by its own translation unit through the
* block-provider declared in option_builder_tests.hpp - into one module_spec
* and hands it to the framework runner.
*
*   Sections:
*     I.   option_partition_wrap    (option_builder_tests_wrap.cpp)
*     II.  tuple_to_option_set      (option_builder_tests_lift.cpp)
*     III. option_set_from_flat_t   (option_builder_tests_pipeline.cpp)
*
*   PDF OUTPUT:
*   This runner configures the report through a test_option_set (test_options.hpp)
* rather than the bare console overload: document = pdf and output_file =
* "option_builder_tests.pdf".  With the default whole_run split and no
* packaging, that renders one loose PDF for the suite.  The options-driven
* run_module overload seeds the report_builder from the option set (it does not
* call use_pdf()); run_module still returns 0 iff every test passed, which
* becomes the process exit status for CI.
*
*   option_builder.hpp is type-level and compiles at the framework's base
* standard, so this suite has no version gating.
*
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_builder_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <string>
// djinterp
#include "option_builder_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_builder.hpp";
    mod.descriptor = "chunk wrapper, tuple-to-set lift, and the flat-schema pipeline";

    mod.blocks.push_back(tt::option_builder_wrap_block());       // I.
    mod.blocks.push_back(tt::option_builder_lift_block());       // II.
    mod.blocks.push_back(tt::option_builder_pipeline_block());   // III.

    // configure the report as a single PDF (whole_run + no packaging -> one
    // loose file named below).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("option_builder_tests.pdf"));

    return dt::run_module(
        mod,
        "djinterp option_builder unit tests",
        "chunk wrapper + tuple lift + flat-schema pipeline",
        opts);
}
