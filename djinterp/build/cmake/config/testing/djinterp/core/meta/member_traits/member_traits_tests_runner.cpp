#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "member_traits_tests.hpp"     // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("member_traits_tests.pdf"));

    return dt::run_module(
        tt::member_traits_spec(),
        "member_traits.hpp unit tests",
        "The deprecated transitional shim: the two forwarded macro spellings "
        "and the transitive re-export of member_types.hpp.",
        opts);
}
