#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "kv_pair_tests.hpp"          // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("kv_pair_tests.pdf"));

    return dt::run_module(
        tt::kv_pair_spec(),
        "kv_pair.hpp unit tests",
        "The minimal key-value pair: construction, make_kv, and key-only "
        "comparison.",
        opts);
}
