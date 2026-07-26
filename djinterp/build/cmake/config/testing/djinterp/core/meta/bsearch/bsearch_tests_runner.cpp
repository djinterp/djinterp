#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "bsearch_tests.hpp"           // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("bsearch_tests.pdf"));

    return dt::run_module(
        tt::bsearch_spec(),
        "bsearch.hpp unit tests",
        "The compile-time binary-search engine: the bounds guard, hits, misses, "
        "needle-agnostic ordering, and behaviour at scale.",
        opts);
}
