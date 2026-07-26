#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "contravariant_tests.hpp"     // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("contravariant_tests.pdf"));

    return dt::run_module(
        tt::contravariant_spec(),
        "contravariant.hpp unit tests",
        "The contravariant functor protocol: contramap, detection, the two "
        "laws, and the C++20 concept.",
        opts);
}
