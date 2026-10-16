#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "member_types_tests.hpp"      // relative to the tests dir via INCLUDES

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("member_types_tests.pdf"));

    return dt::run_module(
        tt::member_types_spec(),
        "member_types.hpp unit tests",
        "The shared nested-typedef detectors and the pick_member_type helper.",
        opts);
}
