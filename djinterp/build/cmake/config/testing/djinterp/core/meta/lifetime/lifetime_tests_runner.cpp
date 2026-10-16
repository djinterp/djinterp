#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "lifetime_tests.hpp"         // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("lifetime_tests.pdf"));

    return dt::run_module(
        tt::lifetime_spec(),
        "lifetime.hpp unit tests",
        "The lifetime lattice, its algebra, the literal-type probe, opt-in "
        "detection, the lifetime_of classifier, and the derived predicates.",
        opts);
}
