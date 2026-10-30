// container_metadata_tests_runner.cpp
//   The thin runner for DContainerMetadata.  It defines DTEST_SPEC_MODE so
// the suite header presents its spec provider rather than its fixtures,
// chooses the output options, and hands the spec to run_module.  No per-test
// logic and no formatting live here: change the document option to alter the
// output face, and test_defaults.hpp to alter the format itself.

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "container_metadata_tests.hpp"

int main()
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
        "A non-owning container pointer paired with an owned payload, its "
        "detection trait, and its deducing factory.",
        opts);
}
