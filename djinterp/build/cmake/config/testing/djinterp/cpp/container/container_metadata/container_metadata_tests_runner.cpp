/*******************************************************************************
* djinterp [test]                            container_metadata_tests_runner.cpp
*
*   The runner for container_metadata.hpp's suite: it selects the suite
* header's spec face (DTEST_SPEC_MODE) and hands the spec to run_module,
* which runs the tests and reports them (docs/djinterp_testing.md, section
* 5). No per-test logic and no formatting live here: change the document
* option to alter the output face, and test_defaults.hpp to alter the format
* itself.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/container_metadata/container_metadata_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header shows its spec provider

// std
#include <string>
// djinterp
#include "container_metadata_tests.hpp"  // via INCLUDES (tests dir)


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
        std::string("container_metadata_tests.pdf"));

    return dt::run_module(
        tt::container_metadata_spec(),
        "container_metadata.hpp unit tests",
        "The metadata key-value store, its detection trait and equality, "
        "and the carrier mixin.",
        opts);
}
