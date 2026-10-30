/******************************************************************************
* djinterp [test]                           decode_tests_runner.cpp
*
*   Entry point for the decode.hpp suite.  Defining DTEST_SPEC_MODE
* selects the header's spec-provider face (declarations + decode_spec(),
* no fixtures); this TU contributes main() and the section TUs the bodies.
* main() only configures options and hands decode_spec() to run_module -
* all templating lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/cpp/binary/decode_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "decode_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("decode_tests.pdf"));

    return dt::run_module(
        tt::decode_spec(),
        "decode.hpp unit tests",
        "the fixed big-endian leaf decoder: reader, result, leaves, round trip",
        opts);
}
