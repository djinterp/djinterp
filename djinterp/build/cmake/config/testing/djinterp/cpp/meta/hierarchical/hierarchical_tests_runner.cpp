#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "hierarchical_tests.hpp"     // relative to the tests dir via INCLUDES

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("hierarchical_tests.pdf"));

    return dt::run_module(
        tt::hierarchical_spec(),
        "hierarchical.hpp unit tests",
        "The HIERARCHICAL structural-kind tag: nests, min_depth, and name().",
        opts);
}
