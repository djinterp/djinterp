/******************************************************************************
* djinterp [parse/test]                                 parse_tests_runner.cpp
*
*   Runner (the main() entry point) for the parse.hpp unit suite.  Per the
* DTest authoring guide a runner is thin: it selects the spec face of the
* suite header, chooses a handful of options, and hands the spec to
* run_module, whose exit code is the whole of main.  It carries no per-test
* logic, no report formatting, and no templating -- run_module inside
* test_defaults.hpp owns all of that.
*
*   The suite exposes its spec through parse_spec() (the shortname is the
* component with no test_ prefix to strip -- the module is `parse`, so the
* provider is parse_spec()).  Defining DTEST_SPEC_MODE before the include
* selects that face; the section translation units, which define the tests_*
* bodies, include the same header WITHOUT the macro.
*
*
* BUILD PREREQUISITE
* ==================
*   parse.hpp does not compile against result.hpp as shipped: it spells the
* base of parse_result as functional::result, but result.hpp declares
* result in ::djinterp and no `functional` namespace exists in the tree.
* The two-token fix is recorded in the suite header's preamble
* (parse_tests.hpp).  This runner needs no change under either repair.
*
*
* path:      /build/cmake/config/testing/djinterp/parse/parse/
*            parse_tests_runner.cpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#define DTEST_SPEC_MODE     // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "parse_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("parse_tests.pdf"));

    return dt::run_module(
        tt::parse_spec(),
        "parse.hpp unit tests",
        "Parsing subframework primitives: the parseable / parse_traits "
        "minor-major mapping, the parse_status code space, the value-"
        "semantic parse_error, the parse_result<T> refinement of "
        "result<T, parse_error>, and the parse_state<E> surface stream.",
        opts);
}
