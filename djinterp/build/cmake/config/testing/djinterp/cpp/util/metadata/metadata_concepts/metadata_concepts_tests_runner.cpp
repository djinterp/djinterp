// metadata_concepts_tests_runner.cpp
//   The thin runner for the C++20 metadata concepts.  It defines
// DTEST_SPEC_MODE so the suite header presents its spec provider rather than
// its fixtures, chooses the output options, and hands the spec to
// run_module.  No per-test logic and no formatting live here: change the
// document option to alter the output face, and test_defaults.hpp to alter
// the format itself.

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "metadata_concepts_tests.hpp"

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("metadata_concepts_tests.pdf"));

    return dt::run_module(
        tt::metadata_concepts_spec(),
        "metadata_concepts.hpp unit tests",
        "Concept restatement of the metadata traits, checked case-for-case "
        "against them and exercised as real constraints.",
        opts);
}
