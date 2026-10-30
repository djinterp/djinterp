// metadata_traits_tests_runner.cpp
//   The thin runner for the metadata protocol traits.  It defines
// DTEST_SPEC_MODE so the suite header presents its spec provider rather than
// its fixtures, chooses the output options, and hands the spec to
// run_module.  No per-test logic and no formatting live here: change the
// document option to alter the output face, and test_defaults.hpp to alter
// the format itself.

#define DTEST_SPEC_MODE

// std
#include <string>
// djinterp
#include "metadata_traits_tests.hpp"

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("metadata_traits_tests.pdf"));

    return dt::run_module(
        tt::metadata_traits_spec(),
        "metadata_traits.hpp unit tests",
        "Structural detection of three canonical metadata names across "
        "three member shapes, and the aliases that extract them.",
        opts);
}
