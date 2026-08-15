/******************************************************************************
* djinterp [test]               container_conversion_concepts_tests_runner.cpp
*
*   Entry point for the container_conversion_concepts.hpp suite.  Defining
* DTEST_SPEC_MODE selects the header's spec-provider face (declarations +
* container_conversion_concepts_spec(), no fixtures); this TU contributes
* main() and the six section TUs the bodies.  main() only configures options
* and hands container_conversion_concepts_spec() to run_module - all templating
* lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/
*                             container_conversion_concepts_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "container_conversion_concepts_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("container_conversion_concepts_tests.pdf"));

    return dt::run_module(
        tt::container_conversion_concepts_spec(),
        "container_conversion_concepts.hpp unit tests",
        "the four-tier conversion classification, the three readings taken "
        "off it, the two range paths - whose operands are currently "
        "exchanged - and the reinterpretation conjunction.",
        opts);
}
