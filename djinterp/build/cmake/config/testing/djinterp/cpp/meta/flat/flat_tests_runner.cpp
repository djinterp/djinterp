#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "flat_tests.hpp"             // relative to the tests dir via INCLUDES

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("flat_tests.pdf"));

    return dt::run_module(
        tt::flat_spec(),
        "flat.hpp unit tests",
        "The FLAT structural-kind tag: nests, min_depth, and name().",
        opts);
}
